#include "RoleplayDeathKnight.h"
#include "DBCStores.h"
#include "SpellMgr.h"
#include <sstream>

namespace Roleplay::DeathKnight
{
namespace
{
struct Unlock { uint8 Level; uint32 Spell; };
// The reference's first-rank cadence, without its campaign skips or level-80 rank compression.
constexpr Unlock Unlocks[] =
{
    {1, 45477}, {1, 45462}, {4, 45902}, {6, 47541}, {8, 49576},
    {10, 48266}, {10, 674}, {12, 46584}, {14, 50842}, {16, 47528},
    {18, 48263}, {20, 45524}, {22, 47476}, {24, 49998}, {26, 48721},
    {28, 43265}, {30, 3714}, {32, 48792}, {34, 56222}, {36, 57330},
    {38, 48743}, {40, 48707}, {42, 48265}, {44, 47568}, {46, 61999},
    {48, 45529}, {50, 49020}, {52, 56815}
};
}

void ApplyProgression(Player* player)
{
    if (!IsManaged(player))
        return;

    for (auto const& unlock : Unlocks)
    {
        // Repair imported/previously learned hero-level ranks only in our explicit unlock chains.
        // Preserve talents and all normal 55+ training. Rank 1 is the intentional low-level fallback.
        if (player->GetLevel() < 55)
            for (uint32 rank = sSpellMgr->GetNextSpellInChain(unlock.Spell); rank;
                rank = sSpellMgr->GetNextSpellInChain(rank))
                if (SpellInfo const* info = sSpellMgr->GetSpellInfo(rank))
                    if (IsPrematureProgressionRank(player->GetLevel(), info->SpellLevel) && player->HasSpell(rank))
                        player->removeSpell(rank, SPEC_MASK_ALL, false);

        if (player->GetLevel() >= unlock.Level)
        {
            if (!player->HasSpell(unlock.Spell))
                player->learnSpell(unlock.Spell, false);
        }
        else if (GetState(player)->Progress == Leveling && player->GetLevel() < 55)
            player->removeSpell(unlock.Spell, SPEC_MASK_ALL, false);
    }

    // Plate is normally a DK creation skill; its DBC minimum is not a usable level-40 trainer rule.
    if (player->GetLevel() >= GetConfig().PlateLevel)
    {
        if (!player->HasSpell(750))
            player->learnSpell(750, false);
        if (!player->HasSkill(SKILL_PLATE_MAIL))
            player->SetSkill(SKILL_PLATE_MAIL, 0, 1, 1);
    }
    else
    {
        player->removeSpell(750, SPEC_MASK_ALL, false);
        if (player->HasSkill(SKILL_PLATE_MAIL))
            player->SetSkill(SKILL_PLATE_MAIL, 0, 0, 0);
    }

    // Explicit proficiencies avoid the level-55 DBC skill gates. Never max an earned weapon skill.
    for (uint32 spell : {196u, 197u, 200u, 201u, 202u, 198u, 199u, 9077u, 8737u, 9078u})
        if (!player->HasSpell(spell))
            player->learnSpell(spell, false);
    for (uint16 skill : {SKILL_AXES, SKILL_2H_AXES, SKILL_POLEARMS, SKILL_SWORDS,
        SKILL_2H_SWORDS, SKILL_MACES, SKILL_2H_MACES})
        if (!player->HasSkill(skill))
            player->SetSkill(skill, 0, 1, player->GetMaxSkillValueForLevel());

    if (player->GetLevel() >= 10 && !player->HasSkill(SKILL_DUAL_WIELD))
        player->SetSkill(SKILL_DUAL_WIELD, 0, 1, 1);
}

void RestoreTaxi(Player* player)
{
    State* state = GetState(player);
    if (!IsManaged(player) || !state->TrackTaxi)
        return;
    // Core re-adds every old-continent node on every DK login AND level-up.
    std::ostringstream mask;
    for (uint32 value : state->Taxi)
        mask << value << ' ';
    player->m_taxi.LoadTaxiMask(mask.str());
    player->m_taxi.InitTaxiNodesForLevel(player->getRace(), CLASS_WARRIOR, player->GetLevel());
}

void RecordTaxi(Player const* player, uint32 node)
{
    State* state = GetState(player);
    if (!IsManaged(player) || !state->TrackTaxi || !node || node > TaxiMaskSize * 32)
        return;
    state->Taxi[(node - 1) / 32] |= uint32(1) << ((node - 1) % 32);
    SaveState(const_cast<Player*>(player));
}
}
