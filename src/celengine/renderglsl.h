// renderglsl.h
//
// Functions for rendering objects using dynamically generated GLSL shaders.
//
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// Atmosphere density is modeled with a exp(-y/H) falloff, where
// H is the scale height of the atmosphere. Thus atmospheres have
// infinite extent, but we still need to choose some finite sphere
// to render. The radius of the sphere is the height at which the
// density of the atmosphere falls to the extinction threshold, i.e.
// -H * ln(extinctionThreshold)
extern const double AtmosphereExtinctionThreshold;

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
                       
void renderModel_GLSL(Model* model,
                      const RenderInfo& ri,
                      ResourceHandle texOverride,
                      const LightingState& ls,
                      float radius,
                      const Mat4f& planetMat);
                      
void renderClouds_GLSL(const RenderInfo& ri,
                       const LightingState& ls,
                       Atmosphere* atmosphere,
                       Texture* cloudTex,
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
                           
                       


