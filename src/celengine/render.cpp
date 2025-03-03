// render.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#define DEBUG_COALESCE               0
#define DEBUG_SECONDARY_ILLUMINATION 0
#define DEBUG_ORBIT_CACHE            0

#include <Eigen/Geometry>

#include <boost/container/static_vector.hpp>
#include <fmt/format.h>

#include "atmosphere.h"
#include "location.h"
#include "render.h"
#include "boundaries.h"
#include "dsorenderer.h"
#include "asterism.h"
#include "glshader.h"
#include "spheremesh.h"
#include "lodspheremesh.h"
#include "geometry.h"
#include "texmanager.h"
#include "meshmanager.h"
#include "renderinfo.h"
#include "renderglsl.h"
#include "axisarrow.h"
#include "frametree.h"
#include "timeline.h"
#include "timelinephase.h"
#include "skygrid.h"
#include "modelgeometry.h"
#include "curveplot.h"
#include "shadermanager.h"
#include "rectangle.h"
#include "framebuffer.h"
#include "planetgrid.h"
#include "pointstarvertexbuffer.h"
#include "pointstarrenderer.h"
#include "orbitsampler.h"
#include "rendcontext.h"
#include "textlayout.h"
#include <celastro/astro.h>
#include <celastro/date.h>
#include <celcompat/numbers.h>
#include <celengine/observer.h>
#include <celephem/rotation.h>
#include <celmath/frustum.h>
#include <celmath/distance.h>
#include <celmath/intersect.h>
#include <celmath/geomutil.h>
#include <celmath/vecgl.h>
#include <celrender/asterismrenderer.h>
#include <celrender/atmosphererenderer.h>
#include <celrender/boundariesrenderer.h>
#include <celrender/cometrenderer.h>
#include <celrender/eclipticlinerenderer.h>
#include <celrender/largestarrenderer.h>
#include <celrender/linerenderer.h>
#include <celrender/galaxyrenderer.h>
#include <celrender/globularrenderer.h>
#include <celrender/nebularenderer.h>
#include <celrender/openclusterrenderer.h>
#include <celrender/ringrenderer.h>
#include <celrender/skygridrenderer.h>
#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>
#include <celutil/logger.h>
#include <celutil/utf8.h>
#include <celutil/timer.h>
#include <celttf/truetypefont.h>
#include "glsupport.h"
#include <algorithm>
#include <cstring>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <numeric>
#ifdef _MSC_VER
#include <malloc.h>
#ifndef alloca
#define alloca(s) _alloca(s)
#endif
#endif

using namespace Eigen;
using namespace std;
using namespace celestia;
using namespace celestia::engine;
using namespace celestia::render;
using celestia::util::GetLogger;

namespace util = celestia::util;

static const int REF_DISTANCE_TO_SCREEN  = 400; //[mm]

// Contribution from planetshine beyond this distance (in units of object radius)
// is considered insignificant.
static const float PLANETSHINE_DISTANCE_LIMIT_FACTOR = 100.0f;

// Planetshine from objects less than this pixel size is treated as insignificant
// and will be ignored.
static const float PLANETSHINE_PIXEL_SIZE_LIMIT      =   0.1f;

// Fractional pixel offset used when rendering text as texture mapped
// quads to ensure consistent mapping of texels to pixels.
static const float PixelOffset = 0.125f;

// These two values constrain the near and far planes of the view frustum
// when rendering planet and object meshes.  The near plane will never be
// closer than MinNearPlaneDistance, and the far plane is set so that far/near
// will not exceed MaxFarNearRatio.
static const float MinNearPlaneDistance = 0.0001f; // km
static const float MaxFarNearRatio      = 2000000.0f;

static const float MinRelativeOccluderRadius = 0.005f;

// The minimum apparent size of an objects orbit in pixels before we display
// a label for it.  This minimizes label clutter.
static const float MinOrbitSizeForLabel = 20.0f;

// The minimum apparent size of a surface feature in pixels before we display
// a label for it.
static const float MinFeatureSizeForLabel = 20.0f;

/* The maximum distance of the observer to the origin of coordinates before
   asterism lines and labels start to linearly fade out (in light years) */
static const float MaxAsterismLabelsConstDist  = 6.0f;
static const float MaxAsterismLinesConstDist   = 600.0f;

/* The maximum distance of the observer to the origin of coordinates before
   asterisms labels and lines fade out completely (in light years) */
static const float MaxAsterismLabelsDist = 20.0f;
static const float MaxAsterismLinesDist  = 6.52e4f;

// Static meshes and textures used by all instances of Simulation

static bool commonDataInitialized = false;


LODSphereMesh* g_lodSphere = nullptr;

static Texture* gaussianDiscTex = nullptr;
static Texture* gaussianGlareTex = nullptr;

static const float CoronaHeight = 0.2f;

// Size at which the orbit cache will be flushed of old orbit paths
static const unsigned int OrbitCacheCullThreshold = 200;
// Age in frames at which unused orbit paths may be eliminated from the cache
static const uint32_t OrbitCacheRetireAge = 16;

Color Renderer::StarLabelColor          (0.471f, 0.356f, 0.682f);
Color Renderer::PlanetLabelColor        (0.407f, 0.333f, 0.964f);
Color Renderer::DwarfPlanetLabelColor   (0.557f, 0.235f, 0.576f);
Color Renderer::MoonLabelColor          (0.231f, 0.733f, 0.792f);
Color Renderer::MinorMoonLabelColor     (0.231f, 0.733f, 0.792f);
Color Renderer::AsteroidLabelColor      (0.596f, 0.305f, 0.164f);
Color Renderer::CometLabelColor         (0.768f, 0.607f, 0.227f);
Color Renderer::SpacecraftLabelColor    (0.93f,  0.93f,  0.93f);
Color Renderer::LocationLabelColor      (0.24f,  0.89f,  0.43f);
Color Renderer::GalaxyLabelColor        (0.0f,   0.45f,  0.5f);
Color Renderer::GlobularLabelColor      (0.8f,   0.45f,  0.5f);
Color Renderer::NebulaLabelColor        (0.541f, 0.764f, 0.278f);
Color Renderer::OpenClusterLabelColor   (0.239f, 0.572f, 0.396f);
Color Renderer::ConstellationLabelColor (0.225f, 0.301f, 0.36f);
Color Renderer::EquatorialGridLabelColor(0.64f,  0.72f,  0.88f);
Color Renderer::PlanetographicGridLabelColor(0.8f, 0.8f, 0.8f);
Color Renderer::GalacticGridLabelColor  (0.88f,  0.72f,  0.64f);
Color Renderer::EclipticGridLabelColor  (0.72f,  0.64f,  0.88f);
Color Renderer::HorizonGridLabelColor   (0.72f,  0.72f,  0.72f);

Color Renderer::StarOrbitColor          (0.5f,   0.5f,   0.8f);
Color Renderer::PlanetOrbitColor        (0.3f,   0.323f, 0.833f);
Color Renderer::DwarfPlanetOrbitColor   (0.557f, 0.235f, 0.576f);
Color Renderer::MoonOrbitColor          (0.08f,  0.407f, 0.392f);
Color Renderer::MinorMoonOrbitColor     (0.08f,  0.407f, 0.392f);
Color Renderer::AsteroidOrbitColor      (0.58f,  0.152f, 0.08f);
Color Renderer::CometOrbitColor         (0.639f, 0.487f, 0.168f);
Color Renderer::SpacecraftOrbitColor    (0.4f,   0.4f,   0.4f);
Color Renderer::SelectionOrbitColor     (1.0f,   0.0f,   0.0f);

Color Renderer::ConstellationColor      (0.0f,   0.24f,  0.36f);
Color Renderer::BoundaryColor           (0.24f,  0.10f,  0.12f);
Color Renderer::EquatorialGridColor     (0.28f,  0.28f,  0.38f);
Color Renderer::PlanetographicGridColor (0.8f,   0.8f,   0.8f);
Color Renderer::PlanetEquatorColor      (0.5f,   1.0f,   1.0f);
Color Renderer::GalacticGridColor       (0.38f,  0.38f,  0.28f);
Color Renderer::EclipticGridColor       (0.38f,  0.28f,  0.38f);
Color Renderer::HorizonGridColor        (0.38f,  0.38f,  0.38f);
Color Renderer::EclipticColor           (0.5f,   0.1f,   0.1f);

Color Renderer::SelectionCursorColor    (1.0f,   0.0f,   0.0f);

// Some useful unit conversions
inline float mmToInches(float mm)
{
    return mm * (1.0f / 25.4f);
}

inline float inchesToMm(float in)
{
    return in * 25.4f;
}


// Fade function for objects that shouldn't be shown when they're too small
// on screen such as orbit paths and some object labels. The will fade linearly
// from invisible at minSize pixels to full visibility at opaqueScale*minSize.
inline float sizeFade(float screenSize, float minScreenSize, float opaqueScale)
{
    return min(1.0f, (screenSize - minScreenSize) / (minScreenSize * (opaqueScale - 1)));
}

inline void glVertexAttrib(GLuint index, const Color &color)
{
#ifdef GL_ES
    glVertexAttrib4fv(index, color.toVector4().data());
#else
    glVertexAttrib4Nubv(index, color.data());
#endif
}

Renderer::Renderer() :
    windowWidth(0),
    windowHeight(0),
    fov(standardFOV),
    screenDpi(96),
    corrFac(1.12f),
    faintestAutoMag45deg(8.0f), //def. 7.0f
#ifndef GL_ES
    renderMode(GL_FILL),
#endif
    brightnessBias(0.0f),
    saturationMagNight(1.0f),
    saturationMag(1.0f),
    pointStarVertexBuffer(nullptr),
    glareVertexBuffer(nullptr),
    frameCount(0),
    lastOrbitCacheFlush(0),
    minOrbitSize(MinOrbitSizeForLabel),
    distanceLimit(1.0e6f),
    minFeatureSize(MinFeatureSizeForLabel),
    locationFilter(~0ull),
    settingsChanged(true),
    objectAnnotationSetOpen(false),
    m_atmosphereRenderer(std::make_unique<AtmosphereRenderer>(*this)),
    m_cometRenderer(std::make_unique<CometRenderer>(*this)),
    m_eclipticLineRenderer(std::make_unique<EclipticLineRenderer>(*this)),
    m_galaxyRenderer(std::make_unique<GalaxyRenderer>(*this)),
    m_globularRenderer(std::make_unique<GlobularRenderer>(*this)),
    m_largeStarRenderer(std::make_unique<LargeStarRenderer>(*this)),
    m_hollowMarkerRenderer(std::make_unique<LineRenderer>(*this, 1.0f, LineRenderer::PrimType::Lines, LineRenderer::StorageType::Static)),
    m_nebulaRenderer(std::make_unique<NebulaRenderer>(*this)),
    m_openClusterRenderer(std::make_unique<OpenClusterRenderer>(*this)),
    m_ringRenderer(std::make_unique<RingRenderer>(*this)),
    m_skyGridRenderer(std::make_unique<SkyGridRenderer>(*this))
{
    pointStarVertexBuffer = new PointStarVertexBuffer(*this, 2048);
    glareVertexBuffer = new PointStarVertexBuffer(*this, 2048);

    for (int i = 0; i < (int) FontCount; i++)
    {
        fonts[i] = nullptr;
    }
    shaderManager = new ShaderManager();
}


Renderer::~Renderer()
{
    delete pointStarVertexBuffer;
    delete glareVertexBuffer;
    delete shaderManager;

    m_atmosphereRenderer->deinitGL();
    m_cometRenderer->deinitGL();
    CurvePlot::deinit();
    PlanetographicGrid::deinit();
}


Renderer::DetailOptions::DetailOptions() :
    orbitPathSamplePoints(100),
    shadowTextureSize(256),
    eclipseTextureSize(128),
    orbitWindowEnd(0.5),
    orbitPeriodsShown(1.0),
    linearFadeFraction(0.0)
{
}


#if 0
// Not used yet.

// The RectToSpherical map converts XYZ coordinates to UV coordinates
// via a cube map lookup. However, a lot of GPUs don't support linear
// interpolation of textures with > 8 bits per component, which is
// inadequate precision for storing texture coordinates. To work around
// this, we'll store the u and v texture coordinates with two 8 bit
// coordinates each: rg for u, ba for v. The coordinates are unpacked
// as: u = r * 255/256 + g * 1/255
//     v = b * 255/256 + a * 1/255
// This gives an effective precision of 16 bits for each texture coordinate.
static void RectToSphericalMapEval(float x, float y, float z,
                                   unsigned char* pixel)
{
    // Compute spherical coodinates (r is always 1)
    double phi = asin(y);
    double theta = atan2(z, -x);

    // Convert to texture coordinates
    double u = (theta / PI + 1.0) * 0.5;
    double v = (-phi / PI + 0.5);

    // Pack texture coordinates in red/green and blue/alpha
    // u = red + green/256
    // v = blue* + alpha/256
    uint16_t rg = (uint16_t) (u * 65535.99);
    uint16_t ba = (uint16_t) (v * 65535.99);
    pixel[0] = rg >> 8;
    pixel[1] = rg & 0xff;
    pixel[2] = ba >> 8;
    pixel[3] = ba & 0xff;
}
#endif


static void BuildGaussianDiscMipLevel(unsigned char* mipPixels,
                                      unsigned int log2size,
                                      float fwhm,
                                      float power)
{
    unsigned int size = 1 << log2size;
    float sigma = fwhm / 2.3548f;
    float isig2 = 1.0f / (2.0f * sigma * sigma);
    // Store 1/sqrt(2*pi) in constexpr sfactor
    constexpr auto sfactor = static_cast<float>(0.5 * celestia::numbers::sqrt2 * celestia::numbers::inv_sqrtpi);
    float s = sfactor / sigma;

    for (unsigned int i = 0; i < size; i++)
    {
        float y = (float) i - size / 2;
        for (unsigned int j = 0; j < size; j++)
        {
            float x = (float) j - size / 2;
            float r2 = x * x + y * y;
            float f = s * std::exp(-r2 * isig2) * power;

            mipPixels[i * size + j] = (unsigned char) (255.99f * std::min(f, 1.0f));
        }
    }
}


static void BuildGlareMipLevel(unsigned char* mipPixels,
                               unsigned int log2size,
                               float scale,
                               float base)
{
    unsigned int size = 1 << log2size;

    for (unsigned int i = 0; i < size; i++)
    {
        float y = (float) i - size / 2;
        for (unsigned int j = 0; j < size; j++)
        {
            float x = (float) j - size / 2;
            float r = std::sqrt(x * x + y * y);
            float f = std::pow(base, r * scale);
            mipPixels[i * size + j] = (unsigned char) (255.99f * std::min(f, 1.0f));
        }
    }
}


#if 0
// An alternate glare function, based roughly on results in Spencer, G. et al,
// 1995, "Physically-Based Glare Effects for Digital Images"
static void BuildGlareMipLevel2(unsigned char* mipPixels,
                                unsigned int log2size,
                                float scale)
{
    unsigned int size = 1 << log2size;

    for (unsigned int i = 0; i < size; i++)
    {
        float y = (float) i - size / 2;
        for (unsigned int j = 0; j < size; j++)
        {
            float x = (float) j - size / 2;
            float r = (float) sqrt(x * x + y * y);
            float f = 0.3f / (0.3f + r * r * scale * scale * 100);
            /*
            if (i == 0 || j == 0 || i == size - 1 || j == size - 1)
                f = 1.0f;
            */
            mipPixels[i * size + j] = (unsigned char) (255.99f * min(f, 1.0f));
        }
    }
}
#endif


static Texture* BuildGaussianDiscTexture(unsigned int log2size)
{
    unsigned int size = 1 << log2size;
    Image* img = new Image(PixelFormat::Luminance, size, size, log2size + 1);

    for (unsigned int mipLevel = 0; mipLevel <= log2size; mipLevel++)
    {
        float fwhm = std::pow(2.0f, (float) (log2size - mipLevel)) * 0.3f;
        BuildGaussianDiscMipLevel(img->getMipLevel(mipLevel),
                                  log2size - mipLevel,
                                  fwhm,
                                  std::pow(2.0f, (float) (log2size - mipLevel)));
    }

    ImageTexture* texture = new ImageTexture(*img,
                                             Texture::EdgeClamp,
                                             Texture::DefaultMipMaps);

    delete img;

    return texture;
}


static Texture* BuildGaussianGlareTexture(unsigned int log2size)
{
    unsigned int size = 1 << log2size;
    Image* img = new Image(PixelFormat::Luminance, size, size, log2size + 1);

    for (unsigned int mipLevel = 0; mipLevel <= log2size; mipLevel++)
    {
        /*
        // Optional gaussian glare
        float fwhm = (float) pow(2.0f, (float) (log2size - mipLevel)) * 0.15f;
        float power = (float) pow(2.0f, (float) (log2size - mipLevel)) * 0.15f;
        BuildGaussianDiscMipLevel(img->getMipLevel(mipLevel),
                                  log2size - mipLevel,
                                  fwhm,
                                  power);
        */
        BuildGlareMipLevel(img->getMipLevel(mipLevel),
                           log2size - mipLevel,
                           25.0f / (float) pow(2.0f, (float) (log2size - mipLevel)),
                           0.66f);
        /*
        BuildGlareMipLevel2(img->getMipLevel(mipLevel),
                            log2size - mipLevel,
                            1.0f / (float) pow(2.0f, (float) (log2size - mipLevel)));
        */
    }

    ImageTexture* texture = new ImageTexture(*img,
                                             Texture::EdgeClamp,
                                             Texture::DefaultMipMaps);

    delete img;

    return texture;
}


static BodyClassification
translateLabelModeToClassMask(RenderLabels labelMode)
{
    BodyClassification classMask = BodyClassification::EmptyMask;

    if (util::is_set(labelMode, RenderLabels::PlanetLabels))
        classMask |= BodyClassification::Planet;
    if (util::is_set(labelMode, RenderLabels::DwarfPlanetLabels))
        classMask |= BodyClassification::DwarfPlanet;
    if (util::is_set(labelMode, RenderLabels::MoonLabels))
        classMask |= BodyClassification::Moon;
    if (util::is_set(labelMode, RenderLabels::MinorMoonLabels))
        classMask |= BodyClassification::MinorMoon;
    if (util::is_set(labelMode, RenderLabels::AsteroidLabels))
        classMask |= BodyClassification::Asteroid;
    if (util::is_set(labelMode, RenderLabels::CometLabels))
        classMask |= BodyClassification::Comet;
    if (util::is_set(labelMode, RenderLabels::SpacecraftLabels))
        classMask |= BodyClassification::Spacecraft;

    return classMask;
}


// Depth comparison function for render list entries
bool operator<(const RenderListEntry& a, const RenderListEntry& b)
{
    // Operation is reversed because -z axis points into the screen
    return a.centerZ - a.radius > b.centerZ - b.radius;
}


// Depth comparison for labels
// Note that it's essential to declare this operator as a member
// function of Renderer::Label; if it's not a class member, C++'s
// argument dependent lookup will not find the operator when it's
// used as a predicate for STL algorithms.
bool Renderer::Annotation::operator<(const Annotation& a) const
{
    // Operation is reversed because -z axis points into the screen
    return position.z() > a.position.z();
}

// Depth comparison for orbit paths
bool Renderer::OrbitPathListEntry::operator<(const Renderer::OrbitPathListEntry& o) const
{
    // Operation is reversed because -z axis points into the screen
    return centerZ - radius > o.centerZ - o.radius;
}


