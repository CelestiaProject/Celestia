// renderglsl.h
//
// Functions for rendering objects using dynamically generated GLSL shaders.
//
// Copyright (C) 2006-2007, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_RENDERGLSL_H_
#define _CELENGINE_RENDERGLSL_H_

#include <celengine/lightenv.h>

void renderSphere_GLSL(const RenderInfo& ri,
                       const LightingState& ls,
                       RingSystem* rings,
                       Atmosphere* atmosphere,
                       float cloudTexOffset,
                       float radius,
                       unsigned int textureRes,
                       int renderFlags,
                       const Mat4f& planetMat,
                       const Frustum& frustum,
                       const GLContext& context);
                       
void renderGeometry_GLSL(Geometry* geometry,
                         const RenderInfo& ri,
                         ResourceHandle texOverride,
                         const LightingState& ls,
                         const Atmosphere* atmosphere,
                         float geometryScale,
                         int renderFlags,
                         const Mat4f& planetMat,
                         double tsec);
                      
void renderClouds_GLSL(const RenderInfo& ri,
                       const LightingState& ls,
                       Atmosphere* atmosphere,
                       Texture* cloudTex,
                       Texture* cloudNormalMap,
                       float texOffset,
                       RingSystem* rings,
                       float radius,
                       unsigned int textureRes,
                       int renderFlags,
                       const Mat4f& planetMat,
                       const Frustum& frustum,
                       const GLContext& context);
                       
void renderAtmosphere_GLSL(const RenderInfo& ri,
                           const LightingState& ls,
                           Atmosphere* atmosphere,
                           float radius,
                           const Mat4f& planetMat,
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
                               const Mat4f& planetMat,
                               double tsec);

                           
#endif // _CELENGINE_RENDERGLSL_H_
                       


