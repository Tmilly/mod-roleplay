#include "RoleplayDeathKnight.h"
#include "Chat.h"
#include "ObjectAccessor.h"
#include "TaskScheduler.h"
#include <chrono>

namespace Roleplay::DeathKnight
{
void OnArrival(Player* player)
{
    if (!IsManaged(player) || GetState(player)->Progress != Prepared || player->GetMapId() != 609)
        return;
    // Acknowledgement, not the teleport request, establishes arrival. Do not reset an aura
    // or phase here: the core has already applied area spells and loaded saved quest auras.
    GetState(player)->Progress = Arrived;
    SaveState(player);
    player->InitTalentForLevel();
    player->SaveToDB(false, false);
}

void TryCampaign(Player* player)
{
    if (!IsManaged(player))
        return;
    State* state = GetState(player);
    CampaignContext context{
        state->Progress, state->Busy, HasCampaignHistory(player), GetConfig().DelayedAcherusCampaign,
        player->GetLevel(), GetConfig().AcherusCampaignLevel,
        !GetConfig().AcherusRequiredQuest || player->IsQuestRewarded(GetConfig().AcherusRequiredQuest),
        player->IsInWorld() && player->IsAlive() && !player->IsInCombat() && !player->IsBeingTeleported()
            && !player->IsInFlight() && !player->GetVehicle() && !player->GetTransport() && !player->GetTradeData()
            && !player->InBattleground() && !player->InArena() && !player->GetInstanceId(),
        player->GetMapId() == 609, player->GetPhaseMask() == PHASEMASK_NORMAL && !player->GetPhaseByAuras()
    };
    CampaignAction action = DecideCampaign(context);
    if (action == CampaignAction::None)
        return;
    if (action == CampaignAction::RecordHistory)
    {
        state->Progress = Arrived;
        SaveState(player);
        player->SaveToDB(false, false);
        return;
    }
    auto start = GetStartingData(player->getRace(), player->getGender());
    if (!start)
        return;
    if (action == CampaignAction::PrepareGear)
    {
        DeliverStartingGear(player);
        return;
    }
    if (action == CampaignAction::RecordArrival)
    {
        OnArrival(player);
        return;
    }
    // A new stock DK has normal phase 1 and no chapter auras. Refuse an unrelated
    // phase rather than clearing a legitimate quest phase or forcing through IP.
    if (player->TeleportTo(start->Location))
        ChatHandler(player->GetSession()).SendSysMessage(
            "The Lich King summons you to Acherus. Speak to him to begin your campaign. "
            "Your starting equipment is in your inventory or mail; equip it when ready.");
}

void ScheduleCampaign(Player* player)
{
    if (!IsManaged(player) || GetState(player)->RetryScheduled || GetState(player)->Progress == Arrived)
        return;
    if (!GetConfig().DelayedAcherusCampaign || player->GetLevel() < GetConfig().AcherusCampaignLevel)
        return;
    GetState(player)->RetryScheduled = true;
    ObjectGuid guid = player->GetGUID();
    GetState(player)->Scheduler.Schedule(std::chrono::seconds(1), [guid](TaskContext context)
    {
        Player* current = ObjectAccessor::FindPlayer(guid);
        if (!current || !IsManaged(current))
            return;
        TryCampaign(current);
        if (GetState(current)->Progress != Arrived)
            context.Repeat(std::chrono::seconds(10));
        else
            GetState(current)->RetryScheduled = false;
    });
}
}
