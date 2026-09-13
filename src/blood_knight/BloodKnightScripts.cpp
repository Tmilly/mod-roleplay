#include "BloodKnight.h"

#include "SpellInfo.h"
#include "Unit.h"
#include "ScriptMgr.h"

#include <algorithm>

namespace Roleplay::BloodKnight
{
namespace
{
class BloodKnightWorldScript : public WorldScript
{
public:
    BloodKnightWorldScript() : WorldScript("RoleplayBloodKnightWorld") { }
    void OnAfterConfigLoad(bool reload) override { LoadConfig(reload); }
};

class BloodKnightPlayerScript : public PlayerScript
{
public:
    BloodKnightPlayerScript() : PlayerScript("RoleplayBloodKnightPlayer") { }

    void OnPlayerLogin(Player* player) override { Refresh(player); }
    void OnPlayerFirstLogin(Player* player) override { Refresh(player); }
    void OnPlayerLevelChanged(Player* player, uint8 /*oldLevel*/) override { Refresh(player); }
    void OnPlayerSave(Player* player) override { Save(player); }

    void OnPlayerUpdate(Player* player, uint32 diff) override
    {
        if (!IsBloodKnight(player) || !GetConfig().ManaEnable)
            return;
        State* state = GetState(player);
        state->ManaElapsed += diff;
        if (state->ManaElapsed < 1000)
            return;
        state->ManaElapsed %= 1000;
        if (!player->IsInCombat() && player->GetMaxPower(POWER_MANA))
        {
            uint32 amount = std::max<uint32>(1, uint32(float(player->GetMaxPower(POWER_MANA))
                * GetConfig().ManaOutOfCombatRegenPct / 100.0f));
            player->ModifyPower(POWER_MANA, int32(amount));
        }
    }

    bool OnPlayerHasActivePowerType(Player const* player, Powers power) override
    {
        return IsBloodKnight(player) && GetConfig().ManaEnable && power == POWER_MANA;
    }
};

class BloodKnightUnitScript : public UnitScript
{
public:
    BloodKnightUnitScript() : UnitScript("RoleplayBloodKnightUnit") { }

    void ModifyPeriodicDamageAurasTick(Unit* /*target*/, Unit* attacker, uint32& damage,
        SpellInfo const* spellInfo) override
    {
        if (!GetConfig().BloodPlagueDrain || !attacker || !spellInfo || !attacker->IsPlayer())
            return;
        Player* player = attacker->ToPlayer();
        if (IsBloodKnight(player) && (spellInfo->Id == 55078 || spellInfo->Id == 59879 || spellInfo->Id == 59921))
            player->ModifyHealth(int32(float(damage) * GetConfig().BloodPlagueDrainPct / 100.0f));
    }

    void ModifyHealReceived(Unit* target, Unit* healer, uint32& heal, SpellInfo const* spellInfo) override
    {
        if (!GetConfig().VampiricBlood || !target || !healer || !spellInfo || target != healer || !target->IsPlayer())
            return;
        if (IsBloodKnight(target->ToPlayer()) && target->HasAura(55233))
            heal = uint32(float(heal) * 1.15f);
    }

    void OnUnitDeath(Unit* unit, Unit* killer) override
    {
        if (!GetConfig().SoulFeast || !unit || !killer || unit->IsPlayer() || !killer->IsPlayer())
            return;
        Player* player = killer->ToPlayer();
        if (!IsBloodKnight(player) || unit->IsCritter())
            return;
        if (!unit->HasAura(55078, player->GetGUID()) && !unit->HasAura(980, player->GetGUID())
            && !unit->HasAura(11713, player->GetGUID()))
            return;
        player->ModifyHealth(std::max<uint32>(1, player->GetMaxHealth() / 50));
        player->ModifyPower(POWER_RUNIC_POWER, 5);
        if (GetConfig().ManaEnable)
            player->ModifyPower(POWER_MANA, int32(std::max<uint32>(1, player->GetMaxPower(POWER_MANA) / 50)));
    }
};
}
}

void AddRoleplayBloodKnightScripts()
{
    new Roleplay::BloodKnight::BloodKnightWorldScript();
    new Roleplay::BloodKnight::BloodKnightPlayerScript();
    new Roleplay::BloodKnight::BloodKnightUnitScript();
}
