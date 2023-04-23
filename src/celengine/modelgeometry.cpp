// modelgeometry.cpp
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <vector>
#include <utility>
#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include "glsupport.h"
#include "modelgeometry.h"
#include "rendcontext.h"

using celestia::util::GetLogger;
namespace gl = celestia::gl;
namespace util = celestia::util;

namespace
{

constexpr gl::VertexObject::DataType GLComponentTypes[static_cast<std::size_t>(cmod::VertexAttributeFormat::FormatMax)] =
{
     gl::VertexObject::DataType::Float,         // Float1
     gl::VertexObject::DataType::Float,         // Float2
     gl::VertexObject::DataType::Float,         // Float3
     gl::VertexObject::DataType::Float,         // Float4,
     gl::VertexObject::DataType::UnsignedByte,  // UByte4
};

constexpr int GLComponentCounts[static_cast<std::size_t>(cmod::VertexAttributeFormat::FormatMax)] =
{
     1,  // Float1
     2,  // Float2
     3,  // Float3
     4,  // Float4,
     4,  // UByte4
};

constexpr bool GLComponentNormalized[static_cast<std::size_t>(cmod::VertexAttributeFormat::FormatMax)] =
{
     false,  // Float1
     false,  // Float2
     false,  // Float3
     false,  // Float4,
     true,  // UByte4
};

constexpr int
convert(cmod::VertexAttributeSemantic semantic)
{
    switch (semantic)
    {
    case cmod::VertexAttributeSemantic::Position:
        return CelestiaGLProgram::VertexCoordAttributeIndex;
    case cmod::VertexAttributeSemantic::Normal:
        return CelestiaGLProgram::NormalAttributeIndex;
    case cmod::VertexAttributeSemantic::Color0:
        return CelestiaGLProgram::ColorAttributeIndex;
    case cmod::VertexAttributeSemantic::Texture0:
        return CelestiaGLProgram::TextureCoord0AttributeIndex;
    case cmod::VertexAttributeSemantic::Tangent:
        return CelestiaGLProgram::TangentAttributeIndex;
    case cmod::VertexAttributeSemantic::PointSize:
        return CelestiaGLProgram::PointSizeAttributeIndex;
    default:
        return -1; // other attributes are not supported
        break;
    }
}

void
setVertexArrays(gl::VertexObject &vao, const gl::Buffer &vbo, const cmod::VertexDescription& desc)
{
    for (const auto &attribute : desc.attributes)
    {
        if (attribute.semantic == cmod::VertexAttributeSemantic::InvalidSemantic)
            continue;

        vao.addVertexBuffer(
            vbo,
            convert(attribute.semantic),
            GLComponentCounts[static_cast<std::size_t>(attribute.format)],
            GLComponentTypes[static_cast<std::size_t>(attribute.format)],
            GLComponentNormalized[static_cast<std::size_t>(attribute.format)],
            desc.strideBytes,
            attribute.offsetWords * sizeof(cmod::VWord));
    }
}

} // anonymous namespace


class ModelOpenGLData
{
public:
    std::vector<gl::Buffer> vbos; // vertex buffer objects
    std::vector<gl::Buffer> vios; // vertex index objects
    std::vector<gl::VertexObject> vaos; // vertex attributes
};


/** Create a new ModelGeometry wrapping the specified model.
  * The ModelGeoemtry takes ownership of the model.
  */
ModelGeometry::ModelGeometry(std::unique_ptr<cmod::Model>&& model) :
    m_model(std::move(model)),
    m_glData(std::make_unique<ModelOpenGLData>())
{
}


// Needs to be defined at a point where ModelOpenGLData is complete
ModelGeometry::~ModelGeometry() = default;


bool
ModelGeometry::pick(const Eigen::ParametrizedLine<double, 3>& r, double& distance) const
{
    return m_model->pick(r.origin(), r.direction(), distance);
}


/*! Render the model; the time parameter is ignored right now
 *  since this class doesn't currently support animation.
 */
