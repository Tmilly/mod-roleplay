#include "../src/death_knight/RoleplayDeathKnightPolicy.h"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace Roleplay::DeathKnight;

int main()
{
    CampaignContext context{Leveling, false, false, true, 54, 55, true, true, false, true};
    assert(DecideCampaign(context) == CampaignAction::None);
    context.Level = 55;
    assert(DecideCampaign(context) == CampaignAction::PrepareGear);
    context.Progress = Prepared; // Committed gear, crash before teleport acknowledgement.
    assert(DecideCampaign(context) == CampaignAction::Teleport);
    context.InAcherus = true; // Crash after arrival, before saving the acknowledgement.
    assert(DecideCampaign(context) == CampaignAction::RecordArrival);
    context.Progress = Arrived;
    context.InAcherus = false; // Finished campaign, ordinary travel or logout.
    for (std::uint8_t level = 1; level <= 80; ++level)
    {
        context.Level = level;
        assert(DecideCampaign(context) == CampaignAction::None);
    }
    for (Stage stage : {Leveling, Prepared})
    {
        context = {stage, false, true, true, 80, 55, true, true, false, true};
        assert(DecideCampaign(context) == CampaignAction::RecordHistory); // Never reset any quest history.
        context.HasHistory = false;
        for (bool* gate : {&context.Enabled, &context.MeetsGate, &context.SafeToTravel, &context.OriginalPhase})
        {
            *gate = false;
            assert(DecideCampaign(context) == CampaignAction::None);
            *gate = true;
        }
        context.Busy = true;
        assert(DecideCampaign(context) == CampaignAction::None); // Uncommitted delivery cannot teleport.
    }
    context.Progress = Unmanaged;
    context.HasHistory = true;
    context.Busy = false;
    assert(DecideCampaign(context) == CampaignAction::None);

    assert(LevelTalentPoints(1) == 0 && LevelTalentPoints(9) == 0 && LevelTalentPoints(10) == 1);
    assert(LevelTalentPoints(40) == 31 && LevelTalentPoints(54) == 45);
    assert(LevelTalentPoints(55) == 46 && LevelTalentPoints(58) == 49 && LevelTalentPoints(80) == 71);
    for (std::uint8_t full : {std::uint8_t(2), std::uint8_t(60), std::uint8_t(80)})
    {
        assert(std::abs(LevelDamageScale(1, full, 0.10f) - 0.10f) < 0.00001f);
        float previous = 0;
        for (std::uint8_t level = 1; level <= 80; ++level)
        {
            float scale = LevelDamageScale(level, full, 0.10f);
            assert(scale >= previous && scale <= 1.0f);
            if (level >= full)
                assert(scale == 1.0f);
            previous = scale;
        }
    }
    std::cout << "Death Knight campaign recovery, talent totals and scaling policy: passed\n";
}
