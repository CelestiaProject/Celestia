// starname.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celutil/util.h>
#include "celestia.h"
#include "star.h"
#include "starname.h"

using namespace std;


static char *greekAlphabet[] =
{
    "Alpha",
    "Beta",
    "Gamma",
    "Delta",
    "Epsilon",
    "Zeta",
    "Eta",
    "Theta",
    "Iota",
    "Kappa",
    "Lambda",
    "Mu",
    "Nu",
    "Xi",
    "Omicron",
    "Pi",
    "Rho",
    "Sigma",
    "Tau",
    "Upsilon",
    "Phi",
    "Chi",
    "Psi",
    "Omega"
};

static char* canonicalAbbrevs[] =
{
    "ALF", "BET", "GAM", "DEL", "EPS", "ZET", "ETA", "TET",
    "IOT", "KAP", "LAM", "MU" , "NU" , "XI" , "OMI", "PI" ,
    "RHO", "SIG", "TAU", "UPS", "PHI", "CHI", "PSI", "OME",
};

static string noAbbrev("");


StarNameDatabase::StarNameDatabase()
{
}

void StarNameDatabase::add(uint32 catalogNumber, const string& name)
{
    nameIndex.insert(NameIndex::value_type(name, catalogNumber));
    numberIndex.insert(NumberIndex::value_type(catalogNumber, name));
}


uint32 StarNameDatabase::findCatalogNumber(const string& name)
{
    NameIndex::const_iterator iter = nameIndex.find(name);
    if (iter == nameIndex.end())
        return Star::InvalidCatalogNumber;
    else
        return iter->second;
}


// Return the first name matching the catalog number or finalName()
// if there are no matching names.  The first name *should* be the
// proper name of the star, if one exists.  This requires that the
// star name database file to have the proper names listed before
// Bayer and other designations.  Also, the STL implementation must
// preserve this order when inserting the names into the multimap
// (not certain whether or not this is behavior is in the STL spec.,
// but it works on the implementations I've tried so far.)
StarNameDatabase::NumberIndex::const_iterator
StarNameDatabase::findFirstName(uint32 catalogNumber) const
{
    NumberIndex::const_iterator iter = numberIndex.lower_bound(catalogNumber);
    if (iter == numberIndex.end() || iter->first != catalogNumber)
        return finalName();
    else
        return iter;
}


StarNameDatabase::NumberIndex::const_iterator
StarNameDatabase::finalName() const
{
    return numberIndex.end();
}


StarNameDatabase* StarNameDatabase::readNames(istream& in)
{
    StarNameDatabase* db = new StarNameDatabase();
    bool failed = false;
    string s;

    while (!failed)
    {
        uint32 catalogNumber = Star::InvalidCatalogNumber;

	in >> catalogNumber;
	if (in.eof())
	    break;
	if (in.bad())
        {
	    failed = true;
	    break;
	}

        in.get(); // skip a space

        string name;
        getline(in, name);
        if (in.bad())
        {
            failed = true;
            break;
        }

        db->add(catalogNumber, name);
    }

    if (failed)
    {
        delete db;
        return NULL;
    }
    else
    {
        return db;
    }
}


// Greek alphabet crud . . . should probably moved to it's own module.

Greek* Greek::instance = NULL;

Greek::Greek()
{
    nLetters = sizeof(greekAlphabet) / sizeof(greekAlphabet[0]);
    names = new string[nLetters];
    abbrevs = new string[nLetters];

    for (int i = 0; i < nLetters; i++)
    {
        names[i] = string(greekAlphabet[i]);
        abbrevs[i] = string(canonicalAbbrevs[i]);
    }
}

Greek::~Greek()
{
    delete[] names;
    delete[] abbrevs;
}

const string& Greek::canonicalAbbreviation(const std::string& letter)
{
    if (instance == NULL)
        instance = new Greek();

    int i;
    for (i = 0; i < Greek::instance->nLetters; i++)
    {
        if (compareIgnoringCase(letter, instance->names[i]) == 0)
            return instance->abbrevs[i];
    }

    for (i = 0; i < Greek::instance->nLetters; i++)
    {
        if (compareIgnoringCase(letter, instance->abbrevs[i]) == 0)
            return instance->abbrevs[i];
    }

    return noAbbrev;
}
