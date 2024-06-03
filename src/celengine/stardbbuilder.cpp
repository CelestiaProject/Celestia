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
#include <string_view>
#include <type_traits>
#include <utility>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <Eigen/Geometry>

#include <celastro/astro.h>
#include <celmath/mathlib.h>
#include <celutil/binaryread.h>
#include <celutil/fsutils.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/timer.h>
#include <celutil/tokenizer.h>
#include "hash.h"
#include "meshmanager.h"
#include "parser.h"
#include "stardb.h"
#include "stellarclass.h"
#include "value.h"

using namespace std::string_view_literals;

namespace astro = celestia::astro;
namespace engine = celestia::engine;
namespace math = celestia::math;
namespace util = celestia::util;

using util::GetLogger;

// Enable the below to switch back to parsing coordinates as float to match
// legacy behaviour. This shouldn't be necessary since stars.dat stores
// Cartesian coordinates.
// #define PARSE_COORDS_FLOAT

struct StarDatabaseBuilder::CustomStarDetails
{
    bool hasCustomDetails{false};
    fs::path modelName;
    fs::path textureName;
    std::shared_ptr<const celestia::ephem::Orbit> orbit;
    std::shared_ptr<const celestia::ephem::RotationModel> rm;
    std::optional<Eigen::Vector3d> semiAxes{std::nullopt};
    std::optional<float> radius{std::nullopt};
    double temperature{0.0};
    std::optional<float> bolometricCorrection{std::nullopt};
    const std::string* infoURL{nullptr};
};

namespace
{

constexpr float STAR_OCTREE_MAGNITUDE = 6.0f;

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

void
stcError(const Tokenizer& tok, std::string_view msg)
{
    GetLogger()->error(_("Error in .stc file (line {}): {}\n"), tok.getLineNumber(), msg);
}

void
modifyStarDetails(Star* star,
                  boost::intrusive_ptr<StarDetails>&& referenceDetails,
                  bool hasCustomDetails)
{
    StarDetails* existingDetails = star->getDetails();
    assert(existingDetails != nullptr);

    if (existingDetails->shared())
    {
        // If the star definition has extended information, clone the
        // star details so we can customize it without affecting other
        // stars of the same spectral type.
        if (hasCustomDetails)
            star->setDetails(referenceDetails == nullptr ? existingDetails->clone() : referenceDetails->clone());
        else if (referenceDetails != nullptr)
            star->setDetails(std::move(referenceDetails));
    }
    else if (referenceDetails != nullptr)
    {
        // If the spectral type was modified, copy the new data
        // to the custom details record.
        existingDetails->setSpectralType(referenceDetails->getSpectralType());
        existingDetails->setTemperature(referenceDetails->getTemperature());
        existingDetails->setBolometricCorrection(referenceDetails->getBolometricCorrection());
        if ((existingDetails->getKnowledge() & StarDetails::KnowTexture) == 0)
            existingDetails->setTexture(referenceDetails->getTexture());
        if ((existingDetails->getKnowledge() & StarDetails::KnowRotation) == 0)
            existingDetails->setRotationModel(referenceDetails->getRotationModel());
        existingDetails->setVisibility(referenceDetails->getVisibility());
    }
}

StarDatabaseBuilder::CustomStarDetails
parseCustomStarDetails(const Hash* starData,
                       const fs::path& path)
{
    StarDatabaseBuilder::CustomStarDetails customDetails;

    if (const std::string* mesh = starData->getString("Mesh"); mesh != nullptr)
    {
        if (auto meshPath = util::U8FileName(*mesh); meshPath.has_value())
            customDetails.modelName = std::move(*meshPath);
        else
            GetLogger()->error("Invalid filename in Mesh\n");
    }

    if (const std::string* texture = starData->getString("Texture"); texture != nullptr)
    {
        if (auto texturePath = util::U8FileName(*texture); texturePath.has_value())
            customDetails.textureName = std::move(*texturePath);
        else
            GetLogger()->error("Invalid filename in Texture\n");
    }

    customDetails.orbit = CreateOrbit(Selection(), starData, path, true);
    customDetails.rm = CreateRotationModel(starData, path, 1.0);
    customDetails.semiAxes = starData->getLengthVector<double>("SemiAxes");
    customDetails.radius = starData->getLength<float>("Radius");
    customDetails.temperature = starData->getNumber<double>("Temperature").value_or(0.0);
    customDetails.bolometricCorrection = starData->getNumber<float>("BoloCorrection");
    customDetails.infoURL = starData->getString("InfoURL");

    customDetails.hasCustomDetails = !customDetails.modelName.empty() ||
                                     !customDetails.textureName.empty() ||
                                     customDetails.orbit != nullptr ||
                                     customDetails.rm != nullptr ||
                                     customDetails.semiAxes.has_value() ||
                                     customDetails.radius.has_value() ||
                                     customDetails.temperature > 0.0 ||
                                     customDetails.bolometricCorrection.has_value() ||
                                     customDetails.infoURL != nullptr;

    return customDetails;
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
            auto x = util::fromMemoryLE<float>(ptr + offsetof(StarsDatRecord, x));
            auto y = util::fromMemoryLE<float>(ptr + offsetof(StarsDatRecord, y));
            auto z = util::fromMemoryLE<float>(ptr + offsetof(StarsDatRecord, z));
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

            Star& star = unsortedStars.emplace_back();
            star.setPosition(x, y, z);
            star.setAbsoluteMagnitude(static_cast<float>(absMag) / 256.0f);
            star.setDetails(std::move(details));
            star.setIndex(catNo);

            ptr += sizeof(StarsDatRecord);
            ++starDB->nStars;
        }

