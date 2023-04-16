//
// C++ Implementation: starname
//
// Description:
//
//
// Author: Toti <root@totibox>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "starname.h"

#include <array>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <istream>
#include <optional>
#include <string>
#include <system_error>

#include <fmt/format.h>

#include <celcompat/charconv.h>
#include <celutil/greek.h>
#include "astroobj.h"
#include "constellation.h"


namespace
{

constexpr std::string_view::size_type MAX_CANONICAL_LENGTH = 256;
constexpr unsigned int FIRST_NUMBERED_VARIABLE = 335;
constexpr unsigned int INVALID_BAYER = static_cast<unsigned int>(-1);

// Try parsing the first word of a name as a Flamsteed number or variable star
// designation. Single-letter variable star designations are handled by the
// Bayer parser due to indistinguishability with case-insensitive lookup.
bool isFlamsteedOrVariable(std::string_view prefix)
{
    using celestia::compat::from_chars;
    switch (prefix.size())
    {
        case 0:
            return false;
        case 1:
            // Match single-digit Flamsteed number
            return prefix[0] >= '1' && prefix[0] <= '9';
        case 2:
            {
                auto p0 = static_cast<unsigned char>(prefix[0]);
                auto p1 = static_cast<unsigned char>(prefix[1]);
                return
                    // Two-digit Flamsteed number
                    (std::isdigit(p0) && p0 != '0' && std::isdigit(p1)) ||
                    (std::isalpha(p0) && std::isalpha(p1) &&
                     std::tolower(p0) != 'j' && std::tolower(p1) != 'j' &&
                     p1 >= p0);
            }
        default:
            {
                // check for either Flamsteed or V### format variable star designations
                std::size_t startNumber = std::tolower(static_cast<unsigned char>(prefix[0])) == 'v'
                    ? 1
                    : 0;
                auto endPtr = prefix.data() + prefix.size();
                unsigned int value;
                auto [ptr, ec] = from_chars(prefix.data() + startNumber, endPtr, value);
                return ec == std::errc{} && ptr == endPtr &&
                       (startNumber == 0 || value >= FIRST_NUMBERED_VARIABLE);
            }
    }
}


struct BayerLetter
{
    std::string_view letter{ };
    unsigned int number{ 0 };
};


// Attempts to parse the first word of a star name as a Greek or Latin-letter
// Bayer designation, with optional numeric suffix
BayerLetter parseBayerLetter(std::string_view prefix)
{
    using celestia::compat::from_chars;

    BayerLetter result;
    if (auto numberPos = prefix.find_first_of("0123456789"); numberPos == std::string_view::npos)
        result.letter = prefix;
    else if (auto [ptr, ec] = from_chars(prefix.data() + numberPos, prefix.data() + prefix.size(), result.number);
             ec == std::errc{} && ptr == prefix.data() + prefix.size())
        result.letter = prefix.substr(0, numberPos);
    else
        return {};

    if (result.letter.empty())
        return {};

    if (auto greek = GetCanonicalGreekAbbreviation(result.letter); !greek.empty())
        result.letter = greek;
    else if (result.letter.size() != 1 || !std::isalpha(static_cast<unsigned char>(result.letter[0])))
        return {};

    return result;
}

} // end unnamed namespace


std::uint32_t
StarNameDatabase::findCatalogNumberByName(std::string_view name, bool i18n) const
{
    if (auto catalogNumber = getCatalogNumberByName(name, i18n);
        catalogNumber != AstroCatalog::InvalidIndex)
        return catalogNumber;

    if (auto pos = name.find(' '); pos != 0 && pos != std::string::npos && pos < name.size() - 1)
    {
        std::string_view prefix = name.substr(0, pos);
        std::string_view remainder = name.substr(pos + 1);

        if (auto catalogNumber = findFlamsteedOrVariable(prefix, remainder, i18n);
            catalogNumber != AstroCatalog::InvalidIndex)
            return catalogNumber;

        if (auto catalogNumber = findBayer(prefix, remainder, i18n);
            catalogNumber != AstroCatalog::InvalidIndex)
            return catalogNumber;
    }

    return findWithComponentSuffix(name, i18n);
}


std::uint32_t
StarNameDatabase::findFlamsteedOrVariable(std::string_view prefix,
                                          std::string_view remainder,
                                          bool i18n) const
{
    if (!isFlamsteedOrVariable(prefix))
        return AstroCatalog::InvalidIndex;

    std::string_view::size_type offset;
    std::string_view constellationAbbrev = ParseConstellation(remainder, offset);
    auto suffix = remainder.substr(offset);
    if (constellationAbbrev.empty() || (!suffix.empty() && suffix.front() != ' '))
        return AstroCatalog::InvalidIndex;

    std::array<char, MAX_CANONICAL_LENGTH> canonical;
    if (auto [it, size] = fmt::format_to_n(canonical.data(), canonical.size(), "{} {}{}",
                                           prefix, constellationAbbrev, suffix);
        size <= canonical.size())
    {
        auto catalogNumber = getCatalogNumberByName({canonical.data(), size}, i18n);
        if (catalogNumber != AstroCatalog::InvalidIndex)
            return catalogNumber;
    }

    if (!suffix.empty())
        return AstroCatalog::InvalidIndex;

    // try appending " A"
    if (auto [it, size] = fmt::format_to_n(canonical.data(), canonical.size(), "{} {} A",
                                           prefix, constellationAbbrev);
        size <= canonical.size())
        return getCatalogNumberByName({canonical.data(), size}, i18n);

    return AstroCatalog::InvalidIndex;
}


