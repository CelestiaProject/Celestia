#include <cassert>
#include <numeric> // std::iota
#include <Eigen/Core>
#include <celmodel/mesh.h>
#include <celutil/logger.h>

using celestia::util::GetLogger;

namespace
{

struct Face
{
    Eigen::Vector3f normal;
    std::uint32_t i[3];    // vertex attribute indices
    std::uint32_t vi[3];   // vertex point indices -- same as above unless welding
};

void
copyVertex(cmod::VWord* newVertexData,
           const cmod::VertexDescription& newDesc,
           const cmod::VWord* oldVertexData,
           const cmod::VertexDescription& oldDesc,
           std::uint32_t oldIndex,
           const std::uint32_t fromOffsets[])
{
    unsigned int stride = oldDesc.strideBytes / sizeof(cmod::VWord);
    const cmod::VWord* oldVertex = oldVertexData + stride * oldIndex;

    for (std::size_t i = 0; i < newDesc.attributes.size(); i++)
    {
        if (fromOffsets[i] != ~0u)
        {
            std::memcpy(newVertexData + newDesc.attributes[i].offsetWords,
                        oldVertex + fromOffsets[i],
                        cmod::VertexAttribute::getFormatSizeWords(newDesc.attributes[i].format) * sizeof(cmod::VWord));
        }
    }
}

Eigen::Vector3f
getVertex(const cmod::VWord* vertexData,
          int positionOffset,
          std::uint32_t strideWords,
          std::uint32_t index)
{
    float fdata[3];
    std::memcpy(fdata, vertexData + strideWords * index + positionOffset, sizeof(float) * 3);
    return Eigen::Vector3f(fdata);
}


Eigen::Vector2f
getTexCoord(const cmod::VWord* vertexData,
            int texCoordOffset,
            uint32_t strideWords,
            uint32_t index)
{
    float fdata[2];
    std::memcpy(fdata, vertexData + strideWords * index + texCoordOffset, sizeof(float) * 2);
    return Eigen::Vector2f(fdata);
}

Eigen::Vector3f
averageFaceVectors(const std::vector<Face>& faces,
                   std::uint32_t thisFace,
                   std::uint32_t* vertexFaces,
                   std::uint32_t vertexFaceCount,
                   float cosSmoothingAngle)
{
    const Face& face = faces[thisFace];

    Eigen::Vector3f v = Eigen::Vector3f::Zero();
    for (std::uint32_t i = 0; i < vertexFaceCount; i++)
    {
        std::uint32_t f = vertexFaces[i];
        float cosAngle = face.normal.dot(faces[f].normal);
        if (f == thisFace || cosAngle > cosSmoothingAngle)
            v += faces[f].normal;
    }

    return v.squaredNorm() == 0.0f ? Eigen::Vector3f::UnitX() : v.normalized();
}

void
augmentVertexDescription(cmod::VertexDescription& desc,
                         cmod::VertexAttributeSemantic semantic,
                         cmod::VertexAttributeFormat format)
{
    std::uint32_t stride = 0;
    bool foundMatch = false;

    auto it = desc.attributes.begin();
    auto end = desc.attributes.end();
    for (auto i = desc.attributes.begin(); i != end; ++i)
    {
        if (semantic == i->semantic && format != i->format)
        {
            // The semantic matches, but the format does not; skip this
            // item.
            continue;
        }

        foundMatch |= (semantic == i->semantic);
        i->offsetWords = stride;
        stride += cmod::VertexAttribute::getFormatSizeWords(i->format);
        *it++ = std::move(*i);
    }

    desc.attributes.erase(it, end);

    if (!foundMatch)
    {
        desc.attributes.emplace_back(semantic, format, stride);
        stride += cmod::VertexAttribute::getFormatSizeWords(format);
    }

    desc.strideBytes = stride * sizeof(cmod::VWord);
}

} // namespace

