#include "roleplay.h"

#include "Log.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "SpellDefines.h"

#include <algorithm>

namespace
{
bool IsEligible(Player const* player)
{
    RoleplayConfig const& config = GetRoleplayConfig();
    return config.Enabled && player && player->GetName() == config.LyonCharacterName &&
        player->HasItemCount(config.ShadowWandItemId, 1, false);
}

class LyonShadowPlayerScript : public PlayerScript
{
public:
    LyonShadowPlayerScript() : PlayerScript("LyonShadowPlayerScript") { }

    void OnPlayerLogin(Player* player) override
    {
        RoleplayConfig const& config = GetRoleplayConfig();
        if (IsEligible(player) && config.PersonalMessages)
            player->SendSystemMessage("The Shadow Wand grows cold.");
    }
};

class LyonShadowUnitScript : public UnitScript
{
public:
    LyonShadowUnitScript() : UnitScript("LyonShadowUnitScript") { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!victim || !victim->IsPlayer() || !damage)
            return;

        Player* player = victim->ToPlayer();
        if (!IsEligible(player))
            return;

        RoleplayConfig const& config = GetRoleplayConfig();
        ApplyDarkAegis(config, damage);

        if (TryRefusalOfDeath(config, player, damage))
            return;

        TryDeathsReprisal(config, player, attacker);
    }

private:
    static void ApplyDarkAegis(RoleplayConfig const& config, uint32& damage)
    {
        if (!config.DarkAegisEnabled)
            return;

        float reductionPct = std::clamp(config.DarkAegisDamageReductionPct, 0.0f, 100.0f);
        damage -= uint32(float(damage) * reductionPct / 100.0f);
    }

    static bool TryRefusalOfDeath(RoleplayConfig const& config, Player* player, uint32& damage)
    {
        if (!config.RefusalOfDeathEnabled || damage < player->GetHealth() ||
            player->HasSpellCooldown(config.RefusalOfDeathSpellId) ||
            !roll_chance_f(std::clamp(config.RefusalOfDeathChancePct, 0.0f, 100.0f)))
        {
            return false;
        }

        damage = player->GetHealth() - 1;
        player->CastSpell(player, config.RefusalOfDeathSpellId, true);
        player->AddSpellCooldown(config.RefusalOfDeathSpellId, 0, config.RefusalOfDeathCooldownMs);

        if (config.PersonalMessages)
            player->SendSystemMessage("The Shadow Wand refuses to let you die.");

        return true;
    }

    static void TryDeathsReprisal(RoleplayConfig const& config, Player* player, Unit* attacker)
    {
        if (!config.DeathsReprisalEnabled || !attacker || attacker == player || !attacker->IsAlive() ||
            !player->IsHostileTo(attacker) || player->HasSpellCooldown(config.DeathsReprisalSpellId) ||
            !roll_chance_f(std::clamp(config.DeathsReprisalChancePct, 0.0f, 100.0f)))
        {
            return;
        }

        int32 reprisalDamage = int32(config.DeathsReprisalBaseDamage +
            config.DeathsReprisalDamagePerLevel * player->GetLevel());
        uint32 manaBefore = player->GetPower(POWER_MANA);
        SpellCastResult castResult = player->CastCustomSpell(
            config.DeathsReprisalSpellId, SPELLVALUE_BASE_POINT0, reprisalDamage, attacker, TRIGGERED_FULL_MASK);
        if (castResult == SPELL_CAST_OK && config.DeathsReprisalVisualSpellId)
            player->CastSpell(attacker, config.DeathsReprisalVisualSpellId, TRIGGERED_FULL_MASK);

        uint32 manaAfter = player->GetPower(POWER_MANA);

        if (config.DeathsReprisalDebugMana)
        {
            LOG_INFO("module.roleplay.debug",
                "Death's Reprisal mana audit for {}: before={}, after={}, delta={}, castResult={}",
                player->GetName(), manaBefore, manaAfter, int64(manaAfter) - int64(manaBefore),
                static_cast<uint32>(castResult));
        }

        player->AddSpellCooldown(config.DeathsReprisalSpellId, 0, config.DeathsReprisalCooldownMs);
    }
};
}

void AddLyonShadowScripts()
{
    new LyonShadowPlayerScript();
    new LyonShadowUnitScript();
}