bool Renderer::init(int winWidth, int winHeight, const DetailOptions& _detailOptions)
{
    detailOptions = _detailOptions;

    m_atmosphereRenderer->initGL();
    if (!m_cometRenderer->initGL())
        return false;

    m_markerVO = std::make_unique<celestia::gl::VertexObject>();
    m_markerBO = std::make_unique<celestia::gl::Buffer>();

    // Initialize static meshes and textures common to all instances of Renderer
    if (!commonDataInitialized)
    {
        g_lodSphere = new LODSphereMesh();

        gaussianDiscTex = BuildGaussianDiscTexture(8);
        gaussianGlareTex = BuildGaussianGlareTexture(9);

        commonDataInitialized = true;
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

#ifndef GL_ES
    if (detailOptions.useMesaPackInvert && gl::MESA_pack_invert)
        glPixelStorei(GL_PACK_INVERT_MESA, GL_TRUE);
    else
        detailOptions.useMesaPackInvert = false;
#endif

    // LEQUAL rather than LESS required for multipass rendering
    glDepthFunc(GL_LEQUAL);

    resize(winWidth, winHeight);

    return true;
}


void Renderer::resize(int width, int height)
{
    windowWidth = width;
    windowHeight = height;
    projectionMode->setSize(static_cast<float>(windowWidth), static_cast<float>(windowHeight));
    // glViewport(windowWidth, windowHeight);
    m_orthoProjMatrix = math::Ortho2D(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);
}

void Renderer::setFieldOfView(float _fov)
{
    fov = _fov;
    corrFac = (0.12f * fov / standardFOV * fov / standardFOV + 1.0f);
}

int Renderer::getScreenDpi() const
{
    return screenDpi;
}

int Renderer::getWindowWidth() const
{
    return windowWidth;
}

int Renderer::getWindowHeight() const
{
    return windowHeight;
}

void Renderer::setScreenDpi(int _dpi)
{
    screenDpi = _dpi;
    projectionMode->setScreenDpi(_dpi);
}

float Renderer::getScaleFactor() const
{
    return screenDpi / 96.0f;
}

float Renderer::getPointWidth() const
{
    return 2.0f / windowWidth * getScaleFactor();
}

float Renderer::getPointHeight() const
{
    return 2.0f / windowHeight * getScaleFactor();
}

void Renderer::setFaintestAM45deg(float _faintestAutoMag45deg)
{
    faintestAutoMag45deg = _faintestAutoMag45deg;
    markSettingsChanged();
}

float Renderer::getFaintestAM45deg() const
{
    return faintestAutoMag45deg;
}

TextureResolution Renderer::getResolution() const
{
    return textureResolution;
}

void Renderer::enableSelectionPointer()
{
    showSelectionPointer = true;
}

void Renderer::disableSelectionPointer()
{
    showSelectionPointer = false;
}

void Renderer::setRTL(bool value)
{
    rtl = value;
}

bool Renderer::isRTL() const
{
    return rtl;
}

void Renderer::setResolution(TextureResolution resolution)
{
    textureResolution = resolution;
    markSettingsChanged();
}


std::shared_ptr<TextureFont> Renderer::getFont(FontStyle fs) const
{
    return fonts[(int) fs];
}

void Renderer::setFont(FontStyle fs, const std::shared_ptr<TextureFont>& font)
{
    fonts[(int) fs] = font;
    markSettingsChanged();
}

void Renderer::setRenderMode(RenderMode _renderMode)
{
#ifndef GL_ES
    switch(_renderMode)
    {
    case RenderMode::Fill:
        renderMode = GL_FILL;
        break;
    case RenderMode::Line:
        renderMode = GL_LINE;
        break;
    default:
        return;
    }
    markSettingsChanged();
#endif
}

RenderFlags Renderer::getRenderFlags() const
{
    return renderFlags;
}

void Renderer::setRenderFlags(RenderFlags _renderFlags)
{
    renderFlags = _renderFlags;
    updateBodyVisibilityMask();
    markSettingsChanged();
}

RenderLabels Renderer::getLabelMode() const
{
    return labelMode;
}

void Renderer::setLabelMode(RenderLabels _labelMode)
{
    labelMode = _labelMode;
    markSettingsChanged();
}

shared_ptr<celestia::engine::ProjectionMode> Renderer::getProjectionMode() const
{
    return projectionMode;
}

void Renderer::setProjectionMode(shared_ptr<celestia::engine::ProjectionMode> _projectionMode)
{
    projectionMode = _projectionMode;
    projectionMode->configureShaderManager(shaderManager);
    markSettingsChanged();
}

BodyClassification
Renderer::getOrbitMask() const
{
    return orbitMask;
}

void
Renderer::setOrbitMask(BodyClassification mask)
{
    orbitMask = mask;
    markSettingsChanged();
}

ColorTableType
Renderer::getStarColorTable() const
{
    return starColors.type();
}


void
Renderer::setStarColorTable(ColorTableType ct)
{
    starColors.setType(ct);
    markSettingsChanged();
}


bool Renderer::getVideoSync() const
{
    return true;
}

void Renderer::setVideoSync(bool /*sync*/)
{
}


float Renderer::getAmbientLightLevel() const
{
    return ambientLightLevel;
}


void Renderer::setAmbientLightLevel(float level)
{
    ambientLightLevel = level;
    markSettingsChanged();
}


float Renderer::getTintSaturation() const
{
    return tintSaturation;
}


void Renderer::setTintSaturation(float level)
{
    tintSaturation = level;
    markSettingsChanged();
}


float Renderer::getMinimumFeatureSize() const
{
    return minFeatureSize;
}


void Renderer::setMinimumFeatureSize(float pixels)
{
    minFeatureSize = pixels;
    markSettingsChanged();
}


float Renderer::getMinimumOrbitSize() const
{
    return minOrbitSize;
}

// Orbits and labels are only rendered when the orbit of the object
// occupies some minimum number of pixels on screen.
void Renderer::setMinimumOrbitSize(float pixels)
{
    minOrbitSize = pixels;
    markSettingsChanged();
}


float Renderer::getDistanceLimit() const
{
    return distanceLimit;
}


void Renderer::setDistanceLimit(float distanceLimit_)
{
    distanceLimit = distanceLimit_;
    markSettingsChanged();
}

void Renderer::getLabelAlignmentInfo(const Annotation &annotation, const TextureFont *font, TextLayout::HorizontalAlignment &halign, float &hOffset, float &vOffset) const
{
    switch (annotation.halign)
    {
    case LabelHorizontalAlignment::Center:
        halign = TextLayout::HorizontalAlignment::Center;
        hOffset = 0.0f;
        break;
    case LabelHorizontalAlignment::End:
        halign = rtl ? TextLayout::HorizontalAlignment::Left : TextLayout::HorizontalAlignment::Right;
        hOffset = -2.0f;
        break;
    case LabelHorizontalAlignment::Start:
        halign = rtl ? TextLayout::HorizontalAlignment::Right : TextLayout::HorizontalAlignment::Left;
        if (annotation.markerRep != nullptr)
            hOffset = 2.0f + std::trunc(annotation.markerRep->size() / 2.0f);
        else
            hOffset = 2.0f;
        break;
    }
    if (rtl)
        hOffset = -hOffset;

    switch (annotation.valign)
    {
    case LabelVerticalAlignment::Center:
        vOffset = -static_cast<float>(font->getHeight()) / 2.0f;
        break;
    case LabelVerticalAlignment::Top:
        vOffset = -static_cast<float>(font->getHeight());
        break;
    case LabelVerticalAlignment::Bottom:
        vOffset = 0.0f;
        break;
    }
}

void Renderer::addAnnotation(vector<Annotation>& annotations,
                             const celestia::MarkerRepresentation* markerRep,
                             std::string_view labelText,
                             Color color,
                             const Vector3f& pos,
                             LabelHorizontalAlignment halign,
                             LabelVerticalAlignment valign,
                             float size,
                             bool special)
{
    std::array<int, 4> view{ 0, 0, windowWidth, windowHeight };
    Vector3f win;
    bool success = projectionMode->project(pos, m_modelMatrix, m_projMatrix, m_MVPMatrix, view, win);
    if (success)
    {
        float depth = pos.x() * m_modelMatrix(2, 0) +
                      pos.y() * m_modelMatrix(2, 1) +
                      pos.z() * m_modelMatrix(2, 2);
        win.z() = -depth;
        // use round to remove precision error (+/- 0.0000x)
        // which causes label jittering
        float x = round(win.x());
        float y = round(win.y());
        if (abs(x - win.x()) < 0.001) win.x() = x;
        if (abs(y - win.y()) < 0.001) win.y() = y;

        Annotation a;
        if (!special || markerRep == nullptr)
             a.labelText = labelText;
        a.markerRep = markerRep;
        a.color = color;
        a.position = win;
        a.halign = halign;
        a.valign = valign;
        a.size = size;
        annotations.push_back(a);
    }
}


void Renderer::addForegroundAnnotation(const celestia::MarkerRepresentation* markerRep,
                                       std::string_view labelText,
                                       Color color,
                                       const Vector3f& pos,
                                       LabelHorizontalAlignment halign,
                                       LabelVerticalAlignment valign,
                                       float size)
{
    addAnnotation(foregroundAnnotations, markerRep, labelText, color, pos, halign, valign, size);
}


void Renderer::addBackgroundAnnotation(const celestia::MarkerRepresentation* markerRep,
                                       std::string_view labelText,
                                       Color color,
                                       const Vector3f& pos,
                                       LabelHorizontalAlignment halign,
                                       LabelVerticalAlignment valign,
                                       float size)
{
    addAnnotation(backgroundAnnotations, markerRep, labelText, color, pos, halign, valign, size);
}


void Renderer::addSortedAnnotation(const celestia::MarkerRepresentation* markerRep,
                                   std::string_view labelText,
                                   Color color,
                                   const Vector3f& pos,
                                   LabelHorizontalAlignment halign,
                                   LabelVerticalAlignment valign,
                                   float size)
{
    addAnnotation(depthSortedAnnotations, markerRep, labelText, color, pos, halign, valign, size, true);
}


// Return the orientation of the transformed camera orientation used to render the current
// frame. Available only while rendering a frame.
Quaterniond Renderer::getCameraOrientation() const
{
    return m_cameraOrientation;
}

Quaternionf Renderer::getCameraOrientationf() const
{
    return getCameraOrientation().cast<float>();
}

Matrix3d Renderer::getCameraTransform() const
{
    return m_cameraTransform;
}

void Renderer::setCameraTransform(const Matrix3d& transform)
{
    m_cameraTransform = transform;
}

float Renderer::getNearPlaneDistance() const
{
    return depthPartitions[currentIntervalIndex].nearZ;
}


void Renderer::beginObjectAnnotations()
{
    // It's an error to call beginObjectAnnotations a second time
    // without first calling end.
    assert(!objectAnnotationSetOpen);
    assert(objectAnnotations.empty());

    objectAnnotations.clear();
    objectAnnotationSetOpen = true;
}


void Renderer::endObjectAnnotations()
{
    objectAnnotationSetOpen = false;

    if (!objectAnnotations.empty())
    {
        Renderer::PipelineState ps;
        ps.blending = true;
        ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
        ps.depthMask = true;
        ps.depthTest = true;
        ps.smoothLines = true;
        setPipelineState(ps);

        renderAnnotations(objectAnnotations.begin(),
                          objectAnnotations.end(),
                          -depthPartitions[currentIntervalIndex].nearZ,
                          -depthPartitions[currentIntervalIndex].farZ,
                          FontNormal);

        objectAnnotations.clear();
    }
}


void Renderer::addObjectAnnotation(const celestia::MarkerRepresentation* markerRep,
                                   std::string_view labelText,
                                   Color color,
                                   const Vector3f& pos,
                                   LabelHorizontalAlignment halign,
                                   LabelVerticalAlignment valign)
{
    assert(objectAnnotationSetOpen);
    if (objectAnnotationSetOpen)
    {
        addAnnotation(objectAnnotations, markerRep, labelText, color, pos, halign, valign);
    }
}

Vector4f renderOrbitColor(const Body *body, bool selected, float opacity)
{
    Color orbitColor;

    if (selected)
    {
        // Highlight the orbit of the selected object in red
        orbitColor = Renderer::SelectionOrbitColor;
    }
    else if (body == nullptr || !GetBodyFeaturesManager()->getOrbitColor(body, orbitColor))
    {
        BodyClassification classification = body != nullptr
            ? body->getOrbitClassification()
            : BodyClassification::Stellar;

        switch (classification)
        {
        case BodyClassification::Moon:
            orbitColor = Renderer::MoonOrbitColor;
            break;
        case BodyClassification::MinorMoon:
            orbitColor = Renderer::MinorMoonOrbitColor;
            break;
        case BodyClassification::Asteroid:
            orbitColor = Renderer::AsteroidOrbitColor;
            break;
        case BodyClassification::Comet:
            orbitColor = Renderer::CometOrbitColor;
            break;
        case BodyClassification::Spacecraft:
            orbitColor = Renderer::SpacecraftOrbitColor;
            break;
        case BodyClassification::Stellar:
            orbitColor = Renderer::StarOrbitColor;
            break;
        case BodyClassification::DwarfPlanet:
            orbitColor = Renderer::DwarfPlanetOrbitColor;
            break;
        case BodyClassification::Planet:
            [[fallthrough]];
        default:
            orbitColor = Renderer::PlanetOrbitColor;
            break;
        }
    }

    return Vector4f(orbitColor.red(), orbitColor.green(), orbitColor.blue(), opacity * orbitColor.alpha());
}

void Renderer::renderOrbit(const OrbitPathListEntry& orbitPath,
                           double t,
                           const Quaterniond& cameraOrientation,
                           const math::Frustum& frustum,
                           float nearDist,
                           float farDist)
{
    const Body* body = orbitPath.body;
    double nearZ = -nearDist;  // negate, becase z is into the screen in camera space
    double farZ = -farDist;

    const auto* orbit = body != nullptr ? body->getOrbit(t) : orbitPath.star->getOrbit();

    CurvePlot* cachedOrbit = nullptr;
    if (auto cached = orbitCache.find(orbit); cached != orbitCache.end())
    {
        cachedOrbit = cached->second;
        cachedOrbit->setLastUsed(frameCount);
    }

    // If it's not in the cache already
    if (cachedOrbit == nullptr)
    {
        double startTime = t;

        // Adjust the number of samples used for aperiodic orbits--these aren't
        // true orbits, but are sampled trajectories, generally of spacecraft.
        // Better control is really needed--some sort of adaptive sampling would
        // be ideal.
        if (orbit->isPeriodic())
        {
            startTime = t - orbit->getPeriod();
        }
        else
        {
            double begin = 0.0, end = 0.0;
            orbit->getValidRange(begin, end);

            if (begin != end)
            {
                startTime = begin;
            }
        }

        cachedOrbit = new CurvePlot(*this);
        cachedOrbit->setLastUsed(frameCount);

        OrbitSampler sampler;
        orbit->sample(startTime,
                      startTime + orbit->getPeriod(),
                      sampler);
        sampler.insertForward(cachedOrbit);

        // If the orbit cache is full, first try and eliminate some old orbits
        if (orbitCache.size() > OrbitCacheCullThreshold)
        {
            // Check for old orbits at most once per frame
            if (lastOrbitCacheFlush != frameCount)
            {
                for (auto iter = orbitCache.begin(); iter != orbitCache.end();)
                {
                    // Tricky code to eliminate a node in the orbit cache without screwing
                    // up the iterator. Should work in all STL implementations.
                    if (frameCount - iter->second->lastUsed() > OrbitCacheRetireAge)
                        orbitCache.erase(iter++);
                    else
                        ++iter;
                }
                lastOrbitCacheFlush = frameCount;
            }
        }

        orbitCache[orbit] = cachedOrbit;
    }

    if (cachedOrbit->empty())
        return;

    //*** Orbit rendering parameters

    // The 'window' is the interval of time for which the orbit will be drawn.

    // End of the orbit window relative to the current simulation time. Units
    // are orbital periods. The default value is 0.5.
    const double OrbitWindowEnd     = detailOptions.orbitWindowEnd;

    // Number of orbit periods shown. The orbit window is:
    //    [ t + (OrbitWindowEnd - OrbitPeriodsShown) * T, t + OrbitWindowEnd * T ]
    // where t is the current simulation time and T is the orbital period.
    // The default value is 1.0.
    const double OrbitPeriodsShown  = detailOptions.orbitPeriodsShown;

    // Fraction of the window over which the orbit fades from opaque to transparent.
    // Fading is disabled when this value is zero.
    // The default value is 0.0.
    const double LinearFadeFraction = detailOptions.linearFadeFraction;

    // Extra size of the internal sample cache.
    const double WindowSlack        = 0.2;

    //***

    // 'Periodic' orbits are generally not strictly periodic because of perturbations
    // from other bodies. Here we update the trajectory samples to make sure that the
    // orbit covers a time range centered at the current time and covering a full revolution.
    if (orbit->isPeriodic())
    {
        double period = orbit->getPeriod();
        double endTime = t + period * OrbitWindowEnd;
        double startTime = endTime - period * OrbitPeriodsShown;

        double currentWindowStart = cachedOrbit->startTime();
        double currentWindowEnd = cachedOrbit->endTime();
        double newWindowStart = startTime - period * WindowSlack;
        double newWindowEnd = endTime + period * WindowSlack;

        if (startTime < currentWindowStart)
        {
            // Remove samples at the end of the time window
            cachedOrbit->removeSamplesAfter(newWindowEnd);

            // Trim the first sample (because it will be duplicated when we sample the orbit.)
            cachedOrbit->removeSamplesBefore(cachedOrbit->startTime() * (1.0 + 1.0e-15));

            // Add the new samples
            OrbitSampler sampler;
            orbit->sample(newWindowStart, min(currentWindowStart, newWindowEnd), sampler);
            sampler.insertBackward(cachedOrbit);
#if DEBUG_ORBIT_CACHE
            clog << "new sample count: " << cachedOrbit->sampleCount() << endl;
#endif
        }
        else if (endTime > currentWindowEnd)
        {
            // Remove samples at the beginning of the time window
            cachedOrbit->removeSamplesBefore(newWindowStart);

            // Trim the last sample (because it will be duplicated when we sample the orbit.)
            cachedOrbit->removeSamplesAfter(cachedOrbit->endTime() * (1.0 - 1.0e-15));

            // Add the new samples
            OrbitSampler sampler;
            orbit->sample(max(currentWindowEnd, newWindowStart), newWindowEnd, sampler);
            sampler.insertForward(cachedOrbit);
#if DEBUG_ORBIT_CACHE
            clog << "new sample count: " << cachedOrbit->sampleCount() << endl;
#endif
        }
    }

    // We perform vertex tranformations on the CPU because double precision is necessary to
    // render orbits properly. Start by computing the modelview matrix, to transform orbit
    // vertices into camera space.
    Affine3d modelview;
    {
        auto orientation = body == nullptr ? Quaterniond::Identity() : body->getOrbitFrame(t)->getOrientation(t);
        modelview = cameraOrientation * Translation3d(orbitPath.origin) * orientation.conjugate();
    }

    bool highlight = body != nullptr ? highlightObject.body() == body : highlightObject.star() == orbitPath.star;
    Vector4f orbitColor = renderOrbitColor(body, highlight, orbitPath.opacity);

#ifdef STIPPLED_LINES
    glLineStipple(3, 0x5555);
    glEnable(GL_LINE_STIPPLE);
#endif

    double subdivisionThreshold = pixelSize * 40.0;

    Eigen::Vector3d viewFrustumPlaneNormals[4];
    for (unsigned int i = 0; i < 4; i++)
    {
        viewFrustumPlaneNormals[i] = frustum.plane(static_cast<math::FrustumPlane>(i)).normal().cast<double>();
    }

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.depthTest = true;
    ps.depthMask = false;
    ps.smoothLines = true;
    setPipelineState(ps);

    if (orbit->isPeriodic())
    {
        double period = orbit->getPeriod();
        double windowEnd = t + period * OrbitWindowEnd;
        double windowStart = windowEnd - period * OrbitPeriodsShown;
        double windowDuration = windowEnd - windowStart;

        if (LinearFadeFraction == 0.0f || !util::is_set(renderFlags, RenderFlags::ShowFadingOrbits))
        {
            cachedOrbit->render(modelview,
                                nearZ, farZ, viewFrustumPlaneNormals,
                                subdivisionThreshold,
                                windowStart, windowEnd,
                                orbitColor);
        }
        else
        {
            cachedOrbit->renderFaded(modelview,
                                     nearZ, farZ, viewFrustumPlaneNormals,
                                     subdivisionThreshold,
                                     windowStart, windowEnd,
                                     orbitColor,
                                     windowStart,
                                     windowEnd - windowDuration * (1.0 - LinearFadeFraction));
        }
    }
    else
    {
        if (util::is_set(renderFlags, RenderFlags::ShowPartialTrajectories))
        {
            // Show the trajectory from the start time until the current simulation time
            cachedOrbit->render(modelview,
                                nearZ, farZ, viewFrustumPlaneNormals,
                                subdivisionThreshold,
                                cachedOrbit->startTime(), t,
                                orbitColor);
        }
        else
        {
            // Show the entire trajectory
            cachedOrbit->render(modelview,
                                nearZ, farZ, viewFrustumPlaneNormals,
                                subdivisionThreshold,
                                orbitColor);
        }
    }

#ifdef STIPPLED_LINES
    glDisable(GL_LINE_STIPPLE);
#endif
}


// Convert a position in the universal coordinate system to astrocentric
// coordinates, taking into account possible orbital motion of the star.
static Vector3d astrocentricPosition(const UniversalCoord& pos,
                                     const Star& star,
                                     double t)
{
    return pos.offsetFromKm(star.getPosition(t));
}


void Renderer::autoMag(float& faintestMag, float zoom)
{
    float fieldCorr = getProjectionMode()->getFieldCorrection(zoom);
    faintestMag = faintestAutoMag45deg * std::sqrt(fieldCorr);
    saturationMag = saturationMagNight * (1.0f + fieldCorr * fieldCorr);
}


static Color legacyTintColor(float temp)
{
    // If the star is sufficiently cool, change the light color
    // from white.  Though our sun appears yellow, we still make
    // it and all hotter stars emit white light, as this is the
    // 'natural' light to which our eyes are accustomed.  We also
    // assign a slight bluish tint to light from O and B type stars,
    // though these will almost never have planets for their light
    // to shine upon.
    if (temp > 30000.0f)
        return Color(0.8f, 0.8f, 1.0f);
    else if (temp > 10000.0f)
        return Color(0.9f, 0.9f, 1.0f);
    else if (temp > 5400.0f)
        return Color(1.0f, 1.0f, 1.0f);
    else if (temp > 3900.0f)
        return Color(1.0f, 0.9f, 0.8f);
    else if (temp > 2000.0f)
        return Color(1.0f, 0.7f, 0.7f);
    return Color(1.0f, 0.4f, 0.4f);
}


// Set up the light sources for rendering a solar system.  The positions of
// all nearby stars are converted from universal to viewer-centered
// coordinates.
static void
setupLightSources(const vector<const Star*>& nearStars,
                  const UniversalCoord& observerPos,
                  double t,
                  vector<LightSource>& lightSources,
                  float tintSaturation,
                  const ColorTemperatureTable* tintColors)
{
    // Fade out the illumination from cool objects. Objects at the Draper
    // point (798 K) should be visibly glowing, so set the minimum temperature
    // for illumination to be slightly below this.
    constexpr float DARK_POINT = 780.0f;
    constexpr float FADE_POINT = 1000.0f;

    lightSources.clear();

    for (const auto star : nearStars)
    {
        if (star->getVisibility())
        {
            Vector3d v = star->getPosition(t).offsetFromKm(observerPos);
            LightSource ls;
            ls.position = v;
            ls.luminosity = star->getLuminosity();
            ls.radius = star->getRadius();

            float temp = star->getTemperature();
            if (temp <= DARK_POINT)
                continue;

            if (tintColors == nullptr)
                ls.color = legacyTintColor(temp);
            else
            {
                float fadeFactor = temp < FADE_POINT
                    ? (temp - DARK_POINT) / (FADE_POINT - DARK_POINT)
                    : 1.0f;

                // Artificially decrease the luminosity below the fade point
                // so that other light sources in the system may provide more
                // illumination.
                ls.luminosity *= fadeFactor;

                // Use a variant of the blackbody colors with the whitepoint
                // set to Sol being white, to ensure consistency of the Solar
                // System textures.
                ls.color = tintColors->lookupTintColor(temp, tintSaturation, fadeFactor);
            }

            lightSources.push_back(ls);
        }
    }
}


// Set up the potential secondary light sources for rendering solar system
// bodies.
static void
setupSecondaryLightSources(vector<SecondaryIlluminator>& secondaryIlluminators,
                           const vector<LightSource>& primaryIlluminators)
{
    constexpr float au2 = math::square(astro::kilometersToAU(1.0f));

    for (auto& i : secondaryIlluminators)
    {
        i.reflectedIrradiance = 0.0f;

        for (const auto& j : primaryIlluminators)
        {
            i.reflectedIrradiance += j.luminosity / ((float) (i.position_v - j.position).squaredNorm() * au2);
        }

        i.reflectedIrradiance *= i.body->getReflectivity();
    }
}


// Render an item from the render list
void Renderer::renderItem(const RenderListEntry& rle,
                          const Observer& observer,
                          float nearPlaneDistance,
                          float farPlaneDistance,
                          const Matrices& m)
{
    switch (rle.renderableType)
    {
    case RenderListEntry::RenderableStar:
        renderStar(*rle.star,
                   rle.position,
                   rle.distance,
                   rle.appMag,
                   observer,
                   nearPlaneDistance, farPlaneDistance,
                   m);
        break;

    case RenderListEntry::RenderableBody:
        renderPlanet(*rle.body,
                     rle.position,
                     rle.distance,
                     rle.appMag,
                     observer,
                     nearPlaneDistance, farPlaneDistance,
                     m);
        break;

    case RenderListEntry::RenderableCometTail:
        renderCometTail(*rle.body,
                        rle.position,
                        observer,
                        rle.radius,
                        rle.discSizeInPixels,
                        m);
        break;

    case RenderListEntry::RenderableReferenceMark:
        renderReferenceMark(*rle.refMark,
                            rle.position,
                            rle.distance,
                            observer.getTime(),
                            nearPlaneDistance,
                            m);
        break;

    default:
        break;
    }
}


void Renderer::render(const Observer& observer,
                      const Universe& universe,
                      float faintestMagNight,
                      const Selection& sel)
{
    // Get the observer's time
    double now = observer.getTime();
    realTime = observer.getRealTime();

    frameCount++;
    settingsChanged = false;

    // Compute the size of a pixel
    float zoom = observer.getZoom();
    setFieldOfView(math::radToDeg(getProjectionMode()->getFOV(zoom)));
    cosViewConeAngle = projectionMode->getViewConeAngleMax(zoom);
    pixelSize = getProjectionMode()->getPixelSize(zoom);

    // Get the displayed surface texture set to use from the observer
    displayedSurface = observer.getDisplayedSurface();

    locationFilter = observer.getLocationFilter();

    // Highlight the selected object
    highlightObject = sel;

    m_cameraOrientation = Quaterniond(m_cameraTransform) * observer.getOrientation();

    // Get the view frustum used for culling in camera space.
    math::InfiniteFrustum frustum = projectionMode->getInfiniteFrustum(MinNearPlaneDistance, zoom);

    // Get the transformed frustum, used for culling in the astrocentric coordinate
    // system.
    math::InfiniteFrustum xfrustum(frustum);
    xfrustum.transform(getCameraOrientationf().conjugate().toRotationMatrix());

    // Set up the projection and modelview matrices.
    // We'll usethem for positioning star and planet labels.
    auto [nearZ, farZ] = projectionMode->getDefaultDepthRange();
    buildProjectionMatrix(m_projMatrix, nearZ, farZ, observer.getZoom());
    m_modelMatrix = Affine3f(getCameraOrientationf()).matrix();
    m_MVPMatrix = m_projMatrix * m_modelMatrix;

    depthSortedAnnotations.clear();
    foregroundAnnotations.clear();
    backgroundAnnotations.clear();
    objectAnnotations.clear();

    // Put all solar system bodies into the render list.  Stars close and
    // large enough to have discernible surface detail are also placed in
    // renderList.
    renderList.clear();
    orbitPathList.clear();
    lightSourceList.clear();
    secondaryIlluminators.clear();
    nearStars.clear();

    // See if we want to use AutoMag.
    if (util::is_set(renderFlags, RenderFlags::ShowAutoMag))
    {
        autoMag(faintestMag, zoom);
    }
    else
    {
        faintestMag = faintestMagNight;
        saturationMag = saturationMagNight;
    }

    faintestPlanetMag = faintestMag;
    if (util::is_set(renderFlags, RenderFlags::ShowSolarSystemObjects | RenderFlags::ShowOrbits))
    {
        buildNearSystemsLists(universe, observer, xfrustum, now);
    }

    setupSecondaryLightSources(secondaryIlluminators, lightSourceList);

    // Scan through the render list to see if we're inside a planetary
    // atmosphere.  If so, we need to adjust the sky color as well as the
    // limiting magnitude of stars (so stars aren't visible in the daytime
    // on planets with thick atmospheres.)
    if (util::is_set(renderFlags, RenderFlags::ShowAtmospheres))
    {
        adjustMagnitudeInsideAtmosphere(faintestMag, saturationMag, now);
    }

    // Now we need to determine how to scale the brightness of stars.  The
    // brightness will be proportional to the apparent magnitude, i.e.
    // a logarithmic function of the stars apparent brightness.  This mimics
    // the response of the human eye.  We sort of fudge things here and
    // maintain a minimum range of six magnitudes between faintest visible
    // and saturation; this keeps stars from popping in or out as the sun
    // sets or rises.
    if (faintestMag - saturationMag >= 6.0f)
        brightnessScale = 1.0f / (faintestMag -  saturationMag);
    else
        brightnessScale = 0.1667f;

    brightnessScale *= corrFac;
    if (starStyle == StarStyle::ScaledDiscStars)
        brightnessScale *= 2.0f;

    // Calculate saturation magnitude
    satPoint = faintestMag - (1.0f - brightnessBias) / brightnessScale;

    ambientColor = Color(ambientLightLevel, ambientLightLevel, ambientLightLevel);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render sky grids first--these will always be in the background
    renderSkyGrids(observer);

    // Render deep sky objects
    if (util::is_set(renderFlags, RenderFlags::ShowDeepSpaceObjects) && universe.getDSOCatalog() != nullptr)
    {
        renderDeepSkyObjects(universe, observer, faintestMag);
    }

    // Render stars
    if (util::is_set(renderFlags, RenderFlags::ShowStars) && universe.getStarCatalog() != nullptr)
    {
        renderPointStars(*universe.getStarCatalog(), faintestMag, observer);
    }

    // Translate the camera before rendering the asterisms and boundaries
    // Set up the camera for star rendering; the units of this phase
    // are light years.
    Vector3f observerPosLY = -observer.getPosition().offsetFromLy(Vector3f::Zero());

    Matrix4f projection = getProjectionMatrix();
    Matrix4f modelView = getModelViewMatrix() * math::translate(observerPosLY);

    Matrices asterismMVP = { &projection, &modelView };

    float dist = observerPosLY.norm() * 1.6e4f;
    renderAsterisms(universe, dist, asterismMVP);
    renderBoundaries(universe, dist, asterismMVP);

    // Render star and deep sky object labels
    renderBackgroundAnnotations(FontNormal);

    // Render constellations labels
    if (util::is_set(labelMode, RenderLabels::ConstellationLabels) && universe.getAsterisms() != nullptr)
    {
        labelConstellations(*universe.getAsterisms(), observer);
        renderBackgroundAnnotations(FontLarge);
    }

    if (util::is_set(renderFlags, RenderFlags::ShowMarkers))
    {
        markersToAnnotations(universe.getMarkers(), observer, now);
    }

    // Draw the selection cursor
    bool selectionVisible = false;
    if (!sel.empty() && util::is_set(renderFlags, RenderFlags::ShowMarkers))
    {
        selectionVisible = selectionToAnnotation(sel, observer, xfrustum, now);
    }

    // Render background markers; rendering of other markers is deferred until
    // solar system objects are rendered.
    renderBackgroundAnnotations(FontNormal);

    removeInvisibleItems(frustum);

    // Sort the annotations
    sort(depthSortedAnnotations.begin(), depthSortedAnnotations.end());

    // Sort the orbit paths
    sort(orbitPathList.begin(), orbitPathList.end());

#ifndef GL_ES
    glPolygonMode(GL_FRONT_AND_BACK, (GLenum) renderMode);
#endif

    int nIntervals = buildDepthPartitions();
    renderSolarSystemObjects(observer, nIntervals, now);

    renderForegroundAnnotations(FontNormal);

    if (showSelectionPointer && !selectionVisible && util::is_set(renderFlags, RenderFlags::ShowMarkers))
    {
        renderSelectionPointer(observer, now, xfrustum, sel);
    }

#ifndef GL_ES
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
}

static Eigen::Vector3f
calculateQuadCenter(const Eigen::Quaternionf &cameraOrientation,
                    const Eigen::Vector3f &position,
                    float radius)
{
    Matrix3f m = cameraOrientation.conjugate().toRotationMatrix();

    // Offset the glare sprite so that it lies in front of the object
    Vector3f direction = position.normalized();

    // Position the sprite on the the line between the viewer and the
    // object, and on a plane normal to the view direction.
    return position + direction * (radius / (m * Vector3f::UnitZ()).dot(direction));
}

void
Renderer::calculatePointSize(float appMag,
                             float size,
                             float &discSize,
                             float &alpha,
                             float &glareSize,
                             float &glareAlpha) const
{
    alpha = std::max(0.0f, (faintestMag - appMag) * brightnessScale + brightnessBias);

    discSize = size;
    if (starStyle == StarStyle::ScaledDiscStars)
    {
        if (alpha > 1.0f)
        {
            float discScale = std::min(MaxScaledDiscStarSize, pow(2.0f, 0.3f * (satPoint - appMag)));
            discSize *= std::max(1.0f, discScale);

            glareAlpha = std::min(0.5f, discScale / 4.0f);
            glareSize = discSize * 3.0f;

            alpha = 1.0f;
        }
        else
        {
            glareSize = glareAlpha = 0.0f;
        }
    }
    else
    {
        if (alpha > 1.0f)
        {
            float discScale = std::min(100.0f, satPoint - appMag + 2.0f);
            glareAlpha = std::min(GlareOpacity, (discScale - 2.0f) / 4.0f);
            glareSize = 2.0f * discScale * size;
            alpha = 1.0f;
        }
        else
        {
            glareSize = glareAlpha = 0.0f;
        }
    }
}

// If the an object occupies a pixel or less of screen space, we don't
// render its mesh at all and just display a starlike point instead.
// Switching between the particle and mesh renderings of an object is
// jarring, however . . . so we'll blend in the particle view of the
// object to smooth things out, making it dimmer as the disc size exceeds the
// max disc size.
void Renderer::renderObjectAsPoint(const Vector3f& position,
                                   float radius,
                                   float appMag,
                                   float discSizeInPixels,
                                   const Color &color,
                                   bool useHalos,
                                   bool emissive,
                                   const Matrices &mvp)
{
    const bool useScaledDiscs = starStyle == StarStyle::ScaledDiscStars;
    float maxDiscSize = useScaledDiscs ? MaxScaledDiscStarSize : 1.0f;
    float maxBlendDiscSize = maxDiscSize + 3.0f;

    if (discSizeInPixels < maxBlendDiscSize || useHalos)
    {
        float fade = 1.0f;
        if (discSizeInPixels > maxDiscSize)
        {
            fade = std::min(1.0f, (maxBlendDiscSize - discSizeInPixels) /
                                  (maxBlendDiscSize - maxDiscSize));
        }

        float scale = static_cast<float>(screenDpi) / 96.0f;
        float pointSize, alpha, glareSize, glareAlpha;
        calculatePointSize(appMag, BaseStarDiscSize * scale, pointSize, alpha, glareSize, glareAlpha);

        if (useScaledDiscs && discSizeInPixels > MaxScaledDiscStarSize)
            glareAlpha = std::min(glareAlpha, (MaxScaledDiscStarSize - discSizeInPixels) / MaxScaledDiscStarSize + 1.0f);

        alpha *= fade;
        if (!emissive)
            glareAlpha *= fade;

        if (glareSize != 0.0f)
            glareSize = std::max(glareSize, pointSize * discSizeInPixels / scale * 3.0f);

        Renderer::PipelineState ps;
        ps.blending = true;
        ps.blendFunc = {GL_SRC_ALPHA, GL_ONE};
        ps.depthTest = true;
        setPipelineState(ps);

        if (starStyle != StarStyle::PointStars)
            gaussianDiscTex->bind();

        if (pointSize > gl::maxPointSize)
            m_largeStarRenderer->render(position, {color, alpha}, pointSize, mvp);
        else
            pointStarVertexBuffer->addStar(position, {color, alpha}, pointSize);

        // If the object is brighter than magnitude 1, add a halo around it to
        // make it appear more brilliant.  This is a hack to compensate for the
        // limited dynamic range of monitors.
        //
        // TODO: Stars look fine but planets look unrealistically bright
        // with halos.
        if (useHalos && glareAlpha > 0.0f)
        {
            Eigen::Vector3f center = calculateQuadCenter(getCameraOrientationf(), position, radius);
            gaussianGlareTex->bind();
            if (glareSize > gl::maxPointSize)
                m_largeStarRenderer->render(center, {color, glareAlpha}, glareSize, mvp);
            else
                glareVertexBuffer->addStar(center, {color, glareAlpha}, glareSize);
        }
    }
}


static void renderSphereUnlit(const RenderInfo& ri,
                              const math::Frustum& frustum,
                              const Matrices &m,
                              Renderer *r)
{
    boost::container::static_vector<Texture*, LODSphereMesh::MAX_SPHERE_MESH_TEXTURES> textures;

    ShaderProperties shadprop;
    shadprop.texUsage = TexUsage::TextureCoordTransform;

    // Set up the textures used by this object
    if (ri.baseTex != nullptr)
    {
        shadprop.texUsage |= TexUsage::DiffuseTexture;
        textures.push_back(ri.baseTex);
    }
    if (ri.nightTex != nullptr)
    {
        shadprop.texUsage |= TexUsage::NightTexture;
        textures.push_back(ri.nightTex);
    }
    if (ri.overlayTex != nullptr)
    {
        shadprop.texUsage |= TexUsage::OverlayTexture;
        textures.push_back(ri.overlayTex);
    }

    // Get a shader for the current rendering configuration
    auto* prog = r->getShaderManager().getShader(shadprop);
    if (prog == nullptr)
        return;
    prog->use();

    prog->setMVPMatrices(*m.projection, *m.modelview);
    prog->textureOffset = 0.0f;
    prog->ambientColor = ri.color.toVector3();
    prog->opacity = 1.0f;

    Renderer::PipelineState ps;
    ps.depthMask = true;
    ps.depthTest = true;
    r->setPipelineState(ps);

    g_lodSphere->render(frustum, ri.pixWidth,
                        textures.data(), static_cast<int>(textures.size()), prog);
}


static void renderCloudsUnlit(const RenderInfo& ri,
                              const math::Frustum& frustum,
                              Texture *cloudTex,
                              float cloudTexOffset,
                              const Matrices &m,
                              Renderer *r)
{
    ShaderProperties shadprop;
    shadprop.texUsage = TexUsage::DiffuseTexture | TexUsage::TextureCoordTransform;
    shadprop.lightModel = LightingModel::UnlitModel;

    // Get a shader for the current rendering configuration
    auto* prog = r->getShaderManager().getShader(shadprop);
    if (prog == nullptr)
        return;
    prog->use();
    prog->setMVPMatrices(*m.projection, *m.modelview);
    prog->textureOffset = cloudTexOffset;

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.depthTest = true;
    r->setPipelineState(ps);

    g_lodSphere->render(frustum, ri.pixWidth, &cloudTex, 1, prog);
}

void
Renderer::locationsToAnnotations(const Body& body,
                                 const Vector3d& bodyPosition,
                                 const Quaterniond& bodyOrientation)
{
    assert(GetBodyFeaturesManager()->hasLocations(&body));
    auto locations = GetBodyFeaturesManager()->getLocations(&body);

    Vector3f semiAxes = body.getSemiAxes();

    float nearDist = getNearPlaneDistance();
    double boundingRadius = semiAxes.maxCoeff();

    Vector3d bodyCenter = bodyPosition;
    Vector3d viewRayOrigin = bodyOrientation * -bodyCenter;
    double labelOffset = 0.0001;

    Vector3f vn  = getCameraOrientationf().conjugate() * -Vector3f::UnitZ();
    Vector3d viewNormal = vn.cast<double>();

    math::Ellipsoidd bodyEllipsoid(semiAxes.cast<double>());

    Matrix3d bodyMatrix = bodyOrientation.conjugate().toRotationMatrix();

    for (const auto location : *locations)
    {
        auto featureType = location->getFeatureType();
        if ((featureType & locationFilter) == 0)
            continue;

        // Get the position of the location with respect to the planet center
        Vector3f ppos = location->getPosition();

        // Compute the bodycentric position of the location
        Vector3d locPos = ppos.cast<double>();

        // Get the planetocentric position of the label.  Add a slight scale factor
        // to keep the point from being exactly on the surface.
        Vector3d pcLabelPos = locPos * (1.0 + labelOffset);

        // Get the camera space label position
        Vector3d labelPos = bodyCenter + bodyMatrix * locPos;

        float effSize = location->getImportance();
        if (effSize < 0.0f)
            effSize = location->getSize();

        if (float pixSize = effSize / (float) (labelPos.norm() * pixelSize);
            pixSize <= minFeatureSize || labelPos.dot(viewNormal) <= 0.0)
        {
            continue;
        }

        // Labels on non-ellipsoidal bodies need special handling; the
        // ellipsoid visibility test will always fail for them, since they
        // will lie on the surface of the mesh, which is inside the
        // the bounding ellipsoid. The following code projects location positions
        // onto the bounding sphere.
        if (!body.isEllipsoid())
        {
            double r = locPos.norm();
            if (r < boundingRadius)
                pcLabelPos = locPos * (boundingRadius * 1.01 / r);
        }

        double t = 0.0;

        // Test for an intersection of the eye-to-location ray with
        // the planet ellipsoid.  If we hit the planet first, then
        // the label is obscured by the planet.  An exact calculation
        // for irregular objects would be too expensive, and the
        // ellipsoid approximation works reasonably well for them.
        Eigen::ParametrizedLine<double, 3> testRay(viewRayOrigin, pcLabelPos - viewRayOrigin);

        if (bool hit = testIntersection(testRay, bodyEllipsoid, t);
            hit && t < 1.0)
        {
            continue;
        }

        // Calculate the intersection of the eye-to-label ray with the plane perpendicular to
        // the view normal that touches the front of the object's bounding sphere
        double planetZ = std::max(viewNormal.dot(bodyCenter) - boundingRadius, -nearDist * 1.001);
        double z = viewNormal.dot(labelPos);
        labelPos *= planetZ / z;

        const celestia::MarkerRepresentation* locationMarker = nullptr;
        if (featureType & Location::City)
            locationMarker = &cityRep;
        else if (featureType & (Location::LandingSite | Location::Observatory))
            locationMarker = &observatoryRep;
        else if (featureType & (Location::Crater | Location::Patera))
            locationMarker = &craterRep;
        else if (featureType & (Location::Mons | Location::Tholus))
            locationMarker = &mountainRep;
        else if (featureType & (Location::EruptiveCenter))
            locationMarker = &genericLocationRep;

        Color labelColor = location->isLabelColorOverridden()
            ? location->getLabelColor()
            : LocationLabelColor;

        addObjectAnnotation(locationMarker,
                            location->getName(true),
                            labelColor,
                            labelPos.cast<float>(),
                            LabelHorizontalAlignment::Start,
                            LabelVerticalAlignment::Bottom);
    }
}


// Estimate the fraction of light reflected from a sphere that
// reaches an object at the specified position relative to that
// sphere.
//
// This is function is just a rough approximation to the actual
// lighting integral, but it reproduces the important features
// of the way that phase and distance affect reflected light:
//    - Higher phase angles mean less reflected light
//    - The closer an object is to the reflector, the less
//      area of the reflector that is visible.
//
// We approximate the reflected light by taking a weighted average
// of the reflected light at three points on the reflector: the
// light receiver's sub-point, and the two horizon points in the
// plane of the light vector and receiver-to-reflector vector.
//
// The reflecting object is assumed to be spherical and perfectly
// Lambertian.
static float
estimateReflectedLightFraction(const Vector3d& toSun,
                               const Vector3d& toObject,
                               float radius)
{
    // Theta is half the arc length visible to the reflector
    double d = toObject.norm();
    float cosTheta = std::min((float) (radius / d), 0.999f);

    // Phi is the angle between the light vector and receiver-to-reflector vector.
    // cos(phi) is thus the illumination at the sub-point. The horizon points are
    // at phi+theta and phi-theta.
    float cosPhi = (float) (toSun.dot(toObject) / (d * toSun.norm()));

    // Use a trigonometric identity to compute cos(phi +/- theta):
    //   cos(phi + theta) = cos(phi) * cos(theta) - sin(phi) * sin(theta)

    // s = sin(phi) * sin(theta)
    float s = std::sqrt((1.0f - cosPhi * cosPhi) * (1.0f - cosTheta * cosTheta));

    float cosPhi1 = cosPhi * cosTheta - s;  // cos(phi + theta)
    float cosPhi2 = cosPhi * cosTheta + s;  // cos(phi - theta)

    // Calculate a weighted average of illumination at the three points
    return (2.0f * std::max(cosPhi, 0.0f) + std::max(cosPhi1, 0.0f) + std::max(cosPhi2, 0.0f)) * 0.25f;
}


static void
setupObjectLighting(const vector<LightSource>& suns,
                    const vector<SecondaryIlluminator>& secondaryIlluminators,
                    const Quaternionf& objOrientation,
                    const Vector3f& objScale,
                    const Vector3f& objPosition_eye,
                    bool isNormalized,
                    LightingState& ls)
{
    unsigned int nLights = min(MaxLights, (unsigned int) suns.size());
    if (nLights == 0)
        return;

    unsigned int i;
    for (i = 0; i < nLights; i++)
    {
        Vector3d dir = suns[i].position - objPosition_eye.cast<double>();

        ls.lights[i].direction_eye = dir.cast<float>();
        float distance = ls.lights[i].direction_eye.norm();
        ls.lights[i].direction_eye *= 1.0f / distance;
        distance = astro::kilometersToAU((float) dir.norm());
        ls.lights[i].irradiance = suns[i].luminosity / (distance * distance);
        ls.lights[i].color = suns[i].color;

        // Store the position and apparent size because we'll need them for
        // testing for eclipses.
        ls.lights[i].position = dir;
        ls.lights[i].apparentSize = (float) (suns[i].radius / dir.norm());
        ls.lights[i].castsShadows = true;
    }

    // Include effects of secondary illumination (i.e. planetshine)
    if (!secondaryIlluminators.empty() && i < MaxLights - 1)
    {
        float maxIrr = 0.0f;
        unsigned int maxIrrSource = 0, counter = 0;
        Vector3d objpos = objPosition_eye.cast<double>();

        // Only account for light from the brightest secondary source
        for (auto& illuminator : secondaryIlluminators)
        {
            Vector3d toIllum = illuminator.position_v - objpos;  // reflector-to-object vector
            float distSquared = (float) toIllum.squaredNorm() / math::square(illuminator.radius);

            if (distSquared > 0.01f)
            {
                // Irradiance falls off with distance^2
                float irr = illuminator.reflectedIrradiance / distSquared;

                // Phase effects will always leave the irradiance unaffected or reduce it;
                // don't bother calculating them if we've already found a brighter secondary
                // source.
                if (irr > maxIrr)
                {
                    // Account for the phase
                    Vector3d toSun = objpos - suns[0].position;
                    irr *= estimateReflectedLightFraction(toSun, toIllum, illuminator.radius);
                    if (irr > maxIrr)
                    {
                        maxIrr = irr;
                        maxIrrSource = counter;
                    }
                }
            }
            counter++;
        }
#if DEBUG_SECONDARY_ILLUMINATION
        clog << "maxIrr = " << maxIrr << ", "
             << secondaryIlluminators[maxIrrSource].body->getName() << ", "
             << secondaryIlluminators[maxIrrSource].reflectedIrradiance << endl;
#endif

        if (maxIrr > 0.0f)
        {
            Vector3d toIllum = secondaryIlluminators[maxIrrSource].position_v - objpos;

            ls.lights[i].direction_eye = toIllum.cast<float>();
            ls.lights[i].direction_eye.normalize();
            ls.lights[i].irradiance = maxIrr;
            ls.lights[i].color = secondaryIlluminators[maxIrrSource].body->getSurface().color;
            ls.lights[i].apparentSize = 0.0f;
            ls.lights[i].castsShadows = false;
            i++;
            nLights++;
        }
    }

    // Sort light sources by brightness.  Light zero should always be the
    // brightest.  Optimize common cases of one and two lights.
    if (nLights == 2)
    {
        if (ls.lights[0].irradiance < ls.lights[1].irradiance)
            swap(ls.lights[0], ls.lights[1]);
    }
    else if (nLights > 2)
    {
        sort(ls.lights, ls.lights + nLights,
             [](const auto &l0, const auto &l1) { return l0.irradiance > l1.irradiance; });
    }

    // Compute the total irradiance
    float totalIrradiance = 0.0f;
    for (i = 0; i < nLights; i++)
        totalIrradiance += ls.lights[i].irradiance;

    // Compute a gamma factor to make dim light sources visible.  This is
    // intended to approximate what we see with our eyes--for example,
    // Earth-shine is visible on the night side of the Moon, even though
    // the amount of reflected light from the Earth is 1/10000 of what
    // the Moon receives directly from the Sun.
    //
    // TODO: Skip this step when high dynamic range rendering to floating point
    //   buffers is enabled.
    float minVisibleFraction = 1.0f / 10000.0f;
    float minDisplayableValue = 1.0f / 255.0f;
    float gamma = log(minDisplayableValue) / log(minVisibleFraction);
    float minVisibleIrradiance = minVisibleFraction * totalIrradiance;

    Matrix3f m = objOrientation.toRotationMatrix();

    // Gamma scale and normalize the light sources; cull light sources that
    // aren't bright enough to contribute the final pixels rendered into the
    // frame buffer.
    ls.nLights = 0;
    for (i = 0; i < nLights && ls.lights[i].irradiance > minVisibleIrradiance; i++)
    {
        ls.lights[i].irradiance = pow(ls.lights[i].irradiance / totalIrradiance, gamma);

        // Compute the direction of the light in object space
        ls.lights[i].direction_obj = m * ls.lights[i].direction_eye;

        ls.nLights++;
    }

    Matrix3f invScale = objScale.cwiseInverse().asDiagonal();
    ls.eyePos_obj = invScale * m * -objPosition_eye;
    ls.eyeDir_obj = (m * -objPosition_eye).normalized();

    // When the camera is very far from the object, some view-dependent
    // calculations in the shaders can exhibit precision problems. This
    // occurs with atmospheres, where the scale height of the atmosphere
    // is very small relative to the planet radius. To address the problem,
    // we'll clamp the eye distance to some maximum value. The effect of the
    // adjustment should be impercetible, since at large distances rays from
    // the camera to object vertices are all nearly parallel to each other.
    float eyeFromCenterDistance = ls.eyePos_obj.norm();
    if (eyeFromCenterDistance > 100.0f && isNormalized)
    {
        ls.eyePos_obj *= 100.0f / eyeFromCenterDistance;
    }

    ls.ambientColor = Vector3f::Zero();
}


void Renderer::renderObject(const Vector3f& pos,
                            float distance,
                            const Observer& observer,
                            float nearPlaneDistance,
                            float farPlaneDistance,
                            RenderProperties& obj,
                            const LightingState& ls,
                            const Matrices &m)
{
    RenderInfo ri;
    double now = observer.getTime();

    float altitude = distance - obj.radius;
    float discSizeInPixels = obj.radius / (max(nearPlaneDistance, altitude) * pixelSize);

    ri.sunDir_eye = Vector3f::UnitY();
    ri.sunDir_obj = Vector3f::UnitY();
    ri.sunColor = Color::Black;
    if (ls.nLights > 0)
    {
        ri.sunDir_eye = ls.lights[0].direction_eye;
        ri.sunDir_obj = ls.lights[0].direction_obj;
        ri.sunColor   = ls.lights[0].color;// * ls.lights[0].intensity;
    }

    // Get the object's geometry; nullptr indicates that object is an
    // ellipsoid.
    Geometry* geometry = nullptr;
    if (obj.geometry != InvalidResource)
    {
        // This is a model loaded from a file
        geometry = engine::GetGeometryManager()->find(obj.geometry);
    }

    // Get the textures . . .
    if (obj.surface->baseTexture.texture(textureResolution) != InvalidResource)
        ri.baseTex = obj.surface->baseTexture.find(textureResolution);
    if ((obj.surface->appearanceFlags & Surface::ApplyBumpMap) != 0 &&
        obj.surface->bumpTexture.texture(textureResolution) != InvalidResource)
        ri.bumpTex = obj.surface->bumpTexture.find(textureResolution);
    if ((obj.surface->appearanceFlags & Surface::ApplyNightMap) != 0 &&
        util::is_set(renderFlags, RenderFlags::ShowNightMaps))
        ri.nightTex = obj.surface->nightTexture.find(textureResolution);
    if ((obj.surface->appearanceFlags & Surface::SeparateSpecularMap) != 0)
        ri.glossTex = obj.surface->specularTexture.find(textureResolution);
    if ((obj.surface->appearanceFlags & Surface::ApplyOverlay) != 0)
        ri.overlayTex = obj.surface->overlayTexture.find(textureResolution);

    // Scaling will be nonuniform for nonspherical planets. As long as the
    // deviation from spherical isn't too large, the nonuniform scale factor
    // shouldn't mess up the lighting calculations enough to be noticeable
    // (and we turn on renormalization anyhow, which most graphics cards
    // support.)
    float radius = obj.radius;
    Vector3f scaleFactors;
    float ringsScaleFactor;
    float geometryScale;
    if (geometry == nullptr || geometry->isNormalized())
    {
        geometryScale = obj.radius;
        scaleFactors = obj.radius * obj.semiAxes;
        ringsScaleFactor = obj.radius * obj.semiAxes.maxCoeff();
        ri.pointScale = 2.0f * obj.radius / pixelSize;
    }
    else
    {
        geometryScale = obj.geometryScale;
        scaleFactors = Vector3f::Constant(geometryScale);
        ringsScaleFactor = geometryScale;
        ri.pointScale = 2.0f * geometryScale / pixelSize;
    }
    // Apply the modelview transform for the object
    Affine3f transform = Translation3f(pos) * obj.orientation.conjugate();
    Matrix4f planetMV  = (*m.modelview) * (transform * Scaling(scaleFactors)).matrix();
    Matrices planetMVP = { m.projection, &planetMV };

    Matrices ringsMVP;
    Matrix4f ringsMV;
    bool showRings = obj.rings != nullptr && util::is_set(renderFlags, RenderFlags::ShowPlanetRings);
    if (showRings)
    {
        ringsMV  = (*m.modelview) * (transform * Scaling(ringsScaleFactor)).matrix();
        ringsMVP = { m.projection, &ringsMV  };
    }

    Matrix3f planetRotation = obj.orientation.toRotationMatrix();

    ri.eyeDir_obj = -(planetRotation * pos).normalized();
    ri.eyePos_obj = -(planetRotation * (pos.cwiseQuotient(scaleFactors)));

    ri.orientation = getCameraOrientationf() * obj.orientation.conjugate();

    ri.pixWidth = discSizeInPixels;

    // Set up the colors
    if (ri.baseTex == nullptr ||
        (obj.surface->appearanceFlags & Surface::BlendTexture) != 0)
    {
        ri.color = obj.surface->color;
    }

    ri.ambientColor = ambientColor;
    ri.specularColor = obj.surface->specularColor;
    ri.specularPower = obj.surface->specularPower;
    ri.lunarLambert = obj.surface->lunarLambert;

    // See if the surface should be lit
    bool lit = (obj.surface->appearanceFlags & Surface::Emissive) == 0;

    // Compute the inverse model/view matrix
    Affine3f invModelView = obj.orientation *
                            Translation3f(-pos / obj.radius) *
                            getCameraOrientationf().conjugate();
    Matrix4f invMV = invModelView.matrix();

    // The sphere rendering code uses the view frustum to determine which
    // patches are visible. In order to avoid rendering patches that can't
    // be seen, make the far plane of the frustum as close to the viewer
    // as possible.
    float frustumFarPlane = farPlaneDistance;
    if (obj.geometry == InvalidResource)
    {
        // Only adjust the far plane for ellipsoidal objects
        float d = pos.norm();

        // Account for non-spherical objects
        float eradius = scaleFactors.minCoeff();

        if (d > eradius)
        {
            // Include a fudge factor to eliminate overaggressive clipping
            // due to limited floating point precision
            frustumFarPlane = std::sqrt(math::square(d) - math::square(eradius)) * 1.1f;
        }
        else
        {
            // We're inside the bounding sphere; leave the far plane alone
        }

        if (obj.atmosphere != nullptr)
        {
            float atmosphereHeight = max(obj.atmosphere->cloudHeight,
                                         obj.atmosphere->mieScaleHeight * -log(AtmosphereExtinctionThreshold));
            if (atmosphereHeight > 0.0f)
            {
                // If there's an atmosphere, we need to move the far plane
                // out so that the clouds and atmosphere shell aren't clipped.
                float atmosphereRadius = eradius + atmosphereHeight;
                frustumFarPlane += std::sqrt(math::square(atmosphereRadius) - math::square(eradius));
            }
        }
    }

    // Transform the frustum into object coordinates using the
    // inverse model/view matrix. The frustum is scaled to a
    // normalized coordinate system where the 1 unit = 1 planet
    // radius (for an ellipsoidal planet, radius is taken to be
    // largest semiaxis.)
    auto viewFrustum = projectionMode->getFrustum(nearPlaneDistance / radius, frustumFarPlane / radius, observer.getZoom());
    viewFrustum.transform(invMV);

    // Get cloud layer parameters
    Texture* cloudTex       = nullptr;
    Texture* cloudNormalMap = nullptr;
    float cloudTexOffset    = 0.0f;
    // Ugly cast required because MultiResTexture::find() is non-const
    Atmosphere* atmosphere  = const_cast<Atmosphere*>(obj.atmosphere);

    if (atmosphere != nullptr)
    {
        if (util::is_set(renderFlags, RenderFlags::ShowCloudMaps))
        {
            if (atmosphere->cloudTexture.texture(textureResolution) != InvalidResource)
                cloudTex = atmosphere->cloudTexture.find(textureResolution);
            if (atmosphere->cloudNormalMap.texture(textureResolution) != InvalidResource)
                cloudNormalMap = atmosphere->cloudNormalMap.find(textureResolution);
        }
        if (atmosphere->cloudSpeed != 0.0f)
            cloudTexOffset = (float) (-math::pfmod(now * atmosphere->cloudSpeed * 0.5 * celestia::numbers::inv_pi, 1.0));
    }

    if (obj.geometry == InvalidResource)
    {
        // A null model indicates that this body is a sphere
        if (lit)
        {
            renderEllipsoid_GLSL(ri, ls,
                                 atmosphere, cloudTexOffset,
                                 scaleFactors,
                                 textureResolution,
                                 renderFlags,
                                 obj.orientation,
                                 viewFrustum,
                                 planetMVP, this);
        }
        else
        {
            renderSphereUnlit(ri, viewFrustum, planetMVP, this);
        }
    }
    else
    {
        if (geometry != nullptr)
        {
            ResourceHandle texOverride = obj.surface->baseTexture.texture(textureResolution);

            if (lit)
            {
                renderGeometry_GLSL(geometry,
                                    ri,
                                    texOverride,
                                    ls,
                                    obj.atmosphere,
                                    geometryScale,
                                    renderFlags,
                                    obj.orientation,
                                    astro::daysToSecs(now - astro::J2000),
                                    planetMVP, this);
            }
            else
            {
                renderGeometry_GLSL_Unlit(geometry,
                                          ri,
                                          texOverride,
                                          geometryScale,
                                          renderFlags,
                                          obj.orientation,
                                          astro::daysToSecs(now - astro::J2000),
                                          planetMVP, this);
            }
            glActiveTexture(GL_TEXTURE0);
        }
    }

    float segmentSizeInPixels = 0.0f;
    if (showRings)
    {
        // calculate ring segment size in pixels, actual size is segmentSizeInPixels * tan(segmentAngle)
        segmentSizeInPixels = 2.0f * obj.rings->outerRadius / (max(nearPlaneDistance, altitude) * pixelSize);
        if (distance <= obj.rings->innerRadius)
        {
            m_ringRenderer->renderRings(*obj.rings, ri, ls,
                                        radius, 1.0f - obj.semiAxes.y(),
                                        util::is_set(renderFlags, RenderFlags::ShowRingShadows) && lit,
                                        segmentSizeInPixels,
                                        ringsMVP, true);
        }
    }

    if (atmosphere != nullptr)
    {
        // Compute the apparent thickness in pixels of the atmosphere.
        // If it's only one pixel thick, it can look quite unsightly
        // due to aliasing.  To avoid popping, we gradually fade in the
        // atmosphere as it grows from two to three pixels thick.
        float fade;
        float thicknessInPixels = 0.0f;
        if (distance - radius > 0.0f)
        {
            thicknessInPixels = atmosphere->height /
                ((distance - radius) * pixelSize);
            fade = std::clamp(thicknessInPixels - 2.0f, 0.0f, 1.0f);
        }
        else
        {
            fade = 1.0f;
        }

        if (fade > 0 && util::is_set(renderFlags, RenderFlags::ShowAtmospheres) && atmosphere->height > 0.0f)
        {
            // Only use new atmosphere code in OpenGL 2.0 path when new style parameters are defined.
            // TODO: convert old style atmopshere parameters
            if (atmosphere->mieScaleHeight > 0.0f)
            {
                float atmScale = 1.0f + atmosphere->height / radius;

                m_atmosphereRenderer->render(
                    ri,
                    *atmosphere,
                    ls,
                    obj.orientation,
                    radius * atmScale,
                    viewFrustum,
                    planetMVP);
            }
            else
            {
                Eigen::Matrix4f modelView = math::rotate(getCameraOrientationf());
                Matrices mvp = { m.projection, &modelView };
                m_atmosphereRenderer->renderLegacy(
                    *atmosphere,
                    ls,
                    pos,
                    obj.orientation,
                    scaleFactors,
                    ri.sunDir_eye,
                    thicknessInPixels,
                    lit,
                    mvp);
            }
        }

        // If there's a cloud layer, we'll render it now.
        if (cloudTex != nullptr)
        {
            float cloudScale = 1.0f + atmosphere->cloudHeight / radius;
            Matrix4f cmv = math::scale(planetMV, cloudScale);
            Matrices mvp = { m.projection, &cmv };

            // If we're beneath the cloud level, render the interior of
            // the cloud sphere.
            if (distance - radius < atmosphere->cloudHeight)
                glFrontFace(GL_CW);

            cloudTex->bind();

            // Cloud layers can be trouble for the depth buffer, since they tend
            // to be very close to the surface of a planet relative to the radius
            // of the planet. We'll help out by offsetting the cloud layer toward
            // the viewer.
            if (distance > radius * 1.1f)
            {
                glEnable(GL_POLYGON_OFFSET_FILL);
                glPolygonOffset(-1.0f, -1.0f);
            }

            if (lit)
            {
                renderClouds_GLSL(ri, ls,
                                  atmosphere,
                                  cloudTex,
                                  cloudNormalMap,
                                  cloudTexOffset,
                                  scaleFactors,
                                  textureResolution,
                                  renderFlags,
                                  obj.orientation,
                                  viewFrustum,
                                  mvp, this);
            }
            else
            {
                renderCloudsUnlit(ri,viewFrustum, cloudTex, cloudTexOffset, mvp, this);
            }

            glDisable(GL_POLYGON_OFFSET_FILL);
            glFrontFace(GL_CCW);
        }
    }

    if (showRings)
    {
        if (lit && util::is_set(renderFlags, RenderFlags::ShowRingShadows))
        {
            Texture* ringsTex = obj.rings->texture.find(textureResolution);
            if (ringsTex != nullptr)
                ringsTex->bind();
        }

        if (distance > obj.rings->innerRadius)
        {
            m_ringRenderer->renderRings(*obj.rings, ri, ls,
                                        radius, 1.0f - obj.semiAxes.y(),
                                        util::is_set(renderFlags, RenderFlags::ShowRingShadows) && lit,
                                        segmentSizeInPixels,
                                        ringsMVP, false);
        }
    }
}


bool Renderer::testEclipse(const Body& receiver,
                           const Body& caster,
                           LightingState& lightingState,
                           unsigned int lightIndex,
                           double now)
{
    bool isReceiverShadowed = false;

    // Ignore situations where the shadow casting body is much smaller than
    // the receiver, as these shadows aren't likely to be relevant.  Also,
    // ignore eclipses where the caster is not an ellipsoid, since we can't
    // generate correct shadows in this case.
    if (caster.getRadius() >= receiver.getRadius() * MinRelativeOccluderRadius &&
        caster.hasVisibleGeometry() &&
        util::is_set(caster.getClassification(), bodyVisibilityMask) &&
        caster.extant(now) &&
        caster.isEllipsoid())
    {
        const DirectionalLight& light = lightingState.lights[lightIndex];
        LightingState::EclipseShadowVector& shadows = *lightingState.shadows[lightIndex];

        // All of the eclipse related code assumes that both the caster
        // and receiver are spherical.  Irregular receivers will work more
        // or less correctly, but casters that are sufficiently non-spherical
        // will produce obviously incorrect shadows.  Another assumption we
        // make is that the distance between the caster and receiver is much
        // less than the distance between the sun and the receiver.  This
        // approximation works everywhere in the solar system, and is likely
        // valid for any orbitally stable pair of objects orbiting a star.
        Vector3d posReceiver = receiver.getAstrocentricPosition(now);
        Vector3d posCaster = caster.getAstrocentricPosition(now);

        //const Star* sun = receiver.getSystem()->getStar();
        //assert(sun != nullptr);
        //double distToSun = posReceiver.distanceFromOrigin();
        //float appSunRadius = (float) (sun->getRadius() / distToSun);
        float appSunRadius = light.apparentSize;

        Vector3d dir = posCaster - posReceiver;
        double distToCaster = dir.norm() - receiver.getRadius();
        float appOccluderRadius = (float) (caster.getRadius() / distToCaster);

        // The shadow radius is the radius of the occluder plus some additional
        // amount that depends upon the apparent radius of the sun.  For
        // a sun that's distant/small and effectively a point, the shadow
        // radius will be the same as the radius of the occluder.
        float shadowRadius = (1 + appSunRadius / appOccluderRadius) *
            caster.getRadius();

        // Test whether a shadow is cast on the receiver.  We want to know
        // if the receiver lies within the shadow volume of the caster.  Since
        // we're assuming that everything is a sphere and the sun is far
        // away relative to the caster, the shadow volume is a
        // cylinder capped at one end.  Testing for the intersection of a
        // singly capped cylinder is as simple as checking the distance
        // from the center of the receiver to the axis of the shadow cylinder.
        // If the distance is less than the sum of the caster's and receiver's
        // radii, then we have an eclipse. We also need to verify that the
        // receiver is behind the caster when seen from the light source.
        float R = receiver.getRadius() + shadowRadius;

        // The stored light position is receiver-relative; thus the caster-to-light
        // direction is casterPos - (receiverPos + lightPos)
        Vector3d lightPosition = posReceiver + light.position;
        Vector3d lightToCasterDir = posCaster - lightPosition;
        Vector3d receiverToCasterDir = posReceiver - posCaster;

        double dist = math::distance(posReceiver,
                                     Eigen::ParametrizedLine<double, 3>(posCaster, lightToCasterDir));
        if (dist < R && lightToCasterDir.dot(receiverToCasterDir) > 0.0)
        {
            Vector3d sunDir = lightToCasterDir.normalized();

            EclipseShadow shadow;
            shadow.origin = dir.cast<float>();
            shadow.direction = sunDir.cast<float>();
            shadow.penumbraRadius = shadowRadius;

            // The umbra radius will be positive if the apparent size of the occluder
            // is greater than the apparent size of the sun, zero if they're equal,
            // and negative when the eclipse is partial. The absolute value of the
            // umbra radius is the radius of the shadow region with constant depth:
            // for total eclipses, this area is actually the umbra, with a depth of
            // 1. For annular eclipses and transits, it is less than 1.
            shadow.umbraRadius = caster.getRadius() *
                (appOccluderRadius - appSunRadius) / appOccluderRadius;
            shadow.maxDepth = std::min(1.0f, math::square(appOccluderRadius / appSunRadius));
            shadow.caster = &caster;

            // Ignore transits that don't produce a visible shadow.
            if (shadow.maxDepth > 1.0f / 256.0f)
                shadows.push_back(shadow);

            isReceiverShadowed = true;
        }

        // If the caster has a ring system, see if it casts a shadow on the receiver.
        // Ring shadows are only supported in the OpenGL 2.0 path.
        const BodyFeaturesManager* bodyFeaturesManager = GetBodyFeaturesManager();
        if (auto rings = bodyFeaturesManager->getRings(&caster); rings != nullptr)
        {
            bool shadowed = false;

            // The shadow volume of the rings is an oblique circular cylinder
            if (dist < rings->outerRadius + receiver.getRadius())
            {
                // Possible intersection, but it depends on the orientation of the
                // rings.
                Quaterniond casterOrientation = caster.getOrientation(now);
                Vector3d ringPlaneNormal = casterOrientation * Vector3d::UnitY();
                Vector3d shadowDirection = lightToCasterDir.normalized();
                Vector3d v = ringPlaneNormal.cross(shadowDirection);
                if (v.squaredNorm() < 1.0e-6)
                {
                    // Shadow direction is nearly coincident with ring plane normal, so
                    // the shadow cross section is close to circular. No additional test
                    // is required.
                    shadowed = true;
                }
                else
                {
                    // minDistance is the cross section of the ring shadows in the plane
                    // perpendicular to the ring plane and containing the light direction.
                    Vector3d shadowPlaneNormal = v.normalized().cross(shadowDirection);
                    Hyperplane<double, 3> shadowPlane(shadowPlaneNormal, posCaster - posReceiver);
                    double minDistance = receiver.getRadius() +
                        rings->outerRadius * ringPlaneNormal.dot(shadowDirection);
                    if (abs(shadowPlane.signedDistance(Vector3d::Zero())) < minDistance)
                    {
                        // TODO: Implement this test and only set shadowed to true if it passes
                    }
                    shadowed = true;
                }

                if (shadowed)
                {
                    RingShadow& shadow = lightingState.ringShadows[lightIndex];
                    shadow.origin = dir.cast<float>();
                    shadow.direction = shadowDirection.cast<float>();
                    shadow.ringSystem = rings;
                    shadow.casterOrientation = casterOrientation.cast<float>();
                }
            }
        }
    }

    return isReceiverShadowed;
}


void Renderer::renderPlanet(Body& body,
                            const Vector3f& pos,
                            float distance,
                            float appMag,
                            const Observer& observer,
                            float nearPlaneDistance,
                            float farPlaneDistance,
                            const Matrices &m)
{
    double now = observer.getTime();
    float altitude = distance - body.getRadius();
    float discSizeInPixels = body.getRadius() /
        (max(nearPlaneDistance, altitude) * pixelSize);

    float maxDiscSize = (starStyle == StarStyle::ScaledDiscStars) ? MaxScaledDiscStarSize : 1.0f;
    if (discSizeInPixels >= maxDiscSize && body.hasVisibleGeometry())
    {
        auto bodyFeaturesManager = GetBodyFeaturesManager();

        RenderProperties rp;
        if (displayedSurface.empty())
        {
            rp.surface = &body.getSurface();
        }
        else
        {
            rp.surface = bodyFeaturesManager->getAlternateSurface(&body, displayedSurface);
            if (rp.surface == nullptr)
                rp.surface = &body.getSurface();
        }
        rp.atmosphere = bodyFeaturesManager->getAtmosphere(&body);
        rp.rings = bodyFeaturesManager->getRings(&body);
        rp.radius = body.getRadius();
        rp.geometry = body.getGeometry();
        rp.semiAxes = body.getSemiAxes() * (1.0f / rp.radius);
        rp.geometryScale = body.getGeometryScale();

        Quaterniond q = body.getRotationModel(now)->spin(now) *
                        body.getEclipticToEquatorial(now);

        rp.orientation = body.getGeometryOrientation() * q.cast<float>();

        if (util::is_set(labelMode, RenderLabels::LocationLabels))
            bodyFeaturesManager->computeLocations(&body);

        Vector3f scaleFactors;
        bool isNormalized = false;
        const Geometry* geometry = nullptr;
        if (rp.geometry != InvalidResource)
            geometry = engine::GetGeometryManager()->find(rp.geometry);
        if (geometry == nullptr || geometry->isNormalized())
        {
            scaleFactors = rp.semiAxes * rp.radius;
            isNormalized = true;
        }
        else
        {
            scaleFactors = Vector3f::Constant(rp.geometryScale);
        }

        LightingState lights;
        setupObjectLighting(lightSourceList,
                            secondaryIlluminators,
                            rp.orientation,
                            scaleFactors,
                            pos,
                            isNormalized,
                            lights);
        assert(lights.nLights <= MaxLights);

        lights.ambientColor = ambientColor.toVector3();

        // Clear out the list of eclipse shadows
        for (unsigned int li = 0; li < lights.nLights; li++)
        {
            eclipseShadows[li].clear();
            lights.shadows[li] = &eclipseShadows[li];
        }


        // Add ring shadow records for each light
        if (rp.rings != nullptr &&
            util::is_set(renderFlags, RenderFlags::ShowPlanetRings) &&
            util::is_set(renderFlags, RenderFlags::ShowRingShadows))
        {
            for (unsigned int li = 0; li < lights.nLights; li++)
            {
                lights.ringShadows[li].ringSystem = rp.rings;
                lights.ringShadows[li].casterOrientation = q.cast<float>();
                lights.ringShadows[li].origin = Vector3f::Zero();
                lights.ringShadows[li].direction = -lights.lights[li].position.normalized().cast<float>();
            }
        }

        // Calculate eclipse circumstances
        if (util::is_set(renderFlags, RenderFlags::ShowEclipseShadows) && body.getSystem() != nullptr)
        {
            if (const auto *system = body.getSystem(); system->getPrimaryBody() == nullptr)
            {
                // The body is a planet.  Check for eclipse shadows
                // from all of its satellites.
                if (const auto *satellites = body.getSatellites(); satellites != nullptr)
                {
                    int nSatellites = satellites->getSystemSize();
                    for (unsigned int li = 0; li < lights.nLights; li++)
                    {
                        if (lights.lights[li].castsShadows)
                        {
                            for (int i = 0; i < nSatellites; i++)
                            {
                                testEclipse(body, *satellites->getBody(i), lights, li, now);
                            }
                        }
                    }
                }
            }
            else
            {
                for (unsigned int li = 0; li < lights.nLights; li++)
                {
                    if (lights.lights[li].castsShadows)
                    {
                        // The body is a moon.  Check for eclipse shadows from
                        // the parent planet and all satellites in the system.
                        // Traverse up the hierarchy so that any parent objects
                        // of the parent are also considered (TODO: their child
                        // objects will not be checked for shadows.)
                        const Body* planet = system->getPrimaryBody();
                        while (planet != nullptr)
                        {
                            testEclipse(body, *planet, lights, li, now);
                            if (planet->getSystem() != nullptr)
                                planet = planet->getSystem()->getPrimaryBody();
                            else
                                planet = nullptr;
                        }

                        int nSatellites = system->getSystemSize();
                        for (int i = 0; i < nSatellites; i++)
                        {
                            if (system->getBody(i) != &body)
                            {
                                testEclipse(body, *system->getBody(i), lights, li, now);
                            }
                        }
                    }
                }
            }
        }

        // Sort out the ring shadows; only one ring shadow source is supported right now. This means
        // that exotic cases with shadows from two ring different ring systems aren't handled.
        for (unsigned int li = 0; li < lights.nLights; li++)
        {
            RingSystem* rings = lights.ringShadows[li].ringSystem;
            if (rings != nullptr)
            {
                // Use the first set of ring shadows found (shadowing the brightest light
                // source.)
                if (lights.shadowingRingSystem == nullptr)
                {
                    lights.shadowingRingSystem = rings;
                    lights.ringPlaneNormal = (rp.orientation * lights.ringShadows[li].casterOrientation.conjugate()) * Vector3f::UnitY();
                    lights.ringCenter = rp.orientation * lights.ringShadows[li].origin;
                }

                // Light sources have a finite size, which causes some blurring of the texture. Simulate
                // this effect by using a lower LOD (i.e. a smaller mipmap level, indicated somewhat
                // confusingly by a _higher_ LOD value.
                float ringWidth = rings->outerRadius - rings->innerRadius;
                float projectedRingSize = std::abs(lights.lights[li].direction_obj.dot(lights.ringPlaneNormal)) * ringWidth;
                float projectedRingSizeInPixels = projectedRingSize / (max(nearPlaneDistance, altitude) * pixelSize);
                const Texture* ringsTex = rings->texture.find(textureResolution);
                if (ringsTex != nullptr)
                {
                    // Calculate the approximate distance from the shadowed object to the rings
                    Hyperplane<float, 3> ringPlane(lights.ringPlaneNormal, lights.ringCenter);
                    float cosLightAngle = lights.lights[li].direction_obj.dot(ringPlane.normal());
                    float approxRingDistance = rings->innerRadius;
                    if (abs(cosLightAngle) < 0.99999f)
                    {
                        approxRingDistance = abs(ringPlane.offset() / cosLightAngle);
                    }
                    if (lights.ringCenter.norm() < rings->innerRadius)
                    {
                        approxRingDistance = max(approxRingDistance, rings->innerRadius - lights.ringCenter.norm());
                    }

                    // Calculate the LOD based on the size of the smallest
                    // ring feature relative to the apparent size of the light source.
                    float ringTextureWidth = ringsTex->getWidth();
                    float ringFeatureSize = (projectedRingSize / ringTextureWidth) / approxRingDistance;
                    float relativeFeatureSize = lights.lights[li].apparentSize / ringFeatureSize;
                    //float areaLightLod = log(max(relativeFeatureSize, 1.0f)) / log(2.0f);
                    float areaLightLod = log2(max(relativeFeatureSize, 1.0f));

                    // Compute the LOD that would be automatically used by the GPU.
                    float texelToPixelRatio = ringTextureWidth / projectedRingSizeInPixels;
                    float gpuLod = log2(texelToPixelRatio);

                    //float lod = max(areaLightLod, log(texelToPixelRatio) / log(2.0f));
                    float lod = max(areaLightLod, gpuLod);

                    // maxLOD is the index of the smallest mipmap (or close to it for non-power-of-two
                    // textures.) We can't make the lod larger than this.
                    float maxLod = log2((float) ringsTex->getWidth());
                    if (maxLod > 1.0f)
                    {
                        // Avoid using the 1x1 mipmap, as it appears to cause 'bleeding' when
                        // the light source is very close to the ring plane. This is probably
                        // a numerical precision issue from calculating the intersection of
                        // between a ray and plane that are nearly parallel.
                        maxLod -= 1.0f;
                    }
                    lod = min(lod, maxLod);

                    // Not all hardware/drivers support GLSL's textureXDLOD instruction, which lets
                    // us explicitly set the LOD. But, they do all have an optional lodBias parameter
                    // for the textureXD instruction. The bias is just the difference between the
                    // area light LOD and the approximate GPU calculated LOD.
                    if (!gl::ARB_shader_texture_lod)
                        lod = max(0.0f, lod - gpuLod);
                    lights.ringShadows[li].texLod = lod;
                }
                else
                {
                    lights.ringShadows[li].texLod = 0.0f;
                }
            }
        }

        renderObject(pos, distance, observer,
                     nearPlaneDistance, farPlaneDistance,
                     rp, lights, m);

        if (bodyFeaturesManager->hasLocations(&body) && util::is_set(labelMode, RenderLabels::LocationLabels))
        {
            // Set up location markers for this body
            using namespace celestia;
            mountainRep    = MarkerRepresentation(MarkerRepresentation::Triangle, 8.0f, LocationLabelColor);
            craterRep      = MarkerRepresentation(MarkerRepresentation::Circle,   8.0f, LocationLabelColor);
            observatoryRep = MarkerRepresentation(MarkerRepresentation::Plus,     8.0f, LocationLabelColor);
            cityRep        = MarkerRepresentation(MarkerRepresentation::X,        3.0f, LocationLabelColor);
            genericLocationRep = MarkerRepresentation(MarkerRepresentation::Square, 8.0f, LocationLabelColor);

            // We need a double precision body-relative position of the
            // observer, otherwise location labels will tend to jitter.
            Vector3d posd = body.getPosition(observer.getTime()).offsetFromKm(observer.getPosition());
            locationsToAnnotations(body, posd, q);
        }
    }

    if (body.isVisibleAsPoint())
    {
        if (float maxCoeff = body.getSurface().color.toVector3().maxCoeff(); maxCoeff > 0.0f) // ignore [ 0 0 0 ]; used by old addons to make objects not get rendered as point
        {
            renderObjectAsPoint(pos,
                                body.getRadius(),
                                appMag,
                                discSizeInPixels,
                                body.getSurface().color * (1.0f / maxCoeff), // normalize point color; 'darkness' is handled by size of point determined by GeomAlbedo.
                                false, false, m);
        }
    }
}


void Renderer::renderStar(const Star& star,
                          const Vector3f& pos,
                          float distance,
                          float appMag,
                          const Observer& observer,
                          float nearPlaneDistance,
                          float farPlaneDistance,
                          const Matrices &m)
{
    if (!star.getVisibility())
        return;

    Color color = starColors.lookupColor(star.getTemperature());
    float radius = star.getRadius();
    float discSizeInPixels = radius / (distance * pixelSize);

    if (discSizeInPixels > 1)
    {
        Surface surface;
        RenderProperties rp;

        surface.color = color;

        if (MultiResTexture mtex = star.getTexture(); mtex.texture(textureResolution) != InvalidResource)
            surface.baseTexture = mtex;
        else
            surface.baseTexture = InvalidResource;
        surface.appearanceFlags |= Surface::ApplyBaseTexture;
        surface.appearanceFlags |= Surface::Emissive;

        rp.surface = &surface;
        rp.rings = nullptr;
        rp.radius = star.getRadius();
        rp.semiAxes = star.getEllipsoidSemiAxes();
        rp.geometry = star.getGeometry();

        Atmosphere atmosphere;

        // Use atmosphere effect to give stars a fuzzy fringe
        if (star.hasCorona() && rp.geometry == InvalidResource)
        {
            Color atmColor(color.red() * 0.5f, color.green() * 0.5f, color.blue() * 0.5f);
            atmosphere.height = radius * CoronaHeight;
            atmosphere.lowerColor = atmColor;
            atmosphere.upperColor = atmColor;
            atmosphere.skyColor = atmColor;

            rp.atmosphere = &atmosphere;
        }
        else
        {
            rp.atmosphere = nullptr;
        }

        rp.orientation = star.getRotationModel()->orientationAtTime(observer.getTime()).cast<float>();

        renderObject(pos, distance, observer,
                     nearPlaneDistance, farPlaneDistance,
                     rp, LightingState(), m);
    }

    renderObjectAsPoint(pos,
                        star.getRadius(),
                        appMag,
                        discSizeInPixels,
                        color,
                        star.hasCorona(), true,
                        m);
}


// Compute a rough estimate of the visible length of the dust tail.
// TODO: This is old code that needs to be rewritten. For one thing,
// the length is inversely proportional to the distance from the sun,
// whereas the 1/distance^2 is probably more realistic. There should
// also be another parameter that specifies how active the comet is.
static float cometDustTailLength(float distanceToSun,
                                 float radius)
{
    return (1.0e8f / distanceToSun) * (radius / 5.0f) * 1.0e7f;
}


void Renderer::renderCometTail(const Body& body,
                               const Vector3f& pos,
                               const Observer& observer,
                               float dustTailLength,
                               float discSizeInPixels,
                               const Matrices &m)
{
    m_cometRenderer->render(body, observer, pos, dustTailLength, discSizeInPixels, m);
}


// Render a reference mark
void Renderer::renderReferenceMark(const ReferenceMark& refMark,
                                   const Vector3f& pos,
                                   float distance,
                                   double now,
                                   float nearPlaneDistance,
                                   const Matrices &m)
{
    float altitude = distance - refMark.boundingSphereRadius();
    float discSizeInPixels = refMark.boundingSphereRadius() /
        (max(nearPlaneDistance, altitude) * pixelSize);

    if (discSizeInPixels <= 1)
        return;

    refMark.render(this, pos, discSizeInPixels, now, m);
}


void Renderer::renderAsterisms(const Universe& universe, float dist, const Matrices& mvp)
{
    auto *asterisms = universe.getAsterisms();

    if (!util::is_set(renderFlags, RenderFlags::ShowDiagrams) || asterisms == nullptr)
        return;

    if (m_asterismRenderer == nullptr || !m_asterismRenderer->sameAsterisms(asterisms))
    {
        m_asterismRenderer = std::make_unique<AsterismRenderer>(*this, asterisms);
    }

    float opacity = 1.0f;
    if (dist > MaxAsterismLinesConstDist)
    {
        opacity = std::clamp((MaxAsterismLinesConstDist - dist)
                             / (MaxAsterismLinesDist - MaxAsterismLinesConstDist) + 1.0f,
                             0.0f,
                             1.0f);
    }

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.smoothLines = true;
    setPipelineState(ps);

    m_asterismRenderer->render(Color(ConstellationColor, opacity), mvp);
}


void Renderer::renderBoundaries(const Universe& universe, float dist, const Matrices& mvp)
{
    auto boundaries = universe.getBoundaries();
    if (!util::is_set(renderFlags, RenderFlags::ShowBoundaries) || boundaries == nullptr)
        return;

    if (m_boundariesRenderer == nullptr || !m_boundariesRenderer->sameBoundaries(boundaries))
    {
        m_boundariesRenderer = std::make_unique<BoundariesRenderer>(*this, boundaries);
    }

    /* We'll linearly fade the boundaries as a function of the
       observer's distance to the origin of coordinates: */
    float opacity = 1.0f;
    if (dist > MaxAsterismLabelsConstDist)
    {
        opacity = std::clamp((MaxAsterismLabelsConstDist - dist)
                             / (MaxAsterismLabelsDist - MaxAsterismLabelsConstDist) + 1.0f,
                             0.0f,
                             1.0f);
    }

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.smoothLines = true;
    setPipelineState(ps);

    m_boundariesRenderer->render(Color(BoundaryColor, opacity), mvp);
}


// Helper function to compute the luminosity of a perfectly
// reflective disc with the specified radius. This is used as an upper
// bound for the apparent brightness of an object when culling
// invisible objects.
static float luminosityAtOpposition(float sunLuminosity,
                                    float distanceFromSun,
                                    float objRadius)
{
    // Compute the total power of the star in Watts
    double power = astro::SOLAR_POWER * sunLuminosity;

    // Compute the irradiance at the body's distance from the star
    double irradiance = power / math::sphereArea(distanceFromSun * 1000);

    // Compute the total energy hitting the planet; assume an albedo of 1.0, so
    // reflected energy = incident energy.
    double incidentEnergy = irradiance * math::circleArea(objRadius * 1000);

    // Compute the luminosity (i.e. power relative to solar power)
    return (float) (incidentEnergy / astro::SOLAR_POWER);
}


static bool isBodyVisible(const Body* body, BodyClassification bodyVisibilityMask)
{
    BodyClassification bodyClassification = body->getClassification();
    switch (bodyClassification)
    {
    // Diffuse objects don't have controls to show/hide visibility
    case BodyClassification::Diffuse:
        return body->isVisible();

    // SurfaceFeature and Component inherit visibility of its parent body
    case BodyClassification::Component:
    case BodyClassification::SurfaceFeature:
        for (const PlanetarySystem* system = body->getSystem(); system != nullptr;)
        {
            auto primaryBody = system->getPrimaryBody();
            if (primaryBody == nullptr)
            {
                // TODO figure out what to do about components/features of stars/barycenters
                return false;
            }

            if (auto primaryClassification = primaryBody->getClassification();
                !util::is_set(primaryClassification, BodyClassification::SurfaceFeature | BodyClassification::Component))
            {
                return primaryBody->isVisible() && util::is_set(primaryClassification, bodyVisibilityMask);
            }

            system = primaryBody->getSystem();
        }

    default:
        return body->isVisible() && util::is_set(bodyClassification, bodyVisibilityMask);
    }
}

void Renderer::addRenderListEntries(RenderListEntry& rle,
                                    Body& body,
                                    bool isLabeled)
{
    bool visibleAsPoint = rle.appMag < faintestPlanetMag && body.isVisibleAsPoint();
    const BodyFeaturesManager* bodyFeaturesManager = GetBodyFeaturesManager();

    if (rle.discSizeInPixels > 1 || visibleAsPoint || isLabeled)
    {
        rle.renderableType = RenderListEntry::RenderableBody;
        rle.body = &body;

        if (body.getGeometry() != InvalidResource && rle.discSizeInPixels > 1)
        {
            const Geometry* geometry = engine::GetGeometryManager()->find(body.getGeometry());
            if (geometry == nullptr)
                rle.isOpaque = true;
            else
                rle.isOpaque = geometry->isOpaque();
        }
        else
        {
            rle.isOpaque = true;
        }
        rle.radius = body.getRadius();
        if (const RingSystem* rings = bodyFeaturesManager->getRings(&body); rings != nullptr)
            rle.radius += rings->outerRadius;
        renderList.push_back(rle);
    }

    if (body.getClassification() == BodyClassification::Comet && util::is_set(renderFlags, RenderFlags::ShowCometTails))
    {
        float radius = cometDustTailLength(rle.sun.norm(), body.getRadius());
        float discSize = (radius / rle.distance) / pixelSize;
        if (discSize > 1)
        {
            rle.renderableType = RenderListEntry::RenderableCometTail;
            rle.body = &body;
            rle.isOpaque = false;
            rle.radius = radius;
            rle.discSizeInPixels = discSize;
            renderList.push_back(rle);
        }
    }

    bodyFeaturesManager->processReferenceMarks(&body,
                                               [this, &rle](const ReferenceMark* rm)
                                               {
                                                   rle.renderableType = RenderListEntry::RenderableReferenceMark;
                                                   rle.refMark = rm;
                                                   rle.isOpaque = rm->isOpaque();
                                                   rle.radius = rm->boundingSphereRadius();
                                                   renderList.push_back(rle);
                                               });
}


void Renderer::buildRenderLists(const Vector3d& astrocentricObserverPos,
                                const math::InfiniteFrustum& viewFrustum,
                                const Vector3d& viewPlaneNormal,
                                const Vector3d& frameCenter,
                                const FrameTree* tree,
                                const Observer& observer,
                                double now)
{
    BodyClassification labelClassMask = translateLabelModeToClassMask(labelMode);

    Matrix3f viewMat = getCameraOrientationf().toRotationMatrix();
    Vector3f viewMatZ = viewMat.row(2);
    double invCosViewAngle = 1.0 / cosViewConeAngle;
    double sinViewAngle = sqrt(1.0 - math::square(cosViewConeAngle));

    unsigned int nChildren = tree != nullptr ? tree->childCount() : 0;
    for (unsigned int i = 0; i < nChildren; i++)
    {
        const TimelinePhase* phase = tree->getChild(i);

        // No need to do anything if the phase isn't active now
        if (!phase->includes(now))
            continue;

        Body* body = phase->body();

        // pos_s: sun-relative position of object
        // pos_v: viewer-relative position of object

        // Get the position of the body relative to the sun.
        Vector3d p = phase->orbit()->positionAtTime(now);
        Vector3d pos_s = frameCenter + phase->orbitFrame()->getOrientation(now).conjugate() * p;

        // We now have the positions of the observer and the planet relative
        // to the sun.  From these, compute the position of the body
        // relative to the observer.
        Vector3d pos_v = pos_s - astrocentricObserverPos;

        // dist_vn: distance along view normal from the viewer to the
        // projection of the object's center.
        double dist_vn = viewPlaneNormal.dot(pos_v);

        // Vector from object center to its projection on the view normal.
        Vector3d toViewNormal = pos_v - dist_vn * viewPlaneNormal;

        float cullingRadius = body->getCullingRadius();

        // The result of the planetshine test can be reused for the view cone
        // test, but only when the object's light influence sphere is larger
        // than the geometry. This is not
        bool viewConeTestFailed = false;
        if (body->isSecondaryIlluminator())
        {
            float influenceRadius = body->getBoundingRadius() + (body->getRadius() * PLANETSHINE_DISTANCE_LIMIT_FACTOR);
            if (dist_vn > -influenceRadius)
            {
                double maxPerpDist = (influenceRadius + dist_vn * sinViewAngle) * invCosViewAngle;
                double perpDistSq = toViewNormal.squaredNorm();
                if (perpDistSq < maxPerpDist * maxPerpDist)
                {
                    if ((body->getRadius() / (float) pos_v.norm()) / pixelSize > PLANETSHINE_PIXEL_SIZE_LIMIT)
                    {
                        // add to planetshine list if larger than 1/10 pixel
#if DEBUG_SECONDARY_ILLUMINATION
                        clog << "Planetshine: " << body->getName()
                             << ", " << body->getRadius() / (float) pos_v.length() / pixelSize << endl;
#endif
                        SecondaryIlluminator illum;
                        illum.body = body;
                        illum.position_v = pos_v;
                        illum.radius = body->getRadius();
                        secondaryIlluminators.push_back(illum);
                    }
                }
                else
                {
                    viewConeTestFailed = influenceRadius > cullingRadius;
                }
            }
            else
            {
                viewConeTestFailed = influenceRadius > cullingRadius;
            }
        }

        bool insideViewCone = false;
        if (!viewConeTestFailed)
        {
            float radius = body->getCullingRadius();
            if (dist_vn > -radius)
            {
                double maxPerpDist = (radius + dist_vn * sinViewAngle) * invCosViewAngle;
                double perpDistSq = toViewNormal.squaredNorm();
                insideViewCone = perpDistSq < maxPerpDist * maxPerpDist;
            }
        }

        if (insideViewCone)
        {
            // Calculate the distance to the viewer
            double dist_v = pos_v.norm();

            // Calculate the size of the planet/moon disc in pixels
            float discSize = (body->getCullingRadius() / (float) dist_v) / pixelSize;

            // Compute the apparent magnitude; instead of summing the reflected
            // light from all nearby stars, we just consider the one with the
            // highest apparent brightness.
            float appMag = 100.0f;
            for (const auto &lightSource : lightSourceList)
            {
                Eigen::Vector3d sunPos = pos_v - lightSource.position;
                appMag = std::min(appMag, body->getApparentMagnitude(lightSource.luminosity, sunPos, pos_v));
            }

            bool visibleAsPoint = appMag < faintestPlanetMag && body->isVisibleAsPoint();
            bool isLabeled = util::is_set(body->getOrbitClassification(), labelClassMask);

            if ((discSize > 1 || visibleAsPoint || isLabeled) && isBodyVisible(body, bodyVisibilityMask))
            {
                RenderListEntry rle;

                rle.position = pos_v.cast<float>();
                rle.distance = (float) dist_v;
                rle.centerZ = pos_v.cast<float>().dot(viewMatZ);
                rle.appMag   = appMag;
                rle.discSizeInPixels = body->getRadius() / ((float) dist_v * pixelSize);

                // TODO: Remove this. It's only used in two places: for calculating comet tail
                // length, and for calculating sky brightness to adjust the limiting magnitude.
                // In both cases, it's the wrong quantity to use (e.g. for objects with orbits
                // defined relative to the SSB.)
                rle.sun = -pos_s.cast<float>();

                addRenderListEntries(rle, *body, isLabeled);
            }
        }

        const FrameTree* subtree = body->getFrameTree();
        if (subtree != nullptr)
        {
            double dist_v = pos_v.norm();
            bool traverseSubtree = false;

            // There are two different tests available to determine whether we can reject
            // the object's subtree. If the subtree contains no light reflecting objects,
            // then render the subtree only when:
            //    - the subtree bounding sphere intersects the view frustum, and
            //    - the subtree contains an object bright or large enough to be visible.
            // Otherwise, render the subtree when any of the above conditions are
            // true or when a subtree object could potentially illuminate something
            // in the view cone.
            auto minPossibleDistance = (float) (dist_v - subtree->boundingSphereRadius());
            float brightestPossible = 0.0f;
            float largestPossible = 0.0f;

            // If the viewer is not within the subtree bounding sphere, see if we can cull it because
            // it contains no objects brighter than the limiting magnitude and no objects that will
            // be larger than one pixel in size.
            if (minPossibleDistance > 1.0f)
            {
                // Figure out the magnitude of the brightest possible object in the subtree.

                // Compute the luminosity from reflected light of the largest object in the subtree
                float lum = 0.0f;
                for (const auto &lightSource : lightSourceList)
                {
                    Eigen::Vector3d sunPos = pos_v - lightSource.position;
                    lum += luminosityAtOpposition(lightSource.luminosity, (float) sunPos.norm(), (float) subtree->maxChildRadius());
                }
                brightestPossible = astro::lumToAppMag(lum, astro::kilometersToLightYears(minPossibleDistance));
                largestPossible = (float) subtree->maxChildRadius() / minPossibleDistance / pixelSize;
            }
            else
            {
                // Viewer is within the bounding sphere, so the object could be very close.
                // Assume that an object in the subree could be very bright or large,
                // so no culling will occur.
                brightestPossible = -100.0f;
                largestPossible = 100.0f;
            }

            if (brightestPossible < faintestPlanetMag || largestPossible > 1.0f)
            {
                // See if the object or any of its children are within the view frustum
                if (viewFrustum.testSphere(pos_v.cast<float>(), (float) subtree->boundingSphereRadius()) != math::FrustumAspect::Outside)
                {
                    traverseSubtree = true;
                }
            }

            // If the subtree contains secondary illuminators, do one last check if it hasn't
            // already been determined if we need to traverse the subtree: see if something
            // in the subtree could possibly contribute significant illumination to an
            // object in the view cone.
            if (subtree->containsSecondaryIlluminators() &&
                !traverseSubtree                         &&
                largestPossible > PLANETSHINE_PIXEL_SIZE_LIMIT)
            {
                auto influenceRadius = (float) (subtree->boundingSphereRadius() +
                    (subtree->maxChildRadius() * PLANETSHINE_DISTANCE_LIMIT_FACTOR));
                if (dist_vn > -influenceRadius)
                {
                    double maxPerpDist = (influenceRadius + dist_vn * sinViewAngle) * invCosViewAngle;
                    double perpDistSq = toViewNormal.squaredNorm();
                    if (perpDistSq < maxPerpDist * maxPerpDist)
                        traverseSubtree = true;
                }
            }

            if (traverseSubtree)
            {
                buildRenderLists(astrocentricObserverPos,
                                 viewFrustum,
                                 viewPlaneNormal,
                                 pos_s,
                                 subtree,
                                 observer,
                                 now);
            }
        } // end subtree traverse
    }
}


void Renderer::buildOrbitLists(const Vector3d& astrocentricObserverPos,
                               const Quaterniond& observerOrientation,
                               const math::InfiniteFrustum& viewFrustum,
                               const FrameTree* tree,
                               double now)
{
    Matrix3d viewMat = observerOrientation.toRotationMatrix();
    Vector3d viewMatZ = viewMat.row(2);

    unsigned int nChildren = tree != nullptr ? tree->childCount() : 0;
    for (unsigned int i = 0; i < nChildren; i++)
    {
        const TimelinePhase* phase = tree->getChild(i);

        // No need to do anything if the phase isn't active now
        if (!phase->includes(now))
            continue;

        Body* body = phase->body();

        // pos_s: sun-relative position of object
        // pos_v: viewer-relative position of object

        // Get the position of the body relative to the sun.
        Vector3d pos_s = body->getAstrocentricPosition(now);

        // We now have the positions of the observer and the planet relative
        // to the sun.  From these, compute the position of the body
        // relative to the observer.
        Vector3d pos_v = pos_s - astrocentricObserverPos;

        // Only show orbits for major bodies or selected objects.
        Body::VisibilityPolicy orbitVis = body->getOrbitVisibility();

        if (body->isVisible() &&
            (body == highlightObject.body() ||
             orbitVis == Body::AlwaysVisible ||
             (orbitVis == Body::UseClassVisibility && util::is_set(body->getOrbitClassification(), orbitMask))))
        {
            Vector3d orbitOrigin = Vector3d::Zero();
            Selection centerObject = phase->orbitFrame()->getCenter();
            if (centerObject.body() != nullptr)
            {
                orbitOrigin = centerObject.body()->getAstrocentricPosition(now);
            }

            // Calculate the origin of the orbit relative to the observer
            Vector3d relOrigin = orbitOrigin - astrocentricObserverPos;

            // Compute the size of the orbit in pixels
            double originDistance = pos_v.norm();
            double boundingRadius = body->getOrbit(now)->getBoundingRadius();
            auto orbitRadiusInPixels = (float) (boundingRadius / (originDistance * pixelSize));

            if (orbitRadiusInPixels > minOrbitSize)
            {
                // Add the orbit of this body to the list of orbits to be rendered
                OrbitPathListEntry path;
                path.body = body;
                path.star = nullptr;
                path.centerZ = (float) relOrigin.dot(viewMatZ);
                path.radius = (float) boundingRadius;
                path.origin = relOrigin;
                path.opacity = sizeFade(orbitRadiusInPixels, minOrbitSize, 2.0f);
                orbitPathList.push_back(path);
            }
        }

        const FrameTree* subtree = body->getFrameTree();
        if (subtree != nullptr)
        {
            // Only try to render orbits of child objects when:
            //   - The apparent size of the subtree bounding sphere is large enough that
            //     orbit paths will be visible, and
            //   - The subtree bounding sphere isn't outside the view frustum
            double dist_v = pos_v.norm();
            auto distanceToBoundingSphere = (float) (dist_v - subtree->boundingSphereRadius());
            bool traverseSubtree = false;
            if (distanceToBoundingSphere > 0.0f)
            {
                // We're inside the subtree's bounding sphere
                traverseSubtree = true;
            }
            else
            {
                float maxPossibleOrbitSize = (float) subtree->boundingSphereRadius() / ((float) dist_v * pixelSize);
                if (maxPossibleOrbitSize > minOrbitSize)
                    traverseSubtree = true;
            }

            if (traverseSubtree)
            {
                // See if the object or any of its children are within the view frustum
                if (viewFrustum.testSphere(pos_v.cast<float>(), (float) subtree->boundingSphereRadius()) != math::FrustumAspect::Outside)
                {
                    buildOrbitLists(astrocentricObserverPos,
                                    observerOrientation,
                                    viewFrustum,
                                    subtree,
                                    now);
                }
            }
        } // end subtree traverse
    }
}


static Color getBodyLabelColor(BodyClassification classification)
{
    switch (classification)
    {
    case BodyClassification::Planet:
        return Renderer::PlanetLabelColor;
    case BodyClassification::DwarfPlanet:
        return Renderer::DwarfPlanetLabelColor;
    case BodyClassification::Moon:
        return Renderer::MoonLabelColor;
    case BodyClassification::MinorMoon:
        return Renderer::MinorMoonLabelColor;
    case BodyClassification::Asteroid:
        return Renderer::AsteroidLabelColor;
    case BodyClassification::Comet:
        return Renderer::CometLabelColor;
    case BodyClassification::Spacecraft:
        return Renderer::SpacecraftLabelColor;
    default:
        return Color::Black;
    }
}

void Renderer::buildLabelLists(const math::InfiniteFrustum& viewFrustum,
                               double now)
{
    BodyClassification labelClassMask = translateLabelModeToClassMask(labelMode);
    const Body* lastPrimary = nullptr;
    math::Sphered primarySphere;

    for (const auto &ri : renderList)
    {
        if (ri.renderableType != RenderListEntry::RenderableBody)
            continue;

        if (!util::is_set(ri.body->getOrbitClassification(), labelClassMask))
            continue;

        if (viewFrustum.testSphere(ri.position, ri.radius) == math::FrustumAspect::Outside)
            continue;

        const Body* body = ri.body;
        auto boundingRadiusSize = (float) (body->getOrbit(now)->getBoundingRadius() / ri.distance) / pixelSize;
        if (boundingRadiusSize <= minOrbitSize)
            continue;

        if (body->getName().empty())
            continue;

        const TimelinePhase* phase = body->getTimeline()->findPhase(now).get();
        const Body* primary = phase->orbitFrame()->getCenter().body();
        if (primary != nullptr && util::is_set(primary->getClassification(), BodyClassification::Invisible))
        {
            const Body* parent = phase->orbitFrame()->getCenter().body();
            if (parent != nullptr)
                primary = parent;
        }

        // Position the label slightly in front of the object along a line from
        // object center to viewer.
        Vector3f pos = ri.position;
        pos = pos * (1.0f - body->getBoundingRadius() * 1.01f / pos.norm());

        // Try and position the label so that it's not partially
        // occluded by other objects. We'll consider just the object
        // that the labeled body is orbiting (its primary) as a
        // potential occluder. If a ray from the viewer to labeled
        // object center intersects the occluder first, skip
        // rendering the object label. Otherwise, ensure that the
        // label is completely in front of the primary by projecting
        // it onto the plane tangent to the primary at the
        // viewer-primary intersection point. Whew. Don't do any of
        // this if the primary isn't an ellipsoid.
        //
        // This only handles the problem of partial label occlusion
        // for low orbiting and surface positioned objects, but that
        // case is *much* more common than other possibilities.
        if (primary != nullptr && primary->isEllipsoid())
        {
            // In the typical case, we're rendering labels for many
            // objects that orbit the same primary. Avoid repeatedly
            // calling getPosition() by caching the last primary
            // position.
            if (primary != lastPrimary)
            {
                Vector3d p = phase->orbitFrame()->getOrientation(now).conjugate() *
                             phase->orbit()->positionAtTime(now);
                Vector3d v = ri.position.cast<double>() - p;

                primarySphere = math::Sphered(v, primary->getRadius());
                lastPrimary = primary;
            }

            Eigen::ParametrizedLine<double, 3> testRay(Vector3d::Zero(), pos.cast<double>());

            // Test the viewer-to-labeled object ray against
            // the primary sphere (TODO: handle ellipsoids)
            double t = 0.0;
            bool isBehindPrimary = false;
            if (testIntersection(testRay, primarySphere, t))
            {
                // Center of labeled object is behind primary
                // sphere; mark it for rejection.
                isBehindPrimary = t < 1.0;
            }

            if (!isBehindPrimary)
            {
                // Not rejected. Compute the plane tangent to
                // the primary at the viewer-to-primary
                // intersection point.
                Vector3d primaryVec = primarySphere.center;
                double distToPrimary = primaryVec.norm();
                double t = 1.0 - primarySphere.radius / distToPrimary;
                double distance = primaryVec.dot(primaryVec * t);

                // Compute the intersection of the viewer-to-labeled
                // object ray with the tangent plane.
                Vector3d posd = pos.cast<double>();
                float u = (float)(distance / primaryVec.dot(posd));

                // If the intersection point is closer to the viewer
                // than the label, then project the label onto the
                // tangent plane.
                if (u < 1.0f && u > 0.0f)
                    pos = pos * u;
            }
        }

        Color labelColor = getBodyLabelColor(ri.body->getOrbitClassification());
        float opacity = sizeFade(boundingRadiusSize, minOrbitSize, 2.0f);
        labelColor.alpha(opacity * labelColor.alpha());
        addSortedAnnotation(nullptr, body->getName(true), labelColor, pos);
    } // for each render list entry
}


// Add a star orbit to the render list
void Renderer::addStarOrbitToRenderList(const Star& star,
                                        const Observer& observer,
                                        double now)
{
    // If the star isn't fixed, add its orbit to the render list
    if (!util::is_set(renderFlags, RenderFlags::ShowOrbits))
        return;
    if (!util::is_set(orbitMask, BodyClassification::Stellar) && highlightObject.star() != &star)
        return;
    if (star.getOrbit() == nullptr)
        return;

    Matrix3d viewMat = getCameraOrientation().toRotationMatrix();
    Vector3d viewMatZ = viewMat.row(2);

    // Get orbit origin relative to the observer
    Vector3d orbitOrigin = star.getOrbitBarycenterPosition(now).offsetFromKm(observer.getPosition());

    // Compute the size of the orbit in pixels
    double originDistance = orbitOrigin.norm();
    double boundingRadius = star.getOrbit()->getBoundingRadius();
    auto orbitRadiusInPixels = (float)(boundingRadius / (originDistance * pixelSize));

    if (orbitRadiusInPixels > minOrbitSize)
    {
        // Add the orbit of this body to the list of orbits to be rendered
        OrbitPathListEntry path;
        path.star = &star;
        path.body = nullptr;
        path.centerZ = (float)orbitOrigin.dot(viewMatZ);
        path.radius = (float)boundingRadius;
        path.origin = orbitOrigin;
        path.opacity = sizeFade(orbitRadiusInPixels, minOrbitSize, 2.0f);
        orbitPathList.push_back(path);
    }
}


// Calculate the maximum field of view (from top left corner to bottom right) of
// a frustum with the specified aspect ratio (width/height) and vertical field of
// view. We follow the convention used elsewhere and use units of degrees for
// the field of view angle.
static float calcMaxFOV(float fovY_degrees, float aspectRatio)
{
    float l = 1.0f / std::tan(math::degToRad(fovY_degrees * 0.5f));
    return math::radToDeg(std::atan(std::sqrt(aspectRatio * aspectRatio + 1.0f) / l)) * 2.0f;
}


void Renderer::renderPointStars(const StarDatabase& starDB,
                                float faintestMagNight,
                                const Observer& observer)
{
#ifndef GL_ES
    // Disable multisample rendering when drawing point stars
    bool toggleAA = (starStyle == StarStyle::PointStars && isMSAAEnabled());
    if (toggleAA)
        disableMSAA();
#endif

    Vector3d obsPos = observer.getPosition().toLy();

    PointStarRenderer starRenderer;

    starRenderer.renderer          = this;
    starRenderer.starDB            = &starDB;
    starRenderer.observer          = &observer;
    starRenderer.obsPos            = obsPos;
    starRenderer.viewNormal        = getCameraOrientationf().conjugate() * -Vector3f::UnitZ();
    starRenderer.renderList        = &renderList;
    starRenderer.starVertexBuffer  = pointStarVertexBuffer;
    starRenderer.glareVertexBuffer = glareVertexBuffer;
    starRenderer.cosFOV            = std::cos(math::degToRad(calcMaxFOV(fov, getAspectRatio())) / 2.0f);

    starRenderer.pixelSize         = pixelSize;
    starRenderer.faintestMag       = faintestMag;
    starRenderer.distanceLimit     = distanceLimit;
    starRenderer.labelMode         = labelMode;
    starRenderer.SolarSystemMaxDistance = SolarSystemMaxDistance;

    // = 1.0 at startup
    float effDistanceToScreen = mmToInches((float) REF_DISTANCE_TO_SCREEN) * pixelSize * getScreenDpi();
    starRenderer.labelThresholdMag = 1.2f * max(1.0f, (faintestMag - 4.0f) * (1.0f - 0.5f * std::log10(effDistanceToScreen)));

    starRenderer.colorTemp = &starColors;

    gaussianDiscTex->bind();
    starRenderer.starVertexBuffer->setTexture(gaussianDiscTex);
    starRenderer.starVertexBuffer->setPointScale(screenDpi / 96.0f);
    starRenderer.glareVertexBuffer->setTexture(gaussianGlareTex);
    starRenderer.glareVertexBuffer->setPointScale(screenDpi / 96.0f);

    PointStarVertexBuffer::enable();
    starRenderer.glareVertexBuffer->startSprites();
    if (starStyle == StarStyle::PointStars)
        starRenderer.starVertexBuffer->startBasicPoints();
    else
        starRenderer.starVertexBuffer->startSprites();

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE};
    setPipelineState(ps);

    starDB.findVisibleStars(starRenderer,
                            obsPos.cast<float>(),
                            getCameraOrientationf(),
                            math::degToRad(fov),
                            getAspectRatio(),
                            faintestMagNight);

    starRenderer.starVertexBuffer->finish();
    starRenderer.glareVertexBuffer->finish();
    PointStarVertexBuffer::disable();

#ifndef GL_ES
    if (toggleAA)
        enableMSAA();
#endif
}

