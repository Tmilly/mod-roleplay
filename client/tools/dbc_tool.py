#!/usr/bin/env python3
"""Small, dependency-free WDBC inspector and row-cloning patcher for mod-roleplay."""

import argparse
import csv
import json
import struct
from pathlib import Path


class Dbc:
    def __init__(self, path: Path):
        self.path = path
        data = path.read_bytes()
        magic, self.count, self.fields, self.record_size, string_size = struct.unpack_from("<4s4I", data)
        if magic != b"WDBC" or self.record_size != self.fields * 4:
            raise ValueError(f"Unsupported DBC layout: {path}")
        start = 20
        self.rows = [list(struct.unpack_from(f"<{self.fields}I", data, start + i * self.record_size))
                     for i in range(self.count)]
        strings_start = start + self.count * self.record_size
        self.strings = data[strings_start:strings_start + string_size]

    def add_string(self, value: str) -> int:
        encoded = value.encode("utf-8") + b"\0"
        offset = len(self.strings)
        self.strings += encoded
        return offset

    def clone(self, source_id: int, new_id: int):
        source = next((row for row in self.rows if row[0] == source_id), None)
        if source is None:
            raise ValueError(f"Missing source row {source_id} in {self.path.name}")
        if any(row[0] == new_id for row in self.rows):
            raise ValueError(f"ID collision {new_id} in {self.path.name}")
        row = source.copy()
        row[0] = new_id
        self.rows.append(row)
        return row

    def write(self, path: Path):
        path.parent.mkdir(parents=True, exist_ok=True)
        header = struct.pack("<4s4I", b"WDBC", len(self.rows), self.fields, self.record_size, len(self.strings))
        records = b"".join(struct.pack(f"<{self.fields}I", *row) for row in self.rows)
        path.write_bytes(header + records + self.strings)

    def text(self, offset: int) -> str:
        if not offset or offset >= len(self.strings):
            return ""
        end = self.strings.find(b"\0", offset)
        return self.strings[offset:end].decode("utf-8", errors="replace")


def inspect(args):
    dbc = Dbc(Path(args.dbc))
    wanted = {int(value) for value in args.ids.split(",")}
    for row in dbc.rows:
        if row[args.match_field] in wanted:
            values = {"id": row[0], "fields": dbc.fields}
            for field in args.string_field:
                values[f"string_{field}"] = dbc.text(row[field])
            values["row"] = row
            print(json.dumps(values))


def inspect_items(args):
    wanted = {int(value) for value in args.ids.split(",")}
    with Path(args.sql).open(encoding="utf-8") as source:
        for line in source:
            if not line.startswith("("):
                continue
            try:
                row = next(csv.reader([line[1:].rstrip("\n,;)")], quotechar="'", escapechar="\\"))
                if int(row[0]) in wanted:
                    print(json.dumps({"id": int(row[0]), "row": row}))
            except (ValueError, csv.Error):
                continue


def build(args):
    source_dir = Path(args.source)
    output_dir = Path(args.output)
    manifest = json.loads(Path(args.manifest).read_text(encoding="utf-8"))

    item_dbc = Dbc(source_dir / "Item.dbc")
    for definition in manifest["items"]:
        row = item_dbc.clone(definition["clone"], definition["id"])
        row[1] = definition["class"]
        row[2] = definition["subclass"]
        row[4] = definition["material"] & 0xFFFFFFFF
        row[5] = definition["display_id"]
        row[6] = definition["inventory_type"]
        row[7] = definition["sheathe_type"]
    item_dbc.write(output_dir / "Item.dbc")

    spell_dbc = Dbc(source_dir / "Spell.dbc")
    for definition in manifest["recipes"]:
        row = spell_dbc.clone(definition["clone_spell"], definition["spell_id"])
        row[52:60] = definition["reagents"] + [0] * (8 - len(definition["reagents"]))
        row[60:68] = definition["counts"] + [0] * (8 - len(definition["counts"]))
        row[107] = definition["item_id"]
        row[222] = manifest["totem_category"]["id"]
        row[136] = spell_dbc.add_string(definition["name"])
        for index in range(137, 152):
            row[index] = 0
    spell_dbc.write(output_dir / "Spell.dbc")

    skill_dbc = Dbc(source_dir / "SkillLineAbility.dbc")
    for definition in manifest["recipes"]:
        row = skill_dbc.clone(definition["clone_skill_line"], definition["skill_line_id"])
        row[1] = 185
        row[2] = definition["spell_id"]
        row[7] = definition["skill"]
        row[8] = 0
        row[9] = 0
        row[10] = definition["gray"]
        row[11] = definition["green"]
    skill_dbc.write(output_dir / "SkillLineAbility.dbc")

    totem_dbc = Dbc(source_dir / "TotemCategory.dbc")
    category = manifest["totem_category"]
    row = totem_dbc.clone(category["clone"], category["id"])
    row[1] = totem_dbc.add_string(category["name"])
    for index in range(2, 17):
        row[index] = 0
    row[18] = category["type"]
    row[19] = category["mask"]
    totem_dbc.write(output_dir / "TotemCategory.dbc")
    print(f"Built synchronized DBC files in {output_dir}")


def main():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(required=True)
    inspect_parser = subparsers.add_parser("inspect")
    inspect_parser.add_argument("dbc")
    inspect_parser.add_argument("ids")
    inspect_parser.add_argument("--string-field", type=int, action="append", default=[])
    inspect_parser.add_argument("--match-field", type=int, default=0)
    inspect_parser.set_defaults(func=inspect)
    item_parser = subparsers.add_parser("inspect-items")
    item_parser.add_argument("sql")
    item_parser.add_argument("ids")
    item_parser.set_defaults(func=inspect_items)
    build_parser = subparsers.add_parser("build")
    build_parser.add_argument("source")
    build_parser.add_argument("output")
    build_parser.add_argument("manifest")
    build_parser.set_defaults(func=build)
    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
