// starname.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <string>

#ifndef _WIN32
#ifndef MACOSX
#include <config.h>
#endif /* ! MACOSX */
#endif /* !_WIN32 */

#include <celutil/util.h>
#include <celutil/debug.h>
#include <celutil/utf8.h>
#include "celestia.h"
#include "star.h"
#include "starname.h"

using namespace std;


static const char *greekAlphabet[] =
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

static const char* greekAlphabetUTF8[] =
{
    "\316\261",
    "\316\262",
    "\316\263",
    "\316\264",
    "\316\265",
    "\316\266",
    "\316\267",
    "\316\270",
    "\316\271",
    "\316\272",
    "\316\273",
    "\316\274",
    "\316\275",
    "\316\276",
    "\316\277",
    "\317\200",
    "\317\201",
    "\317\203",
    "\317\204",
    "\317\205",
    "\317\206",
    "\317\207",
    "\317\210",
    "\317\211",
};

static const char* canonicalAbbrevs[] =
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
    if(name.length() != 0)
    {
#ifdef DEBUG
        uint32 tmp;
        if((tmp=findName(name))!=Star::InvalidCatalogNumber)
            DPRINTF(2,"Duplicated name '%s' on HIP %d and %d\n", name.c_str(),
                    tmp, catalogNumber);
#endif
        nameIndex.insert(NameIndex::value_type(name, catalogNumber));
        numberIndex.insert(NumberIndex::value_type(catalogNumber, name));
    }
}


uint32 StarNameDatabase::findCatalogNumber(const string& name) const
{
    NameIndex::const_iterator iter = nameIndex.find(name);
    if (iter == nameIndex.end())
        return Star::InvalidCatalogNumber;
    else
        return iter->second;
}

std::vector<std::string> StarNameDatabase::getCompletion(const std::string& name) const
{
    std::vector<std::string> completion;
    for (NameIndex::const_iterator iter = nameIndex.begin();
         iter != nameIndex.end() ; iter++)
    {
        if (!UTF8StringCompare(iter->first, name, name.length()))
        {
            completion.push_back(iter->first);
        }
    }
    return completion;
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

        // in.get(); // skip a space (or colon);

        string name;
        getline(in, name);
        if (in.bad())
        {
            failed = true;
            break;
        }

        // Iterate through the string for names delimited
        // by ':', and insert them into the star database. Note that
        // db->add() will skip empty names.
        string::size_type startPos = 0; 
        while (startPos != string::npos)
        {
            startPos++;
            string::size_type next = name.find(':', startPos);
            string::size_type length = string::npos;
            if (next != string::npos)
                length = next - startPos;
            db->add(catalogNumber, name.substr(startPos, length));
            startPos = next;
        }
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


uint32 StarNameDatabase::findName(string name) const
{
    string priName = name;
    string altName;
    // See if the name is a Bayer or Flamsteed designation
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
                    priName = letter + ' ' + con->getAbbreviation();
                    // If 'let con' doesn't match, try using
                    // 'let1 con' instead.
                    altName = letter + '1' + ' ' + con->getAbbreviation();
                }
                else
                {
                    priName = letter + digit + ' ' + con->getAbbreviation();
                }
            }
            else
            {
                // Something other than a Bayer designation
                priName = prefix + ' ' + con->getAbbreviation();
            }
        }
    }

    uint32 catalogNumber = findCatalogNumber(priName);
    if (catalogNumber != Star::InvalidCatalogNumber)
        return catalogNumber;
    priName += " A";  // try by appending an A
    catalogNumber = findCatalogNumber(priName);
    if (catalogNumber != Star::InvalidCatalogNumber)
        return catalogNumber;

    // If the first search failed, try using the alternate name
    if (altName.length() != 0)
    {
        catalogNumber = findCatalogNumber(altName);
        if (catalogNumber == Star::InvalidCatalogNumber)
        {
            altName += " A";
            catalogNumber = findCatalogNumber(altName);
        }   // Intentional fallthrough.
    }
    return catalogNumber;
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

    if (letter.length() == 2)
    {
        for (i = 0; i < Greek::instance->nLetters; i++)
        {
            if (letter[0] == greekAlphabetUTF8[i][0] &&
                letter[1] == greekAlphabetUTF8[i][1])
            {
                return instance->abbrevs[i];
            }
        }
    }

    return noAbbrev;
}
