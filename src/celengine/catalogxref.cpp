// catalogxref.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cctype>
#include <algorithm>
#include <celutil/util.h>
#include "catalogxref.h"
#include "stardb.h"

using namespace std;


string CatalogCrossReference::getPrefix() const
{
    return prefix;
}

void CatalogCrossReference::setPrefix(const string& _prefix)
{
    prefix = _prefix;
}


bool operator<(const CatalogCrossReference::Entry& a,
               const CatalogCrossReference::Entry& b)
{
    return a.catalogNumber < b.catalogNumber;
}

struct XrefEntryPredicate
{
    int dummy{0};

    bool operator()(const CatalogCrossReference::Entry& a,
                    const CatalogCrossReference::Entry& b) const
    {
        return a.catalogNumber < b.catalogNumber;
    }
};


Star* CatalogCrossReference::lookup(uint32_t catalogNumber) const
{
    Entry e;
    e.catalogNumber = catalogNumber;
    e.star = nullptr;

    XrefEntryPredicate pred;
    vector<Entry>::const_iterator iter = lower_bound(entries.begin(),
                                                     entries.end(), e, pred);

    if (iter != entries.end() && iter->catalogNumber == catalogNumber)
        return iter->star;

    return nullptr;
}


Star* CatalogCrossReference::lookup(const string& name) const
{
    uint32_t catalogNumber = parse(name);
    if (catalogNumber == InvalidCatalogNumber)
        return nullptr;

    return lookup(catalogNumber);
}


uint32_t CatalogCrossReference::parse(const string& name) const
{
    if (compareIgnoringCase(name, prefix, prefix.length()) != 0)
        return InvalidCatalogNumber;

    unsigned int i = prefix.length();
    unsigned int n = 0;
    bool readDigit = false;

    // Optional space between prefix and number
    if (name[i] == ' ')
        i++;

    while (isdigit(name[i]))
    {
        n = n * 10 + ((unsigned int) name[i] - (unsigned int) '0');
        readDigit = true;

        // Limited to 24 bits
        if (n >= 0x1000000)
            return InvalidCatalogNumber;
    }

    // Must have read at least one digit
    if (!readDigit)
        return InvalidCatalogNumber;

    // Check for garbage at the end of the string
    if (i != prefix.length())
        return InvalidCatalogNumber;

    return n;
}


void CatalogCrossReference::addEntry(uint32_t catalogNumber, Star* star)
{
    Entry e;
    e.catalogNumber = catalogNumber;
    e.star = star;

    entries.insert(entries.end(), e);
}

void CatalogCrossReference::sortEntries()
{
    XrefEntryPredicate pred;
    sort(entries.begin(), entries.end(), pred);
}

void CatalogCrossReference::reserve(size_t n)
{
    if (n > entries.size())
        entries.reserve(n);
}


static uint32_t readUint32(istream& in)
{
    unsigned char b[4];
    in.read(reinterpret_cast<char*>(b), 4);
    return ((uint32_t) b[3] << 24) + ((uint32_t) b[2] << 16)
           + ((uint32_t) b[1] << 8) + (uint32_t) b[0];
}


CatalogCrossReference* ReadCatalogCrossReference(istream& in,
                                                 const StarDatabase& stardb)
{
    auto* xref = new CatalogCrossReference();

    uint32_t nEntries = readUint32(in);
    if (!in.good())
    {
        delete xref;
        return nullptr;
    }

    xref->reserve(nEntries);

    for (uint32_t i = 0; i < nEntries; i++)
    {
        uint32_t catNo1 = readUint32(in);
        uint32_t catNo2 = readUint32(in);
        Star* star = stardb.find(catNo2);
        if (star != nullptr)
            xref->addEntry(catNo1, star);
    }

    return xref;
}
