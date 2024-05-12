// stardb.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "stardb.h"

#include <algorithm>
#include <cstddef>
#include <set>
#include <system_error>

#include <fmt/format.h>

#include <celcompat/charconv.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/stringutils.h>
#include "starname.h"

using namespace std::string_view_literals;
using celestia::util::GetLogger;

namespace celestia::engine
{

namespace
{

constexpr inline std::string_view HDCatalogPrefix        = "HD "sv;
constexpr inline std::string_view HIPPARCOSCatalogPrefix = "HIP "sv;
constexpr inline std::string_view TychoCatalogPrefix     = "TYC "sv;
constexpr inline std::string_view SAOCatalogPrefix       = "SAO "sv;
#if 0
constexpr inline std::string_view GlieseCatalogPrefix    = "Gliese "sv;
constexpr inline std::string_view RossCatalogPrefix      = "Ross "sv;
constexpr inline std::string_view LacailleCatalogPrefix  = "Lacaille "sv;
#endif

constexpr inline AstroCatalog::IndexNumber TYC3_MULTIPLIER = 1000000000u;
constexpr inline AstroCatalog::IndexNumber TYC2_MULTIPLIER = 10000u;
constexpr inline AstroCatalog::IndexNumber TYC123_MIN = 1u;
constexpr inline AstroCatalog::IndexNumber TYC1_MAX   = 9999u;  // actual upper limit is 9537 in TYC2
constexpr inline AstroCatalog::IndexNumber TYC2_MAX   = 99999u; // actual upper limit is 12121 in TYC2
constexpr inline AstroCatalog::IndexNumber TYC3_MAX   = 3u;     // from TYC2

// In the original Tycho catalog, TYC3 ranges from 1 to 3, so no there is
// no chance of overflow in the multiplication. TDSC (Fabricius et al. 2002)
// adds one entry with TYC3 = 4 (TYC 2907-1276-4) so permit TYC=4 when the
// TYC1 number is <= 2907
constexpr inline AstroCatalog::IndexNumber TDSC_TYC3_MAX            = 4u;
constexpr inline AstroCatalog::IndexNumber TDSC_TYC3_MAX_RANGE_TYC1 = 2907u;

bool
parseSimpleCatalogNumber(std::string_view name,
                         std::string_view prefix,
                         AstroCatalog::IndexNumber& catalogNumber)
{
    using celestia::compat::from_chars;
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
parseHIPPARCOSCatalogNumber(std::string_view name,
                            AstroCatalog::IndexNumber& catalogNumber)
{
    return parseSimpleCatalogNumber(name,
                                    HIPPARCOSCatalogPrefix,
                                    catalogNumber);
}

bool
parseHDCatalogNumber(std::string_view name,
                     AstroCatalog::IndexNumber& catalogNumber)
{
    return parseSimpleCatalogNumber(name,
                                    HDCatalogPrefix,
                                    catalogNumber);
}

bool
parseTychoCatalogNumber(std::string_view name,
                        AstroCatalog::IndexNumber& catalogNumber)
{
    using celestia::compat::from_chars;
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

        catalogNumber = tycParts[2] * TYC3_MULTIPLIER
                      + tycParts[1] * TYC2_MULTIPLIER
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

std::string
catalogNumberToString(AstroCatalog::IndexNumber catalogNumber)
{
    if (catalogNumber <= StarDatabase::MAX_HIPPARCOS_NUMBER)
    {
        return fmt::format("HIP {}", catalogNumber);
    }
    else
    {
        AstroCatalog::IndexNumber tyc3 = catalogNumber / TYC3_MULTIPLIER;
        catalogNumber -= tyc3 * TYC3_MULTIPLIER;
        AstroCatalog::IndexNumber tyc2 = catalogNumber / TYC2_MULTIPLIER;
        catalogNumber -= tyc2 * TYC2_MULTIPLIER;
        AstroCatalog::IndexNumber tyc1 = catalogNumber;
        return fmt::format("TYC {}-{}-{}", tyc1, tyc2, tyc3);
    }
}

} // end unnamed namespace

StarDatabase::StarDatabase()
{
    crossIndexes.resize(static_cast<std::size_t>(StarCatalog::MaxCatalog));
}

StarDatabase::~StarDatabase()
{
    delete[] stars;
}

Star*
StarDatabase::find(AstroCatalog::IndexNumber catalogNumber) const
{
    auto star = std::lower_bound(catalogNumberIndex.cbegin(), catalogNumberIndex.cend(),
                                 catalogNumber,
                                 [](const Star* star, AstroCatalog::IndexNumber catNum) { return star->getIndex() < catNum; });

    if (star != catalogNumberIndex.cend() && (*star)->getIndex() == catalogNumber)
        return *star;
    else
        return nullptr;
}

AstroCatalog::IndexNumber
StarDatabase::findCatalogNumberByName(std::string_view name, bool i18n) const
{
    if (name.empty())
        return AstroCatalog::InvalidIndex;

    AstroCatalog::IndexNumber catalogNumber = AstroCatalog::InvalidIndex;

    if (namesDB != nullptr)
    {
        catalogNumber = namesDB->findCatalogNumberByName(name, i18n);
        if (catalogNumber != AstroCatalog::InvalidIndex)
            return catalogNumber;
    }

    if (parseCelestiaCatalogNumber(name, catalogNumber))
        return catalogNumber;
    if (parseHIPPARCOSCatalogNumber(name, catalogNumber))
        return catalogNumber;
    if (parseTychoCatalogNumber(name, catalogNumber))
        return catalogNumber;
    if (parseHDCatalogNumber(name, catalogNumber))
        return searchCrossIndexForCatalogNumber(StarCatalog::HenryDraper, catalogNumber);
    if (parseSimpleCatalogNumber(name, SAOCatalogPrefix, catalogNumber))
        return searchCrossIndexForCatalogNumber(StarCatalog::SAO, catalogNumber);

    return AstroCatalog::InvalidIndex;
}

Star*
StarDatabase::find(std::string_view name, bool i18n) const
{
    AstroCatalog::IndexNumber catalogNumber = findCatalogNumberByName(name, i18n);
    if (catalogNumber != AstroCatalog::InvalidIndex)
        return find(catalogNumber);
    else
        return nullptr;
}

AstroCatalog::IndexNumber
StarDatabase::crossIndex(StarCatalog catalog, AstroCatalog::IndexNumber celCatalogNumber) const
{
    auto catalogIndex = static_cast<std::size_t>(catalog);
    if (catalogIndex >= crossIndexes.size())
        return AstroCatalog::InvalidIndex;

    const CrossIndex& xindex = crossIndexes[catalogIndex];

    // A simple linear search.  We could store cross indices sorted by
    // both catalog numbers and trade memory for speed
    auto iter = std::find_if(xindex.begin(), xindex.end(),
                             [celCatalogNumber](const CrossIndexEntry& o) { return celCatalogNumber == o.celCatalogNumber; });
    return iter == xindex.end()
        ? AstroCatalog::InvalidIndex
        : iter->catalogNumber;
}

// Return the Celestia catalog number for the star with a specified number
// in a cross index.
AstroCatalog::IndexNumber
StarDatabase::searchCrossIndexForCatalogNumber(StarCatalog catalog, AstroCatalog::IndexNumber number) const
{
    auto catalogIndex = static_cast<std::size_t>(catalog);
    if (catalogIndex >= crossIndexes.size())
        return AstroCatalog::InvalidIndex;

    const CrossIndex& xindex = crossIndexes[catalogIndex];
    auto iter = std::lower_bound(xindex.begin(), xindex.end(), number,
                                 [](const CrossIndexEntry& ent, AstroCatalog::IndexNumber n) { return ent.catalogNumber < n; });
    return iter == xindex.end() || iter->catalogNumber != number
        ? AstroCatalog::InvalidIndex
        : iter->celCatalogNumber;
}

Star*
StarDatabase::searchCrossIndex(StarCatalog catalog, AstroCatalog::IndexNumber number) const
{
    AstroCatalog::IndexNumber celCatalogNumber = searchCrossIndexForCatalogNumber(catalog, number);
    if (celCatalogNumber != AstroCatalog::InvalidIndex)
        return find(celCatalogNumber);
    else
        return nullptr;
}

void
StarDatabase::getCompletion(std::vector<std::string>& completion, std::string_view name) const
{
    // only named stars are supported by completion.
    if (!name.empty() && namesDB != nullptr)
        namesDB->getCompletion(completion, name);
}

// Return the name for the star with specified catalog number.  The returned
// string will be:
//      the common name if it exists, otherwise
//      the Bayer or Flamsteed designation if it exists, otherwise
//      the HD catalog number if it exists, otherwise
//      the HIPPARCOS catalog number.
//
// CAREFUL:
// If the star name is not present in the names database, a new
// string is constructed to contain the catalog number--keep in
// mind that calling this method could possibly incur the overhead
// of a memory allocation (though no explcit deallocation is
// required as it's all wrapped in the string class.)
std::string
StarDatabase::getStarName(const Star& star, bool i18n) const
{
    AstroCatalog::IndexNumber catalogNumber = star.getIndex();

    if (namesDB != nullptr)
    {
        if (auto iter = namesDB->getFirstNameIter(catalogNumber);
            iter != namesDB->getFinalNameIter() && iter->first == catalogNumber)
        {
            if (i18n)
            {
                const char * local = D_(iter->second.c_str());
                if (iter->second != local)
                    return local;
            }
            return iter->second;
        }
    }

    /*
      // Get the HD catalog name
      if (star.getIndex() != AstroCatalog::InvalidIndex)
      return fmt::format("HD {}", star.getIndex(Star::HDCatalog));
      else
    */
    return catalogNumberToString(catalogNumber);
}

std::string
StarDatabase::getStarNameList(const Star& star, const unsigned int maxNames) const
{
    std::string starNames;
    unsigned int catalogNumber = star.getIndex();
    std::set<std::string> nameSet;
    bool isNameSetEmpty = true;

    auto append = [&] (const std::string &str)
    {
        auto inserted = nameSet.insert(str);
        if (inserted.second)
        {
            if (isNameSetEmpty)
                isNameSetEmpty = false;
            else
                starNames += " / ";
            starNames += str;
        }
    };

    if (namesDB != nullptr)
    {
        for (auto iter = namesDB->getFirstNameIter(catalogNumber), end = namesDB->getFinalNameIter();
             iter != end && iter->first == catalogNumber && nameSet.size() < maxNames;
             ++iter)
        {
            append(D_(iter->second.c_str()));
        }
    }

    AstroCatalog::IndexNumber hip  = catalogNumber;
    if (hip != AstroCatalog::InvalidIndex && hip != 0 && nameSet.size() < maxNames)
    {
        if (hip <= Star::MaxTychoCatalogNumber)
        {
            if (hip >= 1000000)
            {
                AstroCatalog::IndexNumber h = hip;
                AstroCatalog::IndexNumber tyc3 = h / TYC3_MULTIPLIER;
                h -= tyc3 * TYC3_MULTIPLIER;
                AstroCatalog::IndexNumber tyc2 = h / TYC2_MULTIPLIER;
                h -= tyc2 * TYC2_MULTIPLIER;
                AstroCatalog::IndexNumber tyc1 = h;

                append(fmt::format("TYC {}-{}-{}", tyc1, tyc2, tyc3));
            }
            else
            {
                append(fmt::format("HIP {}", hip));
            }
        }
    }

    AstroCatalog::IndexNumber hd   = crossIndex(StarCatalog::HenryDraper, hip);
    if (nameSet.size() < maxNames && hd != AstroCatalog::InvalidIndex)
        append(fmt::format("HD {}", hd));

    AstroCatalog::IndexNumber sao   = crossIndex(StarCatalog::SAO, hip);
    if (nameSet.size() < maxNames && sao != AstroCatalog::InvalidIndex)
        append(fmt::format("SAO {}", sao));

    return starNames;
}

void
StarDatabase::findVisibleStars(StarHandler& starHandler,
                               const Eigen::Vector3f& position,
                               const Eigen::Quaternionf& orientation,
                               float fovY,
                               float aspectRatio,
                               float limitingMag) const
{
    // Compute the bounding planes of an infinite view frustum
    Eigen::Hyperplane<float, 3> frustumPlanes[5];
    Eigen::Vector3f planeNormals[5];
    Eigen::Matrix3f rot = orientation.toRotationMatrix();
    float h = std::tan(fovY * 0.5f);
    float w = h * aspectRatio;
    planeNormals[0] = Eigen::Vector3f(0.0f, 1.0f, -h);
    planeNormals[1] = Eigen::Vector3f(0.0f, -1.0f, -h);
    planeNormals[2] = Eigen::Vector3f(1.0f, 0.0f, -w);
    planeNormals[3] = Eigen::Vector3f(-1.0f, 0.0f, -w);
    planeNormals[4] = Eigen::Vector3f(0.0f, 0.0f, -1.0f);
    for (int i = 0; i < 5; i++)
    {
        planeNormals[i] = rot.transpose() * planeNormals[i].normalized();
        frustumPlanes[i] = Eigen::Hyperplane<float, 3>(planeNormals[i], position);
    }

    octreeRoot->processVisibleObjects(starHandler,
                                      position,
                                      frustumPlanes,
                                      limitingMag,
                                      STAR_OCTREE_ROOT_SIZE);
}

void
StarDatabase::findCloseStars(StarHandler& starHandler,
                             const Eigen::Vector3f& position,
                             float radius) const
{
    octreeRoot->processCloseObjects(starHandler,
                                    position,
                                    radius,
                                    STAR_OCTREE_ROOT_SIZE);
}

StarNameDatabase*
StarDatabase::getNameDatabase() const
{
    return namesDB.get();
}

} // end namespace celestia::engine
