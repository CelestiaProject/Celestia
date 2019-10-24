// pluginmanager.h
//
// Copyright (C) 2019, Celestia Development Team
//
// Plugin Manager class
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>
#include <vector>
#include <celcompat/filesystem.h>

class CelestiaCore;

namespace celestia
{
namespace plugin
{

class Plugin;

//
// PluginManager is an owner of all plugins
//
class PluginManager
{
 public:
    PluginManager() = delete;
    explicit PluginManager(CelestiaCore *appCore) :
        m_appCore(appCore)
    {}
    ~PluginManager();
    PluginManager(const PluginManager&) = delete;
    PluginManager(PluginManager&&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    PluginManager& operator=(PluginManager&&) = delete;

    const Plugin* loadByPath(const fs::path&);
    const Plugin* loadByName(const std::string&);

    void setSearchDirectory(const fs::path&);
    const fs::path& getSearchDirectory() const;

    const Plugin* getScriptPlugin(const std::string&) const;

 private:
    std::vector<const Plugin*> m_plugins;
    fs::path m_directory;
    CelestiaCore *m_appCore;
}; // PluginManager
}
}
