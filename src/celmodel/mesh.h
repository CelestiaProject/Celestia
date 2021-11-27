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
using index32 = std::uint32_t;


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
    NextPosition = 10,
    ScaleFactor  = 11,
    SemanticMax  = 12,
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
        offset(0)
    {
    }

    VertexAttribute(VertexAttributeSemantic _semantic,
                    VertexAttributeFormat _format,
                    unsigned int _offset) :
        semantic(_semantic),
        format(_format),
        offset(_offset)
    {
    }

    static constexpr unsigned int getFormatSize(VertexAttributeFormat fmt)
    {
        switch (fmt)
        {
        case VertexAttributeFormat::Float1:
        case VertexAttributeFormat::UByte4:
            return 4;
        case VertexAttributeFormat::Float2:
            return 8;
        case VertexAttributeFormat::Float3:
            return 12;
        case VertexAttributeFormat::Float4:
            return 16;
        default:
            return 0;
        }
    }

    VertexAttributeSemantic semantic;
    VertexAttributeFormat   format;
    unsigned int            offset;
};


struct VertexDescription
{
    VertexDescription(unsigned int _stride,
                      unsigned int _nAttributes,
                      VertexAttribute* _attributes);
    VertexDescription(const VertexDescription& desc);
    ~VertexDescription();

    inline const VertexAttribute& getAttribute(VertexAttributeSemantic semantic) const
    {
        return semanticMap[static_cast<std::size_t>(semantic)];
    }

    VertexDescription appendingAttributes(const VertexAttribute* newAttributes, int count) const;

    bool validate() const;

    VertexDescription& operator=(const VertexDescription&);

    unsigned int stride;
    unsigned int nAttributes;
    VertexAttribute* attributes;

 private:
    void clearSemanticMap();
    void buildSemanticMap();

    // Vertex attributes indexed by semantic
    std::array<VertexAttribute, static_cast<std::size_t>(VertexAttributeSemantic::SemanticMax)> semanticMap;
};


class PrimitiveGroup
{
 public:
    PrimitiveGroup() = default;
    ~PrimitiveGroup() = default;

    unsigned int getPrimitiveCount() const;

    PrimitiveGroupType prim;
    unsigned int materialIndex;
    index32* indices;
    unsigned int nIndices;
    PrimitiveGroupType primOverride;
    void* vertexOverride;
    unsigned int vertexCountOverride;
    index32* indicesOverride;
    unsigned int nIndicesOverride;
    VertexDescription vertexDescriptionOverride { 0, 0, nullptr };
};


class Mesh
{
 public:
    class PickResult
    {
    public:
        PickResult() = default;

        Mesh* mesh{ nullptr };
        PrimitiveGroup* group{ nullptr };
        unsigned int primitiveIndex{ 0 };
        double distance{ -1.0 };
    };

    Mesh() = default;
    ~Mesh();

    void setVertices(unsigned int _nVertices, void* vertexData);
    bool setVertexDescription(const VertexDescription& desc);
    const VertexDescription& getVertexDescription() const;

    PrimitiveGroup* createLinePrimitiveGroup(bool lineStrip, unsigned int nIndices, index32* indices);
    const PrimitiveGroup* getGroup(unsigned int index) const;
    PrimitiveGroup* getGroup(unsigned int index);
    unsigned int addGroup(PrimitiveGroup* group);
    unsigned int addGroup(PrimitiveGroupType prim,
                          unsigned int materialIndex,
                          unsigned int nIndices,
                          index32* indices);
    unsigned int getGroupCount() const;
    void remapIndices(const std::vector<index32>& indexMap);
    void clearGroups();

    void remapMaterials(const std::vector<unsigned int>& materialMap);

    /*! Reorder primitive groups so that groups with identical materials
     *  appear sequentially in the primitive group list. This will reduce
     *  the number of graphics state changes at render time.
     */
    void aggregateByMaterial();

    const std::string& getName() const;
    void setName(const std::string&);

    bool pick(const Eigen::Vector3d& origin, const Eigen::Vector3d& direction, PickResult* result) const;
    bool pick(const Eigen::Vector3d& origin, const Eigen::Vector3d& direction, double& distance) const;

    Eigen::AlignedBox<float, 3> getBoundingBox() const;
    void transform(const Eigen::Vector3f& translation, float scale);

    const void* getVertexData() const { return vertices; }
    unsigned int getVertexCount() const { return nVertices; }
    unsigned int getVertexStride() const { return vertexDesc.stride; }
    unsigned int getPrimitiveCount() const;

 private:
    void recomputeBoundingBox();

    VertexDescription vertexDesc{ 0, 0, nullptr };

    unsigned int nVertices{ 0 };
    void* vertices{ nullptr };

    std::vector<PrimitiveGroup*> groups;

    std::string name;
};

} // namespace cmod
