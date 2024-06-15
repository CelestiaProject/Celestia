// stardb.h
//
// Copyright (C) 2001-2024, the Celestia Development Team
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
#include "starname.h"
#include "staroctree.h"

class Star;
class StarDatabaseBuilder;

class StarDatabase
{
public:
    // The size of the root star octree node is also the maximum distance
    // distance from the Sun at which any star may be located. The current
    // setting of 1.0e7 light years is large enough to contain the entire
    // local group of galaxies. A larger value should be OK, but the
    // performance implications for octree traversal still need to be
    // investigated.
    static constexpr float STAR_OCTREE_ROOT_SIZE = 1000000000.0f;

    // Not exact, but any star with a catalog number greater than this is assumed to not be
    // a HIPPARCOS stars.
    static constexpr AstroCatalog::IndexNumber MAX_HIPPARCOS_NUMBER = 999999;

    static constexpr unsigned int MAX_STAR_NAMES = 10;

    StarDatabase();
    ~StarDatabase();

    Star* getStar(std::uint32_t);
    const Star* getStar(std::uint32_t) const;
    std::uint32_t size() const;

    const celestia::engine::StarOctree& getOctree() const;

    Star* find(AstroCatalog::IndexNumber catalogNumber);
    const Star* find(AstroCatalog::IndexNumber catalogNumber) const;
    Star* find(std::string_view, bool i18n);
    const Star* find(std::string_view, bool i18n) const;

    void getCompletion(std::vector<std::string>&, std::string_view) const;

/*
    void findVisibleStars(StarHandler& starHandler,
                          const Eigen::Vector3f& obsPosition,
                          const Eigen::Quaternionf& obsOrientation,
                          float fovY,
                          float aspectRatio,
                          float limitingMag) const;

    void findCloseStars(StarHandler& starHandler,
                        const Eigen::Vector3f& obsPosition,
                        float radius) const;
*/

    std::string getStarName(const Star&, bool i18n = false) const;
    std::string getStarNameList(const Star&, unsigned int maxNames = MAX_STAR_NAMES) const;

    const StarNameDatabase* getNameDatabase() const;

private:
    celestia::engine::StarOctree      octree;
    std::unique_ptr<StarNameDatabase> namesDB;
    std::vector<std::uint32_t>        catalogNumberIndex;

    friend class StarDatabaseBuilder;
};

inline Star*
StarDatabase::getStar(std::uint32_t n)
{
    return &octree[n];
}

inline const Star*
StarDatabase::getStar(std::uint32_t n) const
{
    return &octree[n];
}

inline std::uint32_t
StarDatabase::size() const
{
    return octree.size();
}

inline const celestia::engine::StarOctree&
StarDatabase::getOctree() const
{
    return octree;
}
