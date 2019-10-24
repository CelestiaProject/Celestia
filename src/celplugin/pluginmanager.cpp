// pluginmanager.cpp
//
// Copyright (C) 2019, Celestia Development Team
//
// Plugin Manager implementation
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <fmt/printf.h>
#include <algorithm>
#ifndef _WIN32
#include <dlfcn.h>
#endif
#include <celutil/util.h>
#include "plugin.h"
#include "pluginmanager.h"

namespace celestia
{
namespace plugin
{

PluginManager::~PluginManager()
{
    for (auto p : m_plugins)
        delete p;
}

void PluginManager::setSearchDirectory(const fs::path &directory)
{
    m_directory = directory;
}

const fs::path& PluginManager::getSearchDirectory() const
{
    return m_directory;
}

const Plugin* PluginManager::loadByPath(const fs::path &path)
{
    auto p = Plugin::load(m_appCore, path);
    if (p != nullptr)
        m_plugins.push_back(p);
    return p;
}

const Plugin* PluginManager::loadByName(const std::string &name)
{
#if defined(_WIN32)
    return loadByPath(m_directory / fmt::sprintf("%s.dll", name));
#elif defined(__APPLE__)
    return loadByPath(m_directory / fmt::sprintf("lib%s.dylib", name));
#else
    return loadByPath(m_directory / fmt::sprintf("lib%s.so", name));
#endif
}

const Plugin* PluginManager::getScriptPlugin(const std::string &lang) const
{
    auto cmp = [&lang](const Plugin* p){ return compareIgnoringCase(p->getScriptLanguage(), lang) == 0; };
    auto it = std::find_if(m_plugins.begin(), m_plugins.end(), cmp);
    return it != m_plugins.end() ? *it : nullptr;
}

}
}
