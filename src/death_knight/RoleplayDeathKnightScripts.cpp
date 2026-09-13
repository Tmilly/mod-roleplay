#include "RoleplayDeathKnight.h"
#include "../blood_knight/BloodKnight.h"
#include "Chat.h"
#include "Creature.h"
#include "Item.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"

namespace Roleplay::DeathKnight
{
namespace
{
class WorldHooks : public WorldScript
{
public:
    WorldHooks() : WorldScript("RoleplayDeathKnightWorld") { }
    void OnAfterConfigLoad(bool reload) override { LoadConfig(reload); }
    void OnStartup() override { PrepareCreationData(); }
};

class PlayerHooks : public PlayerScript
{
public:
    PlayerHooks() : PlayerScript("RoleplayDeathKnightPlayer") { }
    void OnPlayerCreate(Player* player) override { InitializeCharacter(player); }
    void OnPlayerLoadFromDB(Player* player) override { LoadEarlyState(player); }
    void OnPlayerLogin(Player* player) override { ValidateLogin(player); }
    void OnPlayerFirstLogin(Player* player) override
    {
        if (GetConfig().Enable && player->getClass() == CLASS_DEATH_KNIGHT && !IsManaged(player)
            && player->GetLevel() <= GetConfig().StartLevel && !HasCampaignHistory(player))
            InitializeCharacter(player);
    }
    void OnPlayerSave(Player* player) override { SaveState(player); }
    void OnPlayerMapChanged(Player* player) override { OnArrival(player); }
    void OnPlayerLearnTaxiNode(Player const* player, uint32 node) override { RecordTaxi(player, node); }
    void OnPlayerUpdate(Player* player, uint32 diff) override
    {
        if (IsManaged(player))
            GetState(player)->Scheduler.Update(diff);
    }

    void OnPlayerLevelChanged(Player* player, uint8 oldLevel) override
    {
        if (!IsManaged(player) && !Roleplay::BloodKnight::IsBloodKnight(player))
            return;
        RestoreTaxi(player);
        ApplyProgression(player);
        if (oldLevel < GetConfig().AcherusCampaignLevel && player->GetLevel() >= GetConfig().AcherusCampaignLevel)
        {
            // Persist reaching the threshold even if the world crashes before the scheduled transfer.
            player->SaveToDB(false, false);
            ScheduleCampaign(player);
        }
    }

    void OnPlayerCalculateTalentsPoints(Player const* player, uint32& points) override
    {
        if (State* state = GetState(player); state && state->Progress != Unmanaged)
            points = LevelTalentPoints(player->GetLevel());
        // Core applies Rate.Talent AFTER this hook. Quest BonusTalents remain recorded normally,
        // but cannot contribute here. This also intentionally excludes extra bonus talent counters.
    }

    bool OnPlayerCanEquipItem(Player* player, uint8 /*slot*/, uint16& /*dest*/, Item* item,
        bool /*swap*/, bool /*notLoading*/) override
    {
        return !IsManaged(player) || !item || player->GetLevel() >= GetConfig().PlateLevel
            || item->GetTemplate()->Class != ITEM_CLASS_ARMOR
            || item->GetTemplate()->SubClass != ITEM_SUBCLASS_ARMOR_PLATE;
    }
};

class TrainerScript : public CreatureScript
{
public:
    TrainerScript() : CreatureScript("npc_roleplay_death_knight_trainer") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (!IsManaged(player))
        {
            CloseGossipMenuFor(player);
            return true;
        }
        ApplyProgression(player);
        // Use the SAME stock trainer 13 for early rows and later ranks, so IP's trainer edits,
        // costs, prerequisites and talent requirements stay in effect. Campaign-only spells
        // (Runeforging/Death Gate) are deliberately absent until their stock quests teach them.
        player->GetSession()->SendTrainerList(creature);
        return true;
    }
};
}
}

void AddRoleplayDeathKnightScripts()
{
    new Roleplay::DeathKnight::WorldHooks();
    new Roleplay::DeathKnight::PlayerHooks();
    new Roleplay::DeathKnight::TrainerScript();
    Roleplay::DeathKnight::AddScalingScripts();
}