namespace cmod
{

Mesh
GenerateTangents(const Mesh& mesh)
{
    std::uint32_t nVertices = mesh.getVertexCount();

    // In order to generate tangents, we require positions, normals, and
    // 2D texture coordinates in the vertex description.
    const auto& desc = mesh.getVertexDescription();
    if (desc.getAttribute(VertexAttributeSemantic::Position).format != VertexAttributeFormat::Float3)
    {
        GetLogger()->error("Vertex position must be a float3\n");
        return {};
    }

    if (desc.getAttribute(VertexAttributeSemantic::Normal).format != VertexAttributeFormat::Float3)
    {
        GetLogger()->error("float3 format vertex normal required\n");
        return {};
    }

    if (desc.getAttribute(VertexAttributeSemantic::Texture0).format == VertexAttributeFormat::InvalidFormat)
    {
        GetLogger()->error("Texture coordinates must be present in mesh to generate tangents\n");
        return {};
    }

    if (desc.getAttribute(VertexAttributeSemantic::Texture0).format != VertexAttributeFormat::Float2)
    {
        GetLogger()->error("Texture coordinate must be a float2\n");
        return {};
    }

    // Count the number of faces in the mesh.
    // (All geometry should already converted to triangle lists)
    std::uint32_t i;
    std::uint32_t nFaces = 0;
    for (i = 0; mesh.getGroup(i) != nullptr; i++)
    {
        const PrimitiveGroup* group = mesh.getGroup(i);
        if (group->prim == PrimitiveGroupType::TriList)
        {
            assert(group->indices.size() % 3 == 0);
            nFaces += group->indices.size() / 3;
        }
        else
        {
            GetLogger()->error("Mesh should contain just triangle lists\n");
            return {};
        }
    }

    // Build the array of faces; this may require decomposing triangle strips
    // and fans into triangle lists.
    std::vector<Face> faces(nFaces);

    std::uint32_t f = 0;
    for (i = 0; mesh.getGroup(i) != nullptr; i++)
    {
        const PrimitiveGroup* group = mesh.getGroup(i);

        switch (group->prim)
        {
        case PrimitiveGroupType::TriList:
            for (std::uint32_t j = 0; j < group->indices.size() / 3; j++)
            {
                assert(f < nFaces);
                faces[f].i[0] = group->indices[j * 3];
                faces[f].i[1] = group->indices[j * 3 + 1];
                faces[f].i[2] = group->indices[j * 3 + 2];
                f++;
            }
            break;

        default:
            break;
        }
    }

    unsigned int stride = desc.strideBytes / sizeof(VWord);
    std::uint32_t posOffset = desc.getAttribute(VertexAttributeSemantic::Position).offsetWords;
    std::uint32_t texCoordOffset = desc.getAttribute(VertexAttributeSemantic::Texture0).offsetWords;

    const VWord* vertexData = mesh.getVertexData();

    // Compute tangents for faces
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        Eigen::Vector3f p0 = getVertex(vertexData, posOffset, stride, face.i[0]);
        Eigen::Vector3f p1 = getVertex(vertexData, posOffset, stride, face.i[1]);
        Eigen::Vector3f p2 = getVertex(vertexData, posOffset, stride, face.i[2]);
        Eigen::Vector2f tc0 = getTexCoord(vertexData, texCoordOffset, stride, face.i[0]);
        Eigen::Vector2f tc1 = getTexCoord(vertexData, texCoordOffset, stride, face.i[1]);
        Eigen::Vector2f tc2 = getTexCoord(vertexData, texCoordOffset, stride, face.i[2]);
        float s1 = tc1.x() - tc0.x();
        float s2 = tc2.x() - tc0.x();
        float t1 = tc1.y() - tc0.y();
        float t2 = tc2.y() - tc0.y();
        float a = s1 * t2 - s2 * t1;
        if (a != 0.0f)
            face.normal = (t2 * (p1 - p0) - t1 * (p2 - p0)) * (1.0f / a);
        else
            face.normal = Eigen::Vector3f::Zero();
    }

    // For each vertex, create a list of faces that contain it
    std::uint32_t* faceCounts = new std::uint32_t[nVertices];
    std::uint32_t** vertexFaces = new std::uint32_t*[nVertices];

    // Initialize the lists
    for (i = 0; i < nVertices; i++)
    {
        faceCounts[i] = 0;
        vertexFaces[i] = nullptr;
    }

    for (f = 0; f < nFaces; f++)
    {
        faces[f].vi[0] = faces[f].i[0];
        faces[f].vi[1] = faces[f].i[1];
        faces[f].vi[2] = faces[f].i[2];
    }

