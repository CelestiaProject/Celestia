// galaxy.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel, Fridger Schrempp, and Toti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iterator>
#include <map>
#include <optional>
#include <ostream>
#include <utility>
#include <vector>

#include <fmt/printf.h>

#include <celmath/ellipsoid.h>
#include <celmath/intersect.h>
#include <celmath/randutils.h>
#include <celmath/ray.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include "galaxy.h"
#include "glsupport.h"
#include "image.h"
#include "pixelformat.h"
#include "render.h"
#include "texture.h"
#include "vecgl.h"

namespace vecgl = celestia::vecgl;

namespace
{

struct Blob
{
    Eigen::Vector4f position;
    unsigned int    colorIndex;
    float           brightness;
};

bool operator<(const Blob& b1, const Blob& b2)
{
    return (b1.position.squaredNorm() < b2.position.squaredNorm());
}

using BlobVector = std::vector<Blob>;

} // end unnamed namespace


class GalacticForm
{
public:
    BlobVector blobs;
    Eigen::Vector3f scale;
};


namespace
{

constexpr int width = 128;
constexpr int height = 128;
constexpr unsigned int GALAXY_POINTS = 3500;

// TODO: This value is just a guess.
// To be optimal, it should actually be computed:
constexpr float RADIUS_CORRECTION     = 0.025f;
constexpr float MAX_SPIRAL_THICKNESS  = 0.06f;

bool formsInitialized = false;

constexpr std::size_t GalacticFormsReserve = 32;

std::vector<std::optional<GalacticForm>> galacticForms;
std::map<fs::path, std::size_t> customForms;

Texture* galaxyTex = nullptr;
Texture* colorTex  = nullptr;

struct GalaxyTypeName
{
    const char* name;
    GalaxyType type;
};

constexpr const GalaxyTypeName GalaxyTypeNames[] =
{
    { "Irr", GalaxyType::Irr },
    { "S0",  GalaxyType::S0 },
    { "Sa",  GalaxyType::Sa },
    { "Sb",  GalaxyType::Sb },
    { "Sc",  GalaxyType::Sc },
    { "SBa", GalaxyType::SBa },
    { "SBb", GalaxyType::SBb },
    { "SBc", GalaxyType::SBc },
    { "E0",  GalaxyType::E0 },
    { "E1",  GalaxyType::E1 },
    { "E2",  GalaxyType::E2 },
    { "E3",  GalaxyType::E3 },
    { "E4",  GalaxyType::E4 },
    { "E5",  GalaxyType::E5 },
    { "E6",  GalaxyType::E6 },
    { "E7",  GalaxyType::E7 },
};

void galaxyTextureEval(float u, float v, float /*w*/, std::uint8_t *pixel)
{
    float r = 0.9f - std::sqrt(u * u + v * v );
    if (r < 0)
        r = 0;

    auto pixVal = static_cast<std::uint8_t>(r * 255.99f);
    pixel[0] = 255;//65;
    pixel[1] = 255;//64;
    pixel[2] = 255;//65;
    pixel[3] = pixVal;
}

void colorTextureEval(float u, float /*v*/, float /*w*/, std::uint8_t *pixel)
{
    unsigned int i = static_cast<unsigned int>((static_cast<float>(u)*0.5f + 0.5f)*255.99f); // [-1, 1] -> [0, 255]

    // generic Hue profile as deduced from true-color imaging for spirals
    // Hue in degrees
    float hue = 25.0f * std::tanh(0.0615f * (27.0f - static_cast<float>(i)));
    if (i >= 28) hue += 220.0f;
    //convert Hue to RGB
    float r, g, b;
    DeepSkyObject::hsv2rgb(&r, &g, &b, hue, 0.20f, 1.0f);
    pixel[0] = static_cast<std::uint8_t>(r * 255.99f);
    pixel[1] = static_cast<std::uint8_t>(g * 255.99f);
    pixel[2] = static_cast<std::uint8_t>(b * 255.99f);
}

struct GalaxyVertex
{
    Eigen::Vector4f position;
    Eigen::Matrix<GLushort, 4, 1> texCoord; // texCoord.x = x, texCoord.y = y, texCoord.z = color index, texCoord.w = alpha
};

void draw(const GalaxyVertex *v, std::size_t count, const GLushort *indices)
{
    glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                          4, GL_FLOAT, GL_FALSE,
                          sizeof(GalaxyVertex), v[0].position.data());
    glVertexAttribPointer(CelestiaGLProgram::TextureCoord0AttributeIndex,
                          4, GL_UNSIGNED_SHORT, GL_FALSE,
                          sizeof(GalaxyVertex), v[0].texCoord.data());
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, indices);
}

