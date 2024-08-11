// stardbbuilder.cpp
//
// Copyright (C) 2001-2024, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "stardbbuilder.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <istream>
#include <iterator>
#include <string_view>
#include <type_traits>
#include <utility>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <Eigen/Geometry>

#include <fmt/format.h>

#include <celastro/astro.h>
#include <celmath/geomutil.h>
#include <celmath/mathlib.h>
#include <celutil/binaryread.h>
#include <celutil/fsutils.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/timer.h>
#include <celutil/tokenizer.h>
#include "hash.h"
#include "meshmanager.h"
#include "octreebuilder.h"
#include "parser.h"
#include "stardb.h"
#include "stellarclass.h"
#include "value.h"

using namespace std::string_view_literals;

namespace astro = celestia::astro;
namespace engine = celestia::engine;
namespace ephem = celestia::ephem;
namespace math = celestia::math;
namespace util = celestia::util;

using util::GetLogger;

struct StarDatabaseBuilder::StcHeader
{
    explicit StcHeader(const fs::path&);
    explicit StcHeader(fs::path&&) = delete;

    const fs::path* path;
    int lineNumber{ 0 };
    DataDisposition disposition{ DataDisposition::Add };
    bool isStar{ true };
    AstroCatalog::IndexNumber catalogNumber{ AstroCatalog::InvalidIndex };
    std::vector<std::string> names;
};

StarDatabaseBuilder::StcHeader::StcHeader(const fs::path& _path) :
    path(&_path)
{
}

template<>
struct fmt::formatter<StarDatabaseBuilder::StcHeader> : formatter<std::string_view>
{
    auto format(const StarDatabaseBuilder::StcHeader& header, format_context& ctx) const
    {
        fmt::basic_memory_buffer<char> data;
        fmt::format_to(std::back_inserter(data), "line {}", header.lineNumber);
        if (header.catalogNumber <= Star::MaxTychoCatalogNumber)
            fmt::format_to(std::back_inserter(data), " - HIP {}", header.catalogNumber);
        if (!header.names.empty())
            fmt::format_to(std::back_inserter(data), " - {}", header.names.front());
        return formatter<std::string_view>::format(std::string_view(data.data(), data.size()), ctx);
    }
};

namespace
{

// In testing, changing SPLIT_THRESHOLD from 100 to 50 nearly
// doubled the number of nodes in the tree, but provided only between a
// 0 to 5 percent frame rate improvement.
constexpr engine::OctreeObjectIndex StarOctreeSplitThreshold = 75;

// The octree node into which a star is placed is dependent on two properties:
// its obsPosition and its luminosity--the fainter the star, the deeper the node
// in which it will reside.  Each node stores an absolute magnitude; no child
// of the node is allowed contain a star brighter than this value, making it
// possible to determine quickly whether or not to cull subtrees.

struct StarOctreeTraits
{
    using ObjectType = Star;
    using PrecisionType = float;

