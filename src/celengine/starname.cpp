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

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <istream>
#include <iterator>
#include <optional>
#include <string>
#include <system_error>
#include <type_traits>

#include <fmt/format.h>

#include <celcompat/charconv.h>
#include <celutil/binaryread.h>
#include <celutil/gettext.h>
#include <celutil/greek.h>
#include <celutil/logger.h>
#include <celutil/timer.h>
#include "astroobj.h"
#include "constellation.h"

using namespace std::string_view_literals;

namespace compat = celestia::compat;
namespace util = celestia::util;

using util::GetLogger;

namespace
{

constexpr std::string_view CROSSINDEX_MAGIC = "CELINDEX"sv;
constexpr std::uint16_t CrossIndexVersion   = 0x0100;

constexpr std::string_view HDCatalogPrefix        = "HD "sv;
constexpr std::string_view HIPPARCOSCatalogPrefix = "HIP "sv;
constexpr std::string_view TychoCatalogPrefix     = "TYC "sv;
constexpr std::string_view SAOCatalogPrefix       = "SAO "sv;

constexpr AstroCatalog::IndexNumber TYC123_MIN = 1u;
constexpr AstroCatalog::IndexNumber TYC1_MAX   = 9999u;  // actual upper limit is 9537 in TYC2
constexpr AstroCatalog::IndexNumber TYC2_MAX   = 99999u; // actual upper limit is 12121 in TYC2
constexpr AstroCatalog::IndexNumber TYC3_MAX   = 3u;     // from TYC2

// In the original Tycho catalog, TYC3 ranges from 1 to 3, so no there is
// no chance of overflow in the multiplication. TDSC (Fabricius et al. 2002)
// adds one entry with TYC3 = 4 (TYC 2907-1276-4) so permit TYC=4 when the
// TYC1 number is <= 2907
constexpr inline AstroCatalog::IndexNumber TDSC_TYC3_MAX            = 4u;
constexpr inline AstroCatalog::IndexNumber TDSC_TYC3_MAX_RANGE_TYC1 = 2907u;

#pragma pack(push, 1)

// cross-index header structure
struct CrossIndexHeader
{
    CrossIndexHeader() = delete;
    char magic[8]; //NOSONAR
    std::uint16_t version;
};

// cross-index record structure
struct CrossIndexRecord
{
    CrossIndexRecord() = delete;
    std::uint32_t catalogNumber;
    std::uint32_t celCatalogNumber;
};

#pragma pack(pop)

static_assert(std::is_standard_layout_v<CrossIndexHeader>);
static_assert(std::is_standard_layout_v<CrossIndexRecord>);

constexpr unsigned int FIRST_NUMBERED_VARIABLE = 335;

using CanonicalBuffer = fmt::basic_memory_buffer<char, 256>;

// workaround for missing append method in earlier fmt versions
inline void
appendComponentA(CanonicalBuffer& buffer)
{
    buffer.push_back(' ');
    buffer.push_back('A');
}

// Try parsing the first word of a name as a Flamsteed number or variable star
// designation. Single-letter variable star designations are handled by the
// Bayer parser due to indistinguishability with case-insensitive lookup.
bool
isFlamsteedOrVariable(std::string_view prefix)
{
    using compat::from_chars;
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
BayerLetter
parseBayerLetter(std::string_view prefix)
{
    using compat::from_chars;

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

bool
parseSimpleCatalogNumber(std::string_view name,
                         std::string_view prefix,
                         AstroCatalog::IndexNumber& catalogNumber)
{
    using compat::from_chars;
    if (compareIgnoringCase(name, prefix, prefix.size()) != 0)
        return false;

    // skip additional whitespace
    auto pos = name.find_first_not_of(" \t", prefix.size());
    if (pos == std::string_view::npos)
        return false;

    if (auto [ptr, ec] = from_chars(name.data() + pos, name.data() + name.size(), catalogNumber); ec == std::errc{})
    {
        // Do not match if suffix is present
        pos = name.find_first_not_of(" \t", ptr - name.data());
        return pos == std::string_view::npos;
    }

    return false;
}

bool
parseTychoCatalogNumber(std::string_view name,
                        AstroCatalog::IndexNumber& catalogNumber)
{
    using compat::from_chars;
    if (compareIgnoringCase(name, TychoCatalogPrefix, TychoCatalogPrefix.size()) != 0)
        return false;

    // skip additional whitespace
    auto pos = name.find_first_not_of(" \t", TychoCatalogPrefix.size());
    if (pos == std::string_view::npos)
        return false;

    const char* const end_ptr = name.data() + name.size();

    std::array<AstroCatalog::IndexNumber, 3> tycParts;
    auto result = from_chars(name.data() + pos, end_ptr, tycParts[0]);
    if (result.ec != std::errc{}
        || tycParts[0] < TYC123_MIN || tycParts[0] > TYC1_MAX
        || result.ptr == end_ptr
        || *result.ptr != '-')
    {
        return false;
    }

    result = from_chars(result.ptr + 1, end_ptr, tycParts[1]);
    if (result.ec != std::errc{}
        || tycParts[1] < TYC123_MIN || tycParts[1] > TYC2_MAX
        || result.ptr == end_ptr
        || *result.ptr != '-')
    {
        return false;
    }

    if (result = from_chars(result.ptr + 1, end_ptr, tycParts[2]);
        result.ec == std::errc{}
        && tycParts[2] >= TYC123_MIN
        && (tycParts[2] <= TYC3_MAX
            || (tycParts[2] == TDSC_TYC3_MAX && tycParts[0] <= TDSC_TYC3_MAX_RANGE_TYC1)))
    {
        // Do not match if suffix is present
        pos = name.find_first_not_of(" \t", result.ptr - name.data());
        if (pos != std::string_view::npos)
            return false;

        catalogNumber = tycParts[2] * StarNameDatabase::TYC3_MULTIPLIER
                      + tycParts[1] * StarNameDatabase::TYC2_MULTIPLIER
                      + tycParts[0];
        return true;
    }

    return false;
}

bool
parseCelestiaCatalogNumber(std::string_view name,
                           AstroCatalog::IndexNumber& catalogNumber)
{
    using celestia::compat::from_chars;
    if (name.size() == 0 || name[0] != '#')
        return false;

    if (auto [ptr, ec] = from_chars(name.data() + 1, name.data() + name.size(), catalogNumber);
        ec == std::errc{})
    {
        // Do not match if suffix is present
        auto pos = name.find_first_not_of(" \t", ptr - name.data());
        return pos == std::string_view::npos;
    }

    return false;
}

// Verify that the cross index file has a correct header
bool
checkCrossIndexHeader(std::istream& in)
{
    std::array<char, sizeof(CrossIndexHeader)> header;
    if (!in.read(header.data(), header.size()).good()) /* Flawfinder: ignore */
        return false;

    // Verify the magic string
    if (std::string_view(header.data() + offsetof(CrossIndexHeader, magic), CROSSINDEX_MAGIC.size()) != CROSSINDEX_MAGIC)
    {
        GetLogger()->error(_("Bad header for cross index\n"));
        return false;
    }

    // Verify the version
    if (auto version = util::fromMemoryLE<std::uint16_t>(header.data() + offsetof(CrossIndexHeader, version));
        version != CrossIndexVersion)
    {
        GetLogger()->error(_("Bad version for cross index\n"));
        return false;
    }

    return true;
}

} // end unnamed namespace

AstroCatalog::IndexNumber
StarNameDatabase::findCatalogNumberByName(std::string_view name, bool i18n) const
{
    if (name.empty())
        return AstroCatalog::InvalidIndex;

    AstroCatalog::IndexNumber catalogNumber = findByName(name, i18n);
    if (catalogNumber != AstroCatalog::InvalidIndex)
        return catalogNumber;

    if (parseCelestiaCatalogNumber(name, catalogNumber))
        return catalogNumber;
    if (parseSimpleCatalogNumber(name, HIPPARCOSCatalogPrefix, catalogNumber))
        return catalogNumber;
    if (parseTychoCatalogNumber(name, catalogNumber))
        return catalogNumber;
    if (parseSimpleCatalogNumber(name, HDCatalogPrefix, catalogNumber))
        return searchCrossIndexForCatalogNumber(StarCatalog::HenryDraper, catalogNumber);
    if (parseSimpleCatalogNumber(name, SAOCatalogPrefix, catalogNumber))
        return searchCrossIndexForCatalogNumber(StarCatalog::SAO, catalogNumber);

    return AstroCatalog::InvalidIndex;
}

// Return the Celestia catalog number for the star with a specified number
// in a cross index.
AstroCatalog::IndexNumber
StarNameDatabase::searchCrossIndexForCatalogNumber(StarCatalog catalog, AstroCatalog::IndexNumber number) const
{
    auto catalogIndex = static_cast<std::size_t>(catalog);
    if (catalogIndex >= crossIndices.size())
        return AstroCatalog::InvalidIndex;

    const CrossIndex& xindex = crossIndices[catalogIndex];
    auto iter = std::lower_bound(xindex.begin(), xindex.end(), number,
                                 [](const CrossIndexEntry& ent, AstroCatalog::IndexNumber n) { return ent.catalogNumber < n; });
    return iter == xindex.end() || iter->catalogNumber != number
        ? AstroCatalog::InvalidIndex
        : iter->celCatalogNumber;
}

AstroCatalog::IndexNumber
StarNameDatabase::crossIndex(StarCatalog catalog, AstroCatalog::IndexNumber celCatalogNumber) const
{
    auto catalogIndex = static_cast<std::size_t>(catalog);
    if (catalogIndex >= crossIndices.size())
        return AstroCatalog::InvalidIndex;

    const CrossIndex& xindex = crossIndices[catalogIndex];

    // A simple linear search.  We could store cross indices sorted by
    // both catalog numbers and trade memory for speed
    auto iter = std::find_if(xindex.begin(), xindex.end(),
                             [celCatalogNumber](const CrossIndexEntry& o) { return celCatalogNumber == o.celCatalogNumber; });
    return iter == xindex.end()
        ? AstroCatalog::InvalidIndex
        : iter->catalogNumber;
}

AstroCatalog::IndexNumber
StarNameDatabase::findByName(std::string_view name, bool i18n) const
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

AstroCatalog::IndexNumber
StarNameDatabase::findFlamsteedOrVariable(std::string_view prefix,
                                          std::string_view remainder,
                                          bool i18n) const
{
    if (!isFlamsteedOrVariable(prefix))
        return AstroCatalog::InvalidIndex;

    auto [constellationAbbrev, suffix] = ParseConstellation(remainder);
    if (constellationAbbrev.empty() || (!suffix.empty() && suffix.front() != ' '))
        return AstroCatalog::InvalidIndex;

    CanonicalBuffer canonical;
    fmt::format_to(std::back_inserter(canonical), "{} {}{}", prefix, constellationAbbrev, suffix);
    if (auto catalogNumber = getCatalogNumberByName({canonical.data(), canonical.size()}, i18n);
        catalogNumber != AstroCatalog::InvalidIndex)
    {
        return catalogNumber;
    }

    if (!suffix.empty())
        return AstroCatalog::InvalidIndex;

    // try appending " A"
    appendComponentA(canonical);
    return getCatalogNumberByName({canonical.data(), canonical.size()}, i18n);
}

AstroCatalog::IndexNumber
StarNameDatabase::findBayer(std::string_view prefix,
                            std::string_view remainder,
                            bool i18n) const
{
    auto bayerLetter = parseBayerLetter(prefix);
    if (bayerLetter.letter.empty())
        return AstroCatalog::InvalidIndex;

    auto [constellationAbbrev, suffix] = ParseConstellation(remainder);
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

AstroCatalog::IndexNumber
StarNameDatabase::findBayerNoNumber(std::string_view letter,
                                    std::string_view constellationAbbrev,
                                    std::string_view suffix,
                                    bool i18n) const
{
    CanonicalBuffer canonical;
    fmt::format_to(std::back_inserter(canonical), "{} {}{}", letter, constellationAbbrev, suffix);
    if (auto catalogNumber = getCatalogNumberByName({canonical.data(), canonical.size()}, i18n);
        catalogNumber != AstroCatalog::InvalidIndex)
    {
        return catalogNumber;
    }

    // Try appending "1" to the letter, e.g. ALF CVn --> ALF1 CVn
    canonical.clear();
    fmt::format_to(std::back_inserter(canonical), "{}1 {}{}", letter, constellationAbbrev, suffix);
    if (auto catalogNumber = getCatalogNumberByName({canonical.data(), canonical.size()}, i18n);
        catalogNumber != AstroCatalog::InvalidIndex)
    {
        return catalogNumber;
    }

    if (!suffix.empty())
        return AstroCatalog::InvalidIndex;

    // No component suffix, so try appending " A"
    canonical.clear();
    fmt::format_to(std::back_inserter(canonical), "{} {} A", letter, constellationAbbrev);
    if (auto catalogNumber = getCatalogNumberByName({canonical.data(), canonical.size()}, i18n);
        catalogNumber != AstroCatalog::InvalidIndex)
    {
        return catalogNumber;
    }

    // Try appending "1" to the letter and a, e.g. ALF CVn --> ALF1 CVn A
    if (auto [it, size] = fmt::format_to_n(canonical.data(), canonical.size(), "{}1 {} A",
                                           letter, constellationAbbrev);
        size <= canonical.size())
        return getCatalogNumberByName({canonical.data(), size}, i18n);

    return AstroCatalog::InvalidIndex;
}

AstroCatalog::IndexNumber
StarNameDatabase::findBayerWithNumber(std::string_view letter,
                                      unsigned int number,
                                      std::string_view constellationAbbrev,
                                      std::string_view suffix,
                                      bool i18n) const
{
    CanonicalBuffer canonical;
    fmt::format_to(std::back_inserter(canonical), "{}{} {}{}", letter, number, constellationAbbrev, suffix);
    if (auto catalogNumber = getCatalogNumberByName({canonical.data(), canonical.size()}, i18n);
        catalogNumber != AstroCatalog::InvalidIndex)
    {
        return catalogNumber;
    }

    if (!suffix.empty())
        return AstroCatalog::InvalidIndex;

    // No component suffix, so try appending " A"
    appendComponentA(canonical);
    return getCatalogNumberByName({canonical.data(), canonical.size()}, i18n);
}

AstroCatalog::IndexNumber
StarNameDatabase::findWithComponentSuffix(std::string_view name, bool i18n) const
{
    CanonicalBuffer canonical;
    fmt::format_to(std::back_inserter(canonical), "{} A", name);
    return getCatalogNumberByName({canonical.data(), canonical.size()}, i18n);
}

std::unique_ptr<StarNameDatabase>
StarNameDatabase::readNames(std::istream& in)
{
    using compat::from_chars;

    constexpr std::size_t maxLength = 1024;
    auto db = std::make_unique<StarNameDatabase>();
    std::string buffer(maxLength, '\0');
    while (!in.eof())
    {
        in.getline(buffer.data(), maxLength);
        // Delimiter is extracted and contributes to gcount() but is not stored
        std::size_t lineLength;

        if (in.good())
            lineLength = static_cast<std::size_t>(in.gcount() - 1);
        else if (in.eof())
            lineLength = static_cast<std::size_t>(in.gcount());
        else
            return nullptr;

        auto line = static_cast<std::string_view>(buffer).substr(0, lineLength);

        if (line.empty() || line.front() == '#')
            continue;

        auto pos = line.find(':');
        if (pos == std::string_view::npos)
            return nullptr;

        auto catalogNumber = AstroCatalog::InvalidIndex;
        if (auto [ptr, ec] = from_chars(line.data(), line.data() + pos, catalogNumber);
            ec != std::errc{} || ptr != line.data() + pos)
        {
            return nullptr;
        }

        // Iterate through the string for names delimited
        // by ':', and insert them into the star database. Note that
        // db->add() will skip empty names.
        line = line.substr(pos + 1);
        while (!line.empty())
        {
            pos = line.find(':');
            std::string_view name = line.substr(0, pos);
            db->add(catalogNumber, name);
            if (pos == std::string_view::npos)
                break;
            line = line.substr(pos + 1);
        }
    }

    return db;
}

bool
StarNameDatabase::loadCrossIndex(StarCatalog catalog, std::istream& in)
{
    Timer timer{};

    auto catalogIndex = static_cast<std::size_t>(catalog);
    if (catalogIndex >= crossIndices.size())
        return false;

    if (!checkCrossIndexHeader(in))
        return false;

    CrossIndex& xindex = crossIndices[catalogIndex];
    xindex = {};

    constexpr std::uint32_t BUFFER_RECORDS = UINT32_C(4096) / sizeof(CrossIndexRecord);
    std::vector<char> buffer(sizeof(CrossIndexRecord) * BUFFER_RECORDS);
    bool hasMoreRecords = true;
    while (hasMoreRecords)
    {
        std::size_t remainingRecords = BUFFER_RECORDS;
        in.read(buffer.data(), buffer.size()); /* Flawfinder: ignore */
        if (in.bad())
        {
            GetLogger()->error(_("Loading cross index failed\n"));
            xindex = {};
            return false;
        }
        if (in.eof())
        {
            auto bytesRead = static_cast<std::uint32_t>(in.gcount());
            remainingRecords = bytesRead / sizeof(CrossIndexRecord);
            // disallow partial records
            if (bytesRead % sizeof(CrossIndexRecord) != 0)
            {
                GetLogger()->error(_("Loading cross index failed - unexpected EOF\n"));
                xindex = {};
                return false;
            }

            hasMoreRecords = false;
        }

        xindex.reserve(xindex.size() + remainingRecords);

        const char* ptr = buffer.data();
        while (remainingRecords-- > 0)
        {
            CrossIndexEntry& ent = xindex.emplace_back();
            ent.catalogNumber = util::fromMemoryLE<AstroCatalog::IndexNumber>(ptr + offsetof(CrossIndexRecord, catalogNumber));
            ent.celCatalogNumber = util::fromMemoryLE<AstroCatalog::IndexNumber>(ptr + offsetof(CrossIndexRecord, celCatalogNumber));
            ptr += sizeof(CrossIndexRecord);
        }
    }

    GetLogger()->debug("Loaded xindex in {} ms\n", timer.getTime());

    std::sort(xindex.begin(), xindex.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.catalogNumber < rhs.catalogNumber; });
    return true;
}