GalaxyVertex *g_vertices = nullptr;
GLushort *g_indices = nullptr;
constexpr const std::size_t maxPoints = 8192; // 256k buffer

std::optional<GalacticForm> buildGalacticForm(const fs::path& filename)
{
    Blob b;
    BlobVector galacticPoints;

    // Load templates in standard .png format
    int width, height, rgb, j = 0, kmin = 9;
    unsigned char value;
    float h = 0.75f;
    Image* img = LoadImageFromFile(filename);
    if (img == nullptr)
    {
        celestia::util::GetLogger()->error("The galaxy template *** {} *** could not be loaded!\n", filename);
        return std::nullopt;
    }
    width  = img->getWidth();
    height = img->getHeight();
    rgb    = img->getComponents();

    auto& rng = celmath::getRNG();
    for (int i = 0; i < width * height; i++)
    {
        value = img->getPixels()[rgb * i];
        if (value > 10)
        {
            float idxf = static_cast<float>(i);
            float wf = static_cast<float>(width);
            float hf = static_cast<float>(height);
            float z = std::floor(idxf / wf);
            float x = (idxf - wf * z - 0.5f * (wf - 1.0f)) / wf;
            z  = (0.5f * (hf - 1.0f) - z) / hf;
            x  += celmath::RealDists<float>::SignedUnit(rng) * 0.008f;
            z  += celmath::RealDists<float>::SignedUnit(rng) * 0.008f;
            float r2 = x * x + z * z;

            float y;
            if (filename != "models/E0.png")
            {
                float y0 = 0.5f * MAX_SPIRAL_THICKNESS * std::sqrt(static_cast<float>(value)/256.0f) * std::exp(- 5.0f * r2);
                float B = (r2 > 0.35f) ? 1.0f: 0.75f; // the darkness of the "dust lane", 0 < B < 1
                float p0 = 1.0f - B * std::exp(-h * h); // the uniform reference probability, envelopping prob*p0.
                float yr, prob;
                do
                {
                    // generate "thickness" y of spirals with emulation of a dust lane
                    // in galctic plane (y=0)

                    yr =  celmath::RealDists<float>::SignedUnit(rng) * h;
                    prob = (1.0f - B * exp(-yr * yr))/p0;
                } while (celmath::RealDists<float>::Unit(rng) > prob);
                b.brightness  = value * prob;
                y = y0 * yr / h;
            }
            else
            {
                // generate spherically symmetric distribution from E0.png
                float yy, prob;
                do
                {
                    yy = celmath::RealDists<float>::SignedUnit(rng);
                    float ry2 = 1.0f - yy * yy;
                    prob = ry2 > 0 ? std::sqrt(ry2): 0.0f;
                } while (celmath::RealDists<float>::Unit(rng) > prob);
                y = yy * std::sqrt(0.25f - r2) ;
                b.brightness  = value;
                kmin = 12;
            }

            b.position    = Eigen::Vector4f(x, y, z, 1.0f);
            unsigned int rr =  static_cast<unsigned int>(b.position.head(3).norm() * 511);
            b.colorIndex  = rr < 256 ? rr : 255;
            galacticPoints.push_back(b);
            j++;
        }
    }

    delete img;
    galacticPoints.reserve(j);

    // sort to start with the galaxy center region (x^2 + y^2 + z^2 ~ 0), such that
    // the biggest (brightest) sprites will be localized there!

    std::sort(galacticPoints.begin(), galacticPoints.end());

    // reshuffle the galaxy points randomly...except the first kmin+1 in the center!
    // the higher that number the stronger the central "glow"

    std::shuffle(galacticPoints.begin() + kmin, galacticPoints.end(), celmath::getRNG());

    std::optional<GalacticForm> galacticForm(std::in_place);
    galacticForm->blobs = std::move(galacticPoints);
    galacticForm->scale = Eigen::Vector3f::Ones();

    return galacticForm;
}

