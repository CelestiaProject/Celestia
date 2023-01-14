// modelgeometry.cpp
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <vector>
#include <utility>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include "glsupport.h"
#include "modelgeometry.h"
#include "rendcontext.h"

using celestia::util::GetLogger;

class ModelOpenGLData
{
public:
    ModelOpenGLData() = default;
    ModelOpenGLData(const ModelOpenGLData&) = delete;
    ModelOpenGLData(ModelOpenGLData&&) = default;
    ModelOpenGLData& operator=(const ModelOpenGLData&) = delete;
    ModelOpenGLData& operator=(ModelOpenGLData&&) = default;

    ~ModelOpenGLData()
    {
        glDeleteBuffers(vbos.size(), vbos.data());
        glDeleteBuffers(vios.size(), vios.data());
    }

    std::vector<GLuint> vbos; // vertex buffer objects
    std::vector<GLuint> vios; // vertex index objects
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

        for (unsigned int i = 0; i < m_model->getMeshCount(); ++i)
        {
            const cmod::Mesh* mesh = m_model->getMesh(i);
            const cmod::VertexDescription& vertexDesc = mesh->getVertexDescription();

            GLuint vboId = 0;
            glGenBuffers(1, &vboId);
            glBindBuffer(GL_ARRAY_BUFFER, vboId);
            glBufferData(GL_ARRAY_BUFFER,
                         mesh->getVertexCount() * vertexDesc.strideBytes,
                         mesh->getVertexData(),
                         GL_STATIC_DRAW);

            GLuint vioId = 0;
            glGenBuffers(1, &vioId);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vioId);

            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         mesh->getIndexCount() * sizeof(GL_UNSIGNED_INT),
                         nullptr,
                         GL_STATIC_DRAW);

            for (unsigned int offset = 0, groupIndex = 0; groupIndex < mesh->getGroupCount(); ++groupIndex)
            {
                const auto* group = mesh->getGroup(groupIndex);
                auto size = group->indices.size() * sizeof(GL_UNSIGNED_INT);
                glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, group->indices.data());
                offset += size;
            }

            m_glData->vbos.push_back(vboId);
            m_glData->vios.push_back(vioId);
        }
    }

    unsigned int lastMaterial = ~0u;
    unsigned int materialCount = m_model->getMaterialCount();

    // Iterate over all meshes in the model
    for (unsigned int meshIndex = 0; meshIndex < m_model->getMeshCount(); ++meshIndex)
    {
        const cmod::Mesh* mesh = m_model->getMesh(meshIndex);
        GLuint vboId = 0;
        GLuint vioId = 0;

        if (meshIndex < m_glData->vbos.size())
        {
            vboId = m_glData->vbos[meshIndex];
            vioId = m_glData->vios[meshIndex];
        }
        else
        {
            GetLogger()->error(_("Mesh index {} is higher than VBO count {}!"), meshIndex, m_glData->vbos.size());
        }

        glBindBuffer(GL_ARRAY_BUFFER, vboId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vioId);
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
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
