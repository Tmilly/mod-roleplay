// Theoretical payload comparison, NOT a DPS/TTK simulator or an empirical balance test.
// All rows reuse caller-provided stats to isolate the curve; supply actual staging stats per level.
#include "../src/death_knight/RoleplayDeathKnightPolicy.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 5)
    {
        std::cerr << "Usage: model <normalized weapon hit including AP> <AP> <holy SP> <swing seconds>\n";
        return 1;
    }
    double weapon = std::atof(argv[1]), ap = std::atof(argv[2]);
    double holy = std::atof(argv[3]), speed = std::atof(argv[4]);
    if (!std::isfinite(weapon) || !std::isfinite(ap) || !std::isfinite(holy) || !std::isfinite(speed)
        || weapon <= 0 || ap < 0 || holy < 0 || speed <= 0)
        return 1;
    std::cout << "THEORETICAL, pre-mitigation, no talents/presences/crit; fixed inputs at every level.\n"
        << "Heroic Strike is an added bonus replacing a white swing; DK strikes add separate GCD damage.\n"
        << "Seal is added per swing. Columns are not interchangeable DPS/TTK measurements.\n"
        << "Level,Scale,IcyTouch,PlagueStrike,BloodStrike(2 diseases),WarriorHeroicBonus,PaladinJudgement,SealProc\n";
    // Appropriate Heroic Strike ranks from installed Spell.dbc: levels 1/8/16/24/32/40/48.
    struct Sample { unsigned Level; double HeroicBonus; };
    for (auto const& row : {Sample{5, 11}, {10, 21}, {20, 32}, {30, 44}, {40, 93}, {50, 136}, {55, 136}})
    {
        float scale = Roleplay::DeathKnight::LevelDamageScale(row.Level, 55, 1);
        // DK lowest ranks: Icy Touch 127-137 + .10 AP; PS=.50*(W+125); BS=.40*(W+260)*1.25.
        // Paladin coefficients: repository spell_bonus_data + spell_pal_seal_of_righteousness.
        std::cout << std::fixed << std::setprecision(2) << row.Level << ',' << scale << ','
            << (132 + 0.10 * ap) * scale << ',' << 0.50 * (weapon + 125) * scale << ','
            << 0.40 * (weapon + 260) * 1.25 * scale << ',' << row.HeroicBonus << ','
            << 1 + 0.20 * ap + 0.32 * holy << ',' << speed * (0.022 * ap + 0.044 * holy) << '\n';
    }
}
