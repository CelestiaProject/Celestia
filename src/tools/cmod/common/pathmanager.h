#pragma once

#include <map>
#include <vector>

#include <celcompat/filesystem.h>
#include <celmodel/modelfile.h>
#include <celutil/reshandle.h>

class PathManager
{
public:
    PathManager();
    ~PathManager() = default;

    void reset();

    cmod::HandleGetter getHandle;
    cmod::SourceGetter getSource;
private:
    std::vector<fs::path> paths{ };
    std::map<fs::path, ResourceHandle> handles{ };
};


extern PathManager* GetPathManager();
