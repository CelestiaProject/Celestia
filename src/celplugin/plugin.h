// plugin.h
//
// Copyright (C) 2019, Celestia Development Team
//
// Plugin class
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <string>
#include <celcompat/filesystem.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "plugin-common.h"

class CelestiaCore;
class CelestiaConfig;
class ProgressNotifier;
class IScript;
class Hash;
class Renderer;
class CachingOrbit;
class RotationModel;

namespace celestia
{
namespace plugin
{

class PluginManager;

class Plugin
{
 public:
    Plugin() = default;
    ~Plugin();
    Plugin(const Plugin&) = delete;
    Plugin(Plugin&&);
    Plugin& operator=(const Plugin&) = delete;
    Plugin& operator=(Plugin&&) = default;

    void* loadSym(const char*) const;

#ifdef PUBLIC_GET_INFO
    const PluginInfo* getPluginInfo() const { return m_pluginInfo; }
#endif
    bool isSupportedVersion() const;
    PluginType getType() const { return m_pluginInfo->Type; }

    static Plugin* load(const fs::path&);

    const char* getScriptLanguage() const { return m_pluginInfo->ID; }

    // pointers to plugin functions
    /// scripting support
    typedef bool(CreateScriptEnvironmentFunc)(CelestiaCore*, const CelestiaConfig*, ProgressNotifier*);
    typedef IScript*(CreateScriptFunc)(CelestiaCore*);
    typedef RotationModel*(CreateScritedRotationFunc)(const std::string&, const std::string&, Hash*);
    typedef CachingOrbit*(CreateScritedOrbitFunc)(const std::string&, const std::string&, Hash*);

    /// renderer support
    typedef Renderer*(CreateRendererFunc)();

    bool createScriptEnvironment(CelestiaCore *appCore, const CelestiaConfig *config, ProgressNotifier *progressNotifier) const;
    IScript* createScript(CelestiaCore *appCore) const;
    RotationModel* createScritedRotation(const std::string& moduleName, const std::string& funcName, Hash* parameters) const;
    CachingOrbit* createScritedOrbit(const std::string& moduleName, const std::string& funcName, Hash* parameters) const;
    Renderer* createRenderer() const;

 private:
    void loadPluginInfo();
#ifndef PUBLIC_GET_INFO
    const PluginInfo* getPluginInfo() const { return m_pluginInfo; }
#endif

#ifdef _WIN32
    HMODULE m_handle { nullptr };
#else
    void   *m_handle { nullptr };
#endif

    PluginInfo *m_pluginInfo;

    typedef PluginInfo*(RegisterFunc)();
    RegisterFunc *m_regfn;

    union
    {
        struct
        {
            CreateScriptEnvironmentFunc *createScriptEnvironment;
            CreateScriptFunc            *createScript;
            CreateScritedRotationFunc   *createScritedRotation;
            CreateScritedOrbitFunc      *createScritedOrbit;
        };
        struct
        {
            CreateRendererFunc         *createRenderer;
        };
    } m_func;

    friend class PluginManager;
}; // Plugin

}
}
