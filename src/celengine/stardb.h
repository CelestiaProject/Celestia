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
#include <iosfwd>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celcompat/filesystem.h>
#include <celengine/parseobject.h>
#include <celutil/blockarray.h>
#include "astroobj.h"
#include "hash.h"
#include "staroctree.h"


class StarNameDatabase;
class UserCategory;


constexpr inline unsigned int MAX_STAR_NAMES = 10;

enum class StarCatalog
{
    HenryDraper = 0,
    Gliese      = 1,
    SAO         = 2,
    MaxCatalog  = 3,
};


class StarDatabaseBuilder;

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

    void getCompletion(std::vector<std::string>&, std::string_view, bool i18n) const;

    void findVisibleStars(StarHandler& starHandler,
                          const Eigen::Vector3f& obsPosition,
                          const Eigen::Quaternionf&   obsOrientation,
                          float fovY,
                          float aspectRatio,
                          float limitingMag,
                          OctreeProcStats * = nullptr) const;

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
    std::unique_ptr<StarNameDatabase> namesDB{ nullptr };
    std::vector<Star*>                catalogNumberIndex{ };
    StarOctree*                       octreeRoot{ nullptr };

    std::vector<CrossIndex> crossIndexes;

    friend class StarDatabaseBuilder;
};


inline Star* StarDatabase::getStar(const std::uint32_t n) const
{
    return stars + n;
}

inline std::uint32_t StarDatabase::size() const
{
    return nStars;
}


inline bool operator<(const StarDatabase::CrossIndexEntry& lhs, const StarDatabase::CrossIndexEntry& rhs)
{
    return lhs.catalogNumber < rhs.catalogNumber;
}


class StarDatabaseBuilder
{
 public:
    StarDatabaseBuilder() = default;
    ~StarDatabaseBuilder() = default;

    StarDatabaseBuilder(const StarDatabaseBuilder&) = delete;
    StarDatabaseBuilder& operator=(const StarDatabaseBuilder&) = delete;
    StarDatabaseBuilder(StarDatabaseBuilder&&) noexcept = delete;
    StarDatabaseBuilder& operator=(StarDatabaseBuilder&&) noexcept = delete;

    bool load(std::istream&, const fs::path& resourcePath = fs::path());
    bool loadBinary(std::istream&);

    void setNameDatabase(std::unique_ptr<StarNameDatabase>&&);
    bool loadCrossIndex(StarCatalog, std::istream&);

    std::unique_ptr<StarDatabase> finish();

    struct CustomStarDetails;

 private:
    struct BarycenterUsage
    {
        AstroCatalog::IndexNumber catNo;
        AstroCatalog::IndexNumber barycenterCatNo;
    };

    bool createStar(Star* star,
                    DataDisposition disposition,
                    AstroCatalog::IndexNumber catalogNumber,
                    const Hash* starData,
                    const fs::path& path,
                    const bool isBarycenter);
    bool createOrUpdateStarDetails(Star* star,
                                   DataDisposition disposition,
                                   AstroCatalog::IndexNumber catalogNumber,
                                   const Hash* starData,
                                   const fs::path& path,
                                   const bool isBarycenter,
                                   std::optional<Eigen::Vector3f>& barycenterPosition);
    bool applyCustomStarDetails(const Star*,
                                AstroCatalog::IndexNumber,
                                const Hash*,
                                const fs::path&,
                                const CustomStarDetails&,
                                std::optional<Eigen::Vector3f>&);
    bool applyOrbit(AstroCatalog::IndexNumber catalogNumber,
                    const Hash* starData,
                    StarDetails* details,
                    const CustomStarDetails& customDetails,
                    std::optional<Eigen::Vector3f>& barycenterPosition);
    void loadCategories(AstroCatalog::IndexNumber catalogNumber,
                        const Hash *starData,
                        DataDisposition disposition,
                        const std::string &domain);
    void addCategory(AstroCatalog::IndexNumber catalogNumber,
                     const std::string& name,
                     const std::string& domain);

    void buildOctree();
    void buildIndexes();
    Star* findWhileLoading(AstroCatalog::IndexNumber catalogNumber) const;

    std::unique_ptr<StarDatabase> starDB{ std::make_unique<StarDatabase>() };

    AstroCatalog::IndexNumber nextAutoCatalogNumber{ 0xfffffffe };

    BlockArray<Star> unsortedStars{ };
    // List of stars loaded from binary file, sorted by catalog number
    std::vector<Star*> binFileCatalogNumberIndex{ nullptr };
    // Catalog number -> star mapping for stars loaded from stc files
    std::map<AstroCatalog::IndexNumber, Star*> stcFileCatalogNumberIndex{};
    std::vector<BarycenterUsage> barycenters{};
    std::multimap<AstroCatalog::IndexNumber, UserCategory*> categories{};
};
