// plugin.cpp
//
// Copyright (C) 2019, Celestia Development Team
//
// Plugin application side implementation
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <fmt/printf.h>
#include <utility>
#include "plugin.h"
#ifndef _WIN32
#include <dlfcn.h>
#endif

namespace celestia
{
namespace plugin
{

Plugin::~Plugin()
{
    if (m_handle != nullptr)
    {
#ifdef _WIN32
        FreeLibrary(m_handle);
#else
        dlclose(m_handle);
#endif
    }
}

Plugin::Plugin(Plugin &&other) :
    IScriptPlugin(std::forward<IScriptPlugin>(other))
{
    m_handle = other.m_handle;
    other.m_handle = nullptr;
    m_appCore = other.m_appCore;

    m_pluginInfo = other.m_pluginInfo;
    m_func = other.m_func;
}

bool Plugin::isSupportedVersion() const
{
    return m_pluginInfo->APIVersion == 0x0107;
}

Plugin* Plugin::load(CelestiaCore *appCore, const fs::path& path)
{
    Plugin p(appCore);

#ifdef _WIN32
    p.m_handle = ::LoadLibraryW(path.c_str());
    if (p.m_handle == nullptr)
    {
        fmt::print(std::cerr, "::LoadLibraryW({}) failed: {}\n", path);
        return nullptr;
    }
#else
    p.m_handle = dlopen(path.c_str(), RTLD_NOW/*|RTLD_GLOBAL*/);
    if (p.m_handle == nullptr)
    {
        fmt::print(std::cerr, "dlopen({}) failed: {}\n", path, dlerror());
        return nullptr;
    }
#endif

    void *ptr = p.loadSym(CELESTIA_PLUGIN_ENTRY_NAME_STR);
    if (ptr == nullptr)
        return nullptr;
    auto LoadPluginInfo = reinterpret_cast<RegisterFunc*>(ptr);
    p.m_pluginInfo = (*LoadPluginInfo)();

    if (p.m_pluginInfo == nullptr)
    {
        fmt::print(std::cerr, "plugin doesn't have PluginInfo\n");
        return nullptr;
    }

    if (!p.isSupportedVersion())
    {
        fmt::print(std::cerr, "unsupported plugin API version {:#06x}\n", p.m_pluginInfo->APIVersion);
        return nullptr;
    }

    switch (p.getType())
    {
    case TestPlugin:
        break;
    case Scripting:
        ptr = p.loadSym("CreateScriptEnvironment");
        if (ptr != nullptr)
            p.m_func.createScriptEnvironment = reinterpret_cast<CreateScriptEnvironmentFunc*>(ptr);
        break;
    default:
        fmt::print(std::cerr, "unknown plugin type {} ({})\n", p.getType(), path);
        return nullptr;
    }

    return new Plugin(std::move(p));
}

void* Plugin::loadSym(const char* fn) const
{
#ifdef _WIN32
    return ::GetProcAddress(m_handle, fn);
#else
    void *ptr = dlsym(m_handle, fn);
    char *error = dlerror();
    if (error != nullptr)
        fmt::print(std::cerr, "dlsym({}) failed: {}\n", fn, error);
    return ptr;
#endif
}

bool Plugin::createScriptEnvironment(CelestiaCore *appCore, const CelestiaConfig *config, ProgressNotifier *progressNotifier) const
{
    auto fn = m_func.createScriptEnvironment;
    return fn == nullptr ? false : (*fn)(appCore, config, progressNotifier);
}

IScript* Plugin::createScript(CelestiaCore *appCore) const
{
    auto fn = m_func.createScript;
    return fn == nullptr ? nullptr : (*fn)(appCore);
}

RotationModel* Plugin::createScriptedRotation(const std::string& moduleName, const std::string& funcName, Hash* parameters) const
{
    auto fn = m_func.createScriptedRotation;
    return fn == nullptr ? nullptr : (*fn)(moduleName, funcName, parameters);
}

CachingOrbit* Plugin::createScriptedOrbit(const std::string& moduleName, const std::string& funcName, Hash* parameters) const
{
    auto fn = m_func.createScriptedOrbit;
    return fn == nullptr ? nullptr : (*fn)(moduleName, funcName, parameters);
}

bool Plugin::isOurFile(const fs::path &filename) const
{
    auto fn = m_func.isOurFile;
    return fn == nullptr ? false : (*fn)(filename);
}

std::unique_ptr<IScript> Plugin::loadScript(const fs::path &filename)
{
    auto fn = m_func.loadScript;
    return fn == nullptr ? nullptr : (*fn)(filename);
}

Renderer* Plugin::createRenderer() const 
{ 
    auto fn = m_func.createRenderer;
    return fn == nullptr ? nullptr : (*fn)();
}

}
}
