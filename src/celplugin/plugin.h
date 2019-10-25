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
#include <celscript/common/script.h> // IScriptPlugin, IScript
#include <celengine/parser.h> // Hash
#ifdef _WIN32
#include <windows.h>
#endif
#include "plugin-common.h"

class CelestiaCore;
class CelestiaConfig;
class ProgressNotifier;
class Renderer;
class CachingOrbit;
class RotationModel;

namespace celestia
{
namespace plugin
{

using scripts::IScript;
using scripts::IScriptPlugin;

class PluginManager;

class Plugin : public IScriptPlugin
{
 public:
    Plugin() = delete;
    explicit Plugin(CelestiaCore *appCore) :
        IScriptPlugin(appCore)
    {}
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

    static Plugin* load(CelestiaCore*, const fs::path&);

    const char* getScriptLanguage() const { return m_pluginInfo->ID; }

    // pointers to plugin functions
    /// scripting support
    typedef bool(CreateScriptEnvironmentFunc)(CelestiaCore*, const CelestiaConfig*, ProgressNotifier*);
    typedef IScript*(CreateScriptFunc)(CelestiaCore*);
    typedef RotationModel*(CreateScriptedRotationFunc)(const std::string&, const std::string&, Hash*);
    typedef CachingOrbit*(CreateScriptedOrbitFunc)(const std::string&, const std::string&, Hash*);
    typedef bool (IsOurFile)(const fs::path&);
    typedef std::unique_ptr<IScript>(LoadScript)(const fs::path&);

    /// renderer support
    typedef Renderer*(CreateRendererFunc)();


    /// scripting support
    bool createScriptEnvironment(CelestiaCore *appCore, const CelestiaConfig *config, ProgressNotifier *progressNotifier) const;
    IScript* createScript(CelestiaCore *appCore) const;
    RotationModel* createScriptedRotation(const std::string& moduleName, const std::string& funcName, Hash* parameters) const;
    CachingOrbit* createScriptedOrbit(const std::string& moduleName, const std::string& funcName, Hash* parameters) const;
    bool isOurFile(const fs::path&) const;
    std::unique_ptr<IScript> loadScript(const fs::path&);

    /// renderer support
    Renderer* createRenderer() const;

 private:
    typedef PluginInfo*(RegisterFunc)();
#ifndef PUBLIC_GET_INFO
    const PluginInfo* getPluginInfo() const { return m_pluginInfo; }
#endif

#ifdef _WIN32
    HMODULE m_handle { nullptr };
#else
    void   *m_handle { nullptr };
#endif
    CelestiaCore *m_appCore;
    PluginInfo *m_pluginInfo;

    union
    {
        struct
        {
            CreateScriptEnvironmentFunc *createScriptEnvironment;
            CreateScriptFunc            *createScript;
            CreateScriptedRotationFunc  *createScriptedRotation;
            CreateScriptedOrbitFunc     *createScriptedOrbit;
            IsOurFile                   *isOurFile;
            LoadScript                  *loadScript;
        };
        struct
        {
            CreateRendererFunc          *createRenderer;
        };
    } m_func;

    friend class PluginManager;
}; // Plugin

}
}
