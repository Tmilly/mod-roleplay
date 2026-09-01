# Playerbot gender and appearance customization

These player-level commands operate only on currently loaded Playerbots. A bot may be selected, or its online
name may be supplied explicitly. There are intentionally no ownership, account, or companion-registration checks.

## Commands

Selected bot:

```text
.rp bot gender male
.rp bot gender female
.rp bot appearance random
.rp bot appearance skin <id>
.rp bot appearance face <id>
.rp bot appearance hair <id>
.rp bot appearance haircolor <id>
.rp bot appearance facialhair <id>
```

Named loaded bot:

```text
.rp bot gender Dorrin male
.rp bot appearance Dorrin random
.rp bot appearance Dorrin skin 2
.rp bot appearance Dorrin face 3
.rp bot appearance Dorrin hair 5
.rp bot appearance Dorrin haircolor 1
.rp bot appearance Dorrin facialhair 4
```

`CharSections.dbc` is authoritative. Face and skin form a validated pair; hair style and color form another pair.
Facial hair is validated for the current race and gender. Randomization selects complete combinations from the
loaded rows rather than generating arbitrary bytes. Gender changes preserve valid current pairs and replace only
values invalid for the new gender.

Changes update the live Player fields, reinitialize the native display model, update the character cache, and call
`Player::SaveToDB`. No bot logout is required, and changes survive bot reload and worldserver restart.

## Race limitation

`.rp bot race` is not implemented. The installed core exposes full race/faction conversion only inside
`WorldSession::HandleCharFactionOrRaceChangeCallback`, an offline character-service workflow. It also converts
languages, racial spells, factions, taxi paths, homebind, quests, reputation, items, achievements, guild/social
state, appearance, and cache data. A raw `characters.race` update would be unsafe. Race conversion is deferred
until that workflow can be exposed as a reusable core service.

Known limitations: named bots must currently be loaded, and the command does not list every valid DBC option.
Invalid combinations are rejected with an explanatory error.