void Renderer::renderDeepSkyObjects(const Universe& universe,
                                    const Observer& observer,
                                    const float     faintestMagNight)
{
    DSORenderer dsoRenderer;

    auto cameraOrientation = getCameraOrientationf();

    m_galaxyRenderer->update(cameraOrientation, pixelSize, fov, observer.getZoom());
    dsoRenderer.galaxyRenderer = m_galaxyRenderer.get();

    m_globularRenderer->update(cameraOrientation, pixelSize, fov, observer.getZoom());
    dsoRenderer.globularRenderer = m_globularRenderer.get();

    m_nebulaRenderer->update(cameraOrientation, pixelSize, fov, observer.getZoom());
    dsoRenderer.nebulaRenderer = m_nebulaRenderer.get();

    m_openClusterRenderer->update(cameraOrientation, pixelSize, fov, observer.getZoom());
    dsoRenderer.openClusterRenderer = m_openClusterRenderer.get();

    Vector3d obsPos     = observer.getPosition().toLy();

    DSODatabase* dsoDB  = universe.getDSOCatalog();

    dsoRenderer.renderer         = this;
    dsoRenderer.dsoDB            = dsoDB;
    dsoRenderer.orientationMatrixT = cameraOrientation.toRotationMatrix();
    dsoRenderer.observer         = &observer;
    dsoRenderer.obsPos           = obsPos;
    // size/pixelSize =0.86 at 120deg, 1.43 at 45deg and 1.6 at 0deg.
    dsoRenderer.pixelSize        = pixelSize;
    dsoRenderer.avgAbsMag        = dsoDB->getAverageAbsoluteMagnitude();
    dsoRenderer.faintestMag      = faintestMag;
    dsoRenderer.renderFlags      = renderFlags;
    dsoRenderer.labelMode        = labelMode;

    dsoRenderer.frustum = projectionMode->getInfiniteFrustum(MinNearPlaneDistance, observer.getZoom());
    // Use pixelSize * screenDpi instead of FoV, to eliminate windowHeight dependence.
    // = 1.0 at startup
    float effDistanceToScreen = mmToInches((float) REF_DISTANCE_TO_SCREEN) * pixelSize * getScreenDpi();

    dsoRenderer.labelThresholdMag = 2.0f * max(1.0f, (faintestMag - 4.0f) * (1.0f - 0.5f * log10(effDistanceToScreen)));

    using namespace celestia;
    galaxyRep      = MarkerRepresentation(MarkerRepresentation::Triangle, 8.0f, GalaxyLabelColor);
    nebulaRep      = MarkerRepresentation(MarkerRepresentation::Square,   8.0f, NebulaLabelColor);
    openClusterRep = MarkerRepresentation(MarkerRepresentation::Circle,   8.0f, OpenClusterLabelColor);
    globularRep    = MarkerRepresentation(MarkerRepresentation::Circle,   8.0f, GlobularLabelColor);

    dsoDB->findVisibleDSOs(dsoRenderer,
                           obsPos,
                           cameraOrientation,
                           math::degToRad(fov),
                           getAspectRatio(),
                           2 * faintestMagNight);

    m_galaxyRenderer->render();
    m_globularRenderer->render();
    m_nebulaRenderer->render();
    m_openClusterRenderer->render();

    // clog << "DSOs processed: " << dsoRenderer.dsosProcessed << endl;
}