void
ModelGeometry::render(RenderContext& rc, double /* t */)
{
    // The first time the mesh is rendered, we will try and place the
    // vertex data in a vertex buffer object and potentially get a huge
    // rendering performance boost.  This can consume a great deal of
    // memory, since we're duplicating the vertex data.  TODO: investigate
    // the possibility of deleting the original data.  We can always map
    // read-only later on for things like picking, but this could be a low
    // performance path.
    if (!m_vbInitialized)
    {
        m_vbInitialized = true;

        std::vector<cmod::Index32> indices;
        for (unsigned int i = 0; i < m_model->getMeshCount(); ++i)
        {
            const cmod::Mesh* mesh = m_model->getMesh(i);
            const cmod::VertexDescription& vertexDesc = mesh->getVertexDescription();

            m_glData->vbos.emplace_back(
                gl::Buffer::TargetHint::Array,
                util::array_view<const void>(
                    mesh->getVertexData(),
                    mesh->getVertexCount() * vertexDesc.strideBytes));

            indices.reserve(std::max(indices.capacity(), static_cast<std::size_t>(mesh->getIndexCount())));
            for (unsigned int groupIndex = 0; groupIndex < mesh->getGroupCount(); ++groupIndex)
            {
                const auto* group = mesh->getGroup(groupIndex);
                std::copy(group->indices.begin(), group->indices.end(), std::back_inserter(indices));
            }
            m_glData->vios.emplace_back(gl::Buffer::TargetHint::ElementArray, indices);
            indices.clear();

            gl::VertexObject vao;
            setVertexArrays(vao, m_glData->vbos.back(), mesh->getVertexDescription());
            vao.setIndexBuffer(m_glData->vios.back(), 0, gl::VertexObject::IndexType::UnsignedInt);
            m_glData->vaos.emplace_back(std::move(vao));
        }
    }

    unsigned int lastMaterial = ~0u;
    unsigned int materialCount = m_model->getMaterialCount();

    // Iterate over all meshes in the model
    for (unsigned int meshIndex = 0; meshIndex < m_model->getMeshCount(); ++meshIndex)
    {
        const cmod::Mesh* mesh = m_model->getMesh(meshIndex);

        if (meshIndex >= m_glData->vbos.size())
        {
            GetLogger()->error(_("Mesh index {} is higher than VBO count {}!"), meshIndex, m_glData->vbos.size());
            return;
        }

        // Iterate over all primitive groups in the mesh
        for (unsigned int groupIndex = 0; groupIndex < mesh->getGroupCount(); ++groupIndex)
        {
            const cmod::PrimitiveGroup* group = mesh->getGroup(groupIndex);
            rc.updateShader(mesh->getVertexDescription(), group->prim);

            // Set up the material
            const cmod::Material* material = nullptr;
            unsigned int materialIndex = group->materialIndex;
            if (materialIndex != lastMaterial && materialIndex < materialCount)
            {
                material = m_model->getMaterial(materialIndex);
            }

            rc.setMaterial(material);
            rc.drawGroup(m_glData->vaos[meshIndex], *group);
        }
    }
}


bool
ModelGeometry::isOpaque() const
{
    return m_model->isOpaque();
}


bool
ModelGeometry::isNormalized() const
{
    return m_model->isNormalized();
}


bool
ModelGeometry::usesTextureType(cmod::TextureSemantic t) const
{
    return m_model->usesTextureType(t);
}


void
ModelGeometry::loadTextures()
{
#if 0
    for (const auto m : materials)
    {
        if (m->maps[Mesh::DiffuseMap] != InvalidResource)
            GetTextureManager()->find(m->maps[Mesh::DiffuseMap]);
        if (m->maps[Mesh::NormalMap] != InvalidResource)
            GetTextureManager()->find(m->maps[Mesh::NormalMap]);
        if (m->maps[Mesh::SpecularMap] != InvalidResource)
            GetTextureManager()->find(m->maps[Mesh::SpecularMap]);
        if (m->maps[Mesh::EmissiveMap] != InvalidResource)
            GetTextureManager()->find(m->maps[Mesh::EmissiveMap]);
    }
#endif
}
