// stardb.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <celmath/mathlib.h>
#include <celmath/plane.h>
#include <celutil/util.h>
#include <celutil/bytes.h>
#include "celestia.h"
#include "astro.h"
#include "stardb.h"
#include "parser.h"
#include "parseobject.h"
#include "multitexture.h"
#include "meshmanager.h"
#include <celutil/debug.h>

using namespace std;


static string HDCatalogPrefix("HD ");
static string HIPPARCOSCatalogPrefix("HIP ");
static string GlieseCatalogPrefix("Gliese ");
static string RossCatalogPrefix("Ross ");
static string LacailleCatalogPrefix("Lacaille ");
static string TychoCatalogPrefix("TYC ");
static string SAOCatalogPrefix("SAO ");

static const float OctreeRootSize = 15000.0f;
static const float OctreeMagnitude = 6.0f;
static const float ExtraRoom = 0.01f; // Reserve 1% capacity for extra stars

const char* StarDatabase::FileHeader = "CELSTARS";
const char* StarDatabase::CrossIndexFileHeader = "CELINDEX";


StarDatabase::StarDatabase() : nStars(0),
                               capacity(0),
                               stars(NULL),
                               names(NULL),
                               octreeRoot(NULL)
{
    crossIndexes.resize(MaxCatalog);
}


StarDatabase::~StarDatabase()
{
    if (stars != NULL)
	delete [] stars;

    if (catalogNumberIndex != NULL)
        delete [] catalogNumberIndex;

    for (vector<CrossIndex*>::iterator iter = crossIndexes.begin();
         iter != crossIndexes.end(); iter++)
    {
        if (*iter != NULL)
            delete *iter;
    }
}


// Used to sort stars by catalog number
struct CatalogNumberOrderingPredicate
{
    int unused;

    CatalogNumberOrderingPredicate() {};

    bool operator()(const Star& star0, const Star& star1) const
    {
        return (star0.getCatalogNumber() < star1.getCatalogNumber());
    }
};


struct CatalogNumberEquivalencePredicate
{
    int unused;

    CatalogNumberEquivalencePredicate() {};

    bool operator()(const Star& star0, const Star& star1) const
    {
        return (star0.getCatalogNumber() == star1.getCatalogNumber());
    }
};


// Used to sort star pointers by catalog number
struct PtrCatalogNumberOrderingPredicate
{
    int unused;

    PtrCatalogNumberOrderingPredicate() {};

    bool operator()(const Star* const & star0, const Star* const & star1) const
    {
        return (star0->getCatalogNumber() < star1->getCatalogNumber());
    }
};


Star* StarDatabase::find(uint32 catalogNumber) const
{
    Star refStar;
    refStar.setCatalogNumber(catalogNumber);

    Star** star = lower_bound(catalogNumberIndex,
                              catalogNumberIndex + nStars,
                              &refStar,
                              PtrCatalogNumberOrderingPredicate());

    if (star != catalogNumberIndex + nStars &&
        (*star)->getCatalogNumber() == catalogNumber)
        return *star;
    else
        return NULL;
}


uint32
StarDatabase::crossIndex(Catalog catalog, uint32 celCatalogNumber) const
{
    if (static_cast<uint32>(catalog) >= crossIndexes.size())
        return Star::InvalidCatalogNumber;

    CrossIndex* xindex = crossIndexes[catalog];
    if (xindex == NULL)
        return Star::InvalidCatalogNumber;

    // A simple linear search.  We could store cross indices sorted by
    // both catalog numbers and trade memory for speed
    for (CrossIndex::const_iterator iter = xindex->begin();
         iter != xindex->end(); iter++)
    {
        if (celCatalogNumber == iter->celCatalogNumber)
            return iter->catalogNumber;
    }

    return Star::InvalidCatalogNumber;
}


static bool parseSimpleCatalogNumber(const string& name,
                                     const string& prefix,
                                     uint32* catalogNumber)
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
            *catalogNumber = (uint32) num;
            return true;
        }
    }

    return false;
}


static bool parseHIPPARCOSCatalogNumber(const string& name, 
                                        uint32* catalogNumber)
{
    return parseSimpleCatalogNumber(name, HIPPARCOSCatalogPrefix,
                                    catalogNumber);
}


