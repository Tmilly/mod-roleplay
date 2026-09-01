#include "roleplay.h"

#include "Chat.h"
#include "CommandScript.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Item.h"
#include "Log.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"
#include "../../mod-playerbots/src/Bot/PlayerbotMgr.h"
#include "../../mod-transmog/src/Transmogrification.h"

#include <array>
#include <charconv>
#include <cctype>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using namespace Acore::ChatCommands;

namespace
{
enum class RecruitBehavior : uint8
{
    Follow = 0,
    Stay = 1
};

enum class RecruitLifecycle : uint8
{
    Alive = 0,
    Dead = 1,
    Dismissed = 2
};

struct RecruitRecord
{
    uint64 Id = 0;
    ObjectGuid::LowType OwnerGuid = 0;
    uint32 CreatureEntry = 0;
    std::string Name;
    uint16 MapId = 0;
    float PositionX = 0.0f;
    float PositionY = 0.0f;
    float PositionZ = 0.0f;
    float Orientation = 0.0f;
    RecruitBehavior Behavior = RecruitBehavior::Follow;
    RecruitLifecycle Lifecycle = RecruitLifecycle::Alive;
};

struct OutfitSession
{
    std::string Name;
    std::array<uint32, EQUIPMENT_SLOT_END> Baseline = { };
};

struct OutfitSlot
{
    uint8 Slot = 0;
    uint32 AppearanceEntry = 0;
};

constexpr std::array<uint8, 11> SupportedOutfitSlots =
{
    EQUIPMENT_SLOT_HEAD,
    EQUIPMENT_SLOT_SHOULDERS,
    EQUIPMENT_SLOT_BODY,
    EQUIPMENT_SLOT_CHEST,
    EQUIPMENT_SLOT_WAIST,
    EQUIPMENT_SLOT_LEGS,
    EQUIPMENT_SLOT_FEET,
    EQUIPMENT_SLOT_WRISTS,
    EQUIPMENT_SLOT_HANDS,
    EQUIPMENT_SLOT_BACK,
    EQUIPMENT_SLOT_TABARD
};

constexpr float RecruitFollowAngle = 3.14159265f;

std::string TrimAndUnquote(std::string_view value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
        value.remove_prefix(1);
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
        value.remove_suffix(1);

    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
    {
        value.remove_prefix(1);
        value.remove_suffix(1);
    }

    return std::string(value);
}

char const* RecruitBehaviorName(RecruitBehavior behavior)
{
    return behavior == RecruitBehavior::Follow ? "FOLLOWING" : "STAYING";
}

char const* RecruitLifecycleName(RecruitLifecycle lifecycle)
{
    switch (lifecycle)
    {
        case RecruitLifecycle::Alive:
            return "ALIVE";
        case RecruitLifecycle::Dead:
            return "DEAD";
        case RecruitLifecycle::Dismissed:
            return "DISMISSED";
    }

    return "UNKNOWN";
}

char const* OutfitSlotName(uint8 slot)
{
    switch (slot)
    {
        case EQUIPMENT_SLOT_HEAD:
            return "HEAD";
        case EQUIPMENT_SLOT_SHOULDERS:
            return "SHOULDERS";
        case EQUIPMENT_SLOT_BODY:
            return "SHIRT";
        case EQUIPMENT_SLOT_CHEST:
            return "CHEST";
        case EQUIPMENT_SLOT_WAIST:
            return "WAIST";
        case EQUIPMENT_SLOT_LEGS:
            return "LEGS";
        case EQUIPMENT_SLOT_FEET:
            return "FEET";
        case EQUIPMENT_SLOT_WRISTS:
            return "WRISTS";
        case EQUIPMENT_SLOT_HANDS:
            return "HANDS";
        case EQUIPMENT_SLOT_BACK:
            return "BACK";
        case EQUIPMENT_SLOT_TABARD:
            return "TABARD";
        default:
            return "UNKNOWN";
    }
}

uint32 GetEffectiveAppearance(Player* player, uint8 slot)
{
    Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
    if (!item)
        return 0;

    uint32 fakeEntry = sTransmogrification->GetFakeEntry(item->GetGUID());
    return fakeEntry ? fakeEntry : item->GetEntry();
}

class RoleplayPhaseOneManager
{
public:
    static RoleplayPhaseOneManager& Instance()
    {
        static RoleplayPhaseOneManager instance;
        return instance;
    }

