// loadstars.cpp
//
// Copyright (C) 2001-2023, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "loadstars.h"

#include <fstream>

#include <celcompat/filesystem.h>
#include <celengine/stardb.h>
#include <celengine/stardbbuilder.h>
#include <celestia/catalogloader.h>
#include <celestia/configfile.h>
#include <celestia/progressnotifier.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>

namespace celestia
{

using StarLoader = CatalogLoader<StarDatabaseBuilder>;

namespace
{

void
loadCrossIndex(StarNameDatabase& starNamesDB, StarCatalog catalog, const fs::path &filename)
{
    if (filename.empty())
        return;

    if (std::ifstream xrefFile(filename, std::ios::binary); xrefFile.good())
    {
        if (!starNamesDB.loadCrossIndex(catalog, xrefFile))
            util::GetLogger()->error(_("Error reading cross index {}\n"), filename);
        else
            util::GetLogger()->info(_("Loaded cross index {}\n"), filename);
    }
}

} // namespace

std::unique_ptr<StarDatabase>
loadStars(const CelestiaConfig &config, ProgressNotifier *progressNotifier)
{
    // First load the binary star database file. The majority of stars
    // will be defined here.
    StarDatabaseBuilder starDBBuilder;
    if (auto &path = config.paths.starDatabaseFile; !path.empty())
    {
        if (progressNotifier)
            progressNotifier->update(path.string());

        if (std::ifstream starFile(path, std::ios::binary); starFile.good())
        {
            if (!starDBBuilder.loadBinary(starFile))
            {
                util::GetLogger()->error(_("Error reading stars file\n"));
                return nullptr;
            }
        }
        else
        {
            util::GetLogger()->error(_("Error opening {}\n"), path);
            return nullptr;
        }
    }

    // Load star names
    std::unique_ptr<StarNameDatabase> starNameDB = nullptr;
    if (std::ifstream starNamesFile(config.paths.starNamesFile); starNamesFile.good())
    {
        starNameDB = StarNameDatabase::readNames(starNamesFile);
        if (starNameDB == nullptr)
            util::GetLogger()->error(_("Error reading star names file {}\n"),
                                     config.paths.starNamesFile);
    }
    else
    {
        util::GetLogger()->error(_("Error opening {}\n"), config.paths.starNamesFile);
    }

    if (starNameDB == nullptr)
        starNameDB = std::make_unique<StarNameDatabase>();

    loadCrossIndex(*starNameDB, StarCatalog::HenryDraper, config.paths.HDCrossIndexFile);
    loadCrossIndex(*starNameDB, StarCatalog::SAO, config.paths.SAOCrossIndexFile);

    starDBBuilder.setNameDatabase(std::move(starNameDB));

    // TRANSLATORS: this is a part of phrases "Loading {} catalog", "Skipping {} catalog"
    const char *typeDesc = C_("catalog", "star");

    StarLoader loader(&starDBBuilder,
                      typeDesc,
                      ContentType::CelestiaStarCatalog,
                      progressNotifier,
                      config.paths.skipExtras);

    // Next, read any ASCII star catalog files specified in the StarCatalogs list.
    fs::path empty;
    for (const auto &file : config.paths.starCatalogFiles)
        loader.process(file, empty);

    // Now, read supplemental star files from the extras directories
    loader.loadExtras(config.paths.extrasDirs);

    return starDBBuilder.finish();
}

} // namespace celestia
