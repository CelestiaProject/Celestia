// stardb.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _STARDB_H_
#define _STARDB_H_

#include <iostream>
#include <map>
#include "constellation.h"
#include "starname.h"
#include "star.h"


typedef std::map<uint32, StarName*> StarNameDatabase;

class StarDatabase
{
public:
    static StarDatabase* read(std::istream&);
    static StarNameDatabase* readNames(std::istream&);

    StarDatabase();
    ~StarDatabase();

    inline Star* getStar(uint32) const;
    inline uint32 size() const;
    Star* find(uint32 catalogNumber) const;
    Star* find(std::string) const;

    string getStarName(uint32 catalogNumber) const;

    StarNameDatabase* getNameDatabase() const;
    void setNameDatabase(StarNameDatabase*);

private:
    int nStars;
    Star *stars;
    StarNameDatabase* names;
};


Star* StarDatabase::getStar(uint32 n) const
{
    return stars + n;
}

uint32 StarDatabase::size() const
{
    return nStars;
}


#if 0
bool GetCatalogNumber(std::string name,
                      const StarNameDatabase& namedb,
                      uint32& catalogNumber);
#endif

#endif // _STARDB_H_
