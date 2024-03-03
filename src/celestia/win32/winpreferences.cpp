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
#ifdef _UNICODE
#include <celutil/winutil.h>
#endif

#include <basetsd.h>

#include "tstring.h"

using celestia::util::GetLogger;

namespace celestia::win32
{

namespace
{

constexpr TCHAR CelestiaRegKey[] = TEXT("Software\\celestiaproject.space\\Celestia1.7-dev");

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
#ifdef _UNICODE
    if (err != ERROR_SUCCESS || type != REG_SZ || (size % sizeof(wchar_t)) != 0)
#else
    if (err != ERROR_SUCCESS || type != REG_SZ)
#endif
        return false;

    if (size == 0)
        return true;

#ifdef _UNICODE
    fmt::basic_memory_buffer<wchar_t> buffer;
    buffer.resize(size / sizeof(wchar_t));
#else
    fmt::memory_buffer buffer;
    buffer.resize(size);
#endif
    err = RegQueryValueEx(key, value, 0, &type,
                          reinterpret_cast<LPBYTE>(buffer.data()), &size);
#ifdef _UNICODE
    if (err != ERROR_SUCCESS || type != REG_SZ || (size % sizeof(wchar_t)) != 0)
#else
    if (err != ERROR_SUCCESS || type != REG_SZ)
#endif
        return false;

    return AppendTCharToUTF8(tstring_view(buffer.data(), size), strVal) > 0;
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


bool
SetRegistryString(HKEY key, LPCTSTR value, std::string_view strVal)
{
    fmt::basic_memory_buffer<TCHAR> buffer;
    AppendUTF8ToTChar(strVal, buffer);
    buffer.push_back(TEXT('\0'));

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

    GetRegistryInt(key, TEXT("Width"), prefs.winWidth);
    GetRegistryInt(key, TEXT("Height"), prefs.winHeight);
    GetRegistryInt(key, TEXT("XPos"), prefs.winX);
    GetRegistryInt(key, TEXT("YPos"), prefs.winY);
    GetRegistryInt(key, TEXT("RenderFlags"), prefs.renderFlags);
    GetRegistryInt(key, TEXT("LabelMode"), prefs.labelMode);
    GetRegistryInt(key, TEXT("LocationFilter"), prefs.locationFilter);
    if (int orbitMask = 0; GetRegistryInt(key, TEXT("OrbitMask"), orbitMask))
        prefs.orbitMask = static_cast<BodyClassification>(orbitMask);
    GetRegistryFloat(key, TEXT("VisualMagnitude"), prefs.visualMagnitude);
    GetRegistryFloat(key, TEXT("AmbientLight"), prefs.ambientLight);
    GetRegistryFloat(key, TEXT("GalaxyLightGain"), prefs.galaxyLightGain);
    GetRegistryInt(key, TEXT("ShowLocalTime"), prefs.showLocalTime);
    GetRegistryInt(key, TEXT("DateFormat"), prefs.dateFormat);
    GetRegistryInt(key, TEXT("HudDetail"), prefs.hudDetail);
    GetRegistryInt(key, TEXT("FullScreenMode"), prefs.fullScreenMode);
    GetRegistryInt(key, TEXT("StarsColor"), prefs.starsColor);
    if (std::uint32_t starStyle = 0; GetRegistryInt(key, TEXT("StarStyle"), starStyle))
        prefs.starStyle = static_cast<Renderer::StarStyle>(starStyle);
    else
        prefs.starStyle = Renderer::FuzzyPointStars;

    GetRegistryInt(key, TEXT("LastVersion"), prefs.lastVersion);
    GetRegistryInt(key, TEXT("TextureResolution"), prefs.textureResolution);

    if (std::string altSurfaceName; GetRegistryString(key, TEXT("AltSurface"), altSurfaceName))
        prefs.altSurfaceName = std::move(altSurfaceName);

    if (prefs.lastVersion < 0x01020500)
    {
        prefs.renderFlags |= Renderer::ShowCometTails;
        prefs.renderFlags |= Renderer::ShowRingShadows;
    }

#ifndef PORTABLE_BUILD
    if (std::int32_t fav = 0; GetRegistryInt(key, TEXT("IgnoreOldFavorites"), fav))
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

    SetRegistryInt(key, TEXT("Width"), prefs.winWidth);
    SetRegistryInt(key, TEXT("Height"), prefs.winHeight);
    SetRegistryInt(key, TEXT("XPos"), prefs.winX);
    SetRegistryInt(key, TEXT("YPos"), prefs.winY);
    SetRegistryInt(key, TEXT("RenderFlags"), prefs.renderFlags);
    SetRegistryInt(key, TEXT("LabelMode"), prefs.labelMode);
    SetRegistryInt(key, TEXT("LocationFilter"), prefs.locationFilter);
    SetRegistryInt(key, TEXT("OrbitMask"), static_cast<std::uint32_t>(prefs.orbitMask));
    SetRegistryFloat(key, TEXT("VisualMagnitude"), prefs.visualMagnitude);
    SetRegistryFloat(key, TEXT("AmbientLight"), prefs.ambientLight);
    SetRegistryFloat(key, TEXT("GalaxyLightGain"), prefs.galaxyLightGain);
    SetRegistryInt(key, TEXT("ShowLocalTime"), prefs.showLocalTime);
    SetRegistryInt(key, TEXT("DateFormat"), prefs.dateFormat);
    SetRegistryInt(key, TEXT("HudDetail"), prefs.hudDetail);
    SetRegistryInt(key, TEXT("FullScreenMode"), prefs.fullScreenMode);
    SetRegistryInt(key, TEXT("LastVersion"), prefs.lastVersion);
    SetRegistryInt(key, TEXT("StarStyle"), static_cast<std::int32_t>(prefs.starStyle));
    SetRegistryInt(key, TEXT("StarsColor"), prefs.starsColor);
    SetRegistryString(key, TEXT("AltSurface"), prefs.altSurfaceName);
    SetRegistryInt(key, TEXT("TextureResolution"), prefs.textureResolution);
#ifndef PORTABLE_BUILD
    SetRegistryInt(key, TEXT("IgnoreOldFavorites"), prefs.ignoreOldFavorites ? INT32_C(1) : INT32_C(0));
#endif

    RegCloseKey(key);

    return true;
}

} // end namespace celestia::win32