std::uint32_t
StarNameDatabase::findBayer(std::string_view prefix,
                            std::string_view remainder,
                            bool i18n) const
{
    auto bayerLetter = parseBayerLetter(prefix);
    if (bayerLetter.letter.empty())
        return AstroCatalog::InvalidIndex;

    std::string_view::size_type offset;
    std::string_view constellationAbbrev = ParseConstellation(remainder, offset);
    auto suffix = remainder.substr(offset);
    if (constellationAbbrev.empty() || (!suffix.empty() && suffix.front() != ' '))
        return AstroCatalog::InvalidIndex;

    return bayerLetter.number == 0
        ? findBayerNoNumber(bayerLetter.letter, constellationAbbrev, suffix, i18n)
        : findBayerWithNumber(bayerLetter.letter,
                              bayerLetter.number,
                              constellationAbbrev,
                              suffix,
                              i18n);
}


std::uint32_t
StarNameDatabase::findBayerNoNumber(std::string_view letter,
                                    std::string_view constellationAbbrev,
                                    std::string_view suffix,
                                    bool i18n) const
{
    std::array<char, MAX_CANONICAL_LENGTH> canonical;
    if (auto [it, size] = fmt::format_to_n(canonical.data(), canonical.size(), "{} {}{}",
                                           letter, constellationAbbrev, suffix);
        size <= canonical.size())
    {
        auto catalogNumber = getCatalogNumberByName({canonical.data(), size}, i18n);
        if (catalogNumber != AstroCatalog::InvalidIndex)
            return catalogNumber;
    }

    // Try appending "1" to the letter, e.g. ALF CVn --> ALF1 CVn
    if (auto [it, size] = fmt::format_to_n(canonical.data(), canonical.size(), "{}1 {}{}",
                                           letter, constellationAbbrev, suffix);
        size <= canonical.size())
    {
        auto catalogNumber = getCatalogNumberByName({canonical.data(), size}, i18n);
        if (catalogNumber != AstroCatalog::InvalidIndex)
            return catalogNumber;
    }

    if (!suffix.empty())
        return AstroCatalog::InvalidIndex;

    // No component suffix, so try appending " A"
    if (auto [it, size] = fmt::format_to_n(canonical.data(), canonical.size(), "{} {} A",
                                           letter, constellationAbbrev);
        size <= canonical.size())
    {
        auto catalogNumber = getCatalogNumberByName({canonical.data(), size}, i18n);
        if (catalogNumber != AstroCatalog::InvalidIndex)
            return catalogNumber;
    }

    if (auto [it, size] = fmt::format_to_n(canonical.data(), canonical.size(), "{}1 {} A",
                                           letter, constellationAbbrev);
        size <= canonical.size())
        return getCatalogNumberByName({canonical.data(), size}, i18n);

    return AstroCatalog::InvalidIndex;
}


std::uint32_t
StarNameDatabase::findBayerWithNumber(std::string_view letter,
                                      unsigned int number,
                                      std::string_view constellationAbbrev,
                                      std::string_view suffix,
                                      bool i18n) const
{
    std::array<char, MAX_CANONICAL_LENGTH> canonical;
    if (auto [it, size] = fmt::format_to_n(canonical.data(), canonical.size(), "{}{} {}{}",
                                           letter, number, constellationAbbrev, suffix);
        size <= canonical.size())
    {
        auto catalogNumber = getCatalogNumberByName({canonical.data(), size}, i18n);
        if (catalogNumber != AstroCatalog::InvalidIndex)
            return catalogNumber;
    }

    if (!suffix.empty())
        return AstroCatalog::InvalidIndex;

    // No component suffix, so try appending "A"
    if (auto [it, size] = fmt::format_to_n(canonical.data(), canonical.size(), "{}{} {} A",
                                           letter, number, constellationAbbrev);
        size <= canonical.size())
        return getCatalogNumberByName({canonical.data(), size}, i18n);

    return AstroCatalog::InvalidIndex;
}


std::uint32_t
StarNameDatabase::findWithComponentSuffix(std::string_view name, bool i18n) const
{
    std::array<char, MAX_CANONICAL_LENGTH> canonical;
    if (auto [it, size] = fmt::format_to_n(canonical.data(), canonical.size(), "{} A", name);
        size <= canonical.size())
        return getCatalogNumberByName({canonical.data(), size}, i18n);

    return AstroCatalog::InvalidIndex;
}


std::unique_ptr<StarNameDatabase>
StarNameDatabase::readNames(std::istream& in)
{
    auto db = std::make_unique<StarNameDatabase>();
    bool failed = false;
    std::string s;

    while (!failed)
    {
        auto catalogNumber = AstroCatalog::InvalidIndex;

        in >> catalogNumber;
        if (in.eof())
            break;
        if (in.bad())
        {
            failed = true;
            break;
        }

        // in.get(); // skip a space (or colon);

        std::string name;
        std::getline(in, name);
        if (in.bad())
        {
            failed = true;
            break;
        }

        // Iterate through the string for names delimited
        // by ':', and insert them into the star database. Note that
        // db->add() will skip empty names.
        std::string::size_type startPos = 0;
        while (startPos != std::string::npos)
        {
            ++startPos;
            std::string::size_type next = name.find(':', startPos);
            std::string::size_type length = std::string::npos;

            if (next != std::string::npos)
                length = next - startPos;

            db->add(catalogNumber, name.substr(startPos, length));
            startPos = next;
        }
    }

    if (failed)
        return nullptr;

    return db;
}
