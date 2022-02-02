#include <cstdint>
#include <memory>

#include <catch.hpp>

#include <cel3ds/3dsmodel.h>
#include <cel3ds/3dsread.h>

TEST_CASE("Load a 3DS file", "[3ds] [integration]")
{
    std::unique_ptr<M3DScene> scene = Read3DSFile("huygens.3ds");
    REQUIRE(scene != nullptr);
    REQUIRE(scene->getMaterialCount() == 4);

    REQUIRE(scene->getModelCount() == UINT32_C(8));

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

    REQUIRE(meshCount == 8);
    REQUIRE(faceCount == 6098);
    REQUIRE(vertexCount == 3263);
}
