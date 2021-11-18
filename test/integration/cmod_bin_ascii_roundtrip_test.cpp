#include <algorithm>
#include <fstream>
#include <ios>
#include <iterator>
#include <sstream>

#include <celmodel/modelfile.h>

#include <catch.hpp>

TEST_CASE("CMOD binary to ASCII roundtrip", "[cmod] [integration]")
{
    std::ifstream f("iss.cmod", std::ios::in | std::ios::binary);
    REQUIRE(f.good());
    std::stringstream sourceData;
    sourceData << f.rdbuf();

    cmod::Model* modelFromBinary = cmod::LoadModel(sourceData, nullptr);
    REQUIRE(modelFromBinary != nullptr);

    std::stringstream asciiData;
    REQUIRE(cmod::SaveModelAscii(modelFromBinary, asciiData));

    cmod::Model* modelFromAscii = cmod::LoadModel(asciiData);
    REQUIRE(modelFromAscii != nullptr);

    std::stringstream roundtrippedData;
    REQUIRE(cmod::SaveModelBinary(modelFromAscii, roundtrippedData));

    sourceData.clear();
    REQUIRE(sourceData.seekg(0, std::ios_base::beg).good());

    std::istreambuf_iterator<char> end;
    REQUIRE(std::equal(std::istreambuf_iterator<char>(sourceData), end,
                       std::istreambuf_iterator<char>(roundtrippedData), end));
}
