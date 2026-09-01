# mod-roleplay client patch

This directory is the reproducible source for client-facing `mod-roleplay` data. Phase 1 modifies `Item.dbc`,
`Spell.dbc`, `SkillLineAbility.dbc`, and `TotemCategory.dbc`; it reuses Blizzard displays and icons.

Build synchronized DBCs:

```bat
python client\tools\dbc_tool.py build C:\Gaming\AzerothCore\Data\dbc client\build\DBFilesClient client\source\ironbelly.json
```

Then run `client\build_patch.cmd`. It expects Ladik's `MPQEditor.exe` at the path shown in the script and creates
`client\build\patch-R.MPQ`. Copy that MPQ to the client's `Data` directory. Copy the four generated DBC files to
the server's configured `DataDir\dbc` directory as well. Client and server files must come from the same build.

`patch-R.MPQ` was selected after inspecting the local client: no patch-R existed, while many other lettered
patches did. WotLK loads lettered patch archives in lexical order; remove `Cache\WDB\enUS` after changing items.

