# mod-roleplay

Custom roleplay features for AzerothCore WotLK.

Full system, ID, roadmap, and client-patch documentation begins at [`docs/README.md`](docs/README.md).

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
  by a one-second internal cooldown. It uses the zero-cost server spell `900001` and does not require a learned
  player spell.
- **Refusal of Death:** prevents an otherwise lethal hit, leaves Lyon at 1 health, and triggers Icebound Fortitude.
  Its default cooldown is ten minutes.

All mechanics and tuning values can be changed in `Roleplay.conf`.

## Persistent RP recruits

The player-level `.rp recruit` commands create lightweight, mortal temporary creatures backed by records in the
character database. They are reconstructed when their owner logs in, use their creature template's normal combat
AI, and can follow or hold position. Death sets a permanent database state before the corpse expires, so dead
recruits are never reconstructed.

Players may use any valid `creature_template` entry and there is no module-enforced recruit count limit. Every
follow, stay, info, and dismiss operation still validates ownership, and creation remains disabled in instances.

Commands:

- `.rp recruit create <creatureEntry> [name]`
- `.rp recruit create-target <name>`
- `.rp recruit template [creatureEntry]`
- `.rp recruit follow`
- `.rp recruit stay`
- `.rp recruit info`
- `.rp recruit dismiss`

The optional RP name is persistent and appears in `.rp recruit info`. The unmodified 3.3.5 client obtains an
overhead creature name from `creature_template`, so Phase 1 intentionally does not rename the shared template.

## Sparse Playerbot outfits

Outfit editing records a baseline, lets the player use the existing mod-transmog NPC, then persists only armor
slots whose effective item appearance changed. Weapon slots are excluded. Applying an outfit calls mod-transmog's
fake-entry API and never replaces equipment or changes stats. Persistent assignments are reapplied when a bot logs
in and only for the equipment slot that changes when the bot equips an upgrade.

Commands:

- `.rp outfit begin "name"`
- `.rp outfit save`
- `.rp outfit cancel`
- `.rp outfit list`
- `.rp outfit info "name"`
- `.rp outfit apply "name"`
- `.rp outfit assign "name"`
- `.rp outfit clear`
- `.rp outfit delete "name"`

All commands are available at player security level. Outfit definitions are shared for use, but only their creator
may overwrite or delete them. `clear` and `delete` remove persistent associations without destructively restoring
current transmog visuals.

## Playerbot customization and PBC cards

Player-level `.rp bot gender` and `.rp bot appearance` commands edit currently loaded Playerbots. Appearance IDs
are checked against `CharSections.dbc`, including the coupled face/skin and hair-style/color combinations, then
saved through the live Player object. `.rp bot sheet` manages PBC's canonical `<Name>.card.txt` files under its
configured `PBC.CharacterCardsPath`; destructive operations create a `.bak` copy. See the system documentation
for syntax and limitations. Playerbot race conversion remains deferred because this core has no safe module API.

## Installation

Place this directory at `modules/mod-roleplay` in an AzerothCore checkout, then configure and build the core as
usual. AzerothCore discovers the source files and distributed configuration automatically.

After installation, edit `Roleplay.conf` in the server's modules configuration directory to enable or disable
the module.

## Development

- Add C++ scripts under `src/`.
- Register each script from `AddRoleplayScripts()` in `src/roleplay.cpp`.
- Add module database migrations under the matching `data/sql/db-*/updates` directory.