    bool CreateRecruit(Player* owner, uint32 creatureEntry, std::string name, uint64& recruitId)
    {
        if (!owner || !sObjectMgr->GetCreatureTemplate(creatureEntry) || owner->GetMap()->Instanceable())
            return false;

        uint64 creationToken = (uint64(GameTime::GetGameTime().count()) << 32) | uint64(rand32());
        std::string escapedName = name;
        CharacterDatabase.EscapeString(escapedName);
        Position position = owner->GetNearPosition(3.0f, RecruitFollowAngle);

        CharacterDatabase.DirectExecute(
            "INSERT INTO `rp_recruit` (`owner_guid`, `creature_entry`, `rp_name`, `map_id`, `position_x`, "
            "`position_y`, `position_z`, `orientation`, `behavior_state`, `lifecycle_state`, `creation_token`) "
            "VALUES ({}, {}, '{}', {}, {}, {}, {}, {}, {}, {}, {})",
            owner->GetGUID().GetCounter(), creatureEntry, escapedName, owner->GetMapId(), position.GetPositionX(),
            position.GetPositionY(), position.GetPositionZ(), position.GetOrientation(),
            static_cast<uint8>(RecruitBehavior::Follow), static_cast<uint8>(RecruitLifecycle::Alive), creationToken);

        QueryResult result = CharacterDatabase.Query(
            "SELECT `id` FROM `rp_recruit` WHERE `creation_token` = {}", creationToken);
        if (!result)
            return false;

        recruitId = (*result)[0].Get<uint64>();
        RecruitRecord record;
        record.Id = recruitId;
        record.OwnerGuid = owner->GetGUID().GetCounter();
        record.CreatureEntry = creatureEntry;
        record.Name = std::move(name);
        record.MapId = owner->GetMapId();
        record.PositionX = position.GetPositionX();
        record.PositionY = position.GetPositionY();
        record.PositionZ = position.GetPositionZ();
        record.Orientation = position.GetOrientation();
        _recruits[record.Id] = record;
        _ownerRecruits[record.OwnerGuid].insert(record.Id);

        if (!SpawnRecruit(owner, _recruits[record.Id], true))
        {
            SetLifecycle(record.Id, RecruitLifecycle::Dismissed);
            return false;
        }

        LOG_INFO("module.roleplay", "[RP] Created recruit id={} owner={} entry={}", recruitId, owner->GetName(),
            creatureEntry);
        return true;
    }

    void LoadOwnerRecruits(Player* owner)
    {
        if (!owner)
            return;

        UnloadOwnerRecruits(owner, false);
        ObjectGuid::LowType ownerGuid = owner->GetGUID().GetCounter();
        QueryResult result = CharacterDatabase.Query(
            "SELECT `id`, `creature_entry`, `rp_name`, `map_id`, `position_x`, `position_y`, `position_z`, "
            "`orientation`, `behavior_state`, `lifecycle_state` FROM `rp_recruit` "
            "WHERE `owner_guid` = {} AND `lifecycle_state` = {}",
            ownerGuid, static_cast<uint8>(RecruitLifecycle::Alive));
        if (!result)
            return;

        do
        {
            Field* fields = result->Fetch();
            RecruitRecord record;
            record.Id = fields[0].Get<uint64>();
            record.OwnerGuid = ownerGuid;
            record.CreatureEntry = fields[1].Get<uint32>();
            record.Name = fields[2].Get<std::string>();
            record.MapId = fields[3].Get<uint16>();
            record.PositionX = fields[4].Get<float>();
            record.PositionY = fields[5].Get<float>();
            record.PositionZ = fields[6].Get<float>();
            record.Orientation = fields[7].Get<float>();
            record.Behavior = static_cast<RecruitBehavior>(fields[8].Get<uint8>());
            record.Lifecycle = static_cast<RecruitLifecycle>(fields[9].Get<uint8>());
            _recruits[record.Id] = record;
            _ownerRecruits[ownerGuid].insert(record.Id);
        } while (result->NextRow());

        SpawnEligibleRecruits(owner);
    }

    void UnloadOwnerRecruits(Player* owner, bool persistPositions)
    {
        if (!owner)
            return;

        ObjectGuid::LowType ownerGuid = owner->GetGUID().GetCounter();
        auto ownerItr = _ownerRecruits.find(ownerGuid);
        if (ownerItr == _ownerRecruits.end())
            return;

        std::vector<uint64> recruitIds(ownerItr->second.begin(), ownerItr->second.end());
        for (uint64 recruitId : recruitIds)
        {
            if (persistPositions)
                PersistCurrentPosition(recruitId);
            DespawnRecruit(recruitId);
            _recruits.erase(recruitId);
        }

        _ownerRecruits.erase(ownerItr);
    }

    void PrepareOwnerTeleport(Player* owner)
    {
        if (!owner)
            return;

        auto ownerItr = _ownerRecruits.find(owner->GetGUID().GetCounter());
        if (ownerItr == _ownerRecruits.end())
            return;

        for (uint64 recruitId : ownerItr->second)
        {
            auto recordItr = _recruits.find(recruitId);
            if (recordItr != _recruits.end() && recordItr->second.Behavior == RecruitBehavior::Follow)
                DespawnRecruit(recruitId);
        }
    }

    void SpawnEligibleRecruits(Player* owner)
    {
        if (!owner || !owner->IsInWorld() || owner->GetMap()->Instanceable())
            return;

        auto ownerItr = _ownerRecruits.find(owner->GetGUID().GetCounter());
        if (ownerItr == _ownerRecruits.end())
            return;

        for (uint64 recruitId : ownerItr->second)
        {
            auto recordItr = _recruits.find(recruitId);
            if (recordItr == _recruits.end() || recordItr->second.Lifecycle != RecruitLifecycle::Alive ||
                _activeCreatures.contains(recruitId))
            {
                continue;
            }

            RecruitRecord& record = recordItr->second;
            if (record.Behavior == RecruitBehavior::Follow || record.MapId == owner->GetMapId())
                SpawnRecruit(owner, record, record.Behavior == RecruitBehavior::Follow);
        }
    }

    RecruitRecord* GetSelectedOwnedRecruit(Player* owner, Creature*& creature)
    {
        creature = nullptr;
        if (!owner)
            return nullptr;

        Unit* selected = owner->GetSelectedUnit();
        if (!selected || !selected->IsCreature())
            return nullptr;

        creature = selected->ToCreature();
        auto activeItr = _recruitByCreature.find(creature->GetGUID());
        if (activeItr == _recruitByCreature.end())
            return nullptr;

        auto recordItr = _recruits.find(activeItr->second);
        if (recordItr == _recruits.end() || recordItr->second.OwnerGuid != owner->GetGUID().GetCounter())
            return nullptr;

        return &recordItr->second;
    }