        nStarsRemaining -= recordsToRead;
    }

    if (in.bad())
        return false;

    auto loadTime = timer.getTime();

    GetLogger()->debug("StarDatabase::read: nStars = {}, time = {} ms\n", nStarsInFile, loadTime);
    GetLogger()->info(_("{} stars in binary database\n"), starDB->nStars);

    // Create the temporary list of stars sorted by catalog number; this
    // will be used to lookup stars during file loading. After loading is
    // complete, the stars are sorted into an octree and this list gets
    // replaced.
    if (auto binFileStarCount = unsortedStars.size(); binFileStarCount > 0)
    {
        binFileCatalogNumberIndex.reserve(binFileStarCount);
        for (Star& star : unsortedStars)
            binFileCatalogNumberIndex.push_back(&star);

        std::sort(binFileCatalogNumberIndex.begin(), binFileCatalogNumberIndex.end(),
                  [](const Star* star0, const Star* star1) { return star0->getIndex() < star1->getIndex(); });
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

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        bool isStar = true;

        // Parse the disposition--either Add, Replace, or Modify. The disposition
        // may be omitted. The default value is Add.
        DataDisposition disposition = DataDisposition::Add;
        if (auto tokenValue = tokenizer.getNameValue(); tokenValue.has_value())
        {
            if (*tokenValue == "Modify")
            {
                disposition = DataDisposition::Modify;
                tokenizer.nextToken();
            }
            else if (*tokenValue == "Replace")
            {
                disposition = DataDisposition::Replace;
                tokenizer.nextToken();
            }
            else if (*tokenValue == "Add")
            {
                disposition = DataDisposition::Add;
                tokenizer.nextToken();
            }
        }

        // Parse the object type--either Star or Barycenter. The object type
        // may be omitted. The default is Star.
        if (auto tokenValue = tokenizer.getNameValue(); tokenValue.has_value())
        {
            if (*tokenValue == "Star")
            {
                isStar = true;
            }
            else if (*tokenValue == "Barycenter")
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
        if (auto tokenValue = tokenizer.getNumberValue(); tokenValue.has_value())
        {
            catalogNumber = static_cast<AstroCatalog::IndexNumber>(*tokenValue);
            tokenizer.nextToken();
        }

        std::string objName;
        std::string firstName;
        if (auto tokenValue = tokenizer.getStringValue(); tokenValue.has_value())
        {
            // A star name (or names) is present
            objName = *tokenValue;
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
            if (catalogNumber == AstroCatalog::InvalidIndex && !firstName.empty())
                catalogNumber = starDB->namesDB->findCatalogNumberByName(firstName, false);

            if (catalogNumber == AstroCatalog::InvalidIndex)
                catalogNumber = nextAutoCatalogNumber--;
            else
                star = findWhileLoading(catalogNumber);
            break;

        case DataDisposition::Modify:
            // If no catalog number was specified, try looking up the star by name
            if (catalogNumber == AstroCatalog::InvalidIndex && !firstName.empty())
                catalogNumber = starDB->namesDB->findCatalogNumberByName(firstName, false);

            if (catalogNumber != AstroCatalog::InvalidIndex)
                star = findWhileLoading(catalogNumber);

            break;
        }

        bool isNewStar = star == nullptr;

        tokenizer.pushBack();

        const Value starDataValue = parser.readValue();
        const Hash* starData = starDataValue.getHash();
        if (starData == nullptr)
        {
            GetLogger()->error("Bad star definition at line {}.\n", tokenizer.getLineNumber());
            return false;
        }

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
            loadCategories(catalogNumber, starData, disposition, domain);
        }

        if (ok)
        {
            if (isNewStar)
            {
                unsortedStars.push_back(*star);
                ++starDB->nStars;
                delete star;

                // Add the new star to the temporary (load time) index.
                stcFileCatalogNumberIndex[catalogNumber] = &unsortedStars[unsortedStars.size() - 1];
            }

            if (starDB->namesDB != nullptr && !objName.empty())
            {
                // List of namesDB will replace any that already exist for
                // this star.
                starDB->namesDB->erase(catalogNumber);

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
                    starDB->namesDB->add(catalogNumber, starName);
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

void
StarDatabaseBuilder::setNameDatabase(std::unique_ptr<StarNameDatabase>&& nameDB)
{
    starDB->namesDB = std::move(nameDB);
}


std::unique_ptr<StarDatabase>
StarDatabaseBuilder::finish()
{
    GetLogger()->info(_("Total star count: {}\n"), starDB->nStars);

    buildOctree();
    buildIndexes();

    // Resolve all barycenters; this can't be done before star sorting. There's
    // still a bug here: final orbital radii aren't available until after
    // the barycenters have been resolved, and these are required when building
    // the octree.  This will only rarely cause a problem, but it still needs
    // to be addressed.
    for (const auto& b : barycenters)
    {
        Star* star = starDB->find(b.catNo);
        Star* barycenter = starDB->find(b.barycenterCatNo);
        assert(star != nullptr);
        assert(barycenter != nullptr);
        if (star != nullptr && barycenter != nullptr)
        {
            star->setOrbitBarycenter(barycenter);
            barycenter->addOrbitingStar(star);
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
StarDatabaseBuilder::createStar(Star* star,
                                DataDisposition disposition,
                                AstroCatalog::IndexNumber catalogNumber,
                                const Hash* starData,
                                const fs::path& path,
                                bool isBarycenter)
{
    std::optional<Eigen::Vector3f> barycenterPosition = std::nullopt;
    if (!createOrUpdateStarDetails(star,
                                   disposition,
                                   catalogNumber,
                                   starData,
                                   path,
                                   isBarycenter,
                                   barycenterPosition))
        return false;

    if (disposition != DataDisposition::Modify)
        star->setIndex(catalogNumber);

    // Compute the position in rectangular coordinates.  If a star has an
    // orbit and barycenter, its position is the position of the barycenter.
    if (barycenterPosition.has_value())
        star->setPosition(*barycenterPosition);
    else if (auto rectangularPos = starData->getLengthVector<float>("Position", astro::KM_PER_LY<double>); rectangularPos.has_value())
    {
        // "Position" allows the position of the star to be specified in
        // coordinates matching those used in stars.dat, allowing an exact
        // translation of stars.dat entries to .stc.
        star->setPosition(*rectangularPos);
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
                ra = math::radToDeg(std::atan2(v.y(), v.x())) / astro::DEG_PER_HRA;
                dec = math::radToDeg(std::asin(v.z()));
            }
        }

        bool modifyPosition = false;
        if (auto raValue = starData->getAngle<double>("RA", astro::DEG_PER_HRA, 1.0); raValue.has_value())
        {
            ra = *raValue;
            modifyPosition = true;
        }
        else if (disposition != DataDisposition::Modify)
        {
            GetLogger()->error(_("Invalid star: missing right ascension\n"));
            return false;
        }

        if (auto decValue = starData->getAngle<double>("Dec"); decValue.has_value())
        {
            dec = *decValue;
            modifyPosition = true;
        }
        else if (disposition != DataDisposition::Modify)
        {
            GetLogger()->error(_("Invalid star: missing declination.\n"));
            return false;
        }

        if (auto dist = starData->getLength<double>("Distance", astro::KM_PER_LY<double>); dist.has_value())
        {
            distance = *dist;
            modifyPosition = true;
        }
        else if (disposition != DataDisposition::Modify)
        {
            GetLogger()->error(_("Invalid star: missing distance.\n"));
            return false;
        }

        if (modifyPosition)
        {
#ifdef PARSE_COORDS_FLOAT
            // Truncate to floats to match behavior of reading from binary file.
            // (No longer applies since binary file stores Cartesians)
            // The conversion to rectangular coordinates is still performed at
            // double precision, however.
            Eigen::Vector3d pos = astro::equatorialToCelestialCart(static_cast<double>(static_cast<float>(ra)),
                                                                   static_cast<double>(static_cast<float>(dec)),
                                                                   static_cast<double>(static_cast<float>(distance)));
#else
            Eigen::Vector3d pos = astro::equatorialToCelestialCart(ra, dec, distance);
#endif
            star->setPosition(pos.cast<float>());
        }
    }

    if (isBarycenter)
    {
        star->setAbsoluteMagnitude(30.0f);
    }
    else
    {
        bool absoluteDefined = true;
        std::optional<float> magnitude = starData->getNumber<float>("AbsMag");
        if (!magnitude.has_value())
        {
            absoluteDefined = false;
            if (auto appMag = starData->getNumber<float>("AppMag"); appMag.has_value())
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
                magnitude = astro::appToAbsMag(*appMag, distance);
            }
            else if (disposition != DataDisposition::Modify)
            {
                GetLogger()->error(_("Invalid star: missing magnitude.\n"));
                return false;
            }
        }

        if (magnitude.has_value())
            star->setAbsoluteMagnitude(*magnitude);

        if (auto extinction = starData->getNumber<float>("Extinction"); extinction.has_value())
        {
            if (float distance = star->getPosition().norm(); distance != 0.0f)
                star->setExtinction(*extinction / distance);
            else
                extinction = 0.0f;

            if (!absoluteDefined)
                star->setAbsoluteMagnitude(star->getAbsoluteMagnitude() - *extinction);
        }
    }

    return true;
}

bool
StarDatabaseBuilder::createOrUpdateStarDetails(Star* star,
                                               DataDisposition disposition,
                                               AstroCatalog::IndexNumber catalogNumber,
                                               const Hash* starData,
                                               const fs::path& path,
                                               const bool isBarycenter,
                                               std::optional<Eigen::Vector3f>& barycenterPosition)
{
    barycenterPosition = std::nullopt;
    boost::intrusive_ptr<StarDetails> referenceDetails;

    // Get the magnitude and spectral type; if the star is actually
    // a barycenter placeholder, these fields are ignored.
    if (isBarycenter)
    {
        referenceDetails = StarDetails::GetBarycenterDetails();
    }
    else
    {
        const std::string* spectralType = starData->getString("SpectralType");
        if (spectralType != nullptr)
        {
            StellarClass sc = StellarClass::parse(*spectralType);
            referenceDetails = StarDetails::GetStarDetails(sc);
            if (referenceDetails == nullptr)
            {
                GetLogger()->error(_("Invalid star: bad spectral type.\n"));
                return false;
            }
        }
        else if (disposition != DataDisposition::Modify)
        {
            // Spectral type is required for new stars
            GetLogger()->error(_("Invalid star: missing spectral type.\n"));
            return false;
        }
    }

    CustomStarDetails customDetails = parseCustomStarDetails(starData, path);
    barycenterPosition = std::nullopt;

    if (disposition == DataDisposition::Modify)
        modifyStarDetails(star, std::move(referenceDetails), customDetails.hasCustomDetails);
    else
        star->setDetails(customDetails.hasCustomDetails ? referenceDetails->clone() : referenceDetails);

    return applyCustomStarDetails(star,
                                  catalogNumber,
                                  starData,
                                  path,
                                  customDetails,
                                  barycenterPosition);
}

bool
StarDatabaseBuilder::applyCustomStarDetails(const Star* star,
                                            AstroCatalog::IndexNumber catalogNumber,
                                            const Hash* starData,
                                            const fs::path& path,
                                            const CustomStarDetails& customDetails,
                                            std::optional<Eigen::Vector3f>& barycenterPosition)
{
    if (!customDetails.hasCustomDetails)
        return true;

    StarDetails* details = star->getDetails();
    assert(!details->shared());

    if (!customDetails.textureName.empty())
    {
        details->setTexture(MultiResTexture(customDetails.textureName, path));
        details->addKnowledge(StarDetails::KnowTexture);
    }

    if (!customDetails.modelName.empty())
    {
        using engine::GeometryInfo;
        using engine::GetGeometryManager;
        ResourceHandle geometryHandle = GetGeometryManager()->getHandle(GeometryInfo(customDetails.modelName,
                                                                                     path,
                                                                                     Eigen::Vector3f::Zero(),
                                                                                     1.0f,
                                                                                     true));
        details->setGeometry(geometryHandle);
    }

    if (customDetails.semiAxes.has_value())
        details->setEllipsoidSemiAxes(customDetails.semiAxes->cast<float>());

    if (customDetails.radius.has_value())
    {
        details->setRadius(*customDetails.radius);
        details->addKnowledge(StarDetails::KnowRadius);
    }

    if (customDetails.temperature > 0.0)
    {
        details->setTemperature(static_cast<float>(customDetails.temperature));

        if (!customDetails.bolometricCorrection.has_value())
        {
            // if we change the temperature, recalculate the bolometric
            // correction using formula from formula for main sequence
            // stars given in B. Cameron Reed (1998), "The Composite
            // Observational-Theoretical HR Diagram", Journal of the Royal
            // Astronomical Society of Canada, Vol 92. p36.

            double logT = std::log10(customDetails.temperature) - 4;
            double bc = -8.499 * std::pow(logT, 4) + 13.421 * std::pow(logT, 3)
                        - 8.131 * logT * logT - 3.901 * logT - 0.438;

            details->setBolometricCorrection(static_cast<float>(bc));
        }
    }

    if (customDetails.bolometricCorrection.has_value())
    {
        details->setBolometricCorrection(*customDetails.bolometricCorrection);
    }

    if (customDetails.infoURL != nullptr)
        details->setInfoURL(*customDetails.infoURL);

    if (!applyOrbit(catalogNumber, starData, details, customDetails, barycenterPosition))
        return false;

    if (customDetails.rm != nullptr)
        details->setRotationModel(customDetails.rm);

    return true;
}

bool
StarDatabaseBuilder::applyOrbit(AstroCatalog::IndexNumber catalogNumber,
                                const Hash* starData,
                                StarDetails* details,
                                const CustomStarDetails& customDetails,
                                std::optional<Eigen::Vector3f>& barycenterPosition)
{
    if (customDetails.orbit == nullptr)
        return true;

    details->setOrbit(customDetails.orbit);

    // See if a barycenter was specified as well
    AstroCatalog::IndexNumber barycenterCatNo = AstroCatalog::InvalidIndex;
    bool barycenterDefined = false;

    const std::string* barycenterName = starData->getString("OrbitBarycenter");
    if (barycenterName != nullptr)
    {
        barycenterCatNo   = starDB->namesDB->findCatalogNumberByName(*barycenterName, false);
        barycenterDefined = true;
    }
    else if (auto barycenterNumber = starData->getNumber<AstroCatalog::IndexNumber>("OrbitBarycenter");
                barycenterNumber.has_value())
    {
        barycenterCatNo   = *barycenterNumber;
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
            if (const Star* barycenter = findWhileLoading(barycenterCatNo); barycenter != nullptr)
                barycenterPosition = barycenter->getPosition();
        }

        if (!barycenterPosition.has_value())
        {
            if (barycenterName == nullptr)
                GetLogger()->error(_("Barycenter {} does not exist.\n"), barycenterCatNo);
            else
                GetLogger()->error(_("Barycenter {} does not exist.\n"), *barycenterName);
            return false;
        }
    }

    return true;
}

void
StarDatabaseBuilder::loadCategories(AstroCatalog::IndexNumber catalogNumber,
                                    const Hash *hash,
                                    DataDisposition disposition,
                                    const std::string &domain)
{
    if (disposition == DataDisposition::Replace)
        categories.erase(catalogNumber);

    const Value* categoryValue = hash->getValue("Category");
    if (categoryValue == nullptr)
        return;

    if (const std::string* categoryName = categoryValue->getString(); categoryName != nullptr)
    {
        if (categoryName->empty())
            return;

        addCategory(catalogNumber, *categoryName, domain);
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

        addCategory(catalogNumber, *categoryName, domain);
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
    float absMag = astro::appToAbsMag(STAR_OCTREE_MAGNITUDE,
                                      StarDatabase::STAR_OCTREE_ROOT_SIZE * celestia::numbers::sqrt3_v<float>);
    auto root = std::make_unique<DynamicStarOctree>(Eigen::Vector3f(1000.0f, 1000.0f, 1000.0f),
                                                    absMag);
    for (const Star& star : unsortedStars)
        root->insertObject(star, StarDatabase::STAR_OCTREE_ROOT_SIZE);

    GetLogger()->debug("Spatially sorting stars for improved locality of reference . . .\n");
    auto sortedStars = std::make_unique<Star[]>(starDB->nStars);
    Star* firstStar = sortedStars.get();
    root->rebuildAndSort(starDB->octreeRoot, firstStar);

    // ASSERT((int) (firstStar - sortedStars) == nStars);
    GetLogger()->debug("{} stars total\nOctree has {} nodes and {} stars.\n",
                       firstStar - sortedStars.get(),
                       1 + starDB->octreeRoot->countChildren(), starDB->octreeRoot->countObjects());

    unsortedStars.clear();
    starDB->stars = std::move(sortedStars);
}

void
StarDatabaseBuilder::buildIndexes()
{
    // This should only be called once for the database
    // assert(catalogNumberIndexes[0] == nullptr);

    GetLogger()->info("Building catalog number indexes . . .\n");

    starDB->catalogNumberIndex.clear();
    starDB->catalogNumberIndex.reserve(starDB->nStars);
    for (std::uint32_t i = 0; i < starDB->nStars; ++i)
        starDB->catalogNumberIndex.push_back(i);

    const Star* stars = starDB->stars.get();
    std::sort(starDB->catalogNumberIndex.begin(), starDB->catalogNumberIndex.end(),
              [stars](std::uint32_t idx0, std::uint32_t idx1)
              {
                  return stars[idx0].getIndex() < stars[idx1].getIndex();
              });
}
