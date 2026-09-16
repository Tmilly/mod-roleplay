#include "BloodKnight.h"

#include "Config.h"
#include "Log.h"
#include "PlayerSettings.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"
#include "World.h"

#include <algorithm>
#include <cmath>

namespace Roleplay::BloodKnight
{
namespace
{
Config Settings;

struct SpellBand
{
    bool Config::*Enabled;
    uint8 Level;
    std::array<uint32, 6> Ranks;
};

constexpr std::array<SpellBand, 5> SpellBands{{
    { &Config::CurseWeakness, 10, { 702, 1108, 6205, 7646, 11707, 11708 } },
    { &Config::DrainLife, 20, { 689, 699, 709, 7651, 11699, 11700 } },
    { &Config::CurseAgony, 30, { 980, 1014, 6217, 11711, 11712, 11713 } },
    { &Config::Fear, 40, { 5782, 6213, 6215, 0, 0, 0 } },
    { &Config::Shadowfury, 50, { 30283, 30413, 30414, 0, 0, 0 } }
}};

uint32 RankFor(SpellBand const& band, uint8 level)
{
    uint32 result = 0;
    for (uint32 spellId : band.Ranks)
    {
        if (!spellId)
            continue;
        if (SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId))
            if (level >= info->SpellLevel)
                result = spellId;
    }
    return result;
}
}

Config const& GetConfig() { return Settings; }

void LoadConfig(bool reload)
{
    if (reload)
        return;

    Settings.Enable = sConfigMgr->GetOption<bool>("Roleplay.Enable", true)
        && sConfigMgr->GetOption<bool>("Roleplay.BloodKnight.Enable", true);
    Settings.CharacterName = sConfigMgr->GetOption<std::string>("Roleplay.BloodKnight.CharacterName", "Aldric");
    Settings.CharacterGuid = sConfigMgr->GetOption<uint32>("Roleplay.BloodKnight.CharacterGuid", 0);
    Settings.ManaEnable = sConfigMgr->GetOption<bool>("Roleplay.BloodKnight.Mana.Enable", true);
    Settings.ManaBasePerLevel = sConfigMgr->GetOption<uint32>("Roleplay.BloodKnight.Mana.BasePerLevel", 20);
    Settings.ManaIntellectMultiplier = sConfigMgr->GetOption<float>("Roleplay.BloodKnight.Mana.IntellectMultiplier", 15.0f);
    Settings.ManaOutOfCombatRegenPct = sConfigMgr->GetOption<float>("Roleplay.BloodKnight.Mana.OutOfCombatRegenPct", 2.0f);
    Settings.DrainLife = sConfigMgr->GetOption<bool>("Roleplay.BloodKnight.Spells.EnableDrainLife", true);
    Settings.CurseWeakness = sConfigMgr->GetOption<bool>("Roleplay.BloodKnight.Spells.EnableCurseOfWeakness", true);
    Settings.CurseAgony = sConfigMgr->GetOption<bool>("Roleplay.BloodKnight.Spells.EnableCurseOfAgony", true);
    Settings.Fear = sConfigMgr->GetOption<bool>("Roleplay.BloodKnight.Spells.EnableFear", true);
    Settings.Shadowfury = sConfigMgr->GetOption<bool>("Roleplay.BloodKnight.Spells.EnableShadowfury", true);
    Settings.BloodPlagueDrain = sConfigMgr->GetOption<bool>("Roleplay.BloodKnight.Synergy.BloodPlagueDrain", true);
    Settings.VampiricBlood = sConfigMgr->GetOption<bool>("Roleplay.BloodKnight.Synergy.VampiricBlood", true);
    Settings.SoulFeast = sConfigMgr->GetOption<bool>("Roleplay.BloodKnight.Synergy.SoulFeast", true);
    Settings.BloodPlagueDrainPct = sConfigMgr->GetOption<float>("Roleplay.BloodKnight.Synergy.BloodPlagueDrainPct", 7.0f);
    if (!std::isfinite(Settings.ManaIntellectMultiplier)) Settings.ManaIntellectMultiplier = 15.0f;
    if (!std::isfinite(Settings.ManaOutOfCombatRegenPct)) Settings.ManaOutOfCombatRegenPct = 2.0f;
    if (!std::isfinite(Settings.BloodPlagueDrainPct)) Settings.BloodPlagueDrainPct = 7.0f;
    Settings.ManaIntellectMultiplier = std::clamp(Settings.ManaIntellectMultiplier, 0.0f, 100.0f);
    Settings.ManaOutOfCombatRegenPct = std::clamp(Settings.ManaOutOfCombatRegenPct, 0.0f, 20.0f);
    Settings.BloodPlagueDrainPct = std::clamp(Settings.BloodPlagueDrainPct, 0.0f, 25.0f);
    if (Settings.Enable)
        sWorld->setBoolConfig(CONFIG_PLAYER_SETTINGS_ENABLED, true);
}

