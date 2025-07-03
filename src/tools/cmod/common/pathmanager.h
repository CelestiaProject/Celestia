#pragma once

#include <filesystem>
#include <map>
#include <vector>

#include <celmodel/modelfile.h>
#include <celutil/reshandle.h>


namespace cmodtools
{

class PathManager
{
public:
    PathManager();
    ~PathManager() = default;

    void reset();

    cmod::HandleGetter getHandle;
    cmod::SourceGetter getSource;
private:
    std::vector<std::filesystem::path> paths{ };
    std::map<std::filesystem::path, ResourceHandle> handles{ };
};


extern PathManager* GetPathManager();

} // end namespace cmodtools
