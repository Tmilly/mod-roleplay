#include "RoleplayDeathKnight.h"
#include "DatabaseEnv.h"
#include "Errors.h"
#include "Item.h"
#include "Mail.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "WorldSession.h"
#include <memory>

namespace Roleplay::DeathKnight
{
bool DeliverStartingGear(Player* player)
{
    State* state = GetState(player);
    auto start = GetStartingData(player->getRace(), player->getGender());
    if (!state || !start || state->Progress != Leveling || player->IsBeingTeleportedFar())
        return false;

    // Allocate every possible mail attachment first. An invalid template/allocation cannot
    // leave a half-granted outfit. One entry per stack, preserving DBC/SQL quantities.
    std::vector<std::unique_ptr<Item>> items;
    if (GetConfig().GiveAcherusStartingGear)
        for (auto const& [id, count] : start->Items)
        {
            auto itemTemplate = sObjectMgr->GetItemTemplate(id);
            if (!itemTemplate)
                return false;
            for (uint32 remaining = count; remaining;)
            {
                uint32 stack = std::min(remaining, itemTemplate->GetMaxStackSize());
                if (!stack)
                    return false;
                std::unique_ptr<Item> item(Item::CreateItem(id, stack, player));
                if (!item)
                    return false;
                items.push_back(std::move(item));
                remaining -= stack;
            }
        }

    state->Busy = true;
    auto transaction = CharacterDatabase.BeginTransaction();
    auto draft = std::make_unique<MailDraft>("Your Acherus starting equipment",
        "Your inventory could not hold these starting supplies. These are not campaign quest rewards.");
    uint32 attachments = 0;
    auto send = [&]()
    {
        draft->SendMailTo(transaction, MailReceiver(player), MailSender(MAIL_CREATURE, 25462),
            MAIL_CHECK_MASK_HAS_BODY);
        draft = std::make_unique<MailDraft>("Your Acherus starting equipment",
            "Additional starting supplies. Complete the campaign to earn its quest rewards.");
        attachments = 0;
    };
    for (auto& item : items)
    {
        ItemPosCountVec destinations;
        uint32 noSpace = 0;
        uint32 count = item->GetCount();
        InventoryResult result = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, destinations,
            item->GetEntry(), count, &noSpace);
        uint32 stored = result == EQUIP_ERR_OK ? count :
            (result == EQUIP_ERR_INVENTORY_FULL && noSpace <= count ? count - noSpace : 0);
        if (stored)
        {
            // StoreNewItem NEVER equips items; equipment and bag slots already worn remain untouched.
            if (Item* added = player->StoreNewItem(destinations, item->GetEntry(), true))
                player->SendNewItem(added, stored, true, false);
            else
                stored = 0;
        }
        if (stored == count)
            continue;
        item->SetCount(count - stored);
        item->SaveToDB(transaction);
        draft->AddItem(item.release());
        if (++attachments == MAX_MAIL_ITEMS)
            send();
    }
    if (attachments)
        send();

    state->Progress = Prepared;
    SaveState(player);
    // Inventory, attached item instances, mail and the marker share a single transaction.
    // This MUST run before TeleportTo: a far teleport makes SaveToDB silently defer itself.
    player->SaveToDB(transaction, false, false);
    ObjectGuid guid = player->GetGUID();
    player->GetSession()->AddTransactionCallback(CharacterDatabase.AsyncCommitTransaction(transaction))
        .AfterComplete([guid](bool committed)
        {
            // Use the normal character-save queue. A separate synchronous connection could
            // overtake earlier queued saves. Never retain a Player pointer across the callback.
            ASSERT(committed, "Acherus equipment transaction failed; stopping to preserve recoverable state");
            if (Player* current = ObjectAccessor::FindPlayer(guid))
                if (State* currentState = GetState(current))
                    currentState->Busy = false;
        });
    // The retry scheduler will teleport only AFTER acknowledgement of the database commit.
    return false;
}
}
