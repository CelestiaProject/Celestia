// stardb.h
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "astroobj.h"
#include "staroctree.h"

class Star;
class StarNameDatabase;
class UserCategory;

namespace celestia::engine
{

class StarDatabaseBuilder;

constexpr inline unsigned int MAX_STAR_NAMES = 10;

enum class StarCatalog : unsigned int
{
    HenryDraper = 0,
    Gliese      = 1,
    SAO         = 2,
    MaxCatalog  = 3,
};

class StarDatabase
{
public:
    StarDatabase();
    ~StarDatabase();

    inline Star* getStar(const std::uint32_t) const;
    inline std::uint32_t size() const;

    Star* find(AstroCatalog::IndexNumber catalogNumber) const;
    Star* find(std::string_view, bool i18n) const;
    AstroCatalog::IndexNumber findCatalogNumberByName(std::string_view, bool i18n) const;

    void getCompletion(std::vector<std::string>&, std::string_view) const;

    void findVisibleStars(StarHandler& starHandler,
                          const Eigen::Vector3f& obsPosition,
                          const Eigen::Quaternionf&   obsOrientation,
                          float fovY,
                          float aspectRatio,
                          float limitingMag) const;

    void findCloseStars(StarHandler& starHandler,
                        const Eigen::Vector3f& obsPosition,
                        float radius) const;

    std::string getStarName(const Star&, bool i18n = false) const;
    std::string getStarNameList(const Star&, const unsigned int maxNames = MAX_STAR_NAMES) const;

    StarNameDatabase* getNameDatabase() const;

    // Not exact, but any star with a catalog number greater than this is assumed to not be
    // a HIPPARCOS stars.
    static constexpr AstroCatalog::IndexNumber MAX_HIPPARCOS_NUMBER = 999999;

    struct CrossIndexEntry
    {
        AstroCatalog::IndexNumber catalogNumber;
        AstroCatalog::IndexNumber celCatalogNumber;
    };

    using CrossIndex = std::vector<CrossIndexEntry>;

    AstroCatalog::IndexNumber searchCrossIndexForCatalogNumber(StarCatalog, AstroCatalog::IndexNumber number) const;
    Star* searchCrossIndex(StarCatalog, AstroCatalog::IndexNumber number) const;
    AstroCatalog::IndexNumber crossIndex(StarCatalog, AstroCatalog::IndexNumber number) const;

private:
    std::uint32_t nStars{ 0 };

    Star*                             stars{ nullptr };
    std::unique_ptr<StarNameDatabase> namesDB;
    std::vector<Star*>                catalogNumberIndex{ };
    StarOctree*                       octreeRoot{ nullptr };

    std::vector<CrossIndex> crossIndexes;

    friend class StarDatabaseBuilder;
};

inline Star*
StarDatabase::getStar(const std::uint32_t n) const
{
    return stars + n;
}

inline std::uint32_t
StarDatabase::size() const
{
    return nStars;
}

inline bool
operator<(const StarDatabase::CrossIndexEntry& lhs, const StarDatabase::CrossIndexEntry& rhs)
{
    return lhs.catalogNumber < rhs.catalogNumber;
}

} // end namespace celestia::engine
