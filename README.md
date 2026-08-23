# mod-roleplay

Custom roleplay features for AzerothCore WotLK.

## Shadow Wand

The module provides a unique, non-equippable cursed relic for Lyon:

- Item entry: `900000`
- Name: `Shadow Wand`
- Acquisition: `.additem 900000`

The item deliberately has no native stats or item spells. Roleplay mechanics are implemented by the module while
the relic is carried in inventory, allowing those mechanics to evolve independently through future progression.

While Lyon carries the relic, the default mechanics are:

- **Dark Aegis:** reduces incoming damage by 3%.
- **Death's Reprisal:** has a 5% chance after a damaging hit to retaliate with level-scaled Shadow damage, limited
  by a one-second internal cooldown.
- **Refusal of Death:** prevents an otherwise lethal hit, leaves Lyon at 1 health, and triggers Icebound Fortitude.
  Its default cooldown is ten minutes.

All mechanics and tuning values can be changed in `Roleplay.conf`.

## Installation

Place this directory at `modules/mod-roleplay` in an AzerothCore checkout, then configure and build the core as
usual. AzerothCore discovers the source files and distributed configuration automatically.

After installation, edit `Roleplay.conf` in the server's modules configuration directory to enable or disable
the module.

## Development

- Add C++ scripts under `src/`.
- Register each script from `AddRoleplayScripts()` in `src/roleplay.cpp`.
- Add database migrations only under the matching `data/sql/updates/pending_db_*` directory.
