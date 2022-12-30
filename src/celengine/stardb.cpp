// stardb.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <istream>
#include <set>
#include <string_view>
#include <system_error>

#include <fmt/format.h>

#include <celcompat/charconv.h>
#include <celutil/binaryread.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/tokenizer.h>
#include <celutil/stringutils.h>
#include "meshmanager.h"
#include "parser.h"
#include "stardb.h"
#include "starname.h"
#include "value.h"

using namespace std::string_view_literals;
using celestia::util::GetLogger;

namespace celutil = celestia::util;

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

// The size of the root star octree node is also the maximum distance
// distance from the Sun at which any star may be located. The current
// setting of 1.0e7 light years is large enough to contain the entire
// local group of galaxies. A larger value should be OK, but the
// performance implications for octree traversal still need to be
// investigated.
constexpr inline float STAR_OCTREE_ROOT_SIZE   = 1000000000.0f;

constexpr inline float STAR_OCTREE_MAGNITUDE   = 6.0f;
//constexpr const float STAR_EXTRA_ROOM        = 0.01f; // Reserve 1% capacity for extra stars

constexpr inline std::string_view FILE_HEADER            = "CELSTARS"sv;
constexpr inline std::string_view CROSSINDEX_FILE_HEADER = "CELINDEX"sv;

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


bool parseSimpleCatalogNumber(std::string_view name,
                              std::string_view prefix,
                              AstroCatalog::IndexNumber& catalogNumber)
{
    using celestia::compat::from_chars;
    if (compareIgnoringCase(name, prefix, prefix.size()) != 0)
    {
        return false;
    }

    // skip additional whitespace
    auto pos = name.find_first_not_of(" \t", prefix.size());
    if (pos == std::string_view::npos)
    {
        return false;
    }

    if (auto [ptr, ec] = from_chars(name.data() + pos, name.data() + name.size(), catalogNumber); ec == std::errc{})
    {
        // Do not match if suffix is present
        pos = name.find_first_not_of(" \t", ptr - name.data());
        return pos == std::string_view::npos;
    }

    return false;
}


bool parseHIPPARCOSCatalogNumber(std::string_view name,
                                 AstroCatalog::IndexNumber& catalogNumber)
{
    return parseSimpleCatalogNumber(name,
                                    HIPPARCOSCatalogPrefix,
                                    catalogNumber);
}


bool parseHDCatalogNumber(std::string_view name,
                          AstroCatalog::IndexNumber& catalogNumber)
{
    return parseSimpleCatalogNumber(name,
                                    HDCatalogPrefix,
                                    catalogNumber);
}


bool parseTychoCatalogNumber(std::string_view name,
                             AstroCatalog::IndexNumber& catalogNumber)
{
    using celestia::compat::from_chars;
    if (compareIgnoringCase(name, TychoCatalogPrefix, TychoCatalogPrefix.size()) != 0)
    {
        return false;
    }

    // skip additional whitespace
    auto pos = name.find_first_not_of(" \t", TychoCatalogPrefix.size());
    if (pos == std::string_view::npos) { return false; }

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
        if (pos != std::string_view::npos) { return false; }

        catalogNumber = tycParts[2] * TYC3_MULTIPLIER
                      + tycParts[1] * TYC2_MULTIPLIER
                      + tycParts[0];
        return true;
    }

    return false;
}


bool parseCelestiaCatalogNumber(std::string_view name,
                                AstroCatalog::IndexNumber& catalogNumber)
{
    using celestia::compat::from_chars;
    if (name.size() == 0 || name[0] != '#') { return false; }
    if (auto [ptr, ec] = from_chars(name.data() + 1, name.data() + name.size(), catalogNumber); ec == std::errc{})
    {
        // Do not match if suffix is present
        auto pos = name.find_first_not_of(" \t", ptr - name.data());
        return pos == std::string_view::npos;
    }

    return false;
}


std::string catalogNumberToString(AstroCatalog::IndexNumber catalogNumber)
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


void stcError(const Tokenizer& tok,
              std::string_view msg)
{
    GetLogger()->error(_("Error in .stc file (line {}): {}\n"), tok.getLineNumber(), msg);
}
} // end unnamed namespace


bool StarDatabase::CrossIndexEntry::operator<(const StarDatabase::CrossIndexEntry& e) const
{
    return catalogNumber < e.catalogNumber;
}


StarDatabase::StarDatabase()
{
    crossIndexes.resize(MaxCatalog);
}


StarDatabase::~StarDatabase()
{
    delete [] stars;
    delete [] catalogNumberIndex;

    for (const auto index : crossIndexes)
        delete index;
}