    static Eigen::Vector3f getPosition(const ObjectType&);
    static float getRadius(const ObjectType&);
    static float getMagnitude(const ObjectType&);
    static float applyDecay(float);
};

inline Eigen::Vector3f
StarOctreeTraits::getPosition(const ObjectType& obj)
{
    return obj.getPosition();
}

inline float
StarOctreeTraits::getRadius(const ObjectType& obj)
{
    return obj.getOrbitalRadius();
}

inline float
StarOctreeTraits::getMagnitude(const ObjectType& obj)
{
    return obj.getAbsoluteMagnitude();
}

inline float
StarOctreeTraits::applyDecay(float factor)
{
    // Decrease in luminosity by factor of 4
    // -2.5 * log10(1.0 / 4.0) = 1.50515 (nearest float)
    return factor + 1.50515f;
}

constexpr float STAR_OCTREE_MAGNITUDE = 6.0f;

// We can't compute the intrinsic brightness of the star from
// the apparent magnitude if the star is within a few AU of the
// origin.
constexpr float VALID_APPMAG_DISTANCE_THRESHOLD = 1e-5f;

constexpr std::string_view STARSDAT_MAGIC = "CELSTARS"sv;
constexpr std::uint16_t StarDBVersion     = 0x0100;

#pragma pack(push, 1)

// stars.dat header structure
struct StarsDatHeader
{
    StarsDatHeader() = delete;
    char magic[8]; //NOSONAR
    std::uint16_t version;
    std::uint32_t counter;
};

// stars.dat record structure
struct StarsDatRecord
{
    StarsDatRecord() = delete;
    AstroCatalog::IndexNumber catNo;
    float x;
    float y;
    float z;
    std::int16_t absMag;
    std::uint16_t spectralType;
};

#pragma pack(pop)

static_assert(std::is_standard_layout_v<StarsDatHeader>);
static_assert(std::is_standard_layout_v<StarsDatRecord>);

bool
parseStarsDatHeader(std::istream& in, std::uint32_t& nStarsInFile)
{
    std::array<char, sizeof(StarsDatHeader)> header;
    if (!in.read(header.data(), header.size()).good()) /* Flawfinder: ignore */
        return false;

    // Verify the magic string
    if (auto magic = std::string_view(header.data() + offsetof(StarsDatHeader, magic), STARSDAT_MAGIC.size());
        magic != STARSDAT_MAGIC)
    {
        return false;
    }

    // Verify the version
    if (auto version = util::fromMemoryLE<std::uint16_t>(header.data() + offsetof(StarsDatHeader, version));
        version != StarDBVersion)
    {
        return false;
    }

    // Read the star count
    nStarsInFile = util::fromMemoryLE<std::uint32_t>(header.data() + offsetof(StarsDatHeader, counter));
    return true;
}

inline void
stcError(const StarDatabaseBuilder::StcHeader& header, std::string_view msg)
{
    GetLogger()->error(_("Error in .stc file ({}): {}\n"), header, msg);
}

inline void
stcWarn(const StarDatabaseBuilder::StcHeader& header, std::string_view msg)
{
    GetLogger()->warn(_("Warning in .stc file ({}): {}\n"), header, msg);
}

bool
parseStcHeader(Tokenizer& tokenizer, StarDatabaseBuilder::StcHeader& header)
{
    header.lineNumber = tokenizer.getLineNumber();

    header.isStar = true;

    // Parse the disposition--either Add, Replace, or Modify. The disposition
    // may be omitted. The default value is Add.
    header.disposition = DataDisposition::Add;
    if (auto tokenValue = tokenizer.getNameValue(); tokenValue.has_value())
    {
        if (*tokenValue == "Modify")
        {
            header.disposition = DataDisposition::Modify;
            tokenizer.nextToken();
        }
        else if (*tokenValue == "Replace")
        {
            header.disposition = DataDisposition::Replace;
            tokenizer.nextToken();
        }
        else if (*tokenValue == "Add")
        {
            header.disposition = DataDisposition::Add;
            tokenizer.nextToken();
        }
    }

    // Parse the object type--either Star or Barycenter. The object type
    // may be omitted. The default is Star.
    if (auto tokenValue = tokenizer.getNameValue(); tokenValue.has_value())
    {
        if (*tokenValue == "Star")
        {
            header.isStar = true;
        }
        else if (*tokenValue == "Barycenter")
        {
            header.isStar = false;
        }
        else
        {
            stcError(header, _("unrecognized object type"));
            return false;
        }
        tokenizer.nextToken();
    }

    // Parse the catalog number; it may be omitted if a name is supplied.
    header.catalogNumber = AstroCatalog::InvalidIndex;
    if (auto tokenValue = tokenizer.getNumberValue(); tokenValue.has_value())
    {
        header.catalogNumber = static_cast<AstroCatalog::IndexNumber>(*tokenValue);
        tokenizer.nextToken();
    }

    header.names.clear();
    if (auto tokenValue = tokenizer.getStringValue(); tokenValue.has_value())
    {
        for (std::string_view remaining = *tokenValue; !remaining.empty();)
        {
            auto pos = remaining.find(':');
            if (std::string_view name = remaining.substr(0, pos);
                !name.empty() && std::find(header.names.cbegin(), header.names.cend(), name) == header.names.cend())
            {
                header.names.emplace_back(name);
            }

            if (pos == std::string_view::npos || header.names.size() == StarDatabase::MAX_STAR_NAMES)
                break;

            remaining = remaining.substr(pos + 1);
        }

        tokenizer.nextToken();
    }
    else if (header.catalogNumber == AstroCatalog::InvalidIndex)
    {
        stcError(header, _("entry missing name and catalog number"));
        return false;
    }

    return true;
}

bool
checkSpectralType(const StarDatabaseBuilder::StcHeader& header,
                  const AssociativeArray* starData,
                  const Star* star,
                  boost::intrusive_ptr<StarDetails>& newDetails)
{
    const std::string* spectralType = starData->getString("SpectralType");
    if (!header.isStar)
    {
        if (spectralType != nullptr)
            stcWarn(header, _("ignoring SpectralType on Barycenter"));
        newDetails = StarDetails::GetBarycenterDetails();
    }
    else if (spectralType != nullptr)
    {
        newDetails = StarDetails::GetStarDetails(StellarClass::parse(*spectralType));
        if (newDetails == nullptr)
        {
            stcError(header, _("invalid SpectralType"));
            return false;
        }
    }
    else if (header.disposition != DataDisposition::Modify || star->isBarycenter())
    {
        stcError(header, _("missing SpectralType on Star"));
        return false;
    }

    return true;
}

bool
checkPolarCoordinates(const StarDatabaseBuilder::StcHeader& header,
                      const AssociativeArray* starData,
                      const Star* star,
                      std::optional<Eigen::Vector3f>& position)
{
    constexpr unsigned int has_ra = 1;
    constexpr unsigned int has_dec = 2;
    constexpr unsigned int has_distance = 4;
    constexpr unsigned int has_all = has_ra | has_dec | has_distance;

    unsigned int status = 0;

    auto raValue = starData->getAngle<double>("RA", astro::DEG_PER_HRA, 1.0);
    auto decValue = starData->getAngle<double>("Dec");
    auto distanceValue = starData->getLength<double>("Distance", astro::KM_PER_LY<double>);
    status = (static_cast<unsigned int>(raValue.has_value()) * has_ra)
           | (static_cast<unsigned int>(decValue.has_value()) * has_dec)
           | (static_cast<unsigned int>(distanceValue.has_value()) * has_distance);

    if (status == 0)
        return true;

    if (status == has_all)
    {
        position = astro::equatorialToCelestialCart(*raValue, *decValue, *distanceValue).cast<float>();
        return true;
    }

    if (header.disposition != DataDisposition::Modify)
    {
        stcError(header, _("incomplete set of coordinates RA/Dec/Distance specified"));
        return false;
    }

    // Partial modification of polar coordinates
    assert(star != nullptr);

    // Convert from Celestia's coordinate system
    const Eigen::Vector3f& p = star->getPosition();
    Eigen::Vector3d v = math::XRotation(astro::J2000Obliquity) * Eigen::Vector3f(p.x(), -p.z(), p.y()).cast<double>();
    // Disable Sonar on the below: suggests using value-or which would eagerly-evaluate the replacement value
    double distance = distanceValue.has_value() ? *distanceValue : v.norm(); //NOSONAR
    double ra = raValue.has_value() ? *raValue : (math::radToDeg(std::atan2(v.y(), v.x())) / astro::DEG_PER_HRA); //NOSONAR
    double dec = decValue.has_value() ? *decValue : math::radToDeg(std::asin(std::clamp(v.z() / v.norm(), -1.0, 1.0))); //NOSONAR

    position = astro::equatorialToCelestialCart(ra, dec, distance).cast<float>();
    return true;
}

bool
checkMagnitudes(const StarDatabaseBuilder::StcHeader& header,
                const AssociativeArray* starData,
                const Star* star,
                float distance,
                std::optional<float>& absMagnitude,
                std::optional<float>& extinction)
{
    assert(header.disposition != DataDisposition::Modify || star != nullptr);
    absMagnitude = starData->getNumber<float>("AbsMag");
    auto appMagnitude = starData->getNumber<float>("AppMag");

    if (!header.isStar)
    {
        if (absMagnitude.has_value())
            stcWarn(header, _("AbsMag ignored on Barycenter"));
        if (appMagnitude.has_value())
            stcWarn(header, _("AppMag ignored on Barycenter"));
        absMagnitude = 30.0f;
        return true;
    }

    extinction = starData->getNumber<float>("Extinction");
    if (extinction.has_value() && distance < VALID_APPMAG_DISTANCE_THRESHOLD)
    {
        stcWarn(header, _("Extinction ignored for stars close to the origin"));
        extinction = std::nullopt;
    }

    if (absMagnitude.has_value())
    {
        if (appMagnitude.has_value())
            stcWarn(header, _("AppMag ignored when AbsMag is supplied"));
    }
    else if (appMagnitude.has_value())
    {
        if (distance < VALID_APPMAG_DISTANCE_THRESHOLD)
        {
            stcError(header, _("AppMag cannot be used close to the origin"));
            return false;
        }

        float extinctionValue = 0.0;
        if (extinction.has_value())
            extinctionValue = *extinction;
        else if (header.disposition == DataDisposition::Modify)
            extinctionValue = star->getExtinction() * distance;

        absMagnitude = astro::appToAbsMag(*appMagnitude, distance) - extinctionValue;
    }
    else if (header.disposition != DataDisposition::Modify || star->isBarycenter())
    {
        stcError(header, _("no magnitude defined for star"));
        return false;
    }

    return true;
}

void
mergeStarDetails(boost::intrusive_ptr<StarDetails>& existingDetails,
                 const boost::intrusive_ptr<StarDetails>& referenceDetails)
{
    if (referenceDetails == nullptr)
        return;

    if (existingDetails->shared())
    {
        // If there are no extended information values set, just
        // use the new reference details object
        existingDetails = referenceDetails;
    }
    else
    {
        // There are custom details: copy the new data into the
        // existing record
        existingDetails->mergeFromStandard(referenceDetails.get());
    }
}

void
applyTemperatureBoloCorrection(const StarDatabaseBuilder::StcHeader& header,
                               const AssociativeArray* starData,
                               boost::intrusive_ptr<StarDetails>& details)
{
    auto bolometricCorrection = starData->getNumber<float>("BoloCorrection");
    if (bolometricCorrection.has_value())
    {
        if (!header.isStar)
            stcWarn(header, _("BoloCorrection is ignored on Barycenters"));
        else
            StarDetails::setBolometricCorrection(details, *bolometricCorrection);
    }

    if (auto temperature = starData->getNumber<float>("Temperature"); temperature.has_value())
    {
        if (!header.isStar)
        {
            stcWarn(header, _("Temperature is ignored on Barycenters"));
        }
        else if (*temperature > 0.0)
        {
            StarDetails::setTemperature(details, *temperature);
            if (!bolometricCorrection.has_value())
            {
                // if we change the temperature, recalculate the bolometric
                // correction using formula from formula for main sequence
                // stars given in B. Cameron Reed (1998), "The Composite
                // Observational-Theoretical HR Diagram", Journal of the Royal
                // Astronomical Society of Canada, Vol 92. p36.

                double logT = std::log10(static_cast<double>(*temperature)) - 4.0;
                double bc = -8.499 * std::pow(logT, 4) + 13.421 * std::pow(logT, 3)
                            - 8.131 * logT * logT - 3.901 * logT - 0.438;

                StarDetails::setBolometricCorrection(details, static_cast<float>(bc));
            }
        }
        else
        {
            stcWarn(header, _("Temperature value must be greater than zero"));
        }
    }
}

void
applyCustomDetails(const StarDatabaseBuilder::StcHeader& header,
                   const AssociativeArray* starData,
                   boost::intrusive_ptr<StarDetails>& details)
{
    if (const auto* mesh = starData->getString("Mesh"); mesh != nullptr)
    {
        if (!header.isStar)
        {
            stcWarn(header, _("Mesh is ignored on Barycenters"));
        }
        else if (auto meshPath = util::U8FileName(*mesh); meshPath.has_value())
        {
            using engine::GeometryInfo;
            using engine::GetGeometryManager;
            ResourceHandle geometryHandle = GetGeometryManager()->getHandle(GeometryInfo(*meshPath,
                                                                                         *header.path,
                                                                                         Eigen::Vector3f::Zero(),
                                                                                         1.0f,
                                                                                         true));
            StarDetails::setGeometry(details, geometryHandle);
        }
        else
        {
            stcError(header, _("invalid filename in Mesh"));
        }
    }

    if (const auto* texture = starData->getString("Texture"); texture != nullptr)
    {
        if (!header.isStar)
            stcWarn(header, _("Texture is ignored on Barycenters"));
        else if (auto texturePath = util::U8FileName(*texture); texturePath.has_value())
            StarDetails::setTexture(details, MultiResTexture(*texturePath, *header.path));
        else
            stcError(header, _("invalid filename in Texture"));
    }

    if (auto rotationModel = CreateRotationModel(starData, *header.path, 1.0); rotationModel != nullptr)
    {
        if (!header.isStar)
            stcWarn(header, _("Rotation is ignored on Barycenters"));
        else
            StarDetails::setRotationModel(details, rotationModel);
    }

    if (auto semiAxes = starData->getLengthVector<float>("SemiAxes"); semiAxes.has_value())
    {
        if (!header.isStar)
            stcWarn(header, _("SemiAxes is ignored on Barycenters"));
        else if (semiAxes->minCoeff() >= 0.0)
            StarDetails::setEllipsoidSemiAxes(details, *semiAxes);
        else
            stcWarn(header, _("SemiAxes must be greater than zero"));
    }

    if (auto radius = starData->getLength<float>("Radius"); radius.has_value())
    {
        if (!header.isStar)
            stcWarn(header, _("Radius is ignored on Barycenters"));
        else if (*radius >= 0.0)
            StarDetails::setRadius(details, *radius);
        else
            stcWarn(header, _("Radius must be greater than zero"));
    }

    applyTemperatureBoloCorrection(header, starData, details);

    if (const auto* infoUrl = starData->getString("InfoURL"); infoUrl != nullptr)
        StarDetails::setInfoURL(details, *infoUrl);
}

} // end unnamed namespace

