// loaddso.cpp
//
// Copyright (C) 2001-2023, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "loaddso.h"

#include <fstream>

#include <celengine/dsodb.h>
#include <celengine/dsodbbuilder.h>
#include <celestia/catalogloader.h>
#include <celestia/configfile.h>
#include <celestia/progressnotifier.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>

namespace celestia
{
namespace
{

class DeepSkyLoader : public CatalogLoader
{
public:
    DeepSkyLoader(DSODatabaseBuilder& db,
                  const std::string& typeDesc,
                  const ContentType& contentType,
                  ProgressNotifier* notifier,
                  util::array_view<std::filesystem::path> skipPaths,
                  engine::GeometryPaths& geometryPaths,
                  engine::TexturePaths& texturePaths) :
        CatalogLoader(typeDesc, contentType, notifier, skipPaths),
        m_db(&db),
        m_geometryPaths(&geometryPaths),
        m_texturePaths(&texturePaths)
    {
    }

    bool load(std::istream &in, const std::filesystem::path &dir) override
    {
        return m_db->load(in, dir);
    }

private:
    DSODatabaseBuilder* m_db;
    engine::GeometryPaths* m_geometryPaths;
    engine::TexturePaths* m_texturePaths;
};

}

std::unique_ptr<DSODatabase>
loadDSO(const CelestiaConfig& config,
        ProgressNotifier* progressNotifier,
        engine::GeometryPaths& geometryPaths,
        engine::TexturePaths& texturePaths)
{
    auto dsoDB = std::make_unique<DSODatabaseBuilder>(geometryPaths);

    // TRANSLATORS: this is a part of phrases "Loading {} catalog", "Skipping {} catalog"
    const char *typeDesc = C_("catalog", "deep sky");

    DeepSkyLoader loader(*dsoDB,
                         typeDesc,
                         ContentType::CelestiaDeepSkyCatalog,
                         progressNotifier,
                         config.paths.skipExtras,
                         geometryPaths,
                         texturePaths);

    // Load first the vector of dsoCatalogFiles in the data directory (deepsky.dsc,
    // globulars.dsc, ...):
    std::filesystem::path empty;
    for (const auto &file : config.paths.dsoCatalogFiles)
        loader.process(file, empty);

    // Next, read all the deep sky files in the extras directories
    loader.loadExtras(config.paths.extrasDirs);

    return dsoDB->finish();
}

} // namespace celestia