Star* StarDatabase::find(AstroCatalog::IndexNumber catalogNumber) const
{
    Star refStar;
    refStar.setIndex(catalogNumber);

    Star** star = std::lower_bound(catalogNumberIndex,
                                   catalogNumberIndex + nStars,
                                   catalogNumber,
                                   [](const Star* star, AstroCatalog::IndexNumber catNum) { return star->getIndex() < catNum; });

    if (star != catalogNumberIndex + nStars && (*star)->getIndex() == catalogNumber)
        return *star;
    else
        return nullptr;
}


AstroCatalog::IndexNumber StarDatabase::findCatalogNumberByName(const std::string& name, bool i18n) const
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
    {
        return catalogNumber;
    }
    else if (parseHIPPARCOSCatalogNumber(name, catalogNumber))
    {
        return catalogNumber;
    }
    else if (parseTychoCatalogNumber(name, catalogNumber))
    {
        return catalogNumber;
    }
    else if (parseHDCatalogNumber(name, catalogNumber))
    {
        return searchCrossIndexForCatalogNumber(HenryDraper, catalogNumber);
    }
    else if (parseSimpleCatalogNumber(name, SAOCatalogPrefix,
                                      catalogNumber))
    {
        return searchCrossIndexForCatalogNumber(SAO, catalogNumber);
    }
    else
    {
        return AstroCatalog::InvalidIndex;
    }
}


Star* StarDatabase::find(const std::string& name, bool i18n) const
{
    AstroCatalog::IndexNumber catalogNumber = findCatalogNumberByName(name, i18n);
    if (catalogNumber != AstroCatalog::InvalidIndex)
        return find(catalogNumber);
    else
        return nullptr;
}


AstroCatalog::IndexNumber StarDatabase::crossIndex(const Catalog catalog, const AstroCatalog::IndexNumber celCatalogNumber) const
{
    if (static_cast<std::size_t>(catalog) >= crossIndexes.size())
        return AstroCatalog::InvalidIndex;

    CrossIndex* xindex = crossIndexes[catalog];
    if (xindex == nullptr)
        return AstroCatalog::InvalidIndex;

    // A simple linear search.  We could store cross indices sorted by
    // both catalog numbers and trade memory for speed
    auto iter = std::find_if(xindex->begin(), xindex->end(),
                             [celCatalogNumber](CrossIndexEntry& o){ return celCatalogNumber == o.celCatalogNumber; });
    if (iter != xindex->end())
        return iter->catalogNumber;

    return AstroCatalog::InvalidIndex;
}


// Return the Celestia catalog number for the star with a specified number
// in a cross index.
AstroCatalog::IndexNumber StarDatabase::searchCrossIndexForCatalogNumber(const Catalog catalog, const AstroCatalog::IndexNumber number) const
{
    if (static_cast<unsigned int>(catalog) >= crossIndexes.size())
        return AstroCatalog::InvalidIndex;

    CrossIndex* xindex = crossIndexes[catalog];
    if (xindex == nullptr)
        return AstroCatalog::InvalidIndex;

    CrossIndexEntry xindexEnt;
    xindexEnt.catalogNumber = number;

    CrossIndex::iterator iter = lower_bound(xindex->begin(), xindex->end(),
                                            xindexEnt);
    if (iter == xindex->end() || iter->catalogNumber != number)
        return AstroCatalog::InvalidIndex;
    else
        return iter->celCatalogNumber;
}


Star* StarDatabase::searchCrossIndex(const Catalog catalog, const AstroCatalog::IndexNumber number) const
{
    AstroCatalog::IndexNumber celCatalogNumber = searchCrossIndexForCatalogNumber(catalog, number);
    if (celCatalogNumber != AstroCatalog::InvalidIndex)
        return find(celCatalogNumber);
    else
        return nullptr;
}


