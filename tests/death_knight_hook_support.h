// The runner inserts the actual Scaling.cpp implementation after these lightweight test doubles.
// Core routing is checked separately; these doubles do not simulate combat or mitigation.
#include "RoleplayDeathKnightPolicy.h"
#include "SharedDefines.h"
#include "SpellAuraDefines.h"
#include <cassert>
#include <iostream>
#include <vector>

struct Player
{
    bool Managed = true;
};
struct Unit
{
    Player* Character = nullptr;
    uint8 Level = 5;
    Player* ToPlayer() { return Character; }
    uint8 GetLevel() const { return Level; }
};
struct SpellInfo
{
    uint32 Id;
    uint32 SpellFamilyName;
    uint32 Aura = 0;
    bool HasAura(uint32 aura) const { return Aura == aura; }
};
enum
{
    UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN,
    UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK
};
struct UnitScript
{
    std::vector<uint16> Hooks;
    UnitScript(char const*, bool, std::vector<uint16> hooks) : Hooks(hooks) { }
    virtual ~UnitScript() = default;
    virtual void ModifySpellDamageTaken(Unit*, Unit*, int32&, SpellInfo const*) { }
    virtual void ModifyPeriodicDamageAurasTick(Unit*, Unit*, uint32&, SpellInfo const*) { }
    virtual void ModifyMeleeDamage(Unit*, Unit*, uint32&) { }
};
namespace Roleplay::DeathKnight
{
struct Config
{
    bool EnableLowLevelScaling = true;
    uint8 FullDamageScalingLevel = 55;
    float DamageCurveExponent = 1;
} Settings;
Config const& GetConfig() { return Settings; }
bool IsManaged(Player const* player) { return player && player->Managed; }
}
