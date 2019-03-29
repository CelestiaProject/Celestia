// stardb.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstring>
#include <cmath>
#include <cstdlib>
#include <fmt/printf.h>
#include <cassert>
#include <algorithm>
#include <celmath/mathlib.h>
#include <celutil/util.h>
#include <celutil/bytes.h>
#include <celengine/stardb.h>
#include <config.h>
#include "astro.h"
#include "parser.h"
#include "parseobject.h"
#include "multitexture.h"
#include "meshmanager.h"
#include <celutil/debug.h>

using namespace Eigen;
using namespace std;
using namespace celmath;


constexpr const char HDCatalogPrefix[]        = "HD ";
constexpr const char HIPPARCOSCatalogPrefix[] = "HIP ";
constexpr const char GlieseCatalogPrefix[]    = "Gliese ";
constexpr const char RossCatalogPrefix[]      = "Ross ";
constexpr const char LacailleCatalogPrefix[]  = "Lacaille ";
constexpr const char TychoCatalogPrefix[]     = "TYC ";
constexpr const char SAOCatalogPrefix[]       = "SAO ";

// The size of the root star octree node is also the maximum distance
// distance from the Sun at which any star may be located. The current
// setting of 1.0e7 light years is large enough to contain the entire
// local group of galaxies. A larger value should be OK, but the
// performance implications for octree traversal still need to be
// investigated.
constexpr const float STAR_OCTREE_ROOT_SIZE   = 10000000.0f;

constexpr const float STAR_OCTREE_MAGNITUDE   = 6.0f;
//constexpr const float STAR_EXTRA_ROOM        = 0.01f; // Reserve 1% capacity for extra stars

constexpr const char FILE_HEADER[]            = "CELSTARS";
constexpr const char CROSSINDEX_FILE_HEADER[] = "CELINDEX";


// Used to sort stars by catalog number
struct CatalogNumberOrderingPredicate
{
    int unused;

    CatalogNumberOrderingPredicate() = default;

    bool operator()(const Star& star0, const Star& star1) const
    {
        return (star0.getMainIndexNumber() < star1.getMainIndexNumber());
    }
};


struct CatalogNumberEquivalencePredicate
{
    int unused;

    CatalogNumberEquivalencePredicate() = default;

    bool operator()(const Star& star0, const Star& star1) const
    {
        return (star0.getMainIndexNumber() == star1.getMainIndexNumber());
    }
};


// Used to sort star pointers by catalog number
struct PtrCatalogNumberOrderingPredicate
{
    int unused;

    PtrCatalogNumberOrderingPredicate() = default;

    bool operator()(const Star* const & star0, const Star* const & star1) const
    {
        return (star0->getMainIndexNumber() < star1->getMainIndexNumber());
    }
};


static bool parseSimpleCatalogNumber(const string& name,
                                     const string& prefix,
                                     uint32_t* catalogNumber)
{
    char extra[4];
    if (compareIgnoringCase(name, prefix, prefix.length()) == 0)
    {
        unsigned int num;
        // Use scanf to see if we have a valid catalog number; it must be
        // of the form: <prefix> <non-negative integer>  No additional
        // characters other than whitespace are allowed after the number.
        if (sscanf(name.c_str() + prefix.length(), " %u %c", &num, extra) == 1)
        {
            *catalogNumber = (uint32_t) num;
            return true;
        }
    }

    return false;
}


static bool parseHIPPARCOSCatalogNumber(const string& name,
                                        uint32_t* catalogNumber)
{
    return parseSimpleCatalogNumber(name,
                                    HIPPARCOSCatalogPrefix,
                                    catalogNumber);
}


static bool parseHDCatalogNumber(const string& name,
                                 uint32_t* catalogNumber)
{
    return parseSimpleCatalogNumber(name,
                                    HDCatalogPrefix,
                                    catalogNumber);
}


