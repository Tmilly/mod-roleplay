#include "../src/death_knight/RoleplayDeathKnightPolicy.h"
#include <cassert>
#include <iostream>
#include <limits>

using namespace Roleplay::DeathKnight;

int main()
{
    for (auto const& anchor : DamageCurve)
        assert(std::abs(LevelDamageScale(std::uint8_t(anchor.Level), 55, 1) - anchor.Multiplier) < 0.00001f);
    assert(std::abs(LevelDamageScale(5, 55, 1) - 0.08f) < 0.00001f);
    assert(std::abs(LevelDamageScale(10, 55, 1) - 0.12f) < 0.00001f);
    assert(std::abs(LevelDamageScale(54, 55, 1) - 0.96f) < 0.00001f);
    assert(LevelDamageScale(55, 55, 1) == 1);
    assert(LevelDamageScale(60, 55, 1) == 1);
    assert(LevelDamageScale(80, 55, 1) == 1);
    assert(LevelDamageScale(55, 80, 2) == 1); // Legacy configuration cannot extend the penalty.
    assert(LevelDamageScale(40, 40, 1) == 1);
    assert(LevelDamageScale(54, 55, 1) / LevelDamageScale(55, 55, 1) > 0.95f);
    for (unsigned full = 0; full <= 80; ++full)
        for (float exponent : {-1.0f, 0.5f, 1.0f, 2.0f, 3.0f, std::numeric_limits<float>::quiet_NaN()})
        {
            float previous = 0;
            for (unsigned level = 0; level <= 80; ++level)
            {
                float scale = LevelDamageScale(level, full, exponent);
                assert(std::isfinite(scale) && scale > 0 && scale <= 1 && scale >= previous);
                if (level >= 55)
                    assert(scale == 1);
                previous = scale;
            }
        }
    assert(LevelDamageScale(5, 55, 2) < LevelDamageScale(5, 55, 1));
    assert(LevelDamageScale(5, 55, 0.5f) > LevelDamageScale(5, 55, 1));
    assert(ScaleAbilityAmount(0, 0.1f) == 0);
    assert(ScaleAbilityAmount(-5, 0.1f) == -5);
    assert(ScaleAbilityAmount(1u, 0.06f) == 1);
    assert(ScaleAbilityAmount(1000u, 0.1f) == 100);
    assert(ScaleAbilityAmount(1000u, 1) == 1000);

    for (DamagePath path : {DamagePath::DirectSpell, DamagePath::PeriodicDamage})
    {
        assert(ShouldScaleAbility(true, true, true, 5, 45477, path));
        assert(!ShouldScaleAbility(false, true, true, 5, 45477, path));
        assert(!ShouldScaleAbility(true, false, true, 5, 45477, path));
        assert(!ShouldScaleAbility(true, true, false, 5, 689, path));
        for (std::uint8_t level : {55, 60, 80})
            assert(!ShouldScaleAbility(true, true, true, level, 45477, path));
    }
    assert(!ShouldScaleAbility(true, true, true, 5, 6603, DamagePath::Other));
    assert(!ShouldScaleAbility(true, true, true, 5, 50536, DamagePath::PeriodicDamage));
    assert(!ShouldScaleAbility(true, true, false, 5, 50526, DamagePath::DirectSpell));
    // First-rank fallback is handled by progression, never passed to the higher-rank predicate.
    assert(IsPrematureProgressionRank(5, 61));
    assert(IsPrematureProgressionRank(54, 61));
    assert(!IsPrematureProgressionRank(55, 61));
    assert(!IsPrematureProgressionRank(80, 61));
    assert(!IsPrematureProgressionRank(20, 20));
    std::cout << "DK curve, scope, derived damage and rank policy: passed\n";
}
