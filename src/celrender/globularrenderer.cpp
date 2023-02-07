// globularrenderer.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel, Fridger Schrempp, and Toti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cmath>
#include <memory>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celengine/globular.h>
#include <celengine/glsupport.h>
#include <celengine/pixelformat.h>
#include <celengine/render.h>
#include <celengine/shadermanager.h>
#include <celengine/texture.h>
#include <celmath/geomutil.h>
#include <celmath/randutils.h>
#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>
#include <celutil/color.h>
#include "globularrenderer.h"

namespace gl = celestia::gl;
namespace util = celestia::util;

namespace celestia::render
{
namespace
{
constexpr int cntrTexWidth  = 512;
constexpr int cntrTexHeight = 512;
constexpr int starTexWidth  = 128;
constexpr int starTexHeight = 128;

constexpr unsigned int kGlobularPoints = 8192u;

constexpr float kLumiShape = 3.0f;

// P1 determines the zoom level, where individual cluster stars start to appear.
// The smaller P2 (< 1), the faster stars show up when resolution increases.
constexpr float P1 = 65.0f, P2 = 0.75f;

constexpr float kSpriteScaleFactor = 1.0f/1.25f;

float RRatio, XI; // TODO: get rid of these global variables

/// Globular Form

struct GlobularForm
{
    struct Blob
    {
        Eigen::Vector3f position;
        float           radius_2d;
        std::uint8_t    colorIndex;
    };

    using BlobVector = std::vector<Blob>;

    std::vector<Blob> gblobs{ };
    mutable gl::Buffer bo{ util::NoCreateT{} };
    mutable gl::VertexObject vo{ util::NoCreateT{} };
    mutable bool GLDataInitialized{ false };
};

/// GlobularForm Manager
class GlobularFormManager
{
public:
    GlobularFormManager()
    {
        initializeForms();
        centerTex.fill(nullptr);
    }

    const GlobularForm* getForm(int) const;
    Texture* getCenterTex(int);
    Texture* getGlobularTex();
    Texture* getColorTex();

    static GlobularFormManager* get();

private:
    void initializeForms();

