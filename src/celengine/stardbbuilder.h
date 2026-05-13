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

#include <filesystem>
#include <iosfwd>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <Eigen/Core>

#include <celutil/blockarray.h>
#include "astroobj.h"
#include "category.h"
#include "parseobject.h"
#include "star.h"
#include "stardb.h"
#include "starname.h"

class StarDatabase;

namespace celestia
{
namespace engine
{
class GeometryPaths;
class TexturePaths;
}
namespace ephem
{
class Orbit;
}

namespace util
{
class AssociativeArray;
}
}

class StarDatabaseBuilder
{
public:
    StarDatabaseBuilder(celestia::engine::GeometryPaths&,
                        celestia::engine::TexturePaths&);
    ~StarDatabaseBuilder();

    bool load(std::istream&,
              const std::filesystem::path& resourcePath);
    bool loadBinary(std::istream&);

    void setNameDatabase(std::unique_ptr<StarNameDatabase>&&);

    std::unique_ptr<StarDatabase> finish();

    struct StcHeader;

private:
    bool createOrUpdateStar(const StcHeader&,
                            const celestia::util::AssociativeArray*,
                            Star*,
                            const std::filesystem::path&);
    bool checkStcPosition(const StcHeader&,
                          const celestia::util::AssociativeArray*,
                          const Star*,
                          std::optional<Eigen::Vector3f>&,
                          std::optional<AstroCatalog::IndexNumber>&,
                          std::shared_ptr<const celestia::ephem::Orbit>&) const;
    bool checkBarycenter(const StarDatabaseBuilder::StcHeader&,
                         const celestia::util::AssociativeArray*,
                         std::optional<Eigen::Vector3f>&,
                         std::optional<AstroCatalog::IndexNumber>&) const;

    void loadCategories(const StarDatabaseBuilder::StcHeader&,
                        const celestia::util::AssociativeArray* starData,
                        const std::string&);
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
    std::map<AstroCatalog::IndexNumber, AstroCatalog::IndexNumber> barycenters;
    std::multimap<AstroCatalog::IndexNumber, UserCategoryId> categories;
    celestia::engine::GeometryPaths* geometryPaths;
    celestia::engine::TexturePaths* texturePaths;
};