static bool parseTychoCatalogNumber(const string& name,
                                    uint32_t* catalogNumber)
{
    int len = strlen(TychoCatalogPrefix);
    if (compareIgnoringCase(name, TychoCatalogPrefix, len) == 0)
    {
        unsigned int tyc1 = 0, tyc2 = 0, tyc3 = 0;
        if (sscanf(string(name, len, string::npos).c_str(),
                   " %u-%u-%u", &tyc1, &tyc2, &tyc3) == 3)
        {
            *catalogNumber = (uint32_t) (tyc3 * 1000000000 + tyc2 * 10000 + tyc1);
            return true;
        }
    }

    return false;
}


static bool parseCelestiaCatalogNumber(const string& name,
                                       uint32_t* catalogNumber)
{
    char extra[4];

    if (name[0] == '#')
    {
        unsigned int num;
        if (sscanf(name.c_str(), "#%u %c", &num, extra) == 1)
        {
            *catalogNumber = (uint32_t) num;
            return true;
        }
    }

    return false;
}


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


Star* StarDatabase::find(uint32_t catalogNumber) const
{
    Star refStar;
    refStar.setMainIndexNumber(catalogNumber);

    Star** star   = lower_bound(catalogNumberIndex,
                                catalogNumberIndex + nStars,
                                &refStar,
                                PtrCatalogNumberOrderingPredicate());

    if (star != catalogNumberIndex + nStars && (*star)->getMainIndexNumber() == catalogNumber)
        return *star;
    else
        return nullptr;
}


uint32_t StarDatabase::findCatalogNumberByName(const string& name) const
{
    if (name.empty())
        return AstroCatalog::InvalidIndex;

    uint32_t catalogNumber = AstroCatalog::InvalidIndex;

    if (namesDB != nullptr)
    {
        catalogNumber = namesDB->findIndexNumberByName(name);
        if (catalogNumber != AstroCatalog::InvalidIndex)
            return catalogNumber;
    }

    if (parseCelestiaCatalogNumber(name, &catalogNumber))
    {
        return catalogNumber;
    }
    else if (parseHIPPARCOSCatalogNumber(name, &catalogNumber))
    {
        return catalogNumber;
    }
    else if (parseTychoCatalogNumber(name, &catalogNumber))
    {
        return catalogNumber;
    }
    else if (parseHDCatalogNumber(name, &catalogNumber))
    {
        return searchCrossIndexForCatalogNumber(HenryDraper, catalogNumber);
    }
    else if (parseSimpleCatalogNumber(name, SAOCatalogPrefix,
                                      &catalogNumber))
    {
        return searchCrossIndexForCatalogNumber(SAO, catalogNumber);
    }
    else
    {
        return AstroCatalog::InvalidIndex;
    }
}


Star* StarDatabase::find(const string& name) const
{
    uint32_t catalogNumber = findCatalogNumberByName(name);
    if (catalogNumber != AstroCatalog::InvalidIndex)
        return find(catalogNumber);
    else
        return nullptr;
}


