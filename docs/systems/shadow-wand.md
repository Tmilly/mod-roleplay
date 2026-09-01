# Lyon's Shadow Wand

## Current implementation

Item `900000` is a non-equippable (`InventoryType 0`) miscellaneous (`class 15`, `subclass 0`) unique carried
relic using display `18356`, quality rare, no binding, and flavor text “Cold to the touch. Something within refuses
to let go.” SQL is in `data/sql/db-world/updates/rev_20260823150000.sql`.

Only the configured character name (default `Lyon`) qualifies, and only while carrying the item in non-bank
inventory. Dark Aegis directly reduces incoming nonzero damage by 3%. Death's Reprisal rolls 5% per damaging hit,
uses spell `900001`, deals `20 + 2 * level` Shadow damage, and has a 1-second internal cooldown. Its optional
visual spell defaults to `73295`; the authoritative damage remains `900001`. Triggered casting has no mana cost,
GCD, or learned-spell requirement; optional mana audit logging is disabled by default.

Refusal of Death has a 100% roll on otherwise lethal damage, reduces that hit to leave 1 HP, casts `48792`
(Icebound Fortitude), and applies a 600-second cooldown keyed to that spell. Login and refusal messages are
personal system messages. Configuration is in `Roleplay.conf.dist`; implementation is `lyon_shadow.cpp`.

## Older/removed experiments

Spell `686` (Shadow Bolt) was previously used for reprisal and appeared to alter mana. It is no longer the damage
spell. “Ice Fortitude” is historical wording; the active spell is Blizzard's Icebound Fortitude `48792`.

## Testing and future work

Use `.additem 900000` on Lyon, then take damage. Future progression may unlock stronger chapter-specific effects;
the current implementation has no progression database and effects do not stack from multiple relic copies.