    // Count the number of faces in which each vertex appears
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        faceCounts[face.vi[0]]++;
        faceCounts[face.vi[1]]++;
        faceCounts[face.vi[2]]++;
    }

    // Allocate space for the per-vertex face lists
    for (i = 0; i < nVertices; i++)
    {
        if (faceCounts[i] > 0)
        {
            vertexFaces[i] = new std::uint32_t[faceCounts[i] + 1];
            vertexFaces[i][0] = faceCounts[i];
        }
    }

    // Fill in the vertex/face lists
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        vertexFaces[face.vi[0]][faceCounts[face.vi[0]]--] = f;
        vertexFaces[face.vi[1]][faceCounts[face.vi[1]]--] = f;
        vertexFaces[face.vi[2]][faceCounts[face.vi[2]]--] = f;
    }

    // Compute the vertex tangents by averaging
    std::vector<Eigen::Vector3f> vertexTangents(nFaces * 3);
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];
        for (std::uint32_t j = 0; j < 3; j++)
        {
            vertexTangents[f * 3 + j] =
                averageFaceVectors(faces, f,
                                   &vertexFaces[face.vi[j]][1],
                                   vertexFaces[face.vi[j]][0],
                                   0.0f);
        }
    }

    // Create the new vertex description
    VertexDescription newDesc = desc.clone();
    augmentVertexDescription(newDesc, VertexAttributeSemantic::Tangent, VertexAttributeFormat::Float3);

    // We need to convert the copy the old vertex attributes to the new
    // mesh.  In order to do this, we need the old offset of each attribute
    // in the new vertex description.  The fromOffsets array will contain
    // this mapping.
    std::uint32_t tangentOffset = 0;
    std::uint32_t fromOffsets[16];
    for (i = 0; i < newDesc.attributes.size(); i++)
    {
        fromOffsets[i] = ~0;

        if (newDesc.attributes[i].semantic == VertexAttributeSemantic::Tangent)
        {
            tangentOffset = newDesc.attributes[i].offsetWords;
        }
        else
        {
            for (const auto& oldAttr : desc.attributes)
            {
                if (oldAttr.semantic == newDesc.attributes[i].semantic)
                {
                    assert(oldAttr.format == newDesc.attributes[i].format);
                    fromOffsets[i] = oldAttr.offsetWords;
                    break;
                }
            }
        }
    }

    // Copy the old vertex data along with the generated tangents to the
    // new vertex data buffer.
    unsigned int newStride = newDesc.strideBytes / sizeof(VWord);
    std::vector<VWord> newVertexData(newStride * nFaces * 3);
    for (f = 0; f < nFaces; f++)
    {
        Face& face = faces[f];

        for (std::uint32_t j = 0; j < 3; j++)
        {
            VWord* newVertex = newVertexData.data() + (f * 3 + j) * newStride;
            copyVertex(newVertex, newDesc,
                       vertexData, desc,
                       face.i[j],
                       fromOffsets);
            std::memcpy(newVertex + tangentOffset, &vertexTangents[f * 3 + j], 3 * sizeof(float));
        }
    }

    // Create the Celestia mesh
    Mesh newMesh;
    newMesh.setVertexDescription(std::move(newDesc));
    newMesh.setVertices(nFaces * 3, std::move(newVertexData));

    std::uint32_t firstIndex = 0;
    for (std::uint32_t groupIndex = 0; mesh.getGroup(groupIndex) != 0; ++groupIndex)
    {
        const PrimitiveGroup* group = mesh.getGroup(groupIndex);
        unsigned int faceCount = 0;

        switch (group->prim)
        {
        case PrimitiveGroupType::TriList:
            faceCount = group->indices.size() / 3;
            break;
        case PrimitiveGroupType::TriStrip:
        case PrimitiveGroupType::TriFan:
            faceCount = group->indices.size() - 2;
            break;
        default:
            assert(0);
            break;
        }

        // Create a trivial index list
        std::vector<Index32> indices(faceCount * 3);
        std::iota(indices.begin(), indices.end(), firstIndex);

        newMesh.addGroup(PrimitiveGroupType::TriList,
                         mesh.getGroup(groupIndex)->materialIndex,
                         std::move(indices));
        firstIndex += faceCount * 3;
    }

    // Clean up
    delete[] faceCounts;
    for (i = 0; i < nVertices; i++)
    {
        if (vertexFaces[i] != nullptr)
            delete[] vertexFaces[i];
    }
    delete[] vertexFaces;

    return newMesh;
}

} // namespace cmod