static Vector3d toStandardCoords(const Vector3d& v)
{
    return Vector3d(v.x(), -v.z(), v.y());
}


void Renderer::renderSkyGrids(const Observer& observer)
{
    using engine::SkyGrid;
    if (util::is_set(renderFlags, RenderFlags::ShowCelestialSphere))
    {
        SkyGrid grid;
        grid.orientation = Quaterniond(AngleAxis<double>(astro::J2000Obliquity, Vector3d::UnitX()));
        grid.lineColor = EquatorialGridColor;
        grid.labelColor = EquatorialGridLabelColor;
        m_skyGridRenderer->render(grid, observer.getZoom());
    }

    if (util::is_set(renderFlags, RenderFlags::ShowGalacticGrid))
    {
        SkyGrid galacticGrid;
        galacticGrid.orientation = (astro::eclipticToEquatorial() * astro::equatorialToGalactic()).conjugate();
        galacticGrid.lineColor = GalacticGridColor;
        galacticGrid.labelColor = GalacticGridLabelColor;
        galacticGrid.longitudeUnits = SkyGrid::LongitudeDegrees;
        m_skyGridRenderer->render(galacticGrid, observer.getZoom());
    }

    if (util::is_set(renderFlags, RenderFlags::ShowEclipticGrid))
    {
        SkyGrid grid;
        grid.orientation = Quaterniond::Identity();
        grid.lineColor = EclipticGridColor;
        grid.labelColor = EclipticGridLabelColor;
        grid.longitudeUnits = SkyGrid::LongitudeDegrees;
        m_skyGridRenderer->render(grid, observer.getZoom());
    }

    if (util::is_set(renderFlags, RenderFlags::ShowHorizonGrid))
    {
        double tdb = observer.getTime();
        auto frame = observer.getFrame();
        const Body* body = frame->getRefObject().body();

        if (body != nullptr)
        {
            SkyGrid grid;
            grid.lineColor = HorizonGridColor;
            grid.labelColor = HorizonGridLabelColor;
            grid.longitudeUnits = SkyGrid::LongitudeDegrees;
            grid.longitudeDirection = SkyGrid::IncreasingClockwise;

            Vector3d zenithDirection = observer.getPosition().offsetFromKm(body->getPosition(tdb)).normalized();

            Vector3d northPole = body->getEclipticToEquatorial(tdb).conjugate() * Vector3d::UnitY();
            zenithDirection = toStandardCoords(zenithDirection);
            northPole = toStandardCoords(northPole);

            Vector3d v = zenithDirection.cross(northPole);

            // Horizontal coordinate system not well defined when observer
            // is at a pole.
            double tolerance = 1.0e-10;
            if (v.norm() > tolerance && v.norm() < 1.0 - tolerance)
            {
                v.normalize();
                Vector3d u = v.cross(zenithDirection);

                Matrix3d m;
                m.row(0) = u;
                m.row(1) = v;
                m.row(2) = zenithDirection;
                grid.orientation = Quaterniond(m);

                m_skyGridRenderer->render(grid, observer.getZoom());
            }
        }
    }

    if (util::is_set(renderFlags, RenderFlags::ShowEcliptic))
        m_eclipticLineRenderer->render();
}

