// renderglsl.h
//
// Functions for rendering objects using dynamically generated GLSL shaders.
//
// Copyright (C) 2006-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_RENDERGLSL_H_
#define _CELENGINE_RENDERGLSL_H_

#include <celengine/lightenv.h>
#include <Eigen/Geometry>


void renderEllipsoid_GLSL(const RenderInfo& ri,
                       const LightingState& ls,
                       Atmosphere* atmosphere,
                       float cloudTexOffset,
                       const Eigen::Vector3f& semiAxes,
                       unsigned int textureRes,
                       int renderFlags,
                       const Eigen::Quaternionf& planetOrientation,
                       const Frustum& frustum,
                       const GLContext& context);
                       
void renderGeometry_GLSL(Geometry* geometry,
                         const RenderInfo& ri,
                         ResourceHandle texOverride,
                         const LightingState& ls,
                         const Atmosphere* atmosphere,
                         float geometryScale,
                         int renderFlags,
                         const Eigen::Quaternionf& planetOrientation,
                         double tsec);
                      
void renderClouds_GLSL(const RenderInfo& ri,
                       const LightingState& ls,
                       Atmosphere* atmosphere,
                       Texture* cloudTex,
                       Texture* cloudNormalMap,
                       float texOffset,
                       const Eigen::Vector3f& semiAxes,
                       unsigned int textureRes,
                       int renderFlags,
                       const Eigen::Quaternionf& planetOrientation,
                       const Frustum& frustum,
                       const GLContext& context);
                       
void renderAtmosphere_GLSL(const RenderInfo& ri,
                           const LightingState& ls,
                           Atmosphere* atmosphere,
                           float radius,
                           const Eigen::Quaternionf& planetOrientation,
                           const Frustum& frustum,
                           const GLContext& context);
                           
void renderRings_GLSL(RingSystem& rings,
                      RenderInfo& ri,
                      const LightingState& ls,
                      float planetRadius,
                      float planetOblateness,
                      unsigned int textureResolution,
                      bool renderShadow,
                      unsigned int nSections);

void renderGeometry_GLSL_Unlit(Geometry* geometry,
                               const RenderInfo& ri,
                               ResourceHandle texOverride,
                               float geometryScale,
                               int renderFlags,
                               const Eigen::Quaternionf& planetOrientation,
                               double tsec);


class FramebufferObject
{
public:
    enum
    {
        ColorAttachment = 0x1,
        DepthAttachment = 0x2
    };
    FramebufferObject(GLuint width, GLuint height, unsigned int attachments);
    ~FramebufferObject();

    bool isValid() const;
    GLuint width() const
    {
        return m_width;
    }

    GLuint height() const
    {
        return m_height;
    }

    GLuint colorTexture() const;
    GLuint depthTexture() const;

    bool bind();
    bool unbind();



private:
    void generateColorTexture();
    void generateDepthTexture();
    void generateFbo(unsigned int attachments);
    void cleanup();

private:
    GLuint m_width;
    GLuint m_height;
    GLuint m_colorTexId;
    GLuint m_depthTexId;
    GLuint m_fboId;
    GLenum m_status;
};


#endif // _CELENGINE_RENDERGLSL_H_
                       


