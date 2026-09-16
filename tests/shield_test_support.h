#include <cassert>
#include <cstdint>
#include <iostream>
#include <set>
#include <string>
using uint32 = std::uint32_t;
constexpr uint32 CLASS_DEATH_KNIGHT = 6;
constexpr uint32 SKILL_SHIELD = 433;
constexpr uint32 ITEM_SUBCLASS_ARMOR_SHIELD = 6;
constexpr uint32 ITEM_CLASS_ARMOR = 4;
struct
{
    bool Enable = true;
    uint32 CharacterGuid = 42;
    std::string CharacterName = "Aldric";
} Settings;
struct Guid
{
    uint32 Value;
    uint32 GetCounter() const { return Value; }
};
struct Player
{
    uint32 Class = CLASS_DEATH_KNIGHT;
    Guid Id{42};
    std::string Name = "Aldric";
    std::set<uint32> Spells;
    uint32 Skill = 0, Armor = 0, Mutations = 0;
    bool Block = false;
    uint32 getClass() const { return Class; }
    Guid GetGUID() const { return Id; }
    std::string GetName() const { return Name; }
    bool HasSpell(uint32 id) const { return Spells.contains(id); }
    void learnSpell(uint32 id, bool) { Spells.insert(id); ++Mutations; }
    bool HasSkill(uint32 id) const { assert(id == SKILL_SHIELD); return Skill != 0; }
    void SetSkill(uint32 id, uint32 step, uint32 value, uint32 max)
    {
        assert(id == SKILL_SHIELD && step == 0 && value == 1 && max == 1);
        Skill = value;
        ++Mutations;
    }
    uint32 GetArmorProficiency() const { return Armor; }
    void AddArmorProficiency(uint32 mask) { Armor |= mask; ++Mutations; }
    void SendProficiency(uint32 type, uint32 mask) { assert(type == ITEM_CLASS_ARMOR && mask == Armor); }
    bool CanBlock() const { return Block; }
    void SetCanBlock(bool value) { Block = value; ++Mutations; }
};
