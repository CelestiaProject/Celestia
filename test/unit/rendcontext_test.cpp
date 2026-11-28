// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <doctest.h>

#include <vector>

#include <celengine/lightenv.h>
#include <celengine/rendcontext.h>
#include <celengine/shadermanager.h>

namespace
{

class MaterialContext : public RenderContext
{
public:
    MaterialContext() : RenderContext(nullptr) {}

    void makeCurrent(const cmod::Material& m) override
    {
        diffuse = convertMaterialColor(m.diffuse);
        specular = convertMaterialColor(m.specular);
        emissive = convertMaterialColor(m.emissive);
        opacity = m.opacity;
    }

    cmod::Color diffuse;
    cmod::Color specular;
    cmod::Color emissive;
    float opacity{ 1.0f };
};

class ScopedSRGBRendering
{
public:
    explicit ScopedSRGBRendering(bool enabled) : previous(celestia::gl::sRGBRendering)
    {
        celestia::gl::sRGBRendering = enabled;
    }

    ~ScopedSRGBRendering()
    {
        celestia::gl::sRGBRendering = previous;
    }

private:
    bool previous;
};

} // namespace

TEST_CASE("SSC texture overrides retain already-linear material colors")
{
    ScopedSRGBRendering srgb(true);
    MaterialContext context;
    cmod::Material material;
    material.diffuse = cmod::Color(Color(0.8f, 0.6f, 0.4f).linearize());
    material.specular = cmod::Color(Color(0.3f, 0.2f, 0.1f).linearize());
    material.opacity = 0.37f;

    context.setMaterial(&material, RenderContext::MaterialColorSpace::Linear);
    CHECK(context.diffuse == material.diffuse);
    CHECK(context.specular == material.specular);
    CHECK(context.opacity == material.opacity);

    context.lock();
    cmod::Material meshMaterial;
    context.setMaterial(&meshMaterial, RenderContext::MaterialColorSpace::sRGB);
    context.updateShader(cmod::VertexDescription{}, cmod::PrimitiveGroupType::TriList);
    CHECK(context.getMaterial() == &material);
    CHECK(context.diffuse == material.diffuse);
    CHECK(context.specular == material.specular);
}

TEST_CASE("Authored CMOD colors decode once after a linear override")
{
    ScopedSRGBRendering srgb(true);
    MaterialContext context;
    cmod::Material material;
    material.diffuse = cmod::Color(0.8f, 0.6f, 0.02f);
    material.specular = cmod::Color(0.3f, 0.2f, 0.1f);
    material.emissive = cmod::Color(0.1f, 0.02f, 0.01f);
    material.opacity = 0.37f;

    context.setMaterial(&material, RenderContext::MaterialColorSpace::Linear);
    context.setMaterial(&material, RenderContext::MaterialColorSpace::sRGB);
    CHECK(context.diffuse.red() == doctest::Approx(0.603827f));
    CHECK(context.diffuse.green() == doctest::Approx(0.318547f));
    CHECK(context.diffuse.blue() == doctest::Approx(0.00154799f));
    CHECK(context.specular.red() == doctest::Approx(0.073239f));
    CHECK(context.emissive.green() == doctest::Approx(0.00154799f));
    CHECK(context.opacity == material.opacity);
}

TEST_CASE("Legacy material colors are unchanged for either source color space")
{
    ScopedSRGBRendering srgb(false);
    MaterialContext context;
    cmod::Material material;
    material.diffuse = cmod::Color(0.8f, 0.6f, 0.02f);
    material.specular = cmod::Color(0.3f, 0.2f, 0.1f);
    material.emissive = cmod::Color(0.1f, 0.02f, 0.01f);

    for (auto colorSpace : {RenderContext::MaterialColorSpace::sRGB,
                            RenderContext::MaterialColorSpace::Linear})
    {
        context.setMaterial(&material, colorSpace);
        CHECK(context.diffuse == material.diffuse);
        CHECK(context.specular == material.specular);
        CHECK(context.emissive == material.emissive);
    }
}

TEST_CASE("Eclipse shadows from lights beyond MaxShaderLights are ignored")
{
    // Systems with more stars than the shaders support (#2565) have
    // LightingState::nLights > MaxShaderLights. Setting up the shader
    // properties must only touch the lights the shader actually uses;
    // before the fix this tripped the assert in
    // ShaderProperties::setEclipseShadowCountForLight.
    static_assert(MaxLights > MaxShaderLights);

    std::vector<EclipseShadow> shadows(1);
    LightingState ls;
    ls.nLights = MaxLights;
    for (unsigned int i = 0; i < MaxLights; ++i)
    {
        ls.lights[i].castsShadows = true;
        ls.shadows[i] = &shadows;
    }

    // Constructing the context sets up the per-light shadow properties.
    GLSL_RenderContext context(nullptr, ls, 1.0f, Eigen::Quaternionf::Identity(), nullptr, nullptr);
}
