#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ios>
#include <memory>
#include <sstream>
#include <vector>

#include <doctest.h>

#include <celmodel/model.h>
#include <celmodel/modelfile.h>
#include <celutil/texhandle.h>

namespace
{

class ModelIO : public cmod::ModelLoader,
                public cmod::ModelWriter
{
protected:
    celestia::util::TextureHandle getHandle(const std::filesystem::path&, bool) override;
    const std::filesystem::path* getPath(celestia::util::TextureHandle) const override;

private:
    std::vector<std::filesystem::path> paths;
};

celestia::util::TextureHandle
ModelIO::getHandle(const std::filesystem::path& path, bool /* linear */)
{
    auto it = std::ranges::find(paths, path);
    if (it == paths.end())
    {
        auto result = static_cast<celestia::util::TextureHandle>(paths.size());
        paths.push_back(path);
        return result;
    }

    return static_cast<celestia::util::TextureHandle>(it - paths.begin());
}

const std::filesystem::path*
ModelIO::getPath(celestia::util::TextureHandle handle) const
{
    auto handleIdx = static_cast<std::size_t>(handle);
    if (handleIdx >= paths.size())
        return nullptr;

    return &paths[handleIdx];
}

} // end unnamed namespace

TEST_SUITE_BEGIN("CMOD integration");

TEST_CASE("CMOD binary to ASCII roundtrip")
{
    std::vector<std::filesystem::path> paths;
    ModelIO modelIO;

    std::ifstream f("testmodel.cmod", std::ios::in | std::ios::binary);
    REQUIRE(f.good());
    std::stringstream sourceData;
    sourceData << f.rdbuf();

    std::unique_ptr<cmod::Model> modelFromBinary = modelIO.load(sourceData);
    REQUIRE(modelFromBinary != nullptr);

    std::stringstream asciiData;
    REQUIRE(modelIO.saveText(*modelFromBinary, asciiData));

    std::unique_ptr<cmod::Model> modelFromAscii = modelIO.load(asciiData);
    REQUIRE(modelFromAscii != nullptr);

    std::stringstream roundtrippedData;
    REQUIRE(modelIO.saveBinary(*modelFromAscii, roundtrippedData));

    sourceData.clear();
    REQUIRE(sourceData.seekg(0, std::ios_base::beg).good());

    REQUIRE(sourceData.view() == roundtrippedData.view());
}

TEST_SUITE_END();
