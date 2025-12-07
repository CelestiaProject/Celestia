#include "pathmanager.h"

#include <cstddef>

namespace cmodtools
{

PathManager::PathManager()
{
    getHandle = [&](const std::filesystem::path& path)
    {
        auto it = handles.find(path);
        if (it == handles.end())
        {
            paths.push_back(path);
            auto result = static_cast<ResourceHandle>(paths.size() - 1);
            handles.insert({ path, result });
            return result;
        }
        else
        {
            return it->second;
        }
    };
    getSource = [&](ResourceHandle handle) { return paths[static_cast<std::size_t>(handle)]; };
}

void
PathManager::reset()
{
    paths.clear();
    handles.clear();
}

PathManager*
GetPathManager()
{
    static PathManager pathManager;
    return &pathManager;
}

} // end namespace cmodtools