uint32_t StarDatabase::crossIndex(const Catalog catalog, const uint32_t celCatalogNumber) const
{
    if (static_cast<uint32_t>(catalog) >= crossIndexes.size())
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
uint32_t StarDatabase::searchCrossIndexForCatalogNumber(const Catalog catalog, const uint32_t number) const
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


Star* StarDatabase::searchCrossIndex(const Catalog catalog, const uint32_t number) const
{
    uint32_t celCatalogNumber = searchCrossIndexForCatalogNumber(catalog, number);
    if (celCatalogNumber != AstroCatalog::InvalidIndex)
        return find(celCatalogNumber);
    else
        return nullptr;
}


vector<string> StarDatabase::getCompletion(const string& name) const
{
    vector<string> completion;

    // only named stars are supported by completion.
    if (!name.empty() && namesDB != nullptr)
        return namesDB->getCompletion(name);
    else
        return completion;
}


#if 0
static void catalogNumberToString(uint32_t catalogNumber, char* buf, unsigned int bufSize)
{
    // TODO: implement using using fmt::write
}
#endif


static string catalogNumberToString(uint32_t catalogNumber)
{
    if (catalogNumber <= StarDatabase::MAX_HIPPARCOS_NUMBER)
    {
        return fmt::sprintf("HIP %d", catalogNumber);
    }
    else
    {
        uint32_t tyc3 = catalogNumber / 1000000000;
        catalogNumber -= tyc3 * 1000000000;
        uint32_t tyc2 = catalogNumber / 10000;
        catalogNumber -= tyc2 * 10000;
        uint32_t tyc1 = catalogNumber;
        return fmt::sprintf("TYC %d-%d-%d", tyc1, tyc2, tyc3);
    }
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
string StarDatabase::getStarName(const Star& star, bool i18n) const
{
    uint32_t catalogNumber = star.getMainIndexNumber();

    if (namesDB != nullptr)
    {
        StarNameDatabase::NumberIndex::const_iterator iter = namesDB->getFirstNameIter(catalogNumber);
        if (iter != namesDB->getFinalNameIter() && iter->first == catalogNumber)
        {
            if (i18n && iter->second != _(iter->second.c_str()))
                return _(iter->second.c_str());
            else
                return iter->second;
        }
    }

    /*
      // Get the HD catalog name
      if (star.getMainIndexNumber() != AstroCatalog::InvalidIndex)
      return fmt::sprintf("HD %d", star.getCatalogNumber(Star::HDCatalog));
      else
    */
    return catalogNumberToString(catalogNumber);
}

// A less convenient version of getStarName that writes to a char
// array instead of a string. The advantage is that no memory allocation
// will every occur.
void StarDatabase::getStarName(const Star& star, char* nameBuffer, unsigned int bufferSize, bool i18n) const
{
    assert(bufferSize != 0);

    uint32_t catalogNumber = star.getMainIndexNumber();

    if (namesDB != nullptr)
    {
        StarNameDatabase::NumberIndex::const_iterator iter = namesDB->getFirstNameIter(catalogNumber);
        if (iter != namesDB->getFinalNameIter() && iter->first == catalogNumber)
        {
            if (i18n && iter->second != _(iter->second.c_str()))
                strncpy(nameBuffer, _(iter->second.c_str()), bufferSize);
            else
                strncpy(nameBuffer, iter->second.c_str(), bufferSize);

            nameBuffer[bufferSize - 1] = '\0';
            return;
        }
    }

    strncpy(nameBuffer, catalogNumberToString(catalogNumber).c_str(), bufferSize);
    nameBuffer[bufferSize - 1] = '\0';
}


string StarDatabase::getStarNameList(const Star& star, const unsigned int maxNames) const
{
    string starNames;
    unsigned int catalogNumber = star.getMainIndexNumber();
    StarNameDatabase::NumberIndex::const_iterator iter = namesDB->getFirstNameIter(catalogNumber);

    unsigned int count = 0;
    while (iter != namesDB->getFinalNameIter() && iter->first == catalogNumber && count < maxNames)
    {
        if (count != 0)
            starNames += " / ";

        starNames += iter->second;
        ++iter;
        ++count;
    }

    uint32_t hip  = catalogNumber;
    if (hip != AstroCatalog::InvalidIndex && hip != 0 && count < maxNames)
    {
        if (hip <= Star::MaxTychoCatalogNumber)
        {
            if (count != 0)
                starNames += " / ";
            if (hip >= 1000000)
            {
                uint32_t h      = hip;
                uint32_t tyc3   = h / 1000000000;
                       h     -= tyc3 * 1000000000;
                uint32_t tyc2   = h / 10000;
                       h     -= tyc2 * 10000;
                uint32_t tyc1   = h;

                starNames += fmt::sprintf("TYC %u-%u-%u", tyc1, tyc2, tyc3);
            }
            else
            {
                starNames += fmt::sprintf("HIP %u", hip);
            }

            ++count;
        }
    }

    uint32_t hd   = crossIndex(StarDatabase::HenryDraper, hip);
    if (count < maxNames && hd != AstroCatalog::InvalidIndex)
    {
        if (count != 0)
            starNames += " / ";
        starNames += fmt::sprintf("HD %u", hd);
    }

    uint32_t sao   = crossIndex(StarDatabase::SAO, hip);
    if (count < maxNames && sao != AstroCatalog::InvalidIndex)
    {
        if (count != 0)
            starNames += " / ";
        starNames += fmt::sprintf("SAO %u", sao);
    }

    return starNames;
}


void StarDatabase::findVisibleStars(StarHandler& starHandler,
                                    const Vector3f& position,
                                    const Quaternionf& orientation,
                                    float fovY,
                                    float aspectRatio,
                                    float limitingMag,
                                    OctreeProcStats *stats) const
{
    // Compute the bounding planes of an infinite view frustum
    Hyperplane<float, 3> frustumPlanes[5];
    Vector3f planeNormals[5];
    Eigen::Matrix3f rot = orientation.toRotationMatrix();
    float h = (float) tan(fovY / 2);
    float w = h * aspectRatio;
    planeNormals[0] = Vector3f(0.0f, 1.0f, -h);
    planeNormals[1] = Vector3f(0.0f, -1.0f, -h);
    planeNormals[2] = Vector3f(1.0f, 0.0f, -w);
    planeNormals[3] = Vector3f(-1.0f, 0.0f, -w);
    planeNormals[4] = Vector3f(0.0f, 0.0f, -1.0f);
    for (int i = 0; i < 5; i++)
    {
        planeNormals[i] = rot.transpose() * planeNormals[i].normalized();
        frustumPlanes[i] = Hyperplane<float, 3>(planeNormals[i], position);
    }

    octreeRoot->processVisibleObjects(starHandler,
                                      position,
                                      frustumPlanes,
                                      limitingMag,
                                      STAR_OCTREE_ROOT_SIZE,
                                      stats);
}


void StarDatabase::findCloseStars(StarHandler& starHandler,
                                  const Vector3f& position,
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
    namesDB    = _namesDB;
}


bool StarDatabase::loadCrossIndex(const Catalog catalog, istream& in)
{
    if (static_cast<unsigned int>(catalog) >= crossIndexes.size())
        return false;

    if (crossIndexes[catalog] != nullptr)
        delete crossIndexes[catalog];

    // Verify that the star database file has a correct header
    {
        int headerLength = strlen(CROSSINDEX_FILE_HEADER);
        char* header = new char[headerLength];
        in.read(header, headerLength);
        if (strncmp(header, CROSSINDEX_FILE_HEADER, headerLength))
        {
            cerr << _("Bad header for cross index\n");
            delete[] header;
            return false;
        }
        delete[] header;
    }

    // Verify the version
    {
        uint16_t version;
        in.read((char*) &version, sizeof version);
        LE_TO_CPU_INT16(version, version);
        if (version != 0x0100)
        {
            cerr << _("Bad version for cross index\n");
            return false;
        }
    }

    CrossIndex* xindex = new CrossIndex();

    unsigned int record = 0;
    for (;;)
    {
        CrossIndexEntry ent;
        in.read((char *) &ent.catalogNumber, sizeof ent.catalogNumber);
        LE_TO_CPU_INT32(ent.catalogNumber, ent.catalogNumber);
        if (in.eof())
            break;

        in.read((char *) &ent.celCatalogNumber, sizeof ent.celCatalogNumber);
        LE_TO_CPU_INT32(ent.celCatalogNumber, ent.celCatalogNumber);
        if (in.fail())
        {
            fmt::fprintf(cerr, _("Loading cross index failed at record %u\n"), record);
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


bool StarDatabase::loadBinary(istream& in)
{
    uint32_t nStarsInFile = 0;

    // Verify that the star database file has a correct header
    {
        int headerLength = strlen(FILE_HEADER);
        char* header = new char[headerLength];
        in.read(header, headerLength);
        if (strncmp(header, FILE_HEADER, headerLength)) {
            delete[] header;
            return false;
        }
        delete[] header;
    }

    // Verify the version
    {
        uint16_t version;
        in.read((char*) &version, sizeof version);
        LE_TO_CPU_INT16(version, version);
        if (version != 0x0100)
            return false;
    }

    // Read the star count
    in.read((char *) &nStarsInFile, sizeof nStarsInFile);
    LE_TO_CPU_INT32(nStarsInFile, nStarsInFile);
    if (!in.good())
        return false;

    unsigned int totalStars = nStars + nStarsInFile;

    while (((unsigned int) nStars) < totalStars)
    {
        uint32_t catNo = 0;
        float x = 0.0f, y = 0.0f, z = 0.0f;
        int16_t absMag;
        uint16_t spectralType;

        in.read((char *) &catNo, sizeof catNo);
        LE_TO_CPU_INT32(catNo, catNo);
        in.read((char *) &x, sizeof x);
        LE_TO_CPU_FLOAT(x, x);
        in.read((char *) &y, sizeof y);
        LE_TO_CPU_FLOAT(y, y);
        in.read((char *) &z, sizeof z);
        LE_TO_CPU_FLOAT(z, z);
        in.read((char *) &absMag, sizeof absMag);
        LE_TO_CPU_INT16(absMag, absMag);
        in.read((char *) &spectralType, sizeof spectralType);
        LE_TO_CPU_INT16(spectralType, spectralType);
        if (in.bad())
        break;

        Star star;
        star.setPosition(x, y, z);
        star.setAbsoluteMagnitude((float) absMag / 256.0f);

        StarDetails* details = nullptr;
        StellarClass sc;
        if (sc.unpack(spectralType))
            details = StarDetails::GetStarDetails(sc);

        if (details == nullptr)
        {
            fmt::fprintf(cerr, _("Bad spectral type in star database, star #%u\n"), nStars);
            return false;
        }

        star.setDetails(details);
        star.setMainIndexNumber(catNo);
        unsortedStars.add(star);

        nStars++;
    }

    if (in.bad())
        return false;

    DPRINTF(0, "StarDatabase::read: nStars = %d\n", nStarsInFile);
    fmt::fprintf(clog, _("%d stars in binary database\n"), nStars);

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
        sort(binFileCatalogNumberIndex, binFileCatalogNumberIndex + binFileStarCount,
             PtrCatalogNumberOrderingPredicate());
    }

    return true;
}


void StarDatabase::finish()
{
    fmt::fprintf(clog, _("Total star count: %d\n"), nStars);

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


static void stcError(const Tokenizer& tok,
                     const string& msg)
{
    fmt::fprintf(cerr,  _("Error in .stc file (line %i): %s\n"), tok.getLineNumber(), msg);
}


/*! Load star data from a property list into a star instance.
 */
bool StarDatabase::createStar(Star* star,
                              DataDisposition disposition,
                              uint32_t catalogNumber,
                              Hash* starData,
                              const fs::path& path,
                              bool isBarycenter)
{
    StarDetails* details = nullptr;
    string spectralType;

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
                cerr << _("Invalid star: bad spectral type.\n");
                return false;
            }
        }
        else
        {
            // Spectral type is required for new stars
            if (disposition != DataDisposition::Modify)
            {
                cerr << _("Invalid star: missing spectral type.\n");
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

    string modelName;
    string textureName;
    bool hasTexture = starData->getString("Texture", textureName);
    bool hasModel = starData->getString("Mesh", modelName);

    RotationModel* rm = CreateRotationModel(starData, path, 1.0);
    bool hasRotationModel = (rm != nullptr);

    Vector3d semiAxes = Vector3d::Ones();
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

    string infoURL;
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
            ResourceHandle geometryHandle = GetGeometryManager()->getHandle(GeometryInfo(modelName, path, Vector3f::Zero(), 1.0f, true));
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

                double logT = log10(temperature) - 4;
                double bc = -8.499 * pow(logT, 4) + 13.421 * pow(logT, 3)
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
            uint32_t barycenterCatNo = AstroCatalog::InvalidIndex;
            bool barycenterDefined = false;

            string barycenterName;
            if (starData->getString("OrbitBarycenter", barycenterName))
            {
                barycenterCatNo   = findCatalogNumberByName(barycenterName);
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
                    fmt::fprintf(cerr, _("Barycenter %s does not exist.\n"), barycenterName);
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
        star->setMainIndexNumber(catalogNumber);

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
            Vector3f pos = star->getPosition();

            // Convert from Celestia's coordinate system
            Vector3f v(pos.x(), -pos.z(), pos.y());
            v = Quaternionf(AngleAxis<float>((float) astro::J2000Obliquity, Vector3f::UnitX())) * v;

            distance = v.norm();
            if (distance > 0.0)
            {
                v.normalize();
                ra = radToDeg(std::atan2(v.y(), v.x())) / DEG_PER_HRA;
                dec = radToDeg(std::asin(v.z()));
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
                cerr << _("Invalid star: missing right ascension\n");
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
                cerr << _("Invalid star: missing declination.\n");
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
                cerr << _("Invalid star: missing distance.\n");
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
            Vector3d pos = astro::equatorialToCelestialCart((double) raf, (double) decf, (double) distancef);
            star->setPosition(pos.cast<float>());
        }
    }

    if (isBarycenter)
    {
        star->setAbsoluteMagnitude(30.0f);
    }
    else
    {
        double magnitude = 0.0;
        bool magnitudeModified = true;
        if (!starData->getNumber("AbsMag", magnitude))
        {
            if (!starData->getNumber("AppMag", magnitude))
            {
                if (disposition != DataDisposition::Modify)
                {
                    clog << _("Invalid star: missing magnitude.\n");
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
                    clog << _("Invalid star: absolute (not apparent) magnitude must be specified for star near origin\n");
                    return false;
                }
                magnitude = astro::appToAbsMag((float) magnitude, distance);
            }
        }

        if (magnitudeModified)
            star->setAbsoluteMagnitude((float) magnitude);
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
bool StarDatabase::load(istream& in, const fs::path& resourcePath)
{
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    const char *d = resourcePath.string().c_str();
    bindtextdomain(d, d); // domain name is the same as resource path

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        bool isStar = true;

        // Parse the disposition--either Add, Replace, or Modify. The disposition
        // may be omitted. The default value is Add.
        DataDisposition disposition = DataDisposition::Add;
        if (tokenizer.getTokenType() == Tokenizer::TokenName)
        {
            if (tokenizer.getNameValue() == "Modify")
            {
                disposition = DataDisposition::Modify;
                tokenizer.nextToken();
            }
            else if (tokenizer.getNameValue() == "Replace")
            {
                disposition = DataDisposition::Replace;
                tokenizer.nextToken();
            }
            else if (tokenizer.getNameValue() == "Add")
            {
                disposition = DataDisposition::Add;
                tokenizer.nextToken();
            }
        }

        // Parse the object type--either Star or Barycenter. The object type
        // may be omitted. The default is Star.
        if (tokenizer.getTokenType() == Tokenizer::TokenName)
        {
            if (tokenizer.getNameValue() == "Star")
            {
                isStar = true;
            }
            else if (tokenizer.getNameValue() == "Barycenter")
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
        uint32_t catalogNumber = AstroCatalog::InvalidIndex;
        if (tokenizer.getTokenType() == Tokenizer::TokenNumber)
        {
            catalogNumber = (uint32_t) tokenizer.getNumberValue();
            tokenizer.nextToken();
        }

        string objName;
        string firstName;
        if (tokenizer.getTokenType() == Tokenizer::TokenString)
        {
            // A star name (or names) is present
            objName    = tokenizer.getStringValue();
            tokenizer.nextToken();
            if (!objName.empty())
            {
                string::size_type next = objName.find(':', 0);
                firstName = objName.substr(0, next);
            }
        }

        Star* star = nullptr;

        switch (disposition)
        {
        case DataDisposition::Add:
            // Automatically generate a catalog number for the star if one isn't
            // supplied.
            if (catalogNumber == AstroCatalog::InvalidIndex)
            {
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
                    catalogNumber = findCatalogNumberByName(firstName);
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
                catalogNumber = findCatalogNumberByName(firstName);
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
            clog << "Error reading star.\n";
            return false;
        }

        if (starDataValue->getType() != Value::HashType)
        {
            DPRINTF(0, "Bad star definition.\n");
            delete starDataValue;
            return false;
        }
        Hash* starData = starDataValue->getHash();

        if (isNewStar)
            star = new Star();

        bool ok = false;
        if (isNewStar && disposition == DataDisposition::Modify)
        {
            clog << "Modify requested for nonexistent star.\n";
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
                string::size_type startPos = 0;
                while (startPos != string::npos)
                {
                    string::size_type next    = objName.find(':', startPos);
                    string::size_type length = string::npos;
                    if (next != string::npos)
                    {
                        length = next - startPos;
                        ++next;
                    }
                    string starName = objName.substr(startPos, length);
                    namesDB->add(catalogNumber, starName);
                    if (starName != _(starName.c_str()))
                        namesDB->add(catalogNumber, _(starName.c_str()));
                    startPos = next;
                }
            }
        }
        else
        {
            if (isNewStar)
                delete star;
            DPRINTF(1, "Bad star definition--will continue parsing file.\n");
        }
    }

    return true;
}


void StarDatabase::buildOctree()
{
    // This should only be called once for the database
    // ASSERT(octreeRoot == nullptr);

    DPRINTF(1, "Sorting stars into octree . . .\n");
    float absMag = astro::appToAbsMag(STAR_OCTREE_MAGNITUDE,
                                      STAR_OCTREE_ROOT_SIZE * (float) sqrt(3.0));
    DynamicStarOctree* root = new DynamicStarOctree(Vector3f(1000.0f, 1000.0f, 1000.0f),
                                                    absMag);
    for (unsigned int i = 0; i < unsortedStars.size(); ++i)
    {
        root->insertObject(unsortedStars[i], STAR_OCTREE_ROOT_SIZE);
    }

    DPRINTF(1, "Spatially sorting stars for improved locality of reference . . .\n");
    Star* sortedStars    = new Star[nStars];
    Star* firstStar      = sortedStars;
    root->rebuildAndSort(octreeRoot, firstStar);

    // ASSERT((int) (firstStar - sortedStars) == nStars);
    DPRINTF(1, "%d stars total\n", (int) (firstStar - sortedStars));
    DPRINTF(1, "Octree has %d nodes and %d stars.\n",
            1 + octreeRoot->countChildren(), octreeRoot->countObjects());
#ifdef PROFILE_OCTREE
    vector<OctreeLevelStatistics> stats;
    octreeRoot->computeStatistics(stats);
    int level = 0;
    for (const auto& stat : stats)
    {
        level++;
        fmt::fprintf(clog,
                     _("Level %i, %.5f ly, %i nodes, %i  stars\n"),
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

    DPRINTF(1, "Building catalog number indexes . . .\n");

    catalogNumberIndex = new Star*[nStars];
    for (int i = 0; i < nStars; ++i)
        catalogNumberIndex[i] = &stars[i];

    sort(catalogNumberIndex, catalogNumberIndex + nStars, PtrCatalogNumberOrderingPredicate());
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
Star* StarDatabase::findWhileLoading(uint32_t catalogNumber) const
{
    // First check for stars loaded from the binary database
    if (binFileCatalogNumberIndex != nullptr)
    {
        Star refStar;
        refStar.setMainIndexNumber(catalogNumber);

        Star** star   = lower_bound(binFileCatalogNumberIndex,
                                    binFileCatalogNumberIndex + binFileStarCount,
                                    &refStar,
                                    PtrCatalogNumberOrderingPredicate());

        if (star != binFileCatalogNumberIndex + binFileStarCount && (*star)->getMainIndexNumber() == catalogNumber)
            return *star;
    }

    // Next check for stars loaded from an stc file
    map<uint32_t, Star*>::const_iterator iter = stcFileCatalogNumberIndex.find(catalogNumber);
    if (iter != stcFileCatalogNumberIndex.end())
    {
        return iter->second;
    }

    // Star not found
    return nullptr;
}