StarDatabaseBuilder::~StarDatabaseBuilder() = default;

bool
StarDatabaseBuilder::loadBinary(std::istream& in)
{
    Timer timer;
    std::uint32_t nStarsInFile;
    if (!parseStarsDatHeader(in, nStarsInFile))
        return false;

    constexpr std::uint32_t BUFFER_RECORDS = UINT32_C(4096) / sizeof(StarsDatRecord);
    std::vector<char> buffer(sizeof(StarsDatRecord) * BUFFER_RECORDS);
    std::uint32_t nStarsRemaining = nStarsInFile;
    while (nStarsRemaining > 0)
    {
        std::uint32_t recordsToRead = std::min(BUFFER_RECORDS, nStarsRemaining);
        if (!in.read(buffer.data(), sizeof(StarsDatRecord) * recordsToRead).good()) /* Flawfinder: ignore */
            return false;

        const char* ptr = buffer.data();
        for (std::uint32_t i = 0; i < recordsToRead; ++i)
        {
            auto catNo = util::fromMemoryLE<AstroCatalog::IndexNumber>(ptr + offsetof(StarsDatRecord, catNo));
            Eigen::Vector3f position(util::fromMemoryLE<float>(ptr + offsetof(StarsDatRecord, x)),
                                     util::fromMemoryLE<float>(ptr + offsetof(StarsDatRecord, y)),
                                     util::fromMemoryLE<float>(ptr + offsetof(StarsDatRecord, z)));
            auto absMag = util::fromMemoryLE<std::int16_t>(ptr + offsetof(StarsDatRecord, absMag));
            auto spectralType = util::fromMemoryLE<std::uint16_t>(ptr + offsetof(StarsDatRecord, spectralType));

            boost::intrusive_ptr<StarDetails> details = nullptr;
            if (StellarClass sc; sc.unpackV1(spectralType))
                details = StarDetails::GetStarDetails(sc);

            if (details == nullptr)
            {
                GetLogger()->error(_("Bad spectral type in star database, star #{}\n"), catNo);
                continue;
            }

            Star& star = unsortedStars.emplace_back(catNo, details);
            star.setPosition(position);
            star.setAbsoluteMagnitude(static_cast<float>(absMag) / 256.0f);

            ptr += sizeof(StarsDatRecord);
        }

        nStarsRemaining -= recordsToRead;
    }

    if (in.bad())
        return false;

    auto loadTime = timer.getTime();

    GetLogger()->debug("StarDatabase::read: nStars = {}, time = {} ms\n", nStarsInFile, loadTime);
    GetLogger()->info(_("{} stars in binary database\n"), unsortedStars.size());

    // Create the temporary list of stars sorted by catalog number; this
    // will be used to lookup stars during file loading. After loading is
    // complete, the stars are sorted into an octree and this list gets
    // replaced.
    binFileCatalogNumberIndex.reserve(unsortedStars.size());
    for (Star& star : unsortedStars)
        binFileCatalogNumberIndex.push_back(&star);

    std::sort(binFileCatalogNumberIndex.begin(), binFileCatalogNumberIndex.end(),
                [](const Star* star0, const Star* star1) { return star0->getIndex() < star1->getIndex(); });

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
bool
StarDatabaseBuilder::load(std::istream& in, const fs::path& resourcePath)
{
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

#ifdef ENABLE_NLS
    std::string domain = resourcePath.string();
    const char *d = domain.c_str();
    bindtextdomain(d, d); // domain name is the same as resource path
#else
    std::string domain;
#endif

    StcHeader header(resourcePath);
    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        if (!parseStcHeader(tokenizer, header))
            return false;

        // now goes the star definition
        tokenizer.pushBack();
        const Value starDataValue = parser.readValue();
        const Hash* starData = starDataValue.getHash();
        if (starData == nullptr)
        {
            GetLogger()->error(_("Bad star definition at line {}.\n"), tokenizer.getLineNumber());
            return false;
        }

        if (header.disposition != DataDisposition::Add && header.catalogNumber == AstroCatalog::InvalidIndex)
            header.catalogNumber = starDB->namesDB->findCatalogNumberByName(header.names.front(), false);

        Star* star = findWhileLoading(header.catalogNumber);
        if (star == nullptr)
        {
            if (header.disposition == DataDisposition::Modify)
            {
                GetLogger()->error(_("Modify requested for nonexistent star.\n"));
                continue;
            }

            if (header.catalogNumber == AstroCatalog::InvalidIndex)
            {
                header.catalogNumber = nextAutoCatalogNumber;
                --nextAutoCatalogNumber;
            }
        }

        if (createOrUpdateStar(header, starData, star))
        {
            loadCategories(header, starData, domain);

            if (!header.names.empty())
            {
                starDB->namesDB->erase(header.catalogNumber);
                for (const auto& name : header.names)
                    starDB->namesDB->add(header.catalogNumber, name);
            }
        }
    }

    return true;
}