    bool SetRecruitBehavior(Player* owner, RecruitBehavior behavior)
    {
        Creature* creature = nullptr;
        RecruitRecord* record = GetSelectedOwnedRecruit(owner, creature);
        if (!record || !creature || !creature->IsAlive())
            return false;

        RecruitBehavior oldBehavior = record->Behavior;
        record->Behavior = behavior;
        if (behavior == RecruitBehavior::Stay)
        {
            record->MapId = creature->GetMapId();
            record->PositionX = creature->GetPositionX();
            record->PositionY = creature->GetPositionY();
            record->PositionZ = creature->GetPositionZ();
            record->Orientation = creature->GetOrientation();
        }

        PersistBehavior(*record);
        ApplyBehavior(owner, creature, *record);
        LOG_INFO("module.roleplay", "[RP] Recruit id={} changed state {} -> {}", record->Id,
            RecruitBehaviorName(oldBehavior), RecruitBehaviorName(behavior));
        return true;
    }

    bool DismissSelectedRecruit(Player* owner, uint64& recruitId)
    {
        Creature* creature = nullptr;
        RecruitRecord* record = GetSelectedOwnedRecruit(owner, creature);
        if (!record)
            return false;

        recruitId = record->Id;
        ObjectGuid::LowType ownerGuid = record->OwnerGuid;
        SetLifecycle(record->Id, RecruitLifecycle::Dismissed);
        DespawnRecruit(record->Id);
        _ownerRecruits[ownerGuid].erase(record->Id);
        _recruits.erase(record->Id);
        LOG_INFO("module.roleplay", "[RP] Dismissed recruit id={} owner={}", recruitId, owner->GetName());
        return true;
    }

    void HandleRecruitDeath(Unit* unit)
    {
        if (!unit || !unit->IsCreature())
            return;

        auto activeItr = _recruitByCreature.find(unit->GetGUID());
        if (activeItr == _recruitByCreature.end())
            return;

        uint64 recruitId = activeItr->second;
        auto recordItr = _recruits.find(recruitId);
        if (recordItr == _recruits.end())
            return;

        RecruitRecord& record = recordItr->second;
        record.MapId = unit->GetMapId();
        record.PositionX = unit->GetPositionX();
        record.PositionY = unit->GetPositionY();
        record.PositionZ = unit->GetPositionZ();
        record.Orientation = unit->GetOrientation();
        record.Lifecycle = RecruitLifecycle::Dead;
        CharacterDatabase.DirectExecute(
            "UPDATE `rp_recruit` SET `map_id` = {}, `position_x` = {}, `position_y` = {}, `position_z` = {}, "
            "`orientation` = {}, `lifecycle_state` = {}, `died_at` = CURRENT_TIMESTAMP WHERE `id` = {}",
            record.MapId, record.PositionX, record.PositionY, record.PositionZ, record.Orientation,
            static_cast<uint8>(RecruitLifecycle::Dead), record.Id);

        _activeCreatures.erase(recruitId);
        _recruitByCreature.erase(activeItr);
        LOG_INFO("module.roleplay", "[RP] Recruit id={} died permanently", recruitId);
    }

    void ReapplyBehavior(Unit* unit)
    {
        if (!unit || !unit->IsCreature() || !unit->IsAlive())
            return;

        auto activeItr = _recruitByCreature.find(unit->GetGUID());
        if (activeItr == _recruitByCreature.end())
            return;

        auto recordItr = _recruits.find(activeItr->second);
        if (recordItr == _recruits.end())
            return;

        Player* owner = ObjectAccessor::FindConnectedPlayer(
            ObjectGuid::Create<HighGuid::Player>(recordItr->second.OwnerGuid));
        if (owner)
            ApplyBehavior(owner, unit->ToCreature(), recordItr->second);
    }

    void AssistOwner(Player* owner, Unit* enemy)
    {
        if (!owner || !enemy)
            return;

        auto ownerItr = _ownerRecruits.find(owner->GetGUID().GetCounter());
        if (ownerItr == _ownerRecruits.end())
            return;

        for (uint64 recruitId : ownerItr->second)
        {
            Creature* creature = GetActiveCreature(owner, recruitId);
            if (creature && creature->IsAlive() && creature->IsHostileTo(enemy) && creature->IsWithinDistInMap(enemy, 40.0f))
                creature->AI()->AttackStart(enemy);
        }
    }

    void CancelOutfitSession(Player* player)
    {
        if (player)
            _outfitSessions.erase(player->GetGUID().GetCounter());
    }

    bool BeginOutfitSession(Player* player, std::string name)
    {
        if (!player || name.empty() || name.size() > 64)
            return false;

        OutfitSession session;
        session.Name = std::move(name);
        for (uint8 slot : SupportedOutfitSlots)
            session.Baseline[slot] = GetEffectiveAppearance(player, slot);
        _outfitSessions[player->GetGUID().GetCounter()] = std::move(session);
        return true;
    }

