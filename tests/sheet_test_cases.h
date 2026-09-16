int main(int argc, char** argv)
{
    assert(argc == 2);
    ChatHandler handler;
    for (std::string prefix : {std::string(), std::string("Testbot ")})
    {
        CardPath = std::filesystem::path(argv[1]) / (prefix.empty() ? "selected.card.txt" : "named.card.txt");
        auto run = [&](std::string const& args) { assert(SheetCommand(&handler, prefix + args)); };
        auto read = [&](std::filesystem::path const& path)
        {
            std::string text;
            assert(ReadCard(path, text));
            return text;
        };
        run("create");
        std::string expected = read(CardPath);
        run("create");
        assert(read(CardPath) == expected);
        run("show");
        assert(Displayed == expected);
        for (std::string text : {"Background: A veteran of Lordaeron.",
            "Personality: Reserved, loyal, and stubborn.", "Personality: Reserved, loyal, and stubborn.",
            "Quirks: caf\xC3\xA9, \xE4\xBD\xA0\xE5\xA5\xBD!  'Yes'; (no)."})
        {
            run("append " + text);
            expected += "\n" + text;
            assert(read(CardPath) == expected);
        }
        run("append   spaced  text.  ");
        expected += "\nspaced  text.";
        assert(read(CardPath) == expected);
        for (std::string bad : {"", "bogus", "create extra", "show extra", "clear extra", "append", "set  "})
        {
            handler.Messages.clear();
            run(bad);
            assert(!handler.Messages.empty());
            assert(handler.Messages.front().find("Usage:") == 0);
            assert(read(CardPath) == expected);
            assert(!std::filesystem::exists(CardPath.string() + ".bak"));
        }
        run("append " + std::string(4001, 'x'));
        run("set " + std::string(4001, 'x'));
        assert(read(CardPath) == expected);
        run("set Name: Testbot. Background: A former soldier.");
        assert(read(CardPath.string() + ".bak") == expected);
        expected = "Name: Testbot. Background: A former soldier.";
        assert(read(CardPath) == expected);
        run("show");
        assert(Displayed == expected);
        run("clear");
        assert(read(CardPath).empty());
        assert(read(CardPath.string() + ".bak") == expected);
        assert(WriteCard(CardPath, std::string(MaxCardBytes, 'x'), false));
        run("append too large");
        assert(read(CardPath).size() == MaxCardBytes);
    }
    // Demonstrate why the old registration fails before invoking the handler.
    Acore::Impl::ChatCommands::CommandInvoker oldCommand(HandleBotSheet);
    assert(!oldCommand(&handler, "append multiple words"));
    assert(!oldCommand(&handler, "Testbot show"));
    std::cout << "Sheet registration/parser and disposable card tests passed\n";
}
