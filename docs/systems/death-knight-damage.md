# Managed low-level Death Knight damage

Only `DeathKnight::IsManaged()` player casters receive this policy. It requires the module to be enabled,
DK class, and persisted enrollment state. Blood Knight identity alone does not enroll a character.
Ordinary DKs, other classes, pets, white swings, racial abilities, unrelated item procs, environmental
damage and Warlock/Blood Knight spells are outside the policy. No rune, runic-power, healing, talent,
equipment, campaign, taxi or creation rules were changed.

## Audit findings before implementation

Audited core: `efe123fab543c5faf3c477674ec17a18fd59f09f`, using its actual headers, spell scripts,
installed server `Spell.dbc`, and repository `spell_bonus_data`/`spell_ranks`. No live DB was queried.

The suspected weapon-strike bypass is **not present** in this fork. The previous scaler already scaled
the complete ordinary DK strike. Adding `ModifyMeleeDamage` would nerf white attacks and would not repair
a missing strike path. The actual paths are:

| Damage | Scaling point |
| --- | --- |
| Icy Touch | Direct hook once after bonuses |
| Plague/Blood/Death Strike, Obliterate, Rune Strike | Direct hook once for the complete special attack |
| Death Coil | Triggered 47632 direct hit once; no damage in parent dummy |
| Blood Plague/Frost Fever | Periodic hook once per tick, no direct hook |
| Death and Decay | Triggered 52212 direct hit once, no parent periodic-damage hook |
| Blood-Caked Strike | Direct hook once on its new weapon calculation |
| Necrosis | Direct hook once on 51460, derived from unscaled white damage |
| Unholy Blight | Previously twice; now inherits Death Coil's scaling without a second multiplier |
| Wandering Plague | Inherits disease scaling; GENERIC family excludes a second multiplier |
| White swings | No module hook registered for this route |
| Periodic healing | Damage-aura classification excludes it |

`EffectSchoolDMG` includes caster/target bonuses before `Spell::DoAllEffectOnTarget` aggregates `m_damage`.
`EffectWeaponDmg` executes at the last weapon effect and combines weapon roll, flat/AP bonuses, weapon
percentage, diseases/glyph modifiers and melee bonuses into that same `m_damage`. `CalculateSpellDamageTaken`
then calls the direct hook once before armor, crit, block and absorb. White swings instead follow
`CalculateMeleeDamage` -> `ModifyMeleeDamage` -> `DealMeleeDamage`.

For diseases, `AuraEffect::CalculateAmount` snapshots caster/AP bonuses. `HandlePeriodicDamageAurasTick`
calls the periodic hook, applies target modifiers/mitigation, then calls `DealDamage(DOT)` without entering
`CalculateSpellDamageTaken`. Death and Decay's `PERIODIC_DUMMY` casts 52212 rather than using that route.
The Death Coil script supplies its rank's value to 47632; its nominal DBC base of 600 is overwritten.
Necrosis's proc mask 4 in repository data restricts its source to white damage. Unholy Blight copies dealt
Death Coil damage into 50536; Wandering Plague copies dealt disease damage into GENERIC spell 50526.

The generic dynamic-area periodic code can add caster bonuses after the early periodic hook. The installed
DBC has no DK-family persistent-area periodic-damage/leech or percent-damage aura taking that route;
Death and Decay instead uses the triggered-direct path above. The DBC audit test rejects such a new entry
so that a future data/core change requires a fresh audit. No core patch or wider damage hook is needed here.
The generic `DealDamage` hook lacks SpellInfo and is unsuitable for safely classifying these attacks.

Hero-level first-rank constants are the concrete balance problem: Plague Strike rank 1 is
`0.50 * (normalized weapon damage + 125)`, Blood Strike is `0.40 * (weapon + 260)` before its disease
modifier, and Icy Touch is 127-137 plus 0.10 AP. The old curve still supplied about 16.1% at level 5
and penalized level 55 through 59. It was neither designed around these payloads nor covered by path tests.

However, static inspection cannot establish why the reported live Aldric kills mobs in 1-3 hits.
Correctly enrolled level-5 characters running the old scaler should already have much smaller hits than
raw hero-level spells. Before treating persistent extreme damage as a tuning issue, check the deployed
binary, effective enable flags, enrollment (`mod-roleplay.dk` state), spell rank, weapon/AP, buffs and target
level/HP. The old progression preserved any imported higher ranks; that is another possible source, now repaired.
No claim is made that a missing melee hook caused the reported symptom or that live pacing is already proven.

## Curve and weapon choice

| Level | Ability multiplier |
| --- | --- |
| 1 | 0.06 |
| 5 | 0.08 |
| 10 | 0.12 |
| 20 | 0.28 |
| 30 | 0.43 |
| 40 | 0.60 |
| 50 | 0.80 |
| 54 | 0.96 |
| 55, 60, 80 | 1.00 |

