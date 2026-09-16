# Custom ID registry

| ID/range | Type | Name | System | Location | Status | Notes |
|---|---|---|---|---|---|---|
| 900000 | Item | Shadow Wand | Lyon | Server + client item cache | Active | Display 18356 |
| 900001 | Spell | Death's Reprisal | Lyon | Server DBC/SQL | Active | Authoritative Shadow damage |
| 901000 | Item | The Ironbelly Skillet | Ironbelly | Server + client | Active | Display 28866; TotemCategory 901 |
| 901001-901008 | Items | Phase 1 dishes | Ironbelly | Server + client | Active | See client manifest |
| 901100-901107 | Spells | Phase 1 Cooking recipes | Ironbelly | Server + client | Active | Native CREATE_ITEM recipes |
| 901100-901107 | SkillLineAbility rows | Phase 1 Cooking recipes | Ironbelly | Server + client | Active | Cooking skill line 185 |
| 901 | TotemCategory | The Ironbelly Skillet | Ironbelly | Server + client | Active | Private type 901/mask 1 |
| 900120 | Creature template | Runeblade Mentor | Death Knight progression | Server | Retired | Removed by rev_20260916010000; keep ID reserved |
| `LyonShadowPlayerScript` | Script name | Login feedback | Lyon | Server | Active | Personal message |
| `LyonShadowUnitScript` | Script name | Relic combat mechanics | Lyon | Server | Active | Damage hook |
| `RoleplayPhaseOne*` | Script names | Recruits/outfits | RP Phase 1 | Server | Active | Player, Unit, Command scripts |
| `IronbellyFoodItemScript` | Script name | Harmless food reactions | Ironbelly | Server | Active | Items 901005/901008 |

There are no custom gameobject template IDs in the module. Recruit IDs and outfit IDs are database
auto-increment identities, not reserved static content IDs.
