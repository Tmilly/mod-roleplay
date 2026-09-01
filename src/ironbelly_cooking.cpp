#include "roleplay.h"

#include "Chat.h"
#include "Item.h"
#include "ItemScript.h"
#include "Player.h"
#include "Random.h"

#include <array>

namespace
{
constexpr std::array<uint32, 8> IronbellyRecipeSpells =
{
    901100, 901101, 901102, 901103, 901104, 901105, 901106, 901107
};

class IronbellyFoodItemScript : public ItemScript
{
public:
    IronbellyFoodItemScript() : ItemScript("IronbellyFoodItemScript") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        if (!player || !item)
            return false;

        uint32 chance = item->GetEntry() == 901005 ? 3 : 5;
        if (roll_chance_i(chance))
            player->CastSpell(player, 11008, true);
        return false;
    }
};
}

bool HandleIronbellyLearn(ChatHandler* handler)
{
    Player* player = handler->GetSession()->GetPlayer();
    if (player->GetName() != GetRoleplayConfig().IronbellyCharacterName)
    {
        handler->PSendSysMessage("Only {} can learn the Ironbelly recipes in Phase 1.",
            GetRoleplayConfig().IronbellyCharacterName);
        return true;
    }

    if (!player->HasSkill(SKILL_COOKING))
    {
        handler->SendSysMessage("Dorrin must learn Cooking first.");
        return true;
    }

    uint32 learned = 0;
    for (uint32 spellId : IronbellyRecipeSpells)
    {
        if (!player->HasSpell(spellId))
        {
            player->learnSpell(spellId, false);
            ++learned;
        }
    }

    handler->PSendSysMessage("{} learned {} Ironbelly recipes.", player->GetName(), learned);
    return true;
}

void AddIronbellyCookingScripts()
{
    new IronbellyFoodItemScript();
}