void initializeForms()
{
    // Irregular Galaxies
    unsigned int galaxySize = GALAXY_POINTS, ip = 0;
    Blob b;
    Eigen::Vector3f p;

    BlobVector irregularPoints;
    irregularPoints.reserve(galaxySize);

    auto& rng = celmath::getRNG();
    while (ip < galaxySize)
    {
        p = Eigen::Vector3f(celmath::RealDists<float>::SignedUnit(rng),
                            celmath::RealDists<float>::SignedUnit(rng),
                            celmath::RealDists<float>::SignedUnit(rng));
        float r  = p.norm();
        if (r < 1)
        {
            float prob = (1 - r) * (celmath::fractalsum(Eigen::Vector3f(p.x() + 5, p.y() + 5, p.z() + 5), 8) + 1) * 0.5f;
            if (celmath::RealDists<float>::Unit(rng) < prob)
            {
                b.position   = Eigen::Vector4f(p.x(), p.y(), p.z(), 1.0f);
                b.brightness = 64u;
                auto rr      = static_cast<unsigned int>(r * 511);
                b.colorIndex = rr < 256 ? rr : 255;
                irregularPoints.push_back(b);
                ++ip;
            }
        }
    }

    GalacticForm& irregularForm = *galacticForms.emplace_back(std::in_place);
    irregularForm.blobs = std::move(irregularPoints);
    irregularForm.scale = Eigen::Vector3f::Constant(0.5f);

    // Spiral Galaxies, 7 classical Hubble types

    galacticForms.reserve(GalacticFormsReserve);

    galacticForms.push_back(buildGalacticForm("models/S0.png"));
    galacticForms.push_back(buildGalacticForm("models/Sa.png"));
    galacticForms.push_back(buildGalacticForm("models/Sb.png"));
    galacticForms.push_back(buildGalacticForm("models/Sc.png"));
    galacticForms.push_back(buildGalacticForm("models/SBa.png"));
    galacticForms.push_back(buildGalacticForm("models/SBb.png"));
    galacticForms.push_back(buildGalacticForm("models/SBc.png"));

    // Elliptical Galaxies , 8 classical Hubble types, E0..E7,
    //
    // To save space: generate spherical E0 template from S0 disk
    // via rescaling by (1.0f, 3.8f, 1.0f).

    for (unsigned int eform  = 0; eform <= 7; ++eform)
    {
        float ell = 1.0f - static_cast<float>(eform) / 8.0f;

        // note the correct x,y-alignment of 'ell' scaling!!
        // build all elliptical templates from rescaling E0

        galacticForms.push_back(buildGalacticForm("models/E0.png"));
        if (!galacticForms.back().has_value()) { continue; }

        GalacticForm& ellipticalForm = *galacticForms.back();
        ellipticalForm.scale = Eigen::Vector3f(ell, ell, 1.0f);

        for (Blob& blob : ellipticalForm.blobs)
        {
            blob.colorIndex = static_cast<unsigned int>(std::ceil(0.76f * static_cast<float>(blob.colorIndex)));
        }
    }

    formsInitialized = true;
}

std::size_t customForm(const fs::path& path)
{
    auto iter = customForms.find(path);
    if (iter != customForms.end())
    {
        return iter->second;
    }

    std::size_t result = galacticForms.size();
    customForms[path] = result;
    galacticForms.push_back(buildGalacticForm(path));
    return result;
}

} // end unnamed namespace


float Galaxy::lightGain  = 0.0f;

float Galaxy::getDetail() const
{
    return detail;
}

void Galaxy::setDetail(float d)
{
    detail = d;
}

const char* Galaxy::getType() const
{
    return GalaxyTypeNames[static_cast<std::size_t>(type)].name;
}

void Galaxy::setType(const std::string& typeStr)
{
    type = GalaxyType::Irr;
    auto iter = std::find_if(std::begin(GalaxyTypeNames), std::end(GalaxyTypeNames),
                             [&](const GalaxyTypeName& g) { return g.name == typeStr; });
    if (iter != std::end(GalaxyTypeNames))
        type = iter->type;
}

void Galaxy::setForm(const std::string& customTmpName)
{
    if (!formsInitialized)
        initializeForms();

    if (customTmpName.empty())
    {
        form = static_cast<std::size_t>(type);
    }
    else
    {
        form = customForm(fs::path("models") / customTmpName);
    }
}

std::string Galaxy::getDescription() const
{
    return fmt::sprintf(_("Galaxy (Hubble type: %s)"), getType());
}

const char* Galaxy::getObjTypeName() const
{
    return "galaxy";
}