Linear interpolation between these explicit anchors avoids hidden per-spell multipliers. At the default
endpoint the final level raises ability damage by about 4.17%, not a doubling. Positive integer hits round
to nearest with a minimum of one, as before; zero/negative amounts remain unchanged. Normal mitigation
can still reduce a hit to zero. Below the endpoint, the whole special strike is scaled once. This includes
its weapon component because it is additional rune-funded damage, unlike a white swing. Scaling only flat
bonuses would preserve mature strike/AP/disease throughput on top of unchanged auto-attacks. Weapon upgrades
still improve all white damage at full value and improve specials proportionally; no weapon stats are edited.

The anchors were checked against first-rank payloads, then reduced at 5/10 to account for early rune burst.
For a **synthetic** input of normalized weapon damage 15, AP 30 and no talents/presence/crit/mitigation,
level-5 Icy Touch averages 10.8, Plague Strike 5.6, and two-disease Blood Strike 11 before rounding.
Warrior Heroic Strike rank 1 adds 11 to a swing; Paladin Judgement of Righteousness contributes 7 and its seal
adds about 1.98 per 3-second swing under those inputs. These are different resource/cadence mechanisms, not
equivalent DPS. They support an initial low-level budget; they do not establish time-to-kill parity.

A follow-up comparison used repository class stats and actual two-handed weapon templates across levels
5/10/20/30/40/50/55. Existing module SQL supplies missing DK 1-54 stats from Warrior stats, and DK/Warrior/
Paladin use the same melee AP formula. With Training Sword (8178), no armor stats, affixes or racial modifiers,
a level-5 DK/Warrior has 51 AP and an average white hit of 37.75; Paladin has 47 AP and a white hit of 36.75
before self-buffs. Revised DK payloads are about Icy Touch 10.97, Plague Strike 6.48 and two-disease Blood
Strike 11.88 before rounding/mitigation. Heroic Strike adds 11 to a replaced swing; Paladin Judgement adds
10.4 and its seal about 3.62 per swing, before class self-buffs.

Equal-looking payloads understate DK burst: six ready runes fund six attacks, and Death Coil adds another
attack from level 6. Warrior also has Rend/Battle Shout but needs rage; Paladin has Might and seal procs with
Judgement cooldown/mana constraints. A conservative follow-up reduces ability damage by 20% at 5 and 10
(10% -> 8%, 15% -> 12%), tapering back by 20. White damage stays intact, so total damage falls by less than
20%. Later-level talents, pets and rotations prevent an honest precise class ranking from this small model;
20+ was not retuned on that basis. This is an estimate, not measured DPS/TTK parity. Check 5/10/20 in-game
before adding further tuning controls or touching resources.

`tests/death_knight_payload_model.cpp` prints that theoretical comparison for levels 5/10/20/30/40/50/55.
Run its compiled `model.exe <normalized weapon hit including AP> <AP> <holy SP> <swing seconds>` with observed
staging stats. All rows deliberately reuse the supplied stats to isolate the curve. Heroic Strike uses each
level's appropriate rank's base bonus; the DK columns use lowest ranks. It omits per-level spell-point growth,
mitigation, crit, talents, presence, actual resource scheduling and class stat differences. It is a payload
sanity check, not a combat simulator. Do not use the constant-input high-level rows as gearing or TTK results.

## Ranks and unlocks

DK progression does not call Blood Knight's `RankFor`; it grants explicit first-rank IDs. The relevant DBC
levels are Icy Touch/Plague Strike/Blood Strike/Death Coil 55, Death Strike 56, Blood Boil 58, Death and Decay
60, Obliterate 61 and Rune Strike 67. These have no truly low-level ranks, so their first ranks remain the
intentional fallback with ability normalization. `SpellLevel` does not automatically shrink their fixed
damage to match a level-5 caster.

Before granting each unlocked first rank, progression now removes prematurely learned higher ranks only
within that explicit spell chain and only below level 55. This runs at existing login/level refreshes.
It uses the core rank-chain APIs, not a list of rank IDs; legitimate talent spells and unrelated spellbooks
are untouched. At 55+ normal trainer/rank behavior remains unchanged. The module does not grant high ranks
automatically. Artificially GM-teaching a high rank after refresh is not a supported balance-testing setup.

The actual existing unlock table differs from the request's approximate list: level 14 grants **Pestilence
(50842)** and level 26 grants **Blood Boil (48721)**. It does not grant Corpse Explosion. This patch preserves
the table. If separately talented, Corpse Explosion's player-cast damage trigger follows the direct route;
the ghoul's own explosion remains pet damage, outside this player-only policy.

## Configuration and deployment