static bool parseHDCatalogNumber(const string& name, 
                                 uint32* catalogNumber)
{
    return parseSimpleCatalogNumber(name, HDCatalogPrefix,
                                    catalogNumber);
}


static bool parseTychoCatalogNumber(const string& name,
                                    uint32* catalogNumber)
{
    if (compareIgnoringCase(name, TychoCatalogPrefix, TychoCatalogPrefix.length()) == 0)
    {
        unsigned int tyc1 = 0, tyc2 = 0, tyc3 = 0;
        if (sscanf(string(name, TychoCatalogPrefix.length(), string::npos).c_str(), " %u-%u-%u", &tyc1, &tyc2, &tyc3) == 3)
        {
            *catalogNumber = (uint32) (tyc3 * 1000000000 + tyc2 * 10000 + tyc1);
            return true;
        }
    }

    return false;
}


static bool parseCelestiaCatalogNumber(const string& name,
                                       uint32* catalogNumber)
{
    char extra[4];

    if (name[0] == '#')
    {
        unsigned int num;
        if (sscanf(name.c_str(), "#%u %c", &num, &extra) == 1)
        {
            *catalogNumber = (uint32) num;
            return true;
        }
    }

    return false;
}


bool operator< (const StarDatabase::CrossIndexEntry& a,
                const StarDatabase::CrossIndexEntry& b)
{
    return a.catalogNumber < b.catalogNumber;
}


Star*
StarDatabase::searchCrossIndex(Catalog catalog, uint32 number) const
{
    if (static_cast<unsigned int>(catalog) >= crossIndexes.size())
        return NULL;

    CrossIndex* xindex = crossIndexes[catalog];
    if (xindex == NULL)
        return NULL;

    CrossIndexEntry xindexEnt;
    xindexEnt.catalogNumber = number;

    CrossIndex::iterator iter = lower_bound(xindex->begin(), xindex->end(),
                                            xindexEnt);
    if (iter == xindex->end() || iter->catalogNumber != number)
    {
        return NULL;
    }
    else
    {
        if (iter->celCatalogNumber >= (uint32) nStars)
            return NULL;
        else
            return find(iter->celCatalogNumber);
    }
}


Star* StarDatabase::find(const string& name) const
{
    if (name.empty())
        return NULL;

    uint32 catalogNumber = 0;

    if (parseCelestiaCatalogNumber(name, &catalogNumber))
    {
        return find(catalogNumber);
    }
    else if (parseHIPPARCOSCatalogNumber(name, &catalogNumber))
    {
        return find(catalogNumber);
    }
    else if (parseTychoCatalogNumber(name, &catalogNumber))
    {
        return find(catalogNumber);
    }
    else if (parseHDCatalogNumber(name, &catalogNumber))
    {
        return searchCrossIndex(HenryDraper, catalogNumber);
    }
    else if (parseSimpleCatalogNumber(name, SAOCatalogPrefix,
                                      &catalogNumber))
    {
        return searchCrossIndex(SAO, catalogNumber);
    }
    else
    {
        if (names != NULL)
        {
            uint32 catalogNumber = names->findName(name);
            if (catalogNumber != Star::InvalidCatalogNumber)
                return find(catalogNumber);
        }

        return NULL;
    }
}