void Renderer::labelConstellations(const AsterismList& asterisms,
                                   const Observer& observer)
{
    Vector3f observerPos = observer.getPosition().toLy().cast<float>();

    for (const auto& ast : asterisms)
    {
        if (!ast.getActive())
            continue;

        // The constellation label is positioned at the average
        // position of all stars in the first chain.  This usually
        // gives reasonable results.

        // Draw all constellation labels at the same distance.
        // Asterism.averagePosition() is normalized
        Eigen::Vector3f rpos = ast.averagePosition() * 1.0e4f - observerPos;

        if ((getCameraOrientationf() * rpos).z() < 0)
        {
            // We'll linearly fade the labels as a function of the
            // observer's distance to the origin of coordinates:
            float opacity = 1.0f;
            if (float dist = observerPos.norm(); dist > MaxAsterismLabelsConstDist)
            {
                opacity = std::clamp((MaxAsterismLabelsConstDist - dist)
                                     / (MaxAsterismLabelsDist - MaxAsterismLabelsConstDist) + 1.0f,
                                     0.0f,
                                     1.0f);
            }

            // Use the default label color unless the constellation has an
            // override color set.
            Color labelColor = ConstellationLabelColor;
            if (ast.isColorOverridden())
                labelColor = ast.getOverrideColor();

            addBackgroundAnnotation(nullptr,
                                    ast.getName(util::is_set(labelMode, RenderLabels::I18nConstellationLabels)),
                                    Color(labelColor, opacity),
                                    rpos,
                                    LabelHorizontalAlignment::Center, LabelVerticalAlignment::Center);
        }
    }
}


