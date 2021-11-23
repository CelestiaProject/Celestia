#include "pathmanager.h"


PathManager::PathManager()
{
    getHandle = [&](const fs::path& path)
    {
        auto it = handles.find(path);
        if (it == handles.end())
        {
            paths.push_back(path);
            ResourceHandle result = paths.size() - 1;
            handles.insert({ path, result });
            return result;
        }
        else
        {
            return it->second;
        }
    };
    getSource = [&](ResourceHandle handle) { return paths[handle]; };
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
