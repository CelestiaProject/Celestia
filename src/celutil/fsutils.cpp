// fsutils.h
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// Miscellaneous useful filesystem-related functions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <config.h> // HAVE_WORDEXP
#include <fstream>
#include <fmt/format.h>
#include "gettext.h"
#ifdef _WIN32
#include <shlobj.h>
#include "winutil.h"
#else
#ifdef __APPLE__
#include "appleutils.h"
#endif
#include <unistd.h>
#include <pwd.h>
#ifdef HAVE_WORDEXP
#include <wordexp.h>
#endif // HAVE_WORDEXP
#endif // !_WIN32
#include "fsutils.h"

#ifndef PATH_MAX
#define PATH_MAX 260
#endif

namespace celestia::util
{

fs::path LocaleFilename(const fs::path &p)
{
    const char *orig = N_("LANGUAGE");
    const char *lang = _(orig);
    if (lang == orig)
        return p;

    fs::path locPath = p.parent_path() / p.stem().concat("_").concat(lang).replace_extension(p.extension());

    std::error_code ec;
    if (fs::exists(locPath, ec))
        return locPath;

    locPath = fs::path("locale") / locPath;
    if (fs::exists(locPath, ec))
        return locPath;

    return p;
}


fs::path PathExp(const fs::path& filename)
{
#ifdef PORTABLE_BUILD
    return filename;
#elif defined(_WIN32)
    auto str = filename.native();
    if (str[0] == '~')
    {
        if (str.size() == 1)
            return HomeDir();
        if (str[1] == '\\' || str[1] == '/')
            return HomeDir() / str.substr(2);
    }

    return filename;
#elif defined(HAVE_WORDEXP)
    wordexp_t result;

    switch(wordexp(filename.c_str(), &result, WRDE_NOCMD))
    {
    case 0: // successful
        break;
    case WRDE_NOSPACE:
        // If the error was `WRDE_NOSPACE',
        // then perhaps part of the result was allocated.
        wordfree(&result);
    default: // some other error
        return filename;
    }

    if (result.we_wordc != 1)
    {
        wordfree(&result);
        return filename;
    }

    fs::path::string_type expanded(result.we_wordv[0]);
    wordfree(&result);
    return expanded;
#else
    return filename;
#endif
}

fs::path ResolveWildcard(const fs::path& wildcard,
                         array_view<std::string_view> extensions)
{
    fs::path filename(wildcard);

    for (std::string_view ext : extensions)
    {
        filename.replace_extension(ext);
        if (fs::exists(filename))
            return filename;
    }

    return fs::path();
}

#ifndef PORTABLE_BUILD
fs::path HomeDir()
{
#ifdef _WIN32
    wchar_t p[MAX_PATH + 1];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, 0, &p[0])))
        return fs::path(p);

    // fallback to environment variables
    std::size_t size;
    _wgetenv_s(&size, p, L"USERPROFILE");
    if (size != 0)
        return fs::path(p);

    _wgetenv_s(&size, p, L"HOMEDRIVE");
    if (size != 0)
    {
        fs::path ret(p);
        _wgetenv_s(&size, p, L"HOMEPATH");
        if (size != 0)
            return ret / fs::path(p);
    }

    // unlikely this is defined in woe but let's check
    _wgetenv_s(&size, p, L"HOME");
    if (size != 0)
        return fs::path(p);
#elif defined(__APPLE__)
    return AppleHomeDirectory();
#else
    const auto *home = getenv("HOME");
    if (home != nullptr)
        return home;

    struct passwd pw, *result = nullptr;
    char pw_dir[PATH_MAX];
    getpwuid_r(geteuid(), &pw, pw_dir, sizeof(pw_dir), &result);
    if (result != nullptr)
        return pw_dir;
#endif

    return fs::path();
}

fs::path WriteableDataPath()
{
#if defined(_WIN32)
    wchar_t p[MAX_PATH + 1];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, &p[0])))
        return PathExp(p) / "Celestia";

    // fallback to environment variables
    std::size_t size;
    _wgetenv_s(&size, p, L"APPDATA");
    if (size != 0)
        return PathExp(p) / "Celestia";

    return PathExp("~\\AppData\\Roaming") / "Celestia";

#elif defined(__APPLE__)
    return PathExp(AppleApplicationSupportDirectory()) / "Celestia";

#else
    const char *p = getenv("XDG_DATA_HOME");
    p = p != nullptr ? p : "~/.local/share";
    return PathExp(p) / "Celestia";
#endif
}
#endif // !PORTABLE_BUILD

} // end namespace celestia::util