bool Galaxy::pick(const Eigen::ParametrizedLine<double, 3>& ray,
                  double& distanceToPicker,
                  double& cosAngleToBoundCenter) const
{
    if (!galacticForms[form].has_value() || !isVisible())
        return false;

    GalacticForm& galacticForm = *galacticForms[form];

    // The ellipsoid should be slightly larger to compensate for the fact
    // that blobs are considered points when galaxies are built, but have size
    // when they are drawn.
    float yscale = (type > GalaxyType::Irr && type < GalaxyType::E0)
        ? MAX_SPIRAL_THICKNESS
        : galacticForm.scale.y() + RADIUS_CORRECTION;
    Eigen::Vector3d ellipsoidAxes(getRadius()*(galacticForm.scale.x() + RADIUS_CORRECTION),
                                  getRadius()* yscale,
                                  getRadius()*(galacticForm.scale.z() + RADIUS_CORRECTION));

    Eigen::Matrix3d rotation = getOrientation().cast<double>().toRotationMatrix();
    return celmath::testIntersection(
        celmath::transformRay(Eigen::ParametrizedLine<double, 3>(ray.origin() - getPosition(), ray.direction()),
                              rotation),
        celmath::Ellipsoidd(ellipsoidAxes),
        distanceToPicker,
        cosAngleToBoundCenter);
}

bool Galaxy::load(AssociativeArray* params, const fs::path& resPath)
{
    double detail = 1.0;
    params->getNumber("Detail", detail);
    setDetail(static_cast<float>(detail));

    std::string customTmpName;
    params->getString("CustomTemplate", customTmpName);

    std::string typeName;
    params->getString("Type", typeName);
    setType(typeName);
    setForm(customTmpName);

    return DeepSkyObject::load(params, resPath);
}

