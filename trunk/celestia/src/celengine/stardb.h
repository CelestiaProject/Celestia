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
#include <celengine/constellation.h>
#include <celengine/starname.h>
#include <celengine/star.h>
#include <celengine/octree.h>


class StarDatabase
{        
 public:
    StarDatabase();
    ~StarDatabase();

    inline Star* getStar(uint32) const;
    inline uint32 size() const;
    Star* find(uint32 catalogNumber) const;
    Star* find(const std::string&) const;
    std::vector<std::string> getCompletion(const std::string&) const;

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
    
    bool load(std::istream&, const std::string& resourcePath);
    bool loadBinary(std::istream&);
    bool loadOldFormatBinary(std::istream&);

    
    enum Catalog
    {
        HenryDraper = 0,
        Gliese      = 1,
        SAO         = 2,
        MaxCatalog  = 3,
    };

    struct CrossIndexEntry
    {
        uint32 catalogNumber;
        uint32 celCatalogNumber;
    };

    typedef std::vector<CrossIndexEntry> CrossIndex;
    bool loadCrossIndex(Catalog, std::istream&);
    Star* searchCrossIndex(Catalog catalog, uint32 number) const;
    uint32 crossIndex(Catalog catalog, uint32 number) const;

    void finish();

    static StarDatabase* read(std::istream&);

    static const char* FileHeader;
    static const char* CrossIndexFileHeader;

 private:
    void buildOctree();
    void buildIndexes();

    int nStars;
    int capacity;
    Star* stars;
    StarNameDatabase* names;
    Star** catalogNumberIndex;
    StarOctree* octreeRoot;

    std::vector<CrossIndex*> crossIndexes;
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