    std::array<GlobularForm, Globular::GlobularBuckets> globularForms{ };
    std::array<Texture*, Globular::GlobularBuckets> centerTex{ };
    std::unique_ptr<Texture> globularTex{ nullptr };
    std::unique_ptr<Texture> colorTex{ nullptr };
};

float
relStarDensity(float eta)
{
    constexpr float RRatio_min = 50.11872336272722f; // 10 ** 1.7

    /*! As alpha blending weight (relStarDensity) I take the theoretical
     *  number of globular stars in 2d projection at a distance
     *  rho = r / r_c = eta * r_t from the center (cf. King_1962's Eq.(18)),
     *  divided by the area = PI * rho * rho . This number density of stars
     *  I normalized to 1 at rho=0.

     *  The resulting blending weight increases strongly -> 1 if the
     *  2d number density of stars rises, i.e for rho -> 0.

     *  Since the central "cloud" is due to lack of visual resolution,
     *  rather than cluster morphology, we limit it's size by
     *  taking max(C_ref, CBin). Smaller c gives a shallower distribution!
     */

    float rRatio = std::max(RRatio_min, RRatio);
    float Xi = 1.0f / std::sqrt(1.0f + rRatio * rRatio);
    float XI2 = Xi * Xi;
    float rho2 = 1.0001f + eta * eta * rRatio * rRatio; //add 1e-4 as regulator near rho=0

    return ((std::log(rho2) + 4.0f * (1.0f - std::sqrt(rho2)) * Xi) / (rho2 - 1.0f) + XI2) / (1.0f - 2.0f * Xi + XI2);
}

void
centerCloudTexEval(float u, float v, float /*w*/, std::uint8_t *pixel)
{
    /*! For reasons of speed, calculate central "cloud" texture only for
     *  8 bins of King_1962 concentration, c = CBin, XI(CBin), RRatio(CBin).
     */

    // Skyplane projected King_1962 profile at center (rho = eta = 0):
    float c2d = 1.0f - XI;

    // eta^2 = u * u  + v * v = 1 is the biggest circle fitting into the quadratic
    // procedural texture. Hence clipping
    float eta = std::min(1.0f, std::hypot(u, v));  // u,v = (-1..1)

    // eta = 1 corresponds to tidalRadius:

    float rho   = eta  * RRatio;
    float rho2  = 1.0f + rho * rho;

    // Skyplane projected King_1962 profile (Eq.(14)), vanishes for eta = 1:
    // i.e. absolutely no globular stars for r > tidalRadius:

    float profile_2d = (1.0f / std::sqrt(rho2) - 1.0f)/c2d + 1.0f;
    profile_2d = profile_2d * profile_2d;

    *pixel = static_cast<std::uint8_t>(relStarDensity(eta) * profile_2d * 255.99f);
}

void
colorTextureEval(float u, float /*v*/, float /*w*/, std::uint8_t *pixel)
{
    auto i = static_cast<int>((u * 0.5f + 0.5f) * 255.99f); // [-1, 1] -> [0, 255]

    // Build RGB color table, using hue, saturation, value as input.
    // Hue in degrees.

    // Location of hue transition and saturation peak in color index space:
    constexpr int i0 = 36;
    constexpr int i_satmax = 16;
    // Width of hue transition in color index space:
    constexpr float i_width = 3.0f;

    constexpr float sat_l = 0.08f;
    constexpr float sat_h = 0.1f;
    constexpr float hue_r = 27.0f;
    constexpr float hue_b = 220.0f;

    if (i == 255)
    {
        // Red Giant star color: i = 255:
        Color::fromHSV(25.0f, 0.65f, 1.0f).get(pixel);
    }
    else
    {
        // normal stars: i < 255, generic color profile for now, improve later
        // simple qualitative saturation profile:
        // i_satmax is value of i where sat = sat_h + sat_l maximal

        float x = static_cast<float>(i) / static_cast<float>(i_satmax);
        float sat = sat_l + 2 * sat_h /(x + 1.0f / x);

        // Fast transition from hue_r to hue_b at i = i0 within a width
        // i_width in color index space:

        float hue = hue_r + 0.5f * (hue_b - hue_r) * (std::tanh(static_cast<float>(i - i0) / i_width) + 1.0f);

        // Prevent green stars
        if (hue > 60.0 && hue < 180.0)
            sat = 0.0f;

        Color::fromHSV(hue, sat, 0.85f).get(pixel);
    }
}

void
initGlobularData(gl::Buffer &bo, gl::VertexObject &vo, const GlobularForm::BlobVector &points)
{
    struct GlobularVtx
    {
        Eigen::Matrix<short, 3, 1> position;
        std::array<std::uint8_t, 3> texCoord; // reuse it for starSize, relStarDensity and colorIndex
    };

    std::vector<GlobularVtx> globularVtx;
    globularVtx.reserve(4 + points.size());

    // Reuse the buffer for a tidal. Tidal uses color index 0.
    globularVtx.push_back({{-32767, -32767, 0}, {  0,   0, 0}});
    globularVtx.push_back({{ 32767, -32767, 0}, {255,   0, 0}});
    globularVtx.push_back({{ 32767,  32767, 0}, {255, 255, 0}});
    globularVtx.push_back({{-32767,  32767, 0}, {  0, 255, 0}});

    // regarding used constants:
    // pow2 = 128;           // Associate "Red Giants" with the 128 biggest star-sprites
    // starSize = br * 0.5f; // Maximal size of star sprites -> "Red Giants"
    // br = 2 * brightness, where `brightness' is passed to Globular::render()

    float starSize = 0.5f;
    std::size_t pow2 = 128;
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        /*! Note that the [axis,angle] input in globulars.dsc transforms the
         *  2d projected star distance r_2d in the globular frame to refer to the
         *  skyplane frame for each globular! That's what I need here.
         *
         *  The [axis,angle] input will be needed anyway, when upgrading to
         *  account for  ellipticities, with corresponding  inclinations and
         *  position angles...
         */

        if ((i & pow2) != 0)
        {
            pow2    <<= 1;
            starSize *= kSpriteScaleFactor;
        }

        const auto &b = points[i];
        GlobularVtx vtx;
        vtx.position    = (b.position * 32767.99f).cast<short>();
        vtx.texCoord[0] = static_cast<std::uint8_t>(starSize * 255.99f);
        vtx.texCoord[1] = static_cast<std::uint8_t>(relStarDensity(b.radius_2d) * 255.99f);

        /* Colors of normal globular stars are given by color profile.
         * Associate orange "Red Giant" stars with the largest sprite
         * sizes (while pow2 = 128).
         */

        vtx.texCoord[2] = (pow2 < 256) ? 255 : b.colorIndex;

        globularVtx.push_back(vtx);
    }

