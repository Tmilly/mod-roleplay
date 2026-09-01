#include "roleplay.h"

#include "CharacterCache.h"
#include "Chat.h"
#include "Config.h"
#include "DBCStores.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Random.h"
#include "WorldSession.h"
#include "../../mod-playerbots/src/Bot/PlayerbotMgr.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
constexpr std::size_t MaxCardBytes = 128 * 1024;
constexpr std::size_t MaxCardEditBytes = 4000;
constexpr std::size_t MaxCardDisplayBytes = 4000;
constexpr std::size_t ChatChunkBytes = 220;

struct Appearance
{
    uint8 Gender = GENDER_MALE;
    uint8 Skin = 0;
    uint8 Face = 0;
    uint8 HairStyle = 0;
    uint8 HairColor = 0;
    uint8 FacialHair = 0;
};

struct AppearanceOptions
{
    std::vector<std::pair<uint8, uint8>> Faces;
    std::vector<std::pair<uint8, uint8>> Hairs;
    std::vector<uint8> FacialHair;
};

std::string_view Trim(std::string_view value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
        value.remove_prefix(1);
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
        value.remove_suffix(1);
    return value;
}

std::string_view TakeToken(std::string_view& value)
{
    value = Trim(value);
    std::size_t separator = value.find_first_of(" \t");
    std::string_view token = value.substr(0, separator);
    value = separator == std::string_view::npos ? std::string_view() : value.substr(separator + 1);
    return token;
}

std::string Lower(std::string_view value)
{
    std::string result(value);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character)
    {
        return static_cast<char>(std::tolower(character));
    });
    return result;
}

Player* GetSelectedPlayerbot(ChatHandler* handler)
{
    Player* caller = handler && handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
    Player* target = caller ? caller->GetSelectedPlayer() : nullptr;
    if (!target || !sPlayerbotsMgr.GetPlayerbotAI(target))
    {
        handler->SendSysMessage("Select a currently loaded Playerbot, or provide its online character name.");
        return nullptr;
    }
    return target;
}

Player* FindPlayerbot(ChatHandler* handler, std::string_view name)
{
    if (name.empty())
        return GetSelectedPlayerbot(handler);

    Player* target = ObjectAccessor::FindPlayerByName(std::string(name));
    if (!target)
    {
        handler->PSendSysMessage("Playerbot '{}' is not currently loaded.", name);
        return nullptr;
    }
    if (!sPlayerbotsMgr.GetPlayerbotAI(target))
    {
        handler->PSendSysMessage("'{}' is not a Playerbot.", target->GetName());
        return nullptr;
    }
    return target;
}

AppearanceOptions GetAppearanceOptions(uint8 race, uint8 gender)
{
    AppearanceOptions options;
    for (CharSectionsEntry const* section : sCharSectionsStore)
    {
        if (!section || section->Race != race || section->Gender != gender ||
            section->Type > std::numeric_limits<uint8>::max() ||
            section->Color > std::numeric_limits<uint8>::max())
        {
            continue;
        }

        switch (section->GenType)
        {
            case SECTION_TYPE_FACE:
                options.Faces.emplace_back(static_cast<uint8>(section->Type), static_cast<uint8>(section->Color));
                break;
            case SECTION_TYPE_HAIR:
                options.Hairs.emplace_back(static_cast<uint8>(section->Type), static_cast<uint8>(section->Color));
                break;
            case SECTION_TYPE_FACIAL_HAIR:
                options.FacialHair.push_back(static_cast<uint8>(section->Type));
                break;
            default:
                break;
        }
    }

    std::sort(options.Faces.begin(), options.Faces.end());
    options.Faces.erase(std::unique(options.Faces.begin(), options.Faces.end()), options.Faces.end());
    std::sort(options.Hairs.begin(), options.Hairs.end());
    options.Hairs.erase(std::unique(options.Hairs.begin(), options.Hairs.end()), options.Hairs.end());
    std::sort(options.FacialHair.begin(), options.FacialHair.end());
    options.FacialHair.erase(std::unique(options.FacialHair.begin(), options.FacialHair.end()),
        options.FacialHair.end());
    return options;
}

