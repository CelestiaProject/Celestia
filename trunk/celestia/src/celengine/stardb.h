// stardb.h
//
// Copyright (C) 2001-2008, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_STARDB_H_
#define _CELENGINE_STARDB_H_

#include <iostream>
#include <vector>
#include <celengine/constellation.h>
#include <celengine/starname.h>
#include <celengine/star.h>
#include <celengine/staroctree.h>
#include <celengine/parser.h>


static const unsigned int MAX_STAR_NAMES = 10;


class StarDatabase
{        
 public:
    StarDatabase();
    ~StarDatabase();


    inline Star*  getStar(const uint32) const;
    inline uint32 size() const;

    Star* find(const uint32 catalogNumber) const;
    Star* find(const std::string&) const;
    uint32 findCatalogNumberByName(const std::string&) const;

    std::vector<std::string> getCompletion(const std::string&) const;

    void findVisibleStars(StarHandler& starHandler,
                          const Point3f& obsPosition,
                          const Quatf&   obsOrientation,
                          float fovY,
                          float aspectRatio,
                          float limitingMag) const;

    void findCloseStars(StarHandler& starHandler,
                        const Point3f& obsPosition,
                        float radius) const;

    std::string getStarName    (const Star&) const;
    void getStarName(const Star& star, char* nameBuffer, unsigned int bufferSize) const;
    std::string getStarNameList(const Star&, const unsigned int maxNames = MAX_STAR_NAMES) const;

    StarNameDatabase* getNameDatabase() const;
    void setNameDatabase(StarNameDatabase*);
    
    bool load(std::istream&, const std::string& resourcePath);
    bool loadBinary(std::istream&);
    bool loadOldFormatBinary(std::istream&);

    Star* createStar(const uint32 catalogNumber,
                     Hash* starData,
                     const std::string& path,
                     const bool isBarycenter);

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

    bool   loadCrossIndex  (const Catalog, std::istream&);
    uint32 searchCrossIndexForCatalogNumber(const Catalog, const uint32 number) const;
    Star*  searchCrossIndex(const Catalog, const uint32 number) const;
    uint32 crossIndex      (const Catalog, const uint32 number) const;

    void finish();

    static StarDatabase* read(std::istream&);

    static const char* FILE_HEADER;
    static const char* CROSSINDEX_FILE_HEADER;

 private:
    void buildOctree();
    void buildIndexes();

    int nStars;
    int capacity;
    Star* stars;
    StarNameDatabase* namesDB;
    Star** catalogNumberIndex;
    StarOctree* octreeRoot;
    uint32            nextAutoCatalogNumber;

    std::vector<CrossIndex*> crossIndexes;


    struct BarycenterUsage
    {
        uint32 catNo;
        uint32 barycenterCatNo;
    };
    std::vector<BarycenterUsage> barycenters;
};


Star* StarDatabase::getStar(const uint32 n) const
{
    return stars + n;
}

uint32 StarDatabase::size() const
{
    return nStars;
}

#endif // _CELENGINE_STARDB_H_
