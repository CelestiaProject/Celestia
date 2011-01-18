// mesh.cpp
//
// Copyright (C) 2004-2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cassert>
#include <iostream>
#include "mesh.h"
#include "rendcontext.h"
#include "gl.h"
#include "glext.h"

using namespace std;


static size_t VertexAttributeFormatSizes[Mesh::FormatMax] =
{
     4,  // Float1
     8,  // Float2
     12, // Float3
     16, // Float4,
     4,  // UByte4
};


// Vertex buffer object support

// VBO optimization is only worthwhile for large enough vertex lists
static const unsigned int MinVBOSize = 4096;
static bool VBOSupportTested = false;
static bool VBOSupported = false;

static bool isVBOSupported()
{
    if (!VBOSupportTested)
    {
        VBOSupportTested = true;
        VBOSupported = ExtensionSupported("GL_ARB_vertex_buffer_object");
    }

    return VBOSupported;
}


Mesh::Material::Material() :
    diffuse(0.0f, 0.0f, 0.0f),
    emissive(0.0f, 0.0f, 0.0f),
    specular(0.0f, 0.0f, 0.0f),
    specularPower(1.0f),
    opacity(1.0f),
    blend(NormalBlend)
{
    for (int i = 0; i < TextureSemanticMax; i++)
        maps[i] = InvalidResource;

}


Mesh::VertexDescription::VertexDescription(uint32 _stride,
                                           uint32 _nAttributes,
                                           VertexAttribute* _attributes) :
            stride(_stride),
            nAttributes(_nAttributes),
            attributes(NULL)
{
    if (nAttributes != 0)
    {
        attributes = new VertexAttribute[nAttributes];
        for (uint32 i = 0; i < nAttributes; i++)
            attributes[i] = _attributes[i];
        buildSemanticMap();
    }
}


Mesh::VertexDescription::VertexDescription(const VertexDescription& desc) :
    stride(desc.stride),
    nAttributes(desc.nAttributes),
    attributes(NULL)
{
    if (nAttributes != 0)
    {
        attributes = new VertexAttribute[nAttributes];
        for (uint32 i = 0; i < nAttributes; i++)
            attributes[i] = desc.attributes[i];
        buildSemanticMap();
    }
}


Mesh::VertexDescription&
Mesh::VertexDescription::operator=(const Mesh::VertexDescription& desc)
{
    if (nAttributes < desc.nAttributes)
    {
        if (attributes != NULL)
            delete[] attributes;
        attributes = new VertexAttribute[desc.nAttributes];
    }

    nAttributes = desc.nAttributes;
    stride = desc.stride;
    for (uint32 i = 0; i < nAttributes; i++)
        attributes[i] = desc.attributes[i];
    clearSemanticMap();
    buildSemanticMap();

    return *this;
}


// TODO: This should be called in the constructor; we should start using
// exceptions in Celestia.
bool
Mesh::VertexDescription::validate() const
{
    for (uint32 i = 0; i < nAttributes; i++)
    {
        VertexAttribute& attr = attributes[i];

        // Validate the attribute
        if (attr.semantic >= SemanticMax || attr.format >= FormatMax)
            return false;
        if (attr.offset % 4 != 0)
            return false;
        if (attr.offset + VertexAttributeFormatSizes[attr.format] > stride)
            return false;
        // TODO: check for repetition of attributes
        // if (vertexAttributeMap[attr->semantic].format != InvalidFormat)
        //   return false;
    }

    return true;
}


Mesh::VertexDescription::~VertexDescription()
{
    delete[] attributes;
}


void
Mesh::VertexDescription::buildSemanticMap()
{
    for (uint32 i = 0; i < nAttributes; i++)
        semanticMap[attributes[i].semantic] = attributes[i];
}


void
Mesh::VertexDescription::clearSemanticMap()
{
    for (uint32 i = 0; i < SemanticMax; i++)
        semanticMap[i] = VertexAttribute();
}


Mesh::PrimitiveGroup::PrimitiveGroup()
{
}


Mesh::PrimitiveGroup::~PrimitiveGroup()
{
    // TODO: probably should free index list; need to sort out
    // ownership issues.
}


uint32
Mesh::PrimitiveGroup::getPrimitiveCount() const
{
    switch (prim)
    {
    case TriList:
        return nIndices / 3;
    case TriStrip:
    case TriFan:
        return nIndices - 2;
    case LineList:
        return nIndices / 2;
    case LineStrip:
        return nIndices - 2;
    case PointList:
    case SpriteList:
        return nIndices;
    default:
        // Invalid value
        return 0;
    }
}


Mesh::Mesh() :
    vertexDesc(0, 0, NULL),
    nVertices(0),
    vertices(NULL),
    vbObject(0),
    vbInitialized(false)
{
}


