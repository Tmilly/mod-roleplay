#include "roleplay.h"

#include "Config.h"
#include "Log.h"
#include "ScriptMgr.h"

namespace
{
RoleplayConfig Config;

class RoleplayWorldScript : public WorldScript
{
public:
    RoleplayWorldScript() : WorldScript("RoleplayWorldScript") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        Config.Enabled = sConfigMgr->GetOption<bool>("Roleplay.Enable", true);
        Config.LyonCharacterName = sConfigMgr->GetOption<std::string>("Roleplay.Lyon.CharacterName", "Lyon");
        Config.ShadowWandItemId = sConfigMgr->GetOption<uint32>("Roleplay.Lyon.ShadowWandItemId", 32618);
        Config.DarkAegisEnabled = sConfigMgr->GetOption<bool>("Roleplay.Lyon.DarkAegis.Enable", true);
        Config.DarkAegisDamageReductionPct =
            sConfigMgr->GetOption<float>("Roleplay.Lyon.DarkAegis.DamageReductionPct", 3.0f);
        Config.DeathsReprisalEnabled = sConfigMgr->GetOption<bool>("Roleplay.Lyon.DeathsReprisal.Enable", true);
        Config.DeathsReprisalChancePct =
            sConfigMgr->GetOption<float>("Roleplay.Lyon.DeathsReprisal.ChancePct", 5.0f);
        Config.DeathsReprisalCooldownMs =
            sConfigMgr->GetOption<uint32>("Roleplay.Lyon.DeathsReprisal.CooldownMs", 1000);
        Config.DeathsReprisalSpellId =
            sConfigMgr->GetOption<uint32>("Roleplay.Lyon.DeathsReprisal.SpellId", 900001);
        Config.DeathsReprisalVisualSpellId =
            sConfigMgr->GetOption<uint32>("Roleplay.Lyon.DeathsReprisal.VisualSpellId", 33335);
        Config.DeathsReprisalBaseDamage =
            sConfigMgr->GetOption<uint32>("Roleplay.Lyon.DeathsReprisal.BaseDamage", 25);
        Config.DeathsReprisalDamagePerLevel =
            sConfigMgr->GetOption<uint32>("Roleplay.Lyon.DeathsReprisal.DamagePerLevel", 2);
        Config.DeathsReprisalDebugMana =
            sConfigMgr->GetOption<bool>("Roleplay.Lyon.DeathsReprisal.DebugMana", true);
        Config.RefusalOfDeathEnabled = sConfigMgr->GetOption<bool>("Roleplay.Lyon.RefusalOfDeath.Enable", true);
        Config.RefusalOfDeathChancePct =
            sConfigMgr->GetOption<float>("Roleplay.Lyon.RefusalOfDeath.ChancePct", 100.0f);
        Config.RefusalOfDeathCooldownMs =
            sConfigMgr->GetOption<uint32>("Roleplay.Lyon.RefusalOfDeath.CooldownMs", 600000);
        Config.RefusalOfDeathSpellId =
            sConfigMgr->GetOption<uint32>("Roleplay.Lyon.RefusalOfDeath.SpellId", 48792);
        Config.PersonalMessages = sConfigMgr->GetOption<bool>("Roleplay.Lyon.Messages.Personal", true);
    }

    void OnStartup() override
    {
        if (Config.Enabled)
            LOG_INFO("module.roleplay", "Roleplay module enabled.");
    }
};
}

RoleplayConfig const& GetRoleplayConfig()
{
    return Config;
}

void AddRoleplayScripts()
{
    new RoleplayWorldScript();
    AddLyonShadowScripts();
}