bool HasFace(AppearanceOptions const& options, uint8 face, uint8 skin)
{
    return std::find(options.Faces.begin(), options.Faces.end(), std::pair<uint8, uint8>(face, skin)) !=
        options.Faces.end();
}

bool HasHair(AppearanceOptions const& options, uint8 style, uint8 color)
{
    return std::find(options.Hairs.begin(), options.Hairs.end(), std::pair<uint8, uint8>(style, color)) !=
        options.Hairs.end();
}

bool HasFacialHair(AppearanceOptions const& options, uint8 value)
{
    if (options.FacialHair.empty())
        return value == 0;
    return std::find(options.FacialHair.begin(), options.FacialHair.end(), value) != options.FacialHair.end();
}

Appearance GetAppearance(Player* bot)
{
    Appearance appearance;
    appearance.Gender = bot->GetByteValue(PLAYER_BYTES_3, PLAYER_BYTES_3_OFFSET_GENDER);
    appearance.Skin = bot->GetByteValue(PLAYER_BYTES, PLAYER_BYTES_OFFSET_SKIN_ID);
    appearance.Face = bot->GetByteValue(PLAYER_BYTES, PLAYER_BYTES_OFFSET_FACE_ID);
    appearance.HairStyle = bot->GetByteValue(PLAYER_BYTES, PLAYER_BYTES_OFFSET_HAIR_STYLE_ID);
    appearance.HairColor = bot->GetByteValue(PLAYER_BYTES, PLAYER_BYTES_OFFSET_HAIR_COLOR_ID);
    appearance.FacialHair = bot->GetByteValue(PLAYER_BYTES_2, PLAYER_BYTES_2_OFFSET_FACIAL_STYLE);
    return appearance;
}

bool NormalizeAppearance(uint8 race, Appearance& appearance, bool randomize)
{
    AppearanceOptions options = GetAppearanceOptions(race, appearance.Gender);
    if (options.Faces.empty() || options.Hairs.empty())
        return false;

    if (randomize || !HasFace(options, appearance.Face, appearance.Skin))
    {
        auto const& face = options.Faces[urand(0, options.Faces.size() - 1)];
        appearance.Face = face.first;
        appearance.Skin = face.second;
    }
    if (randomize || !HasHair(options, appearance.HairStyle, appearance.HairColor))
    {
        auto const& hair = options.Hairs[urand(0, options.Hairs.size() - 1)];
        appearance.HairStyle = hair.first;
        appearance.HairColor = hair.second;
    }
    if (randomize || !HasFacialHair(options, appearance.FacialHair))
    {
        appearance.FacialHair = options.FacialHair.empty() ? 0 :
            options.FacialHair[urand(0, options.FacialHair.size() - 1)];
    }
    return true;
}

void ApplyAppearance(Player* bot, Appearance const& appearance)
{
    bot->SetByteValue(UNIT_FIELD_BYTES_0, 2, appearance.Gender);
    bot->SetByteValue(PLAYER_BYTES_3, PLAYER_BYTES_3_OFFSET_GENDER, appearance.Gender);
    bot->SetByteValue(PLAYER_BYTES, PLAYER_BYTES_OFFSET_SKIN_ID, appearance.Skin);
    bot->SetByteValue(PLAYER_BYTES, PLAYER_BYTES_OFFSET_FACE_ID, appearance.Face);
    bot->SetByteValue(PLAYER_BYTES, PLAYER_BYTES_OFFSET_HAIR_STYLE_ID, appearance.HairStyle);
    bot->SetByteValue(PLAYER_BYTES, PLAYER_BYTES_OFFSET_HAIR_COLOR_ID, appearance.HairColor);
    bot->SetByteValue(PLAYER_BYTES_2, PLAYER_BYTES_2_OFFSET_FACIAL_STYLE, appearance.FacialHair);
    bot->InitDisplayIds();
    bot->SaveToDB(false, false);
    sCharacterCache->UpdateCharacterData(bot->GetGUID(), bot->GetName(), appearance.Gender, bot->getRace(true));
}

