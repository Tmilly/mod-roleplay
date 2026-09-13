#include "RoleplayDeathKnight.h"
#include "SpellInfo.h"
#include "UnitScript.h"
#include <algorithm>
#include <cmath>

namespace Roleplay::DeathKnight
{
float DamageMultiplier(uint8 level)
{
    auto const& config = GetConfig();
    if (!config.EnableLowLevelScaling || level >= config.FullDamageScalingLevel)
        return 1.0f;
    return LevelDamageScale(level, config.FullDamageScalingLevel, config.MinimumDamageMultiplier);
}

namespace
{
template <typename T>
void Scale(Unit* caster, SpellInfo const* spell, T& amount)
{
    if (!caster || !IsManaged(caster->ToPlayer()) || !spell || spell->SpellFamilyName != SPELLFAMILY_DEATHKNIGHT
        || amount <= 0 || caster->GetLevel() >= GetConfig().FullDamageScalingLevel)
        return;
    amount = static_cast<T>(std::max(1.0, std::round(double(amount) * DamageMultiplier(caster->GetLevel()))));
}

class ScalingScript : public UnitScript
{
public:
    ScalingScript() : UnitScript("RoleplayDeathKnightScaling", true,
        {UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN, UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK}) { }

    void ModifySpellDamageTaken(Unit* /*target*/, Unit* caster, int32& damage, SpellInfo const* spell) override
    {
        Scale(caster, spell, damage);
    }

    void ModifyPeriodicDamageAurasTick(Unit* /*target*/, Unit* caster, uint32& damage,
        SpellInfo const* spell) override
    {
        // This core also calls this hook for periodic healing. Do not scale percentage heals,
        // or add a second healing hook (which would scale those ticks twice).
        if (spell && (spell->HasAura(SPELL_AURA_PERIODIC_DAMAGE) || spell->HasAura(SPELL_AURA_PERIODIC_LEECH)))
            Scale(caster, spell, damage);
    }
};
}

void AddScalingScripts() { new ScalingScript(); }
}
