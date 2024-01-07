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
#include <array>
#include <cctype>
#include <fstream>
#include <utility>
#include <fmt/format.h>
#include "gettext.h"
#include "logger.h"
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

std::optional<fs::path>
U8FileName(std::string_view source, bool allowWildcardExtension)
{
    using namespace std::string_view_literals;

    // Windows: filenames cannot end with . or space
    if (source.empty() || source.back() == '.' || source.back() == ' ')
        return std::nullopt;

    // Various characters disallowed in Windows filenames
    constexpr std::string_view badChars = R"("/:<>?\|)"sv;
    const auto lastPos = source.size() - 1;
    for (std::string_view::size_type i = 0; i < source.size(); ++i)
    {
        char ch = source[i];
        // Windows (and basic politeness) disallows all control characters
        // Only allow * as extension .* at the end of the name
        if (ch < ' ' || badChars.find(ch) != std::string_view::npos ||
            (ch == '*' && !(allowWildcardExtension && i == lastPos && i > 0 && source[i - 1] == '.')))
        {
            return std::nullopt;
        }
    }

    // Disallow reserved Windows device names
    if (std::string_view stem = source.substr(0, source.rfind('.'));
        stem.size() >= 3 && stem.size() <= 5)
    {
        std::array<char, 5> buffer{ '\0', '\0', '\0', '\0', '\0' };
        stem.copy(buffer.data(), 5);
        for (std::size_t i = 0; i < stem.size(); ++i)
        {
            buffer[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(buffer[i])));
        }

        stem = std::string_view(buffer.data(), stem.size());
        if (stem == "CON"sv || stem == "PRN"sv || stem == "AUX"sv || stem == "NUL"sv)
            return std::nullopt;

        constexpr std::string_view superscriptTrailBytes = "\xb2\xb3\xb9"sv;

        // COM or LPT followed by digits 0-9 or superscript digits 1-3
        if ((stem.substr(0, 3) == "COM"sv || stem.substr(0, 3) == "LPT"sv) &&
            ((stem.size() == 4 && stem[3] >= '0' && stem[3] <= '9') ||
             (stem.size() == 5 && stem[3] == '\xc2' &&
              superscriptTrailBytes.find(stem[4]) != std::string_view::npos)))
        {
            return std::nullopt;
        }
    }

    return fs::u8path(source);
}

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


fs::path PathExp(fs::path&& filename)
{
#ifdef PORTABLE_BUILD
    return std::move(filename);
#elif defined(_WIN32)
    const auto& str = filename.native();
    if (str[0] == L'~')
    {
        if (str.size() == 1)
            return HomeDir();
        if (str[1] == L'\\' || str[1] == L'/')
            return HomeDir() / str.substr(2);
    }

    return std::move(filename);
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
        return std::move(filename);
    }

    if (result.we_wordc != 1)
    {
        wordfree(&result);
        return std::move(filename);
    }

    fs::path::string_type expanded(result.we_wordv[0]);
    wordfree(&result);
    return expanded;
#else
    return std::move(filename);
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

bool IsValidDirectory(const fs::path& dir)
{
    if (dir.empty())
        return false;

    if (std::error_code ec; !fs::is_directory(dir, ec))
    {
        GetLogger()->error(_("Path {} doesn't exist or isn't a directory\n"), dir);
        return false;
    }

    return true;
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
