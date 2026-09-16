#include "RoleplayDeathKnightPolicy.h"
#include "SharedDefines.h"
#include <cassert>
#include <iostream>
#include <map>
#include <set>

struct SpellInfo { uint32 SpellLevel; };
struct SpellManager
{
    std::map<uint32, uint32> Next{{45477, 49896}, {49896, 49903}};
    std::map<uint32, SpellInfo> Info{{49896, {61}}, {49903, {67}}};
    uint32 GetNextSpellInChain(uint32 id) const
    {
        auto itr = Next.find(id);
        return itr == Next.end() ? 0 : itr->second;
    }
    SpellInfo const* GetSpellInfo(uint32 id) const
    {
        auto itr = Info.find(id);
        return itr == Info.end() ? nullptr : &itr->second;
    }
} Manager;
SpellManager* sSpellMgr = &Manager;
constexpr uint8 SPEC_MASK_ALL = 3;
struct Player
{
    bool Managed = true;
    uint8 Level = 5;
    std::set<uint32> Spells;
    std::set<uint16> Skills;
    uint8 GetLevel() const { return Level; }
    bool HasSpell(uint32 id) const { return Spells.contains(id); }
    void learnSpell(uint32 id, bool) { Spells.insert(id); }
    void removeSpell(uint32 id, uint8 mask, bool)
    {
        assert(mask == SPEC_MASK_ALL);
        Spells.erase(id);
    }
    bool HasSkill(uint16 id) const { return Skills.contains(id); }
    void SetSkill(uint16 id, uint16, uint16 value, uint16)
    {
        if (value)
            Skills.insert(id);
        else
            Skills.erase(id);
    }
    uint16 GetMaxSkillValueForLevel() const { return Level * 5; }
};
namespace Roleplay::DeathKnight
{
struct Config { uint8 PlateLevel = 40; } Settings;
struct State { Stage Progress = Leveling; } Progress;
Config const& GetConfig() { return Settings; }
State* GetState(Player*) { return &Progress; }
bool IsManaged(Player const* player) { return player && player->Managed; }
}