void
Renderer::renderAnnotationMarker(const Annotation &a,
                                 TextLayout &layout,
                                 float depth,
                                 const Matrices &m)
{
    const celestia::MarkerRepresentation& markerRep = *a.markerRep;
    float size = a.size > 0.0f ? a.size : markerRep.size();

    glVertexAttrib(CelestiaGLProgram::ColorAttributeIndex, a.color);

    Matrix4f mv = math::translate(*m.modelview, (float)(int)a.position.x(), (float)(int)a.position.y(), depth);
    Matrices mm = { m.projection, &mv };

    if (markerRep.symbol() == celestia::MarkerRepresentation::Crosshair)
        renderCrosshair(size, realTime, a.color, mm);
    else
        markerRep.render(*this, size, mm);

    if (!markerRep.label().empty())
    {
        layout.setHorizontalAlignment(rtl ? TextLayout::HorizontalAlignment::Left : TextLayout::HorizontalAlignment::Right);
        layout.begin(*m.projection, mv);
        float labelOffset = markerRep.size() / 2.0f;
        float x = labelOffset + PixelOffset;
        if (rtl)
            x = -x;
        float y = -labelOffset - static_cast<float>(layout.getLineHeight()) + PixelOffset;
        layout.moveAbsolute(x, y);
        layout.render(markerRep.label());
        layout.end();
    }
}