void
StarDatabaseBuilder::setNameDatabase(std::unique_ptr<StarNameDatabase>&& nameDB)
{
    starDB->namesDB = std::move(nameDB);
}

std::unique_ptr<StarDatabase>
StarDatabaseBuilder::finish()
{
    GetLogger()->info(_("Total star count: {}\n"), unsortedStars.size());

    buildOctree();
    buildIndexes();

    // Resolve all barycenters; this can't be done before star sorting. There's
    // still a bug here: final orbital radii aren't available until after
    // the barycenters have been resolved, and these are required when building
    // the octree.  This will only rarely cause a problem, but it still needs
    // to be addressed.
    for (const auto [starIdx, barycenterIdx] : barycenters)
    {
        Star* star = starDB->find(starIdx);
        Star* barycenter = starDB->find(barycenterIdx);
        assert(star != nullptr);
        assert(barycenter != nullptr);
        if (star != nullptr && barycenter != nullptr)
        {
            StarDetails::setOrbitBarycenter(star->details, barycenter);
            StarDetails::addOrbitingStar(barycenter->details, star);
        }
    }

    for (const auto& [catalogNumber, category] : categories)
    {
        Star* star = starDB->find(catalogNumber);
        UserCategory::addObject(star, category);
    }

    return std::move(starDB);
}

