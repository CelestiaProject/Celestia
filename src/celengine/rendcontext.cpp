// rendcontext.cpp
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "rendcontext.h"
#include "texmanager.h"
#include "gl.h"
#include "glext.h"
#include "vecgl.h"


static Mesh::Material defaultMaterial;

static GLenum GLPrimitiveModes[Mesh::PrimitiveTypeMax] = 
{
    GL_TRIANGLES,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLE_FAN,
    GL_LINES,
    GL_LINE_STRIP,
    GL_POINTS
};

static GLenum GLComponentTypes[Mesh::FormatMax] = 
{
     GL_FLOAT,          // Float1
     GL_FLOAT,          // Float2
     GL_FLOAT,          // Float3
     GL_FLOAT,          // Float4,
     GL_UNSIGNED_BYTE,  // UByte4
};

static int GLComponentCounts[Mesh::FormatMax] =
{
     1,  // Float1
     2,  // Float2
     3,  // Float3
     4,  // Float4,
     4,  // UByte4
};


enum {
    TangentAttributeIndex = 6,
};


static void
setStandardVertexArrays(const Mesh::VertexDescription& desc,
                        void* vertexData);
static void
setExtendedVertexArrays(const Mesh::VertexDescription& desc,
                        const void* vertexData);


RenderContext::RenderContext() :
    material(&defaultMaterial),
    locked(false)
{
}


RenderContext::RenderContext(const Mesh::Material* _material)
{
    if (_material == NULL)
        material = &defaultMaterial;
    else
        material = _material;
}


static void setVertexArrays(const Mesh::VertexDescription& desc)
{
}


void
RenderContext::setMaterial(const Mesh::Material* newMaterial)
{
    if (!locked)
    {
        if (newMaterial == NULL)
            newMaterial = &defaultMaterial;

        if (newMaterial != material)
        {
            material = newMaterial;
            makeCurrent(*material);
        }
    }
}


void
RenderContext::drawGroup(const Mesh::PrimitiveGroup& group)
{
    glDrawElements(GLPrimitiveModes[(int) group.prim],
                   group.nIndices,
                   GL_UNSIGNED_INT,
                   group.indices);
}


FixedFunctionRenderContext::FixedFunctionRenderContext() :
    RenderContext(),
    blendOn(false),
    specularOn(false)
{
}


FixedFunctionRenderContext::FixedFunctionRenderContext(const Mesh::Material* _material) :
    RenderContext(_material),
    blendOn(false),
    specularOn(false)
{
}


void
FixedFunctionRenderContext::makeCurrent(const Mesh::Material& m)
{
    Texture* t = NULL;
    if (m.maps[Mesh::DiffuseMap] != InvalidResource)
        t = GetTextureManager()->find(m.maps[Mesh::DiffuseMap]);

    if (t == NULL)
    {
        glDisable(GL_TEXTURE_2D);
    }
    else
    {
        glEnable(GL_TEXTURE_2D);
        t->bind();
    }

    bool blendOnNow = false;
    if (m.opacity != 1.0f || (t != NULL && t->hasAlpha()))
        blendOnNow = true;

    if (blendOnNow != blendOn)
    {
        blendOn = blendOnNow;
        if (blendOn)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
        }
        else
        {
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
        }
    }

    glColor4f(m.diffuse.red(),
              m.diffuse.green(),
              m.diffuse.blue(),
              m.opacity);

    if (m.specular == Color::Black)
    {
        float matSpecular[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        float zero = 0.0f;
        glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
        glMaterialfv(GL_FRONT, GL_SHININESS, &zero);
        specularOn = false;
    }
    else
    {
        float matSpecular[4] = { m.specular.red(),
                                 m.specular.green(),
                                 m.specular.blue(),
                                 1.0f };
        glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
        glMaterialfv(GL_FRONT, GL_SHININESS, &m.specularPower);
        specularOn = true;
    }

    {
        float matEmissive[4] = { m.emissive.red(),
                                 m.emissive.green(),
                                 m.emissive.blue(),
                                 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, matEmissive);
    }
}


