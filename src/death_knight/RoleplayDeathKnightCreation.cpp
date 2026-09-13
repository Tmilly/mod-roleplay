#include "RoleplayDeathKnight.h"
#include "DBCStores.h"
#include "Errors.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "World.h"
#include <map>

namespace Roleplay::DeathKnight
{
namespace
{
std::map<uint32, StartingData> Originals;

uint32 Key(uint8 race, uint8 gender) { return race | (uint32(gender) << 8); }

uint32 OutfitCount(ItemTemplate const* item)
{
    uint32 count = item->BuyCount;
    if (item->Class == ITEM_CLASS_CONSUMABLE && item->SubClass == ITEM_SUBCLASS_FOOD)
    {
        if (item->Spells[0].SpellCategory == SPELL_CATEGORY_FOOD)
            count = 10;
        else if (item->Spells[0].SpellCategory == SPELL_CATEGORY_DRINK)
            count = 2;
        count = std::min(count, item->GetMaxStackSize());
    }
    return count;
}
}

StartingData const* GetStartingData(uint8 race, uint8 gender)
{
    auto itr = Originals.find(Key(race, gender));
    return itr == Originals.end() ? nullptr : &itr->second;
}

void PrepareCreationData()
{
    if (!GetConfig().Enable)
        return;

    for (uint8 race : {RACE_HUMAN, RACE_ORC, RACE_DWARF, RACE_NIGHTELF, RACE_UNDEAD_PLAYER,
        RACE_TAUREN, RACE_GNOME, RACE_TROLL, RACE_BLOODELF, RACE_DRAENEI})
    {
        PlayerInfo const* original = sObjectMgr->GetPlayerInfo(race, CLASS_DEATH_KNIGHT);
        if (!original)
            continue;

        // This runs after ObjectMgr finishes loading, before the world accepts characters.
        // Only in-memory DK creation data changes; playercreateinfo SQL stays owned by the realm.
        PlayerInfo* info = const_cast<PlayerInfo*>(original);
        for (uint8 gender = GENDER_MALE; gender <= GENDER_FEMALE; ++gender)
        {
            auto outfit = GetCharStartOutfitEntry(race, CLASS_DEATH_KNIGHT, gender);
            ASSERT(outfit, "Missing DK race/gender CharStartOutfit entry");
            StartingData& saved = Originals[Key(race, gender)];
            ASSERT(info->mapId == 609, "DK playercreateinfo must still contain the stock Acherus start");
            saved.Location = WorldLocation(info->mapId, info->positionX, info->positionY,
                info->positionZ, info->orientation);
            saved.Area = info->areaId;
            for (int32 itemId : outfit->ItemId)
                if (itemId > 0)
                {
                    auto item = sObjectMgr->GetItemTemplate(itemId);
                    ASSERT(item, "Missing DK starter item template");
                    saved.Items.emplace_back(itemId, OutfitCount(item));
                }
            for (auto const& item : info->item)
                saved.Items.emplace_back(item.item_id, item.item_amount);

            // Stock first-level clothes, then a two-handed sword all supported DK races can use.
            // These changes never touch the immutable copy used for the level-55 outfit.
            auto mutableOutfit = const_cast<CharStartOutfitEntry*>(outfit);
            std::fill(std::begin(mutableOutfit->ItemId), std::end(mutableOutfit->ItemId), 0);
            uint8 slot = 0;
            auto clothes = GetCharStartOutfitEntry(race, race == RACE_BLOODELF ? CLASS_PALADIN : CLASS_WARRIOR,
                gender);
            if (clothes)
                for (int32 itemId : clothes->ItemId)
                {
                    auto item = itemId > 0 ? sObjectMgr->GetItemTemplate(itemId) : nullptr;
                    if (item && item->Class == ITEM_CLASS_ARMOR && item->SubClass != ITEM_SUBCLASS_ARMOR_SHIELD
                        && item->SubClass <= ITEM_SUBCLASS_ARMOR_MAIL && item->RequiredLevel <= GetConfig().StartLevel)
                        mutableOutfit->ItemId[slot++] = itemId;
                }
            ASSERT(slot + 3 <= MAX_OUTFIT_ITEMS);
            mutableOutfit->ItemId[slot++] = 49778; // Worn Greatsword, level-1 weapon (not an Acherus reward).
            mutableOutfit->ItemId[slot++] = 6948;
            mutableOutfit->ItemId[slot] = 117;
        }

        if (GetConfig().UseRacialStartingZone)
        {
            PlayerInfo const* racial = sObjectMgr->GetPlayerInfo(race,
                race == RACE_BLOODELF ? CLASS_PALADIN : CLASS_WARRIOR);
            ASSERT(racial, "Missing racial starting data");
            info->mapId = racial->mapId;
            info->areaId = racial->areaId;
            info->positionX = racial->positionX;
            info->positionY = racial->positionY;
            info->positionZ = racial->positionZ;
            info->orientation = racial->orientation;
        }
        info->item.clear();
        info->skills.remove_if([](PlayerCreateInfoSkill const& skill)
        {
            return skill.SkillId == SKILL_FIRST_AID || skill.SkillId == SKILL_PLATE_MAIL
                || skill.SkillId == SKILL_RIDING || skill.SkillId == SKILL_DUAL_WIELD;
        });
        info->customSpells.remove_if([](uint32 id)
        {
            auto spell = sSpellMgr->GetSpellInfo(id);
            return spell && spell->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT;
        });
        info->castSpells.remove(48266);
        info->action.clear();
        info->action.emplace_back(0, 6603, ACTION_BUTTON_SPELL);
        info->action.emplace_back(1, 45477, ACTION_BUTTON_SPELL);
        info->action.emplace_back(2, 45462, ACTION_BUTTON_SPELL);
        info->customSpells.push_back(45477);
        info->customSpells.push_back(45462);
        info->customSpells.push_back(202); // Two-Handed Swords, before initial equipment is stored.
    }
    LOG_INFO("module.roleplay", "Death Knight racial creation data prepared; stock Acherus outfits preserved.");
}

void InitializeCharacter(Player* player)
{
    if (!GetConfig().Enable || player->getClass() != CLASS_DEATH_KNIGHT)
        return;

    State* state = player->CustomData.GetDefault<State>(SettingsSource);
    state->Progress = Leveling;
    state->TrackTaxi = true;
    // Creation hooks run after the core's first save, so persist all corrections explicitly.
    player->GiveLevel(GetConfig().StartLevel);
    player->InitTalentForLevel();
    player->SetMoney(sWorld->getIntConfig(CONFIG_START_PLAYER_MONEY));
    player->SetFullHealth();
    player->m_taxi = PlayerTaxi();
    player->m_taxi.InitTaxiNodesForLevel(player->getRace(), CLASS_WARRIOR, player->GetLevel());
    for (uint32 node = 1; node <= TaxiMaskSize * 32; ++node)
        if (player->m_taxi.IsTaximaskNodeKnown(node))
            state->Taxi[(node - 1) / 32] |= uint32(1) << ((node - 1) % 32);
    ApplyProgression(player);
    SaveState(player);
    player->SaveToDB(false, false);
}
}
