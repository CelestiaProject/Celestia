// mesh.h
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_MESH_H_
#define _CELENGINE_MESH_H_

#include <celutil/basictypes.h>
#include <celutil/color.h>
#include <celutil/reshandle.h>
#include <celmath/ray.h>
#include <celmath/aabox.h>
#include <vector>
#include <string>


class RenderContext;

class Mesh
{
 public:
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
        SemanticMax  = 9,
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
                        uint32 _offset) :
            semantic(_semantic),
            format(_format),
            offset(_offset)
        {
        }

        VertexAttributeSemantic semantic;
        VertexAttributeFormat   format;
        uint32                  offset;
    };

    struct VertexDescription
    {
        VertexDescription(uint32 _stride,
                          uint32 _nAttributes,
                          VertexAttribute* _attributes);
        VertexDescription(const VertexDescription& desc);
        ~VertexDescription();

        const VertexAttribute& getAttribute(VertexAttributeSemantic semantic) const
        {
            return semanticMap[semantic];
        }

        bool validate() const;

        VertexDescription& operator=(const VertexDescription&);

        uint32 stride;
        uint32 nAttributes;
        VertexAttribute* attributes;

    private:
        void clearSemanticMap();
        void buildSemanticMap();
        
        // Vertex attributes indexed by semantic
        VertexAttribute semanticMap[SemanticMax];
    };

    enum TextureSemantic
    {
        DiffuseMap             =  0,
        NormalMap              =  1,
        SpecularMap            =  2,
        EmissiveMap            =  3,
        TextureSemanticMax     =  4,
        InvalidTextureSemantic = -1,
    };

    class Material
    {
    public:
        Material();

        Color diffuse;
        Color emissive;
        Color specular;
        float specularPower;
        float opacity;
        ResourceHandle maps[TextureSemanticMax];
    };

    enum PrimitiveGroupType
    {
        TriList    = 0,
        TriStrip   = 1,
        TriFan     = 2,
        LineList   = 3,
        LineStrip  = 4,
        PointList  = 5,
        PrimitiveTypeMax = 6,
        InvalidPrimitiveGroupType = -1
    };

    class PrimitiveGroup
    {
    public:
        PrimitiveGroupType prim;
        uint32 materialIndex;
        uint32* indices;
        uint32 nIndices;
    };

    Mesh();
    ~Mesh();

    void setVertices(uint32 _nVertices, void* vertexData);
    bool setVertexDescription(const VertexDescription& desc);
    const VertexDescription& getVertexDescription() const;

    const PrimitiveGroup* getGroup(uint32) const;
    uint32 addGroup(PrimitiveGroupType prim,
                    uint32 materialIndex,
                    uint32 nIndices,
                    uint32* indices);
    void remapIndices(const std::vector<uint32>& indexMap);

    bool pick(const Ray3d& r, double& distance) const;
    void render(const std::vector<const Material*>& materials,
                RenderContext&) const;

    AxisAlignedBox getBoundingBox() const;
    void transform(Vec3f translation, float scale);

    const void* getVertexData() const { return vertices; }
    uint32 getVertexCount() const { return nVertices; }
    uint32 getVertexStride() const { return vertexDesc.stride; }

    static PrimitiveGroupType      parsePrimitiveGroupType(const std::string&);
    static VertexAttributeSemantic parseVertexAttributeSemantic(const std::string&);
    static VertexAttributeFormat   parseVertexAttributeFormat(const std::string&);
    static TextureSemantic         parseTextureSemantic(const std::string&);
    static uint32                  getVertexAttributeSize(VertexAttributeFormat);

 private:
    void recomputeBoundingBox();

 private:
    VertexDescription vertexDesc;

    uint32 nVertices;
    void* vertices;

    std::vector<PrimitiveGroup*> groups;
};


#if 0
#include <celmath/frustum.h>
#include <celmath/ray.h>


class Mesh
{
 public:
    virtual ~Mesh() {};
    virtual void render(float lod) = 0;
    virtual void render(unsigned int attributes, float lod) = 0;
    virtual void render(unsigned int attributes, const Frustum&, float lod) = 0;
    virtual bool pick(const Ray3d&, double&) { return false; };

    enum {
        Normals    = 0x01,
        Tangents   = 0x02,
        Colors     = 0x04,
        TexCoords0 = 0x08,
        TexCoords1 = 0x10,
        VertexProgParams = 0x1000,
        Multipass  = 0x10000000,
    };
};
#endif

#endif // !_CELMESH_MESH_H_