void
Renderer::renderAnnotationLabel(const Annotation &a,
                                TextLayout &layout,
                                float hOffset,
                                float vOffset,
                                float depth,
                                const Matrices &m)
{
    glVertexAttrib(CelestiaGLProgram::ColorAttributeIndex, a.color);

    Matrix4f mv = math::translate(*m.modelview,
                                     std::trunc(a.position.x()) + hOffset + PixelOffset,
                                     std::trunc(a.position.y()) + vOffset + PixelOffset,
                                     depth);

    layout.begin(*m.projection, mv);
    layout.moveAbsolute(0.0f, 0.0f);
    layout.render(a.labelText);
    layout.end();
}

// stars and constellations. DSOs
void Renderer::renderAnnotations(const vector<Annotation>& annotations,
                                 FontStyle fs)
{
    auto font = getFont(fs);
    if (font == nullptr)
        return;

    TextLayout layout{ screenDpi };
    layout.setFont(font);

    Matrix4f mv = Matrix4f::Identity();
    Matrices m = { &m_orthoProjMatrix, &mv };

    for (const auto &annotation : annotations)
    {
        if (annotation.markerRep != nullptr)
        {
            renderAnnotationMarker(annotation, layout, 0.0f, m);
        }

        if (!annotation.labelText.empty())
        {
            TextLayout::HorizontalAlignment alignment = TextLayout::HorizontalAlignment::Left;
            float hOffset = 0.0f;
            float vOffset = 0.0f;

            getLabelAlignmentInfo(annotation, font.get(), alignment, hOffset, vOffset);

            layout.setHorizontalAlignment(alignment);
            renderAnnotationLabel(annotation, layout, hOffset, vOffset, 0.0f, m);
        }
    }
}


void
Renderer::renderBackgroundAnnotations(FontStyle fs)
{
    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.depthTest = true;
    ps.smoothLines = true;
    setPipelineState(ps);

    renderAnnotations(backgroundAnnotations, fs);
    backgroundAnnotations.clear();
}


void
Renderer::renderForegroundAnnotations(FontStyle fs)
{
    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.depthMask = true;
    ps.smoothLines = true;
    setPipelineState(ps);

    renderAnnotations(foregroundAnnotations, fs);
    foregroundAnnotations.clear();
}


// solar system objects
vector<Renderer::Annotation>::iterator
Renderer::renderSortedAnnotations(vector<Annotation>::iterator iter,
                                  float nearDist,
                                  float farDist,
                                  FontStyle fs)
{
    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.depthMask = true;
    ps.depthTest = true;
    ps.smoothLines = true;
    setPipelineState(ps);

    return renderAnnotations(iter, depthSortedAnnotations.end(), nearDist, farDist, fs);
}


// locations
vector<Renderer::Annotation>::iterator
Renderer::renderAnnotations(vector<Annotation>::iterator startIter,
                            vector<Annotation>::iterator endIter,
                            float nearDist,
                            float farDist,
                            FontStyle fs)
{
    auto font = getFont(fs);
    if (font == nullptr)
        return endIter;

    TextLayout layout{ screenDpi };
    layout.setFont(font);

    Matrix4f mv = Matrix4f::Identity();
    Matrices m = { &m_orthoProjMatrix, &mv };

    // Precompute values that will be used to generate the normalized device z value;
    // we're effectively just handling the projection instead of OpenGL. We use an orthographic
    // projection matrix in order to get the label text position exactly right but need to mimic
    // the depth coordinate generation of a projection.

    vector<Annotation>::iterator iter = startIter;
    for (; iter != endIter && iter->position.z() > nearDist; ++iter)
    {
        // Compute normalized device z
        float z = getProjectionMode()->getNormalizedDeviceZ(nearDist, farDist, iter->position.z());
        float ndc_z = std::clamp(z, -1.0f, 1.0f);

        if (iter->markerRep != nullptr)
        {
            renderAnnotationMarker(*iter, layout, ndc_z, m);
        }

        if (!iter->labelText.empty())
        {
            TextLayout::HorizontalAlignment alignment = TextLayout::HorizontalAlignment::Left;
            float labelHOffset = 0.0f;
            float labelVOffset = 0.0f;

            getLabelAlignmentInfo(*iter, font.get(), alignment, labelHOffset, labelVOffset);

            layout.setHorizontalAlignment(alignment);
            renderAnnotationLabel(*iter, layout, labelHOffset, labelVOffset, ndc_z, m);
        }
    }

    return iter;
}


void Renderer::markersToAnnotations(const celestia::MarkerList& markers,
                                    const Observer& observer,
                                    double jd)
{
    const UniversalCoord& cameraPosition = observer.getPosition();
    const Quaterniond& cameraOrientation = getCameraOrientation();
    Vector3d viewVector = cameraOrientation.conjugate() * -Vector3d::UnitZ();

    for (const auto& marker : markers)
    {
        Vector3d offset = marker.position(jd).offsetFromKm(cameraPosition);

        double distance = offset.norm();
        // Only render those markers that lie withing the field of view.
        if ((offset.dot(viewVector)) > cosViewConeAngle * distance)
        {
            float symbolSize = 0.0f;
            if (marker.sizing() == celestia::DistanceBasedSize)
            {
                symbolSize = (float) (marker.representation().size() / distance) / pixelSize;
            }

            auto *a = &foregroundAnnotations;
            if (marker.occludable())
            {
                // If the marker is occludable, add it to the sorted annotation list if it's relatively
                // nearby, and to the background list if it's very distant.
                if (distance < astro::lightYearsToKilometers(1.0))
                {
                    // Modify the marker position so that it is always in front of the marked object.
                    double boundingRadius;
                    if (marker.object().body() != nullptr)
                        boundingRadius = marker.object().body()->getBoundingRadius();
                    else
                        boundingRadius = marker.object().radius();
                    offset *= (1.0 - boundingRadius * 1.01 / distance);

                    a = &depthSortedAnnotations;
                }
                else
                {
                    a = &backgroundAnnotations;
                }
            }

            addAnnotation(*a, &(marker.representation()), "",
                          marker.representation().color(),
                          offset.cast<float>(),
                          LabelHorizontalAlignment::Start, LabelVerticalAlignment::Top, symbolSize);
        }
    }
}


void Renderer::setStarStyle(StarStyle style)
{
    starStyle = style;
    markSettingsChanged();
}


StarStyle Renderer::getStarStyle() const
{
    return starStyle;
}


void Renderer::loadTextures(Body* body)
{
    Surface& surface = body->getSurface();

    if (surface.baseTexture.texture(textureResolution) != InvalidResource)
        surface.baseTexture.find(textureResolution);
    if ((surface.appearanceFlags & Surface::ApplyBumpMap) != 0 &&
        surface.bumpTexture.texture(textureResolution) != InvalidResource)
        surface.bumpTexture.find(textureResolution);
    if ((surface.appearanceFlags & Surface::ApplyNightMap) != 0 &&
        util::is_set(renderFlags, RenderFlags::ShowNightMaps))
        surface.nightTexture.find(textureResolution);
    if ((surface.appearanceFlags & Surface::SeparateSpecularMap) != 0 &&
        surface.specularTexture.texture(textureResolution) != InvalidResource)
        surface.specularTexture.find(textureResolution);

    const BodyFeaturesManager* bodyFeaturesManager = GetBodyFeaturesManager();
    if (util::is_set(renderFlags, RenderFlags::ShowCloudMaps))
    {
        Atmosphere* atmosphere = bodyFeaturesManager->getAtmosphere(body);
        if (atmosphere != nullptr && atmosphere->cloudTexture.texture(textureResolution) != InvalidResource)
            atmosphere->cloudTexture.find(textureResolution);
    }

    if (auto rings = bodyFeaturesManager->getRings(body);
        rings != nullptr && rings->texture.texture(textureResolution) != InvalidResource)
    {
        rings->texture.find(textureResolution);
    }

    if (body->getGeometry() != InvalidResource)
    {
        Geometry* geometry = engine::GetGeometryManager()->find(body->getGeometry());
        if (geometry != nullptr)
        {
            geometry->loadTextures();
        }
    }
}


void Renderer::invalidateOrbitCache()
{
    orbitCache.clear();
}


bool Renderer::settingsHaveChanged() const
{
    return settingsChanged;
}


void Renderer::markSettingsChanged()
{
    settingsChanged = true;
    notifyWatchers();
}


void Renderer::addWatcher(RendererWatcher* watcher)
{
    assert(watcher != nullptr);
    watchers.push_back(watcher);
}


void Renderer::removeWatcher(RendererWatcher* watcher)
{
    auto iter = find(watchers.begin(), watchers.end(), watcher);
    if (iter != watchers.end())
        watchers.erase(iter);
}


void Renderer::notifyWatchers() const
{
    for (const auto watcher : watchers)
    {
        watcher->notifyRenderSettingsChanged(this);
    }
}

void Renderer::updateBodyVisibilityMask()
{
    // Bodies with type `Invisible' (e.g. ReferencePoints) are not drawn,
    // but if their property `Visible' is set they have visible labels,
    // so we make `Body::Invisible' class visible.
    BodyClassification flags = BodyClassification::Invisible;

    if (util::is_set(renderFlags, RenderFlags::ShowPlanets))
        flags |= BodyClassification::Planet;
    if (util::is_set(renderFlags, RenderFlags::ShowDwarfPlanets))
        flags |= BodyClassification::DwarfPlanet;
    if (util::is_set(renderFlags, RenderFlags::ShowMoons))
        flags |= BodyClassification::Moon;
    if (util::is_set(renderFlags, RenderFlags::ShowMinorMoons))
        flags |= BodyClassification::MinorMoon;
    if (util::is_set(renderFlags, RenderFlags::ShowAsteroids))
        flags |= BodyClassification::Asteroid;
    if (util::is_set(renderFlags, RenderFlags::ShowComets))
        flags |= BodyClassification::Comet;
    if (util::is_set(renderFlags, RenderFlags::ShowSpacecrafts))
        flags |= BodyClassification::Spacecraft;

    bodyVisibilityMask = flags;
}

void Renderer::setSolarSystemMaxDistance(float t)
{
    SolarSystemMaxDistance = std::clamp(t, 1.0f, 10.0f);
}


void Renderer::getViewport(int* x, int* y, int* w, int* h) const
{
    if (x != nullptr)
        *x = m_viewport[0];
    if (y != nullptr)
        *y = m_viewport[1];
    if (w != nullptr)
        *w = m_viewport[2];
    if (h != nullptr)
        *h = m_viewport[3];
}

void Renderer::getViewport(std::array<int, 4>& viewport) const
{
    static_assert(sizeof(int) == sizeof(GLint), "int and GLint size mismatch");
    std::copy(std::begin(m_viewport), std::end(m_viewport), std::begin(viewport));
}

void Renderer::setViewport(int x, int y, int w, int h)
{
    m_viewport = {x, y, w, h};
    glViewport(x, y, w, h);
}

void Renderer::setViewport(const std::array<int, 4>& viewport)
{
    std::copy(std::begin(viewport), std::end(viewport), std::begin(m_viewport));
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

void Renderer::setScissor(int x, int y, int w, int h)
{
    if (!m_pipelineState.scissor)
    {
        glEnable(GL_SCISSOR_TEST);
        m_pipelineState.scissor = true;
    }
    glScissor(x, y, w, h);
}

void Renderer::removeScissor()
{
    if (m_pipelineState.scissor)
    {
        glDisable(GL_SCISSOR_TEST);
        m_pipelineState.scissor = false;
    }
}

void Renderer::enableMSAA() noexcept
{
#ifndef GL_ES
    if (!m_pipelineState.multisample)
    {
        glEnable(GL_MULTISAMPLE);
        m_pipelineState.multisample = true;
    }
#endif
}
void Renderer::disableMSAA() noexcept
{
#ifndef GL_ES
    if (m_pipelineState.multisample)
    {
        glDisable(GL_MULTISAMPLE);
        m_pipelineState.multisample = false;
    }
#endif
}

bool Renderer::isMSAAEnabled() const noexcept
{
    return m_pipelineState.multisample;
}

constexpr GLenum toGLFormat(PixelFormat format)
{
    return (GLenum) format;
}

constexpr int formatWidth(PixelFormat format)
{
    return format == PixelFormat::RGB
#ifndef GL_ES
           || format == PixelFormat::BGR
#endif
           ? 3 : 4;
}

PixelFormat
Renderer::getPreferredCaptureFormat() const noexcept
{
#ifdef GL_ES
    return PixelFormat::RGBA;
#else
    return PixelFormat::RGB;
#endif
}

bool Renderer::captureFrame(int x, int y, int w, int h, PixelFormat format, unsigned char* buffer) const
{
    glReadPixels(x, y, w, h, toGLFormat(format), GL_UNSIGNED_BYTE, (void*) buffer);
    bool ok = glGetError() == GL_NO_ERROR;
    if (!ok)
        return false;

#ifndef GL_ES
    if (!detailOptions.useMesaPackInvert)
#endif
    {
        int realWidth = w * formatWidth(format);
        realWidth = (realWidth + 3) & ~0x3;
#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
        uint8_t tempLine[realWidth]; // G++ supports VLA as an extension
#else
        uint8_t *tempLine = static_cast<uint8_t*>(alloca(realWidth));
#endif
        uint8_t *fb = buffer;
        for (int i = 0, p = realWidth * (h - 1); i < p; i += realWidth, p -= realWidth)
        {
            memcpy(tempLine, &fb[i],   realWidth);
            memcpy(&fb[i],   &fb[p],   realWidth);
            memcpy(&fb[p],   tempLine, realWidth);
        }
    }
    return ok;
}

static void draw_rectangle_border(const Renderer &renderer,
                                  const celestia::Rect &rect,
                                  FisheyeOverrideMode fishEyeOverrideMode,
                                  const Eigen::Matrix4f& p,
                                  const Eigen::Matrix4f& m)
{
    LineRenderer lr(renderer, rect.lw, LineRenderer::PrimType::LineStrip);
    if (fishEyeOverrideMode == FisheyeOverrideMode::Disabled)
        lr.setHints(LineRenderer::DISABLE_FISHEYE_TRANFORMATION);
    lr.startUpdate();
    lr.addVertex(rect.x,          rect.y);
    lr.addVertex(rect.x + rect.w, rect.y);
    lr.addVertex(rect.x + rect.w, rect.y + rect.h);
    lr.addVertex(rect.x,          rect.y + rect.h);
    lr.addVertex(rect.x,          rect.y);
    lr.render({&p, &m}, rect.colors[0], 4);
    lr.finish();
}

static void draw_rectangle_solid(const Renderer &renderer,
                                 const celestia::Rect &r,
                                 FisheyeOverrideMode fishEyeOverrideMode,
                                 const Eigen::Matrix4f& p,
                                 const Eigen::Matrix4f& m)
{
    ShaderProperties shadprop;
    shadprop.lightModel = LightingModel::UnlitModel;
    if (r.hasColors)
        shadprop.texUsage |= TexUsage::VertexColors;
    if (r.tex != nullptr)
        shadprop.texUsage |= TexUsage::DiffuseTexture;

    shadprop.fishEyeOverride = fishEyeOverrideMode;

    auto *prog = renderer.getShaderManager().getShader(shadprop);
    if (prog == nullptr)
        return;

    struct RectVtx
    {
        float x, y, u, v; // NOSONAR
        uint8_t color[4]; // NOSONAR
    };

    array<RectVtx, 4> vertices = {
        RectVtx{ r.x,       r.y,       0.0f, 1.0f, {} },
        RectVtx{ r.x + r.w, r.y,       1.0f, 1.0f, {} },
        RectVtx{ r.x + r.w, r.y + r.h, 1.0f, 0.0f, {} },
        RectVtx{ r.x,       r.y + r.h, 0.0f, 0.0f, {} },
    };

    if (r.hasColors)
    {
        for (int i = 0; i < 4; i++)
            r.colors[i].get(vertices[i].color);
    }

    static GLuint vbo = 0u;
    if (vbo == 0u)
        glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(RectVtx), vertices.data(), GL_STREAM_DRAW);

    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                          2, GL_FLOAT, GL_FALSE, sizeof(RectVtx), reinterpret_cast<void*>(offsetof(RectVtx, x))); //NOSONAR

    if (r.tex != nullptr)
    {
        glEnableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
        glVertexAttribPointer(CelestiaGLProgram::TextureCoord0AttributeIndex,
                              2, GL_FLOAT, GL_FALSE, sizeof(RectVtx), reinterpret_cast<void*>(offsetof(RectVtx, u))); //NOSONAR
        r.tex->bind();
    }
    if (r.hasColors)
    {
        glEnableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
        glVertexAttribPointer(CelestiaGLProgram::ColorAttributeIndex,
                             4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(RectVtx), reinterpret_cast<void*>(offsetof(RectVtx, color))); //NOSONAR
    }

    prog->use();
    prog->setMVPMatrices(p, m);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
    if (r.tex != nullptr)
        glDisableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
    if (r.hasColors)
        glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::drawRectangle(const celestia::Rect &r,
                             FisheyeOverrideMode fishEyeOverrideMode,
                             const Eigen::Matrix4f& p,
                             const Eigen::Matrix4f& m) const
{
    if(r.type == celestia::Rect::Type::BorderOnly)
        draw_rectangle_border(*this, r, fishEyeOverrideMode, p, m);
    else
        draw_rectangle_solid(*this, r, fishEyeOverrideMode, p, m);
}

void Renderer::setRenderRegion(int x, int y, int width, int height, bool withScissor)
{
    if (withScissor)
        setScissor(x, y, width, height);
    else
        removeScissor();

    setViewport(x, y, width, height);
    resize(width, height);
}

float Renderer::getAspectRatio() const
{
    return static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
}

bool Renderer::getInfo(map<string, string>& info) const
{
    info["API"] = "OpenGL";

    const char* s;
    s = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (s != nullptr)
        info["APIVersion"] = s;

    s = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    if (s != nullptr)
        info["Vendor"] = s;

    s = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    if (s != nullptr)
        info["Renderer"] = s;

    s = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    if (s != nullptr)
    {
        info["Language"] = "GLSL";
        info["LanguageVersion"] = s;
    }

    GLint redBits = 0;
    GLint greenBits = 0;
    GLint blueBits = 0;
    GLint alphaBits = 0;
    GLint depthBits = 0;
    glGetIntegerv(GL_RED_BITS, &redBits);
    glGetIntegerv(GL_GREEN_BITS, &greenBits);
    glGetIntegerv(GL_BLUE_BITS, &blueBits);
    glGetIntegerv(GL_ALPHA_BITS, &alphaBits);
    glGetIntegerv(GL_DEPTH_BITS, &depthBits);

    if (alphaBits == 0)
        info["ColorComponent"] = fmt::format("RGB{}{}{}", redBits, greenBits, blueBits);
    else
        info["ColorComponent"] = fmt::format("RGBA{}{}{}{}", redBits, greenBits, blueBits, alphaBits);

    info["DepthComponent"] = to_string(depthBits);

    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    info["MaxTextureSize"] = to_string(maxTextureSize);

    GLint maxTextureUnits = 1;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
    info["MaxTextureUnits"] = to_string(maxTextureUnits);

    GLint pointSizeRange[2];
    GLfloat lineWidthRange[2];
#ifdef GL_ES
    glGetIntegerv(GL_ALIASED_POINT_SIZE_RANGE, pointSizeRange);
    glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
#else
    glGetIntegerv(GL_SMOOTH_POINT_SIZE_RANGE, pointSizeRange);
    glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, lineWidthRange);