    bo = gl::Buffer(gl::Buffer::TargetHint::Array, util::array_view<GlobularVtx>(globularVtx));
    vo = gl::VertexObject(gl::VertexObject::Primitive::Points);
    vo.addVertexBuffer(
        bo,
        CelestiaGLProgram::VertexCoordAttributeIndex,
        3,
        gl::VertexObject::DataType::Short,
        true,
        sizeof(GlobularVtx),
        offsetof(GlobularVtx, position));
    vo.addVertexBuffer(
        bo,
        CelestiaGLProgram::TextureCoord0AttributeIndex,
        3,
        gl::VertexObject::DataType::UnsignedByte,
        true,
        sizeof(GlobularVtx),
        offsetof(GlobularVtx, texCoord));
}

void
buildGlobularForm(GlobularForm& globularForm, float c)
{
    GlobularForm::BlobVector globularPoints;

    float rRatio = std::pow(10.0f, c); //  = r_t / r_c
    float cc = 1.0f + rRatio * rRatio;
    unsigned int i = 0, k = 0;

    // Value of King_1962 luminosity profile at center:

    float prob0 = std::sqrt(cc) - 1.0f;

    /*! Generate the globular star distribution randomly, according
     *  to the  King_1962 surface density profile f(r), eq.(14).
     *
     *  rho = r / r_c = eta r_t / r_c, 0 <= eta <= 1,
     *  coreRadius r_c, tidalRadius r_t, King concentration c = log10(r_t/r_c).
     */

    auto& rng = celmath::getRNG();
    rng.seed(1312);
    while (i < kGlobularPoints)
    {
        /*!
         *  Use a combination of the Inverse Transform method and
         *  Von Neumann's Acceptance-Rejection method for generating sprite stars
         *  with eta distributed according to the exact King luminosity profile.
         *
         * This algorithm leads to almost 100% efficiency for all values of
         * parameters and variables!
         */

        float uu = celmath::RealDists<float>::Unit(rng);

        /* First step: eta distributed as inverse power distribution (~1/Z^2)
         * that majorizes the exact King profile. Compute eta in terms of uniformly
         * distributed variable uu! Normalization to 1 for eta -> 0.
         */

        float eta = std::tan(uu * std::atan(rRatio)) / rRatio;

        float rho = eta * rRatio;
        float  cH = 1.0f/(1.0f + rho * rho);
        float   Z = std::sqrt((1.0f + rho * rho)/cc); // scaling variable

        // Express King_1962 profile in terms of the UNIVERSAL variable 0 < Z <= 1,

        float prob = (1.0f - 1.0f / Z) / prob0;
        prob = prob * prob;

        /* Second step: Use Acceptance-Rejection method (Von Neumann) for
         * correcting the power distribution of eta into the exact,
         * desired King form 'prob'!
         */

        k++;

        if (celmath::RealDists<float>::Unit(rng) < prob / cH)
        {
            /* Generate 3d points of globular cluster stars in polar coordinates:
             * Distribution in eta (<=> r) according to King's profile.
             * Uniform distribution on any spherical surface for given eta.
             * Note: u = cos(phi) must be used as a stochastic variable to get uniformity in angle!
             */
            float u = celmath::RealDists<float>::SignedUnit(rng);
            float theta = celmath::RealDists<float>::SignedFullAngle(rng);
            float sthetu2 = std::sin(theta) * std::sqrt(1.0f - u * u);

            // x,y,z points within -0.5..+0.5, as required for consistency:
            GlobularForm::Blob b;

            b.position = 0.5f * Eigen::Vector3f(eta * std::sqrt(1.0f - u * u) * std::cos(theta),
                                                eta * sthetu2,
                                                eta * u);

            /*
             * Note: 2d projection in x-z plane, according to Celestia's
             * conventions! Hence...
             */
            b.radius_2d = eta * std::sqrt(1.0f - sthetu2 * sthetu2);

            /* For now, implement only a generic spectrum for normal cluster
             * stars, modelled from Hubble photo of M80.
             * Blue Stragglers are qualitatively accounted for...
             * assume color index poportional to Z as function of which the King profile
             * becomes universal!
             */

            b.colorIndex = static_cast<std::uint8_t>(Z * 254);

            globularPoints.push_back(b);
            i++;
        }
    }

    globularForm.gblobs = std::move(globularPoints);
}

void
globularTextureEval(float u, float v, float /*w*/, std::uint8_t *pixel)
{
    // use an exponential luminosity shape for the individual stars
    // giving sort of a halo for the brighter (i.e.bigger) stars.

    static const float Lumi0 = std::exp(-kLumiShape);
    float lumi = std::max(0.0f, std::exp(-kLumiShape * std::hypot(u, v)) - Lumi0);

    *pixel = static_cast<std::uint8_t>(lumi * 255.99f);
}

const GlobularForm*
GlobularFormManager::getForm(int form) const
{
    return form < globularForms.size()
        ? &globularForms[form]
        : nullptr;
}

Texture*
GlobularFormManager::getCenterTex(int form)
{
    if(centerTex[form] == nullptr)
    {
        centerTex[form] = CreateProceduralTexture(cntrTexWidth, cntrTexHeight,
                                                  celestia::PixelFormat::LUMINANCE,
                                                  centerCloudTexEval).release();
    }

    assert(centerTex[form] != nullptr);
    return centerTex[form];
}

Texture*
GlobularFormManager::getGlobularTex()
{
    if (globularTex == nullptr)
    {
        globularTex = CreateProceduralTexture(starTexWidth, starTexHeight,
                                              celestia::PixelFormat::LUMINANCE,
                                              globularTextureEval);
    }
    assert(globularTex != nullptr);
    return globularTex.get();
}

Texture*
GlobularFormManager::getColorTex()
{
    if (colorTex == nullptr)
    {
        colorTex = CreateProceduralTexture(256, 1, celestia::PixelFormat::RGBA,
                                           colorTextureEval,
                                           Texture::EdgeClamp,
                                           Texture::NoMipMaps);
    }
    assert(colorTex != nullptr);
    return colorTex.get();
}

void
GlobularFormManager::initializeForms()
{
    // Define globularForms corresponding to 8 different bins of King concentration c
    for (unsigned int ic  = 0; ic < Globular::GlobularBuckets; ++ic)
    {
        float cbin = Globular::MinC + (0.5f + static_cast<float>(ic)) * Globular::BinWidth;
        buildGlobularForm(globularForms[ic], cbin);
    }
}

GlobularFormManager*
GlobularFormManager::get()
{
    static GlobularFormManager* globularFormManager = new GlobularFormManager();
    return globularFormManager;
}

Eigen::Vector4f
toVector4(const Eigen::Vector3f &v, float w)
{
    return Eigen::Vector4f(v.x(), v.y(), v.z(), w);
}

float
CalculateSpriteSize(int w, int h, const Eigen::Matrix4f &mvp, const Eigen::Matrix3f &viewMat)
{
    /*
     * in original code sprite was a quad with coordinates (v0, v1, v2, v3), where
     *   v2 = viewMat * vec3( 1, 1, 0),
     *   v3 = viewMat * vec3(-1, 1, 0).
     * So the width in world units is v2 - v3. The value remanes the same if we translate vertices by vec3(1, -1, 0).
     * Trtanslated values are:
     *   v2 = viewMat * vec3(2, 0, 0)
     *   v3 = viewMat * vec3(0, 0, 0)
     * Taking into account multiplication rules v2 becomes just 2 * viewMat.col(0) and v3 is just vec3(0, 0, 0).
     * To get normalized coordinates we convert v2 and v3 into vec4 and multiply by MVP.
     * As v3 is zero, then MVP * vec4(v3, 1) is equivalent to taking mvp.col(3).
     */
    Eigen::Vector4f v2 = mvp * toVector4(2.0f * viewMat.col(0), 1.0f);
    Eigen::Vector2f ndc2(v2.head(2) / v2.w());
    Eigen::Vector2f ndc3(mvp.col(3).head(2) / mvp(3, 3));
    Eigen::Vector2f dev(static_cast<float>(w), static_cast<float>(h));
    // ac - bc <=> (a - b)c
    return 0.5f * (ndc2 - ndc3).cwiseProduct(dev).norm();
}

int
CalculateSpriteCount(const GlobularForm* form, float detail, float starSize, float minimumFeatureSize)
{
    auto nPoints = static_cast<int>(static_cast<float>(form->gblobs.size()) * std::clamp(detail, 0.0f, 1.0f));

    for (int i = 128; i < nPoints; i<<=1)
    {
        starSize *= kSpriteScaleFactor;

        if (starSize < minimumFeatureSize)
        {
            nPoints = i;
            break;
        }
    }
    return nPoints;
}

} // anonymous namespace

