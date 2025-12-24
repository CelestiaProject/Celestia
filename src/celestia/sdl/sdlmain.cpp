// sdlmain.cpp
//
// Copyright (C) 2020-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cerrno>
#include <cstdlib>
#include <system_error>

#include <celestia/celestiacore.h>
#include <celutil/gettext.h>
#include "appwindow.h"
#include "environment.h"
#include "settings.h"

#ifdef _WIN32
#include <fmt/format.h>
#include <windows.h>
#endif

namespace
{

std::filesystem::path
getDataDir()
{
#ifdef _WIN32
    fmt::basic_memory_buffer<wchar_t, MAX_PATH + 1> buffer;
    buffer.resize(MAX_PATH + 1);

    std::size_t size;
    errno_t result;
    while ((result = _wgetenv_s(&size, buffer.data(), buffer.size(), L"CELESTIA_DATA_DIR")) == ERANGE)
        buffer.resize(size);
    if (result == 0 && size > 0)
        return buffer.data();
#else
    if (const char* dataDirEnv = std::getenv("CELESTIA_DATA_DIR"); dataDirEnv)
        return dataDirEnv;
#endif

    return CONFIG_DATA_DIR;
}

}

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

    std::filesystem::path dataDir = getDataDir();

    std::error_code ec;
    std::filesystem::current_path(dataDir, ec);
    if (ec)
        return EXIT_FAILURE;

    Settings settings = Settings::load(environment->getSettingsPath());

    auto window = environment->createAppWindow(settings);
    if (!window)
        return EXIT_FAILURE;

    window->dumpGLInfo();
    return window->run(settings) ? EXIT_SUCCESS : EXIT_FAILURE;
}