std::vector<std::string> StarDatabase::getCompletion(const string& name) const
{
    std::vector<std::string> completion;

    // only named stars are supported by completion.
    if (!name.empty() && names != NULL)
        return names->getCompletion(name);
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
string StarDatabase::getStarName(const Star& star) const
{
    uint32 catalogNumber = star.getCatalogNumber();

    if (names != NULL)
    {
        StarNameDatabase::NumberIndex::const_iterator iter = getStarNames(catalogNumber);
        if (iter != finalName() && iter->first == catalogNumber)
        {
            return iter->second;
        }
    }

    char buf[20];
    /*
      // Get the HD catalog name
      if (star.getCatalogNumber() != Star::InvalidCatalogNumber)
      sprintf(buf, "HD %d", star.getCatalogNumber(Star::HDCatalog));
      else
    */
    {
        if (catalogNumber < 1000000)
        {
            sprintf(buf, "HIP %d", catalogNumber);
        }
        else
        {
            uint32 tyc3 = catalogNumber / 1000000000;
            catalogNumber -= tyc3 * 1000000000;
            uint32 tyc2 = catalogNumber / 10000;
            catalogNumber -= tyc2 * 10000;
            uint32 tyc1 = catalogNumber;
            sprintf(buf, "TYC %d-%d-%d", tyc1, tyc2, tyc3);
        }
    }
    return string(buf);
}


StarNameDatabase::NumberIndex::const_iterator
StarDatabase::getStarNames(uint32 catalogNumber) const
{
    // assert(names != NULL);
    return names->findFirstName(catalogNumber);
}


StarNameDatabase::NumberIndex::const_iterator
StarDatabase::finalName() const
{
    // assert(names != NULL);
    return names->finalName();
}



void StarDatabase::findVisibleStars(StarHandler& starHandler,
                                    const Point3f& position,
                                    const Quatf& orientation,
                                    float fovY,
                                    float aspectRatio,
                                    float limitingMag) const
{
    // Compute the bounding planes of an infinite view frustum
    Planef frustumPlanes[5];
    Vec3f planeNormals[5];
    Mat3f rot = orientation.toMatrix3();
    float h = (float) tan(fovY / 2);
    float w = h * aspectRatio;
    planeNormals[0] = Vec3f(0, 1, -h);
    planeNormals[1] = Vec3f(0, -1, -h);
    planeNormals[2] = Vec3f(1, 0, -w);
    planeNormals[3] = Vec3f(-1, 0, -w);
    planeNormals[4] = Vec3f(0, 0, -1);
    for (int i = 0; i < 5; i++)
    {
        planeNormals[i].normalize();
        planeNormals[i] = planeNormals[i] * rot;
        frustumPlanes[i] = Planef(planeNormals[i], position);
    }
    
    octreeRoot->findVisibleStars(starHandler, position, frustumPlanes,
                                 limitingMag, OctreeRootSize);
}


void StarDatabase::findCloseStars(StarHandler& starHandler,
                                  const Point3f& position,
                                  float radius) const
{
    octreeRoot->findCloseStars(starHandler, position, radius, OctreeRootSize);
}


StarNameDatabase* StarDatabase::getNameDatabase() const
{
    return names;
}

void StarDatabase::setNameDatabase(StarNameDatabase* _names)
{
    names = _names;
}


bool StarDatabase::loadCrossIndex(Catalog catalog, istream& in)
{
    if (static_cast<unsigned int>(catalog) >= crossIndexes.size())
        return false;

    if (crossIndexes[catalog] != NULL)
        delete crossIndexes[catalog];

    // Verify that the star database file has a correct header
    {
        int headerLength = strlen(CrossIndexFileHeader);
        char* header = new char[headerLength];
        in.read(header, headerLength);
        if (strncmp(header, CrossIndexFileHeader, headerLength))
        {
            cerr << "Bad header for cross index\n";
            return false;
        }
        delete[] header;
    }

    // Verify the version
    {
        uint16 version;
        in.read((char*) &version, sizeof version);
        LE_TO_CPU_INT16(version, version);
        if (version != 0x0100)
        {
            cerr << "Bad version for cross index\n";
            return false;
        }
    }

    CrossIndex* xindex = new CrossIndex();
    if (xindex == NULL)
        return false;

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
            cerr << "Loading cross index failed at record " << record << '\n';
            delete xindex;
            return false;
        }

        xindex->insert(xindex->end(), ent);

        record++;
    }

    sort(xindex->begin(), xindex->end());

    crossIndexes[catalog] = xindex;

    return true;
}


