#include "modelio.h"

#include <cstddef>

namespace cmodtools
{

celestia::util::TextureHandle
ModelIO::getHandle(const std::filesystem::path& path)
{
    auto [it, inserted] = m_handles.try_emplace(path, static_cast<celestia::util::TextureHandle>(m_handles.size()));
    if (inserted)
        m_paths.push_back(path);
    return it->second;
}

const std::filesystem::path*
ModelIO::getPath(celestia::util::TextureHandle handle) const
{
    const auto handleIdx = static_cast<std::size_t>(handle);
    if (handleIdx >= m_paths.size())
        return nullptr;
    return &m_paths[handleIdx];
}

void
ModelIO::reset()
{
    m_paths.clear();
    m_handles.clear();
}

ModelIO*
GetModelIO()
{
    static ModelIO modelIO;
    return &modelIO;
}

} // end namespace cmodtools