    bool SaveOutfitSession(Player* player, std::string& outfitName, std::vector<OutfitSlot>& capturedSlots)
    {
        if (!player)
            return false;

        auto sessionItr = _outfitSessions.find(player->GetGUID().GetCounter());
        if (sessionItr == _outfitSessions.end())
            return false;

        OutfitSession const& session = sessionItr->second;
        for (uint8 slot : SupportedOutfitSlots)
        {
            uint32 currentAppearance = GetEffectiveAppearance(player, slot);
            if (currentAppearance && currentAppearance != session.Baseline[slot])
                capturedSlots.push_back({ slot, currentAppearance });
        }

        if (capturedSlots.empty())
            return false;

        outfitName = session.Name;
        std::string escapedName = outfitName;
        CharacterDatabase.EscapeString(escapedName);
        QueryResult result = CharacterDatabase.Query(
            "SELECT `id`, `creator_guid` FROM `rp_outfit` WHERE `name` = '{}'", escapedName);
        uint64 outfitId = 0;
        if (result)
        {
            if ((*result)[1].Get<ObjectGuid::LowType>() != player->GetGUID().GetCounter())
                return false;
            outfitId = (*result)[0].Get<uint64>();
            CharacterDatabase.DirectExecute(
                "UPDATE `rp_outfit` SET `creator_guid` = {}, `updated_at` = CURRENT_TIMESTAMP WHERE `id` = {}",
                player->GetGUID().GetCounter(), outfitId);
            CharacterDatabase.DirectExecute("DELETE FROM `rp_outfit_slot` WHERE `outfit_id` = {}", outfitId);
        }
        else
        {
            CharacterDatabase.DirectExecute(
                "INSERT INTO `rp_outfit` (`name`, `creator_guid`) VALUES ('{}', {})",
                escapedName, player->GetGUID().GetCounter());
            result = CharacterDatabase.Query("SELECT `id` FROM `rp_outfit` WHERE `name` = '{}'", escapedName);
            if (!result)
                return false;
            outfitId = (*result)[0].Get<uint64>();
        }

        for (OutfitSlot const& slot : capturedSlots)
        {
            CharacterDatabase.DirectExecute(
                "INSERT INTO `rp_outfit_slot` (`outfit_id`, `equipment_slot`, `appearance_entry`, `operation`) "
                "VALUES ({}, {}, {}, 1)", outfitId, slot.Slot, slot.AppearanceEntry);
        }

        _outfitSessions.erase(sessionItr);
        LOG_INFO("module.roleplay", "[RP] Saved outfit '{}' with {} controlled slots", outfitName,
            capturedSlots.size());
        return true;
    }

    std::vector<std::pair<uint64, std::string>> ListOutfits()
    {
        std::vector<std::pair<uint64, std::string>> outfits;
        QueryResult result = CharacterDatabase.Query("SELECT `id`, `name` FROM `rp_outfit` ORDER BY `name`");
        if (!result)
            return outfits;

        do
        {
            outfits.emplace_back((*result)[0].Get<uint64>(), (*result)[1].Get<std::string>());
        } while (result->NextRow());
        return outfits;
    }

    bool GetOutfit(std::string name, uint64& outfitId, std::string& storedName, std::vector<OutfitSlot>& slots)
    {
        CharacterDatabase.EscapeString(name);
        QueryResult result = CharacterDatabase.Query(
            "SELECT `id`, `name` FROM `rp_outfit` WHERE `name` = '{}'", name);
        if (!result)
            return false;

        outfitId = (*result)[0].Get<uint64>();
        storedName = (*result)[1].Get<std::string>();
        result = CharacterDatabase.Query(
            "SELECT `equipment_slot`, `appearance_entry` FROM `rp_outfit_slot` "
            "WHERE `outfit_id` = {} ORDER BY `equipment_slot`", outfitId);
        if (result)
        {
            do
            {
                slots.push_back({ (*result)[0].Get<uint8>(), (*result)[1].Get<uint32>() });
            } while (result->NextRow());
        }
        return true;
    }