bool StarDatabase::loadOldFormatBinary(istream& in)
{
    uint32 nStarsInFile = 0;
    in.read((char *) &nStarsInFile, sizeof nStarsInFile);
    LE_TO_CPU_INT32(nStarsInFile, nStarsInFile);
    if (!in.good())
        return false;

    int requiredCapacity = (int) ((nStars + nStarsInFile) * (1.0 + ExtraRoom));
    if (capacity < requiredCapacity)
    {
        Star* newStars = new Star[requiredCapacity];
        if (newStars == NULL)
            return false;
        
        if (stars != NULL)
        {
            copy(stars, stars + nStars, newStars);
            delete[] stars;
        }

        stars = newStars;

        capacity = requiredCapacity;
    }
    
    uint32 throwOut = 0;
    uint32 fixUp = 0;
    unsigned int totalStars = nStars + nStarsInFile;

    while (((unsigned int) nStars) < totalStars)
    {
	uint32 catNo = 0;
        uint32 hdCatNo = 0;
	float RA = 0, dec = 0, parallax = 0;
	int16 appMag;
	uint16 spectralType;
	uint8 parallaxError;

	in.read((char *) &catNo, sizeof catNo);
        LE_TO_CPU_INT32(catNo, catNo);
        in.read((char *) &hdCatNo, sizeof hdCatNo);
        LE_TO_CPU_INT32(hdCatNo, hdCatNo);
	in.read((char *) &RA, sizeof RA);
        LE_TO_CPU_FLOAT(RA, RA);
	in.read((char *) &dec, sizeof dec);
        LE_TO_CPU_FLOAT(dec, dec);
	in.read((char *) &parallax, sizeof parallax);
        LE_TO_CPU_FLOAT(parallax, parallax);
	in.read((char *) &appMag, sizeof appMag);
        LE_TO_CPU_INT16(appMag, appMag);
	in.read((char *) &spectralType, sizeof spectralType);
        LE_TO_CPU_INT16(spectralType, spectralType);
	in.read((char *) &parallaxError, sizeof parallaxError);
        if ( in.bad() )
	    break;

	Star* star = &stars[nStars];

	// Compute distance based on parallax
	double distance = LY_PER_PARSEC / (parallax > 0.0 ? parallax / 1000.0 : 1e-6);
#ifdef DEBUG
	if (distance > 50000.0)
	{
	    DPRINTF(0, "Warning, distance of star # %ld of %12.2f ly seems excessive (parallax: %2.5f)!\n", catNo, distance, parallax);
	}
#endif // DEBUG
        Point3d pos = astro::equatorialToCelestialCart((double) RA, (double) dec, distance);
        star->setPosition(Point3f((float) pos.x, (float) pos.y, (float) pos.z));

	// Use apparent magnitude and distance to determine the absolute
	// magnitude of the star.
	star->setAbsoluteMagnitude((float) (appMag / 256.0 + 5 -
					    5 * log10(distance / 3.26)));

        
        StarDetails* details = NULL;
        StellarClass sc;
        if (sc.unpack(spectralType))
            details = StarDetails::GetStarDetails(sc);

        if (details == NULL)
        {
            cerr << "Bad spectral type in star database, star #\n";
            return false;
        }
        
        star->setDetails(details);
        star->setCatalogNumber(catNo);

	// TODO: Use a photometric estimate of distance if parallaxError is
	// greater than 25%.
	if (parallaxError > 50)
        {
	    if (appMag / 256.0 > 6)
		throwOut++;
	    else
		fixUp++;
	}

	nStars++;
    }

    if (in.bad())
        return false;

    DPRINTF(0, "StarDatabase::read: nStars = %d\n", nStarsInFile);
    cout << "nStars: " << nStars << '\n';

    return true;
}


