// modelgeometry.cpp
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <map>
#include <utility>

#include "glsupport.h"
#include "modelgeometry.h"
#include "rendcontext.h"


// Vertex buffer object support

// VBO optimization is only worthwhile for large enough vertex lists
static const unsigned int MinVBOSize = 4096;


class ModelOpenGLData
{
public:
    ModelOpenGLData() = default;

    ~ModelOpenGLData()
    {
        for (auto vboId : vbos)
        {
            if (vboId.second)
            {
                glDeleteBuffers(1, &vboId.second);
            }
        }
    }

    std::map<const void*, GLuint> vbos; // vertex buffer objects
};


/** Create a new ModelGeometry wrapping the specified model.
  * The ModelGeoemtry takes ownership of the model.
  */
ModelGeometry::ModelGeometry(std::unique_ptr<cmod::Model>&& model) :
    m_model(std::move(model)),
    m_glData(std::make_unique<ModelOpenGLData>())
{
}


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
            cmod::Mesh* mesh = m_model->getMesh(i);
            const cmod::VertexDescription& vertexDesc = mesh->getVertexDescription();

            GLuint vboId = 0;
            if (mesh->getVertexCount() * vertexDesc.stride > MinVBOSize)
            {
                glGenBuffers(1, &vboId);
                if (vboId != 0)
                {
                    glBindBuffer(GL_ARRAY_BUFFER, vboId);
                    glBufferData(GL_ARRAY_BUFFER,
                                    mesh->getVertexCount() * vertexDesc.stride,
                                    mesh->getVertexData(),
                                    GL_STATIC_DRAW);
                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                    m_glData->vbos[mesh->getVertexData()] = vboId;
                }
            }

            for (unsigned int groupIndex = 0; groupIndex < mesh->getGroupCount(); ++groupIndex)
            {
                const cmod::PrimitiveGroup* group = mesh->getGroup(groupIndex);
                if (group->vertexOverride != nullptr
                    && group->vertexCountOverride * group->vertexDescriptionOverride.stride > MinVBOSize)
                {
                    glGenBuffers(1, &vboId);
                    if (vboId != 0)
                    {
                        glBindBuffer(GL_ARRAY_BUFFER, vboId);
                        glBufferData(GL_ARRAY_BUFFER,
                                     group->vertexCountOverride * group->vertexDescriptionOverride.stride,
                                     group->vertexOverride, GL_STATIC_DRAW);
                        glBindBuffer(GL_ARRAY_BUFFER, 0);
                        m_glData->vbos[group->vertexOverride] = vboId;
                    }
                }
            }
        }
    }

    unsigned int lastMaterial = ~0u;
    unsigned int materialCount = m_model->getMaterialCount();

    // Iterate over all meshes in the model
    for (unsigned int meshIndex = 0; meshIndex < m_model->getMeshCount(); ++meshIndex)
    {
        cmod::Mesh* mesh = m_model->getMesh(meshIndex);

        const void* currentData = nullptr;
        GLuint currentVboId = 0;

        // Iterate over all primitive groups in the mesh
        for (unsigned int groupIndex = 0; groupIndex < mesh->getGroupCount(); ++groupIndex)
        {
            const cmod::PrimitiveGroup* group = mesh->getGroup(groupIndex);
            bool useOverrideValue = group->vertexOverride != nullptr && rc.shouldDrawLineAsTriangles();

            const void* data = useOverrideValue ? group->vertexOverride : mesh->getVertexData();
            auto vertexDescription = useOverrideValue ? group->vertexDescriptionOverride : mesh->getVertexDescription();

            if (currentData != data)
            {
                GLuint vboId = m_glData->vbos[data];
                if (vboId != 0)
                {
                    // Bind the vertex buffer object.
                    glBindBuffer(GL_ARRAY_BUFFER, vboId);
                    rc.setVertexArrays(vertexDescription, nullptr);
                }
                else
                {
                    if (currentVboId != 0)
                    {
                        glBindBuffer(GL_ARRAY_BUFFER, 0);
                    }
                    // No vertex buffer object; just use normal vertex arrays
                    rc.setVertexArrays(vertexDescription, data);
                }
                currentData = data;
                currentVboId = vboId;
            }

            rc.updateShader(vertexDescription, group->prim);

            // Set up the material
            const cmod::Material* material = nullptr;
            unsigned int materialIndex = group->materialIndex;
            if (materialIndex != lastMaterial && materialIndex < materialCount)
            {
                material = m_model->getMaterial(materialIndex);
            }

            rc.setMaterial(material);
            rc.drawGroup(*group, useOverrideValue);
        }

        // If we set a VBO, unbind it.
        if (currentVboId != 0)
        {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
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
