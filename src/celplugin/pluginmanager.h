#pragma once

#include <string>
#include <vector>
#include <celcompat/filesystem.h>


namespace celestia
{
namespace plugin
{
class Plugin;

class PluginManager
{
 public:
    ~PluginManager();
    const Plugin* loadByPath(const fs::path&);
    const Plugin* loadByName(const std::string &name);
    fs::path& searchDirectory() { return m_directory; }
    const fs::path searchDirectory() const { return m_directory; }

 private:
    std::vector<Plugin*> m_plugins;
    fs::path m_directory;
}; // PluginManager
}
}
