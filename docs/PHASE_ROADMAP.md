# Phase roadmap

## Existing

- Shadow Wand: carried relic, damage reduction, retaliation, lethal-damage refusal and RP feedback.
- Sparse Playerbot outfits: transmog-derived delta sets with persistent bot assignment.
- Mortal recruits: persistent owner identity, follow/stay, combat, dismissal and permanent death.
- Playerbot appearance: DBC-validated gender and appearance editing for loaded bots.
- PBC sheets: safe in-game creation and editing of PBC's canonical freeform character cards.
- Recruit template discovery: target inspection and create-from-target convenience.

## Ironbelly Phase 1

The Ironbelly Skillet is how Dorrin cooks: a tradable Bind-on-Equip held-offhand item and reusable required tool.
Eight real Cooking recipes are granted with `.rp ironbelly learn`. Standard crafting handles reagent consumption,
output storage safety, profession display, skill-ups, and tool validation.

## Ironbelly Phase 2 (not implemented)

Dorrin's Cookbook will represent what Dorrin has learned. Candidate work includes persistent recipe discovery,
exploration and unusual-creature discoveries, first-time discoveries where appropriate, and rare recipes. The
Cookbook must not replace the physical skillet tool.

## Later candidates (not implemented)

Boss ingredients, rare recipes, creature-family discoveries, a traveling dwarven field kitchen, RP culinary
achievements/titles, unusual harmless food reactions, more food icons/models, recruit stationing and patrols,
RP props and richer recruit identity/history.

## Playerbot race conversion (deferred)

This AzerothCore revision keeps complete race/faction conversion inside the character-service packet handler and
does not expose it as a reusable offline-character API. A future phase may extract that workflow into a supported
core service so Playerbots receive all language, racial spell, faction, taxi, homebind, quest, reputation, item,
achievement, guild/social, appearance, and cache conversions. A partial SQL race update is explicitly forbidden.