Mesh::~Mesh()
{
    for (vector<PrimitiveGroup*>::iterator iter = groups.begin();
         iter != groups.end(); iter++)
    {
        delete *iter;
    }

    // TODO: this is just to cast away void* and shut up GCC warnings;
    // should probably be static_cast<VertexList::VertexPart*>
    if (vertices != NULL)
        delete[] static_cast<char*>(vertices);

    if (vbObject != 0)
    {
        glx::glDeleteBuffersARB(1, &vbObject);
    }
}


void
Mesh::setVertices(uint32 _nVertices, void* vertexData)
{
    nVertices = _nVertices;
    vertices = vertexData;
}


bool
Mesh::setVertexDescription(const VertexDescription& desc)
{
    if (!desc.validate())
        return false;

    vertexDesc = desc;

    return true;
}


const Mesh::VertexDescription& Mesh::getVertexDescription() const
{
    return vertexDesc;
}


const Mesh::PrimitiveGroup*
Mesh::getGroup(uint32 index) const
{
    if (index >= groups.size())
        return NULL;
    else
        return groups[index];
}


uint32
Mesh::addGroup(PrimitiveGroup* group)
{
    groups.push_back(group);
    return groups.size();
}


uint32
Mesh::addGroup(PrimitiveGroupType prim,
               uint32 materialIndex,
               uint32 nIndices,
               uint32* indices)
{
    PrimitiveGroup* g = new PrimitiveGroup();
    g->prim = prim;
    g->materialIndex = materialIndex;
    g->nIndices = nIndices;
    g->indices = indices;

    return addGroup(g);
}


uint32
Mesh::getGroupCount() const
{
    return groups.size();
}


void
Mesh::clearGroups()
{
    for (vector<PrimitiveGroup*>::iterator iter = groups.begin();
         iter != groups.end(); iter++)
    {
        delete *iter;
    }

    groups.clear();
}


const string&
Mesh::getName() const
{
    return name;
}


void
Mesh::setName(const string& _name)
{
    name = _name;
}


void
Mesh::remapIndices(const vector<uint32>& indexMap)
{
    for (vector<PrimitiveGroup*>::iterator iter = groups.begin();
         iter != groups.end(); iter++)
    {
        PrimitiveGroup* group = *iter;
        for (uint32 i = 0; i < group->nIndices; i++)
        {
            group->indices[i] = indexMap[group->indices[i]];
        }
    }
}


void
Mesh::remapMaterials(const vector<uint32>& materialMap)
{
    for (vector<PrimitiveGroup*>::iterator iter = groups.begin();
         iter != groups.end(); iter++)
    {
        (*iter)->materialIndex = materialMap[(*iter)->materialIndex];
    }
}


class PrimitiveGroupComparator : public std::binary_function<const Mesh::PrimitiveGroup*, const Mesh::PrimitiveGroup*, bool>
{
public:
    bool operator()(const Mesh::PrimitiveGroup* g0, const Mesh::PrimitiveGroup* g1) const
    {
        return g0->materialIndex < g1->materialIndex;
    }

private:
    int unused;
};


void
Mesh::aggregateByMaterial()
{
    sort(groups.begin(), groups.end(), PrimitiveGroupComparator());
}


