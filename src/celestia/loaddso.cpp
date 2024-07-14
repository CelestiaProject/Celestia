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
using DeepSkyLoader = CatalogLoader<DSODatabaseBuilder>;

std::unique_ptr<DSODatabase>
loadDSO(const CelestiaConfig &config, ProgressNotifier *progressNotifier)
{
    auto dsoDB = std::make_unique<DSODatabaseBuilder>();

    // TRANSLATORS: this is a part of phrases "Loading {} catalog", "Skipping {} catalog"
    const char *typeDesc = C_("catalog", "deep sky");

    DeepSkyLoader loader(dsoDB.get(),
                         typeDesc,
                         ContentType::CelestiaDeepSkyCatalog,
                         progressNotifier,
                         config.paths.skipExtras);

    // Load first the vector of dsoCatalogFiles in the data directory (deepsky.dsc,
    // globulars.dsc, ...):
    fs::path empty;
    for (const auto &file : config.paths.dsoCatalogFiles)
        loader.process(file, empty);

    // Next, read all the deep sky files in the extras directories
    loader.loadExtras(config.paths.extrasDirs);

    return dsoDB->finish();
}

} // namespace celestia
