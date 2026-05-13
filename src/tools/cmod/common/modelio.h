#pragma once

#include <filesystem>
#include <unordered_map>
#include <vector>

#include <celmodel/modelfile.h>
#include <celutil/fsutils.h>
#include <celutil/texhandle.h>

namespace cmodtools
{

class ModelIO final : public cmod::ModelLoader,
                      public cmod::ModelWriter
{
public:
    void reset();
    celestia::util::TextureHandle handle(const std::filesystem::path& path) { return getHandle(path); }
    const std::filesystem::path* path(celestia::util::TextureHandle handle) const { return getPath(handle); }

protected:
    celestia::util::TextureHandle getHandle(const std::filesystem::path&) override;
    const std::filesystem::path* getPath(celestia::util::TextureHandle) const override;

private:
    std::vector<std::filesystem::path> m_paths;
    std::unordered_map<std::filesystem::path, celestia::util::TextureHandle, celestia::util::PathHasher> m_handles;
};

extern ModelIO* GetModelIO();

} // end namespace cmodtools
