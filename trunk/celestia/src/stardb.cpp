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


#define MAX_STARS 120000


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
                               catalogNumberIndex(NULL),
                               octreeRoot(NULL)
{
}

StarDatabase::~StarDatabase()
{
    if (stars != NULL)
	delete [] stars;
    if (catalogNumberIndex != NULL)
        delete [] catalogNumberIndex;
}


// Less than operator for stars is used to sort and find stars by catalog
// number
bool operator<(const StarRecord& a,
               const StarRecord& b)
{
    return a.star->getCatalogNumber() < b.star->getCatalogNumber();
}

bool operator<(StarRecord& s, uint32 n)
{
    return s.star->getCatalogNumber() < n;
}


Star* StarDatabase::find(uint32 catalogNumber) const
{
    StarRecord* star = lower_bound(catalogNumberIndex, catalogNumberIndex + nStars,
                                   catalogNumber);
    if (star != catalogNumberIndex + nStars &&
        star->star->getCatalogNumber() == catalogNumber)
        return star->star;
    else
        return NULL;                           
}


#if 0
bool startsWith(const string& s, const string& prefix)
{
#ifndef NONSTANDARD_STRING_COMPARE
    return s.compare(0, prefix.length(), prefix) == 0;
#else
    return s.compare(prefix, 0, prefix.length());
#endif // NONSTANDARD_STRING_COMPARE
}
#endif

Star* StarDatabase::find(string name) const
{
    if (compareIgnoringCase(name, HDCatalogPrefix, HDCatalogPrefix.length()) == 0)
    {
        // Search by catalog number
        uint32 catalogNumber = (uint32) atoi(string(name, HDCatalogPrefix.length(),
                                                    string::npos).c_str());
        return find(catalogNumber);
    }
    else if (compareIgnoringCase(name, HIPPARCOSCatalogPrefix, HIPPARCOSCatalogPrefix.length()) == 0)
    {
        uint32 catalogNumber = (uint32) atoi(string(name, HIPPARCOSCatalogPrefix.length(), string::npos).c_str()) | 0x10000000;
        return find(catalogNumber);
    }
    else
    {
        string conAbbrev;
        string designation;

        if (name.length() > 3)
        {
            conAbbrev = string(name, name.length() - 3, 3);
            designation = string(name, 0, name.length() - 4);
        }

        // Search the list of star proper names and designations
        for (StarNameDatabase::const_iterator iter = names->begin();
             iter != names->end(); iter++)
        {
            if (compareIgnoringCase(iter->second->getName(), name) == 0)
            {
                return find(iter->first);
            }
            else
            {
                Constellation* con = iter->second->getConstellation();
                if (con != NULL)
                {
                    if (compareIgnoringCase(con->getAbbreviation(), conAbbrev) == 0 &&
                        compareIgnoringCase(iter->second->getDesignation(), designation) == 0)
                    {
                        return find(iter->first);
                    }
                }
            }
        }

        return NULL;
    }
}


// Return the name for the star with specified catalog number.  The returned string
// will be:
//      the common name if it exists, otherwise
//      the Bayer or Flamsteed designation if it exists, otherwise
//      the HD catalog number if it exists, otherwise
//      the HIPPARCOS catalog number.
string StarDatabase::getStarName(uint32 catalogNumber) const
{
    StarNameDatabase::iterator iter = names->find(catalogNumber);
    if (iter != names->end())
    {
        StarName* starName = iter->second;

        if (starName->getName() != "")
            return starName->getName();

        Constellation* constellation = starName->getConstellation();
        if (constellation != NULL)
        {
            string name = starName->getDesignation();
            name += ' ';
            name += constellation->getGenitive();
            return name;
        }
    }

    char buf[20];
    switch ((catalogNumber & 0xf0000000) >> 28)
    {
    case 0:
        sprintf(buf, "HD %d", catalogNumber);
        break;
    case 1:
        sprintf(buf, "HIP %d", catalogNumber & 0x0fffffff);
        break;
    default:
        sprintf(buf, "? %d", catalogNumber & 0x0fffffff);
        break;
    }

    return string(buf);
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

    while (db->nStars < MAX_STARS)
    {
	uint32 catNo = 0;
	float RA = 0, dec = 0, parallax = 0;
	int16 appMag;
	uint16 stellarClass;
	uint8 parallaxError;

	in.read((char *) &catNo, sizeof catNo);
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

	star->setCatalogNumber(catNo);

	// Use a photometric estimate of distance if parallaxError is
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
                                      OctreeRootSize * (float) sqrt(3));
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
    // ASSERT(catalogNumberIndex == NULL);

    cout << "Building catalog number index . . .\n";
    cout.flush();

    catalogNumberIndex = new StarRecord[nStars];
    for (int i = 0; i < nStars; i++)
        catalogNumberIndex[i].star = &stars[i];
    sort(catalogNumberIndex, catalogNumberIndex + nStars);
}


StarNameDatabase* StarDatabase::readNames(istream& in)
{
    StarNameDatabase* db = new StarNameDatabase();
    bool failed = false;
    string s;

    for (;;)
    {
	unsigned int catalogNumber;
	char sep;

	in >> catalogNumber;
	if (in.eof())
	    break;
	if (in.bad())
        {
	    failed = true;
	    break;
	}

	in >> sep;
	if (sep != ':')
        {
	    failed = true;
	    break;
	}

	// Get the rest of the line
	getline(in, s);

	unsigned int nextSep = s.find_first_of(':');
	if (nextSep == string::npos)
        {
	    failed = true;
	    break;
	}

	string common, designation;
	string conAbbr;
	string conName;
	string bayerLetter;
	
	if (nextSep != 0)
	    common = s.substr(0, nextSep);
	designation = s.substr(nextSep + 1, string::npos);

	if (designation != "")
        {
	    nextSep = designation.find_last_of(' ');
	    if (nextSep != string::npos)
	    {
		bayerLetter = designation.substr(0, nextSep);
		conAbbr = designation.substr(nextSep + 1, string::npos);
	    }
	}

	Constellation *constel;
        
        if (designation != "")
        {
            for (int i = 0; i < 88; i++)
            {
                constel = Constellation::getConstellation(i);
                if (constel == NULL)
                {
                    DPRINTF("Error getting constellation %d", i);
                    break;
                }
                if (constel->getAbbreviation() == conAbbr)
                {
                    conName = constel->getName();
                    break;
                }
            }
        }
        else
        {
            constel = NULL;
        }

	StarName* sn = new StarName(common, bayerLetter, constel);

	db->insert(StarNameDatabase::value_type(catalogNumber, sn));
    }

    if (failed)
    {
	delete db;
	db = NULL;
    }

    return db;
}
