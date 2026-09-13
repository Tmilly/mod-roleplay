#ifndef ROLEPLAY_DEATH_KNIGHT_H
#define ROLEPLAY_DEATH_KNIGHT_H

#include "DataMap.h"
#include "RoleplayDeathKnightPolicy.h"
#include "Player.h"
#include "TaskScheduler.h"
#include <array>
#include <vector>

namespace Roleplay::DeathKnight
{
inline constexpr char SettingsSource[] = "mod-roleplay.dk";

struct Config
{
    bool Enable = true;
    uint8 StartLevel = 1;
    bool UseRacialStartingZone = true;
    uint8 PlateLevel = 40;
    bool EnableLowLevelScaling = true;
    uint8 FullDamageScalingLevel = 60;
    float MinimumDamageMultiplier = 0.10f;
    bool DelayedAcherusCampaign = true;
    uint8 AcherusCampaignLevel = 55;
    bool GiveAcherusStartingGear = true;
    bool RecoverExistingCharacters = true;
    uint32 AcherusRequiredQuest = 0;
};

struct State : DataMap::Base
{
    Stage Progress = Unmanaged;
    bool Busy = false;
    bool RetryScheduled = false;
    bool TrackTaxi = false;
    TaxiMask Taxi{};
    TaskScheduler Scheduler;
};

struct StartingData
{
    WorldLocation Location;
    uint32 Area = 0;
    std::vector<std::pair<uint32, uint32>> Items;
};

Config const& GetConfig();
void LoadConfig(bool reload);
void PrepareCreationData();
StartingData const* GetStartingData(uint8 race, uint8 gender);
State* GetState(Player const* player);
bool IsManaged(Player const* player);
void LoadEarlyState(Player* player);
void SaveState(Player* player);
void InitializeCharacter(Player* player);
void ValidateLogin(Player* player);
void ApplyProgression(Player* player);
void RestoreTaxi(Player* player);
void RecordTaxi(Player const* player, uint32 node);
bool HasCampaignHistory(Player* player);
void TryCampaign(Player* player);
void ScheduleCampaign(Player* player);
void OnArrival(Player* player);
bool DeliverStartingGear(Player* player);
float DamageMultiplier(uint8 level);
void AddScalingScripts();
}

void AddRoleplayDeathKnightScripts();

#endif
