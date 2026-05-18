// loadsso.cpp
//
// Copyright (C) 2001-2023, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "loadsso.h"

#include <fstream>
#include <memory>

#include <celengine/frame.h>
#include <celengine/universe.h>
#include <celestia/configfile.h>
#include <celestia/progressnotifier.h>
#include <celestia/catalogloader.h>
#include <celutil/gettext.h>

namespace celestia
{

namespace
{

class SolarSystemLoader : public CatalogLoader
{
public:
    SolarSystemLoader(Universe* universe,
                      const std::string& typeDesc,
                      const ContentType& contentType,
                      ProgressNotifier* notifier,
                      util::array_view<std::filesystem::path> skipPaths,
                      engine::GeometryPaths& geometryPaths,
                      engine::TexturePaths& texturePaths,
                      FrameCache& frameCache) :
        CatalogLoader(typeDesc, contentType, notifier, skipPaths),
        m_universe(universe),
        m_geometryPaths(&geometryPaths),
        m_texturePaths(&texturePaths),
        m_frameCache(&frameCache)
    {
    }

    bool load(std::istream &in, const std::filesystem::path &dir) override
    {
        return LoadSolarSystemObjects(in, *m_universe, dir, *m_geometryPaths, *m_texturePaths, *m_frameCache);
    }

private:
    Universe* m_universe;
    engine::GeometryPaths* m_geometryPaths;
    engine::TexturePaths* m_texturePaths;
    FrameCache* m_frameCache;
};

} // end unnamed namespace

void
loadSSO(const CelestiaConfig &config,
        ProgressNotifier *progressNotifier,
        Universe *universe,
        engine::GeometryPaths& geometryPaths,
        engine::TexturePaths& texturePaths)
{
    auto solarSystem = std::make_unique<SolarSystemCatalog>();
    universe->setSolarSystemCatalog(std::move(solarSystem));

    // TRANSLATORS: this is a part of phrases "Loading {} catalog", "Skipping {} catalog"
    const char *typeDesc = C_("catalog", "solar system");

    FrameCache frameCache;

    SolarSystemLoader loader(universe,
                             typeDesc,
                             ContentType::CelestiaCatalog,
                             progressNotifier,
                             config.paths.skipExtras,
                             geometryPaths,
                             texturePaths,
                             frameCache);

    // First read the solar system files listed individually in the config file.
    std::filesystem::path empty;
    for (const auto &file : config.paths.solarSystemFiles)
        loader.process(file, empty);

    // Next, read all the solar system files in the extras directories
    loader.loadExtras(config.paths.extrasDirs);
}

} // namespace celestia