bool
Mesh::pick(const Ray3d& ray, double& distance) const
{
    double maxDistance = 1.0e30;
    double closest = maxDistance;

    // Pick will automatically fail without vertex positions--no reasonable
    // mesh should lack these.
    if (vertexDesc.getAttribute(Position).semantic != Position ||
        vertexDesc.getAttribute(Position).format != Float3)
    {
        return false;
    }

    uint posOffset = vertexDesc.getAttribute(Position).offset;
    char* vdata = reinterpret_cast<char*>(vertices);

    // Iterate over all primitive groups in the mesh
    for (vector<PrimitiveGroup*>::const_iterator iter = groups.begin();
         iter != groups.end(); iter++)
    {
        Mesh::PrimitiveGroupType primType = (*iter)->prim;
        uint32 nIndices = (*iter)->nIndices;

        // Only attempt to compute the intersection of the ray with triangle
        // groups.
        if ((primType == TriList || primType == TriStrip || primType == TriFan) &&
            (nIndices >= 3) &&
            !(primType == TriList && nIndices % 3 != 0))
        {
            uint32 index = 0;
            uint32 i0 = (*iter)->indices[0];
            uint32 i1 = (*iter)->indices[1];
            uint32 i2 = (*iter)->indices[2];

            // Iterate over the triangles in the primitive group
            do
            {
                // Get the triangle vertices v0, v1, and v2
                float* f0 = reinterpret_cast<float*>(vdata + i0 * vertexDesc.stride + posOffset);
                float* f1 = reinterpret_cast<float*>(vdata + i1 * vertexDesc.stride + posOffset);
                float* f2 = reinterpret_cast<float*>(vdata + i2 * vertexDesc.stride + posOffset);
                Point3d v0(f0[0], f0[1], f0[2]);
                Point3d v1(f1[0], f1[1], f1[2]);
                Point3d v2(f2[0], f2[1], f2[2]);

                // Compute the edge vectors e0 and e1, and the normal n
                Vec3d e0 = v1 - v0;
                Vec3d e1 = v2 - v0;
                Vec3d n = e0 ^ e1;

                // c is the cosine of the angle between the ray and triangle normal
                double c = n * ray.direction;

                // If the ray is parallel to the triangle, it either misses the
                // triangle completely, or is contained in the triangle's plane.
                // If it's contained in the plane, we'll still call it a miss.
                if (c != 0.0)
                {
                    double t = (n * (v0 - ray.origin)) / c;
                    if (t < closest && t > 0.0)
                    {
                        double m00 = e0 * e0;
                        double m01 = e0 * e1;
                        double m10 = e1 * e0;
                        double m11 = e1 * e1;
                        double det = m00 * m11 - m01 * m10;
                        if (det != 0.0)
                        {
                            Point3d p = ray.point(t);
                            Vec3d q = p - v0;
                            double q0 = e0 * q;
                            double q1 = e1 * q;
                            double d = 1.0 / det;
                            double s0 = (m11 * q0 - m01 * q1) * d;
                            double s1 = (m00 * q1 - m10 * q0) * d;
                            if (s0 >= 0.0 && s1 >= 0.0 && s0 + s1 <= 1.0)
                                closest = t;
                        }
                    }
                }

                // Get the indices for the next triangle
                if (primType == TriList)
                {
                    index += 3;
                    if (index < nIndices)
                    {
                        i0 = (*iter)->indices[index + 0];
                        i1 = (*iter)->indices[index + 1];
                        i2 = (*iter)->indices[index + 2];
                    }
                }
                else if (primType == TriStrip)
                {
                    index += 1;
                    if (index < nIndices)
                    {
                        i0 = i1;
                        i1 = i2;
                        i2 = (*iter)->indices[index];
                        // TODO: alternate orientation of triangles in a strip
                    }
                }
                else // primType == TriFan
                {
                    index += 1;
                    if (index < nIndices)
                    {
                        index += 1;
                        i1 = i2;
                        i2 = (*iter)->indices[index];
                    }
                }

            } while (index < nIndices);
        }
    }

    if (closest != maxDistance)
    {
        distance = closest;
        return true;
    }
    else
    {
        return false;
    }
}


void
Mesh::render(const std::vector<const Material*>& materials,
             RenderContext& rc) const
{
    // The first time the mesh is rendered, we will try and place the
    // vertex data in a vertex buffer object and potentially get a huge
    // rendering performance boost.  This can consume a great deal of
    // memory, since we're duplicating the vertex data.  TODO: investigate
    // the possibility of deleting the original data.  We can always map
    // read-only later on for things like picking, but this could be a low
    // performance path.
    if (!vbInitialized && isVBOSupported())
    {
        vbInitialized = true;

        if (nVertices * vertexDesc.stride > MinVBOSize)
        {
            glx::glGenBuffersARB(1, &vbObject);
            if (vbObject != 0)
            {
                glx::glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbObject);
                glx::glBufferDataARB(GL_ARRAY_BUFFER_ARB,
                                     nVertices * vertexDesc.stride,
                                     vertices,
                                     GL_STATIC_DRAW_ARB);
            }
        }
    }

    if (vbObject != 0)
    {
        glx::glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbObject);
        rc.setVertexArrays(vertexDesc, NULL);
    }
    else
    {
        rc.setVertexArrays(vertexDesc, vertices);
    }

    uint32 lastMaterial = ~0u;

    // Iterate over all primitive groups in the mesh
    for (vector<PrimitiveGroup*>::const_iterator iter = groups.begin();
         iter != groups.end(); iter++)
    {
        // Set up the material
        const Material* mat = NULL;
        uint32 materialIndex = (*iter)->materialIndex;
        if (materialIndex != lastMaterial && materialIndex < materials.size())
            mat = materials[materialIndex];

        rc.setMaterial(mat);
        rc.drawGroup(**iter);
    }

    if (vbObject != 0)
        glx::glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}


