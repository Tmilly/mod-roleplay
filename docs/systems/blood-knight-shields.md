# Blood Knight shield support

Shield support uses `BloodKnight::IsBloodKnight`: Roleplay and Blood Knight must be enabled, the character
must be a Death Knight, and the configured GUID takes precedence over the configured name fallback.
There is no race or level gate, so level-1 characters and existing level-5 characters qualify, including
after an Undead-to-Human race change. No new configuration setting, SQL migration, manual training,
client DBC/MPQ patch, damage/progression change, or dual-wield change is needed.

`EnsureShieldSupport` learns missing Shield (9116) and Block (107), repairs missing `SKILL_SHIELD` (433)
to its normal 1/1 range, and restores the runtime armor-proficiency mask and `CanBlock` capability if absent.
It runs before the mana-only return in `Refresh` (login, first login and level changes), never on update
ticks. Existing spells/skills are not repeatedly taught/reset. Normal character saving persists the state;
initialization also repairs it after server restart.

## Evidence from the installed core and data

Inspected core revision: `efe123fab543c5faf3c477674ec17a18fd59f09f`.
The installed server's `Spell.dbc` identifies 9116 as Shield, armor subclass mask 64, effect 60
(`SPELL_EFFECT_PROFICIENCY`); 107 is Block, effect 23 (`SPELL_EFFECT_BLOCK`). `Item::GetSpell()` maps
shields to 9116; `ItemTemplate::GetSkill()` maps them to `SKILL_SHIELD`, defined as 433.
`Spell::EffectProficiency` sends the armor proficiency mask to the client; `Spell::EffectBlock` sets
`CanBlock`. The repair uses those same supported Player APIs when already-known spells have not restored
runtime state.

`Player::CanUseItem(Item*)` requires a nonzero item skill. A proficiency spell in the spellbook therefore
does not by itself prove that a shield can be equipped. The installed `SkillRaceClassInfo.dbc` shield row
has class mask 67 (warrior/paladin/shaman), excluding DK. `_LoadSkills` discards that skill on relog; optional
`CheckSkillLearnedBySpell` validation can remove its spell as well. Login callbacks occur after inventory
loading. The supported `OnPlayerCheckItemInSlotAtLoadInventory` hook repairs the state before the normal
`CanEquipItem` call, then returns true to retain that call. This prevents equipped shields being rejected
at load merely because the DBC skill was discarded. The core may still log that discard once per login;
this module does not alter global DBC tables or suppress validation logs.

Normal armor/stat application remains in core equipment code. `Unit::GetUnitBlockChance` requires
`CanBlock` and a usable, unbroken off-hand item with block value. `Player::UpdateBlockPercentage` supplies
the normal base chance and defense/rating adjustments; the module adds no block bonus. Level requirements,
class/race-exclusive items, required spells/skills, slot checks and two-handed/off-hand restrictions remain
in `CanUseItem`/`CanEquipItem`. No Titan's Grip capability is granted. The custom DK progression cleanup
only removes its enumerated level unlocks and plate proficiency; it does not remove 9116, 107 or skill 433.

A red tooltip alone is not proof of server rejection: record the exact inventory error and inspect skill,
proficiency, item class mask and required level separately. This change supplies stock client proficiency
state but cannot promise every client tooltip treats a nonstandard class identically. LFG need-roll logic
also has a separate class-based shield check; changing loot eligibility is outside this equipment change.
No server core patch is required for the equipment hooks present in this checkout. On another fork lacking
the pre-equipment-load hook, a login-only fallback is insufficient: a server hook after skill/spell load
and before `CanEquipItem` during `_LoadInventory` would be the smallest separate change to review.

## Verification

`tests/run-focused-tests.ps1` compiles the actual identity/initialization functions with a Player test double
and tests GUID priority, name fallback, feature/class exclusion, missing-state repair, idempotence and
preservation of learned dual wield. This does not simulate core item loading or combat. `-CompileModule`
also compiles the four changed production translation units using the existing core build's include paths,
without linking or installing. The existing generated project is used only to obtain those include paths.

MSVC compilation of all four translation units and all three regression executables passed locally.
No full worldserver link or in-game test was performed. The repository Python codestyle command was attempted
but blocked because the available `python.exe` could not execute. `git diff --check` is checked separately.

Use a staging server with a copy of the intended configuration/character, not production, for these checks:

- Log in the existing level-5 Aldric without GM training. Confirm spells 9116/107, skill 433 at 1/1, shield
  proficiency and block capability; also verify a newly created level-1 targeted DK.
- Equip, unequip and re-equip a normal shield with a proficient one-handed weapon. Compare armor and any
  shield stats before/after; verify they return to baseline when unequipped.
- Fight a suitable melee attacker while facing it with an unbroken shield. Observe actual combat-log blocks
  and damage reduction. Remove/break the shield and verify normal blocking stops; spellbook state is not enough.
- Relog with the shield equipped, level up, then restart the staging server and relog. Confirm the shield
  stays equipped (not mailed), retains stats and still blocks. Repeat with Blood Knight mana disabled.
- Verify configured GUID priority over a mismatching name, name fallback with GUID zero, race change to
  Human, and no new support for an unrelated DK or a character outside the enabled/class targeting rules.
- Verify over-level and class-exclusive shields remain rejected and a two-handed weapon cannot coexist
  with the shield. Confirm existing dual wield still behaves according to existing progression.

For deployment after review, merge/pull the module changes, rebuild and install the compatible server/modules
using the server's normal build procedure, restart during a maintenance window, and relog Aldric. No new
SQL/configuration/training step is required when his existing Blood Knight targeting is correct. No live
server restart, production configuration edit, database mutation or real card edit was performed here.
