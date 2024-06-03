// stardbbuilder.h
//
// Copyright (C) 2001-2024, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <iosfwd>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <Eigen/Core>

#include <celcompat/filesystem.h>
#include <celutil/blockarray.h>
#include "astroobj.h"
#include "category.h"
#include "parseobject.h"
#include "star.h"
#include "stardb.h"
#include "starname.h"

class AssociativeArray;
class StarDatabase;

class StarDatabaseBuilder
{
public:
    StarDatabaseBuilder() = default;
    ~StarDatabaseBuilder();

    StarDatabaseBuilder(const StarDatabaseBuilder&) = delete;
    StarDatabaseBuilder& operator=(const StarDatabaseBuilder&) = delete;
    StarDatabaseBuilder(StarDatabaseBuilder&&) noexcept = delete;
    StarDatabaseBuilder& operator=(StarDatabaseBuilder&&) noexcept = delete;

    bool load(std::istream&, const fs::path& resourcePath = fs::path());
    bool loadBinary(std::istream&);

    void setNameDatabase(std::unique_ptr<StarNameDatabase>&&);

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
                    const AssociativeArray* starData,
                    const fs::path& path,
                    const bool isBarycenter);
    bool createOrUpdateStarDetails(Star* star,
                                   DataDisposition disposition,
                                   AstroCatalog::IndexNumber catalogNumber,
                                   const AssociativeArray* starData,
                                   const fs::path& path,
                                   const bool isBarycenter,
                                   std::optional<Eigen::Vector3f>& barycenterPosition);
    bool applyCustomStarDetails(const Star*,
                                AstroCatalog::IndexNumber,
                                const AssociativeArray*,
                                const fs::path&,
                                const CustomStarDetails&,
                                std::optional<Eigen::Vector3f>&);
    bool applyOrbit(AstroCatalog::IndexNumber catalogNumber,
                    const AssociativeArray* starData,
                    StarDetails* details,
                    const CustomStarDetails& customDetails,
                    std::optional<Eigen::Vector3f>& barycenterPosition);
    void loadCategories(AstroCatalog::IndexNumber catalogNumber,
                        const AssociativeArray *starData,
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

    BlockArray<Star> unsortedStars;
    // List of stars loaded from binary file, sorted by catalog number
    std::vector<Star*> binFileCatalogNumberIndex;
    // Catalog number -> star mapping for stars loaded from stc files
    std::map<AstroCatalog::IndexNumber, Star*> stcFileCatalogNumberIndex;
    std::vector<BarycenterUsage> barycenters;
    std::multimap<AstroCatalog::IndexNumber, UserCategoryId> categories;
};