    bool ApplyOutfit(Player* bot, uint64 outfitId, std::string const& outfitName, bool logReapply)
    {
        if (!IsPlayerbot(bot))
            return false;

        QueryResult result = CharacterDatabase.Query(
            "SELECT `equipment_slot`, `appearance_entry` FROM `rp_outfit_slot` WHERE `outfit_id` = {}",
            outfitId);
        if (!result)
            return false;

        do
        {
            uint8 slot = (*result)[0].Get<uint8>();
            uint32 appearanceEntry = (*result)[1].Get<uint32>();
            if (slot >= EQUIPMENT_SLOT_END || (!sObjectMgr->GetItemTemplate(appearanceEntry) &&
                appearanceEntry != HIDDEN_ITEM_ID))
            {
                continue;
            }

            if (Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                sTransmogrification->SetFakeEntry(bot, appearanceEntry, slot, item);
        } while (result->NextRow());

        if (logReapply)
            LOG_INFO("module.roleplay", "[RP] Reapplied outfit '{}' to bot {}", outfitName, bot->GetName());
        return true;
    }

    bool ApplyNamedOutfit(Player* bot, std::string name)
    {
        uint64 outfitId = 0;
        std::string storedName;
        std::vector<OutfitSlot> slots;
        return GetOutfit(std::move(name), outfitId, storedName, slots) && ApplyOutfit(bot, outfitId, storedName, false);
    }

    bool AssignOutfit(Player* bot, std::string name, std::string& storedName)
    {
        if (!IsPlayerbot(bot))
            return false;

        uint64 outfitId = 0;
        std::vector<OutfitSlot> slots;
        if (!GetOutfit(std::move(name), outfitId, storedName, slots))
            return false;

        CharacterDatabase.DirectExecute(
            "REPLACE INTO `rp_playerbot_outfit` (`playerbot_guid`, `outfit_id`) VALUES ({}, {})",
            bot->GetGUID().GetCounter(), outfitId);
        ApplyOutfit(bot, outfitId, storedName, false);
        LOG_INFO("module.roleplay", "[RP] Assigned outfit '{}' to bot {}", storedName, bot->GetName());
        return true;
    }

    bool ClearOutfitAssignment(Player* bot)
    {
        if (!IsPlayerbot(bot))
            return false;

        CharacterDatabase.DirectExecute(
            "DELETE FROM `rp_playerbot_outfit` WHERE `playerbot_guid` = {}", bot->GetGUID().GetCounter());
        return true;
    }

    bool DeleteOutfit(Player* player, std::string name, std::string& storedName)
    {
        if (!player)
            return false;

        uint64 outfitId = 0;
        std::vector<OutfitSlot> slots;
        if (!GetOutfit(std::move(name), outfitId, storedName, slots))
            return false;

        QueryResult result = CharacterDatabase.Query(
            "SELECT `creator_guid` FROM `rp_outfit` WHERE `id` = {}", outfitId);
        if (!result || (*result)[0].Get<ObjectGuid::LowType>() != player->GetGUID().GetCounter())
            return false;

        CharacterDatabase.DirectExecute("DELETE FROM `rp_playerbot_outfit` WHERE `outfit_id` = {}", outfitId);
        CharacterDatabase.DirectExecute("DELETE FROM `rp_outfit_slot` WHERE `outfit_id` = {}", outfitId);
        CharacterDatabase.DirectExecute("DELETE FROM `rp_outfit` WHERE `id` = {}", outfitId);
        return true;
    }

    void ReapplyAssignedOutfit(Player* bot, bool logReapply)
    {
        if (!IsPlayerbot(bot))
            return;

        QueryResult result = CharacterDatabase.Query(
            "SELECT o.`id`, o.`name` FROM `rp_playerbot_outfit` a "
            "INNER JOIN `rp_outfit` o ON o.`id` = a.`outfit_id` WHERE a.`playerbot_guid` = {}",
            bot->GetGUID().GetCounter());
        if (!result)
            return;

        ApplyOutfit(bot, (*result)[0].Get<uint64>(), (*result)[1].Get<std::string>(), logReapply);
    }

    void ReapplyAssignedSlot(Player* bot, Item* item, uint8 slot)
    {
        if (!IsPlayerbot(bot) || !item || slot >= EQUIPMENT_SLOT_END)
            return;

        QueryResult result = CharacterDatabase.Query(
            "SELECT s.`appearance_entry`, o.`name` FROM `rp_playerbot_outfit` a "
            "INNER JOIN `rp_outfit` o ON o.`id` = a.`outfit_id` "
            "INNER JOIN `rp_outfit_slot` s ON s.`outfit_id` = o.`id` "
            "WHERE a.`playerbot_guid` = {} AND s.`equipment_slot` = {}",
            bot->GetGUID().GetCounter(), slot);
        if (!result)
            return;

        uint32 appearanceEntry = (*result)[0].Get<uint32>();
        if (appearanceEntry == HIDDEN_ITEM_ID || sObjectMgr->GetItemTemplate(appearanceEntry))
        {
            if (sTransmogrification->GetFakeEntry(item->GetGUID()) == appearanceEntry)
                return;

            sTransmogrification->SetFakeEntry(bot, appearanceEntry, slot, item);
            LOG_INFO("module.roleplay", "[RP] Reapplied outfit '{}' after equipment change on bot {} slot {}",
                (*result)[1].Get<std::string>(), bot->GetName(), OutfitSlotName(slot));
        }
    }

    bool IsPlayerbot(Player* player) const
    {
        return player && sPlayerbotsMgr.GetPlayerbotAI(player) != nullptr;
    }

private:
    bool SpawnRecruit(Player* owner, RecruitRecord& record, bool nearOwner)
    {
        if (!owner || !owner->IsInWorld() || owner->GetMap()->Instanceable())
            return false;

        Position position;
        if (nearOwner)
        {
            position = owner->GetNearPosition(3.0f, RecruitFollowAngle);
            record.MapId = owner->GetMapId();
            record.PositionX = position.GetPositionX();
            record.PositionY = position.GetPositionY();
            record.PositionZ = position.GetPositionZ();
            record.Orientation = position.GetOrientation();
        }
        else
        {
            position.Relocate(record.PositionX, record.PositionY, record.PositionZ, record.Orientation);
        }

        TempSummon* summon = owner->SummonCreature(record.CreatureEntry, position,
            TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60 * IN_MILLISECONDS);
        if (!summon)
            return false;

        summon->SetFaction(owner->GetFaction());
        summon->SetReactState(REACT_DEFENSIVE);
        summon->SetHomePosition(position);
        _activeCreatures[record.Id] = summon->GetGUID();
        _recruitByCreature[summon->GetGUID()] = record.Id;
        ApplyBehavior(owner, summon, record);
        PersistBehavior(record);
        return true;
    }

    Creature* GetActiveCreature(Player* owner, uint64 recruitId)
    {
        auto activeItr = _activeCreatures.find(recruitId);
        if (activeItr == _activeCreatures.end() || !owner)
            return nullptr;
        return ObjectAccessor::GetCreature(*owner, activeItr->second);
    }

    void DespawnRecruit(uint64 recruitId)
    {
        auto activeItr = _activeCreatures.find(recruitId);
        if (activeItr == _activeCreatures.end())
            return;

        ObjectGuid creatureGuid = activeItr->second;
        auto recordItr = _recruits.find(recruitId);
        if (recordItr != _recruits.end())
        {
            Player* owner = ObjectAccessor::FindConnectedPlayer(
                ObjectGuid::Create<HighGuid::Player>(recordItr->second.OwnerGuid));
            if (Creature* creature = GetActiveCreature(owner, recruitId))
                creature->ToTempSummon()->UnSummon();
        }

        _activeCreatures.erase(activeItr);
        _recruitByCreature.erase(creatureGuid);
    }

    void ApplyBehavior(Player* owner, Creature* creature, RecruitRecord const& record)
    {
        if (!owner || !creature || !creature->IsAlive() || creature->IsInCombat())
            return;

        creature->GetMotionMaster()->Clear();
        if (record.Behavior == RecruitBehavior::Follow)
            creature->GetMotionMaster()->MoveFollow(owner, 3.0f, RecruitFollowAngle);
        else
        {
            Position home;
            home.Relocate(record.PositionX, record.PositionY, record.PositionZ, record.Orientation);
            creature->SetHomePosition(home);
            creature->GetMotionMaster()->MoveIdle();
        }
    }

    void PersistBehavior(RecruitRecord const& record)
    {
        CharacterDatabase.DirectExecute(
            "UPDATE `rp_recruit` SET `map_id` = {}, `position_x` = {}, `position_y` = {}, `position_z` = {}, "
            "`orientation` = {}, `behavior_state` = {} WHERE `id` = {}",
            record.MapId, record.PositionX, record.PositionY, record.PositionZ, record.Orientation,
            static_cast<uint8>(record.Behavior), record.Id);
    }

    void PersistCurrentPosition(uint64 recruitId)
    {
        auto recordItr = _recruits.find(recruitId);
        if (recordItr == _recruits.end() || recordItr->second.Behavior != RecruitBehavior::Stay)
            return;

        Player* owner = ObjectAccessor::FindConnectedPlayer(
            ObjectGuid::Create<HighGuid::Player>(recordItr->second.OwnerGuid));
        Creature* creature = GetActiveCreature(owner, recruitId);
        if (!creature)
            return;

        RecruitRecord& record = recordItr->second;
        record.MapId = creature->GetMapId();
        record.PositionX = creature->GetPositionX();
        record.PositionY = creature->GetPositionY();
        record.PositionZ = creature->GetPositionZ();
        record.Orientation = creature->GetOrientation();
        PersistBehavior(record);
    }

    void SetLifecycle(uint64 recruitId, RecruitLifecycle lifecycle)
    {
        CharacterDatabase.DirectExecute(
            "UPDATE `rp_recruit` SET `lifecycle_state` = {} WHERE `id` = {}",
            static_cast<uint8>(lifecycle), recruitId);
        auto recordItr = _recruits.find(recruitId);
        if (recordItr != _recruits.end())
            recordItr->second.Lifecycle = lifecycle;
    }

    std::unordered_map<uint64, RecruitRecord> _recruits;
    std::unordered_map<ObjectGuid::LowType, std::set<uint64>> _ownerRecruits;
    std::unordered_map<uint64, ObjectGuid> _activeCreatures;
    std::unordered_map<ObjectGuid, uint64> _recruitByCreature;
    std::unordered_map<ObjectGuid::LowType, OutfitSession> _outfitSessions;
};

class RoleplayPhaseOnePlayerScript : public PlayerScript
{
public:
    RoleplayPhaseOnePlayerScript() : PlayerScript("RoleplayPhaseOnePlayerScript") { }

