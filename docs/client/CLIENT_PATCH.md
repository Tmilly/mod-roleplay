# Client patch deployment

The canonical patch source is `client/source/ironbelly.json`; `client/tools/dbc_tool.py` clones verified Blizzard
rows and rejects ID collisions. Build output contains complete replacement DBCs, not partial row fragments.

Both worldserver and client require the same generated `Spell.dbc`, `SkillLineAbility.dbc`, and
`TotemCategory.dbc`. The client also needs `Item.dbc`. Deploy all four to server `DataDir/dbc`, then package the
same files at `DBFilesClient/*` in `patch-R.MPQ`. Never deploy only the MPQ: server/client disagreement can hide
recipes, reject casts, or crash the client.

After copying `patch-R.MPQ` to `<WoW>\Data`, delete `<WoW>\Cache\WDB\enUS`, launch the client, add item `901000`,
and verify its pan model/icon and the eight Cooking rows. Future systems extend the JSON/source and this same MPQ.

