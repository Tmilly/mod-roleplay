#ifndef ROLEPLAY_DEATH_KNIGHT_POLICY_H
#define ROLEPLAY_DEATH_KNIGHT_POLICY_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace Roleplay::DeathKnight
{
enum Stage : std::uint32_t { Unmanaged = 0, Leveling = 1, Prepared = 2, Arrived = 3 };
enum class CampaignAction { None, RecordHistory, PrepareGear, Teleport, RecordArrival };

struct CampaignContext
{
    Stage Progress;
    bool Busy;
    bool HasHistory;
    bool Enabled;
    std::uint8_t Level;
    std::uint8_t RequiredLevel;
    bool MeetsGate;
    bool SafeToTravel;
    bool InAcherus;
    bool OriginalPhase;
};

constexpr CampaignAction DecideCampaign(CampaignContext const& context)
{
    if (context.Progress == Unmanaged || context.Progress == Arrived || context.Busy)
        return CampaignAction::None;
    if (context.HasHistory)
        return CampaignAction::RecordHistory;
    if (!context.Enabled || context.Level < context.RequiredLevel || !context.MeetsGate || !context.SafeToTravel)
        return CampaignAction::None;
    if (!context.InAcherus && !context.OriginalPhase)
        return CampaignAction::None;
    if (context.Progress == Leveling)
        return CampaignAction::PrepareGear;
    return context.InAcherus ? CampaignAction::RecordArrival : CampaignAction::Teleport;
}

constexpr std::uint32_t LevelTalentPoints(std::uint8_t level)
{
    return level < 10 ? 0 : level - 9;
}

struct DamageAnchor
{
    float Level;
    float Multiplier;
};

// First-rank DK spells carry level-55+ flat damage even with a level-1 weapon.
// Only ability damage follows this curve; white damage retains normal weapon/AP progression.
inline constexpr std::array<DamageAnchor, 8> DamageCurve{{
    {1, 0.06f}, {5, 0.08f}, {10, 0.12f}, {20, 0.28f},
    {30, 0.43f}, {40, 0.60f}, {50, 0.80f}, {55, 1.00f}
}};

inline float LevelDamageScale(std::uint8_t level, std::uint8_t fullLevel, float exponent)
{
    // A legacy FullDamageScalingLevel=60 must never penalize normal level-55 DKs.
    fullLevel = std::clamp<std::uint8_t>(fullLevel, 2, 55);
    if (level >= fullLevel)
        return 1.0f;
    exponent = std::isfinite(exponent) ? std::clamp(exponent, 0.5f, 2.0f) : 1.0f;
    float position = 1.0f + float(std::max<std::uint8_t>(level, 1) - 1) * 54.0f / float(fullLevel - 1);
    for (std::size_t i = 1; i < DamageCurve.size(); ++i)
    {
        auto const& left = DamageCurve[i - 1];
        auto const& right = DamageCurve[i];
        if (position <= right.Level)
        {
            float fraction = (position - left.Level) / (right.Level - left.Level);
            return std::pow(left.Multiplier + fraction * (right.Multiplier - left.Multiplier), exponent);
        }
    }
    return 1.0f;
}

enum class DamagePath { DirectSpell, PeriodicDamage, Other };

constexpr bool ShouldScaleAbility(bool managed, bool enabled, bool deathKnightFamily,
    std::uint8_t level, std::uint32_t spellId, DamagePath path)
{
    // Unholy Blight's core proc copies damage from the already-scaled Death Coil hit.
    // Its ticks must inherit that scaling, not multiply it again. Wandering Plague
    // similarly copies disease damage, but is GENERIC in stock data and fails the family gate.
    return managed && enabled && deathKnightFamily && level < 55 && spellId != 50536
        && (path == DamagePath::DirectSpell || path == DamagePath::PeriodicDamage);
}

template <typename T>
T ScaleAbilityAmount(T amount, float multiplier)
{
    if (amount <= 0 || multiplier >= 1.0f)
        return amount;
    return static_cast<T>(std::max(1.0, std::round(double(amount) * multiplier)));
}

constexpr bool IsPrematureProgressionRank(std::uint8_t level, std::uint32_t spellLevel)
{
    // Call only for higher ranks in the module's unlock chains. The first rank is
    // intentionally available early because no genuine low-level DK ranks exist.
    return level < 55 && spellLevel > level;
}
}

#endif