    void OnPlayerLogin(Player* player) override
    {
        RoleplayPhaseOneManager::Instance().LoadOwnerRecruits(player);
        RoleplayPhaseOneManager::Instance().ReapplyAssignedOutfit(player, false);
    }

    void OnPlayerBeforeLogout(Player* player) override
    {
        RoleplayPhaseOneManager::Instance().CancelOutfitSession(player);
        RoleplayPhaseOneManager::Instance().UnloadOwnerRecruits(player, true);
    }

    bool OnPlayerBeforeTeleport(Player* player, uint32 mapid, float /*x*/, float /*y*/, float /*z*/,
        float /*orientation*/, uint32 /*options*/, Unit* /*target*/) override
    {
        if (mapid != player->GetMapId())
            RoleplayPhaseOneManager::Instance().PrepareOwnerTeleport(player);
        return true;
    }

    void OnPlayerUpdateZone(Player* player, uint32 /*newZone*/, uint32 /*newArea*/) override
    {
        RoleplayPhaseOneManager::Instance().SpawnEligibleRecruits(player);
    }

    void OnPlayerEnterCombat(Player* player, Unit* enemy) override
    {
        RoleplayPhaseOneManager::Instance().AssistOwner(player, enemy);
    }

    void OnPlayerEquip(Player* player, Item* item, uint8 bag, uint8 slot, bool /*update*/) override
    {
        if (bag == INVENTORY_SLOT_BAG_0)
            RoleplayPhaseOneManager::Instance().ReapplyAssignedSlot(player, item, slot);
    }

    void OnPlayerAfterSetVisibleItemSlot(Player* player, uint8 slot, Item* item) override
    {
        RoleplayPhaseOneManager::Instance().ReapplyAssignedSlot(player, item, slot);
    }
};

class RoleplayPhaseOneUnitScript : public UnitScript
{
public:
    RoleplayPhaseOneUnitScript() : UnitScript("RoleplayPhaseOneUnitScript") { }

    void OnUnitDeath(Unit* unit, Unit* /*killer*/) override
    {
        RoleplayPhaseOneManager::Instance().HandleRecruitDeath(unit);
    }

