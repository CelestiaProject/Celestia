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
#include "celestia.h"
#include "mathlib.h"
#include "util.h"
#include "astro.h"
#include "plane.h"
#include "stardb.h"

using namespace std;


static string HDCatalogPrefix("HD ");
static string HIPPARCOSCatalogPrefix("HIP ");
static string GlieseCatalogPrefix("Gliese ");
static string RossCatalogPrefix("Ross ");
static string LacailleCatalogPrefix("Lacaille ");

static const float OctreeRootSize = 5000.0f;
static const float OctreeMagnitude = 6.0f;


StarDatabase::StarDatabase() : nStars(0),
                               stars(NULL),
                               names(NULL),
                               octreeRoot(NULL)
{
}

StarDatabase::~StarDatabase()
{
    if (stars != NULL)
	delete [] stars;

    for (int i = 0; i < Star::CatalogCount; i++)
    {
        if (catalogNumberIndexes[i] != NULL)
            delete [] catalogNumberIndexes[i];
    }
}


// Used to sort stars by catalog number
struct CatalogNumberPredicate
{
    int which;

    CatalogNumberPredicate(int _which) : which(_which) {};

    bool operator()(const Star* const & star0, const Star* const & star1) const
    {
        return (star0->getCatalogNumber(which) < star1->getCatalogNumber(which));
    }
};


Star* StarDatabase::find(uint32 catalogNumber, unsigned int whichCatalog) const
{
    // assert(whichCatalog < Star::CatalogCount);
    Star refStar;
    refStar.setCatalogNumber(whichCatalog, catalogNumber);

    CatalogNumberPredicate pred(whichCatalog);
    Star** star = lower_bound(catalogNumberIndexes[whichCatalog],
                              catalogNumberIndexes[whichCatalog] + nStars,
                              &refStar, pred);
    if (star != catalogNumberIndexes[whichCatalog] + nStars &&
        (*star)->getCatalogNumber(whichCatalog) == catalogNumber)
        return *star;
    else
        return NULL;
}