std::vector<std::string> StarDatabase::getCompletion(const std::string& name, bool i18n) const
{
    std::vector<std::string> completion;

    // only named stars are supported by completion.
    if (!name.empty() && namesDB != nullptr)
        return namesDB->getCompletion(name, i18n);
    else
        return completion;
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
std::string StarDatabase::getStarName(const Star& star, bool i18n) const
{
    AstroCatalog::IndexNumber catalogNumber = star.getIndex();

    if (namesDB != nullptr)
    {
        StarNameDatabase::NumberIndex::const_iterator iter = namesDB->getFirstNameIter(catalogNumber);
        if (iter != namesDB->getFinalNameIter() && iter->first == catalogNumber)
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


std::string StarDatabase::getStarNameList(const Star& star, const unsigned int maxNames) const
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
        StarNameDatabase::NumberIndex::const_iterator iter = namesDB->getFirstNameIter(catalogNumber);

        while (iter != namesDB->getFinalNameIter() && iter->first == catalogNumber && nameSet.size() < maxNames)
        {
            append(D_(iter->second.c_str()));
            ++iter;
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

    AstroCatalog::IndexNumber hd   = crossIndex(StarDatabase::HenryDraper, hip);
    if (nameSet.size() < maxNames && hd != AstroCatalog::InvalidIndex)
    {
        append(fmt::format("HD {}", hd));
    }

    AstroCatalog::IndexNumber sao   = crossIndex(StarDatabase::SAO, hip);
    if (nameSet.size() < maxNames && sao != AstroCatalog::InvalidIndex)
    {
        append(fmt::format("SAO {}", sao));
    }

    return starNames;
}


void StarDatabase::findVisibleStars(StarHandler& starHandler,
                                    const Eigen::Vector3f& position,
                                    const Eigen::Quaternionf& orientation,
                                    float fovY,
                                    float aspectRatio,
                                    float limitingMag,
                                    OctreeProcStats *stats) const
{
    // Compute the bounding planes of an infinite view frustum
    Eigen::Hyperplane<float, 3> frustumPlanes[5];
    Eigen::Vector3f planeNormals[5];
    Eigen::Matrix3f rot = orientation.toRotationMatrix();
    float h = (float) tan(fovY / 2);
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
                                      STAR_OCTREE_ROOT_SIZE,
                                      stats);
}


void StarDatabase::findCloseStars(StarHandler& starHandler,
                                  const Eigen::Vector3f& position,
                                  float radius) const
{
    octreeRoot->processCloseObjects(starHandler,
                                    position,
                                    radius,
                                    STAR_OCTREE_ROOT_SIZE);
}


StarNameDatabase* StarDatabase::getNameDatabase() const
{
    return namesDB;
}


void StarDatabase::setNameDatabase(StarNameDatabase* _namesDB)
{
    namesDB = _namesDB;
}


bool StarDatabase::loadCrossIndex(const Catalog catalog, std::istream& in)
{
    if (static_cast<unsigned int>(catalog) >= crossIndexes.size())
        return false;

    if (crossIndexes[catalog] != nullptr)
        delete crossIndexes[catalog];

    // Verify that the star database file has a correct header
    {
        std::array<char, CROSSINDEX_FILE_HEADER.size()> header;
        if (!in.read(header.data(), header.size()).good()
            || CROSSINDEX_FILE_HEADER != std::string_view(header.data(), header.size()))
        {
            GetLogger()->error(_("Bad header for cross index\n"));
            return false;
        }
    }

    // Verify the version
    {
        std::uint16_t version;
        if (!celutil::readLE<std::uint16_t>(in, version) || version != 0x0100)
        {
            GetLogger()->error(_("Bad version for cross index\n"));
            return false;
        }
    }

    CrossIndex* xindex = new CrossIndex();

    unsigned int record = 0;
    for (;;)
    {
        CrossIndexEntry ent;
        if (!celutil::readLE<AstroCatalog::IndexNumber>(in, ent.catalogNumber))
        {
            if (in.eof()) { break; }
            GetLogger()->error(_("Loading cross index failed\n"));
            delete xindex;
            return false;
        }

        if (!celutil::readLE<AstroCatalog::IndexNumber>(in, ent.celCatalogNumber))
        {
            GetLogger()->error(_("Loading cross index failed at record {}\n"), record);
            delete xindex;
            return false;
        }

        xindex->push_back(ent);

        record++;
    }

    sort(xindex->begin(), xindex->end());

    crossIndexes[catalog] = xindex;

    return true;
}


bool StarDatabase::loadBinary(std::istream& in)
{
    std::uint32_t nStarsInFile = 0;

    // Verify that the star database file has a correct header
    {
        std::array<char, FILE_HEADER.size()> header;
        if (!in.read(header.data(), header.size()).good()
            || FILE_HEADER != std::string_view(header.data(), header.size())) {
            return false;
        }
    }

    // Verify the version
    {
        std::uint16_t version;
        if (!celutil::readLE<std::uint16_t>(in, version) || version != 0x0100)
        {
            return false;
        }
    }

    // Read the star count
    if (!celutil::readLE<std::uint32_t>(in, nStarsInFile))
    {
        return false;
    }

    std::uint32_t totalStars = nStars + nStarsInFile;

    while (nStars < totalStars)
    {
        AstroCatalog::IndexNumber catNo = 0;
        float x = 0.0f, y = 0.0f, z = 0.0f;
        std::int16_t absMag;
        std::uint16_t spectralType;

        if (!celutil::readLE<AstroCatalog::IndexNumber>(in, catNo)
            || !celutil::readLE<float>(in, x)
            || !celutil::readLE<float>(in, y)
            || !celutil::readLE<float>(in, z)
            || !celutil::readLE<std::int16_t>(in, absMag)
            || !celutil::readLE<std::uint16_t>(in, spectralType))
        {
            return false;
        }

        Star star;
        star.setPosition(x, y, z);
        star.setAbsoluteMagnitude(static_cast<float>(absMag) / 256.0f);

        StarDetails* details = nullptr;
        StellarClass sc;
        if (sc.unpackV1(spectralType))
            details = StarDetails::GetStarDetails(sc);

        if (details == nullptr)
        {
            GetLogger()->error(_("Bad spectral type in star database, star #{}\n"), nStars);
            return false;
        }

        star.setDetails(details);
        star.setIndex(catNo);
        unsortedStars.add(star);

        nStars++;
    }

    if (in.bad())
        return false;

    GetLogger()->debug("StarDatabase::read: nStars = {}\n", nStarsInFile);
    GetLogger()->info(_("{} stars in binary database\n"), nStars);

    // Create the temporary list of stars sorted by catalog number; this
    // will be used to lookup stars during file loading. After loading is
    // complete, the stars are sorted into an octree and this list gets
    // replaced.
    if (unsortedStars.size() > 0)
    {
        binFileStarCount = unsortedStars.size();
        binFileCatalogNumberIndex = new Star*[binFileStarCount];
        for (unsigned int i = 0; i < binFileStarCount; i++)
        {
            binFileCatalogNumberIndex[i] = &unsortedStars[i];
        }
        std::sort(binFileCatalogNumberIndex, binFileCatalogNumberIndex + binFileStarCount,
                  [](const Star* star0, const Star* star1) { return star0->getIndex() < star1->getIndex(); });
    }

    return true;
}


void StarDatabase::finish()
{
    GetLogger()->info(_("Total star count: {}\n"), nStars);

    buildOctree();
    buildIndexes();

    // Delete the temporary indices used only during loading
    delete[] binFileCatalogNumberIndex;
    stcFileCatalogNumberIndex.clear();

    // Resolve all barycenters; this can't be done before star sorting. There's
    // still a bug here: final orbital radii aren't available until after
    // the barycenters have been resolved, and these are required when building
    // the octree.  This will only rarely cause a problem, but it still needs
    // to be addressed.
    for (const auto& b : barycenters)
    {
        Star* star = find(b.catNo);
        Star* barycenter = find(b.barycenterCatNo);
        assert(star != nullptr);
        assert(barycenter != nullptr);
        if (star != nullptr && barycenter != nullptr)
        {
            star->setOrbitBarycenter(barycenter);
            barycenter->addOrbitingStar(star);
        }
    }

    barycenters.clear();
}


/*! Load star data from a property list into a star instance.
 */
bool StarDatabase::createStar(Star* star,
                              DataDisposition disposition,
                              AstroCatalog::IndexNumber catalogNumber,
                              Hash* starData,
                              const fs::path& path,
                              bool isBarycenter)
{
    StarDetails* details = nullptr;
    std::string spectralType;

    // Get the magnitude and spectral type; if the star is actually
    // a barycenter placeholder, these fields are ignored.
    if (isBarycenter)
    {
        details = StarDetails::GetBarycenterDetails();
    }
    else
    {
        if (starData->getString("SpectralType", spectralType))
        {
            StellarClass sc = StellarClass::parse(spectralType);
            details = StarDetails::GetStarDetails(sc);
            if (details == nullptr)
            {
                GetLogger()->error(_("Invalid star: bad spectral type.\n"));
                return false;
            }
        }
        else
        {
            // Spectral type is required for new stars
            if (disposition != DataDisposition::Modify)
            {
                GetLogger()->error(_("Invalid star: missing spectral type.\n"));
                return false;
            }
        }
    }

    bool modifyExistingDetails = false;
    if (disposition == DataDisposition::Modify)
    {
        StarDetails* existingDetails = star->getDetails();

        // If we're modifying an existing star and it already has a
        // customized details record, we'll just modify that.
        if (!existingDetails->shared())
        {
            modifyExistingDetails = true;
            if (details != nullptr)
            {
                // If the spectral type was modified, copy the new data
                // to the custom details record.
                existingDetails->setSpectralType(details->getSpectralType());
                existingDetails->setTemperature(details->getTemperature());
                existingDetails->setBolometricCorrection(details->getBolometricCorrection());
                if ((existingDetails->getKnowledge() & StarDetails::KnowTexture) == 0)
                    existingDetails->setTexture(details->getTexture());
                if ((existingDetails->getKnowledge() & StarDetails::KnowRotation) == 0)
                    existingDetails->setRotationModel(details->getRotationModel());
                existingDetails->setVisibility(details->getVisibility());
            }

            details = existingDetails;
        }
        else if (details == nullptr)
        {
            details = existingDetails;
        }
    }

    std::string modelName;
    std::string textureName;
    bool hasTexture = starData->getString("Texture", textureName);
    bool hasModel = starData->getString("Mesh", modelName);

    RotationModel* rm = CreateRotationModel(starData, path, 1.0);
    bool hasRotationModel = (rm != nullptr);

    Eigen::Vector3d semiAxes = Eigen::Vector3d::Ones();
    bool hasSemiAxes = starData->getLengthVector("SemiAxes", semiAxes);
    bool hasBarycenter = false;
    Eigen::Vector3f barycenterPosition;

    double radius;
    bool hasRadius = starData->getLength("Radius", radius);

    double temperature = 0.0;
    bool hasTemperature = starData->getNumber("Temperature", temperature);
    // disallow unphysical temperature values
    if (temperature <= 0.0)
    {
        hasTemperature = false;
    }

    double bolometricCorrection;
    bool hasBolometricCorrection = starData->getNumber("BoloCorrection", bolometricCorrection);

    std::string infoURL;
    bool hasInfoURL = starData->getString("InfoURL", infoURL);

    Orbit* orbit = CreateOrbit(Selection(), starData, path, true);

    if (hasTexture              ||
        hasModel                ||
        orbit != nullptr        ||
        hasSemiAxes             ||
        hasRadius               ||
        hasTemperature          ||
        hasBolometricCorrection ||
        hasRotationModel        ||
        hasInfoURL)
    {
        // If the star definition has extended information, clone the
        // star details so we can customize it without affecting other
        // stars of the same spectral type.
        bool free_details = false;
        if (!modifyExistingDetails)
        {
            details = new StarDetails(*details);
            free_details = true;
        }

        if (hasTexture)
        {
            details->setTexture(MultiResTexture(textureName, path));
            details->addKnowledge(StarDetails::KnowTexture);
        }

        if (hasModel)
        {
            ResourceHandle geometryHandle = GetGeometryManager()->getHandle(GeometryInfo(modelName,
                                                                                         path,
                                                                                         Eigen::Vector3f::Zero(),
                                                                                         1.0f,
                                                                                         true));
            details->setGeometry(geometryHandle);
        }

        if (hasSemiAxes)
        {
            details->setEllipsoidSemiAxes(semiAxes.cast<float>());
        }

        if (hasRadius)
        {
            details->setRadius((float) radius);
            details->addKnowledge(StarDetails::KnowRadius);
        }

        if (hasTemperature)
        {
            details->setTemperature((float) temperature);

            if (!hasBolometricCorrection)
            {
                // if we change the temperature, recalculate the bolometric
                // correction using formula from formula for main sequence
                // stars given in B. Cameron Reed (1998), "The Composite
                // Observational-Theoretical HR Diagram", Journal of the Royal
                // Astronomical Society of Canada, Vol 92. p36.

                double logT = std::log10(temperature) - 4;
                double bc = -8.499 * std::pow(logT, 4) + 13.421 * std::pow(logT, 3)
                            - 8.131 * logT * logT - 3.901 * logT - 0.438;

                details->setBolometricCorrection((float) bc);
            }
        }

        if (hasBolometricCorrection)
        {
            details->setBolometricCorrection((float) bolometricCorrection);
        }

        if (hasInfoURL)
        {
            details->setInfoURL(infoURL);
        }

        if (orbit != nullptr)
        {
            details->setOrbit(orbit);

            // See if a barycenter was specified as well
            AstroCatalog::IndexNumber barycenterCatNo = AstroCatalog::InvalidIndex;
            bool barycenterDefined = false;

            std::string barycenterName;
            if (starData->getString("OrbitBarycenter", barycenterName))
            {
                barycenterCatNo   = findCatalogNumberByName(barycenterName, false);
                barycenterDefined = true;
            }
            else if (starData->getNumber("OrbitBarycenter", barycenterCatNo))
            {
                barycenterDefined = true;
            }

            if (barycenterDefined)
            {
                if (barycenterCatNo != AstroCatalog::InvalidIndex)
                {
                    // We can't actually resolve the barycenter catalog number
                    // to a Star pointer until after all stars have been loaded
                    // and spatially sorted.  Just store it in a list to be
                    // resolved after sorting.
                    BarycenterUsage bc;
                    bc.catNo = catalogNumber;
                    bc.barycenterCatNo = barycenterCatNo;
                    barycenters.push_back(bc);

                    // Even though we can't actually get the Star pointer for
                    // the barycenter, we can get the star information.
                    Star* barycenter = findWhileLoading(barycenterCatNo);
                    if (barycenter != nullptr)
                    {
                        hasBarycenter = true;
                        barycenterPosition = barycenter->getPosition();
                    }
                }

                if (!hasBarycenter)
                {
                    GetLogger()->error(_("Barycenter {} does not exist.\n"), barycenterName);
                    delete rm;
                    if (free_details)
                        delete details;
                    return false;
                }
            }
        }

        if (hasRotationModel)
            details->setRotationModel(rm);
    }

    if (!modifyExistingDetails)
        star->setDetails(details);
    if (disposition != DataDisposition::Modify)
        star->setIndex(catalogNumber);

    // Compute the position in rectangular coordinates.  If a star has an
    // orbit and barycenter, it's position is the position of the barycenter.
    if (hasBarycenter)
    {
        star->setPosition(barycenterPosition);
    }
    else
    {
        double ra = 0.0;
        double dec = 0.0;
        double distance = 0.0;

        if (disposition == DataDisposition::Modify)
        {
            Eigen::Vector3f pos = star->getPosition();

            // Convert from Celestia's coordinate system
            Eigen::Vector3f v(pos.x(), -pos.z(), pos.y());
            v = Eigen::Quaternionf(Eigen::AngleAxis<float>((float) astro::J2000Obliquity, Eigen::Vector3f::UnitX())) * v;

            distance = v.norm();
            if (distance > 0.0)
            {
                v.normalize();
                ra = celmath::radToDeg(std::atan2(v.y(), v.x())) / DEG_PER_HRA;
                dec = celmath::radToDeg(std::asin(v.z()));
            }
        }

        bool modifyPosition = false;
        if (starData->getAngle("RA", ra, DEG_PER_HRA, 1.0))
        {
            modifyPosition = true;
        }
        else
        {
            if (disposition != DataDisposition::Modify)
            {
                GetLogger()->error(_("Invalid star: missing right ascension\n"));
                return false;
            }
        }

        if (starData->getAngle("Dec", dec))
        {
            modifyPosition = true;
        }
        else
        {
            if (disposition != DataDisposition::Modify)
            {
                GetLogger()->error(_("Invalid star: missing declination.\n"));
                return false;
            }
        }

        if (starData->getLength("Distance", distance, KM_PER_LY))
        {
            modifyPosition = true;
        }
        else
        {
            if (disposition != DataDisposition::Modify)
            {
                GetLogger()->error(_("Invalid star: missing distance.\n"));
                return false;
            }
        }

        // Truncate to floats to match behavior of reading from binary file.
        // The conversion to rectangular coordinates is still performed at
        // double precision, however.
        if (modifyPosition)
        {
            float raf = ((float) ra);
            float decf = ((float) dec);
            float distancef = ((float) distance);
            Eigen::Vector3d pos = astro::equatorialToCelestialCart((double) raf, (double) decf, (double) distancef);
            star->setPosition(pos.cast<float>());
        }
    }

    if (isBarycenter)
    {
        star->setAbsoluteMagnitude(30.0f);
    }
    else
    {
        float magnitude = 0.0f;
        bool magnitudeModified = true;
        bool absoluteDefined = true;
        if (!starData->getNumber("AbsMag", magnitude))
        {
            absoluteDefined = false;
            if (!starData->getNumber("AppMag", magnitude))
            {
                if (disposition != DataDisposition::Modify)
                {
                    GetLogger()->error(_("Invalid star: missing magnitude.\n"));
                    return false;
                }
                else
                {
                    magnitudeModified = false;
                }
            }
            else
            {
                float distance = star->getPosition().norm();

                // We can't compute the intrinsic brightness of the star from
                // the apparent magnitude if the star is within a few AU of the
                // origin.
                if (distance < 1e-5f)
                {
                    GetLogger()->error(_("Invalid star: absolute (not apparent) magnitude must be specified for star near origin\n"));
                    return false;
                }
                magnitude = astro::appToAbsMag(magnitude, distance);
            }
        }

        if (magnitudeModified)
            star->setAbsoluteMagnitude(magnitude);

        float extinction = 0.0f;
        if (starData->getNumber("Extinction", extinction))
        {
            float distance = star->getPosition().norm();
            if (distance != 0.0f)
                star->setExtinction(extinction / distance);
            else
                extinction = 0.0f;
            if (!absoluteDefined)
                star->setAbsoluteMagnitude(star->getAbsoluteMagnitude() - extinction);
        }
    }

    return true;
}


/*! Load an STC file with star definitions. Each definition has the form:
 *
 *  [disposition] [object type] [catalog number] [name]
 *  {
 *      [properties]
 *  }
 *
 *  Disposition is either Add, Replace, or Modify; Add is the default.
 *  Object type is either Star or Barycenter, with Star the default
 *  It is an error to omit both the catalog number and the name.
 *
 *  The dispositions are slightly more complicated than suggested by
 *  their names. Every star must have an unique catalog number. But
 *  instead of generating an error, Adding a star with a catalog
 *  number that already exists will actually replace that star. Here
 *  are how all of the possibilities are handled:
 *
 *  <name> or <number> already exists:
 *  Add <name>        : new star
 *  Add <number>      : replace star
 *  Replace <name>    : replace star
 *  Replace <number>  : replace star
 *  Modify <name>     : modify star
 *  Modify <number>   : modify star
 *
 *  <name> or <number> doesn't exist:
 *  Add <name>        : new star
 *  Add <number>      : new star
 *  Replace <name>    : new star
 *  Replace <number>  : new star
 *  Modify <name>     : error
 *  Modify <number>   : error
 */
bool StarDatabase::load(std::istream& in, const fs::path& resourcePath)
{
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

#ifdef ENABLE_NLS
    std::string s = resourcePath.string();
    const char *d = s.c_str();
    bindtextdomain(d, d); // domain name is the same as resource path
#endif

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        bool isStar = true;

        // Parse the disposition--either Add, Replace, or Modify. The disposition
        // may be omitted. The default value is Add.
        DataDisposition disposition = DataDisposition::Add;
        if (tokenizer.getTokenType() == Tokenizer::TokenName)
        {
            if (tokenizer.getStringValue() == "Modify")
            {
                disposition = DataDisposition::Modify;
                tokenizer.nextToken();
            }
            else if (tokenizer.getStringValue() == "Replace")
            {
                disposition = DataDisposition::Replace;
                tokenizer.nextToken();
            }
            else if (tokenizer.getStringValue() == "Add")
            {
                disposition = DataDisposition::Add;
                tokenizer.nextToken();
            }
        }

        // Parse the object type--either Star or Barycenter. The object type
        // may be omitted. The default is Star.
        if (tokenizer.getTokenType() == Tokenizer::TokenName)
        {
            if (tokenizer.getStringValue() == "Star")
            {
                isStar = true;
            }
            else if (tokenizer.getStringValue() == "Barycenter")
            {
                isStar = false;
            }
            else
            {
                stcError(tokenizer, "unrecognized object type");
                return false;
            }
            tokenizer.nextToken();
        }

        // Parse the catalog number; it may be omitted if a name is supplied.
        AstroCatalog::IndexNumber catalogNumber = AstroCatalog::InvalidIndex;
        if (tokenizer.getTokenType() == Tokenizer::TokenNumber)
        {
            catalogNumber = (AstroCatalog::IndexNumber) tokenizer.getNumberValue();
            tokenizer.nextToken();
        }

        std::string objName;
        std::string firstName;
        if (tokenizer.getTokenType() == Tokenizer::TokenString)
        {
            // A star name (or names) is present
            objName    = tokenizer.getStringValue();
            tokenizer.nextToken();
            if (!objName.empty())
            {
                std::string::size_type next = objName.find(':', 0);
                firstName = objName.substr(0, next);
            }
        }

        // now goes the star definition
        if (tokenizer.getTokenType() != Tokenizer::TokenBeginGroup)
        {
            GetLogger()->error("Unexpected token at line {}!\n", tokenizer.getLineNumber());
            return false;
        }

        Star* star = nullptr;

        switch (disposition)
        {
        case DataDisposition::Add:
            // Automatically generate a catalog number for the star if one isn't
            // supplied.
            if (catalogNumber == AstroCatalog::InvalidIndex)
            {
                if (!isStar && firstName.empty())
                {
                    GetLogger()->error("Bad barycenter: neither catalog number nor name set at line {}.\n", tokenizer.getLineNumber());
                    return false;
                }
                catalogNumber = nextAutoCatalogNumber--;
            }
            else
            {
                star = findWhileLoading(catalogNumber);
            }
            break;

        case DataDisposition::Replace:
            if (catalogNumber == AstroCatalog::InvalidIndex)
            {
                if (!firstName.empty())
                {
                    catalogNumber = findCatalogNumberByName(firstName, false);
                }
            }

            if (catalogNumber == AstroCatalog::InvalidIndex)
            {
                catalogNumber = nextAutoCatalogNumber--;
            }
            else
            {
                star = findWhileLoading(catalogNumber);
            }
            break;

        case DataDisposition::Modify:
            // If no catalog number was specified, try looking up the star by name
            if (catalogNumber == AstroCatalog::InvalidIndex && !firstName.empty())
            {
                catalogNumber = findCatalogNumberByName(firstName, false);
            }

            if (catalogNumber != AstroCatalog::InvalidIndex)
            {
                star = findWhileLoading(catalogNumber);
            }

            break;
        }

        bool isNewStar = star == nullptr;

        tokenizer.pushBack();

        Value* starDataValue = parser.readValue();
        if (starDataValue == nullptr)
        {
            GetLogger()->error("Error reading star at line {}.\n", tokenizer.getLineNumber());
            return false;
        }

        if (starDataValue->getType() != Value::HashType)
        {
            GetLogger()->error("Bad star definition at line {}.\n", tokenizer.getLineNumber());
            delete starDataValue;
            return false;
        }
        Hash* starData = starDataValue->getHash();

        if (isNewStar)
            star = new Star();

        bool ok = false;
        if (isNewStar && disposition == DataDisposition::Modify)
        {
            GetLogger()->warn("Modify requested for nonexistent star.\n");
        }
        else
        {
            ok = createStar(star, disposition, catalogNumber, starData, resourcePath, !isStar);
            star->loadCategories(starData, disposition, resourcePath.string());
        }
        delete starDataValue;

        if (ok)
        {
            if (isNewStar)
            {
                unsortedStars.add(*star);
                nStars++;
                delete star;

                // Add the new star to the temporary (load time) index.
                stcFileCatalogNumberIndex[catalogNumber] = &unsortedStars[unsortedStars.size() - 1];
            }

            if (namesDB != nullptr && !objName.empty())
            {
                // List of namesDB will replace any that already exist for
                // this star.
                namesDB->erase(catalogNumber);

                // Iterate through the string for names delimited
                // by ':', and insert them into the star database.
                // Note that db->add() will skip empty namesDB.
                std::string::size_type startPos = 0;
                while (startPos != std::string::npos)
                {
                    std::string::size_type next   = objName.find(':', startPos);
                    std::string::size_type length = std::string::npos;
                    if (next != std::string::npos)
                    {
                        length = next - startPos;
                        ++next;
                    }
                    std::string starName = objName.substr(startPos, length);
                    namesDB->add(catalogNumber, starName);
                    startPos = next;
                }
            }
        }
        else
        {
            if (isNewStar)
                delete star;
            GetLogger()->info("Bad star definition--will continue parsing file.\n");
        }
    }

    return true;
}


void StarDatabase::buildOctree()
{
    // This should only be called once for the database
    // ASSERT(octreeRoot == nullptr);

    GetLogger()->debug("Sorting stars into octree . . .\n");
    float absMag = astro::appToAbsMag(STAR_OCTREE_MAGNITUDE,
                                      STAR_OCTREE_ROOT_SIZE * (float) sqrt(3.0));
    DynamicStarOctree* root = new DynamicStarOctree(Eigen::Vector3f(1000.0f, 1000.0f, 1000.0f),
                                                    absMag);
    for (unsigned int i = 0; i < unsortedStars.size(); ++i)
    {
        root->insertObject(unsortedStars[i], STAR_OCTREE_ROOT_SIZE);
    }

    GetLogger()->debug("Spatially sorting stars for improved locality of reference . . .\n");
    Star* sortedStars    = new Star[nStars];
    Star* firstStar      = sortedStars;
    root->rebuildAndSort(octreeRoot, firstStar);

    // ASSERT((int) (firstStar - sortedStars) == nStars);
    GetLogger()->debug("{} stars total\nOctree has {} nodes and {} stars.\n",
                       firstStar - sortedStars,
                       1 + octreeRoot->countChildren(), octreeRoot->countObjects());
#ifdef PROFILE_OCTREE
    vector<OctreeLevelStatistics> stats;
    octreeRoot->computeStatistics(stats);
    int level = 0;
    for (const auto& stat : stats)
    {
        level++;
        clog << fmt::format(
                     "Level {}, {:.5f} ly, {} nodes, {} stars\n",
                     level,
                     STAR_OCTREE_ROOT_SIZE / pow(2.0, (double) level),
                     stat.nodeCount,
                     stat.objectCount;
    }
#endif

    // Clean up . . .
    //delete[] stars;
    unsortedStars.clear();
    delete root;

    stars = sortedStars;
}


void StarDatabase::buildIndexes()
{
    // This should only be called once for the database
    // assert(catalogNumberIndexes[0] == nullptr);

    GetLogger()->info("Building catalog number indexes . . .\n");

    catalogNumberIndex = new Star*[nStars];
    for (int i = 0; i < nStars; ++i)
        catalogNumberIndex[i] = &stars[i];

    std::sort(catalogNumberIndex, catalogNumberIndex + nStars,
              [](const Star* star0, const Star* star1) { return star0->getIndex() < star1->getIndex(); });
}


/*! While loading the star catalogs, this function must be called instead of
 *  find(). The final catalog number index for stars cannot be built until
 *  after all stars have been loaded. During catalog loading, there are two
 *  separate indexes: one for the binary catalog and another index for stars
 *  loaded from stc files. They binary catalog index is a sorted array, while
 *  the stc catalog index is an STL map. Since the binary file can be quite
 *  large, we want to avoid creating a map with as many nodes as there are
 *  stars. Stc files should collectively contain many fewer stars, and stars
 *  in an stc file may reference each other (barycenters). Thus, a dynamic
 *  structure like a map is both practical and essential.
 */
Star* StarDatabase::findWhileLoading(AstroCatalog::IndexNumber catalogNumber) const
{
    // First check for stars loaded from the binary database
    if (binFileCatalogNumberIndex != nullptr)
    {
        Star** star = std::lower_bound(binFileCatalogNumberIndex,
                                       binFileCatalogNumberIndex + binFileStarCount,
                                       catalogNumber,
                                       [](const Star* star, AstroCatalog::IndexNumber catNum) { return star->getIndex() < catNum; });

        if (star != binFileCatalogNumberIndex + binFileStarCount && (*star)->getIndex() == catalogNumber)
            return *star;
    }

    // Next check for stars loaded from an stc file
    std::map<AstroCatalog::IndexNumber, Star*>::const_iterator iter = stcFileCatalogNumberIndex.find(catalogNumber);
    if (iter != stcFileCatalogNumberIndex.end())
    {
        return iter->second;
    }

    // Star not found
    return nullptr;
}
