#ifndef ROLEPLAY_BLOOD_KNIGHT_H
#define ROLEPLAY_BLOOD_KNIGHT_H

#include "DataMap.h"
#include "Player.h"

#include <array>
#include <string>

namespace Roleplay::BloodKnight
{
inline constexpr char SettingsSource[] = "mod-roleplay.bloodknight";

struct Config
{
    bool Enable = true;
    std::string CharacterName = "Aldric";
    uint32 CharacterGuid = 0;
    bool ManaEnable = true;
    uint32 ManaBasePerLevel = 20;
    float ManaIntellectMultiplier = 15.0f;
    float ManaOutOfCombatRegenPct = 2.0f;
    bool DrainLife = true;
    bool CurseWeakness = true;
    bool CurseAgony = true;
    bool Fear = true;
    bool Shadowfury = true;
    bool BloodPlagueDrain = true;
    bool VampiricBlood = true;
    bool SoulFeast = true;
    float BloodPlagueDrainPct = 7.0f;
};

struct State : DataMap::Base
{
    uint32 ManaElapsed = 0;
    bool ManaInitialized = false;
    uint32 SavedMana = 0;
};

Config const& GetConfig();
void LoadConfig(bool reload);
bool IsBloodKnight(Player const* player);
State* GetState(Player const* player);
uint32 ManaFor(Player const* player);
void Refresh(Player* player);
void Save(Player* player);
void AddScripts();
}

void AddRoleplayBloodKnightScripts();

#endif