#endif
    info["PointSizeMin"] = to_string(pointSizeRange[0]);
    info["PointSizeMax"] = to_string(pointSizeRange[1]);
    info["LineWidthMin"] = to_string(lineWidthRange[0]);
    info["LineWidthMax"] = to_string(lineWidthRange[1]);

#ifndef GL_ES
    GLfloat pointSizeGran = 0;
    glGetFloatv(GL_SMOOTH_POINT_SIZE_GRANULARITY, &pointSizeGran);
    info["PointSizeGran"] = fmt::format("{:.2f}", pointSizeGran);

    GLint maxVaryings = 0;
    glGetIntegerv(GL_MAX_VARYING_FLOATS, &maxVaryings);
    info["MaxVaryingFloats"] = to_string(maxVaryings);
#endif

    if (gl::EXT_texture_filter_anisotropic)
    {
        float maxAnisotropy = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
        info["MaxAnisotropy"] = fmt::format("{:.2f}", maxAnisotropy);
    }

#if 0 // we don't use cubemaps yet
    GLint maxCubeMapSize = 0;
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &maxCubeMapSize);
    info["MaxCubeMapSize"] = to_string(maxCubeMapSize);
#endif

    s = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (s != nullptr)
        info["Extensions"] = s;

    return true;
}

FramebufferObject*
Renderer::getShadowFBO(int index) const
{
    return index == 0 ? m_shadowFBO.get() : nullptr;
}

void
Renderer::createShadowFBO()
{
    m_shadowFBO = std::make_unique<FramebufferObject>(m_shadowMapSize,
                                                      m_shadowMapSize,
                                                      FramebufferObject::DepthAttachment);
    if (!m_shadowFBO->isValid())
    {
        GetLogger()->warn("Error creating shadow FBO.\n");
        m_shadowFBO = nullptr;
    }
}

void
Renderer::setShadowMapSize(unsigned size)
{
    m_shadowMapSize = std::min(size, static_cast<unsigned>(gl::maxTextureSize));
    if (m_shadowFBO != nullptr && m_shadowMapSize == m_shadowFBO->width())
        return;
    if (m_shadowMapSize == 0)
        m_shadowFBO = nullptr;
    else
        createShadowFBO();
}

void
Renderer::removeInvisibleItems(const math::InfiniteFrustum &frustum)
{
    // Remove objects from the render list that lie completely outside the
    // view frustum.
    auto notCulled = renderList.begin();
    const BodyFeaturesManager* bodyFeaturesManager = GetBodyFeaturesManager();
    for (auto &ri : renderList)
    {
        bool convex = true;
        float radius = 1.0f;
        float cullRadius = 1.0f;
        float cloudHeight = 0.0f;

        switch (ri.renderableType)
        {
        case RenderListEntry::RenderableStar:
            radius = ri.star->getRadius();
            cullRadius = radius * (1.0f + CoronaHeight);
            break;

        case RenderListEntry::RenderableCometTail:
        case RenderListEntry::RenderableReferenceMark:
            radius = ri.radius;
            cullRadius = radius;
            convex = false;
            break;

        case RenderListEntry::RenderableBody:
            radius = ri.body->getBoundingRadius();
            if (const RingSystem* rings = bodyFeaturesManager->getRings(ri.body); rings != nullptr)
            {
                radius = rings->outerRadius;
                convex = false;
            }

            if (!ri.body->isEllipsoid())
                convex = false;

            cullRadius = radius;
            if (const Atmosphere* atmosphere = bodyFeaturesManager->getAtmosphere(ri.body); atmosphere != nullptr)
            {
                cullRadius += atmosphere->height;
                cloudHeight = max(atmosphere->cloudHeight,
                                  atmosphere->mieScaleHeight * -log(AtmosphereExtinctionThreshold));
            }
            break;

        default:
            break;
        }

        Vector3f center = getCameraOrientationf().toRotationMatrix() * ri.position;
        // Test the object's bounding sphere against the view frustum
        if (frustum.testSphere(center, cullRadius) != math::FrustumAspect::Outside)
        {
            float nearZ = center.norm() - radius;
            float maxSpan = hypot((float) windowWidth, (float) windowHeight);
            float nearZcoeff = cos(math::degToRad(fov / 2.0f)) * ((float) windowHeight / maxSpan);
            nearZ = -nearZ * nearZcoeff;

            if (nearZ > -MinNearPlaneDistance)
                ri.nearZ = -max(MinNearPlaneDistance, radius / 2000.0f);
            else
                ri.nearZ = nearZ;

            if (!convex)
            {
                ri.farZ = center.z() - radius;
                if (ri.farZ / ri.nearZ > MaxFarNearRatio * 0.5f)
                    ri.nearZ = ri.farZ / (MaxFarNearRatio * 0.5f);
            }
            else
            {
                // Make the far plane as close as possible
                float d = center.norm();

                // Account for ellipsoidal objects
                float eradius = radius;
                if (ri.renderableType == RenderListEntry::RenderableBody)
                {
                    float minSemiAxis = ri.body->getSemiAxes().minCoeff();
                    eradius *= minSemiAxis / radius;
                }

                if (d > eradius)
                {
                    ri.farZ = ri.centerZ - ri.radius;
                }
                else
                {
                    // We're inside the bounding sphere (and, if the planet
                    // is spherical, inside the planet.)
                    ri.farZ = ri.nearZ * 2.0f;
                }

                if (cloudHeight > 0.0f)
                {
                    // If there's a cloud layer, we need to move the
                    // far plane out so that the clouds aren't clipped
                    float cloudLayerRadius = eradius + cloudHeight;
                    ri.farZ -= sqrt(math::square(cloudLayerRadius) - math::square(eradius));
                }
            }

            *notCulled = ri;
            notCulled++;
        }
    }

    renderList.resize(notCulled - renderList.begin());

    // The calls to buildRenderLists/renderStars filled renderList
    // with visible bodies.  Sort it front to back, then
    // render each entry in reverse order (TODO: convenient, but not
    // ideal for performance; should render opaque objects front to
    // back, then translucent objects back to front. However, the
    // amount of overdraw in Celestia is typically low.)
    sort(renderList.begin(), renderList.end());
}

bool
Renderer::selectionToAnnotation(const Selection &sel,
                                const Observer &observer,
                                const math::InfiniteFrustum &xfrustum,
                                double jd)
{
    Vector3d offset = sel.getPosition(jd).offsetFromKm(observer.getPosition());

    static celestia::MarkerRepresentation cursorRep(celestia::MarkerRepresentation::Crosshair);
    if (xfrustum.testSphere(offset, sel.radius()) == math::FrustumAspect::Outside)
        return false;

    double distance = offset.norm();
    float symbolSize = (float)(sel.radius() / distance) / pixelSize;

    // Modify the marker position so that it is always in front of the marked object.
    double boundingRadius = sel.body() != nullptr ? sel.body()->getBoundingRadius() : sel.radius();
    offset *= (1.0 - boundingRadius * 1.01 / distance);

    // The selection cursor is only partially visible when the selected object is obscured. To implement
    // this behavior we'll draw two markers at the same position: one that's always visible, and another one
    // that's depth sorted. When the selection is occluded, only the foreground marker is visible. Otherwise,
    // both markers are drawn and cursor appears much brighter as a result.
    if (distance < astro::lightYearsToKilometers(1.0))
    {
        addSortedAnnotation(&cursorRep, "", SelectionCursorColor,
                            offset.cast<float>(),
                            LabelHorizontalAlignment::Start, LabelVerticalAlignment::Top, symbolSize);
    }
    else
    {
        addBackgroundAnnotation(&cursorRep, "", SelectionCursorColor,
                                offset.cast<float>(),
                                LabelHorizontalAlignment::Start, LabelVerticalAlignment::Top, symbolSize);
    }

    Color occludedCursorColor(SelectionCursorColor.red(),
                              SelectionCursorColor.green() + 0.3f,
                              SelectionCursorColor.blue(),
                              0.4f);
    addForegroundAnnotation(&cursorRep, "", occludedCursorColor,
                            offset.cast<float>(),
                            LabelHorizontalAlignment::Start, LabelVerticalAlignment::Top, symbolSize);
    return true;
}

void
Renderer::adjustMagnitudeInsideAtmosphere(float &faintestMag,
                                          float &saturationMag,
                                          double now)
{
    const BodyFeaturesManager* bodyFeaturesManager = GetBodyFeaturesManager();
    for (const auto& ri : renderList)
    {
        if (ri.renderableType != RenderListEntry::RenderableBody)
            continue;

        // Compute the density of the atmosphere, and from that
        // the amount light scattering.  It's complicated by the
        // possibility that the planet is oblate and a simple distance
        // to sphere calculation will not suffice.
        const Atmosphere* atmosphere = bodyFeaturesManager->getAtmosphere(ri.body);
        if (atmosphere == nullptr || atmosphere->height <= 0.0f)
            continue;

        float radius = ri.body->getRadius();
        Vector3f semiAxes = ri.body->getSemiAxes() / radius;

        Vector3f recipSemiAxes = semiAxes.cwiseInverse();
        Vector3f eyeVec = ri.position / radius;

        // Compute the orientation of the planet before axial rotation
        Quaternionf q = ri.body->getEclipticToEquatorial(now).cast<float>();
        eyeVec = q * eyeVec;

        // ellipDist is not the true distance from the surface unless
        // the planet is spherical.  The quantity that we do compute
        // is the distance to the surface along a line from the eye
        // position to the center of the ellipsoid.
        float ellipDist = eyeVec.cwiseProduct(recipSemiAxes).norm() - 1.0f;
        if (ellipDist >= atmosphere->height / radius)
            continue;

        float density = std::min(1.0f, 1.0f - ellipDist / (atmosphere->height / radius));

        Vector3f sunDir = ri.sun.normalized();
        Vector3f normal = -ri.position.normalized();
        float illumination = std::clamp(sunDir.dot(normal) + 0.2f, 0.0f, 1.0f);

        float lightness = illumination * density;
        faintestMag = faintestMag - 15.0f * lightness;
        saturationMag = saturationMag - 15.0f * lightness;
    }
}

void
Renderer::buildNearSystemsLists(const Universe &universe,
                                const Observer &observer,
                                const math::InfiniteFrustum &xfrustum,
                                double now)
{
    UniversalCoord observerPos = observer.getPosition();
    Eigen::Quaterniond observerOrient = getCameraOrientation();

    universe.getNearStars(observerPos, SolarSystemMaxDistance, nearStars);

    // Set up direct light sources (i.e. just stars at the moment)
    // Skip if only star orbits to be shown
    if (util::is_set(renderFlags, RenderFlags::ShowSolarSystemObjects))
        setupLightSources(nearStars,
                          observerPos,
                          now,
                          lightSourceList,
                          tintSaturation,
                          starColors.type() == ColorTableType::Enhanced ? nullptr : &tintColors);

    // Traverse the frame trees of each nearby solar system and
    // build the list of objects to be rendered.
    for (const auto sun : nearStars)
    {
        addStarOrbitToRenderList(*sun, observer, now);
        // Skip if only star orbits to be shown
        if (!util::is_set(renderFlags, RenderFlags::ShowSolarSystemObjects))
            continue;

        const SolarSystem* solarSystem = universe.getSolarSystem(sun);
        if (solarSystem == nullptr)
            continue;

        FrameTree* solarSysTree = solarSystem->getFrameTree();
        if (solarSysTree == nullptr)
            continue;

        if (solarSysTree->updateRequired())
        {
            // Tree has changed, so we must recompute bounding spheres.
            solarSysTree->recomputeBoundingSphere();
            solarSysTree->markUpdated();
        }

        // Compute the position of the observer in astrocentric coordinates
        Vector3d astrocentricObserverPos = astrocentricPosition(observerPos, *sun, now);

        // Build render lists for bodies and orbits paths
        buildRenderLists(astrocentricObserverPos, xfrustum,
                         observerOrient.conjugate() * -Vector3d::UnitZ(),
                         Vector3d::Zero(), solarSysTree, observer, now);
        if (util::is_set(renderFlags, RenderFlags::ShowOrbits))
        {
            buildOrbitLists(astrocentricObserverPos, observerOrient,
                            xfrustum, solarSysTree, now);
        }
    }

    if (util::is_set(labelMode, RenderLabels::BodyLabelMask))
        buildLabelLists(xfrustum, now);
}

int
Renderer::buildDepthPartitions()
{
    // Since we're rendering objects of a huge range of sizes spread over
    // vast distances, we can't just rely on the hardware depth buffer to
    // handle hidden surface removal without a little help. We'll partition
    // the depth buffer into spans that can be rendered without running
    // into terrible depth buffer precision problems. Typically, each body
    // with an apparent size greater than one pixel is allocated its own
    // depth buffer interval. However, this will not correctly handle
    // overlapping objects.  If two objects overlap in depth, we must
    // assign them to the same interval.

    depthPartitions.clear();
    int nIntervals = 0;
    int nEntries = (int)renderList.size();
    float prevNear = -1e12f; // ~ 1 light year
    if (nEntries > 0)
        prevNear = renderList[nEntries - 1].farZ * 1.01f;

    int i;

    // Completely partition the depth buffer. Scan from back to front
    // through all the renderable items that passed the culling test.
    for (i = nEntries - 1; i >= 0; i--)
    {
        // Only consider renderables that will occupy more than one pixel.
        if (renderList[i].discSizeInPixels > 1)
        {
            if (nIntervals == 0 ||
                renderList[i].farZ >= depthPartitions[nIntervals - 1].nearZ)
            {
                // This object spans a depth interval that's disjoint with
                // the current interval, so create a new one for it, and
                // another interval to fill the gap between the last
                // interval.
                DepthBufferPartition partition;
                partition.index = nIntervals;
                partition.nearZ = renderList[i].farZ;
                partition.farZ = prevNear;

                // Omit null intervals
                // TODO: Is this necessary? Shouldn't the >= test prevent this?
                if (partition.nearZ != partition.farZ)
                {
                    depthPartitions.push_back(partition);
                    nIntervals++;
                }

                partition.index = nIntervals;
                partition.nearZ = renderList[i].nearZ;
                partition.farZ = renderList[i].farZ;
                depthPartitions.push_back(partition);
                nIntervals++;

                prevNear = partition.nearZ;
            }
            else
            {
                // This object overlaps the current span; expand the
                // interval so that it completely contains the object.
                DepthBufferPartition& partition = depthPartitions[nIntervals - 1];
                partition.nearZ = max(partition.nearZ, renderList[i].nearZ);
                partition.farZ = min(partition.farZ, renderList[i].farZ);
                prevNear = partition.nearZ;
            }
        }
    }

    // Scan the list of orbit paths and find the closest one. We'll need
    // adjust the nearest interval to accommodate it.
    float zNearest = prevNear;
    for (i = 0; i < (int)orbitPathList.size(); i++)
    {
        const OrbitPathListEntry& o = orbitPathList[i];
        float minNearDistance = min(-MinNearPlaneDistance, o.centerZ + o.radius);
        if (minNearDistance > zNearest)
            zNearest = minNearDistance;
    }

    // Adjust the nearest interval to include the closest marker (if it's
    // closer to the observer than anything else
    if (!depthSortedAnnotations.empty())
    {
        // Factor of 0.999 makes sure ensures that the near plane does not fall
        // exactly at the marker's z coordinate (in which case the marker
        // would be susceptible to getting clipped.)
        if (-depthSortedAnnotations[0].position.z() > zNearest)
            zNearest = -depthSortedAnnotations[0].position.z() * 0.999f;
    }

#if DEBUG_COALESCE
    clog << "nEntries: " << nEntries << ",   zNearest: " << zNearest
         << ",   prevNear: " << prevNear << "\n";
#endif

    // If the nearest distance wasn't set, nothing should appear
    // in the frontmost depth buffer interval (so we can set the near plane
    // of the front interval to whatever we want as long as it's less than
    // the far plane distance.
    if (zNearest == prevNear)
        zNearest = 0.0f;

    // Add one last interval for the span from 0 to the front of the
    // nearest object
    // TODO: closest object may not be at entry 0, since objects are
    // sorted by far distance.
    float closest = zNearest;
    if (nEntries > 0)
    {
        closest = max(closest, renderList[0].nearZ);

        // Setting a the near plane distance to zero results in unreliable rendering, even
        // if we don't care about the depth buffer. Compromise and set the near plane
        // distance to a small fraction of distance to the nearest object.
        if (closest == 0.0f)
        {
            closest = renderList[0].nearZ * 0.01f;
        }
    }

    DepthBufferPartition partition;
    partition.index = nIntervals;
    partition.nearZ = closest;
    partition.farZ = prevNear;
    depthPartitions.push_back(partition);

    nIntervals++;

    // If orbits are enabled, adjust the farthest partition so that it
    // can contain the orbit.
    if (!orbitPathList.empty())
    {
        depthPartitions[0].farZ = min(depthPartitions[0].farZ,
                                      orbitPathList.back().centerZ - orbitPathList.back().radius);
    }

    // We want to avoid overpartitioning the depth buffer. In this stage, we
    // coalesce partitions that have small spans in the depth buffer.
    // TODO: Implement this step!
    return nIntervals;
}

void
Renderer::renderSolarSystemObjects(const Observer &observer,
                                   int nIntervals,
                                   double now)
{
    // Render everything that wasn't culled.
    auto annotation = depthSortedAnnotations.begin();
    float intervalSize = 1.0f / static_cast<float>(max(1, nIntervals));
    int i = static_cast<int>(renderList.size()) - 1;
    for (int interval = 0; interval < nIntervals; interval++)
    {
        currentIntervalIndex = interval;
        beginObjectAnnotations();

        const float nearPlaneDistance = -depthPartitions[interval].nearZ;
        const float farPlaneDistance = -depthPartitions[interval].farZ;

        // Set the depth range for this interval--each interval is allocated an
        // equal section of the depth buffer.
        glDepthRange(1.0f - (interval + 1) * intervalSize,
                     1.0f - interval * intervalSize);

        // Set up a perspective projection using the current interval's near and
        // far clip planes.
        Matrix4f proj;
        buildProjectionMatrix(proj, nearPlaneDistance, farPlaneDistance, observer.getZoom());
        Matrices m = { &proj, &m_modelMatrix };

        setCurrentProjectionMatrix(proj);

        int firstInInterval = i;

        // Render just the opaque objects in the first pass
        while (i >= 0 && renderList[i].farZ < depthPartitions[interval].nearZ)
        {
            // This interval should completely contain the item
            // Unless it's just a point?
            // assert(renderList[i].nearZ <= depthPartitions[interval].near);

            // Treat objects that are smaller than one pixel as transparent and
            // render them in the second pass.
            if (renderList[i].isOpaque && renderList[i].discSizeInPixels > 1.0f)
                renderItem(renderList[i], observer, nearPlaneDistance, farPlaneDistance, m);

            i--;
        }

        // Render orbit paths
        if (!orbitPathList.empty())
        {
            math::Frustum intervalFrustum = projectionMode->getFrustum(nearPlaneDistance, farPlaneDistance, observer.getZoom());

            // Scan through the list of orbits and render any that overlap this interval
            for (const auto& orbit : orbitPathList)
            {
                // Test for overlap
                float nearZ = -orbit.centerZ - orbit.radius;
                float farZ = -orbit.centerZ + orbit.radius;

                // Don't render orbits when they're completely outside this
                // depth interval.
                if (nearZ < farPlaneDistance && farZ > nearPlaneDistance)
                {
                    renderOrbit(orbit, now,
                                getCameraOrientation(),
                                intervalFrustum,
                                nearPlaneDistance,
                                farPlaneDistance);
                }
            }
        }

        // Render transparent objects in the second pass
        i = firstInInterval;
        while (i >= 0 && renderList[i].farZ < depthPartitions[interval].nearZ)
        {
            if (!renderList[i].isOpaque || renderList[i].discSizeInPixels <= 1.0f)
                renderItem(renderList[i], observer, nearPlaneDistance, farPlaneDistance, m);

            i--;
        }

        Renderer::PipelineState ps;
        ps.blending = true;
        ps.blendFunc = {GL_SRC_ALPHA, GL_ONE};
        ps.depthTest = true;
        setPipelineState(ps);

        PointStarVertexBuffer::enable();
        glareVertexBuffer->startSprites();
        glareVertexBuffer->render();
        glareVertexBuffer->finish();
        if (starStyle == StarStyle::PointStars)
            pointStarVertexBuffer->startBasicPoints();
        else
            pointStarVertexBuffer->startSprites();
        pointStarVertexBuffer->render();
        pointStarVertexBuffer->finish();
        PointStarVertexBuffer::disable();

        // Render annotations in this interval
        annotation = renderSortedAnnotations(annotation,
                                             nearPlaneDistance,
                                             farPlaneDistance,
                                             FontNormal);
        endObjectAnnotations();
    }

    // reset the depth range
    glDepthRange(0, 1);
    setDefaultProjectionMatrix();
}

void
Renderer::setPipelineState(const Renderer::PipelineState &ps) noexcept
{
    if (ps.blending != m_pipelineState.blending)
    {
        if (ps.blending)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
        m_pipelineState.blending = ps.blending;
    }
    if (ps.blending && (ps.blendFunc.src != m_pipelineState.blendFunc.src || ps.blendFunc.dst != m_pipelineState.blendFunc.dst))
    {
        glBlendFuncSeparate(ps.blendFunc.src, ps.blendFunc.dst, GL_ZERO, GL_ONE);
        m_pipelineState.blendFunc = ps.blendFunc;
    }
    if (ps.depthTest != m_pipelineState.depthTest)
    {
        if (ps.depthTest)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        m_pipelineState.depthTest = ps.depthTest;
    }
    if (ps.depthMask != m_pipelineState.depthMask)
    {
        glDepthMask(ps.depthMask ? GL_TRUE : GL_FALSE);
        m_pipelineState.depthMask = ps.depthMask;
    }
    if (ps.smoothLines != m_pipelineState.smoothLines)
    {
#ifndef GL_ES
        if (ps.smoothLines && util::is_set(renderFlags, RenderFlags::ShowSmoothLines))
            glEnable(GL_LINE_SMOOTH);
        else
            glDisable(GL_LINE_SMOOTH);
#endif
        m_pipelineState.smoothLines = ps.smoothLines;
    }
}

void Renderer::buildProjectionMatrix(Eigen::Matrix4f &mat, float nearZ, float farZ, float zoom) const
{
    mat = projectionMode->getProjectionMatrix(nearZ, farZ, zoom);
}
