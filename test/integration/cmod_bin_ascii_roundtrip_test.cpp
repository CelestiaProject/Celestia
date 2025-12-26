#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
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
    celestia::util::TextureHandle getHandle(const std::filesystem::path&) override;
    const std::filesystem::path* getPath(celestia::util::TextureHandle) const override;

private:
    std::vector<std::filesystem::path> paths;
};

celestia::util::TextureHandle
ModelIO::getHandle(const std::filesystem::path& path)
{
    auto it = std::find(paths.cbegin(), paths.cend(), path);
    if (it == paths.cend())
    {
        auto result = static_cast<celestia::util::TextureHandle>(paths.size());
        paths.push_back(path);
        return result;
    }

    return static_cast<celestia::util::TextureHandle>(it - paths.cbegin());
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

    std::istreambuf_iterator<char> end;
    REQUIRE(std::equal(std::istreambuf_iterator<char>(sourceData), end,
                       std::istreambuf_iterator<char>(roundtrippedData), end));
}

TEST_SUITE_END();
