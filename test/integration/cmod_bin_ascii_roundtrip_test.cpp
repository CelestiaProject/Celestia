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
#include <celutil/reshandle.h>

TEST_SUITE_BEGIN("CMOD integration");

TEST_CASE("CMOD binary to ASCII roundtrip")
{
    std::vector<std::filesystem::path> paths;
    cmod::HandleGetter handleGetter = [&](const std::filesystem::path& path)
    {
        auto it = std::find(paths.cbegin(), paths.cend(), path);
        if (it == paths.cend())
        {
            paths.push_back(path);
            return static_cast<ResourceHandle>(paths.size() - 1);
        }
        else
        {
            return static_cast<ResourceHandle>(it - paths.cbegin());
        }
    };
    cmod::SourceGetter sourceGetter = [&](ResourceHandle handle) { return paths[static_cast<std::size_t>(handle)]; };

    std::ifstream f("testmodel.cmod", std::ios::in | std::ios::binary);
    REQUIRE(f.good());
    std::stringstream sourceData;
    sourceData << f.rdbuf();

    std::unique_ptr<cmod::Model> modelFromBinary = cmod::LoadModel(sourceData, handleGetter);
    REQUIRE(modelFromBinary != nullptr);

    std::stringstream asciiData;
    REQUIRE(cmod::SaveModelAscii(modelFromBinary.get(), asciiData, sourceGetter));

    std::unique_ptr<cmod::Model> modelFromAscii = cmod::LoadModel(asciiData, handleGetter);
    REQUIRE(modelFromAscii != nullptr);

    std::stringstream roundtrippedData;
    REQUIRE(cmod::SaveModelBinary(modelFromAscii.get(), roundtrippedData, sourceGetter));

    sourceData.clear();
    REQUIRE(sourceData.seekg(0, std::ios_base::beg).good());

    std::istreambuf_iterator<char> end;
    REQUIRE(std::equal(std::istreambuf_iterator<char>(sourceData), end,
                       std::istreambuf_iterator<char>(roundtrippedData), end));
}

TEST_SUITE_END();