bool ParseByte(std::string_view value, uint8& result)
{
    if (value.empty())
        return false;
    uint32 parsed = 0;
    for (char character : value)
    {
        if (!std::isdigit(static_cast<unsigned char>(character)))
            return false;
        parsed = parsed * 10 + static_cast<uint32>(character - '0');
        if (parsed > std::numeric_limits<uint8>::max())
            return false;
    }
    result = static_cast<uint8>(parsed);
    return true;
}

bool IsAppearanceOperation(std::string_view value)
{
    std::string operation = Lower(value);
    return operation == "random" || operation == "skin" || operation == "face" || operation == "hair" ||
        operation == "haircolor" || operation == "facialhair";
}

std::string GetValidAppearanceValues(std::string const& operation, AppearanceOptions const& options,
    Appearance const& appearance)
{
    std::vector<uint8> values;
    if (operation == "skin")
    {
        for (auto const& [face, skin] : options.Faces)
            if (face == appearance.Face)
                values.push_back(skin);
    }
    else if (operation == "face")
    {
        for (auto const& [face, skin] : options.Faces)
            if (skin == appearance.Skin)
                values.push_back(face);
    }
    else if (operation == "hair")
    {
        for (auto const& [style, color] : options.Hairs)
            if (color == appearance.HairColor)
                values.push_back(style);
    }
    else if (operation == "haircolor")
    {
        for (auto const& [style, color] : options.Hairs)
            if (style == appearance.HairStyle)
                values.push_back(color);
    }
    else if (operation == "facialhair")
        values = options.FacialHair.empty() ? std::vector<uint8>{0} : options.FacialHair;

    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    std::ostringstream output;
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index)
            output << ", ";
        output << static_cast<uint32>(values[index]);
    }
    return output.str();
}

bool IsSheetOperation(std::string_view value)
{
    std::string operation = Lower(value);
    return operation == "create" || operation == "show" || operation == "append" || operation == "set" ||
        operation == "clear";
}

bool IsSafeCharacterFilename(std::string_view name)
{
    return !name.empty() && std::all_of(name.begin(), name.end(), [](unsigned char character)
    {
        return std::isalnum(character) != 0;
    });
}

bool GetCardPath(ChatHandler* handler, Player* bot, std::filesystem::path& path)
{
    if (!IsSafeCharacterFilename(bot->GetName()))
    {
        handler->SendSysMessage("The Playerbot name cannot be represented as a safe PBC card filename.");
        return false;
    }

    std::string configuredPath = sConfigMgr->GetOption<std::string>("PBC.CharacterCardsPath",
        "../../../modules/mod-pbc/characters");
    std::error_code error;
    std::filesystem::path directory = std::filesystem::absolute(configuredPath, error).lexically_normal();
    if (error || !std::filesystem::is_directory(directory, error) || error)
    {
        handler->PSendSysMessage("PBC character-card directory is unavailable: {}", configuredPath);
        return false;
    }

    path = (directory / (bot->GetName() + ".card.txt")).lexically_normal();
    if (path.parent_path() != directory)
    {
        handler->SendSysMessage("Refusing an unsafe PBC character-card path.");
        return false;
    }
    return true;
}

std::string GetLocalizedName(char const* const* names, LocaleConstant locale)
{
    if (names[locale] && *names[locale])
        return names[locale];
    return names[DEFAULT_LOCALE] ? names[DEFAULT_LOCALE] : "Unknown";
}

std::string BuildStarterCard(ChatHandler* handler, Player* bot)
{
    LocaleConstant locale = handler->GetSession()->GetSessionDbcLocale();
    ChrRacesEntry const* race = sChrRacesStore.LookupEntry(bot->getRace(true));
    ChrClassesEntry const* playerClass = sChrClassesStore.LookupEntry(bot->getClass());
    std::ostringstream card;
    card << "Name: " << bot->GetName() << '\n';
    card << "Race: " << (race ? GetLocalizedName(race->name, locale) : "Unknown") << '\n';
    card << "Class: " << (playerClass ? GetLocalizedName(playerClass->name, locale) : "Unknown") << '\n';
    uint8 gender = bot->GetByteValue(PLAYER_BYTES_3, PLAYER_BYTES_3_OFFSET_GENDER);
    card << "Gender: " << (gender == GENDER_FEMALE ? "Female" : "Male") << "\n\n";
    card << "Background:\n\nPersonality:\n\nSpeaking Style:\n\nMotivations:\n\nLikes:\n\nDislikes:\n\n";
    card << "Quirks:\n\nRole in the Party:\n";
    return card.str();
}