void
FixedFunctionRenderContext::setVertexArrays(const Mesh::VertexDescription& desc, void* vertexData)
{
    setStandardVertexArrays(desc, vertexData);
}


void
VP_Combiner_RenderContext::setVertexArrays(const Mesh::VertexDescription& desc, void* vertexData)
{
    setStandardVertexArrays(desc, vertexData);
    setExtendedVertexArrays(desc, vertexData);
}


void
VP_FP_RenderContext::setVertexArrays(const Mesh::VertexDescription& desc, void* vertexData)
{
    setStandardVertexArrays(desc, vertexData);
    setExtendedVertexArrays(desc, vertexData);
}



void
setStandardVertexArrays(const Mesh::VertexDescription& desc,
                        void* vertexData)
{
    const Mesh::VertexAttribute& position  = desc.getAttribute(Mesh::Position);
    const Mesh::VertexAttribute& normal    = desc.getAttribute(Mesh::Normal);
    const Mesh::VertexAttribute& color0    = desc.getAttribute(Mesh::Color0);
    const Mesh::VertexAttribute& texCoord0 = desc.getAttribute(Mesh::Texture0);

    // Can't render anything unless we have positions
    if (position.format != Mesh::Float3)
        return;

    // Set up the vertex arrays
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, desc.stride,
                    reinterpret_cast<char*>(vertexData) + position.offset);

    // Set up the normal array
    switch (normal.format)
    {
    case Mesh::Float3:
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GLComponentTypes[(int) normal.format],
                        desc.stride,
                        reinterpret_cast<char*>(vertexData) + normal.offset);
        break;
    default:
        glDisableClientState(GL_NORMAL_ARRAY);
        break;
    }

    // Set up the color array
    switch (color0.format)
    {
    case Mesh::Float3:
    case Mesh::Float4:
    case Mesh::UByte4:
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(GLComponentCounts[color0.format],
                       GLComponentTypes[color0.format],
                       desc.stride,
                       reinterpret_cast<char*>(vertexData) + color0.offset);
        break;
    default:
        glDisableClientState(GL_COLOR_ARRAY);
        break;
    }

    // Set up the texture coordinate array
    switch (texCoord0.format)
    {
    case Mesh::Float1:
    case Mesh::Float2:
    case Mesh::Float3:
    case Mesh::Float4:
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(GLComponentCounts[(int) texCoord0.format],
                          GLComponentTypes[(int) texCoord0.format],
                          desc.stride,
                          reinterpret_cast<char*>(vertexData) + texCoord0.offset);
        break;
    default:
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        break;
    }
}


void
setExtendedVertexArrays(const Mesh::VertexDescription& desc,
                        const void* vertexData)
{
    const Mesh::VertexAttribute& tangent  = desc.getAttribute(Mesh::Tangent);
    const char* vertices = reinterpret_cast<const char*>(vertexData);

    switch (tangent.format)
    {
    case Mesh::Float3:
        glx::glEnableVertexAttribArrayARB(TangentAttributeIndex);
        glEnableClientState(GL_NORMAL_ARRAY);
        glx::glVertexAttribPointerARB(TangentAttributeIndex,
                                      GLComponentCounts[(int) tangent.format],
                                      GLComponentTypes[(int) tangent.format],
                                      GL_FALSE,
                                      desc.stride,
                                      vertices + tangent.offset);
        break;
    default:
        glx::glDisableVertexAttribArrayARB(TangentAttributeIndex);
        break;
    }
}


GLSL_RenderContext::GLSL_RenderContext(const LightingState& ls) :
    lightingState(ls)
{
}


GLSL_RenderContext::GLSL_RenderContext(const LightingState& ls,
				       const Mesh::Material* material) :
    lightingState(ls)
{
}
  


void
GLSL_RenderContext::makeCurrent(const Mesh::Material&)
{
}


void
GLSL_RenderContext::setVertexArrays(const Mesh::VertexDescription& desc,
				    void* vertexData)
{
}