    void OnUnitExitCombat(Unit* unit) override
    {
        RoleplayPhaseOneManager::Instance().ReapplyBehavior(unit);
    }
};

class RoleplayPhaseOneCommandScript : public CommandScript
{
public:
    RoleplayPhaseOneCommandScript() : CommandScript("RoleplayPhaseOneCommandScript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable recruitTable =
        {
            { "create",  HandleRecruitCreate,  SEC_PLAYER, Console::No },
            { "follow",  HandleRecruitFollow,  SEC_PLAYER, Console::No },
            { "stay",    HandleRecruitStay,    SEC_PLAYER, Console::No },
            { "info",    HandleRecruitInfo,    SEC_PLAYER, Console::No },
            { "dismiss", HandleRecruitDismiss, SEC_PLAYER, Console::No }
        };
        static ChatCommandTable outfitTable =
        {
            { "begin",  HandleOutfitBegin,  SEC_PLAYER, Console::No },
            { "save",   HandleOutfitSave,   SEC_PLAYER, Console::No },
            { "cancel", HandleOutfitCancel, SEC_PLAYER, Console::No },
            { "list",   HandleOutfitList,   SEC_PLAYER, Console::No },
            { "info",   HandleOutfitInfo,   SEC_PLAYER, Console::No },
            { "apply",  HandleOutfitApply,  SEC_PLAYER, Console::No },
            { "assign", HandleOutfitAssign, SEC_PLAYER, Console::No },
            { "clear",  HandleOutfitClear,  SEC_PLAYER, Console::No },
            { "delete", HandleOutfitDelete, SEC_PLAYER, Console::No }
        };
        static ChatCommandTable ironbellyTable =
        {
            { "learn", HandleIronbellyLearn, SEC_PLAYER, Console::No }
        };
        static ChatCommandTable roleplayTable =
        {
            { "recruit", recruitTable },
            { "outfit", outfitTable },
            { "ironbelly", ironbellyTable }
        };
        static ChatCommandTable commandTable =
        {
            { "rp", roleplayTable }
        };
        return commandTable;
    }

private:
    static Player* GetPlayer(ChatHandler* handler)
    {
        return handler && handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
    }

    static Player* GetSelectedPlayerbot(ChatHandler* handler)
    {
        Player* player = GetPlayer(handler);
        Player* target = player ? player->GetSelectedPlayer() : nullptr;
        if (!target || !RoleplayPhaseOneManager::Instance().IsPlayerbot(target))
        {
            handler->SendSysMessage("Select a currently loaded Playerbot first.");
            return nullptr;
        }
        return target;
    }

    static bool HandleRecruitCreate(ChatHandler* handler, std::string_view args)
    {
        std::size_t separator = args.find(' ');
        std::string_view entryText = args.substr(0, separator);
        uint32 creatureEntry = 0;
        auto parseResult = std::from_chars(entryText.data(), entryText.data() + entryText.size(), creatureEntry);
        if (parseResult.ec != std::errc() || parseResult.ptr != entryText.data() + entryText.size() || !creatureEntry)
            return false;

        std::string name;
        if (separator != std::string_view::npos)
            name = TrimAndUnquote(args.substr(separator + 1));
        if (name.size() > 64)
        {
            handler->SendSysMessage("Recruit names may not exceed 64 characters.");
            return true;
        }

        Player* player = GetPlayer(handler);
        if (!sObjectMgr->GetCreatureTemplate(creatureEntry))
        {
            handler->PSendSysMessage("Creature template {} does not exist.", creatureEntry);
            return true;
        }
        if (player->GetMap()->Instanceable())
        {
            handler->SendSysMessage("Persistent recruits cannot be created in instances or battlegrounds.");
            return true;
        }

        uint64 recruitId = 0;
        if (!RoleplayPhaseOneManager::Instance().CreateRecruit(player, creatureEntry, name, recruitId))
        {
            handler->SendSysMessage("The recruit could not be created.");
            return true;
        }

        handler->PSendSysMessage("Created RP recruit {} (ID {}).", name.empty() ? "unnamed" : name, recruitId);
        return true;
    }

    static bool HandleRecruitFollow(ChatHandler* handler)
    {
        if (!RoleplayPhaseOneManager::Instance().SetRecruitBehavior(GetPlayer(handler), RecruitBehavior::Follow))
            handler->SendSysMessage("Select one of your living RP recruits first.");
        else
            handler->SendSysMessage("The selected recruit is now following you.");
        return true;
    }

    static bool HandleRecruitStay(ChatHandler* handler)
    {
        if (!RoleplayPhaseOneManager::Instance().SetRecruitBehavior(GetPlayer(handler), RecruitBehavior::Stay))
            handler->SendSysMessage("Select one of your living RP recruits first.");
        else
            handler->SendSysMessage("The selected recruit will hold this position.");
        return true;
    }

    static bool HandleRecruitInfo(ChatHandler* handler)
    {
        Player* player = GetPlayer(handler);
        Creature* creature = nullptr;
        RecruitRecord* record = RoleplayPhaseOneManager::Instance().GetSelectedOwnedRecruit(player, creature);
        if (!record)
        {
            handler->SendSysMessage("Select one of your RP recruits first.");
            return true;
        }

        handler->PSendSysMessage("Name: {}", record->Name.empty() ? "(unnamed)" : record->Name);
        handler->PSendSysMessage("Recruit ID: {}", record->Id);
        handler->PSendSysMessage("Owner: {}", player->GetName());
        handler->PSendSysMessage("Creature entry: {}", record->CreatureEntry);
        handler->PSendSysMessage("State: {}", RecruitBehaviorName(record->Behavior));
        handler->PSendSysMessage("Lifecycle: {}", RecruitLifecycleName(record->Lifecycle));
        handler->PSendSysMessage("Alive: {}", creature && creature->IsAlive() ? "Yes" : "No");
        return true;
    }

