#pragma once

#include <filesystem>
#include <unordered_map>
#include <vector>

#include <celmodel/modelfile.h>
#include <celutil/reshandle.h>

namespace cmodtools
{

class ModelIO final : public cmod::ModelLoader,
                      public cmod::ModelWriter
{
public:
    void reset();
    ResourceHandle handle(const std::filesystem::path& path) { return getHandle(path); }
    const std::filesystem::path* path(ResourceHandle handle) const { return getPath(handle); }

protected:
    ResourceHandle getHandle(const std::filesystem::path&) override;
    const std::filesystem::path* getPath(ResourceHandle) const override;

private:
    std::vector<std::filesystem::path> m_paths;
    std::unordered_map<std::filesystem::path, ResourceHandle> m_handles;
};

extern ModelIO* GetModelIO();

} // end namespace cmodtools
