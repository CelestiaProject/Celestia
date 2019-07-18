// modelgeometry.cpp
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "modelgeometry.h"
#include "rendcontext.h"
#include "texmanager.h"
#include <Eigen/Core>
#include <functional>
#include <algorithm>
#include <cassert>

using namespace cmod;
using namespace Eigen;
using namespace std;
using namespace celmath;


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
            if (vboId != 0)
            {
                glDeleteBuffers(1, &vboId);
            }
        }
    }

    std::vector<GLuint> vbos; // vertex buffer objects
};


/** Create a new ModelGeometry wrapping the specified model.
  * The ModelGeoemtry takes ownership of the model.
  */
ModelGeometry::ModelGeometry(unique_ptr<cmod::Model>&& model) :
    m_model(move(model)),
    m_glData(unique_ptr<ModelOpenGLData>(new ModelOpenGLData()))
{
}


bool
ModelGeometry::pick(const Ray3d& r, double& distance) const
{
    return m_model->pick(r.origin, r.direction, distance);
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
            Mesh* mesh = m_model->getMesh(i);
            const Mesh::VertexDescription& vertexDesc = mesh->getVertexDescription();

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
                }
            }

            m_glData->vbos.push_back(vboId);
        }
    }

    unsigned int lastMaterial = ~0u;
    unsigned int materialCount = m_model->getMaterialCount();

    // Iterate over all meshes in the model
    for (unsigned int meshIndex = 0; meshIndex < m_model->getMeshCount(); ++meshIndex)
    {
        Mesh* mesh = m_model->getMesh(meshIndex);
        GLuint vboId = 0;

        if (meshIndex < m_glData->vbos.size())
        {
            vboId = m_glData->vbos[meshIndex];
        }

        if (vboId != 0)
        {
            // Bind the vertex buffer object.
            glBindBuffer(GL_ARRAY_BUFFER, vboId);
            rc.setVertexArrays(mesh->getVertexDescription(), nullptr);
        }
        else
        {
            // No vertex buffer object; just use normal vertex arrays
            rc.setVertexArrays(mesh->getVertexDescription(), mesh->getVertexData());
        }

        // Iterate over all primitive groups in the mesh
        for (unsigned int groupIndex = 0; groupIndex < mesh->getGroupCount(); ++groupIndex)
        {
            const Mesh::PrimitiveGroup* group = mesh->getGroup(groupIndex);

            // Set up the material
            const Material* material = nullptr;
            unsigned int materialIndex = group->materialIndex;
            if (materialIndex != lastMaterial && materialIndex < materialCount)
            {
                material = m_model->getMaterial(materialIndex);
            }

            rc.setMaterial(material);
            rc.drawGroup(*group);
        }

        // If we set a VBO, unbind it.
        if (vboId != 0)
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
ModelGeometry::usesTextureType(Material::TextureSemantic t) const
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


string
CelestiaTextureResource::source() const
{
    if (m_textureHandle == InvalidResource)
        return "";

    const TextureInfo* t = GetTextureManager()->getResourceInfo(textureHandle());
    return t ? t->source : "";
}