bool ReadCard(std::filesystem::path const& path, std::string& contents)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    std::ostringstream stream;
    stream << input.rdbuf();
    contents = stream.str();
    return true;
}

bool WriteCard(std::filesystem::path const& path, std::string_view contents, bool append)
{
    std::ofstream output(path, std::ios::binary | (append ? std::ios::app : std::ios::trunc));
    if (!output)
        return false;
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    return output.good();
}

bool BackupCard(std::filesystem::path const& path)
{
    std::error_code error;
    std::filesystem::copy_file(path, path.string() + ".bak", std::filesystem::copy_options::overwrite_existing,
        error);
    return !error;
}

void ShowCard(ChatHandler* handler, Player* bot, std::string const& contents)
{
    handler->PSendSysMessage("PBC character card for {}:", bot->GetName());
    std::size_t displaySize = std::min(contents.size(), MaxCardDisplayBytes);
    for (std::size_t offset = 0; offset < displaySize;)
    {
        std::size_t end = std::min(offset + ChatChunkBytes, displaySize);
        while (end < displaySize && end > offset &&
            (static_cast<unsigned char>(contents[end]) & 0xC0) == 0x80)
        {
            --end;
        }
        if (end == offset)
            end = std::min(offset + ChatChunkBytes, displaySize);
        handler->SendSysMessage(std::string_view(contents).substr(offset, end - offset));
        offset = end;
    }
    if (contents.size() > displaySize)
        handler->PSendSysMessage("Card output truncated after {} characters.", displaySize);
}
}

bool HandleBotGender(ChatHandler* handler, std::string_view args)
{
    std::string_view first = TakeToken(args);
    std::string_view second = TakeToken(args);
    std::string_view botName;
    std::string_view genderText;
    if (second.empty())
        genderText = first;
    else
    {
        botName = first;
        genderText = second;
    }
    if (genderText.empty() || !Trim(args).empty())
        return false;

    std::string gender = Lower(genderText);
    if (gender != "male" && gender != "female")
    {
        handler->SendSysMessage("Gender must be 'male' or 'female'.");
        return true;
    }

    Player* bot = FindPlayerbot(handler, botName);
    if (!bot)
        return true;

    Appearance appearance = GetAppearance(bot);
    appearance.Gender = gender == "female" ? GENDER_FEMALE : GENDER_MALE;
    if (!NormalizeAppearance(bot->getRace(true), appearance, false))
    {
        handler->SendSysMessage("No valid appearance data exists for that race and gender.");
        return true;
    }

    ApplyAppearance(bot, appearance);
    handler->PSendSysMessage("Set {}'s gender to {} and validated the resulting appearance.", bot->GetName(), gender);
    return true;
}

bool HandleBotAppearance(ChatHandler* handler, std::string_view args)
{
    std::string_view first = TakeToken(args);
    std::string_view botName;
    std::string_view operationText;
    if (IsAppearanceOperation(first))
        operationText = first;
    else
    {
        botName = first;
        operationText = TakeToken(args);
    }
    if (operationText.empty())
        return false;

    std::string operation = Lower(operationText);
    Player* bot = FindPlayerbot(handler, botName);
    if (!bot)
        return true;

    Appearance appearance = GetAppearance(bot);
    AppearanceOptions options = GetAppearanceOptions(bot->getRace(true), appearance.Gender);
    if (operation == "random")
    {
        if (!Trim(args).empty())
            return false;
        if (!NormalizeAppearance(bot->getRace(true), appearance, true))
        {
            handler->SendSysMessage("No valid appearance data exists for this race and gender.");
            return true;
        }
    }
    else
    {
        std::string_view valueText = TakeToken(args);
        uint8 value = 0;
        if (!ParseByte(valueText, value) || !Trim(args).empty())
        {
            handler->SendSysMessage("Appearance IDs must be integers from 0 through 255.");
            return true;
        }

        bool valid = false;
        if (operation == "skin")
        {
            valid = HasFace(options, appearance.Face, value);
            appearance.Skin = value;
        }
        else if (operation == "face")
        {
            valid = HasFace(options, value, appearance.Skin);
            appearance.Face = value;
        }
        else if (operation == "hair")
        {
            valid = HasHair(options, value, appearance.HairColor);
            appearance.HairStyle = value;
        }
        else if (operation == "haircolor")
        {
            valid = HasHair(options, appearance.HairStyle, value);
            appearance.HairColor = value;
        }
        else if (operation == "facialhair")
        {
            valid = HasFacialHair(options, value);
            appearance.FacialHair = value;
        }

        if (!valid)
        {
            handler->PSendSysMessage(
                "Appearance value {} is invalid for {}'s current race, gender, and paired customization values.",
                value, bot->GetName());
            handler->PSendSysMessage("Valid values for the current paired selection: {}",
                GetValidAppearanceValues(operation, options, appearance));
            return true;
        }
    }

    ApplyAppearance(bot, appearance);
    handler->PSendSysMessage("Updated {}'s appearance: skin {}, face {}, hair {}, hair color {}, facial hair {}.",
        bot->GetName(), appearance.Skin, appearance.Face, appearance.HairStyle, appearance.HairColor,
        appearance.FacialHair);
    return true;
}

