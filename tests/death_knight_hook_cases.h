int main()
{
    using namespace Roleplay::DeathKnight;
    ScalingScript script;
    UnitScript& hooks = script;
    assert(script.Hooks == std::vector<uint16>({UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN,
        UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK}));
    Player managed;
    Unit caster{&managed, 5};
    // Audited DBC IDs: all strikes aggregate weapon+bonus before the direct hook.
    for (uint32 id : {45477u, 45462u, 45902u, 47632u, 49998u, 49020u, 56815u, 52212u, 50463u, 51460u})
    {
        SpellInfo spell{id, SPELLFAMILY_DEATHKNIGHT};
        int32 damage = 1000;
        hooks.ModifySpellDamageTaken(nullptr, &caster, damage, &spell);
        assert(damage == 100); // Once, not 10 (twice) or 1000 (missed).
        uint32 noPeriodicDamage = damage;
        hooks.ModifyPeriodicDamageAurasTick(nullptr, &caster, noPeriodicDamage, &spell);
        assert(noPeriodicDamage == 100);
    }
    for (uint32 id : {55078u, 55095u})
    {
        SpellInfo disease{id, SPELLFAMILY_DEATHKNIGHT, SPELL_AURA_PERIODIC_DAMAGE};
        uint32 tick = 1000;
        hooks.ModifyPeriodicDamageAurasTick(nullptr, &caster, tick, &disease);
        assert(tick == 100);
        // Aura-only applications have no m_damage, so core does not call the direct hook for them.
        // Core Wandering Plague copies this tick; its generic family excludes a second multiplier.
        SpellInfo plague{50526, SPELLFAMILY_GENERIC};
        int32 triggered = tick;
        hooks.ModifySpellDamageTaken(nullptr, &caster, triggered, &plague);
        assert(triggered == 100);
    }
    SpellInfo coil{47632, SPELLFAMILY_DEATHKNIGHT};
    int32 coilDamage = 1000;
    hooks.ModifySpellDamageTaken(nullptr, &caster, coilDamage, &coil);
    SpellInfo blight{50536, SPELLFAMILY_DEATHKNIGHT, SPELL_AURA_PERIODIC_DAMAGE};
    uint32 blightTick = coilDamage / 10;
    hooks.ModifyPeriodicDamageAurasTick(nullptr, &caster, blightTick, &blight);
    assert(blightTick == 10); // Already normalized by the source Death Coil, not 1.

    uint32 white = 1000;
    hooks.ModifyMeleeDamage(nullptr, &caster, white);
    assert(white == 1000);
    for (uint32 family : {SPELLFAMILY_GENERIC, SPELLFAMILY_WARRIOR, SPELLFAMILY_PALADIN, SPELLFAMILY_WARLOCK})
    {
        SpellInfo other{1, family, SPELL_AURA_PERIODIC_DAMAGE};
        int32 direct = 1000;
        uint32 tick = 1000;
        hooks.ModifySpellDamageTaken(nullptr, &caster, direct, &other);
        hooks.ModifyPeriodicDamageAurasTick(nullptr, &caster, tick, &other);
        assert(direct == 1000 && tick == 1000);
    }
    SpellInfo healing{1, SPELLFAMILY_DEATHKNIGHT, SPELL_AURA_PERIODIC_HEAL};
    uint32 heal = 1000;
    hooks.ModifyPeriodicDamageAurasTick(nullptr, &caster, heal, &healing);
    assert(heal == 1000);
    for (int scenario = 0; scenario < 7; ++scenario)
    {
        managed.Managed = scenario != 0;
        Settings.EnableLowLevelScaling = scenario != 1;
        caster.Level = scenario == 2 ? 55 : scenario == 3 ? 60 : scenario == 4 ? 80 : 5;
        caster.Character = scenario == 5 ? nullptr : &managed;
        Unit* source = scenario == 6 ? nullptr : &caster;
        int32 direct = 1000;
        uint32 tick = 1000;
        SpellInfo disease{55095, SPELLFAMILY_DEATHKNIGHT, SPELL_AURA_PERIODIC_DAMAGE};
        hooks.ModifySpellDamageTaken(nullptr, source, direct, &coil);
        hooks.ModifyPeriodicDamageAurasTick(nullptr, source, tick, &disease);
        assert(direct == 1000 && tick == 1000);
    }
    std::cout << "Actual DK hook implementation: direct, strikes, ticks, triggers and exclusions passed\n";
}
