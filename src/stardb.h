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
#include <vector>
#include "constellation.h"
#include "starname.h"
#include "star.h"
#include "catalogxref.h"
#include "octree.h"


class StarDatabase
{        
 public:
    StarDatabase();
    ~StarDatabase();

    inline Star* getStar(uint32) const;
    inline uint32 size() const;
    Star* find(uint32 catalogNumber, unsigned int whichCatalog = 0) const;
    Star* find(std::string) const;

    void findVisibleStars(StarHandler& starHandler,
                          const Point3f& position,
                          const Quatf& orientation,
                          float fovY,
                          float aspectRatio,
                          float limitingMag) const;
    void findCloseStars(StarHandler& starHandler,
                        const Point3f& position,
                        float radius) const;

    std::string getStarName(const Star&) const;
    StarNameDatabase::NumberIndex::const_iterator 
        getStarNames(uint32 catalogNumber) const;
    StarNameDatabase::NumberIndex::const_iterator finalName() const;

    StarNameDatabase* getNameDatabase() const;
    void setNameDatabase(StarNameDatabase*);
    
    void addCrossReference(const CatalogCrossReference*);

    static StarDatabase* read(std::istream&);

 private:
    void buildOctree();
    void buildIndexes();

    int nStars;
    Star* stars;
    StarNameDatabase* names;
    Star** catalogNumberIndexes[Star::CatalogCount];
    StarOctree* octreeRoot;

    std::vector<const CatalogCrossReference*> catalogs;
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
