// winpreferences.cpp
//
// Copyright (C) 2023, Celestia Development Team
//
// Extracted from winmain.cpp:
// Copyright (C) 2001-2007, Chris Laurel <claurel@shatters.net>
//
// The main application window.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "winpreferences.h"

#include <string_view>
#include <type_traits>

#include <fmt/format.h>

#include <celengine/galaxy.h>
#include <celengine/observer.h>
#include <celengine/simulation.h>
#include <celestia/celestiacore.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/winutil.h>

#include <basetsd.h>

#include "wstringutils.h"

using celestia::util::GetLogger;

namespace celestia::win32
{

namespace
{

constexpr wchar_t CelestiaRegKey[] = L"Software\\celestiaproject.space\\Celestia1.7-dev";

template<typename T,
         std::enable_if_t<std::is_integral_v<T> && sizeof(T) == sizeof(DWORD), int> = 0>
bool
GetRegistryInt(HKEY key, LPCTSTR value, T& intVal)
{
    DWORD type;
    auto size = static_cast<DWORD>(sizeof(T));
    LSTATUS err = RegQueryValueEx(key, value, 0,
                                  &type, reinterpret_cast<BYTE*>(&intVal), &size);
    if (err != ERROR_SUCCESS || type != REG_DWORD)
    {
        intVal = 0;
        return false;
    }

    return true;
}

template<typename T,
         std::enable_if_t<std::is_integral_v<T> && sizeof(T) == sizeof(DWORD64), int> = 0>
bool
GetRegistryInt(HKEY key, LPCTSTR value, T& intVal)
{
    DWORD type;
    auto size = static_cast<DWORD>(sizeof(T));
    LSTATUS err = RegQueryValueEx(key, value, 0, &type,
                                  reinterpret_cast<LPBYTE>(&intVal),
                                  &size);
    if (err != ERROR_SUCCESS || type != REG_QWORD)
    {
        intVal = 0;
        return false;
    }

    return true;
}

template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
bool
GetRegistryEnum(HKEY key, LPCTSTR value, T& enumVal)
{
    if (std::underlying_type_t<T> intVal = 0; GetRegistryInt(key, value, intVal))
    {
        enumVal = static_cast<T>(intVal);
        return true;
    }

    return false;
}

template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
bool
GetRegistryEnum(HKEY key, LPCTSTR value, T& enumVal, T minValue, T maxValue)
{
    const auto minInteger = static_cast<std::underlying_type_t<T>>(minValue);
    const auto maxInteger = static_cast<std::underlying_type_t<T>>(maxValue);
    if (std::underlying_type_t<T> intVal = 0;
        GetRegistryInt(key, value, intVal) && intVal >= minInteger && intVal <= maxInteger)
    {
        enumVal = static_cast<T>(intVal);
        return true;
    }

    return false;
}

template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
bool
GetRegistryFloat(HKEY key, LPCTSTR value, T& floatVal)
{
    DWORD type;
    constexpr DWORD expectedSize = static_cast<DWORD>(sizeof(T));
    auto size = expectedSize;
    LSTATUS err = RegQueryValueEx(key, value, 0, &type,
                                  reinterpret_cast<LPBYTE>(&floatVal),
                                  &size);
    if (err != ERROR_SUCCESS || type != REG_BINARY || size != expectedSize)
    {
        floatVal = T(0);
        return false;
    }

    return true;
}

bool
GetRegistryString(HKEY key, LPCTSTR value, std::string& strVal)
{
    strVal.clear();

    DWORD type;
    DWORD size;
    LSTATUS err = RegQueryValueEx(key, value, 0, &type, nullptr, &size);
    if (err != ERROR_SUCCESS || type != REG_SZ || (size % sizeof(wchar_t)) != 0)
        return false;

    if (size == 0)
        return true;

    fmt::basic_memory_buffer<wchar_t> buffer;
    buffer.resize(size / sizeof(wchar_t));
    err = RegQueryValueEx(key, value, 0, &type,
                          reinterpret_cast<LPBYTE>(buffer.data()), &size);
    if (err != ERROR_SUCCESS || type != REG_SZ || (size % sizeof(wchar_t)) != 0)
        return false;

    return AppendWideToUTF8(std::wstring_view(buffer.data(), size), strVal) > 0;
}

template<typename T,
         std::enable_if_t<std::is_integral_v<T> && sizeof(T) == sizeof(DWORD), int> = 0>
bool
SetRegistryInt(HKEY key, LPCTSTR value, T intVal)
{
    LSTATUS err = RegSetValueEx(key, value, 0, REG_DWORD,
                                reinterpret_cast<const BYTE*>(&intVal),
                                sizeof(intVal));
    return err == ERROR_SUCCESS;
}

template<typename T,
         std::enable_if_t<std::is_integral_v<T> && sizeof(T) == sizeof(DWORD64), int> = 0>
bool
SetRegistryInt(HKEY key, LPCTSTR value, T intVal)
{
    LSTATUS err = RegSetValueEx(key, value, 0, REG_QWORD,
                                reinterpret_cast<const BYTE*>(&intVal),
                                sizeof(intVal));
    return err == ERROR_SUCCESS;
}

template<typename T,
         std::enable_if_t<std::is_enum_v<T>, int> = 0>
bool
SetRegistryEnum(HKEY key, LPCTSTR value, T enumVal)
{
    return SetRegistryInt(key, value, static_cast<std::underlying_type_t<T>>(enumVal));
}

bool
SetRegistryString(HKEY key, LPCTSTR value, std::string_view strVal)
{
    fmt::basic_memory_buffer<wchar_t> buffer;
    AppendUTF8ToWide(strVal, buffer);
    buffer.push_back(L'\0');

    LSTATUS err = RegSetValueEx(key, value, 0, REG_SZ,
                                reinterpret_cast<const BYTE*>(buffer.data()),
                                static_cast<DWORD>(buffer.size()));

    return err == ERROR_SUCCESS;
}

template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
bool
SetRegistryFloat(HKEY key, LPCTSTR value, T floatVal)
{
    LSTATUS err = RegSetValueEx(key, value, 0, REG_BINARY,
                                reinterpret_cast<const BYTE*>(&floatVal),
                                static_cast<DWORD>(sizeof(T)));
    return err == ERROR_SUCCESS;
}

} // end unnamed namespace

bool
LoadPreferencesFromRegistry(AppPreferences& prefs)
{
    LONG err;
    HKEY key;

    DWORD disposition;
    err = RegCreateKeyEx(HKEY_CURRENT_USER,
                         CelestiaRegKey,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &key,
                         &disposition);
    if (err  != ERROR_SUCCESS)
    {
        GetLogger()->error(_("Error opening registry key: {}\n"), err);
        return false;
    }

    GetRegistryInt(key, L"Width", prefs.winWidth);
    GetRegistryInt(key, L"Height", prefs.winHeight);
    GetRegistryInt(key, L"XPos", prefs.winX);
    GetRegistryInt(key, L"YPos", prefs.winY);
    GetRegistryEnum(key, L"RenderFlags", prefs.renderFlags);
    GetRegistryEnum(key, L"LabelMode", prefs.labelMode);
    GetRegistryInt(key, L"LocationFilter", prefs.locationFilter);
    GetRegistryEnum(key, L"OrbitMask", prefs.orbitMask);
    GetRegistryFloat(key, L"VisualMagnitude", prefs.visualMagnitude);
    GetRegistryFloat(key, L"AmbientLight", prefs.ambientLight);
    GetRegistryFloat(key, L"GalaxyLightGain", prefs.galaxyLightGain);
    GetRegistryInt(key, L"ShowLocalTime", prefs.showLocalTime);
    GetRegistryInt(key, L"DateFormat", prefs.dateFormat);
    GetRegistryInt(key, L"HudDetail", prefs.hudDetail);
    GetRegistryInt(key, L"FullScreenMode", prefs.fullScreenMode);
    GetRegistryInt(key, L"StarsColor", prefs.starsColor);
    GetRegistryEnum(key, L"StarStyle", prefs.starStyle, StarStyle::FuzzyPointStars, StarStyle::ScaledDiscStars);
    GetRegistryInt(key, L"LastVersion", prefs.lastVersion);
    GetRegistryEnum(key, L"TextureResolution", prefs.textureResolution, TextureResolution::lores, TextureResolution::hires);

    if (std::string altSurfaceName; GetRegistryString(key, L"AltSurface", altSurfaceName))
        prefs.altSurfaceName = std::move(altSurfaceName);

    if (prefs.lastVersion < 0x01020500)
    {
        prefs.renderFlags |= RenderFlags::ShowCometTails;
        prefs.renderFlags |= RenderFlags::ShowRingShadows;
    }

#ifndef PORTABLE_BUILD
    if (std::int32_t fav = 0; GetRegistryInt(key, L"IgnoreOldFavorites", fav))
        prefs.ignoreOldFavorites = fav != 0;
#endif

    RegCloseKey(key);

    return true;
}

bool
SavePreferencesToRegistry(AppPreferences& prefs)
{
    LONG err;
    HKEY key;

    GetLogger()->info(_("Saving preferences...\n"));
    err = RegOpenKeyEx(HKEY_CURRENT_USER,
                       CelestiaRegKey,
                       0,
                       KEY_ALL_ACCESS,
                       &key);
    if (err  != ERROR_SUCCESS)
    {
        GetLogger()->error(_("Error opening registry key: {}\n"), err);
        return false;
    }

    GetLogger()->info(_("Opened registry key\n"));

    SetRegistryInt(key, L"Width", prefs.winWidth);
    SetRegistryInt(key, L"Height", prefs.winHeight);
    SetRegistryInt(key, L"XPos", prefs.winX);
    SetRegistryInt(key, L"YPos", prefs.winY);
    SetRegistryEnum(key, L"RenderFlags", prefs.renderFlags);
    SetRegistryEnum(key, L"LabelMode", prefs.labelMode);
    SetRegistryInt(key, L"LocationFilter", prefs.locationFilter);
    SetRegistryInt(key, L"OrbitMask", static_cast<std::uint32_t>(prefs.orbitMask));
    SetRegistryFloat(key, L"VisualMagnitude", prefs.visualMagnitude);
    SetRegistryFloat(key, L"AmbientLight", prefs.ambientLight);
    SetRegistryFloat(key, L"GalaxyLightGain", prefs.galaxyLightGain);
    SetRegistryInt(key, L"ShowLocalTime", prefs.showLocalTime);
    SetRegistryInt(key, L"DateFormat", prefs.dateFormat);
    SetRegistryInt(key, L"HudDetail", prefs.hudDetail);
    SetRegistryInt(key, L"FullScreenMode", prefs.fullScreenMode);
    SetRegistryInt(key, L"LastVersion", prefs.lastVersion);
    SetRegistryEnum(key, L"StarStyle", prefs.starStyle);
    SetRegistryInt(key, L"StarsColor", prefs.starsColor);
    SetRegistryString(key, L"AltSurface", prefs.altSurfaceName);
    SetRegistryEnum(key, L"TextureResolution", prefs.textureResolution);
#ifndef PORTABLE_BUILD
    SetRegistryInt(key, L"IgnoreOldFavorites", prefs.ignoreOldFavorites ? INT32_C(1) : INT32_C(0));
#endif

    RegCloseKey(key);

    return true;
}

} // end namespace celestia::win32