/*! Load star data from a property list into a star instance.
 */
bool
StarDatabaseBuilder::createOrUpdateStar(const StcHeader& header,
                                        const AssociativeArray* starData,
                                        Star* star)
{
    boost::intrusive_ptr<StarDetails> newDetails = nullptr;
    if (!checkSpectralType(header, starData, star, newDetails))
        return false;

    std::optional<Eigen::Vector3f> position = std::nullopt;
    std::optional<AstroCatalog::IndexNumber> barycenterNumber = std::nullopt;
    std::shared_ptr<const ephem::Orbit> orbit = nullptr;
    if (!checkStcPosition(header, starData, star, position, barycenterNumber, orbit))
        return false;

    std::optional<float> absMagnitude = std::nullopt;
    std::optional<float> extinction = std::nullopt;
    float distance;
    if (position.has_value())
    {
        distance = position->norm();
    }
    else
    {
        assert(star != nullptr);
        distance = star->getPosition().norm();
    }

    if (!checkMagnitudes(header, starData, star, distance, absMagnitude, extinction))
        return false;

    if (star == nullptr)
    {
        assert(newDetails != nullptr);
        star = &unsortedStars.emplace_back(header.catalogNumber, newDetails);
        stcFileCatalogNumberIndex[header.catalogNumber] = star;
    }
    else if (header.disposition == DataDisposition::Modify)
    {
        mergeStarDetails(star->details, newDetails);
    }
    else
    {
        assert(newDetails != nullptr);
        star->details = newDetails;
    }

    if (position.has_value())
        star->setPosition(*position);

    if (absMagnitude.has_value())
        star->setAbsoluteMagnitude(*absMagnitude);

    if (extinction.has_value())
        star->setExtinction(*extinction / distance);

    if (barycenterNumber == AstroCatalog::InvalidIndex)
        barycenters.erase(header.catalogNumber);
    else if (barycenterNumber.has_value())
        barycenters[header.catalogNumber] = *barycenterNumber;

    if (orbit != nullptr)
        StarDetails::setOrbit(star->details, orbit);

    applyCustomDetails(header, starData, star->details);
    return true;
}

