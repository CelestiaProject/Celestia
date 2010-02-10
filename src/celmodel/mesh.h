// mesh.h
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELMODEL_MESH_H_
#define _CELMODEL_MESH_H_

#include "material.h"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <vector>
#include <string>


namespace cmod
{

class Mesh
{
 public:
    // 32-bit index type
    typedef unsigned int index32;
    class BufferResource
    {
    };

    enum VertexAttributeSemantic
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

    enum VertexAttributeFormat
    {
        Float1    = 0,
        Float2    = 1,
        Float3    = 2,
        Float4    = 3,
        UByte4    = 4,
        FormatMax = 5,
        InvalidFormat = -1,
    };

    struct VertexAttribute
    {
        VertexAttribute() :
            semantic(InvalidSemantic),
            format(InvalidFormat),
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

        const VertexAttribute& getAttribute(VertexAttributeSemantic semantic) const
        {
            return semanticMap[semantic];
        }

        bool validate() const;

        VertexDescription& operator=(const VertexDescription&);

        unsigned int stride;
        unsigned int nAttributes;
        VertexAttribute* attributes;

    private:
        void clearSemanticMap();
        void buildSemanticMap();
        
        // Vertex attributes indexed by semantic
        VertexAttribute semanticMap[SemanticMax];
    };

    enum PrimitiveGroupType
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

    class PrimitiveGroup
    {
    public:
        PrimitiveGroup();
        ~PrimitiveGroup();

        unsigned int getPrimitiveCount() const;
        
        PrimitiveGroupType prim;
        unsigned int materialIndex;
        index32* indices;
        unsigned int nIndices;
    };

    class PickResult
    {
    public:
        PickResult();

        Mesh* mesh;
        PrimitiveGroup* group;
        unsigned int primitiveIndex;
        double distance;
    };

    Mesh();
    ~Mesh();

    void setVertices(unsigned int _nVertices, void* vertexData);
    bool setVertexDescription(const VertexDescription& desc);
    const VertexDescription& getVertexDescription() const;

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

    static PrimitiveGroupType        parsePrimitiveGroupType(const std::string&);
    static VertexAttributeSemantic   parseVertexAttributeSemantic(const std::string&);
    static VertexAttributeFormat     parseVertexAttributeFormat(const std::string&);
    static Material::TextureSemantic parseTextureSemantic(const std::string&);
    static unsigned int              getVertexAttributeSize(VertexAttributeFormat);

 private:
    void recomputeBoundingBox();

 private:
    VertexDescription vertexDesc;

    unsigned int nVertices;
    void* vertices;
    mutable BufferResource* vbResource;

    std::vector<PrimitiveGroup*> groups;

    std::string name;
};

} // namespace cmod

#endif // !_CELMESH_MESH_H_

