# Mortal RP recruits

Commands are `.rp recruit create <creatureEntry> [name]`, `follow`, `stay`, `info`, and `dismiss`. Any player may
use any valid creature template; ownership is checked for every selected-recruit operation. Creation is blocked in
instances. There is no configured count limit.

`rp_recruit` is authoritative. ALIVE records reconstruct as temporary summons when their owner logs in. FOLLOW
uses `MoveFollow`; STAY persists the home location and uses idle movement. Template AI handles self-defense and
the module calls `AttackStart` when the owner enters combat. Cross-map followers reconstruct near the owner.

`UnitScript::OnUnitDeath` synchronously stores DEAD before a natural 60-second corpse despawn. DEAD recruits are
never reconstructed; dismissal stores DISMISSED. Recruits are not Playerbots and cannot receive RP outfits. RP
names persist and appear in `info`, but 3.3.5 overhead names remain the shared creature-template name.

Future candidates—not implemented—include stationing, patrols, props, history/personality, and automation.

