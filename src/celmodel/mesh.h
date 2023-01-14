// mesh.h
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "material.h"


namespace cmod
{
// 32-bit index type
using Index32 = std::uint32_t;

using VWord = std::uint32_t;

enum class VertexAttributeSemantic : std::int16_t
{
    Position     = 0,
    Color0       = 1,
    Color1       = 2,
    Normal       = 3,
    Tangent      = 4,
    Texture0     = 5,
    Texture1     = 6,
    Texture2     = 7,
    Texture3     = 8,
    PointSize    = 9,
    SemanticMax  = 10,
    InvalidSemantic  = -1,
};


enum class VertexAttributeFormat : std::int16_t
{
    Float1    = 0,
    Float2    = 1,
    Float3    = 2,
    Float4    = 3,
    UByte4    = 4,
    FormatMax = 5,
    InvalidFormat = -1,
};


enum class PrimitiveGroupType : std::int16_t
{
    TriList    = 0,
    TriStrip   = 1,
    TriFan     = 2,
    LineList   = 3,
    LineStrip  = 4,
    PointList  = 5,
    SpriteList = 6,
    PrimitiveTypeMax = 7,
    InvalidPrimitiveGroupType = -1
};


struct VertexAttribute
{
    VertexAttribute() :
        semantic(VertexAttributeSemantic::InvalidSemantic),
        format(VertexAttributeFormat::InvalidFormat),
        offsetWords(0)
    {
    }

    VertexAttribute(VertexAttributeSemantic _semantic,
                    VertexAttributeFormat _format,
                    unsigned int _offsetWords) :
        semantic(_semantic),
        format(_format),
        offsetWords(_offsetWords)
    {
    }

    static constexpr unsigned int getFormatSizeWords(VertexAttributeFormat fmt)
    {
        switch (fmt)
        {
        case VertexAttributeFormat::Float1:
        case VertexAttributeFormat::UByte4:
            return 1;
        case VertexAttributeFormat::Float2:
            return 2;
        case VertexAttributeFormat::Float3:
            return 3;
        case VertexAttributeFormat::Float4:
            return 4;
        default:
            return 0;
        }
    }

    VertexAttributeSemantic semantic;
    VertexAttributeFormat   format;
    unsigned int            offsetWords;
};

bool operator==(const VertexAttribute& a, const VertexAttribute& b);
bool operator<(const VertexAttribute& a, const VertexAttribute& b);


struct VertexDescription
{
    VertexDescription() = default;
    explicit VertexDescription(std::vector<VertexAttribute>&& attributes);
    ~VertexDescription() = default;
    VertexDescription(VertexDescription&&) = default;
    VertexDescription& operator=(VertexDescription&&) = default;

    VertexDescription clone() const { return *this; }

    inline const VertexAttribute& getAttribute(VertexAttributeSemantic semantic) const
    {
        return semanticMap[static_cast<std::size_t>(semantic)];
    }

    bool validate() const;

    unsigned int strideBytes{ 0 };
    std::vector<VertexAttribute> attributes{ };

 private:
    VertexDescription(const VertexDescription&) = default;
    VertexDescription& operator=(const VertexDescription&) = default;

    void clearSemanticMap();
    void buildSemanticMap();

    // Vertex attributes indexed by semantic
    std::array<VertexAttribute, static_cast<std::size_t>(VertexAttributeSemantic::SemanticMax)> semanticMap;
};


bool operator==(const VertexDescription& a, const VertexDescription& b);
bool operator<(const VertexDescription& a, const VertexDescription& b);


struct PrimitiveGroup
{
    PrimitiveGroup() = default;
    ~PrimitiveGroup() = default;
    PrimitiveGroup(const PrimitiveGroup&) = delete;
    PrimitiveGroup& operator=(const PrimitiveGroup&) = delete;
    PrimitiveGroup(PrimitiveGroup&&) = default;
    PrimitiveGroup& operator=(PrimitiveGroup&&) = default;

    PrimitiveGroup clone() const;
    unsigned int getPrimitiveCount() const;

    PrimitiveGroupType prim{ PrimitiveGroupType::InvalidPrimitiveGroupType };
    unsigned int materialIndex{ 0 };
    int indicesCount{ 0 };
    int indicesOffset{ 0 };
    std::vector<Index32> indices{ };
};


class Mesh
{
 public:
    class PickResult
    {
    public:
        PickResult() = default;

        const Mesh* mesh{ nullptr };
        const PrimitiveGroup* group{ nullptr };
        unsigned int primitiveIndex{ 0 };
        double distance{ -1.0 };
    };

    Mesh() = default;
    ~Mesh() = default;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) = default;
    Mesh& operator=(Mesh&&) = default;

    Mesh clone() const;

    void setVertices(unsigned int _nVertices, std::vector<VWord>&& vertexData);
    bool setVertexDescription(VertexDescription&& desc);
    const VertexDescription& getVertexDescription() const;

    const PrimitiveGroup* getGroup(unsigned int index) const;
    PrimitiveGroup* getGroup(unsigned int index);
    unsigned int addGroup(PrimitiveGroup&& group);
    unsigned int addGroup(PrimitiveGroupType prim,
                          unsigned int materialIndex,
                          std::vector<Index32>&& indices);
    unsigned int getGroupCount() const;
    void remapIndices(const std::vector<Index32>& indexMap);
    void clearGroups();

    void remapMaterials(const std::vector<unsigned int>& materialMap);

    /*! Reorder primitive groups so that groups with identical materials
     *  appear sequentially in the primitive group list. This will reduce
     *  the number of graphics state changes at render time.
     */
    void aggregateByMaterial();

    const std::string& getName() const;
    void setName(std::string&&);

    bool pick(const Eigen::Vector3d& origin, const Eigen::Vector3d& direction, PickResult* result) const;
    bool pick(const Eigen::Vector3d& origin, const Eigen::Vector3d& direction, double& distance) const;

    Eigen::AlignedBox<float, 3> getBoundingBox() const;
    void transform(const Eigen::Vector3f& translation, float scale);

    const VWord* getVertexData() const { return vertices.data(); }
    unsigned int getVertexCount() const { return nVertices; }
    unsigned int getVertexStrideWords() const { return vertexDesc.strideBytes / sizeof(cmod::VWord); }
    unsigned int getPrimitiveCount() const;

    unsigned int getIndexCount() const { return nTotalIndices; }

    void merge(const Mesh&);
    bool canMerge(const Mesh&, const std::vector<Material> &materials) const;
    void optimize();

    void rebuildIndexMetadata();

 private:
    void mergePrimitiveGroups();

    VertexDescription vertexDesc{ };

    unsigned int nVertices{ 0 };
    std::vector<VWord> vertices{ };

    unsigned int nTotalIndices{ 0 };

    std::vector<PrimitiveGroup> groups;

    std::string name;
};

} // namespace cmod