bool IsBloodKnight(Player const* player)
{
    if (!Settings.Enable || !player || player->getClass() != CLASS_DEATH_KNIGHT)
        return false;
    if (Settings.CharacterGuid)
        return player->GetGUID().GetCounter() == Settings.CharacterGuid;
    return player->GetName() == Settings.CharacterName;
}

State* GetState(Player const* player)
{
    return player ? const_cast<Player*>(player)->CustomData.GetDefault<State>(SettingsSource) : nullptr;
}

uint32 ManaFor(Player const* player)
{
    if (!player)
        return 0;
    float value = float(Settings.ManaBasePerLevel) * player->GetLevel()
        + Settings.ManaIntellectMultiplier * player->GetStat(STAT_INTELLECT);
    return uint32(std::clamp(value, 1.0f, 100000.0f));
}

void EnsureShieldSupport(Player* player)
{
    if (!IsBloodKnight(player))
        return;

    // Stock 3.3.5 Shield proficiency and Block passive, independent of mana and race.
    for (uint32 spell : {9116u, 107u})
        if (!player->HasSpell(spell))
            player->learnSpell(spell, false);

    // Learning the spell alone need not grant a skill for a DK's DBC race/class combination.
    if (!player->HasSkill(SKILL_SHIELD))
        player->SetSkill(SKILL_SHIELD, 0, 1, 1);

    // Repair runtime capability too when the spells were already known.
    constexpr uint32 shieldMask = 1u << ITEM_SUBCLASS_ARMOR_SHIELD;
    if (!(player->GetArmorProficiency() & shieldMask))
    {
        player->AddArmorProficiency(shieldMask);
        player->SendProficiency(ITEM_CLASS_ARMOR, player->GetArmorProficiency());
    }
    if (!player->CanBlock())
        player->SetCanBlock(true);
}

void Refresh(Player* player)
{
    EnsureShieldSupport(player);
    if (!IsBloodKnight(player) || !Settings.ManaEnable)
        return;
    State* state = GetState(player);
    uint32 oldMana = player->GetPower(POWER_MANA);
    player->SetMaxPower(POWER_MANA, ManaFor(player));
    if (!state->ManaInitialized)
    {
        bool initialized = player->GetPlayerSetting(SettingsSource, 0).IsEnabled();
        uint32 saved = player->GetPlayerSetting(SettingsSource, 1).value;
        player->SetPower(POWER_MANA, initialized ? std::min(saved, player->GetMaxPower(POWER_MANA)) : player->GetMaxPower(POWER_MANA));
        state->ManaInitialized = true;
    }
    else
        player->SetPower(POWER_MANA, std::min(oldMana, player->GetMaxPower(POWER_MANA)));

    LOG_INFO("module.roleplay", "Blood Knight resource setup for {}: mana {}/{}, active power {}",
        player->GetName(), player->GetPower(POWER_MANA), player->GetMaxPower(POWER_MANA), uint32(player->getPowerType()));

    for (SpellBand const& band : SpellBands)
    {
        uint32 wanted = (Settings.*band.Enabled && player->GetLevel() >= band.Level) ? RankFor(band, player->GetLevel()) : 0;
        for (uint32 rank : band.Ranks)
            if (rank && rank != wanted && player->HasSpell(rank))
                player->removeSpell(rank, 0, false);
        if (wanted && !player->HasSpell(wanted))
            player->learnSpell(wanted, false);
    }
}

void Save(Player* player)
{
    if (!IsBloodKnight(player) || !Settings.ManaEnable)
        return;
    player->UpdatePlayerSetting(SettingsSource, 0, 1);
    player->UpdatePlayerSetting(SettingsSource, 1, player->GetPower(POWER_MANA));
}

void AddScripts() { }
}