struct GlobularRenderer::Object
{
    Object(const Eigen::Vector3f &offset, float brightness, float nearZ, float farZ, const Globular *globular) :
        offset(offset),
        brightness(brightness),
        nearZ(nearZ),
        farZ(farZ),
        globular(globular)
    {
    }

    Eigen::Vector3f offset; // distance to the globular
    float           brightness;
    float           nearZ;  // if nearZ != & farZ != then use custom projection matrix
    float           farZ;
    const Globular *globular;
};

GlobularRenderer::GlobularRenderer(Renderer &renderer) :
    m_renderer(renderer)
{
    m_objects.reserve(1024);
}

GlobularRenderer::~GlobularRenderer() = default; // define here as Object is not defined in the header file

void
GlobularRenderer::update(const Eigen::Quaternionf &viewerOrientation, float pixelSize, float fov)
{
    m_viewerOrientation = viewerOrientation;
    m_viewMat = viewerOrientation.conjugate().toRotationMatrix();
    m_pixelSize = pixelSize;
    m_fov = fov;
}

void
GlobularRenderer::add(const Globular *globular, const Eigen::Vector3f &offset, float brightness, float nearZ, float farZ)
{
    m_objects.emplace_back(offset, brightness, nearZ, farZ, globular);
}

void
GlobularRenderer::render()
{
    if (m_objects.empty())
        return;

    auto *tidalProg = m_renderer.getShaderManager().getShader("tidal");
    auto *globProg  = m_renderer.getShaderManager().getShader("globular");
    if (tidalProg == nullptr || globProg == nullptr)
        return;

    GlobularFormManager* globularFormManager = GlobularFormManager::get();

    glActiveTexture(GL_TEXTURE0);
    globularFormManager->getColorTex()->bind();

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.smoothLines = true;
    m_renderer.setPipelineState(ps);

#ifndef GL_ES
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif

    for (const auto &obj : m_objects)
        renderForm(tidalProg, globProg, obj);

    m_objects.clear();
#ifndef GL_ES
    glDisable(GL_POINT_SPRITE);
    glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
    glActiveTexture(GL_TEXTURE0);
}