bool
StarDatabaseBuilder::checkStcPosition(const StarDatabaseBuilder::StcHeader& header,
                                      const AssociativeArray* starData,
                                      const Star* star,
                                      std::optional<Eigen::Vector3f>& position,
                                      std::optional<AstroCatalog::IndexNumber>& barycenterNumber,
                                      std::shared_ptr<const ephem::Orbit>& orbit) const
{
    position = std::nullopt;
    barycenterNumber = std::nullopt;

    if (!checkPolarCoordinates(header, starData, star, position))
        return false;

    if (auto positionValue = starData->getLengthVector<float>("Position", astro::KM_PER_LY<double>);
        positionValue.has_value())
    {
        if (position.has_value())
            stcWarn(header, _("ignoring RA/Dec/Distance in favor of Position"));
        position = *positionValue;
    }

    if (!checkBarycenter(header, starData, position, barycenterNumber))
        return false;

    // we consider a star to have a barycenter if it has an OrbitBarycenter defined
    // or the star is modified without overriding its position, and it has no other
    // position overrides.
    bool hasBarycenter = (barycenterNumber.has_value() && *barycenterNumber != AstroCatalog::InvalidIndex)
                         || (header.disposition == DataDisposition::Modify
                             && !position.has_value()
                             && barycenters.find(header.catalogNumber) != barycenters.end());

    if (auto newOrbit = CreateOrbit(Selection(), starData, *header.path, true); newOrbit != nullptr)
    {
        if (hasBarycenter)
            orbit = std::move(newOrbit);
        else
            stcWarn(header, _("ignoring orbit for object without OrbitBarycenter"));
    }
    else if (hasBarycenter && star != nullptr && star->getOrbit() == nullptr)
    {
        stcError(header, _("no orbit specified for star with OrbitBarycenter"));
        return false;
    }

    return true;
}

