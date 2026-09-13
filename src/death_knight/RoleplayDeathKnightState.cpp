#include "RoleplayDeathKnight.h"
#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "QuestDef.h"
#include "StringConvert.h"
#include "Tokenize.h"

namespace Roleplay::DeathKnight
{
State* GetState(Player const* player)
{
    return player ? player->CustomData.Get<State>(SettingsSource) : nullptr;
}

bool IsManaged(Player const* player)
{
    State* state = GetState(player);
    return GetConfig().Enable && player && player->getClass() == CLASS_DEATH_KNIGHT
        && state && state->Progress != Unmanaged;
}

void LoadEarlyState(Player* player)
{
    if (player->getClass() != CLASS_DEATH_KNIGHT)
        return;
    // _LoadCharacterSettings occurs AFTER InitTalentForLevel. Reading here prevents a
    // map-609 talent reset before OnPlayerLogin can run. Reuse the core prepared statement.
    State* state = player->CustomData.GetDefault<State>(SettingsSource);
    auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHAR_SETTINGS);
    stmt->SetData(0, player->GetGUID().GetCounter());
    auto result = CharacterDatabase.Query(stmt);
    if (!result)
        return;
    do
    {
        Field* fields = result->Fetch();
        if (fields[0].Get<std::string>() != SettingsSource)
            continue;
        auto settings = PlayerSettingsStore::ParseSettingsData(fields[1].Get<std::string>());
        if (!settings.empty() && settings[0].value <= Arrived)
            state->Progress = static_cast<Stage>(settings[0].value);
        if (settings.size() >= TaxiMaskSize + 2 && settings[1].value == 1)
        {
            state->TrackTaxi = true;
            for (uint32 i = 0; i < TaxiMaskSize; ++i)
                state->Taxi[i] = settings[i + 2].value;
        }
        break;
    } while (result->NextRow());
}

void SaveState(Player* player)
{
    State* state = GetState(player);
    if (!state || state->Progress == Unmanaged)
        return;
    player->UpdatePlayerSetting(SettingsSource, 0, state->Progress);
    player->UpdatePlayerSetting(SettingsSource, 1, state->TrackTaxi ? 1 : 0);
    if (state->TrackTaxi)
        for (uint32 i = 0; i < TaxiMaskSize; ++i)
            player->UpdatePlayerSetting(SettingsSource, i + 2, state->Taxi[i]);
}

bool HasCampaignHistory(Player* player)
{
    // All stock DK intro quests use sort -372. Include the finale explicitly and
    // Scarlet Enclave quests, so a migrated character with partial history is safe too.
    auto isCampaign = [](uint32 id)
    {
        if (id == 13188 || id == 13189 || id == 12687)
            return true;
        Quest const* quest = sObjectMgr->GetQuestTemplate(id);
        return quest && (quest->GetZoneOrSort() == -372 || quest->GetZoneOrSort() == 4298);
    };
    for (uint32 quest : player->getRewardedQuests())
        if (isCampaign(quest))
            return true;
    for (auto const& [id, status] : player->getQuestStatusMap())
        if (status.Status != QUEST_STATUS_NONE && isCampaign(id))
            return true;
    return false;
}

void ValidateLogin(Player* player)
{
    if (!GetConfig().Enable || player->getClass() != CLASS_DEATH_KNIGHT)
        return;
    if (!IsManaged(player))
    {
        if (HasCampaignHistory(player))
            return;
        // A crash between stock creation's commit and OnPlayerCreate is recoverable.
        if (player->HasAtLoginFlag(AT_LOGIN_FIRST) && player->GetLevel() <= GetConfig().StartLevel)
            InitializeCharacter(player);
        else if (GetConfig().RecoverExistingCharacters && player->GetLevel() >= 55 && player->GetMapId() != 609)
        {
            // Existing DKs already in 609 are ambiguous stock characters: leave them alone.
            player->CustomData.GetDefault<State>(SettingsSource)->Progress = Leveling;
            SaveState(player);
            player->SaveToDB(false, false);
        }
        else
            return;
    }
    RestoreTaxi(player);
    ApplyProgression(player);
    OnArrival(player);
    ScheduleCampaign(player);
}
}
