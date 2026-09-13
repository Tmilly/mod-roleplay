#ifndef ROLEPLAY_DEATH_KNIGHT_POLICY_H
#define ROLEPLAY_DEATH_KNIGHT_POLICY_H

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

constexpr float LevelDamageScale(std::uint8_t level, std::uint8_t fullLevel, float minimum)
{
    if (fullLevel <= 1 || level >= fullLevel)
        return 1.0f;
    if (level <= 1)
        return minimum;
    return minimum + (1.0f - minimum) * float(level - 1) / float(fullLevel - 1);
}
}

#endif
