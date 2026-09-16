int main()
{
    using namespace Roleplay::DeathKnight;
    Player player;
    player.Spells = {49896, 49903, 49194, 9116, 107}; // Imported ranks, a talent, and shield support.
    ApplyProgression(&player);
    assert(player.HasSpell(45477) && player.HasSpell(45462) && player.HasSpell(45902));
    assert(!player.HasSpell(49896) && !player.HasSpell(49903) && !player.HasSpell(47541));
    assert(player.HasSpell(49194) && player.HasSpell(9116) && player.HasSpell(107));
    auto spells = player.Spells;
    ApplyProgression(&player);
    assert(player.Spells == spells);
    player.Level = 6;
    ApplyProgression(&player);
    assert(player.HasSpell(47541));
    player.Level = 10;
    ApplyProgression(&player);
    assert(player.HasSpell(674) && player.HasSkill(SKILL_DUAL_WIELD));
    player.Level = 40;
    ApplyProgression(&player);
    assert(player.HasSpell(750) && player.HasSkill(SKILL_PLATE_MAIL));
    player.Level = 55;
    player.Spells.insert(49896);
    ApplyProgression(&player);
    assert(player.HasSpell(49896)); // Normal 55+ progression left alone.
    player.Level = 5;
    player.Managed = false;
    spells = player.Spells;
    ApplyProgression(&player);
    assert(player.Spells == spells);
    std::cout << "Actual DK progression: rank repair, unlocks, exclusions and preservation passed\n";
}