void
GlobularRenderer::renderForm(CelestiaGLProgram *tidalProg, CelestiaGLProgram *globProg, const Object &obj) const
{
    const Globular *globular = obj.globular;
    auto* globularFormManager = GlobularFormManager::get();

    const auto *form = globularFormManager->getForm(globular->getFormId());
    if (form == nullptr)
        return;

    float radius = globular->getRadius();
    float distanceToDSO = std::max(0.0f, obj.offset.norm() - radius);
    float minimumFeatureSize = 0.5f * m_pixelSize * distanceToDSO;
    float diskSizeInPixels = radius / minimumFeatureSize;

    /*
     * Is the globular's apparent size big enough to
     * be noticeable on screen? If it's not, break right here to
     * avoid all the overhead of the matrix transformations and
     * GL state changes:
     */

    if (diskSizeInPixels < 1.0f)
        return;

    /*
     * When resolution (zoom) varies, the blended texture opacity is controlled by the
     * factor 'pixelWeight'. At low resolution, the latter starts at 1, but tends to 0,
     * if the resolution increases sufficiently (diskSizeInPixels >= P1 pixels)!
     * The smaller P2 (<1), the faster pixelWeight -> 0, for diskSizeInPixels >= P1.
     */

    float pixelWeight = 1.0f;
    if (diskSizeInPixels >= P1)
        pixelWeight = 1.0f / (P2 + (1.0f - P2) * diskSizeInPixels / P1);

    /* Use same 8 c-bins as in globularForms below!
     * center value of (ic+1)th c-bin
     */

    float cbin = Globular::MinC
           + (static_cast<float>(globular->getFormId()) + 0.5f) * Globular::BinWidth;

    RRatio = std::pow(10.0f, cbin);
    XI = 1.0f / std::sqrt(1.0f + RRatio * RRatio);

    float tidalSize = 2.0f * globular->getBoundingSphereRadius();

    /* Render central cloud sprite (centerTex). It fades away when
     * distance from center or resolution increases sufficiently.
     */

    gl::VertexObject &vo = form->vo;

    if (!form->GLDataInitialized)
    {
        initGlobularData(form->bo, vo, form->gblobs);
        form->GLDataInitialized = true;
    }

    glActiveTexture(GL_TEXTURE1);
    globularFormManager->getCenterTex(obj.globular->getFormId())->bind();

    Eigen::Matrix4f mv = celmath::translate(m_renderer.getModelViewMatrix(), obj.offset);
    Eigen::Matrix4f pr;
    if (obj.nearZ != 0.0f && obj.farZ != 0.0f)
        m_renderer.buildProjectionMatrix(pr, obj.nearZ, obj.farZ);
    else
        pr = m_renderer.getProjectionMatrix();

    tidalProg->use();
    tidalProg->setMVPMatrices(pr, mv);
    tidalProg->mat3Param("viewMat")      = m_viewMat;
    tidalProg->floatParam("brightness")  = obj.brightness;
    tidalProg->floatParam("pixelWeight") = pixelWeight;
    tidalProg->floatParam("tidalSize")   = tidalSize;
    tidalProg->samplerParam("colorTex")  = 0;
    tidalProg->samplerParam("tidalTex")  = 1;

    vo.draw(gl::VertexObject::Primitive::TriangleFan, 4);

    /*! Next, render globular cluster via distinct "star" sprites (globularTex)
     * for sufficiently large resolution and distance from center of globular.
     *
     * This RGBA texture fades away when resolution decreases (e.g. via automag!),
     * or when distance from globular center decreases.
     */
    glActiveTexture(GL_TEXTURE2);
    GlobularFormManager::get()->getGlobularTex()->bind();

    Eigen::Matrix3f mx = obj.globular->getOrientation().conjugate().toRotationMatrix() * Eigen::Scaling(tidalSize);

    int w, h; // NOSONAR
    m_renderer.getViewport(nullptr, nullptr, &w, &h);
    float size = CalculateSpriteSize(w, h, pr * mv, m_viewMat);

    globProg->use();
    globProg->setMVPMatrices(pr, mv);
    globProg->mat3Param("m")            = mx;
    globProg->vec3Param("offset")       = obj.offset;
    globProg->floatParam("brightness")  = obj.brightness;
    globProg->floatParam("pixelWeight") = pixelWeight;
    globProg->floatParam("scale")       = size * static_cast<float>(m_renderer.getScreenDpi()) / 96.0f;
    globProg->samplerParam("colorTex")  = 0;
    globProg->samplerParam("starTex")   = 2;

    vo.draw(gl::VertexObject::Primitive::Points, CalculateSpriteCount(form, globular->getDetail(), obj.brightness, minimumFeatureSize), 4);
}

} // namespace celestia::render