AxisAlignedBox
Mesh::getBoundingBox() const
{
    AxisAlignedBox bbox;

    // Return an empty box if there's no position info
    if (vertexDesc.getAttribute(Position).format != Float3)
        return bbox;

    char* vdata = reinterpret_cast<char*>(vertices) + vertexDesc.getAttribute(Position).offset;

    if (vertexDesc.getAttribute(PointSize).format == Float1)
    {
        // Handle bounding box calculation for point sprites. Unlike other
        // primitives, point sprite vertices have a non-zero size.
        int pointSizeOffset = (int) vertexDesc.getAttribute(PointSize).offset -
            (int) vertexDesc.getAttribute(Position).offset;
        
        for (uint32 i = 0; i < nVertices; i++, vdata += vertexDesc.stride)
        {
            Point3f center = Point3f(reinterpret_cast<float*>(vdata));
            float pointSize = (reinterpret_cast<float*>(vdata + pointSizeOffset))[0];
            Vec3f offsetVec(pointSize, pointSize, pointSize);

            AxisAlignedBox pointbox(center - offsetVec, center + offsetVec);
            bbox.include(pointbox);
        }
    }
    else
    {
        for (uint32 i = 0; i < nVertices; i++, vdata += vertexDesc.stride)
            bbox.include(Point3f(reinterpret_cast<float*>(vdata)));
    }

    return bbox;
}


void
Mesh::transform(Vec3f translation, float scale)
{
    if (vertexDesc.getAttribute(Position).format != Float3)
        return;

    char* vdata = reinterpret_cast<char*>(vertices) + vertexDesc.getAttribute(Position).offset;
    uint32 i;

    // Scale and translate the vertex positions
    for (i = 0; i < nVertices; i++, vdata += vertexDesc.stride)
    {
        Vec3f tv = (Vec3f(reinterpret_cast<float*>(vdata)) + translation) * scale;
        reinterpret_cast<float*>(vdata)[0] = tv.x;
        reinterpret_cast<float*>(vdata)[1] = tv.y;
        reinterpret_cast<float*>(vdata)[2] = tv.z;
    }

    // Point sizes need to be scaled as well
    if (vertexDesc.getAttribute(PointSize).format == Float1)
    {
        vdata = reinterpret_cast<char*>(vertices) + vertexDesc.getAttribute(PointSize).offset;
        for (i = 0; i < nVertices; i++, vdata += vertexDesc.stride)
            reinterpret_cast<float*>(vdata)[0] *= scale;
    }
}


uint32
Mesh::getPrimitiveCount() const
{
    uint32 count = 0;

    for (vector<PrimitiveGroup*>::const_iterator iter = groups.begin();
         iter != groups.end(); iter++)
    {
        count += (*iter)->getPrimitiveCount();
    }

    return count;
}


Mesh::PrimitiveGroupType
Mesh::parsePrimitiveGroupType(const string& name)
{
    if (name == "trilist")
        return TriList;
    else if (name == "tristrip")
        return TriStrip;
    else if (name == "trifan")
        return TriFan;
    else if (name == "linelist")
        return LineList;
    else if (name == "linestrip")
        return LineStrip;
    else if (name == "points")
        return PointList;
    else if (name == "sprites")
        return SpriteList;
    else
        return InvalidPrimitiveGroupType;
}


Mesh::VertexAttributeSemantic
Mesh::parseVertexAttributeSemantic(const string& name)
{
    if (name == "position")
        return Position;
    else if (name == "normal")
        return Normal;
    else if (name == "color0")
        return Color0;
    else if (name == "color1")
        return Color1;
    else if (name == "tangent")
        return Tangent;
    else if (name == "texcoord0")
        return Texture0;
    else if (name == "texcoord1")
        return Texture1;
    else if (name == "texcoord2")
        return Texture2;
    else if (name == "texcoord3")
        return Texture3;
    else if (name == "pointsize")
        return PointSize;
    else
        return InvalidSemantic;
}


Mesh::VertexAttributeFormat
Mesh::parseVertexAttributeFormat(const string& name)
{
    if (name == "f1")
        return Float1;
    else if (name == "f2")
        return Float2;
    else if (name == "f3")
        return Float3;
    else if (name == "f4")
        return Float4;
    else if (name == "ub4")
        return UByte4;
    else
        return InvalidFormat;
}


Mesh::TextureSemantic
Mesh::parseTextureSemantic(const string& name)
{
    if (name == "texture0")
        return DiffuseMap;
    else if (name == "normalmap")
        return NormalMap;
    else if (name == "specularmap")
        return SpecularMap;
    else if (name == "emissivemap")
        return EmissiveMap;
    else
        return InvalidTextureSemantic;
}


uint32
Mesh::getVertexAttributeSize(VertexAttributeFormat fmt)
{
    switch (fmt)
    {
    case Float1:
    case UByte4:
        return 4;
    case Float2:
        return 8;
    case Float3:
        return 12;
    case Float4:
        return 16;
    default:
        return 0;
    }
}
