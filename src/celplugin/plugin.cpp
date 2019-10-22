#include <fmt/printf.h>
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

Plugin::Plugin(Plugin &&other)
{
    m_handle = other.m_handle;
    other.m_handle = nullptr;
    m_regfn = other.m_regfn;
    other.m_regfn = nullptr;

    setupScriptEnvironment = other.setupScriptEnvironment;
}

PluginInfo* Plugin::getPluginInfo() const
{
    return (*m_regfn)();
}

Plugin* Plugin::load(const fs::path& path)
{
    Plugin p;

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
    p.m_regfn = reinterpret_cast<RegisterFunc*>(ptr);

    PluginInfo *pi = p.getPluginInfo();
    switch (pi->Type)
    {
    case Nothing:
        break;
    case Script:
        ptr = p.loadSym("SetupScriptEnvironment");
        if (ptr != nullptr)
            p.setupScriptEnvironment = reinterpret_cast<SetupScriptEnvironmentFunc*>(ptr);
        break;
    default:
        fmt::print(std::cerr, "unknown plugin type {} ({})\n", pi->Type, path);
        return nullptr;
    }
    p.m_type = pi->Type;

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

}
}