Star* StarDatabase::find(string name) const
{
    string altName;

    if (compareIgnoringCase(name, HDCatalogPrefix, HDCatalogPrefix.length()) == 0)
    {
        // Search by catalog number
        uint32 catalogNumber = (uint32) atoi(string(name, HDCatalogPrefix.length(),
                                                    string::npos).c_str());
        return find(catalogNumber, Star::HDCatalog);
    } else if (compareIgnoringCase(name, HIPPARCOSCatalogPrefix, HIPPARCOSCatalogPrefix.length()) == 0)
    {
        uint32 catalogNumber = (uint32) atoi(string(name, HIPPARCOSCatalogPrefix.length(), string::npos).c_str());
        return find(catalogNumber, Star::HIPCatalog);
    }
    else
    {
#if 0
        // Search through the catalog cross references
        {
            for (vector<CatalogCrossReference*>::const_iterator iter = catalogs->begin();
                 iter != catalogs->end(); iter++)
            {
                Star* star = (*iter)->lookup(name);
                if (star != NULL)
                    return star;
            }
        }
#endif

        if (names != NULL)
        {
            // See if the name is a Bayer or Flamsteed designation
            {
                string::size_type pos = name.find(' ');
                if (pos != 0 && pos != string::npos && pos < name.length() - 1)
                {
                    string prefix(name, 0, pos);
                    string conName(name, pos + 1, string::npos);
                    Constellation* con = Constellation::getConstellation(conName);
                    if (con != NULL)
                    {
                        char digit = ' ';
                        int len = prefix.length();

                        // If the first character of the prefix is a letter
                        // and the last character is a digit, we may have
                        // something like 'Alpha2 Cen' . . . Extract the digit
                        // before trying to match a Greek letter.
                        if (len > 2 &&
                            isalpha(prefix[0]) && isdigit(prefix[len - 1]))
                        {
                            len--;
                            digit = prefix[len];
                        }

                        // We have a valid constellation as the last part
                        // of the name.  Next, we see if the first part of
                        // the name is a greek letter.
                        const string& letter = Greek::canonicalAbbreviation(string(prefix, 0, len));
                        if (letter != "")
                        {
                            // Matched . . . this is a Bayer designation
                            if (digit == ' ')
                            {
                                name = letter + ' ' + con->getAbbreviation();
                                // If 'let con' doesn't match, try using
                                // 'let1 con' instead.
                                altName = letter + '1' + ' ' + con->getAbbreviation();
                            }
                            else
                            {
                                name = letter + digit + ' ' + con->getAbbreviation();
                            }
                        }
                        else
                        {
                            // Something other than a Bayer designation
                            name = prefix + ' ' + con->getAbbreviation();
                        }
                    }
                }
            }

            uint32 catalogNumber = names->findCatalogNumber(name);

            // If the first search failed, try using the alternate name
            if (catalogNumber == Star::InvalidCatalogNumber &&
                altName.length() != 0)
                catalogNumber = names->findCatalogNumber(altName);

            if (catalogNumber != Star::InvalidCatalogNumber)
                return find(catalogNumber, Star::HIPCatalog);
        }

        return NULL;
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
// If the star name is not present int the names database, a new
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
    if (star.getCatalogNumber(Star::HDCatalog) != Star::InvalidCatalogNumber)
        sprintf(buf, "HD %d", star.getCatalogNumber(Star::HDCatalog));
    else
        sprintf(buf, "HIP %d", catalogNumber);
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


void StarDatabase::addCrossReference(const CatalogCrossReference* xref)
{
    catalogs.insert(catalogs.end(), xref);
}


StarDatabase *StarDatabase::read(istream& in)
{
    StarDatabase *db = new StarDatabase();

    if (db == NULL)
	return NULL;

    uint32 nStars = 0;
    in.read((char *) &nStars, sizeof nStars);
    if (!in.good())
    {
        delete db;
        return NULL;
    };

    db->stars = new Star[nStars];
    if (db->stars == NULL)
    {
	delete db;
	return NULL;
    }

    uint32 throwOut = 0;
    uint32 fixUp = 0;

    while (db->nStars < nStars)
    {
	uint32 catNo = 0;
        uint32 hdCatNo = 0;
	float RA = 0, dec = 0, parallax = 0;
	int16 appMag;
	uint16 stellarClass;
	uint8 parallaxError;

	in.read((char *) &catNo, sizeof catNo);
        in.read((char *) &hdCatNo, sizeof hdCatNo);
	in.read((char *) &RA, sizeof RA);
	in.read((char *) &dec, sizeof dec);
	in.read((char *) &parallax, sizeof parallax);
	in.read((char *) &appMag, sizeof appMag);
	in.read((char *) &stellarClass, sizeof stellarClass);
	in.read((char *) &parallaxError, sizeof parallaxError);
	if (!in.good())
	    break;

	Star *star = &db->stars[db->nStars];

	// Compute distance based on parallax
	double distance = 3.26 / (parallax > 0.0 ? parallax / 1000.0 : 1e-6);
        star->setPosition(astro::equatorialToCelestialCart(RA, dec, (float) distance));

	// Use apparent magnitude and distance to determine the absolute
	// magnitude of the star.
	star->setAbsoluteMagnitude((float) (appMag / 256.0 + 5 -
					    5 * log10(distance / 3.26)));

	StellarClass sc((StellarClass::StarType) (stellarClass >> 12),
			(StellarClass::SpectralClass)(stellarClass >> 8 & 0xf),
			(unsigned int) (stellarClass >> 4 & 0xf),
			(StellarClass::LuminosityClass) (stellarClass & 0xf));
	star->setStellarClass(sc);

	star->setCatalogNumber(Star::HIPCatalog, catNo);
        star->setCatalogNumber(Star::HDCatalog, hdCatNo);

	// TODO: Use a photometric estimate of distance if parallaxError is
	// greater than 25%.
	if (parallaxError > 50)
        {
	    if (appMag / 256.0 > 6)
		throwOut++;
	    else
		fixUp++;
	}

	db->nStars++;
    }

    if (in.bad())
    {
	delete db;
	db = NULL;
    }

    cout << "nStars = " << db->nStars << '\n';

    db->buildOctree();
    db->buildIndexes();

    return db;
}


void StarDatabase::buildOctree()
{
    // This should only be called once for the database
    // ASSERT(octreeRoot == NULL);

    cout << "Sorting stars into octree . . .\n";
    cout.flush();
    float absMag = astro::appToAbsMag(OctreeMagnitude,
                                      OctreeRootSize * (float) sqrt(3.0));
    DynamicStarOctree* root = new DynamicStarOctree(Point3f(1000, 1000, 1000),
                                                    absMag);
    for (int i = 0; i < nStars; i++)
        root->insertStar(stars[i], OctreeRootSize);

    cout << "Spatially sorting stars for improved locality of reference . . .\n";
    cout.flush();
    Star* sortedStars = new Star[nStars];
    Star* firstStar = sortedStars;
    root->rebuildAndSort(octreeRoot, firstStar);

    // ASSERT((int) (firstStar - sortedStars) == nStars);
    cout << (int) (firstStar - sortedStars) << " stars total\n";
    cout << "Octree has " << 1 + octreeRoot->countChildren() << " nodes " <<
        " and " << octreeRoot->countStars() << " stars.\n";

    // Clean up . . .
    delete stars;
    delete root;

    stars = sortedStars;
}


void StarDatabase::buildIndexes()
{
    // This should only be called once for the database
    // assert(catalogNumberIndexes[0] == NULL);

    cout << "Building catalog number indexes . . .\n";
    cout.flush();

    for (int whichCatalog = 0;
         whichCatalog < sizeof(catalogNumberIndexes) / sizeof(catalogNumberIndexes[0]);
         whichCatalog++)
    {
        cout << "Sorting catalog number index #" << whichCatalog << '\n';
        catalogNumberIndexes[whichCatalog] = new Star*[nStars];
        for (int i = 0; i < nStars; i++)
            catalogNumberIndexes[whichCatalog][i] = &stars[i];

        CatalogNumberPredicate pred(whichCatalog);
        sort(catalogNumberIndexes[whichCatalog],
             catalogNumberIndexes[whichCatalog] + nStars, pred);
    }
}