bool StarDatabase::loadBinary(istream& in)
{
    uint32 nStarsInFile = 0;

    // Verify that the star database file has a correct header
    {
        int headerLength = strlen(FileHeader);
        char* header = new char[headerLength];
        in.read(header, headerLength);
        if (strncmp(header, FileHeader, headerLength))
            return false;
        delete[] header;
    }

    // Verify the version
    {
        uint16 version;
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

    int requiredCapacity = (int) ((nStars + nStarsInFile) * (1.0 + ExtraRoom));
    if (capacity < requiredCapacity)
    {
        Star* newStars = new Star[requiredCapacity];
        if (newStars == NULL)
            return false;
        
        if (stars != NULL)
        {
            copy(stars, stars + nStars, newStars);
            delete[] stars;
        }

        stars = newStars;

        capacity = requiredCapacity;
    }
    
    unsigned int totalStars = nStars + nStarsInFile;

    while (((unsigned int) nStars) < totalStars)
    {
	uint32 catNo = 0;
	float x = 0.0f, y = 0.0f, z = 0.0f;
	int16 absMag;
	uint16 spectralType;

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

	Star* star = &stars[nStars];

        star->setPosition(x, y, z);
	star->setAbsoluteMagnitude((float) absMag / 256.0f);

        StarDetails* details = NULL;
        StellarClass sc;
        if (sc.unpack(spectralType))
            details = StarDetails::GetStarDetails(sc);

        if (details == NULL)
        {
            cerr << "Bad spectral type in star database, star #" << nStars << "\n";
            return false;
        }

        star->setDetails(details);
	star->setCatalogNumber(catNo);

	nStars++;
    }

    if (in.bad())
        return false;

    DPRINTF(0, "StarDatabase::read: nStars = %d\n", nStarsInFile);
    clog << nStars << " stars in binary database\n";

    return true;
}


void StarDatabase::finish()
{
    // Eliminate duplicate stars; reverse the list so that for stars with
    // identical catalog numbers, the most recently added one is kept.
    reverse(stars, stars + nStars);
    stable_sort(stars, stars + nStars, CatalogNumberOrderingPredicate());
    Star* lastStar = unique(stars, stars + nStars,
                            CatalogNumberEquivalencePredicate());

    int nUniqueStars = lastStar - stars;
    clog << "Total star count: " << nUniqueStars <<
        " (" << (nStars - nUniqueStars) <<
        " star(s) with duplicate catalog numbers deleted.)\n";
    nStars = nUniqueStars;

    buildOctree();
    buildIndexes();
}


void StarDatabase::buildOctree()
{
    // This should only be called once for the database
    // ASSERT(octreeRoot == NULL);

    DPRINTF(1, "Sorting stars into octree . . .\n");
    cout.flush();
    float absMag = astro::appToAbsMag(OctreeMagnitude,
                                      OctreeRootSize * (float) sqrt(3.0));
    DynamicStarOctree* root = new DynamicStarOctree(Point3f(1000, 1000, 1000),
                                                    absMag);
    for (int i = 0; i < nStars; i++)
        root->insertStar(stars[i], OctreeRootSize);

    DPRINTF(1, "Spatially sorting stars for improved locality of reference . . .\n");
    cout.flush();
    Star* sortedStars = new Star[nStars];
    Star* firstStar = sortedStars;
    root->rebuildAndSort(octreeRoot, firstStar);

    // ASSERT((int) (firstStar - sortedStars) == nStars);
    DPRINTF(1, "%d stars total\n", (int) (firstStar - sortedStars));
    DPRINTF(1, "Octree has %d nodes and %d stars.\n",
            1 + octreeRoot->countChildren(), octreeRoot->countStars());

    // Clean up . . .
    delete[] stars;
    delete root;

    stars = sortedStars;
}


void StarDatabase::buildIndexes()
{
    // This should only be called once for the database
    // assert(catalogNumberIndexes[0] == NULL);

    DPRINTF(1, "Building catalog number indexes . . .\n");

    catalogNumberIndex = new Star*[nStars];
    for (int i = 0; i < nStars; i++)
        catalogNumberIndex[i] = &stars[i];

    sort(catalogNumberIndex, catalogNumberIndex + nStars,
         PtrCatalogNumberOrderingPredicate());
}


static Star* CreateStar(uint32 catalogNumber,
                        Hash* starData,
                        const string& path)
{
    double ra = 0.0;
    double dec = 0.0;
    double distance = 0.0;
    string spectralType;

    if (!starData->getNumber("RA", ra))
    {
        DPRINTF(1, "Invalid star: missing right ascension\n");
        return NULL;
    }

    if (!starData->getNumber("Dec", dec))
    {
        DPRINTF(1, "Invalid star: missing declination.\n");
        return NULL;
    }

    if (!starData->getNumber("Distance", distance))
    {
        DPRINTF(1, "Invalid star: missing distance.\n");
        return NULL;
    }

    if (!starData->getString("SpectralType", spectralType))
    {
        DPRINTF(1, "Invalid star: missing spectral type.\n");
        return NULL;
    }
    StellarClass sc = StellarClass::parse(spectralType);
    StarDetails* details = NULL;
    details = StarDetails::GetStarDetails(sc);
    if (details == NULL)
    {
        DPRINTF(1, "Invalid star: bad spectral type.\n");
        return NULL;
    }

    double absMag = 0.0;
    if (!starData->getNumber("AbsMag", absMag))
    {
        double appMag;
        if (starData->getNumber("AppMag", appMag))
            absMag = astro::appToAbsMag((float) appMag, (float) distance);
    }

    string modelName;
    string textureName;
    bool hasTexture = starData->getString("Texture", textureName);
    bool hasModel = starData->getString("Mesh", modelName);
    Orbit* orbit = CreateOrbit(NULL, starData, path, true);

    if (hasTexture || hasModel || orbit != NULL)
    {
        // If the star definition has extended information, clone the
        // star details so we can customize it without affecting other
        // stars of the same spectral type.
        // TODO: Need better management of star details objects.  The
        // standard ones should be persistent, but the custom ones should
        // probably be destroyed in the star destructor.  Reference counting
        // is probably the best strategy.
        details = new StarDetails(*details);

        if (hasTexture)
            details->setTexture(MultiResTexture(textureName, path));

        if (hasModel)
        {
            ResourceHandle modelHandle = GetModelManager()->getHandle(ModelInfo(modelName, path, Vec3f(0.0f, 0.0f, 0.0f)));
            details->setModel(modelHandle);
        }

        if (orbit != NULL)
        {
            details->setOrbit(orbit);
            bool barycenterIsOrigin = false;
            starData->getBoolean("BarycenterIsOrigin", barycenterIsOrigin);
            if (barycenterIsOrigin)
                details->setSystemOrigin(StarDetails::OriginBarycenter);
            else
                details->setSystemOrigin(StarDetails::OriginStar);
        }
    }

    Star* star = new Star();
    star->setDetails(details);
    star->setCatalogNumber(catalogNumber);
    star->setAbsoluteMagnitude((float) absMag);

    // Truncate to floats to match behavior of reading from binary file.  The
    // conversion to rectangular coordinates is still performed at double
    // precision, however.
    float raf = ((float) (ra * 24.0 / 360.0));
    float decf = ((float) dec);
    float distancef = ((float) distance);

    Point3d pos = astro::equatorialToCelestialCart((double) raf, (double) decf, (double) distancef);
    star->setPosition(Point3f((float) pos.x, (float) pos.y, (float) pos.z));

    return star;
}


bool StarDatabase::load(istream& in, const string& resourcePath)
{
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        if (tokenizer.getTokenType() != Tokenizer::TokenNumber)
        {
            DPRINTF(0, "Error parsing star file.\n");
            return false;
        }
        uint32 catalogNumber = (uint32) tokenizer.getNumberValue();

        string name;
        if (tokenizer.nextToken() == Tokenizer::TokenString)
        {
            // A star name (or names) is present
            name = tokenizer.getStringValue();
        }
        else
        {
            tokenizer.pushBack();
        }

        Value* starDataValue = parser.readValue();
        if (starDataValue == NULL)
        {
            DPRINTF(0, "Error reading star.\n");
            return false;
        }

        if (starDataValue->getType() != Value::HashType)
        {
            DPRINTF(0, "Bad star definition.\n");
            return false;
        }
        Hash* starData = starDataValue->getHash();

        Star* star = CreateStar(catalogNumber, starData, resourcePath);
        if (star != NULL)
        {
            // Ensure that the star array is large enough
            if (nStars == capacity)
            {
                // Grow the array by 5%--this may be too little, but the
                // assumption here is that there will be small numbers of
                // stars in text files added to a big collection loaded from
                // a binary file.
                capacity = (int) (capacity * 1.05);

                // 100 stars seems like a reasonable minimum
                if (capacity < 100)
                    capacity = 100;

                Star* newStars = new Star[capacity];
                if (newStars == NULL)
                {
                    DPRINTF(0, "Out of memory!");
                    return false;
                }

                if (stars != NULL)
                {
                    copy(stars, stars + nStars, newStars);
                    delete[] stars;
                }
                stars = newStars;
            }
            stars[nStars++] = *star;

            if (names != NULL && !name.empty())
            {
                // Iterate through the string for names delimited
                // by ':', and insert them into the star database.
                // Note that db->add() will skip empty names.
                string::size_type startPos = 0; 
                while (startPos != string::npos)
                {
                    string::size_type next = name.find(':', startPos);
                    string::size_type length = string::npos;
                    if (next != string::npos)
                    {
                        length = next - startPos;
                        next++;
                    }
                    names->add(catalogNumber, name.substr(startPos, length));
                    startPos = next;
                }
            }
        }
        else
        {
            DPRINTF(1, "Bad star definition--will continue parsing file.\n");
        }
    }

    return true;
}
