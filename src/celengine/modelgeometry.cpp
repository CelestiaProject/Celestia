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
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include "glsupport.h"
#include "modelgeometry.h"
#include "rendcontext.h"

using celestia::util::GetLogger;
namespace gl = celestia::gl;
namespace util = celestia::util;

class ModelOpenGLData
{
public:
    std::vector<gl::Buffer> vbos; // vertex buffer objects
    std::vector<gl::Buffer> vios; // vertex index objects
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

        m_glData->vbos[meshIndex].bind();
        m_glData->vios[meshIndex].bind();

        rc.setVertexArrays(mesh->getVertexDescription(), nullptr);

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
            rc.drawGroup(*group);
        }
    }

    gl::VertexObject::unbind();
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
