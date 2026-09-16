int main()
{
    EnsureShieldSupport(nullptr);
    Player player;
    player.Spells.insert(674); // Existing dual wield must survive.
    EnsureShieldSupport(&player);
    assert(player.Spells == std::set<uint32>({674, 9116, 107}));
    assert(player.Skill == 1 && player.Armor == 64 && player.Block);
    uint32 mutations = player.Mutations;
    EnsureShieldSupport(&player);
    assert(player.Mutations == mutations);
    // Known spells but lost runtime/skill state (e.g. database load).
    player.Skill = 0;
    player.Armor = 0;
    player.Block = false;
    EnsureShieldSupport(&player);
    assert(player.Skill == 1 && player.Armor == 64 && player.Block);
    player.Name = "Renamed";
    assert(IsBloodKnight(&player)); // Configured GUID takes precedence.
    player.Id.Value = 43;
    player.Name = "Aldric";
    assert(!IsBloodKnight(&player));
    mutations = player.Mutations;
    EnsureShieldSupport(&player);
    assert(player.Mutations == mutations);
    Settings.CharacterGuid = 0;
    assert(IsBloodKnight(&player));
    player.Class = 1;
    assert(!IsBloodKnight(&player));
    EnsureShieldSupport(&player);
    assert(player.Mutations == mutations);
    player.Class = CLASS_DEATH_KNIGHT;
    Settings.Enable = false;
    EnsureShieldSupport(&player);
    assert(player.Mutations == mutations);
    std::cout << "Shield targeting, repair and idempotence tests passed\n";
}
