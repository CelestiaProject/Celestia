#include <fmt/printf.h>
#include "plugin.h"
#include "pluginmanager.h"
#ifndef _WIN32
#include <dlfcn.h>
#endif

namespace celestia
{
namespace plugin
{
PluginManager::~PluginManager()
{
    for (auto p : m_plugins)
        delete p;
}

Plugin* PluginManager::loadByPath(const fs::path &path)
{
    return Plugin::load(path);
}

Plugin* PluginManager::loadByName(const std::string &name)
{
#if defined(_WIN32)
    return Plugin::load(m_directory / fmt::sprintf("%s.dll", name));
#elif defined(__APPLE__)
    return nullptr;
#else
    return Plugin::load(m_directory / fmt::sprintf("lib%s.so", name));
#endif
}

}
}