void Galaxy::render(const Eigen::Vector3f& offset,
                    const Eigen::Quaternionf& viewerOrientation,
                    float brightness,
                    float pixelSize,
                    const Matrices& ms,
                    Renderer* renderer)
{
    if (!galacticForms[form].has_value())
        return;

    const GalacticForm& galacticForm = *galacticForms[form];

    /* We'll first see if the galaxy's apparent size is big enough to
       be noticeable on screen; if it's not we'll break right here,
       avoiding all the overhead of the matrix transformations and
       GL state changes: */
    float distanceToDSO = offset.norm() - getRadius();
    if (distanceToDSO < 0)
        distanceToDSO = 0;

    float minimumFeatureSize = pixelSize * distanceToDSO;
    float size  = 2 * getRadius();

    if (size < minimumFeatureSize)
        return;

    auto *prog = renderer->getShaderManager().getShader("galaxy");
    if (prog == nullptr)
        return;

    if (galaxyTex == nullptr)
    {
        galaxyTex = CreateProceduralTexture(width, height, celestia::PixelFormat::RGBA,
                                            galaxyTextureEval);
    }
    assert(galaxyTex != nullptr);
    glActiveTexture(GL_TEXTURE0);
    galaxyTex->bind();

    if (colorTex == nullptr)
    {
        colorTex = CreateProceduralTexture(256, 1, celestia::PixelFormat::RGBA,
                                           colorTextureEval,
                                           Texture::EdgeClamp,
                                           Texture::NoMipMaps);
    }
    assert(colorTex != nullptr);
    glActiveTexture(GL_TEXTURE1);
    colorTex->bind();

    Eigen::Matrix3f viewMat = viewerOrientation.conjugate().toRotationMatrix();
    Eigen::Vector4f v0(Eigen::Vector4f::Zero());
    Eigen::Vector4f v1(Eigen::Vector4f::Zero());
    Eigen::Vector4f v2(Eigen::Vector4f::Zero());
    Eigen::Vector4f v3(Eigen::Vector4f::Zero());
    v0.head(3) = viewMat * Eigen::Vector3f(-1, -1, 0) * size;
    v1.head(3) = viewMat * Eigen::Vector3f( 1, -1, 0) * size;
    v2.head(3) = viewMat * Eigen::Vector3f( 1,  1, 0) * size;
    v3.head(3) = viewMat * Eigen::Vector3f(-1,  1, 0) * size;

    Eigen::Quaternionf orientation = getOrientation().conjugate();
    Eigen::Matrix3f mScale = galacticForm.scale.asDiagonal() * size;
    Eigen::Matrix3f mLinear = orientation.toRotationMatrix() * mScale;

    Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
    m.topLeftCorner(3,3) = mLinear;
    m.block<3,1>(0, 3) = offset;

    int pow2 = 1;

    const BlobVector& points = galacticForm.blobs;
    unsigned int nPoints = static_cast<unsigned int>(points.size() * std::clamp(getDetail(), 0.0f, 1.0f));
    // corrections to avoid excessive brightening if viewed e.g. edge-on

    float brightness_corr = 1.0f;
    float cosi;

    if (type < GalaxyType::E0 || type > GalaxyType::E3) //all galaxies, except ~round elliptics
    {
        cosi = (orientation * Eigen::Vector3f::UnitY()).dot(offset) / offset.norm();
        brightness_corr = std::sqrt(std::abs(cosi));
        if (brightness_corr < 0.2f)
            brightness_corr = 0.2f;
    }
    if (type > GalaxyType::E3) // only elliptics with higher ellipticities
    {
        cosi = (orientation * Eigen::Vector3f::UnitX()).dot(offset) / offset.norm();
        brightness_corr = brightness_corr * std::abs(cosi);
        if (brightness_corr < 0.45f)
            brightness_corr = 0.45f;
    }

    Eigen::Matrix4f mv = vecgl::translate(*ms.modelview, Eigen::Vector3f(-offset));

    const float btot = (type == GalaxyType::Irr || type >= GalaxyType::E0) ? 2.5f : 5.0f;
    const float spriteScaleFactor = 1.0f / 1.55f;

    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glEnableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);

    if (g_vertices == nullptr)
        g_vertices = new GalaxyVertex[maxPoints];
    if (g_indices == nullptr)
        g_indices = new GLushort[(maxPoints / 4 + 1) * 6];

    GLushort j = 0;

    prog->use();
    prog->setMVPMatrices(*ms.projection, mv);
    prog->samplerParam("galaxyTex") = 0;
    prog->samplerParam("colorTex") = 1;

    std::size_t vertex = 0, index = 0;
    for (unsigned int i = 0; i < nPoints; ++i)
    {
        if ((i & pow2) != 0)
        {
            pow2 <<= 1;
            size *= spriteScaleFactor;
            v0 *= spriteScaleFactor;
            v1 *= spriteScaleFactor;
            v2 *= spriteScaleFactor;
            v3 *= spriteScaleFactor;
            if (size < minimumFeatureSize)
                break;
        }

        const Blob& b = points[i];
        Eigen::Vector4f p  = m * b.position;
        float br = b.brightness / 255.0f;

        float screenFrac = size / p.norm();
        if (screenFrac < 0.1f)
        {
            float a = (4.0f * lightGain + 1.0f) * btot * (0.1f - screenFrac) * brightness_corr * brightness * br;
            GLushort alpha = static_cast<GLushort>(std::min(1.0f, a) * 65535.99f);
            GLushort color = static_cast<GLushort>(b.colorIndex);
            g_vertices[vertex++] = { p + v0, { 0, 0, color, alpha } };
            g_vertices[vertex++] = { p + v1, { 1, 0, color, alpha } };
            g_vertices[vertex++] = { p + v2, { 1, 1, color, alpha } };
            g_vertices[vertex++] = { p + v3, { 0, 1, color, alpha } };

            g_indices[index++] = j;
            g_indices[index++] = j + 1;
            g_indices[index++] = j + 2;
            g_indices[index++] = j;
            g_indices[index++] = j + 2;
            g_indices[index++] = j + 3;
            j += 4;

            if (vertex + 4 > maxPoints)
            {
                draw(g_vertices, index, g_indices);
                index = 0;
                vertex = 0;
                j = 0;
            }
        }
    }

    if (index > 0)
        draw(g_vertices, index, g_indices);

    glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
    glActiveTexture(GL_TEXTURE0);
}

std::uint64_t Galaxy::getRenderMask() const
{
    return Renderer::ShowGalaxies;
}

unsigned int Galaxy::getLabelMask() const
{
    return Renderer::GalaxyLabels;
}

void Galaxy::increaseLightGain()
{
    lightGain  += 0.05f;
    if (lightGain > 1.0f)
        lightGain  = 1.0f;
}

void Galaxy::decreaseLightGain()
{
    lightGain  -= 0.05f;
    if (lightGain < 0.0f)
        lightGain  = 0.0f;
}

float Galaxy::getLightGain()
{
    return lightGain;
}

void Galaxy::setLightGain(float lg)
{
    lightGain = std::clamp(lg, 0.0f, 1.0f);
}

std::ostream& operator<<(std::ostream& s, const GalaxyType& sc)
{
    return s << GalaxyTypeNames[static_cast<std::size_t>(sc)].name;
}