- `Roleplay.DeathKnight.EnableLowLevelScaling` remains the enable switch.
- `FullDamageScalingLevel` now defaults to 55 and clamps to 2-55. Lower settings compress the same curve
  horizontally; an old value of 60 cannot penalize level 55+.
- `DamageCurveExponent` defaults to 1, range 0.5-2. Raising it weakens early ability damage, lowering it
  strengthens it. Exponentiation keeps the positive monotonic curve and the endpoint at 1.
- `MinimumDamageMultiplier` is obsolete and ignored. Remove it when reviewing the distributed config;
  existing configs get the new default curve without adding the new setting. Restart for configuration changes.

No SQL migration, client patch, live database edits, or new manual spell-training step is required. After
review, merge/pull this module branch into the server's source checkout; reconcile the three scaling options
in the installed `Roleplay.conf` without replacing unrelated settings. Build using the existing compatible
core/toolchain and install the resulting server/modules by the normal server procedure. Stop/start worldserver
during maintenance and relog Aldric so rank repair runs. Test first on staging. Do not install object files
from the isolated test runner as a server build. Earlier sheet/shield work is a separate branch and can be
merged independently; this damage branch does not include those unrelated commits.

## Tests and remaining live acceptance

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/run-dk-tests.ps1 -CompileModule
powershell -NoProfile -ExecutionPolicy Bypass -File tests/audit-dk-dbc.ps1 -DbcPath C:/Gaming/AzerothCore/Data/dbc
```

Override `-Core`/`-VsDevCmd` for another checkout/MSVC installation. Tests use actual production hook and
progression bodies with lightweight Player/SpellInfo test doubles, real core enum headers, pure curve/rank
policies, and checks of the installed core's routing. They cover all requested level anchors, monotonicity,
invalid tuning inputs, the 54-55 transition, strikes, diseases, triggered damage, double-scaling regressions,
unmanaged/other-family/55+ exclusions, unchanged whites/healing and preservation of progression/shield spells.
The DBC test validates the audited spell families/payloads/rank levels and absence of an unsupported periodic
route. It reads only Spell.dbc. All test/build artifacts go to a disposable temporary directory.

Locally, all four DK regression executables passed, the payload model ran, DBC/routing checks passed, and all
eight DK production translation units compiled with MSVC against the available core. The earlier sheet/shield
regressions also passed with this work present. No full worldserver link or live combat test was performed.
The Python codestyle command could not run because the available `python.exe` was inaccessible.

For each of levels **5, 10, 20, 30, 40, 50, 55**, compare a managed DK, Warrior and Paladin on staging:

1. Use similarly geared, level-appropriate weapons/armor and equal-level ordinary outdoor mobs of the same
   type/HP. No outside buffs, heirlooms, elites, playerbots or assistance. Record weapon, AP, armor, talents,
   presence, spell ranks and mob HP/level. Test normal class self-buffs consistently and identify them.
2. Record at least 20 pulls per class, allowing normal resource limitations. Log elapsed combat time, ability
   globals, white/ability/DoT damage separately, damage taken, ending HP/resources, and downtime until the next
   pull. Include a consecutive-pull series, not just full-resource openers. Compare medians and ranges.
3. Inspect separate combat-log events for Icy Touch, Plague/Blood/Death Strike, Death Coil's 47632, both diseases,
   Obliterate and Rune Strike at their unlocks. Compare controlled scaling-on/off runs with identical stats.
   Expect one factor, allowing rounding/crit/mitigation; verify unchanged white damage with both a base weapon
   and an upgrade. Verify Unholy Blight/Wandering Plague inherit one factor and DnD ticks get one factor.
4. Verify unrelated DK, Warlock/racial/item damage, heals, and level-55+ damage are unchanged. Check 54 -> 55
   with identical gear. Relog an imported low-level character with a premature higher rank and verify first-rank
   fallback, while normal 55+ ranks and unlock/proficiency/campaign behavior remain intact.
5. Accept comparable general leveling difficulty, not exact DPS equality. Investigate any routine 1-3-global
   kills absent from both references. Record the event IDs and raw combat log before adjusting tuning.

Six mature rune slots allow a front-loaded opener independent of rage buildup; later Runic Power adds damage
between rune cycles. This could still create excessive burst/sustained throughput despite normalized payloads.
Rune regeneration and RP generation were deliberately left unchanged. Pets and talent interactions likewise
need staging observation. Death Strike's health-per-disease heal, Death Pact's percentage heal, Blood Presence
damage/healing, Frost Presence mitigation and cooldowns can reduce downtime more than comparator classes.
None was broadly nerfed. Blood Knight's Blood Plague drain still observes the scaled disease tick with the
existing loader order; its mana, Warlock unlocks and healing synergies were not changed. Treat any remaining
resource/survivability outlier as a separate measured balance task.