bool
StarDatabaseBuilder::checkBarycenter(const StarDatabaseBuilder::StcHeader& header,
                                     const AssociativeArray* starData,
                                     std::optional<Eigen::Vector3f>& position,
                                     std::optional<AstroCatalog::IndexNumber>& barycenterNumber) const
{
    // If we override RA/Dec/Position, remove the barycenter
    if (position.has_value())
        barycenterNumber = AstroCatalog::InvalidIndex;

    const Value* orbitBarycenterValue = starData->getValue("OrbitBarycenter");
    if (orbitBarycenterValue == nullptr)
        return true;

    if (auto bcNumber = orbitBarycenterValue->getNumber(); bcNumber.has_value())
    {
        barycenterNumber = static_cast<AstroCatalog::IndexNumber>(*bcNumber);
    }
    else if (auto bcName = orbitBarycenterValue->getString(); bcName != nullptr)
    {
        barycenterNumber = starDB->namesDB->findCatalogNumberByName(*bcName, false);
    }
    else
    {
        stcError(header, _("OrbitBarycenter should be either a string or an integer"));
        return false;
    }

    if (*barycenterNumber == header.catalogNumber)
    {
        stcError(header, _("OrbitBarycenter cycle detected"));
        return false;
    }

    if (const Star* barycenter = findWhileLoading(*barycenterNumber); barycenter != nullptr)
    {
        if (position.has_value())
            stcWarn(header, "ignoring stellar coordinates in favor of OrbitBarycenter");
        position = barycenter->getPosition();
    }
    else
    {
        stcError(header, _("OrbitBarycenter refers to nonexistent star"));
        return false;
    }

    for (auto it = barycenters.find(*barycenterNumber); it != barycenters.end(); it = barycenters.find(it->second))
    {
        if (it->second == header.catalogNumber)
        {
            stcError(header, _("OrbitBarycenter cycle detected"));
            return false;
        }
    }

    return true;
}

