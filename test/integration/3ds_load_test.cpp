#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <doctest.h>

#include <cel3ds/3dsmodel.h>
#include <cel3ds/3dsread.h>
#include <celutil/binarywrite.h>

namespace
{

using Face = std::array<std::uint16_t, 3>;

std::string makeChunk(std::uint16_t type, const std::string& payload)
{
    std::ostringstream out(std::ios::out | std::ios::binary);
    REQUIRE(celestia::util::writeLE(out, type));
    REQUIRE(celestia::util::writeLE(out, static_cast<std::int32_t>(payload.size() + 6)));
    out.write(payload.data(), payload.size());
    REQUIRE(out.good());
    return out.str();
}

std::string makePoints(std::uint16_t count)
{
    std::ostringstream out(std::ios::out | std::ios::binary);
    REQUIRE(celestia::util::writeLE(out, count));
    for (std::uint32_t i = 0; i < count; ++i)
    {
        REQUIRE(celestia::util::writeLE(out, static_cast<float>(i)));
        REQUIRE(celestia::util::writeLE(out, 0.0f));
        REQUIRE(celestia::util::writeLE(out, 0.0f));
    }
    return makeChunk(0x4110, out.str());
}

std::string makeMaterialGroup(std::uint16_t faceIndex)
{
    std::ostringstream out(std::ios::out | std::ios::binary);
    out.write("material", 9);
    REQUIRE(celestia::util::writeLE(out, std::uint16_t{1}));
    REQUIRE(celestia::util::writeLE(out, faceIndex));
    return makeChunk(0x4130, out.str());
}

std::string makeFaces(const std::vector<Face>& faces, const std::string& subchunks = {})
{
    std::ostringstream out(std::ios::out | std::ios::binary);
    REQUIRE(faces.size() <= std::numeric_limits<std::uint16_t>::max());
    REQUIRE(celestia::util::writeLE(out, static_cast<std::uint16_t>(faces.size())));
    for (const auto& face : faces)
    {
        for (auto vertexIndex : face)
            REQUIRE(celestia::util::writeLE(out, vertexIndex));
        REQUIRE(celestia::util::writeLE(out, std::uint16_t{0}));
    }
    out.write(subchunks.data(), subchunks.size());
    REQUIRE(out.good());
    return makeChunk(0x4120, out.str());
}

std::unique_ptr<M3DScene> readMesh(const std::string& chunks)
{
    auto mesh = makeChunk(0x4100, chunks);
    auto object = makeChunk(0x4000, std::string("mesh\0", 5) + mesh);
    auto meshData = makeChunk(0x3d3d, object);
    auto data = makeChunk(0x4d4d, meshData);
    std::istringstream in(data, std::ios::in | std::ios::binary);
    return Read3DSFile(in);
}

} // end unnamed namespace

TEST_SUITE_BEGIN("3ds integration");

TEST_CASE("Load a 3DS file")
{
    std::unique_ptr<M3DScene> scene = Read3DSFile("icosphere.3ds");
    REQUIRE(scene != nullptr);
    REQUIRE(scene->getMaterialCount() == 1);

    REQUIRE(scene->getModelCount() == UINT32_C(1));

    std::uint32_t meshCount = 0;
    std::uint32_t faceCount = 0;
    std::uint32_t vertexCount = 0;
    for (std::uint32_t i = 0; i < scene->getModelCount(); ++i)
    {
        const M3DModel* model = scene->getModel(i);
        REQUIRE(model != nullptr);
        meshCount += model->getTriMeshCount();
        for (std::uint32_t j = 0; j < model->getTriMeshCount(); ++j)
        {
            const M3DTriangleMesh* mesh = model->getTriMesh(j);
            REQUIRE(mesh != nullptr);
            faceCount += static_cast<std::uint32_t>(mesh->getFaceCount());
            vertexCount += static_cast<std::uint32_t>(mesh->getVertexCount());
        }
    }

    REQUIRE(meshCount == 1);
    REQUIRE(faceCount == 80);
    REQUIRE(vertexCount == 63);
}

TEST_CASE("3DS rejects out-of-range face vertex indices")
{
    for (auto invalidIndex : {std::uint16_t{3}, std::numeric_limits<std::uint16_t>::max()})
    {
        for (std::size_t corner = 0; corner < 3; ++corner)
        {
            CAPTURE(invalidIndex);
            CAPTURE(corner);
            Face face{0, 1, 2};
            face[corner] = invalidIndex;
            CHECK(readMesh(makePoints(3) + makeFaces({face}, makeMaterialGroup(0))) == nullptr);
            CHECK(readMesh(makeFaces({face}, makeMaterialGroup(0)) + makePoints(3)) == nullptr);
        }
    }
    CHECK(readMesh(makeFaces({{0, 0, 0}}, makeMaterialGroup(0))) == nullptr);
}

TEST_CASE("3DS rejects out-of-range material-group face indices")
{
    for (auto invalidIndex : {std::uint16_t{1}, std::numeric_limits<std::uint16_t>::max()})
    {
        CAPTURE(invalidIndex);
        CHECK(readMesh(makePoints(3) + makeFaces({{0, 1, 2}}, makeMaterialGroup(invalidIndex))) == nullptr);
    }
    CHECK(readMesh(makePoints(3) + makeFaces({}, makeMaterialGroup(0))) == nullptr);
}

TEST_CASE("3DS validates references after all mesh arrays are read")
{
    auto points = makePoints(3);
    auto faces = makeFaces({{0, 1, 2}}, makeMaterialGroup(0));
    for (const auto& chunks : {points + faces, faces + points})
    {
        auto scene = readMesh(chunks);
        REQUIRE(scene != nullptr);
        REQUIRE(scene->getModelCount() == 1);
        auto* model = scene->getModel(0);
        REQUIRE(model->getTriMeshCount() == 1);
        auto* mesh = model->getTriMesh(0);
        CHECK(mesh->getVertexCount() == 3);
        CHECK(mesh->getFaceCount() == 1);
        CHECK(mesh->getMeshMaterialGroup(0)->faces == std::vector<std::uint16_t>{0});
    }

    // Repeated arrays retain their existing append semantics within the count limit.
    CHECK(readMesh(makePoints(1) + faces + makePoints(2)) != nullptr);
    auto firstFaces = makeFaces({{0, 1, 2}}, makeMaterialGroup(1));
    auto secondFaces = makeFaces({{2, 1, 0}});
    CHECK(readMesh(points + firstFaces + secondFaces) != nullptr);
}

TEST_CASE("3DS rejects accumulated mesh counts that overflow")
{
    constexpr auto maxCount = std::numeric_limits<std::uint16_t>::max();
    constexpr auto lastIndex = static_cast<std::uint16_t>(maxCount - 1);
    auto points = makePoints(maxCount);
    auto face = makeFaces({{0, 1, lastIndex}}, makeMaterialGroup(0));
    CHECK(readMesh(points + face) != nullptr);
    CHECK(readMesh(points + makePoints(1) + face) == nullptr);

    std::vector<Face> triangles(maxCount, {0, 1, 2});
    auto faces = makeFaces(triangles, makeMaterialGroup(lastIndex));
    CHECK(readMesh(makePoints(3) + faces) != nullptr);
    CHECK(readMesh(makePoints(3) + faces + makeFaces({{0, 1, 2}})) == nullptr);
}

TEST_SUITE_END();
