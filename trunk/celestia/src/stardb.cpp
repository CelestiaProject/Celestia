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
#include "stardb.h"

using namespace std;


#define MAX_STARS 120000


static string HDCatalogPrefix("HD ");
static string HIPPARCOSCatalogPrefix("HIP ");
static string GlieseCatalogPrefix("Gliese ");
static string RossCatalogPrefix("Ross ");
static string LacailleCatalogPrefix("Lacaille ");


StarDatabase::StarDatabase() : nStars(0), stars(NULL), names(NULL)
{
}

StarDatabase::~StarDatabase()
{
    if (stars != NULL)
	delete [] stars;
}

// Less than operator for stars is used to sort and find stars by catalog
// number
bool operator<(Star& a, Star& b)
{
    return a.getCatalogNumber() < b.getCatalogNumber();
}

bool operator<(Star& s, uint32 n)
{
    return s.getCatalogNumber() < n;
}


Star* StarDatabase::find(uint32 catalogNumber) const
{
    Star* star = lower_bound(stars, stars + nStars, catalogNumber);
    if (star != stars + nStars && star->getCatalogNumber() == catalogNumber)
        return star;
    else
        return NULL;
}


Star* StarDatabase::find(string name) const
{
    if (name.compare(0, HDCatalogPrefix.length(), HDCatalogPrefix) == 0)
    {
        // Search by catalog number
        uint32 catalogNumber = (uint32) atoi(string(name, HDCatalogPrefix.length(),
                                                    string::npos).c_str());
        return find(catalogNumber);
    }
    else if (name.compare(0, HIPPARCOSCatalogPrefix.length(), HIPPARCOSCatalogPrefix) == 0)
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
            if (iter->second->getName() == name)
            {
                return find(iter->first);
            }
            else
            {
                Constellation* con = iter->second->getConstellation();
                if (con != NULL)
                {
                    if (con->getAbbreviation() == conAbbrev &&
                        iter->second->getDesignation() == designation)
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

    Mat3f equatorialToEcliptical = Mat3f::xrotation(degToRad(-23.4392911));

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

	// Convert from RA, dec spherical to cartesian coordinates
	double theta = RA / 24.0 * PI * 2;
	// double phi = (1.0 - dec / 90.0) * PI / 2;
        double phi = (dec / 90.0 - 1.0) * PI / 2;
	double x = -cos(theta) * sin(phi) * distance;
	double y = -cos(phi) * distance;
	double z = -sin(theta) * sin(phi) * distance;
	star->setPosition(Point3f((float) x, (float) y, (float) z) *
                          equatorialToEcliptical);

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

    sort(db->stars, db->stars + db->nStars);

    cout << "nStars = " << db->nStars << '\n';
    cout << "throw out = " << throwOut << '\n';
    cout << "fix up = " << fixUp << '\n';

    return db;
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