void
StarDatabaseBuilder::loadCategories(const StcHeader& header,
                                    const AssociativeArray* starData,
                                    const std::string& domain)
{
    if (header.disposition == DataDisposition::Replace)
        categories.erase(header.catalogNumber);

    const Value* categoryValue = starData->getValue("Category");
    if (categoryValue == nullptr)
        return;

    if (const std::string* categoryName = categoryValue->getString(); categoryName != nullptr)
    {
        if (categoryName->empty())
            return;

        addCategory(header.catalogNumber, *categoryName, domain);
        return;
    }

    const ValueArray *arr = categoryValue->getArray();
    if (arr == nullptr)
        return;

    for (const auto& it : *arr)
    {
        const std::string* categoryName = it.getString();
        if (categoryName == nullptr || categoryName->empty())
            continue;

        addCategory(header.catalogNumber, *categoryName, domain);
    }
}

void
StarDatabaseBuilder::addCategory(AstroCatalog::IndexNumber catalogNumber,
                                 const std::string& name,
                                 const std::string& domain)
{
    auto category = UserCategory::findOrAdd(name, domain);
    if (category == UserCategoryId::Invalid)
        return;

    auto [start, end] = categories.equal_range(catalogNumber);
    if (start == end)
    {
        categories.emplace(catalogNumber, category);
        return;
    }

    if (std::any_of(start, end, [category](const auto& it) { return it.second == category; }))
        return;

    categories.emplace_hint(end, catalogNumber, category);
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
Star*
StarDatabaseBuilder::findWhileLoading(AstroCatalog::IndexNumber catalogNumber) const
{
    if (catalogNumber == AstroCatalog::InvalidIndex)
        return nullptr;

    // First check for stars loaded from the binary database
    if (auto it = std::lower_bound(binFileCatalogNumberIndex.cbegin(), binFileCatalogNumberIndex.cend(),
                                   catalogNumber,
                                   [](const Star* star, AstroCatalog::IndexNumber catNum) { return star->getIndex() < catNum; });
        it != binFileCatalogNumberIndex.cend() && (*it)->getIndex() == catalogNumber)
    {
        return *it;
    }

    // Next check for stars loaded from an stc file
    if (auto it = stcFileCatalogNumberIndex.find(catalogNumber); it != stcFileCatalogNumberIndex.end())
        return it->second;

    // Star not found
    return nullptr;
}

void
StarDatabaseBuilder::buildOctree()
{
    // This should only be called once for the database
    GetLogger()->debug("Sorting stars into octree . . .\n");
    auto starCount = static_cast<engine::OctreeObjectIndex>(unsortedStars.size());

    float absMag = astro::appToAbsMag(STAR_OCTREE_MAGNITUDE,
                                      StarDatabase::STAR_OCTREE_ROOT_SIZE * celestia::numbers::sqrt3_v<float>);

    auto root = engine::makeDynamicOctree<StarOctreeTraits>(std::move(unsortedStars),
                                                            Eigen::Vector3f(1000.0f, 1000.0f, 1000.0f),
                                                            StarDatabase::STAR_OCTREE_ROOT_SIZE,
                                                            absMag,
                                                            StarOctreeSplitThreshold);

    GetLogger()->debug("Spatially sorting stars for improved locality of reference . . .\n");
    starDB->octreeRoot = root->build();

    GetLogger()->debug("{} stars total\nOctree has {} nodes and {} stars.\n",
                       starCount,
                       starDB->octreeRoot->nodeCount(),
                       starDB->octreeRoot->size());

    unsortedStars.clear();
}

void
StarDatabaseBuilder::buildIndexes()
{
    // This should only be called once for the database
    // assert(catalogNumberIndexes[0] == nullptr);

    GetLogger()->info("Building catalog number indexes . . .\n");

    auto nStars = starDB->octreeRoot->size();

    starDB->catalogNumberIndex.clear();
    starDB->catalogNumberIndex.reserve(nStars);
    for (std::uint32_t i = 0; i < nStars; ++i)
        starDB->catalogNumberIndex.push_back(i);

    const auto& octreeRoot = *starDB->octreeRoot;
    std::sort(starDB->catalogNumberIndex.begin(), starDB->catalogNumberIndex.end(),
              [&octreeRoot](std::uint32_t idx0, std::uint32_t idx1)
              {
                  return octreeRoot[idx0].getIndex() < octreeRoot[idx1].getIndex();
              });
}
