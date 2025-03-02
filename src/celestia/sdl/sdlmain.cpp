// sdlmain.cpp
//
// Copyright (C) 2020-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdlib>
#include <system_error>

#include <celestia/celestiacore.h>
#include <celutil/gettext.h>
#include "appwindow.h"
#include "environment.h"
#include "settings.h"

int
main(int argc, char **argv)
{
    using celestia::sdl::Environment;
    using celestia::sdl::Settings;

    CelestiaCore::initLocale();

#ifdef ENABLE_NLS
    bindtextdomain("celestia", LOCALEDIR);
    bind_textdomain_codeset("celestia", "UTF-8");
    bindtextdomain("celestia-data", LOCALEDIR);
    bind_textdomain_codeset("celestia", "UTF-8");
    textdomain("celestia");
#endif

    auto environment = Environment::init();
    if (!environment)
        return EXIT_FAILURE;

    if (!environment->setGLAttributes())
        return EXIT_FAILURE;

    fs::path dataDir;
#ifdef _WIN32
    if (const wchar_t* dataDirEnv = _wgetenv(L"CELESTIA_DATA_DIR"); dataDirEnv == nullptr)
#else
    if (const char* dataDirEnv = std::getenv("CELESTIA_DATA_DIR"); dataDirEnv == nullptr)
#endif
        dataDir = CONFIG_DATA_DIR;
    else
        dataDir = dataDirEnv;

    std::error_code ec;
    fs::current_path(dataDir, ec);
    if (ec)
        return EXIT_FAILURE;

    Settings settings = Settings::load(environment->getSettingsPath());

    auto window = environment->createAppWindow(settings);
    if (!window)
        return EXIT_FAILURE;

    window->dumpGLInfo();
    return window->run(settings) ? EXIT_SUCCESS : EXIT_FAILURE;
}
