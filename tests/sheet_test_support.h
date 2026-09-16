// Test doubles only for world/session lookup and presentation. Parsing uses real core headers.
#include "ChatCommand.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace Acore::ChatCommands;
constexpr std::size_t MaxCardBytes = 128 * 1024;
constexpr std::size_t MaxCardEditBytes = 4000;
struct Player
{
    std::string GetName() const { return "Testbot"; }
} Testbot;
class ChatHandler
{
public:
    std::vector<std::string> Messages;
    void SendSysMessage(std::string_view text) { Messages.emplace_back(text); }
    template<class... Args> void PSendSysMessage(char const* text, Args&&...) { Messages.emplace_back(text); }
};
void Acore::Impl::ChatCommands::SendErrorMessageToHandler(ChatHandler* handler, std::string_view text)
{
    handler->SendSysMessage(text);
}
namespace Acore
{
[[noreturn]] void Assert(std::string_view, unsigned int, std::string_view, std::string_view,
    std::string_view, std::string_view) { std::abort(); }
}
std::string GetDebugInfo() { return {}; }
std::filesystem::path CardPath;
Player* FindPlayerbot(ChatHandler*, std::string_view name)
{
    return name.empty() || name == "Testbot" ? &Testbot : nullptr;
}
bool GetCardPath(ChatHandler*, Player*, std::filesystem::path& path) { path = CardPath; return true; }
std::string BuildStarterCard(ChatHandler*, Player*) { return "Name: Testbot\nBackground:\n"; }
std::string Displayed;
void ShowCard(ChatHandler*, Player*, std::string const& text) { Displayed = text; }
