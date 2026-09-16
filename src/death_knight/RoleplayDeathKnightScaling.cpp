#include "RoleplayDeathKnight.h"
#include "SpellInfo.h"
#include "UnitScript.h"

namespace Roleplay::DeathKnight
{
float DamageMultiplier(uint8 level)
{
    auto const& config = GetConfig();
    if (!config.EnableLowLevelScaling || level >= config.FullDamageScalingLevel)
        return 1.0f;
    return LevelDamageScale(level, config.FullDamageScalingLevel, config.DamageCurveExponent);
}

namespace
{
template <typename T>
void Scale(Unit* caster, SpellInfo const* spell, T& amount, DamagePath path)
{
    if (!caster || !spell || !ShouldScaleAbility(IsManaged(caster->ToPlayer()),
        GetConfig().EnableLowLevelScaling, spell->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT,
        caster->GetLevel(), spell->Id, path))
        return;
    amount = ScaleAbilityAmount(amount, DamageMultiplier(caster->GetLevel()));
}

class ScalingScript : public UnitScript
{
public:
    ScalingScript() : UnitScript("RoleplayDeathKnightScaling", true,
        {UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN, UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK}) { }

    void ModifySpellDamageTaken(Unit* /*target*/, Unit* caster, int32& damage, SpellInfo const* spell) override
    {
        // EffectWeaponDmg aggregates the complete special strike into m_damage before
        // this hook. A melee hook would hit white swings, not add missing strike coverage.
        Scale(caster, spell, damage, DamagePath::DirectSpell);
    }

    void ModifyPeriodicDamageAurasTick(Unit* /*target*/, Unit* caster, uint32& damage,
        SpellInfo const* spell) override
    {
        // Diseases snapshot caster bonuses before this hook and never call the direct hook.
        // Death and Decay is periodic-dummy -> triggered direct damage, scaled there only.
        // This hook is also used for healing: do not classify periodic healing as damage.
        if (spell && (spell->HasAura(SPELL_AURA_PERIODIC_DAMAGE) || spell->HasAura(SPELL_AURA_PERIODIC_LEECH)))
            Scale(caster, spell, damage, DamagePath::PeriodicDamage);
    }
};
}

void AddScalingScripts() { new ScalingScript(); }
}
