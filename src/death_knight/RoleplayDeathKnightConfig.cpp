#include "RoleplayDeathKnight.h"
#include "Config.h"
#include "Log.h"
#include "World.h"
#include <algorithm>
#include <cmath>

namespace Roleplay::DeathKnight
{
namespace
{
Config Settings;
}

Config const& GetConfig() { return Settings; }

void LoadConfig(bool reload)
{
    // Creation data and outfit snapshots are immutable while players are connected.
    if (reload)
    {
        LOG_INFO("module.roleplay", "Death Knight progression configuration requires a worldserver restart.");
        if (Settings.Enable)
        {
            sWorld->setIntConfig(CONFIG_START_HEROIC_PLAYER_LEVEL, Settings.StartLevel);
            sWorld->setIntConfig(CONFIG_CHARACTER_CREATING_MIN_LEVEL_FOR_HEROIC_CHARACTER, 0);
            sWorld->setBoolConfig(CONFIG_PLAYER_SETTINGS_ENABLED, true);
        }
        return;
    }

    Settings.Enable = sConfigMgr->GetOption<bool>("Roleplay.Enable", true)
        && sConfigMgr->GetOption<bool>("Roleplay.DeathKnight.Enable", true);
    Settings.StartLevel = std::clamp(sConfigMgr->GetOption<uint32>("Roleplay.DeathKnight.StartLevel", 1), 1u, 55u);
    Settings.UseRacialStartingZone =
        sConfigMgr->GetOption<bool>("Roleplay.DeathKnight.UseRacialStartingZone", true);
    Settings.PlateLevel = std::clamp(sConfigMgr->GetOption<uint32>("Roleplay.DeathKnight.PlateLevel", 40), 1u, 80u);
    Settings.EnableLowLevelScaling =
        sConfigMgr->GetOption<bool>("Roleplay.DeathKnight.EnableLowLevelScaling", true);
    Settings.FullDamageScalingLevel =
        std::clamp(sConfigMgr->GetOption<uint32>("Roleplay.DeathKnight.FullDamageScalingLevel", 60), 2u, 80u);
    Settings.MinimumDamageMultiplier =
        sConfigMgr->GetOption<float>("Roleplay.DeathKnight.MinimumDamageMultiplier", 0.10f);
    if (!std::isfinite(Settings.MinimumDamageMultiplier))
        Settings.MinimumDamageMultiplier = 0.10f;
    Settings.MinimumDamageMultiplier = std::clamp(Settings.MinimumDamageMultiplier, 0.01f, 1.0f);
    Settings.DelayedAcherusCampaign =
        sConfigMgr->GetOption<bool>("Roleplay.DeathKnight.DelayedAcherusCampaign", true);
    // Stock intro quests require 55; lowering only the teleport level strands the character.
    Settings.AcherusCampaignLevel =
        std::clamp(sConfigMgr->GetOption<uint32>("Roleplay.DeathKnight.AcherusCampaignLevel", 55), 55u, 80u);
    Settings.GiveAcherusStartingGear =
        sConfigMgr->GetOption<bool>("Roleplay.DeathKnight.GiveAcherusStartingGear", true);
    Settings.RecoverExistingCharacters =
        sConfigMgr->GetOption<bool>("Roleplay.DeathKnight.RecoverExistingCharacters", true);
    Settings.AcherusRequiredQuest =
        sConfigMgr->GetOption<uint32>("Roleplay.DeathKnight.AcherusRequiredQuest", 0);

    if (!Settings.Enable)
        return;

    if (!Settings.UseRacialStartingZone && Settings.StartLevel < 55)
    {
        LOG_ERROR("module.roleplay", "A level <55 DK cannot start in stock Acherus. Enabling racial starts.");
        Settings.UseRacialStartingZone = true;
    }
    sWorld->setIntConfig(CONFIG_START_HEROIC_PLAYER_LEVEL, Settings.StartLevel);
    sWorld->setIntConfig(CONFIG_CHARACTER_CREATING_MIN_LEVEL_FOR_HEROIC_CHARACTER, 0);
    // Required for transactional recovery, including when the global default is disabled.
    sWorld->setBoolConfig(CONFIG_PLAYER_SETTINGS_ENABLED, true);

    if (sConfigMgr->GetOption<uint32>("IndividualProgression.DeathKnightUnlockProgression", 0)
        || sConfigMgr->GetOption<uint32>("IndividualProgression.DeathKnightStartingProgression", 0))
        LOG_ERROR("module.roleplay", "Level-1 DKs require IndividualProgression.DeathKnightUnlockProgression = 0 "
            "and IndividualProgression.DeathKnightStartingProgression = 0. Roleplay will not bypass IP restrictions.");
}
}
