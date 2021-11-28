#include <algorithm>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <sstream>
#include <vector>

#include <catch.hpp>

#include <celcompat/filesystem.h>
#include <celmodel/model.h>
#include <celmodel/modelfile.h>
#include <celutil/reshandle.h>


TEST_CASE("CMOD binary to ASCII roundtrip", "[cmod] [integration]")
{
    std::vector<fs::path> paths;
    cmod::HandleGetter handleGetter = [&](const fs::path& path)
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
    cmod::SourceGetter sourceGetter = [&](ResourceHandle handle) { return paths[handle]; };

    std::ifstream f("iss.cmod", std::ios::in | std::ios::binary);
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
