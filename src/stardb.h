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
#include "octree.h"


typedef std::map<uint32, StarName*> StarNameDatabase;

struct StarRecord
{
    Star* star;
};

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

    void processVisibleStars(StarHandler& starHandler,
                             const Point3f& position,
                             const Quatf& orientation,
                             float fovY,
                             float aspectRatio,
                             float limitingMag) const;

    string getStarName(uint32 catalogNumber) const;

    StarNameDatabase* getNameDatabase() const;
    void setNameDatabase(StarNameDatabase*);

 private:
    void buildOctree();
    void buildIndexes();

    int nStars;
    Star *stars;
    StarNameDatabase* names;
    StarRecord* catalogNumberIndex;
    StarOctree* octreeRoot;
};


Star* StarDatabase::getStar(uint32 n) const
{
    return stars + n;
}

uint32 StarDatabase::size() const
{
    return nStars;
}

#endif // _STARDB_H_