    static bool HandleRecruitDismiss(ChatHandler* handler)
    {
        uint64 recruitId = 0;
        if (!RoleplayPhaseOneManager::Instance().DismissSelectedRecruit(GetPlayer(handler), recruitId))
            handler->SendSysMessage("Select one of your RP recruits first.");
        else
            handler->PSendSysMessage("Recruit {} was permanently dismissed.", recruitId);
        return true;
    }

    static bool HandleOutfitBegin(ChatHandler* handler, std::string_view args)
    {
        std::string name = TrimAndUnquote(args);
        if (!RoleplayPhaseOneManager::Instance().BeginOutfitSession(GetPlayer(handler), name))
        {
            handler->SendSysMessage("Usage: .rp outfit begin \"name\" (maximum 64 characters)");
            return true;
        }
        handler->PSendSysMessage("Started sparse outfit session '{}'. Use the transmog NPC, then run .rp outfit save.",
            name);
        return true;
    }

    static bool HandleOutfitSave(ChatHandler* handler)
    {
        std::string name;
        std::vector<OutfitSlot> capturedSlots;
        if (!RoleplayPhaseOneManager::Instance().SaveOutfitSession(GetPlayer(handler), name, capturedSlots))
        {
            handler->SendSysMessage("No active outfit session was found, or no supported armor slots changed.");
            return true;
        }

        std::ostringstream slotList;
        for (std::size_t index = 0; index < capturedSlots.size(); ++index)
        {
            if (index)
                slotList << ", ";
            slotList << OutfitSlotName(capturedSlots[index].Slot);
        }
        handler->PSendSysMessage("Saved outfit '{}' with {} controlled slots: {}", name, capturedSlots.size(),
            slotList.str());
        return true;
    }

    static bool HandleOutfitCancel(ChatHandler* handler)
    {
        RoleplayPhaseOneManager::Instance().CancelOutfitSession(GetPlayer(handler));
        handler->SendSysMessage("Outfit edit session cancelled.");
        return true;
    }

    static bool HandleOutfitList(ChatHandler* handler)
    {
        auto outfits = RoleplayPhaseOneManager::Instance().ListOutfits();
        if (outfits.empty())
        {
            handler->SendSysMessage("No RP outfits have been saved.");
            return true;
        }
        for (auto const& [id, name] : outfits)
            handler->PSendSysMessage("{}: {}", id, name);
        return true;
    }

    static bool HandleOutfitInfo(ChatHandler* handler, std::string_view args)
    {
        uint64 outfitId = 0;
        std::string storedName;
        std::vector<OutfitSlot> slots;
        if (!RoleplayPhaseOneManager::Instance().GetOutfit(TrimAndUnquote(args), outfitId, storedName, slots))
        {
            handler->SendSysMessage("That RP outfit does not exist.");
            return true;
        }
        handler->PSendSysMessage("Outfit: {} (ID {})", storedName, outfitId);
        for (OutfitSlot const& slot : slots)
            handler->PSendSysMessage("{}: appearance item {}", OutfitSlotName(slot.Slot), slot.AppearanceEntry);
        return true;
    }

    static bool HandleOutfitApply(ChatHandler* handler, std::string_view args)
    {
        Player* bot = GetSelectedPlayerbot(handler);
        if (!bot)
            return true;
        std::string name = TrimAndUnquote(args);
        if (!RoleplayPhaseOneManager::Instance().ApplyNamedOutfit(bot, name))
            handler->SendSysMessage("That RP outfit does not exist or contains no usable slots.");
        else
            handler->PSendSysMessage("Applied '{}' to {} once.", name, bot->GetName());
        return true;
    }

    static bool HandleOutfitAssign(ChatHandler* handler, std::string_view args)
    {
        Player* bot = GetSelectedPlayerbot(handler);
        if (!bot)
            return true;
        std::string storedName;
        if (!RoleplayPhaseOneManager::Instance().AssignOutfit(bot, TrimAndUnquote(args), storedName))
            handler->SendSysMessage("That RP outfit does not exist.");
        else
            handler->PSendSysMessage("Assigned '{}' persistently to {}.", storedName, bot->GetName());
        return true;
    }

    static bool HandleOutfitClear(ChatHandler* handler)
    {
        Player* bot = GetSelectedPlayerbot(handler);
        if (!bot)
            return true;
        RoleplayPhaseOneManager::Instance().ClearOutfitAssignment(bot);
        handler->PSendSysMessage("Cleared {}'s RP outfit assignment. Current visuals were left unchanged.", bot->GetName());
        return true;
    }

    static bool HandleOutfitDelete(ChatHandler* handler, std::string_view args)
    {
        std::string storedName;
        if (!RoleplayPhaseOneManager::Instance().DeleteOutfit(GetPlayer(handler), TrimAndUnquote(args), storedName))
            handler->SendSysMessage("That RP outfit does not exist, or you are not its creator.");
        else
            handler->PSendSysMessage("Deleted outfit '{}'. Assigned bots keep their current visuals.", storedName);
        return true;
    }
};
}

void AddRoleplayPhaseOneScripts()
{
    new RoleplayPhaseOnePlayerScript();
    new RoleplayPhaseOneUnitScript();
    new RoleplayPhaseOneCommandScript();
}
