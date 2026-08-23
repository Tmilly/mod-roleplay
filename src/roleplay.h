#ifndef MOD_ROLEPLAY_H
#define MOD_ROLEPLAY_H

#include "Define.h"
#include <string>

struct RoleplayConfig
{
    bool Enabled = true;
    std::string LyonCharacterName = "Lyon";
    uint32 ShadowWandItemId = 900000;
    bool DarkAegisEnabled = true;
    float DarkAegisDamageReductionPct = 3.0f;
    bool DeathsReprisalEnabled = true;
    float DeathsReprisalChancePct = 5.0f;
    uint32 DeathsReprisalCooldownMs = 1000;
    uint32 DeathsReprisalSpellId = 900001;
    uint32 DeathsReprisalBaseDamage = 25;
    uint32 DeathsReprisalDamagePerLevel = 2;
    bool DeathsReprisalDebugMana = true;
    bool RefusalOfDeathEnabled = true;
    float RefusalOfDeathChancePct = 100.0f;
    uint32 RefusalOfDeathCooldownMs = 600000;
    uint32 RefusalOfDeathSpellId = 48792;
    bool PersonalMessages = true;
};

RoleplayConfig const& GetRoleplayConfig();

void AddLyonShadowScripts();

#endif