bool HandleBotSheet(ChatHandler* handler, std::string_view args)
{
    std::string_view first = TakeToken(args);
    std::string_view botName;
    std::string_view operationText;
    if (IsSheetOperation(first))
        operationText = first;
    else
    {
        botName = first;
        operationText = TakeToken(args);
    }
    if (operationText.empty())
        return false;

    std::string operation = Lower(operationText);
    Player* bot = FindPlayerbot(handler, botName);
    if (!bot)
        return true;

    std::filesystem::path path;
    if (!GetCardPath(handler, bot, path))
        return true;

    std::error_code error;
    bool exists = std::filesystem::is_regular_file(path, error) && !error;
    if (operation == "create")
    {
        if (!Trim(args).empty())
            return false;
        if (exists)
        {
            handler->PSendSysMessage("PBC character card already exists for {}. It was not overwritten.",
                bot->GetName());
            return true;
        }
        if (!WriteCard(path, BuildStarterCard(handler, bot), false))
        {
            handler->SendSysMessage("Could not create the PBC character card.");
            return true;
        }
    }
    else if (operation == "show")
    {
        if (!Trim(args).empty())
            return false;
        std::string contents;
        if (!exists || !ReadCard(path, contents))
        {
            handler->PSendSysMessage("No readable PBC character card exists for {}.", bot->GetName());
            return true;
        }
        ShowCard(handler, bot, contents);
        return true;
    }
    else if (operation == "append")
    {
        std::string_view text = Trim(args);
        if (!exists || text.empty() || text.size() > MaxCardEditBytes)
        {
            handler->SendSysMessage("Append requires an existing card and 1-4000 characters of text.");
            return true;
        }
        std::uintmax_t currentSize = std::filesystem::file_size(path, error);
        if (error || currentSize + text.size() + 1 > MaxCardBytes ||
            !WriteCard(path, std::string("\n") + std::string(text), true))
        {
            handler->SendSysMessage("Could not append the card; it may be unavailable or over 128 KiB.");
            return true;
        }
    }
    else if (operation == "set")
    {
        std::string_view text = Trim(args);
        if (!exists || text.empty() || text.size() > MaxCardEditBytes)
        {
            handler->SendSysMessage("Set requires an existing card and 1-4000 characters of replacement text.");
            return true;
        }
        if (!BackupCard(path) || !WriteCard(path, text, false))
        {
            handler->SendSysMessage("Could not back up and replace the PBC character card.");
            return true;
        }
    }
    else if (operation == "clear")
    {
        if (!Trim(args).empty())
            return false;
        if (!exists || !BackupCard(path) || !WriteCard(path, "", false))
        {
            handler->SendSysMessage("Could not back up and clear the PBC character card.");
            return true;
        }
    }
    else
        return false;

    handler->PSendSysMessage("PBC character card for {} updated at {}.", bot->GetName(), path.string());
    handler->SendSysMessage("Run .chars reload as a GM, or from the worldserver console, to apply the change.");
    return true;
}
