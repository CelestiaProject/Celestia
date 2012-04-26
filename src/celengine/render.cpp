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

//#define DEBUG_HDR
#ifdef DEBUG_HDR
//#define DEBUG_HDR_FILE
//#define DEBUG_HDR_ADAPT
//#define DEBUG_HDR_TONEMAP
#endif
#ifdef DEBUG_HDR_FILE
#include <fstream>
std::ofstream hdrlog;
#define HDR_LOG     hdrlog
#else
#define HDR_LOG     cout
#endif

#ifdef USE_HDR
#define BLUR_PASS_COUNT     2
#define BLUR_SIZE           128
#define DEFAULT_EXPOSURE    -23.35f
#define EXPOSURE_HALFLIFE   0.4f
//#define USE_BLOOM_LISTS
#endif

// #define ENABLE_SELF_SHADOW

#ifndef _WIN32
#ifndef TARGET_OS_MAC
#include <config.h>
#endif
#endif /* _WIN32 */

#include "render.h"
#include "boundaries.h"
#include "asterism.h"
#include "astro.h"
#include "vecgl.h"
#include "glshader.h"
#include "shadermanager.h"
#include "spheremesh.h"
#include "lodspheremesh.h"
#include "geometry.h"
#include "regcombine.h"
#include "vertexprog.h"
#include "texmanager.h"
#include "meshmanager.h"
#include "renderinfo.h"
#include "renderglsl.h"
#include "axisarrow.h"
#include "frametree.h"
#include "timelinephase.h"
#include "skygrid.h"
#include "modelgeometry.h"
#include <celutil/debug.h>
#include <celmath/frustum.h>
#include <celmath/distance.h>
#include <celmath/intersect.h>
#include <celmath/geomutil.h>
#include <celutil/utf8.h>
#include <celutil/util.h>
#include <celutil/timer.h>
#include <curveplot.h>
#include <GL/glew.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <sstream>
#include <iomanip>
#include "eigenport.h"

using namespace cmod;
using namespace Eigen;
using namespace std;

#define FOV           45.0f
#define NEAR_DIST      0.5f
#define FAR_DIST       1.0e9f

// This should be in the GL headers, but where?
#ifndef GL_COLOR_SUM_EXT
#define GL_COLOR_SUM_EXT 0x8458
#endif

static const float  STAR_DISTANCE_LIMIT  = 1.0e6f;
static const int REF_DISTANCE_TO_SCREEN  = 400; //[mm]

// Contribution from planetshine beyond this distance (in units of object radius)
// is considered insignificant.
static const float PLANETSHINE_DISTANCE_LIMIT_FACTOR = 100.0f;

// Planetshine from objects less than this pixel size is treated as insignificant
// and will be ignored.
static const float PLANETSHINE_PIXEL_SIZE_LIMIT      =   0.1f;

// Distance from the Sun at which comet tails will start to fade out
static const float COMET_TAIL_ATTEN_DIST_SOL = astro::AUtoKilometers(5.0f);

static const int StarVertexListSize = 1024;

// Fractional pixel offset used when rendering text as texture mapped
// quads to ensure consistent mapping of texels to pixels.
static const float PixelOffset = 0.125f;

// These two values constrain the near and far planes of the view frustum
// when rendering planet and object meshes.  The near plane will never be
// closer than MinNearPlaneDistance, and the far plane is set so that far/near
// will not exceed MaxFarNearRatio.
static const float MinNearPlaneDistance = 0.0001f; // km
static const float MaxFarNearRatio      = 2000000.0f;

static const float RenderDistance       = 50.0f;

// Star disc size in pixels
static const float BaseStarDiscSize      = 5.0f;
static const float MaxScaledDiscStarSize = 8.0f;
static const float GlareOpacity = 0.65f;

static const float MinRelativeOccluderRadius = 0.005f;

static const float CubeCornerToCenterDistance = (float) sqrt(3.0);


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

// Maximum size of a solar system in light years. Features beyond this distance
// will not necessarily be rendered correctly. This limit is used for
// visibility culling of solar systems.
static const float MaxSolarSystemSize = 1.0f;


// Static meshes and textures used by all instances of Simulation

static bool commonDataInitialized = false;

static const string EMPTY_STRING("");


LODSphereMesh* g_lodSphere = NULL;

static Texture* normalizationTex = NULL;

static Texture* starTex = NULL;
static Texture* glareTex = NULL;
static Texture* shadowTex = NULL;
static Texture* gaussianDiscTex = NULL;
static Texture* gaussianGlareTex = NULL;

// Shadow textures are scaled down slightly to leave some extra blank pixels
// near the border.  This keeps axis aligned streaks from appearing on hardware
// that doesn't support clamp to border color.
static const float ShadowTextureScale = 15.0f / 16.0f;

static Texture* eclipseShadowTextures[4];
static Texture* shadowMaskTexture = NULL;
static Texture* penumbraFunctionTexture = NULL;

Texture* rectToSphericalTexture = NULL;

static const Color compassColor(0.4f, 0.4f, 1.0f);

static const float CoronaHeight = 0.2f;

static bool buggyVertexProgramEmulation = true;

static const int MaxSkyRings = 32;
static const int MaxSkySlices = 180;
static const int MinSkySlices = 30;

// Size at which the orbit cache will be flushed of old orbit paths
static const unsigned int OrbitCacheCullThreshold = 200;
// Age in frames at which unused orbit paths may be eliminated from the cache
static const uint32 OrbitCacheRetireAge = 16;

Color Renderer::StarLabelColor          (0.471f, 0.356f, 0.682f);
Color Renderer::PlanetLabelColor        (0.407f, 0.333f, 0.964f);
Color Renderer::DwarfPlanetLabelColor   (0.407f, 0.333f, 0.964f);
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
Color Renderer::DwarfPlanetOrbitColor   (0.3f,   0.323f, 0.833f);
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


#ifdef ENABLE_SELF_SHADOW
static FramebufferObject* shadowFbo = NULL;
#endif


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


// Calculate the cosine of half the maximum field of view. We'll use this for
// fast testing of object visibility.  The function takes the vertical FOV (in
// degrees) as an argument. When computing the view cone, we want the field of
// view as measured on the diagonal between viewport corners.
double computeCosViewConeAngle(double verticalFOV, double width, double height)
{
    double h = tan(degToRad(verticalFOV / 2));
    double diag = sqrt(1.0 + square(h) + square(h * width / height));
    return 1.0 / diag;
}


/**** Star vertex buffer classes ****/

class StarVertexBuffer
{
public:
    StarVertexBuffer(unsigned int _capacity);
    ~StarVertexBuffer();
    void start();
    void render();
    void finish();
    void addStar(const Eigen::Vector3f&, const Color&, float);
    void setBillboardOrientation(const Eigen::Quaternionf&);

private:
    unsigned int capacity;
    unsigned int nStars;
    Eigen::Vector3f* vertices;
    float* texCoords;
    unsigned char* colors;
    Eigen::Vector3f v0, v1, v2, v3;
};


// PointStarVertexBuffer is used instead of StarVertexBuffer when the
// hardware supports point sprites.
class PointStarVertexBuffer
{
public:
    PointStarVertexBuffer(unsigned int _capacity);
    ~PointStarVertexBuffer();
    void startPoints(const GLContext&);
    void startSprites(const GLContext&);
    void render();
    void finish();
    inline void addStar(const Eigen::Vector3f& pos, const Color&, float);
	void setTexture(Texture*);

private:
    struct StarVertex
    {
        Eigen::Vector3f position;
        float size;
        unsigned char color[4];
        float pad;
    };

    unsigned int capacity;
    unsigned int nStars;
    StarVertex* vertices;
    const GLContext* context;
    bool useSprites;
	Texture* texture;
};


StarVertexBuffer::StarVertexBuffer(unsigned int _capacity) :
    capacity(_capacity),
    vertices(NULL),
    texCoords(NULL),
    colors(NULL)
{
    nStars = 0;
    vertices = new Vector3f[capacity * 4];
    texCoords = new float[capacity * 8];
    colors = new unsigned char[capacity * 16];

    // Fill the texture coordinate array now, since it will always have
    // the same contents.
    for (unsigned int i = 0; i < capacity; i++)
    {
        unsigned int n = i * 8;
        texCoords[n    ] = 0; texCoords[n + 1] = 0;
        texCoords[n + 2] = 1; texCoords[n + 3] = 0;
        texCoords[n + 4] = 1; texCoords[n + 5] = 1;
        texCoords[n + 6] = 0; texCoords[n + 7] = 1;
    }
}

StarVertexBuffer::~StarVertexBuffer()
{
    if (vertices != NULL)
        delete[] vertices;
    if (colors != NULL)
        delete[] colors;
    if (texCoords != NULL)
        delete[] texCoords;
}

void StarVertexBuffer::start()
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
    glDisableClientState(GL_NORMAL_ARRAY);
}

void StarVertexBuffer::render()
{
    if (nStars != 0)
    {
        glDrawArrays(GL_QUADS, 0, nStars * 4);
        nStars = 0;
    }
}

void StarVertexBuffer::finish()
{
    render();
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void StarVertexBuffer::addStar(const Vector3f& pos,
                               const Color& color,
                               float size)
{
    if (nStars < capacity)
    {
        int n = nStars * 4;
        vertices[n + 0]  = pos + v0 * size;
        vertices[n + 1]  = pos + v1 * size;
        vertices[n + 2]  = pos + v2 * size;
        vertices[n + 3] = pos + v3 * size;

        n = nStars * 16;
        color.get(colors + n);
        color.get(colors + n + 4);
        color.get(colors + n + 8);
        color.get(colors + n + 12);

        nStars++;
    }

    if (nStars == capacity)
    {
        render();
        nStars = 0;
    }
}

void StarVertexBuffer::setBillboardOrientation(const Quaternionf& q)
{
    Matrix3f m = q.conjugate().toRotationMatrix();
    v0 = m * Vector3f(-1, -1, 0);
    v1 = m * Vector3f( 1, -1, 0);
    v2 = m * Vector3f( 1,  1, 0);
    v3 = m * Vector3f(-1,  1, 0);
}


PointStarVertexBuffer::PointStarVertexBuffer(unsigned int _capacity) :
    capacity(_capacity),
    nStars(0),
    vertices(NULL),
    context(NULL),
    useSprites(false),
	texture(NULL)
{
    vertices = new StarVertex[capacity];
}

PointStarVertexBuffer::~PointStarVertexBuffer()
{
    if (vertices != NULL)
        delete[] vertices;
}

void PointStarVertexBuffer::startSprites(const GLContext& _context)
{
    context = &_context;
    assert(context->getVertexProcessor() != NULL || !useSprites); // vertex shaders required for new star rendering

    unsigned int stride = sizeof(StarVertex);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, stride, &vertices[0].position);
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, stride, &vertices[0].color);

    VertexProcessor* vproc = context->getVertexProcessor();
    vproc->enable();
    vproc->use(vp::starDisc);
    vproc->enableAttribArray(6);
    vproc->attribArray(6, 1, GL_FLOAT, stride, &vertices[0].size);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);

    glEnable(GL_POINT_SPRITE_ARB);
    glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);

    useSprites = true;
}

void PointStarVertexBuffer::startPoints(const GLContext& _context)
{
    context = &_context;

    unsigned int stride = sizeof(StarVertex);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, stride, &vertices[0].position);
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, stride, &vertices[0].color);

    // An option to control the size of the stars would be helpful.
    // Which size looks best depends a lot on the resolution and the
    // type of display device.
    // glPointSize(2.0f);
    // glEnable(GL_POINT_SMOOTH);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_2D);

    glDisableClientState(GL_NORMAL_ARRAY);

    useSprites = false;
}

void PointStarVertexBuffer::render()
{
    if (nStars != 0)
    {
        unsigned int stride = sizeof(StarVertex);
        if (useSprites)
        {
            glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
            glEnable(GL_TEXTURE_2D);
        }
        else
        {
            glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
            glDisable(GL_TEXTURE_2D);
            glPointSize(1.0f);
        }
        glVertexPointer(3, GL_FLOAT, stride, &vertices[0].position);
        glColorPointer(4, GL_UNSIGNED_BYTE, stride, &vertices[0].color);

        if (useSprites)
        {
            VertexProcessor* vproc = context->getVertexProcessor();
            vproc->attribArray(6, 1, GL_FLOAT, stride, &vertices[0].size);
        }

        if (texture != NULL)
            texture->bind();
        glDrawArrays(GL_POINTS, 0, nStars);
        nStars = 0;
    }
}

void PointStarVertexBuffer::finish()
{
    render();
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    if (useSprites)
    {
        VertexProcessor* vproc = context->getVertexProcessor();
        vproc->disableAttribArray(6);
        vproc->disable();

        glDisable(GL_POINT_SPRITE_ARB);
    }
    else
    {
        glEnable(GL_TEXTURE_2D);
    }
}

inline void PointStarVertexBuffer::addStar(const Eigen::Vector3f& pos,
                                           const Color& color,
                                           float size)
{
    if (nStars < capacity)
    {
        vertices[nStars].position = pos;
        vertices[nStars].size = size;
        color.get(vertices[nStars].color);
        nStars++;
    }

    if (nStars == capacity)
    {
        render();
        nStars = 0;
    }
}

void PointStarVertexBuffer::setTexture(Texture* _texture)
{
	texture = _texture;
}

/**** End star vertex buffer classes ****/


Renderer::Renderer() :
    context(0),
    windowWidth(0),
    windowHeight(0),
    fov(FOV),
    cosViewConeAngle(computeCosViewConeAngle(fov, 1, 1)),
    screenDpi(96),
    corrFac(1.12f),
    faintestAutoMag45deg(7.0f),
    renderMode(GL_FILL),
    labelMode(NoLabels),
    renderFlags(ShowStars | ShowPlanets),
    orbitMask(Body::Planet | Body::Moon | Body::Stellar),
    ambientLightLevel(0.1f),
    fragmentShaderEnabled(false),
    vertexShaderEnabled(false),
    brightnessBias(0.0f),
    saturationMagNight(1.0f),
    saturationMag(1.0f),
    starStyle(FuzzyPointStars),
    starVertexBuffer(NULL),
    pointStarVertexBuffer(NULL),
    glareVertexBuffer(NULL),
    useVertexPrograms(false),
    useRescaleNormal(false),
    usePointSprite(false),
    textureResolution(medres),
    useNewStarRendering(false),
    frameCount(0),
    lastOrbitCacheFlush(0),
    minOrbitSize(MinOrbitSizeForLabel),
    distanceLimit(1.0e6f),
    minFeatureSize(MinFeatureSizeForLabel),
    locationFilter(~0u),
    colorTemp(NULL),
#ifdef USE_HDR
    sceneTexture(0),
    blurFormat(GL_RGBA),
    useBlendSubtract(true),
    useLuminanceAlpha(false),
    bloomEnabled(true),
    maxBodyMag(100.0f),
    exposure(1.0f),
    exposurePrev(1.0f),
    brightPlus(0.0f),
#endif
    videoSync(false),
    settingsChanged(true),
    objectAnnotationSetOpen(false)
{
    starVertexBuffer = new StarVertexBuffer(2048);
    pointStarVertexBuffer = new PointStarVertexBuffer(2048);
    glareVertexBuffer = new PointStarVertexBuffer(2048);
    skyVertices = new SkyVertex[MaxSkySlices * (MaxSkyRings + 1)];
    skyIndices = new uint32[(MaxSkySlices + 1) * 2 * MaxSkyRings];
    skyContour = new SkyContourPoint[MaxSkySlices + 1];
    colorTemp = GetStarColorTable(ColorTable_Enhanced);
#ifdef DEBUG_HDR_FILE
    HDR_LOG.open("hdr.log", ios_base::app);
#endif
#ifdef USE_HDR
    blurTextures = new Texture*[BLUR_PASS_COUNT];
    blurTempTexture = NULL;
    for (size_t i = 0; i < BLUR_PASS_COUNT; ++i)
    {
        blurTextures[i] = NULL;
    }
#endif
#ifdef USE_BLOOM_LISTS
    for (size_t i = 0; i < (sizeof gaussianLists/sizeof(GLuint)); ++i)
    {
        gaussianLists[i] = 0;
    }
#endif

    for (int i = 0; i < (int) FontCount; i++)
    {
        font[i] = NULL;
    }
}


Renderer::~Renderer()
{
    if (starVertexBuffer != NULL)
        delete starVertexBuffer;
    if (pointStarVertexBuffer != NULL)
        delete pointStarVertexBuffer;
    delete[] skyVertices;
    delete[] skyIndices;
    delete[] skyContour;
#ifdef USE_BLOOM_LISTS
    for (size_t i = 0; i < (sizeof gaussianLists/sizeof(GLuint)); ++i)
    {
        if (gaussianLists[i] != 0)
            glDeleteLists(gaussianLists[i], 1);
    }
#endif
#ifdef USE_HDR
    for (size_t i = 0; i < BLUR_PASS_COUNT; ++i)
    {
        if (blurTextures[i] != NULL)
            delete blurTextures[i];
    }
    delete [] blurTextures;
    if (blurTempTexture)
        delete blurTempTexture;

    if (sceneTexture != 0)
        glDeleteTextures(1, &sceneTexture);
#endif
}


Renderer::DetailOptions::DetailOptions() :
    ringSystemSections(100),
    orbitPathSamplePoints(100),
    shadowTextureSize(256),
    eclipseTextureSize(128)
{
}


static void StarTextureEval(float u, float v, float,
                            unsigned char *pixel)
{
    float r = 1 - (float) sqrt(u * u + v * v);
    if (r < 0)
        r = 0;
    else if (r < 0.5f)
        r = 2.0f * r;
    else
        r = 1;

    int pixVal = (int) (r * 255.99f);
    pixel[0] = pixVal;
    pixel[1] = pixVal;
    pixel[2] = pixVal;
}

static void GlareTextureEval(float u, float v, float,
                             unsigned char *pixel)
{
    float r = 0.9f - (float) sqrt(u * u + v * v);
    if (r < 0)
        r = 0;

    int pixVal = (int) (r * 255.99f);
    pixel[0] = 65;
    pixel[1] = 64;
    pixel[2] = 65;
    pixel[3] = pixVal;
}

static void ShadowTextureEval(float u, float v, float,
                              unsigned char *pixel)
{
    float r = (float) sqrt(u * u + v * v);

    // Leave some white pixels around the edges to the shadow doesn't
    // 'leak'.  We'll also set the maximum mip map level for this texture to 3
    // so we don't have problems with the edge texels at high mip map levels.
    int pixVal = r < 15.0f / 16.0f ? 0 : 255;
    pixel[0] = pixVal;
    pixel[1] = pixVal;
    pixel[2] = pixVal;
}


//! Lookup function for eclipse penumbras--the input is the amount of overlap
//  between the occluder and sun disc, and the output is the fraction of
//  full brightness.
static void PenumbraFunctionEval(float u, float, float,
                                 unsigned char *pixel)
{
    u = (u + 1.0f) * 0.5f;

    // Using the cube root produces a good visual result
    unsigned char pixVal = (unsigned char) (::pow((double) u, 0.33) * 255.99);

    pixel[0] = pixVal;
}


// ShadowTextureFunction is a function object for creating shadow textures
// used for rendering eclipses.
class ShadowTextureFunction : public TexelFunctionObject
{
public:
    ShadowTextureFunction(float _umbra) : umbra(_umbra) {};
    virtual void operator()(float u, float v, float w, unsigned char* pixel);
    float umbra;
};

void ShadowTextureFunction::operator()(float u, float v, float,
                                       unsigned char* pixel)
{
    float r = (float) sqrt(u * u + v * v);
    int pixVal = 255;

    // Leave some white pixels around the edges to the shadow doesn't
    // 'leak'.  We'll also set the maximum mip map level for this texture to 3
    // so we don't have problems with the edge texels at high mip map levels.
    r = r / (15.0f / 16.0f);
    if (r < 1)
    {
        // The pixel value should depend on the area of the sun which is
        // occluded.  We just fudge it here and use the square root of the
        // radius.
        if (r <= umbra)
            pixVal = 0;
        else
            pixVal = (int) (sqrt((r - umbra) / (1 - umbra)) * 255.99f);
    }

    pixel[0] = pixVal;
    pixel[1] = pixVal;
    pixel[2] = pixVal;
};


class ShadowMaskTextureFunction : public TexelFunctionObject
{
public:
    ShadowMaskTextureFunction() {};
    virtual void operator()(float u, float v, float w, unsigned char* pixel);
    float dummy;
};

void ShadowMaskTextureFunction::operator()(float u, float, float,
                                           unsigned char* pixel)
{
    unsigned char a = u > 0.0f ? 255 : 0;
    pixel[0] = a;
    pixel[1] = a;
    pixel[2] = a;
    pixel[3] = a;
}


static void IllumMapEval(float x, float y, float z,
                         unsigned char* pixel)
{
    pixel[0] = 128 + (int) (127 * x);
    pixel[1] = 128 + (int) (127 * y);
    pixel[2] = 128 + (int) (127 * z);
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
    uint16 rg = (uint16) (u * 65535.99);
    uint16 ba = (uint16) (v * 65535.99);
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
    float s = 1.0f / (sigma * (float) sqrt(2.0 * PI));

    for (unsigned int i = 0; i < size; i++)
    {
        float y = (float) i - size / 2;
        for (unsigned int j = 0; j < size; j++)
        {
            float x = (float) j - size / 2;
            float r2 = x * x + y * y;
            float f = s * (float) exp(-r2 * isig2) * power;

            mipPixels[i * size + j] = (unsigned char) (255.99f * min(f, 1.0f));
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
            float r = (float) sqrt(x * x + y * y);
            float f = (float) pow(base, r * scale);
            mipPixels[i * size + j] = (unsigned char) (255.99f * min(f, 1.0f));
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
    Image* img = new Image(GL_LUMINANCE, size, size, log2size + 1);

    for (unsigned int mipLevel = 0; mipLevel <= log2size; mipLevel++)
    {
        float fwhm = (float) pow(2.0f, (float) (log2size - mipLevel)) * 0.3f;
        BuildGaussianDiscMipLevel(img->getMipLevel(mipLevel),
                                  log2size - mipLevel,
                                  fwhm,
                                  (float) pow(2.0f, (float) (log2size - mipLevel)));
    }

    ImageTexture* texture = new ImageTexture(*img,
                                             Texture::BorderClamp,
                                             Texture::DefaultMipMaps);
    texture->setBorderColor(Color(0.0f, 0.0f, 0.0f, 0.0f));

    delete img;

    return texture;
}


static Texture* BuildGaussianGlareTexture(unsigned int log2size)
{
    unsigned int size = 1 << log2size;
    Image* img = new Image(GL_LUMINANCE, size, size, log2size + 1);

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
                                             Texture::BorderClamp,
                                             Texture::DefaultMipMaps);
    texture->setBorderColor(Color(0.0f, 0.0f, 0.0f, 0.0f));

    delete img;

    return texture;
}


static int translateLabelModeToClassMask(int labelMode)
{
    int classMask = 0;

    if (labelMode & Renderer::PlanetLabels)
        classMask |= Body::Planet;
    if (labelMode & Renderer::DwarfPlanetLabels)
        classMask |= Body::DwarfPlanet;
    if (labelMode & Renderer::MoonLabels)
        classMask |= Body::Moon;
    if (labelMode & Renderer::MinorMoonLabels)
        classMask |= Body::MinorMoon;
    if (labelMode & Renderer::AsteroidLabels)
        classMask |= Body::Asteroid;
    if (labelMode & Renderer::CometLabels)
        classMask |= Body::Comet;
    if (labelMode & Renderer::SpacecraftLabels)
        classMask |= Body::Spacecraft;

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


bool Renderer::init(GLContext* _context,
                    int winWidth, int winHeight,
                    DetailOptions& _detailOptions)
{
    context = _context;
    detailOptions = _detailOptions;

    // Initialize static meshes and textures common to all instances of Renderer
    if (!commonDataInitialized)
    {
        g_lodSphere = new LODSphereMesh();

        starTex = CreateProceduralTexture(64, 64, GL_RGB, StarTextureEval);

        glareTex = LoadTextureFromFile("textures/flare.jpg");
        if (glareTex == NULL)
            glareTex = CreateProceduralTexture(64, 64, GL_RGB, GlareTextureEval);

        // Max mipmap level doesn't work reliably on all graphics
        // cards.  In particular, Rage 128 and TNT cards resort to software
        // rendering when this feature is enabled.  The only workaround is to
        // disable mipmapping completely unless texture border clamping is
        // supported, which solves the problem much more elegantly than all
        // the mipmap level nonsense.
        // shadowTex->setMaxMipMapLevel(3);
        Texture::AddressMode shadowTexAddress = Texture::EdgeClamp;
        Texture::MipMapMode shadowTexMip = Texture::NoMipMaps;
        useClampToBorder = (GLEW_ARB_texture_border_clamp == GL_TRUE);
        if (useClampToBorder)
        {
            shadowTexAddress = Texture::BorderClamp;
            shadowTexMip = Texture::DefaultMipMaps;
        }

        shadowTex = CreateProceduralTexture(detailOptions.shadowTextureSize,
                                            detailOptions.shadowTextureSize,
                                            GL_RGB,
                                            ShadowTextureEval,
                                            shadowTexAddress, shadowTexMip);
        shadowTex->setBorderColor(Color::White);

        if (gaussianDiscTex == NULL)
            gaussianDiscTex = BuildGaussianDiscTexture(8);
        if (gaussianGlareTex == NULL)
            gaussianGlareTex = BuildGaussianGlareTexture(9);

        // Create the eclipse shadow textures
        {
            for (int i = 0; i < 4; i++)
            {
                ShadowTextureFunction func(i * 0.25f);
                eclipseShadowTextures[i] =
                    CreateProceduralTexture(detailOptions.eclipseTextureSize,
                                            detailOptions.eclipseTextureSize,
                                            GL_RGB, func,
                                            shadowTexAddress, shadowTexMip);
                if (eclipseShadowTextures[i] != NULL)
                {
                    // eclipseShadowTextures[i]->setMaxMipMapLevel(2);
                    eclipseShadowTextures[i]->setBorderColor(Color::White);
                }
            }
        }

        // Create the shadow mask texture
        {
            ShadowMaskTextureFunction func;
            shadowMaskTexture = CreateProceduralTexture(128, 2, GL_RGBA, func);
            //shadowMaskTexture->bindName();
        }

        // Create a function lookup table in a texture for use with
        // fragment program eclipse shadows.
        penumbraFunctionTexture = CreateProceduralTexture(512, 1, GL_LUMINANCE,
                                                          PenumbraFunctionEval,
                                                          Texture::EdgeClamp);

        if (GLEW_ARB_texture_cube_map)
        {
            normalizationTex = CreateProceduralCubeMap(64, GL_RGB, IllumMapEval);
#if ADVANCED_CLOUD_SHADOWS
            rectToSphericalTexture = CreateProceduralCubeMap(128, GL_RGBA, RectToSphericalMapEval);
#endif
        }

#ifdef USE_HDR
        genSceneTexture();
        genBlurTextures();
#endif

#ifdef ENABLE_SELF_SHADOW
        if (GLEW_EXT_framebuffer_object)
        {
            shadowFbo = new FramebufferObject(1024, 1024, FramebufferObject::DepthAttachment);
            if (!shadowFbo->isValid())
            {
                clog << "Error creating shadow FBO.\n";
            }
        }
#endif

        commonDataInitialized = true;
    }

#if 0
    if (GLEW_ARB_multisample)
    {
        int nSamples = 0;
        int sampleBuffers = 0;
        int enabled = (int) glIsEnabled(GL_MULTISAMPLE_ARB);
        glGetIntegerv(GL_SAMPLE_BUFFERS_ARB, &sampleBuffers);
        glGetIntegerv(GL_SAMPLES_ARB, &nSamples);
        clog << "AA samples: " << nSamples
             << ", enabled=" << (int) enabled
             << ", sample buffers=" << (sampleBuffers)
             << "\n";
        glEnable(GL_MULTISAMPLE_ARB);
    }
#endif

    if (GLEW_EXT_rescale_normal)
    {
        // We need this enabled because we use glScale, but only
        // with uniform scale factors.
        DPRINTF(1, "Renderer: EXT_rescale_normal supported.\n");
        useRescaleNormal = true;
        glEnable(GL_RESCALE_NORMAL_EXT);
    }

    if (GLEW_ARB_point_sprite)
    {
        DPRINTF(1, "Renderer: point sprites supported.\n");
        usePointSprite = true;
    }

    if (GLEW_EXT_separate_specular_color)
    {
        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT, GL_SEPARATE_SPECULAR_COLOR_EXT);
    }

    // Ugly renderer-specific bug workarounds follow . . .
    char* glRenderer = (char*) glGetString(GL_RENDERER);
    if (glRenderer != NULL)
    {
        // Fog is broken with vertex program emulation in most versions of
        // the GF 1 and 2 drivers; we need to detect this and disable
        // vertex programs which output fog coordinates
        if (strstr(glRenderer, "GeForce3") != NULL ||
            strstr(glRenderer, "GeForce4") != NULL)
        {
            buggyVertexProgramEmulation = false;
        }

        if (strstr(glRenderer, "Savage4") != NULL ||
            strstr(glRenderer, "ProSavage") != NULL)
        {
            // S3 Savage4 drivers appear to rescale normals without reporting
            // EXT_rescale_normal.  Lighting will be messed up unless
            // we set the useRescaleNormal flag.
            useRescaleNormal = true;
        }
#ifdef TARGET_OS_MAC
        if (strstr(glRenderer, "ATI") != NULL ||
            strstr(glRenderer, "GMA 900") != NULL)
        {
            // Some drivers on the Mac appear to limit point sprite size.
            // This causes an abrupt size transition when going from billboards
            // to sprites. Rather than incur overhead accounting for the size limit,
            // do not use sprites on these renderers.
            // Affected cards: ATI (various), etc
            // Renderer strings are not unique.
            usePointSprite = false;
        }
#endif
    }

    // More ugly hacks; according to Matt Craighead at NVIDIA, an NVIDIA
    // OpenGL driver that reports version 1.3.1 or greater will have working
    // fog in emulated vertex programs.
    char* glVersion = (char*) glGetString(GL_VERSION);
    if (glVersion != NULL)
    {
        int major = 0, minor = 0, extra = 0;
        int nScanned = sscanf(glVersion, "%d.%d.%d", &major, &minor, &extra);

        if (nScanned >= 2)
        {
            if (major > 1 || minor > 3 || (minor == 3 && extra >= 1))
                buggyVertexProgramEmulation = false;
        }
    }

#ifdef USE_HDR
    useBlendSubtract = GLEW_EXT_blend_subtract;
    Image        *testImg = new Image(GL_LUMINANCE_ALPHA, 1, 1);
    ImageTexture *testTex = new ImageTexture(*testImg,
                                             Texture::EdgeClamp,
                                             Texture::NoMipMaps);
    delete testImg;
    GLint actualTexFormat = 0;
    glEnable(GL_TEXTURE_2D);
    testTex->bind();
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &actualTexFormat);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    switch (actualTexFormat)
    {
    case 2:
    case GL_LUMINANCE_ALPHA:
    case GL_LUMINANCE4_ALPHA4:
    case GL_LUMINANCE6_ALPHA2:
    case GL_LUMINANCE8_ALPHA8:
    case GL_LUMINANCE12_ALPHA4:
    case GL_LUMINANCE12_ALPHA12:
    case GL_LUMINANCE16_ALPHA16:
        useLuminanceAlpha = true;
        break;
    default:
        useLuminanceAlpha = false;
        break;
    }

    blurFormat = useLuminanceAlpha ? GL_LUMINANCE_ALPHA : GL_RGBA;
    delete testTex;
#endif

    glLoadIdentity();

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

    // LEQUAL rather than LESS required for multipass rendering
    glDepthFunc(GL_LEQUAL);

    resize(winWidth, winHeight);

    return true;
}


void Renderer::resize(int width, int height)
{
#ifdef USE_HDR
    if (width == windowWidth && height == windowHeight)
        return;
#endif
    windowWidth = width;
    windowHeight = height;
    cosViewConeAngle = computeCosViewConeAngle(fov, windowWidth, windowHeight);
    // glViewport(windowWidth, windowHeight);

#ifdef USE_HDR
    if (commonDataInitialized)
    {
        genSceneTexture();
        genBlurTextures();
    }
#endif
}

float Renderer::calcPixelSize(float fovY, float windowHeight)
{
    return 2 * (float) tan(degToRad(fovY / 2.0)) / (float) windowHeight;
}

void Renderer::setFieldOfView(float _fov)
{
    fov = _fov;
    corrFac = (0.12f * fov/FOV * fov/FOV + 1.0f);
    cosViewConeAngle = computeCosViewConeAngle(fov, windowWidth, windowHeight);
}

int Renderer::getScreenDpi() const
{
    return screenDpi;
}

void Renderer::setScreenDpi(int _dpi)
{
    screenDpi = _dpi;
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

unsigned int Renderer::getResolution() const
{
    return textureResolution;
}


void Renderer::setResolution(unsigned int resolution)
{
    if (resolution < TEXTURE_RESOLUTION)
        textureResolution = resolution;
    markSettingsChanged();
}


TextureFont* Renderer::getFont(FontStyle fs) const
{
    return font[(int) fs];
}

void Renderer::setFont(FontStyle fs, TextureFont* txf)
{
    font[(int) fs] = txf;
    markSettingsChanged();
}

void Renderer::setRenderMode(int _renderMode)
{
    renderMode = _renderMode;
    markSettingsChanged();
}

int Renderer::getRenderFlags() const
{
    return renderFlags;
}

void Renderer::setRenderFlags(int _renderFlags)
{
    renderFlags = _renderFlags;
    markSettingsChanged();
}

int Renderer::getLabelMode() const
{
    return labelMode;
}

void Renderer::setLabelMode(int _labelMode)
{
    labelMode = _labelMode;
    markSettingsChanged();
}

int Renderer::getOrbitMask() const
{
    return orbitMask;
}

void Renderer::setOrbitMask(int mask)
{
    orbitMask = mask;
    markSettingsChanged();
}


const ColorTemperatureTable*
Renderer::getStarColorTable() const
{
    return colorTemp;
}


void
Renderer::setStarColorTable(const ColorTemperatureTable* ct)
{
    colorTemp = ct;
    markSettingsChanged();
}


bool Renderer::getVideoSync() const
{
    return videoSync;
}

void Renderer::setVideoSync(bool sync)
{
    videoSync = sync;
    markSettingsChanged();
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


bool Renderer::getFragmentShaderEnabled() const
{
    return fragmentShaderEnabled;
}

void Renderer::setFragmentShaderEnabled(bool enable)
{
    fragmentShaderEnabled = enable && fragmentShaderSupported();
    markSettingsChanged();
}

bool Renderer::fragmentShaderSupported() const
{
    return context->bumpMappingSupported();
}

bool Renderer::getVertexShaderEnabled() const
{
    return vertexShaderEnabled;
}

void Renderer::setVertexShaderEnabled(bool enable)
{
    vertexShaderEnabled = enable && vertexShaderSupported();
    markSettingsChanged();
}

bool Renderer::vertexShaderSupported() const
{
    return useVertexPrograms;
}


void Renderer::addAnnotation(vector<Annotation>& annotations,
                             const MarkerRepresentation* markerRep,
                             const string& labelText,
                             Color color,
                             const Vector3f& pos,
                             LabelAlignment halign,
                             LabelVerticalAlignment valign,
                             float size)
{
    double winX, winY, winZ;
    GLint view[4] = { 0, 0, windowWidth, windowHeight };
    float depth = (float) (pos.x() * modelMatrix[2] +
                           pos.y() * modelMatrix[6] +
                           pos.z() * modelMatrix[10]);
    if (gluProject(pos.x(), pos.y(), pos.z(),
                   modelMatrix,
                   projMatrix,
                   view,
                   &winX, &winY, &winZ) != GL_FALSE)
    {
        Annotation a;

        a.labelText[0] = '\0';
        ReplaceGreekLetterAbbr(a.labelText, MaxLabelLength, labelText.c_str(), labelText.length());
        a.labelText[MaxLabelLength - 1] = '\0';

        a.markerRep = markerRep;
        a.color = color;
        a.position = Vector3f((float) winX, (float) winY, -depth);
        a.halign = halign;
        a.valign = valign;
        a.size = size;
        annotations.push_back(a);
    }
}


void Renderer::addForegroundAnnotation(const MarkerRepresentation* markerRep,
                                       const string& labelText,
                                       Color color,
                                       const Vector3f& pos,
                                       LabelAlignment halign,
                                       LabelVerticalAlignment valign,
                                       float size)
{
    addAnnotation(foregroundAnnotations, markerRep, labelText, color, pos, halign, valign, size);
}


void Renderer::addBackgroundAnnotation(const MarkerRepresentation* markerRep,
                                       const string& labelText,
                                       Color color,
                                       const Vector3f& pos,
                                       LabelAlignment halign,
                                       LabelVerticalAlignment valign,
                                       float size)
{
    addAnnotation(backgroundAnnotations, markerRep, labelText, color, pos, halign, valign, size);
}


void Renderer::addSortedAnnotation(const MarkerRepresentation* markerRep,
                                   const string& labelText,
                                   Color color,
                                   const Vector3f& pos,
                                   LabelAlignment halign,
                                   LabelVerticalAlignment valign,
                                   float size)
{
    double winX, winY, winZ;
    GLint view[4] = { 0, 0, windowWidth, windowHeight };
    float depth = (float) (pos.x() * modelMatrix[2] +
                           pos.y() * modelMatrix[6] +
                           pos.z() * modelMatrix[10]);
    if (gluProject(pos.x(), pos.y(), pos.z(),
                   modelMatrix,
                   projMatrix,
                   view,
                   &winX, &winY, &winZ) != GL_FALSE)
    {
        Annotation a;
        
        a.labelText[0] = '\0';
        if (markerRep == NULL)
        {
            //l.text = ReplaceGreekLetterAbbr(_(text.c_str()));
            strncpy(a.labelText, labelText.c_str(), MaxLabelLength);
            a.labelText[MaxLabelLength - 1] = '\0';
        }
        a.markerRep = markerRep;
        a.color = color;
        a.position = Vector3f((float) winX, (float) winY, -depth);
        a.halign = halign;
        a.valign = valign;
        a.size = size;
        depthSortedAnnotations.push_back(a);
    }
}


void Renderer::clearAnnotations(vector<Annotation>& annotations)
{
    annotations.clear();
}


void Renderer::clearSortedAnnotations()
{
    depthSortedAnnotations.clear();
}


// Return the orientation of the camera used to render the current
// frame. Available only while rendering a frame.
Quaternionf Renderer::getCameraOrientation() const
{
    return m_cameraOrientation;
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
        renderAnnotations(objectAnnotations.begin(),
                          objectAnnotations.end(),
                          -depthPartitions[currentIntervalIndex].nearZ,
                          -depthPartitions[currentIntervalIndex].farZ,
                          FontNormal);

        objectAnnotations.clear();
    }
}


void Renderer::addObjectAnnotation(const MarkerRepresentation* markerRep,
                                   const string& labelText,
                                   Color color,
                                   const Vector3f& pos)
{
    assert(objectAnnotationSetOpen);
    if (objectAnnotationSetOpen)
    {
        double winX, winY, winZ;
        GLint view[4] = { 0, 0, windowWidth, windowHeight };
        float depth = (float) (pos.x() * modelMatrix[2] +
                               pos.y() * modelMatrix[6] +
                               pos.z() * modelMatrix[10]);
        if (gluProject(pos.x(), pos.y(), pos.z(),
                       modelMatrix,
                       projMatrix,
                       view,
                       &winX, &winY, &winZ) != GL_FALSE)
        {

            Annotation a;
            
            a.labelText[0] = '\0';
            if (!labelText.empty())
            {
                strncpy(a.labelText, labelText.c_str(), MaxLabelLength);
                a.labelText[MaxLabelLength - 1] = '\0';
            }
            a.markerRep = markerRep;
            a.color = color;
            a.position = Vector3f((float) winX, (float) winY, -depth);
            a.size = 0.0f;

            objectAnnotations.push_back(a);
        }
    }
}


static void enableSmoothLines()
{
    // glEnable(GL_BLEND);
#ifdef USE_HDR
    glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
#else
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(1.5f);
}

static void disableSmoothLines()
{
    // glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDisable(GL_LINE_SMOOTH);
    glLineWidth(1.0f);
}


class OrbitSampler : public OrbitSampleProc
{
public:
    vector<CurvePlotSample> samples;

    OrbitSampler()
    {
    }

    void sample(double t, const Vector3d& position, const Vector3d& velocity)
    {
        CurvePlotSample samp;
        samp.t = t;
        samp.position = position;
        samp.velocity = velocity;
        samples.push_back(samp);
    }

    void insertForward(CurvePlot* plot)
    {
        for (vector<CurvePlotSample>::const_iterator iter = samples.begin(); iter != samples.end(); ++iter)
        {
            plot->addSample(*iter);
        }
    }

    void insertBackward(CurvePlot* plot)
    {
        for (vector<CurvePlotSample>::const_reverse_iterator iter = samples.rbegin(); iter != samples.rend(); ++iter)
        {
            plot->addSample(*iter);
        }
    }
};


Vector4f renderOrbitColor(const Body *body, bool selected, float opacity)
{
    Color orbitColor;

    if (selected)
    {
        // Highlight the orbit of the selected object in red
        orbitColor = Renderer::SelectionOrbitColor;
    }
    else if (body != NULL && body->isOrbitColorOverridden())
    {
        orbitColor = body->getOrbitColor();
    }
    else
    {
        int classification;
        if (body != NULL)
            classification = body->getOrbitClassification();
        else
            classification = Body::Stellar;

        switch (classification)
        {
        case Body::Moon:
            orbitColor = Renderer::MoonOrbitColor;
            break;
        case Body::MinorMoon:
            orbitColor = Renderer::MinorMoonOrbitColor;
            break;
        case Body::Asteroid:
            orbitColor = Renderer::AsteroidOrbitColor;
            break;
        case Body::Comet:
            orbitColor = Renderer::CometOrbitColor;
            break;
        case Body::Spacecraft:
            orbitColor = Renderer::SpacecraftOrbitColor;
            break;
        case Body::Stellar:
            orbitColor = Renderer::StarOrbitColor;
            break;
        case Body::DwarfPlanet:
            orbitColor = Renderer::DwarfPlanetOrbitColor;
            break;
        case Body::Planet:
        default:
            orbitColor = Renderer::PlanetOrbitColor;
            break;
        }
    }

#ifdef USE_HDR
    return Vector4f(orbitColor.red(), orbitColor.green(), orbitColor.blue(), 1.0f - opacity * orbitColor.alpha());
#else
    return Vector4f(orbitColor.red(), orbitColor.green(), orbitColor.blue(), opacity * orbitColor.alpha());
#endif
}


static int orbitsRendered = 0;
static int orbitsSkipped = 0;
static int sectionsCulled = 0;

void Renderer::renderOrbit(const OrbitPathListEntry& orbitPath,
                           double t,
                           const Quaterniond& cameraOrientation,
                           const Frustum& frustum,
                           float nearDist,
                           float farDist)
{
    Body* body = orbitPath.body;
    double nearZ = -nearDist;  // negate, becase z is into the screen in camera space
    double farZ = -farDist;

    const Orbit* orbit = NULL;
    if (body != NULL)
        orbit = body->getOrbit(t);
    else
        orbit = orbitPath.star->getOrbit();

    CurvePlot* cachedOrbit = NULL;
    OrbitCache::iterator cached = orbitCache.find(orbit);
    if (cached != orbitCache.end())
    {
        cachedOrbit = cached->second;
        cachedOrbit->setLastUsed(frameCount);
    }

    // If it's not in the cache already
    if (cachedOrbit == NULL)
    {
        double startTime = t;
        int nSamples = detailOptions.orbitPathSamplePoints;

        // Adjust the number of samples used for aperiodic orbits--these aren't
        // true orbits, but are sampled trajectories, generally of spacecraft.
        // Better control is really needed--some sort of adaptive sampling would
        // be ideal.
        if (!orbit->isPeriodic())
        {
            double begin = 0.0, end = 0.0;
            orbit->getValidRange(begin, end);

            if (begin != end)
            {
                startTime = begin;
                nSamples = (int) (orbit->getPeriod() * 100.0);
                nSamples = max(min(nSamples, 1000), 100);
            }
            else
            {
                // If the orbit is aperiodic and doesn't have a
                // finite duration, we don't render it. A compromise
                // would be to pick some time window centered at the
                // current time, but we'd have to pick some arbitrary
                // duration.
                nSamples = 0;
            }
        }
        else
        {
            startTime = t - orbit->getPeriod();
        }

        cachedOrbit = new CurvePlot();
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
                for (OrbitCache::iterator iter = orbitCache.begin(); iter != orbitCache.end();)
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
    // are orbital periods.
    const double OrbitWindowEnd     = 0.5;

    // Number of orbit periods shown. The orbit window is:
    //    [ t + (OrbitWindowEnd - OrbitPeriodsShown) * T, t + OrbitWindowEnd * T ]
    // where t is the current simulation time and T is the orbital period.
    const double OrbitPeriodsShown  = 1.0;

    // Fraction of the window over which the orbit fades from opaque to transparent.
    // Fading is disabled when this value is zero.
    const double LinearFadeFraction = 0.0;

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
    Transform3d modelview;
    {
        Quaterniond orientation = Quaterniond::Identity();
        if (body)
        {
            orientation = body->getOrbitFrame(t)->getOrientation(t);
        }

        modelview = cameraOrientation * Translation3d(orbitPath.origin) * orientation.conjugate();
    }

    glPushMatrix();
    glLoadIdentity();

    bool highlight;
    if (body != NULL)
        highlight = highlightObject.body() == body;
    else
        highlight = highlightObject.star() == orbitPath.star;
    Vector4f orbitColor = renderOrbitColor(body, highlight, orbitPath.opacity);
    glColor4fv(orbitColor.data());

#ifdef STIPPLED_LINES
    glLineStipple(3, 0x5555);
    glEnable(GL_LINE_STIPPLE);
#endif

    double subdivisionThreshold = pixelSize * 40.0;

    Eigen::Vector3d viewFrustumPlaneNormals[4];
    for (int i = 0; i < 4; i++)
    {
        viewFrustumPlaneNormals[i] = frustum.plane(i).normal().cast<double>();
    }

    if (orbit->isPeriodic())
    {
        double period = orbit->getPeriod();
        double windowEnd = t + period * OrbitWindowEnd;
        double windowStart = windowEnd - period * OrbitPeriodsShown;
        double windowDuration = windowEnd - windowStart;

        if (LinearFadeFraction == 0.0f)
        {
            cachedOrbit->render(modelview,
                                nearZ, farZ, viewFrustumPlaneNormals,
                                subdivisionThreshold,
                                windowStart, windowEnd);
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
        if (renderFlags & ShowPartialTrajectories)
        {
            // Show the trajectory from the start time until the current simulation time
            cachedOrbit->render(modelview,
                                nearZ, farZ, viewFrustumPlaneNormals,
                                subdivisionThreshold,
                                cachedOrbit->startTime(), t);
        }
        else
        {
            // Show the entire trajectory
            cachedOrbit->render(modelview,
                                nearZ, farZ, viewFrustumPlaneNormals,
                                subdivisionThreshold);
        }
    }

#ifdef STIPPLED_LINES
    glDisable(GL_LINE_STIPPLE);
#endif

    glPopMatrix();
}


// Convert a position in the universal coordinate system to astrocentric
// coordinates, taking into account possible orbital motion of the star.
static Vector3d astrocentricPosition(const UniversalCoord& pos,
                                     const Star& star,
                                     double t)
{
    return pos.offsetFromKm(star.getPosition(t));
}


void Renderer::autoMag(float& faintestMag)
{
      float fieldCorr = 2.0f * FOV/(fov + FOV);
      faintestMag = (float) (faintestAutoMag45deg * sqrt(fieldCorr));
      saturationMag = saturationMagNight * (1.0f + fieldCorr * fieldCorr);
}


// Set up the light sources for rendering a solar system.  The positions of
// all nearby stars are converted from universal to viewer-centered
// coordinates.
static void
setupLightSources(const vector<const Star*>& nearStars,
                  const UniversalCoord& observerPos,
                  double t,
                  vector<LightSource>& lightSources)
{
    lightSources.clear();

    for (vector<const Star*>::const_iterator iter = nearStars.begin();
         iter != nearStars.end(); iter++)
    {
        if ((*iter)->getVisibility())
        {
            Vector3d v = (*iter)->getPosition(t).offsetFromKm(observerPos);
            LightSource ls;
            ls.position = v;
            ls.luminosity = (*iter)->getLuminosity();
            ls.radius = (*iter)->getRadius();

            // If the star is sufficiently cool, change the light color
            // from white.  Though our sun appears yellow, we still make
            // it and all hotter stars emit white light, as this is the
            // 'natural' light to which our eyes are accustomed.  We also
            // assign a slight bluish tint to light from O and B type stars,
            // though these will almost never have planets for their light
            // to shine upon.
            float temp = (*iter)->getTemperature();
            if (temp > 30000.0f)
                ls.color = Color(0.8f, 0.8f, 1.0f);
            else if (temp > 10000.0f)
                ls.color = Color(0.9f, 0.9f, 1.0f);
            else if (temp > 5400.0f)
                ls.color = Color(1.0f, 1.0f, 1.0f);
            else if (temp > 3900.0f)
                ls.color = Color(1.0f, 0.9f, 0.8f);
            else if (temp > 2000.0f)
                ls.color = Color(1.0f, 0.7f, 0.7f);
            else
                ls.color = Color(1.0f, 0.4f, 0.4f);

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
    float au2 = square(astro::kilometersToAU(1.0f));

    for (vector<SecondaryIlluminator>::iterator i = secondaryIlluminators.begin();
         i != secondaryIlluminators.end(); i++)
    {
        i->reflectedIrradiance = 0.0f;

        for (vector<LightSource>::const_iterator j = primaryIlluminators.begin();
             j != primaryIlluminators.end(); j++)
        {
            i->reflectedIrradiance += j->luminosity / ((float) (i->position_v - j->position).squaredNorm() * au2);
        }

        i->reflectedIrradiance *= i->body->getAlbedo();
    }
}


// Render an item from the render list
void Renderer::renderItem(const RenderListEntry& rle,
                          const Observer& observer,
                          const Quaternionf& cameraOrientation,
                          float nearPlaneDistance,
                          float farPlaneDistance)
{
    switch (rle.renderableType)
    {
    case RenderListEntry::RenderableStar:
        renderStar(*rle.star,
                   rle.position,
                   rle.distance,
                   rle.appMag,
                   cameraOrientation,
                   observer.getTime(),
                   nearPlaneDistance, farPlaneDistance);
        break;

    case RenderListEntry::RenderableBody:
        renderPlanet(*rle.body,
                     rle.position,
                     rle.distance,
                     rle.appMag,
                     observer,
                     cameraOrientation,
                     nearPlaneDistance, farPlaneDistance);
        break;

    case RenderListEntry::RenderableCometTail:
        renderCometTail(*rle.body,
                        rle.position,
                        observer.getTime(),
                        rle.discSizeInPixels);
        break;

    case RenderListEntry::RenderableReferenceMark:
        renderReferenceMark(*rle.refMark,
                            rle.position,
                            rle.distance,
                            observer.getTime(),
                            nearPlaneDistance);
        break;

    default:
        break;
    }
}


#ifdef USE_HDR
void Renderer::genBlurTextures()
{
    for (size_t i = 0; i < BLUR_PASS_COUNT; ++i)
    {
        if (blurTextures[i] != NULL)
        {
            delete blurTextures[i];
            blurTextures[i] = NULL;
        }
    }
    if (blurTempTexture)
    {
        delete blurTempTexture;
        blurTempTexture = NULL;
    }

    blurBaseWidth = sceneTexWidth, blurBaseHeight = sceneTexHeight;

    if (blurBaseWidth > blurBaseHeight)
    {
        while (blurBaseWidth > BLUR_SIZE)
        {
            blurBaseWidth  >>= 1;
            blurBaseHeight >>= 1;
        }
    }
    else
    {
        while (blurBaseHeight > BLUR_SIZE)
        {
            blurBaseWidth  >>= 1;
            blurBaseHeight >>= 1;
        }
    }
    genBlurTexture(0);
    genBlurTexture(1);

    Image *tempImg;
    ImageTexture *tempTexture;
    tempImg = new Image(GL_LUMINANCE, blurBaseWidth, blurBaseHeight);
    tempTexture = new ImageTexture(*tempImg, Texture::EdgeClamp, Texture::DefaultMipMaps);
    delete tempImg;
    if (tempTexture && tempTexture->getName() != 0)
        blurTempTexture = tempTexture;
}

void Renderer::genBlurTexture(int blurLevel)
{
    Image *img;
    ImageTexture *texture;

#ifdef DEBUG_HDR
    HDR_LOG <<
        "Window width = "    << windowWidth << ", " <<
        "Window height = "   << windowHeight << ", " <<
        "Blur tex width = "  << (blurBaseWidth>>blurLevel) << ", " <<
        "Blur tex height = " << (blurBaseHeight>>blurLevel) << endl;
#endif
    img = new Image(blurFormat,
                    blurBaseWidth>>blurLevel,
                    blurBaseHeight>>blurLevel);
    texture = new ImageTexture(*img,
                               Texture::EdgeClamp,
                               Texture::NoMipMaps);
    delete img;

    if (texture && texture->getName() != 0)
        blurTextures[blurLevel] = texture;
}

void Renderer::genSceneTexture()
{
	unsigned int *data;
    if (sceneTexture != 0)
        glDeleteTextures(1, &sceneTexture);

    sceneTexWidth  = 1;
    sceneTexHeight = 1;
    while (sceneTexWidth < windowWidth)
        sceneTexWidth <<= 1;
    while (sceneTexHeight < windowHeight)
        sceneTexHeight <<= 1;
    sceneTexWScale = (windowWidth > 0)  ? (GLfloat)sceneTexWidth  / (GLfloat)windowWidth :
        1.0f;
    sceneTexHScale = (windowHeight > 0) ? (GLfloat)sceneTexHeight / (GLfloat)windowHeight :
        1.0f;
	data = (unsigned int* )malloc(sceneTexWidth*sceneTexHeight*4*sizeof(unsigned int));
    memset(data, 0, sceneTexWidth*sceneTexHeight*4*sizeof(unsigned int));
    
    glGenTextures(1, &sceneTexture);
	glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, sceneTexWidth, sceneTexHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    
	free(data);
#ifdef DEBUG_HDR
    static int genSceneTexCounter = 1;
    HDR_LOG <<
        "[" << genSceneTexCounter++ << "] " <<
        "Window width = "  << windowWidth << ", " <<
        "Window height = " << windowHeight << ", " <<
        "Tex width = "  << sceneTexWidth << ", " <<
        "Tex height = " << sceneTexHeight << endl;
#endif
}

void Renderer::renderToBlurTexture(int blurLevel)
{
    if (blurTextures[blurLevel] == NULL)
        return;
    GLsizei blurTexWidth  = blurBaseWidth>>blurLevel;
    GLsizei blurTexHeight = blurBaseHeight>>blurLevel;
    GLsizei blurDrawWidth = (GLfloat)windowWidth/(GLfloat)sceneTexWidth * blurTexWidth;
    GLsizei blurDrawHeight = (GLfloat)windowHeight/(GLfloat)sceneTexHeight * blurTexHeight;
    GLfloat blurWScale = 1.f;
    GLfloat blurHScale = 1.f;
    GLfloat savedWScale = 1.f;
    GLfloat savedHScale = 1.f;

    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);
    glClearColor(0, 0, 0, 1.f);
    glViewport(0, 0, blurDrawWidth, blurDrawHeight);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);

    if (useBlendSubtract)
    {
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f, 0.0f, 1.0f);
        glEnd();
        // Do not need to scale alpha so mask it off
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
        glEnable(GL_BLEND);
        savedWScale = sceneTexWScale;
        savedHScale = sceneTexHScale;

        // Remove ldr part of image
        {
            const GLfloat bias  = -0.5f;
            glBlendFunc(GL_ONE, GL_ONE);
            glBlendEquationEXT(GL_FUNC_REVERSE_SUBTRACT_EXT);
            glColor4f(-bias, -bias, -bias, 0.0f);

            glDisable(GL_TEXTURE_2D);
            glBegin(GL_QUADS);
            glVertex2f(0.0f,           0.0f);
            glVertex2f(1.f, 0.0f);
            glVertex2f(1.f, 1.f);
            glVertex2f(0.0f,           1.f);
            glEnd();

            glEnable(GL_TEXTURE_2D);
            blurTextures[blurLevel]->bind();
            glCopyTexImage2D(GL_TEXTURE_2D, 0, blurFormat, 0, 0,
                             blurTexWidth, blurTexHeight, 0);
        }

        // Scale back up hdr part
        {
            glBlendEquationEXT(GL_FUNC_ADD_EXT);
            glBlendFunc(GL_DST_COLOR, GL_ONE);

            glBegin(GL_QUADS);
            drawBlendedVertices(0.f, 0.f, 1.f); //x2
            drawBlendedVertices(0.f, 0.f, 1.f); //x2
            glEnd();
        }

        glDisable(GL_BLEND);

        if (!useLuminanceAlpha)
        {
            blurTempTexture->bind();
            glCopyTexImage2D(GL_TEXTURE_2D, blurLevel, GL_LUMINANCE, 0, 0,
                             blurTexWidth, blurTexHeight, 0);
            // Erase color, replace with luminance image
            glBegin(GL_QUADS);
            glColor4f(0.f, 0.f, 0.f, 1.f);
            glVertex2f(0.0f, 0.0f);
            glVertex2f(1.0f, 0.0f);
            glVertex2f(1.0f, 1.0f);
            glVertex2f(0.0f, 1.0f);
            glEnd();
            glBegin(GL_QUADS);
            drawBlendedVertices(0.f, 0.f, 1.f);
            glEnd();
        }

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        blurTextures[blurLevel]->bind();
        glCopyTexImage2D(GL_TEXTURE_2D, 0, blurFormat, 0, 0,
                         blurTexWidth, blurTexHeight, 0);
    }
    else
    {
        // GL_EXT_blend_subtract not supported
        // Use compatible (but slow) glPixelTransfer instead
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f, 0.0f, 1.0f);
        glEnd();
        savedWScale = sceneTexWScale;
        savedHScale = sceneTexHScale;
        sceneTexWScale = blurWScale;
        sceneTexHScale = blurHScale;

        blurTextures[blurLevel]->bind();
        glPixelTransferf(GL_RED_SCALE,   8.f);
        glPixelTransferf(GL_GREEN_SCALE, 8.f);
        glPixelTransferf(GL_BLUE_SCALE,  8.f);
        glPixelTransferf(GL_RED_BIAS,   -0.5f);
        glPixelTransferf(GL_GREEN_BIAS, -0.5f);
        glPixelTransferf(GL_BLUE_BIAS,  -0.5f);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, blurFormat, 0, 0,
                         blurTexWidth, blurTexHeight, 0);
        glPixelTransferf(GL_RED_SCALE,   1.f);
        glPixelTransferf(GL_GREEN_SCALE, 1.f);
        glPixelTransferf(GL_BLUE_SCALE,  1.f);
        glPixelTransferf(GL_RED_BIAS,    0.f);
        glPixelTransferf(GL_GREEN_BIAS,  0.f);
        glPixelTransferf(GL_BLUE_BIAS,   0.f);
    }

    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLfloat xdelta = 1.0f / (GLfloat)blurTexWidth;
    GLfloat ydelta = 1.0f / (GLfloat)blurTexHeight;
    blurWScale = ((GLfloat)blurTexWidth / (GLfloat)blurDrawWidth);
    blurHScale = ((GLfloat)blurTexHeight / (GLfloat)blurDrawHeight);
    sceneTexWScale = blurWScale;
    sceneTexHScale = blurHScale;

    // Butterworth low pass filter to reduce flickering dots
    {
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f,    0.0f, .5f*1.f);
        drawBlendedVertices(-xdelta, 0.0f, .5f*0.333f);
        drawBlendedVertices( xdelta, 0.0f, .5f*0.25f);
        glEnd();
        glCopyTexImage2D(GL_TEXTURE_2D, 0, blurFormat, 0, 0,
                         blurTexWidth, blurTexHeight, 0);
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f, -ydelta, .5f*0.667f);
        drawBlendedVertices(0.0f,  ydelta, .5f*0.333f);
        glEnd();
        glCopyTexImage2D(GL_TEXTURE_2D, 0, blurFormat, 0, 0,
                         blurTexWidth, blurTexHeight, 0);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // Gaussian blur
    switch (blurLevel)
    {
/*
    case 0:
        drawGaussian3x3(xdelta, ydelta, blurTexWidth, blurTexHeight, 1.f);
        break;
*/
#ifdef TARGET_OS_MAC
    case 0:
        drawGaussian5x5(xdelta, ydelta, blurTexWidth, blurTexHeight, 1.f);
        break;
    case 1:
        drawGaussian9x9(xdelta, ydelta, blurTexWidth, blurTexHeight, .3f);
        break;
#else
    // Gamma correct: windows=(mac^1.8)^(1/2.2)
    case 0:
        drawGaussian5x5(xdelta, ydelta, blurTexWidth, blurTexHeight, 1.f);
        break;
    case 1:
        drawGaussian9x9(xdelta, ydelta, blurTexWidth, blurTexHeight, .373f);
        break;
#endif
    default:
        break;
    }

    blurTextures[blurLevel]->bind();
    glCopyTexImage2D(GL_TEXTURE_2D, 0, blurFormat, 0, 0,
                     blurTexWidth, blurTexHeight, 0);

    glDisable(GL_BLEND);
    glClear(GL_COLOR_BUFFER_BIT);
    glPopAttrib();
    sceneTexWScale = savedWScale;
    sceneTexHScale = savedHScale;
}

void Renderer::renderToTexture(const Observer& observer,
                               const Universe& universe,
                               float faintestMagNight,
                               const Selection& sel)
{
    if (sceneTexture == 0)
        return;
    glPushAttrib(GL_COLOR_BUFFER_BIT);

    draw(observer, universe, faintestMagNight, sel);

    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0,
                     sceneTexWidth, sceneTexHeight, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPopAttrib();
}

void Renderer::drawSceneTexture()
{
    if (sceneTexture == 0)
        return;
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glBegin(GL_QUADS);
    drawBlendedVertices(0.0f, 0.0f, 1.0f);
    glEnd();
}

void Renderer::drawBlendedVertices(float xdelta, float ydelta, float blend)
{
    glColor4f(1.0f, 1.0f, 1.0f, blend);
    glTexCoord2i(0, 0); glVertex2f(xdelta,                ydelta);
    glTexCoord2i(1, 0); glVertex2f(sceneTexWScale+xdelta, ydelta);
    glTexCoord2i(1, 1); glVertex2f(sceneTexWScale+xdelta, sceneTexHScale+ydelta);
    glTexCoord2i(0, 1); glVertex2f(xdelta,                sceneTexHScale+ydelta);
}

void Renderer::drawGaussian3x3(float xdelta, float ydelta, GLsizei width, GLsizei height, float blend)
{
#ifdef USE_BLOOM_LISTS
    if (gaussianLists[0] == 0)
    {
        gaussianLists[0] = glGenLists(1);
        glNewList(gaussianLists[0], GL_COMPILE);
#endif
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f, 0.0f, blend);
        drawBlendedVertices(-xdelta, 0.0f, 0.25f*blend);
        drawBlendedVertices( xdelta, 0.0f, 0.20f*blend);
        glEnd();

        // Take result of horiz pass and apply vertical pass
        glCopyTexImage2D(GL_TEXTURE_2D, 0, blurFormat, 0, 0,
                         width, height, 0);
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f, -ydelta, 0.429f);
        drawBlendedVertices(0.0f,  ydelta, 0.300f);
        glEnd();
#ifdef USE_BLOOM_LISTS
        glEndList();
    }
    glCallList(gaussianLists[0]);
#endif
}

void Renderer::drawGaussian5x5(float xdelta, float ydelta, GLsizei width, GLsizei height, float blend)
{
#ifdef USE_BLOOM_LISTS
    if (gaussianLists[1] == 0)
    {
        gaussianLists[1] = glGenLists(1);
        glNewList(gaussianLists[1], GL_COMPILE);
#endif
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f, 0.0f, blend);
        drawBlendedVertices(-xdelta,      0.0f, 0.475f*blend);
        drawBlendedVertices( xdelta,      0.0f, 0.475f*blend);
        drawBlendedVertices(-2.0f*xdelta, 0.0f, 0.075f*blend);
        drawBlendedVertices( 2.0f*xdelta, 0.0f, 0.075f*blend);
        glEnd();
        glCopyTexImage2D(GL_TEXTURE_2D, 0, blurFormat, 0, 0,
                         width, height, 0);
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f, -ydelta,      0.475f);
        drawBlendedVertices(0.0f,  ydelta,      0.475f);
        drawBlendedVertices(0.0f, -2.0f*ydelta, 0.075f);
        drawBlendedVertices(0.0f,  2.0f*ydelta, 0.075f);
        glEnd();
#ifdef USE_BLOOM_LISTS
        glEndList();
    }
    glCallList(gaussianLists[1]);
#endif
}

void Renderer::drawGaussian9x9(float xdelta, float ydelta, GLsizei width, GLsizei height, float blend)
{
#ifdef USE_BLOOM_LISTS
    if (gaussianLists[2] == 0)
    {
        gaussianLists[2] = glGenLists(1);
        glNewList(gaussianLists[2], GL_COMPILE);
#endif
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f, 0.0f, blend);
        drawBlendedVertices(-xdelta,      0.0f, 0.632f*blend);
        drawBlendedVertices( xdelta,      0.0f, 0.632f*blend);
        drawBlendedVertices(-2.0f*xdelta, 0.0f, 0.159f*blend);
        drawBlendedVertices( 2.0f*xdelta, 0.0f, 0.159f*blend);
        drawBlendedVertices(-3.0f*xdelta, 0.0f, 0.016f*blend);
        drawBlendedVertices( 3.0f*xdelta, 0.0f, 0.016f*blend);
        glEnd();

        glCopyTexImage2D(GL_TEXTURE_2D, 0, blurFormat, 0, 0,
                         width, height, 0);
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f, -ydelta,      0.632f);
        drawBlendedVertices(0.0f,  ydelta,      0.632f);
        drawBlendedVertices(0.0f, -2.0f*ydelta, 0.159f);
        drawBlendedVertices(0.0f,  2.0f*ydelta, 0.159f);
        drawBlendedVertices(0.0f, -3.0f*ydelta, 0.016f);
        drawBlendedVertices(0.0f,  3.0f*ydelta, 0.016f);
        glEnd();
#ifdef USE_BLOOM_LISTS
        glEndList();
    }
    glCallList(gaussianLists[2]);
#endif
}

void Renderer::drawBlur()
{
    blurTextures[0]->bind();
    glBegin(GL_QUADS);
    drawBlendedVertices(0.0f, 0.0f, 1.0f);
    glEnd();
    blurTextures[1]->bind();
    glBegin(GL_QUADS);
    drawBlendedVertices(0.0f, 0.0f, 1.0f);
    glEnd();
}

bool Renderer::getBloomEnabled()
{
    return bloomEnabled;
}

void Renderer::setBloomEnabled(bool aBloomEnabled)
{
    bloomEnabled = aBloomEnabled;
}

void Renderer::increaseBrightness()
{
    brightPlus += 1.0f;
}

void Renderer::decreaseBrightness()
{
    brightPlus -= 1.0f;
}

float Renderer::getBrightness()
{
    return brightPlus;
}
#endif // USE_HDR

void Renderer::render(const Observer& observer,
                      const Universe& universe,
                      float faintestMagNight,
                      const Selection& sel)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

#ifdef USE_HDR
    renderToTexture(observer, universe, faintestMagNight, sel);

    //------------- Post processing from here ------------//
    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho( 0.0, 1.0, 0.0, 1.0, -1.0, 1.0 );
    glMatrixMode (GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    if (bloomEnabled)
    {
        renderToBlurTexture(0);
        renderToBlurTexture(1);
//        renderToBlurTexture(2);
    }

    drawSceneTexture();

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

#ifdef HDR_COMPRESS
    // Assume luminance 1.0 mapped to 128 previously
    // Compositing a 2nd copy doubles 128->255
    drawSceneTexture();
#endif

    if (bloomEnabled)
    {
        drawBlur();
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
#else
    draw(observer, universe, faintestMagNight, sel);
#endif
}

void Renderer::draw(const Observer& observer,
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
    setFieldOfView(radToDeg(observer.getFOV()));
    pixelSize = calcPixelSize(fov, (float) windowHeight);

    // Set up the projection we'll use for rendering stars.
    gluPerspective(fov,
                   (float) windowWidth / (float) windowHeight,
                   NEAR_DIST, FAR_DIST);

    // Set the modelview matrix
    glMatrixMode(GL_MODELVIEW);

    // Get the displayed surface texture set to use from the observer
    displayedSurface = observer.getDisplayedSurface();

    locationFilter = observer.getLocationFilter();

    if (usePointSprite && getGLContext()->getVertexProcessor() != NULL)
    {
        useNewStarRendering = true;
    }
    else
    {
        useNewStarRendering = false;
    }

    // Highlight the selected object
    highlightObject = sel;

    m_cameraOrientation = observer.getOrientationf();

    // Get the view frustum used for culling in camera space.
    float viewAspectRatio = (float) windowWidth / (float) windowHeight;
    Frustum frustum(degToRad(fov),
                    viewAspectRatio,
                    MinNearPlaneDistance);

    // Get the transformed frustum, used for culling in the astrocentric coordinate
    // system.
    Frustum xfrustum(degToRad(fov),
                     viewAspectRatio,
                     MinNearPlaneDistance);
    xfrustum.transform(observer.getOrientationf().conjugate().toRotationMatrix());

    // Set up the camera for star rendering; the units of this phase
    // are light years.
    Vector3f observerPosLY = observer.getPosition().offsetFromLy(Vector3f::Zero());
    glPushMatrix();
    glRotate(m_cameraOrientation);

    // Get the model matrix *before* translation.  We'll use this for
    // positioning star and planet labels.
    glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);

    clearSortedAnnotations();

    // Put all solar system bodies into the render list.  Stars close and
    // large enough to have discernible surface detail are also placed in
    // renderList.
    renderList.clear();
    orbitPathList.clear();
    lightSourceList.clear();
    secondaryIlluminators.clear();

    // See if we want to use AutoMag.
    if ((renderFlags & ShowAutoMag) != 0)
    {
        autoMag(faintestMag);
    }
    else
    {
        faintestMag = faintestMagNight;
        saturationMag = saturationMagNight;
    }

    faintestPlanetMag = faintestMag;
#ifdef USE_HDR
    float maxBodyMagPrev = saturationMag;
    maxBodyMag = min(maxBodyMag, saturationMag);
    vector<RenderListEntry>::iterator closestBody;
    const Star *brightestStar = NULL;
    bool foundClosestBody   = false;
    bool foundBrightestStar = false;
#endif

    if (renderFlags & ShowPlanets)
    {
        nearStars.clear();
        universe.getNearStars(observer.getPosition(), 1.0f, nearStars);

        // Set up direct light sources (i.e. just stars at the moment)
        setupLightSources(nearStars, observer.getPosition(), now, lightSourceList);

        // Traverse the frame trees of each nearby solar system and
        // build the list of objects to be rendered.
        for (vector<const Star*>::const_iterator iter = nearStars.begin();
             iter != nearStars.end(); iter++)
        {
            const Star* sun = *iter;
            SolarSystem* solarSystem = universe.getSolarSystem(sun);
            if (solarSystem != NULL)
            {
                FrameTree* solarSysTree = solarSystem->getFrameTree();
                if (solarSysTree != NULL)
                {
                    if (solarSysTree->updateRequired())
                    {
                        // Tree has changed, so we must recompute bounding spheres.
                        solarSysTree->recomputeBoundingSphere();
                        solarSysTree->markUpdated();
                    }

                    // Compute the position of the observer in astrocentric coordinates
                    Vector3d astrocentricObserverPos = astrocentricPosition(observer.getPosition(), *sun, now);

                    // Build render lists for bodies and orbits paths
                    buildRenderLists(astrocentricObserverPos,
                                     xfrustum,
                                     observer.getOrientation().conjugate() * -Vector3d::UnitZ(),
                                     Vector3d::Zero(),
                                     solarSysTree,
                                     observer,
                                     now);
                    if (renderFlags & ShowOrbits)
                    {
                        buildOrbitLists(astrocentricObserverPos,
                                        observer.getOrientation(),
                                        xfrustum,
                                        solarSysTree,
                                        now);
                    }
                }
            }

            addStarOrbitToRenderList(*sun, observer, now);
        }

        if ((labelMode & (BodyLabelMask)) != 0)
        {
            buildLabelLists(xfrustum, now);
        }

        starTex->bind();
    }

    setupSecondaryLightSources(secondaryIlluminators, lightSourceList);

#ifdef USE_HDR
    Mat3f viewMat = conjugate(observer.getOrientationf()).toMatrix3();
    float maxSpan = (float) sqrt(square((float) windowWidth) +
                                 square((float) windowHeight));
    float nearZcoeff = (float) cos(degToRad(fov / 2)) *
        ((float) windowHeight / maxSpan);

    // Remove objects from the render list that lie completely outside the
    // view frustum.
    vector<RenderListEntry>::iterator notCulled = renderList.begin();
    for (vector<RenderListEntry>::iterator iter = renderList.begin();
         iter != renderList.end(); iter++)
    {
        Point3f center = iter->position * viewMat;

        bool convex = true;
        float radius = 1.0f;
        float cullRadius = 1.0f;
        float cloudHeight = 0.0f;

        switch (iter->renderableType)
        {
        case RenderListEntry::RenderableStar:
            continue;

        case RenderListEntry::RenderableCometTail:
            radius = iter->radius;
            cullRadius = radius;
            convex = false;
            break;

        case RenderListEntry::RenderableReferenceMark:
            radius = iter->radius;
            cullRadius = radius;
            convex = false;
            break;

        case RenderListEntry::RenderableBody:
        default:
            radius = iter->body->getBoundingRadius();
            if (iter->body->getRings() != NULL)
            {
                radius = iter->body->getRings()->outerRadius;
                convex = false;
            }

            if (!iter->body->isEllipsoid())
                convex = false;

            cullRadius = radius;
            if (iter->body->getAtmosphere() != NULL)
            {
                cullRadius += iter->body->getAtmosphere()->height;
                cloudHeight = max(iter->body->getAtmosphere()->cloudHeight,
                                  iter->body->getAtmosphere()->mieScaleHeight * (float) -log(AtmosphereExtinctionThreshold));
            }
            break;
        }

        // Test the object's bounding sphere against the view frustum
        if (frustum.testSphere(center, cullRadius) != Frustum::Outside)
        {
            float nearZ = center.distanceFromOrigin() - radius;
            nearZ = -nearZ * nearZcoeff;
            
            if (nearZ > -MinNearPlaneDistance)
                iter->nearZ = -max(MinNearPlaneDistance, radius / 2000.0f);
            else
                iter->nearZ = nearZ;
            
            if (!convex)
            {
                iter->farZ = center.z - radius;
                if (iter->farZ / iter->nearZ > MaxFarNearRatio * 0.5f)
                    iter->nearZ = iter->farZ / (MaxFarNearRatio * 0.5f);
            }
            else
            {
                // Make the far plane as close as possible
                float d = center.distanceFromOrigin();
                
                // Account for ellipsoidal objects
                float eradius = radius;
                if (iter->body != NULL)
                {
                    Vec3f semiAxes = iter->body->getSemiAxes();
                    float minSemiAxis = min(semiAxes.x, min(semiAxes.y, semiAxes.z));
                    eradius *= minSemiAxis / radius;
                }

                if (d > eradius)
                {
                    iter->farZ = iter->centerZ - iter->radius;
                }
                else
                {
                    // We're inside the bounding sphere (and, if the planet
                    // is spherical, inside the planet.)
                    iter->farZ = iter->nearZ * 2.0f;
                }
                
                if (cloudHeight > 0.0f)
                {
                    // If there's a cloud layer, we need to move the
                    // far plane out so that the clouds aren't clipped
                    float cloudLayerRadius = eradius + cloudHeight;
                    iter->farZ -= (float) sqrt(square(cloudLayerRadius) -
                                               square(eradius));
                }
            }
            
            *notCulled = *iter;
            notCulled++;

            maxBodyMag = min(maxBodyMag, iter->appMag);
            foundClosestBody = true;
        }
    }

    renderList.resize(notCulled - renderList.begin());
    saturationMag = maxBodyMag;
#endif // USE_HDR

    Color skyColor(0.0f, 0.0f, 0.0f);

    // Scan through the render list to see if we're inside a planetary
    // atmosphere.  If so, we need to adjust the sky color as well as the
    // limiting magnitude of stars (so stars aren't visible in the daytime
    // on planets with thick atmospheres.)
    if ((renderFlags & ShowAtmospheres) != 0)
    {
        for (vector<RenderListEntry>::iterator iter = renderList.begin();
             iter != renderList.end(); iter++)
        {
            if (iter->renderableType == RenderListEntry::RenderableBody && iter->body->getAtmosphere() != NULL)
            {
                // Compute the density of the atmosphere, and from that
                // the amount light scattering.  It's complicated by the
                // possibility that the planet is oblate and a simple distance
                // to sphere calculation will not suffice.
                const Atmosphere* atmosphere = iter->body->getAtmosphere();
                float radius = iter->body->getRadius();
                Vector3f semiAxes = iter->body->getSemiAxes() / radius;

                Vector3f recipSemiAxes = semiAxes.cwise().inverse();
                Vector3f eyeVec = iter->position / radius;

                // Compute the orientation of the planet before axial rotation
                Quaterniond qd = iter->body->getEclipticToEquatorial(now);
                Quaternionf q = qd.cast<float>();
                eyeVec = q * eyeVec;

                // ellipDist is not the true distance from the surface unless
                // the planet is spherical.  The quantity that we do compute
                // is the distance to the surface along a line from the eye
                // position to the center of the ellipsoid.
                float ellipDist = (eyeVec.cwise() * recipSemiAxes).norm() - 1.0f;
                if (ellipDist < atmosphere->height / radius &&
                    atmosphere->height > 0.0f)
                {
                    float density = 1.0f - ellipDist /
                        (atmosphere->height / radius);
                    if (density > 1.0f)
                        density = 1.0f;

                    Vector3f sunDir = iter->sun.normalized();
                    Vector3f normal = -iter->position.normalized();
#ifdef USE_HDR
                    // Ignore magnitude of planet underneath when lighting atmosphere
                    // Could be changed to simulate light pollution, etc
                    maxBodyMag = maxBodyMagPrev;
                    saturationMag = maxBodyMag;
#endif
                    float illumination = Math<float>::clamp(sunDir.dot(normal) + 0.2f);

                    float lightness = illumination * density;
                    faintestMag = faintestMag  - 15.0f * lightness;
                    saturationMag = saturationMag - 15.0f * lightness;
                }
            }
        }
    }

    // Now we need to determine how to scale the brightness of stars.  The
    // brightness will be proportional to the apparent magnitude, i.e.
    // a logarithmic function of the stars apparent brightness.  This mimics
    // the response of the human eye.  We sort of fudge things here and
    // maintain a minimum range of six magnitudes between faintest visible
    // and saturation; this keeps stars from popping in or out as the sun
    // sets or rises.
#ifdef USE_HDR
    brightnessScale = 1.0f / (faintestMag -  saturationMag);
#else
    if (faintestMag - saturationMag >= 6.0f)
        brightnessScale = 1.0f / (faintestMag -  saturationMag);
    else
        brightnessScale = 0.1667f;
#endif

#ifdef USE_HDR
    exposurePrev = exposure;
    float exposureNow = 1.f / (1.f+exp((faintestMag - saturationMag + DEFAULT_EXPOSURE)/2.f));
    exposure = exposurePrev + (exposureNow - exposurePrev) * (1.f - exp(-1.f/(15.f * EXPOSURE_HALFLIFE)));
    brightnessScale /= exposure;
#endif

#ifdef DEBUG_HDR_TONEMAP
    HDR_LOG <<
//        "brightnessScale = " << brightnessScale <<
        "faint = "    << faintestMag << ", " <<
        "sat = "      << saturationMag << ", " <<
        "exposure = " << (exposure+brightPlus) << endl;
#endif

#ifdef HDR_COMPRESS
    ambientColor = Color(ambientLightLevel*.5f, ambientLightLevel*.5f, ambientLightLevel*.5f);
#else
    ambientColor = Color(ambientLightLevel, ambientLightLevel, ambientLightLevel);
#endif

    // Create the ambient light source.  For realistic scenes in space, this
    // should be black.
    glAmbientLightColor(ambientColor);

#ifdef USE_HDR
    glClearColor(skyColor.red(), skyColor.green(), skyColor.blue(), 0.0f);
#else
    glClearColor(skyColor.red(), skyColor.green(), skyColor.blue(), 1);
#endif
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    
    // Render sky grids first--these will always be in the background
    {
        glDisable(GL_TEXTURE_2D);
        if ((renderFlags & ShowSmoothLines) != 0)
            enableSmoothLines();
        renderSkyGrids(observer);
        if ((renderFlags & ShowSmoothLines) != 0)
            disableSmoothLines();
        glEnable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
    }

    // Render deep sky objects
    if ((renderFlags & (ShowGalaxies | 
						ShowGlobulars |
                        ShowNebulae |
                        ShowOpenClusters)) != 0 &&
        universe.getDSOCatalog() != NULL)
    {
        renderDeepSkyObjects(universe, observer, faintestMag);
    }

    // Translate the camera before rendering the stars
    glPushMatrix();

    // Render stars
#ifdef USE_HDR
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
#endif
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    if ((renderFlags & ShowStars) != 0 && universe.getStarCatalog() != NULL)
    {
        // Disable multisample rendering when drawing point stars
        bool toggleAA = (starStyle == Renderer::PointStars && glIsEnabled(GL_MULTISAMPLE_ARB));
        if (toggleAA)
            glDisable(GL_MULTISAMPLE_ARB);

        if (useNewStarRendering)
            renderPointStars(*universe.getStarCatalog(), faintestMag, observer);
        else
            renderStars(*universe.getStarCatalog(), faintestMag, observer);

        if (toggleAA)
            glEnable(GL_MULTISAMPLE_ARB);
    }

#ifdef USE_HDR
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#endif

    glTranslatef(-observerPosLY.x(), -observerPosLY.y(), -observerPosLY.z());

    // Render asterisms
    if ((renderFlags & ShowDiagrams) != 0 && universe.getAsterisms() != NULL)
    {
        /* We'll linearly fade the lines as a function of the observer's
           distance to the origin of coordinates: */
        float opacity = 1.0f;
        float dist = observerPosLY.norm();
        if (dist > MaxAsterismLinesConstDist)
        {
            opacity = clamp((MaxAsterismLinesConstDist - dist) /
                            (MaxAsterismLinesDist - MaxAsterismLinesConstDist) + 1);
        }

        glColor(ConstellationColor, opacity);
        glDisable(GL_TEXTURE_2D);
        if ((renderFlags & ShowSmoothLines) != 0)
            enableSmoothLines();
        AsterismList* asterisms = universe.getAsterisms();
        for (AsterismList::const_iterator iter = asterisms->begin();
             iter != asterisms->end(); iter++)
        {
            Asterism* ast = *iter;

			if (ast->getActive())
            {
				if (ast->isColorOverridden())
					glColor(ast->getOverrideColor(), opacity);
				else
					glColor(ConstellationColor, opacity);

				for (int i = 0; i < ast->getChainCount(); i++)
				{
					const Asterism::Chain& chain = ast->getChain(i);

					glBegin(GL_LINE_STRIP);
					for (Asterism::Chain::const_iterator iter = chain.begin();
						 iter != chain.end(); iter++)
						glVertex3fv(iter->data());
					glEnd();
				}
			}
        }

        if ((renderFlags & ShowSmoothLines) != 0)
            disableSmoothLines();
    }

    if ((renderFlags & ShowBoundaries) != 0)
    {
        /* We'll linearly fade the boundaries as a function of the
           observer's distance to the origin of coordinates: */
        float opacity = 1.0f;
        float dist = observerPosLY.norm() * 1e6f;
        if (dist > MaxAsterismLabelsConstDist)
        {
            opacity = clamp((MaxAsterismLabelsConstDist - dist) /
                            (MaxAsterismLabelsDist - MaxAsterismLabelsConstDist) + 1);
        }
        glColor(BoundaryColor, opacity);

        glDisable(GL_TEXTURE_2D);
        if ((renderFlags & ShowSmoothLines) != 0)
            enableSmoothLines();
        if (universe.getBoundaries() != NULL)
            universe.getBoundaries()->render();
        if ((renderFlags & ShowSmoothLines) != 0)
            disableSmoothLines();
    }

    // Render star and deep sky object labels
    renderBackgroundAnnotations(FontNormal);

    // Render constellations labels
    if ((labelMode & ConstellationLabels) != 0 && universe.getAsterisms() != NULL)
    {
        labelConstellations(*universe.getAsterisms(), observer);
        renderBackgroundAnnotations(FontLarge);
    }

    // Pop observer translation
    glPopMatrix();

    if ((renderFlags & ShowMarkers) != 0)
    {
        renderMarkers(*universe.getMarkers(),
                      observer.getPosition(),
        		      observer.getOrientation(),
                      now);
        
        // Render background markers; rendering of other markers is deferred until
        // solar system objects are rendered.
        renderBackgroundAnnotations(FontNormal);
    }    

    // Draw the selection cursor
    bool selectionVisible = false;
    if (!sel.empty() && (renderFlags & ShowMarkers))
    {
        Vector3d offset = sel.getPosition(now).offsetFromKm(observer.getPosition());

        static MarkerRepresentation cursorRep(MarkerRepresentation::Crosshair);
        selectionVisible = xfrustum.testSphere(offset, sel.radius()) != Frustum::Outside;

        if (selectionVisible)
        {
            double distance = offset.norm();
            float symbolSize = (float) (sel.radius() / distance) / pixelSize;

            // Modify the marker position so that it is always in front of the marked object.
            double boundingRadius;
            if (sel.body() != NULL)
                boundingRadius = sel.body()->getBoundingRadius();
            else
                boundingRadius = sel.radius();                
            offset *= (1.0 - boundingRadius * 1.01 / distance);

            // The selection cursor is only partially visible when the selected object is obscured. To implement
            // this behavior we'll draw two markers at the same position: one that's always visible, and another one
            // that's depth sorted. When the selection is occluded, only the foreground marker is visible. Otherwise,
            // both markers are drawn and cursor appears much brighter as a result.
            if (distance < astro::lightYearsToKilometers(1.0))
            {
                addSortedAnnotation(&cursorRep, EMPTY_STRING, Color(SelectionCursorColor, 1.0f),
                                    offset.cast<float>(),
                                    AlignLeft, VerticalAlignTop, symbolSize);
            }
            else
            {
                addAnnotation(backgroundAnnotations, &cursorRep, EMPTY_STRING, Color(SelectionCursorColor, 1.0f),
                              offset.cast<float>(),
                              AlignLeft, VerticalAlignTop, symbolSize);
            }

            Color occludedCursorColor(SelectionCursorColor.red(), SelectionCursorColor.green() + 0.3f, SelectionCursorColor.blue());
            addAnnotation(foregroundAnnotations,
                          &cursorRep, EMPTY_STRING, Color(occludedCursorColor, 0.4f),
                          offset.cast<float>(),
                          AlignLeft, VerticalAlignTop, symbolSize);
        }
    }

    glPolygonMode(GL_FRONT, (GLenum) renderMode);
    glPolygonMode(GL_BACK, (GLenum) renderMode);

    {
        Matrix3f viewMat = observer.getOrientationf().conjugate().toRotationMatrix();

        // Remove objects from the render list that lie completely outside the
        // view frustum.
#ifdef USE_HDR
        maxBodyMag = maxBodyMagPrev;
        float starMaxMag = maxBodyMagPrev;
        notCulled = renderList.begin();
#else
        vector<RenderListEntry>::iterator notCulled = renderList.begin();
#endif
        for (vector<RenderListEntry>::iterator iter = renderList.begin();
             iter != renderList.end(); iter++)
        {
#ifdef USE_HDR
            switch (iter->renderableType)
            {
            case RenderListEntry::RenderableStar:
                break;
            default:
                *notCulled = *iter;
                notCulled++;
                continue;
            }
#endif
            Vector3f center = viewMat.transpose() * iter->position;

            bool convex = true;
            float radius = 1.0f;
            float cullRadius = 1.0f;
            float cloudHeight = 0.0f;

#ifndef USE_HDR
            switch (iter->renderableType)
            {
            case RenderListEntry::RenderableStar:
                radius = iter->star->getRadius();
                cullRadius = radius * (1.0f + CoronaHeight);
                break;

            case RenderListEntry::RenderableCometTail:
                radius = iter->radius;
                cullRadius = radius;
                convex = false;
                break;

            case RenderListEntry::RenderableBody:
                radius = iter->body->getBoundingRadius();
                if (iter->body->getRings() != NULL)
                {
                    radius = iter->body->getRings()->outerRadius;
                    convex = false;
                }

                if (!iter->body->isEllipsoid())
                    convex = false;

                cullRadius = radius;
                if (iter->body->getAtmosphere() != NULL)
                {
                    cullRadius += iter->body->getAtmosphere()->height;
                    cloudHeight = max(iter->body->getAtmosphere()->cloudHeight,
                                      iter->body->getAtmosphere()->mieScaleHeight * (float) -log(AtmosphereExtinctionThreshold));
                }
                break;

            case RenderListEntry::RenderableReferenceMark:
                radius = iter->radius;
                cullRadius = radius;
                convex = false;
                break;

            default:
                break;
            }
#else
            radius = iter->star->getRadius();
            cullRadius = radius * (1.0f + CoronaHeight);
#endif // USE_HDR

            // Test the object's bounding sphere against the view frustum
            if (frustum.testSphere(center, cullRadius) != Frustum::Outside)
            {
                float nearZ = center.norm() - radius;
#ifdef USE_HDR
                nearZ = -nearZ * nearZcoeff;
#else
                float maxSpan = (float) sqrt(square((float) windowWidth) +
                                             square((float) windowHeight));

                nearZ = -nearZ * (float) cos(degToRad(fov / 2)) *
                    ((float) windowHeight / maxSpan);
#endif
                if (nearZ > -MinNearPlaneDistance)
                    iter->nearZ = -max(MinNearPlaneDistance, radius / 2000.0f);
                else
                    iter->nearZ = nearZ;

                if (!convex)
                {
                    iter->farZ = center.z() - radius;
                    if (iter->farZ / iter->nearZ > MaxFarNearRatio * 0.5f)
                        iter->nearZ = iter->farZ / (MaxFarNearRatio * 0.5f);
                }
                else
                {
                    // Make the far plane as close as possible
                    float d = center.norm();

                    // Account for ellipsoidal objects
                    float eradius = radius;
                    if (iter->renderableType == RenderListEntry::RenderableBody)
                    {
                        float minSemiAxis = iter->body->getSemiAxes().minCoeff();
                        eradius *= minSemiAxis / radius;
                    }

                    if (d > eradius)
                    {
                        iter->farZ = iter->centerZ - iter->radius;
                    }
                    else
                    {
                        // We're inside the bounding sphere (and, if the planet
                        // is spherical, inside the planet.)
                        iter->farZ = iter->nearZ * 2.0f;
                    }

                    if (cloudHeight > 0.0f)
                    {
                        // If there's a cloud layer, we need to move the
                        // far plane out so that the clouds aren't clipped
                        float cloudLayerRadius = eradius + cloudHeight;
                        iter->farZ -= (float) sqrt(square(cloudLayerRadius) -
                                                   square(eradius));
                    }
                }

                *notCulled = *iter;
                notCulled++;
#ifdef USE_HDR
                if (iter->discSizeInPixels > 1.0f &&
                    iter->appMag < starMaxMag)
                {
                    starMaxMag = iter->appMag;
                    brightestStar = iter->star;
                    foundBrightestStar = true;
                }
#endif
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

        // Sort the annotations
        sort(depthSortedAnnotations.begin(), depthSortedAnnotations.end());

        // Sort the orbit paths
        sort(orbitPathList.begin(), orbitPathList.end());

        int nEntries = renderList.size();

#ifdef USE_HDR
        // Compute 1 eclipse between eye - closest body - brightest star
        // This prevents an eclipsed star from increasing exposure
        bool eyeNotEclipsed = true;
        closestBody = renderList.empty() ? renderList.end() : renderList.begin();
        if (foundClosestBody &&
            closestBody != renderList.end() &&
            closestBody->renderableType == RenderListEntry::RenderableBody &&
            closestBody->body && brightestStar)
        {
            const Body *body = closestBody->body;
            double scale = astro::microLightYearsToKilometers(1.0);
            Point3d posBody = body->getAstrocentricPosition(now);
            Point3d posStar;
            Point3d posEye = astrocentricPosition(observer.getPosition(), *brightestStar, now);

            if (body->getSystem() &&
                body->getSystem()->getStar() &&
                body->getSystem()->getStar() != brightestStar)
            {
                UniversalCoord center = body->getSystem()->getStar()->getPosition(now);
                Vec3d v = brightestStar->getPosition(now) - center;
                posStar = Point3d(v.x, v.y, v.z);
            }
            else
            {
                posStar = brightestStar->getPosition(now);
            }

            posStar.x /= scale;
            posStar.y /= scale;
            posStar.z /= scale;
            Vec3d lightToBodyDir = posBody - posStar;
            Vec3d bodyToEyeDir = posEye - posBody;

            if (lightToBodyDir * bodyToEyeDir > 0.0)
            {
                double dist = distance(posEye,
                                       Ray3d(posBody, lightToBodyDir));
                if (dist < body->getRadius())
                    eyeNotEclipsed = false;
            }
        }

        if (eyeNotEclipsed)
        {
            maxBodyMag = min(maxBodyMag, starMaxMag);
        }
#endif

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
        float prevNear = -1e12f;  // ~ 1 light year
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
                if (nIntervals == 0 || renderList[i].farZ >= depthPartitions[nIntervals - 1].nearZ)
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
        for (i = 0; i < (int) orbitPathList.size(); i++)
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
        clog << "nEntries: " << nEntries << ",   zNearest: " << zNearest << ",   prevNear: " << prevNear << "\n";
#endif

        // If the nearest distance wasn't set, nothing should appear
        // in the frontmost depth buffer interval (so we can set the near plane
        // of the front interval to whatever we want as long as it's less than
        // the far plane distance.
        if (zNearest == prevNear)
            zNearest = 0.0f;

        // Add one last interval for the span from 0 to the front of the
        // nearest object
        {
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
        }

        // If orbits are enabled, adjust the farthest partition so that it
        // can contain the orbit.
        if (!orbitPathList.empty())
        {
            depthPartitions[0].farZ = min(depthPartitions[0].farZ,
                                           orbitPathList[orbitPathList.size() - 1].centerZ -
                                           orbitPathList[orbitPathList.size() - 1].radius);
        }

        // We want to avoid overpartitioning the depth buffer. In this stage, we coalesce
        // partitions that have small spans in the depth buffer.
        // TODO: Implement this step!

        vector<Annotation>::iterator annotation = depthSortedAnnotations.begin();

        // Render everything that wasn't culled.
        float intervalSize = 1.0f / (float) max(1, nIntervals);
        i = nEntries - 1;
        for (int interval = 0; interval < nIntervals; interval++)
        {
            currentIntervalIndex = interval;
            beginObjectAnnotations();

            float nearPlaneDistance = -depthPartitions[interval].nearZ;
            float farPlaneDistance  = -depthPartitions[interval].farZ;

            // Set the depth range for this interval--each interval is allocated an
            // equal section of the depth buffer.
            glDepthRange(1.0f - (float) (interval + 1) * intervalSize,
                         1.0f - (float) interval * intervalSize);

            // Set up a perspective projection using the current interval's near and
            // far clip planes.
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            gluPerspective(fov,
                           (float) windowWidth / (float) windowHeight,
                           nearPlaneDistance,
                           farPlaneDistance);
            glMatrixMode(GL_MODELVIEW);

            Frustum intervalFrustum(degToRad(fov),
                                    (float) windowWidth / (float) windowHeight,
                                    -depthPartitions[interval].nearZ,
                                    -depthPartitions[interval].farZ);


#if DEBUG_COALESCE
            clog << "interval: " << interval <<
                    ", near: " << -depthPartitions[interval].nearZ <<
                    ", far: " << -depthPartitions[interval].farZ <<
                    "\n";
#endif
            int firstInInterval = i;

            // Render just the opaque objects in the first pass
            while (i >= 0 && renderList[i].farZ < depthPartitions[interval].nearZ)
            {
                // This interval should completely contain the item
                // Unless it's just a point?
                //assert(renderList[i].nearZ <= depthPartitions[interval].near);

#if DEBUG_COALESCE
                switch (renderList[i].renderableType)
                {
                case RenderListEntry::RenderableBody:
                    if (renderList[i].discSizeInPixels > 1)
                    {
                        clog << renderList[i].body->getName() << "\n";
                    }
                    else
                    {
                        clog << "point: " << renderList[i].body->getName() << "\n";
                    }
                    break;
                        
                case RenderListEntry::RenderableStar:
                    if (renderList[i].discSizeInPixels > 1)
                    {
                        clog << "Star\n";
                    }
                    else
                    {
                        clog << "point: " << "Star" << "\n";
                    }
                    break;
                        
                default:
                    break;
                }
#endif
                // Treat objects that are smaller than one pixel as transparent and render
                // them in the second pass.
                if (renderList[i].isOpaque && renderList[i].discSizeInPixels > 1.0f)
                    renderItem(renderList[i], observer, m_cameraOrientation, nearPlaneDistance, farPlaneDistance);

                i--;
            }

            // Render orbit paths
            if (!orbitPathList.empty())
            {
                glDisable(GL_LIGHTING);
                glDisable(GL_TEXTURE_2D);
                glEnable(GL_DEPTH_TEST);
                glDepthMask(GL_FALSE);
#ifdef USE_HDR
                glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
#else
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
                if ((renderFlags & ShowSmoothLines) != 0)
                {
                    enableSmoothLines();
                }

                // Scan through the list of orbits and render any that overlap this interval
                for (vector<OrbitPathListEntry>::const_iterator orbitIter = orbitPathList.begin();
                     orbitIter != orbitPathList.end(); orbitIter++)
                {
                    // Test for overlap
                    float nearZ = -orbitIter->centerZ - orbitIter->radius;
                    float farZ = -orbitIter->centerZ + orbitIter->radius;

                    // Don't render orbits when they're completely outside this
                    // depth interval.
                    if (nearZ < farPlaneDistance && farZ > nearPlaneDistance)
                    {
#ifdef DEBUG_COALESCE
                        switch (interval % 6)
                        {
                            case 0: glColor4f(1.0f, 0.0f, 0.0f, 1.0f); break;
                            case 1: glColor4f(1.0f, 1.0f, 0.0f, 1.0f); break;
                            case 2: glColor4f(0.0f, 1.0f, 0.0f, 1.0f); break;
                            case 3: glColor4f(0.0f, 1.0f, 1.0f, 1.0f); break;
                            case 4: glColor4f(0.0f, 0.0f, 1.0f, 1.0f); break;
                            case 5: glColor4f(1.0f, 0.0f, 1.0f, 1.0f); break;
                            default: glColor4f(1.0f, 1.0f, 1.0f, 1.0f); break;
                        }
#endif
                        orbitsRendered++;
                        renderOrbit(*orbitIter, now, m_cameraOrientation.cast<double>(), intervalFrustum, nearPlaneDistance, farPlaneDistance);

#if DEBUG_COALESCE
                        if (highlightObject.body() == orbitIter->body)
                        {
                            clog << "orbit, radius=" << orbitIter->radius << "\n";
                        }
#endif
                    }
                    else
                        orbitsSkipped++;
                }

                if ((renderFlags & ShowSmoothLines) != 0)
                    disableSmoothLines();
                glDepthMask(GL_FALSE);
            }

            // Render transparent objects in the second pass
            i = firstInInterval;
            while (i >= 0 && renderList[i].farZ < depthPartitions[interval].nearZ)
            {
                if (!renderList[i].isOpaque || renderList[i].discSizeInPixels <= 1.0f)
                    renderItem(renderList[i], observer, m_cameraOrientation, nearPlaneDistance, farPlaneDistance);

                i--;
            }

            // Render annotations in this interval
            if ((renderFlags & ShowSmoothLines) != 0)
                enableSmoothLines();
            annotation = renderSortedAnnotations(annotation, -depthPartitions[interval].nearZ, -depthPartitions[interval].farZ, FontNormal);
            endObjectAnnotations();
            if ((renderFlags & ShowSmoothLines) != 0)
                disableSmoothLines();
            glDisable(GL_DEPTH_TEST);
        }
#if 0
        // TODO: Debugging output for new orbit code; remove when development is complete
        clog << "orbits: " << orbitsRendered
             << ", skipped: " << orbitsSkipped
             << ", sections culled: " << sectionsCulled
             << ", nIntervals: " << nIntervals << "\n";
#endif
        orbitsRendered = 0;
        orbitsSkipped = 0;
        sectionsCulled = 0;

        // reset the depth range
        glDepthRange(0, 1);
    }
    
    renderForegroundAnnotations(FontNormal);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov,
                   (float) windowWidth / (float) windowHeight,
                   NEAR_DIST, FAR_DIST);
    glMatrixMode(GL_MODELVIEW);

    if (!selectionVisible && (renderFlags & ShowMarkers))
        renderSelectionPointer(observer, now, xfrustum, sel);

    // Pop camera orientation matrix
    glPopMatrix();

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_FILL);
    
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);

#if 0
    int errCode = glGetError();
    if (errCode != GL_NO_ERROR)
    {
        cout << "glError: " << (char*) gluErrorString(errCode) << '\n';
    }
#endif

#ifdef VIDEO_SYNC
    if (videoSync && glXWaitVideoSyncSGI != NULL)
    {
        unsigned int count;
        glXGetVideoSyncSGI(&count);
        glXWaitVideoSyncSGI(2, (count+1) & 1, &count);
    }
#endif
}


static void renderRingSystem(float innerRadius,
                             float outerRadius,
                             float beginAngle,
                             float endAngle,
                             unsigned int nSections)
{
    float angle = endAngle - beginAngle;

    glBegin(GL_QUAD_STRIP);
    for (unsigned int i = 0; i <= nSections; i++)
    {
        float t = (float) i / (float) nSections;
        float theta = beginAngle + t * angle;
        float s = (float) sin(theta);
        float c = (float) cos(theta);
        glTexCoord2f(0, 0.5f);
        glVertex3f(c * innerRadius, 0, s * innerRadius);
        glTexCoord2f(1, 0.5f);
        glVertex3f(c * outerRadius, 0, s * outerRadius);
    }
    glEnd();
}


// If the an object occupies a pixel or less of screen space, we don't
// render its mesh at all and just display a starlike point instead.
// Switching between the particle and mesh renderings of an object is
// jarring, however . . . so we'll blend in the particle view of the
// object to smooth things out, making it dimmer as the disc size exceeds the
// max disc size.
void Renderer::renderObjectAsPoint_nosprite(const Vector3f& position,
                                            float radius,
                                            float appMag,
                                            float _faintestMag,
                                            float discSizeInPixels,
                                            Color color,
                                            const Quaternionf& cameraOrientation,
                                            bool useHalos)
{
    float maxDiscSize = 1.0f;
    float maxBlendDiscSize = maxDiscSize + 3.0f;
    float discSize = 1.0f;

    if (discSizeInPixels < maxBlendDiscSize || useHalos)
    {
        float fade = 1.0f;
        if (discSizeInPixels > maxDiscSize)
        {
            fade = (maxBlendDiscSize - discSizeInPixels) /
                (maxBlendDiscSize - maxDiscSize - 1.0f);
            if (fade > 1)
                fade = 1;
        }

#ifdef USE_HDR
        float fieldCorr = 2.0f * FOV/(fov + FOV);
        float satPoint = saturationMagNight * (1.0f + fieldCorr * fieldCorr);
#else
        float satPoint = saturationMag;
#endif
        float a = (_faintestMag - appMag) * brightnessScale + brightnessBias;
        if (starStyle == ScaledDiscStars && a > 1.0f)
            discSize = min(discSize * (2.0f * a - 1.0f), maxDiscSize);
        a = clamp(a) * fade;

        // We scale up the particle by a factor of 1.6 (at fov = 45deg)
        // so that it's more visible--the texture we use has fuzzy edges,
        // and if we render it in just one pixel, it's likely to disappear.
        Matrix3f m = cameraOrientation.conjugate().toRotationMatrix();
        Vector3f center = position;

        // Offset the glare sprite so that it lies in front of the object
        Vector3f direction = center.normalized();

        // Position the sprite on the the line between the viewer and the
        // object, and on a plane normal to the view direction.
        center = center + direction * (radius / (m * Vector3f::UnitZ()).dot(direction));

        float centerZ = (m.transpose() * center).z();
        float size = discSize * pixelSize * 1.6f * centerZ / corrFac;

        Vector3f v0 = m * Vector3f(-1, -1, 0);
        Vector3f v1 = m * Vector3f( 1, -1, 0);
        Vector3f v2 = m * Vector3f( 1,  1, 0);
        Vector3f v3 = m * Vector3f(-1,  1, 0);

        glEnable(GL_DEPTH_TEST);

        starTex->bind();
        glColor(color, a);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 1);
        glVertex(center + (v0 * size));
        glTexCoord2f(1, 1);
        glVertex(center + (v1 * size));
        glTexCoord2f(1, 0);
        glVertex(center + (v2 * size));
        glTexCoord2f(0, 0);
        glVertex(center + (v3 * size));
        glEnd();

        // If the object is brighter than magnitude 1, add a halo around it to
        // make it appear more brilliant.  This is a hack to compensate for the
        // limited dynamic range of monitors.
        if (useHalos && appMag < satPoint)
        {
            float dist = center.norm();
            float s    = dist * 0.001f * (3 - (appMag - satPoint)) * 2;
            if (s > size * 3)
                size = s * 2.0f/(1.0f + FOV/fov);
            else
                size = size * 3;

            float realSize = discSizeInPixels * pixelSize * dist;
            if (size < realSize * 6)
                size = realSize * 6;

            a = GlareOpacity * clamp((appMag - satPoint) * -0.8f);
            gaussianGlareTex->bind();
            glColor(color, a);
            glBegin(GL_QUADS);
            glTexCoord2f(0, 1);
            glVertex(center + (v0 * size));
            glTexCoord2f(1, 1);
            glVertex(center + (v1 * size));
            glTexCoord2f(1, 0);
            glVertex(center + (v2 * size));
            glTexCoord2f(0, 0);
            glVertex(center + (v3 * size));
            glEnd();
        }

        glDisable(GL_DEPTH_TEST);
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
                                   float _faintestMag,
                                   float discSizeInPixels,
                                   Color color,
                                   const Quaternionf& cameraOrientation,
                                   bool useHalos,
                                   bool emissive)
{
    float maxDiscSize = (starStyle == ScaledDiscStars) ? MaxScaledDiscStarSize : 1.0f;
    float maxBlendDiscSize = maxDiscSize + 3.0f;

    bool useScaledDiscs = starStyle == ScaledDiscStars;

    if (discSizeInPixels < maxBlendDiscSize || useHalos)
    {
        float alpha = 1.0f;
        float fade = 1.0f;
        float size = BaseStarDiscSize;
#ifdef USE_HDR
        float fieldCorr = 2.0f * FOV/(fov + FOV);
        float satPoint = saturationMagNight * (1.0f + fieldCorr * fieldCorr);
        satPoint += brightPlus;
#else
        float satPoint = _faintestMag - (1.0f - brightnessBias) / brightnessScale;
#endif

        if (discSizeInPixels > maxDiscSize)
        {
            fade = (maxBlendDiscSize - discSizeInPixels) /
                (maxBlendDiscSize - maxDiscSize);
            if (fade > 1)
                fade = 1;
        }

        alpha = (_faintestMag - appMag) * brightnessScale * 2.0f + brightnessBias;

        float pointSize = size;
        float glareSize = 0.0f;
        float glareAlpha = 0.0f;
        if (useScaledDiscs)
        {
            if (alpha < 0.0f)
            {
                alpha = 0.0f;
            }
            else if (alpha > 1.0f)
            {
                float discScale = min(MaxScaledDiscStarSize, (float) pow(2.0f, 0.3f * (satPoint - appMag)));
                pointSize *= max(1.0f, discScale);

                glareAlpha = min(0.5f, discScale / 4.0f);
                if (discSizeInPixels > MaxScaledDiscStarSize)
                {
                    glareAlpha = min(glareAlpha,
                                     (MaxScaledDiscStarSize - discSizeInPixels) / MaxScaledDiscStarSize + 1.0f);
                }
                glareSize = pointSize * 3.0f;

                alpha = 1.0f;
            }
        }
        else
        {
            if (alpha < 0.0f)
            {
                alpha = 0.0f;
            }
            else if (alpha > 1.0f)
            {
                float discScale = min(100.0f, satPoint - appMag + 2.0f);
                glareAlpha = min(GlareOpacity, (discScale - 2.0f) / 4.0f);
                glareSize = pointSize * discScale * 2.0f ;
                if (emissive)
                    glareSize = max(glareSize, pointSize * discSizeInPixels * 3.0f);
            }
        }

        alpha *= fade;
        if (!emissive)
        {
            glareSize = max(glareSize, pointSize * discSizeInPixels * 3.0f);
            glareAlpha *= fade;
        }

        Matrix3f m = cameraOrientation.conjugate().toRotationMatrix();
        Vector3f center = position;

        // Offset the glare sprite so that it lies in front of the object
        Vector3f direction = center.normalized();

        // Position the sprite on the the line between the viewer and the
        // object, and on a plane normal to the view direction.
        center = center + direction * (radius / (m * Vector3f::UnitZ()).dot(direction));

        glEnable(GL_DEPTH_TEST);
#if !defined(NO_MAX_POINT_SIZE)
        // TODO: OpenGL appears to limit the max point size unless we
        // actually set up a shader that writes the pointsize values. To get
        // around this, we'll use billboards.
        Vector3f v0 = m * Vector3f(-1, -1, 0);
        Vector3f v1 = m * Vector3f( 1, -1, 0);
        Vector3f v2 = m * Vector3f( 1,  1, 0);
        Vector3f v3 = m * Vector3f(-1,  1, 0);
        float distanceAdjust = pixelSize * center.norm() * 0.5f;

        if (starStyle == PointStars)
        {
            glDisable(GL_TEXTURE_2D);
            glBegin(GL_POINTS);
            glColor(color, alpha);
            glVertex(center);
            glEnd();
            glEnable(GL_TEXTURE_2D);
        }
        else
        {
            gaussianDiscTex->bind();

            pointSize *= distanceAdjust;
            glBegin(GL_QUADS);
            glColor(color, alpha);
            glTexCoord2f(0, 1);
            glVertex(center + (v0 * pointSize));
            glTexCoord2f(1, 1);
            glVertex(center + (v1 * pointSize));
            glTexCoord2f(1, 0);
            glVertex(center + (v2 * pointSize));
            glTexCoord2f(0, 0);
            glVertex(center + (v3 * pointSize));
            glEnd();
        }

        // If the object is brighter than magnitude 1, add a halo around it to
        // make it appear more brilliant.  This is a hack to compensate for the
        // limited dynamic range of monitors.
        //
        // TODO: Stars look fine but planets look unrealistically bright
        // with halos.
        if (useHalos && glareAlpha > 0.0f)
        {
            gaussianGlareTex->bind();

            glareSize *= distanceAdjust;
            glBegin(GL_QUADS);
            glColor(color, glareAlpha);
            glTexCoord2f(0, 1);
            glVertex(center + (v0 * glareSize));
            glTexCoord2f(1, 1);
            glVertex(center + (v1 * glareSize));
            glTexCoord2f(1, 0);
            glVertex(center + (v2 * glareSize));
            glTexCoord2f(0, 0);
            glVertex(center + (v3 * glareSize));
            glEnd();
        }
#else
        // Disabled because of point size limits
        glEnable(GL_POINT_SPRITE_ARB);
        glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);

        gaussianDiscTex->bind();
        glColor(color, alpha);
        glPointSize(pointSize);
        glBegin(GL_POINTS);
        glVertex(center);
        glEnd();

        // If the object is brighter than magnitude 1, add a halo around it to
        // make it appear more brilliant.  This is a hack to compensate for the
        // limited dynamic range of monitors.
        //
        // TODO: Stars look fine but planets look unrealistically bright
        // with halos.
        if (useHalos && glareAlpha > 0.0f)
        {
            gaussianGlareTex->bind();

            glColor(color, glareAlpha);
            glPointSize(glareSize);
            glBegin(GL_POINTS);
            glVertex(center);
            glEnd();
        }

        glDisable(GL_POINT_SPRITE_ARB);
        glDisable(GL_DEPTH_TEST);
#endif // NO_MAX_POINT_SIZE
    }
}


static void renderBumpMappedMesh(const GLContext& context,
                                 Texture& baseTexture,
                                 Texture& bumpTexture,
                                 const Vector3f& lightDirection,
                                 const Quaternionf& orientation,
                                 Color ambientColor,
                                 const Frustum& frustum,
                                 float lod)
{
    // We're doing our own per-pixel lighting, so disable GL's lighting
    glDisable(GL_LIGHTING);

    // Render the base texture on the first pass . . .  The color
    // should have already been set up by the caller.
    g_lodSphere->render(context,
                        LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                        frustum, lod,
                        &baseTexture);

    // The 'default' light vector for the bump map is (0, 0, 1).  Determine
    // a rotation transformation that will move the sun direction to
    // this vector.
    Quaternionf lightOrientation;
    lightOrientation.setFromTwoVectors(Vector3f::UnitZ(), lightDirection);

    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_ZERO);

    // Set up the bump map with one directional light source
    SetupCombinersBumpMap(bumpTexture, *normalizationTex, ambientColor);

    // The second set texture coordinates will contain the light
    // direction in tangent space.  We'll generate the texture coordinates
    // from the surface normals using GL_NORMAL_MAP_EXT and then
    // use the texture matrix to rotate them into tangent space.
    // This method of generating tangent space light direction vectors
    // isn't as general as transforming the light direction by an
    // orthonormal basis for each mesh vertex, but it works well enough
    // for spheres illuminated by directional light sources.
    glActiveTextureARB(GL_TEXTURE1_ARB);

    // Set up GL_NORMAL_MAP_EXT texture coordinate generation.  This
    // mode is part of the cube map extension.
    glEnable(GL_TEXTURE_GEN_R);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
    glEnable(GL_TEXTURE_GEN_S);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);

    // Set up the texture transformation--the light direction and the
    // viewer orientation both need to be considered.
    glMatrixMode(GL_TEXTURE);
    glScalef(-1.0f, 1.0f, 1.0f);
    glRotate(lightOrientation * orientation.conjugate());
    glMatrixMode(GL_MODELVIEW);
    glActiveTextureARB(GL_TEXTURE0_ARB);

    g_lodSphere->render(context,
                        LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                        frustum, lod,
                        &bumpTexture);

    // Reset the second texture unit
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_TEXTURE_GEN_R);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);

    DisableCombiners();
    glDisable(GL_BLEND);
}


static void renderSmoothMesh(const GLContext& context,
                             Texture& baseTexture,
                             const Vector3f& lightDirection,
                             const Quaternionf& orientation,
                             Color ambientColor,
                             float lod,
                             const Frustum& frustum,
                             bool invert = false)
{
    Texture* textures[4];

    // We're doing our own per-pixel lighting, so disable GL's lighting
    glDisable(GL_LIGHTING);

    // The 'default' light vector for the bump map is (0, 0, 1).  Determine
    // a rotation transformation that will move the sun direction to
    // this vector.
    Quaternionf lightOrientation;
    lightOrientation.setFromTwoVectors(Vector3f::UnitZ(), lightDirection);

    SetupCombinersSmooth(baseTexture, *normalizationTex, ambientColor, invert);

    // The second set texture coordinates will contain the light
    // direction in tangent space.  We'll generate the texture coordinates
    // from the surface normals using GL_NORMAL_MAP_EXT and then
    // use the texture matrix to rotate them into tangent space.
    // This method of generating tangent space light direction vectors
    // isn't as general as transforming the light direction by an
    // orthonormal basis for each mesh vertex, but it works well enough
    // for spheres illuminated by directional light sources.
    glActiveTextureARB(GL_TEXTURE1_ARB);

    // Set up GL_NORMAL_MAP_EXT texture coordinate generation.  This
    // mode is part of the cube map extension.
    glEnable(GL_TEXTURE_GEN_R);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
    glEnable(GL_TEXTURE_GEN_S);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);

    // Set up the texture transformation--the light direction and the
    // viewer orientation both need to be considered.
    glMatrixMode(GL_TEXTURE);
    glRotate(lightOrientation * orientation.conjugate());
    glMatrixMode(GL_MODELVIEW);
    glActiveTextureARB(GL_TEXTURE0_ARB);

    textures[0] = &baseTexture;
    g_lodSphere->render(context,
                        LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                        frustum, lod,
                        textures, 1);

    // Reset the second texture unit
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_TEXTURE_GEN_R);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);

    DisableCombiners();
}


// Used to sort light sources in order of decreasing irradiance
struct LightIrradiancePredicate
{
    int unused;

    LightIrradiancePredicate() {};

    bool operator()(const DirectionalLight& l0,
                    const DirectionalLight& l1) const
    {
        return (l0.irradiance > l1.irradiance);
    }
};


void renderAtmosphere(const Atmosphere& atmosphere,
                      const Vector3f& center,
                      float radius,
                      const Vector3f& sunDirection,
                      Color ambientColor,
                      float fade,
                      bool lit)
{
    if (atmosphere.height == 0.0f)
        return;

    glDepthMask(GL_FALSE);

    Vector3f eyeVec = center;
    double centerDist = eyeVec.norm();

    Vector3f normal = eyeVec;
    normal = normal / (float) centerDist;

    float tangentLength = (float) sqrt(square(centerDist) - square(radius));
    float atmRadius = tangentLength * radius / (float) centerDist;
    float atmOffsetFromCenter = square(radius) / (float) centerDist;
    Vector3f atmCenter = center - atmOffsetFromCenter * normal;

    Vector3f uAxis, vAxis;
    if (abs(normal.x()) < abs(normal.y()) && abs(normal.x()) < abs(normal.z()))
    {
        uAxis = Vector3f::UnitX().cross(normal);
    }
    else if (abs(eyeVec.y()) < abs(normal.z()))
    {
        uAxis = Vector3f::UnitY().cross(normal);
    }
    else
    {
        uAxis = Vector3f::UnitZ().cross(normal);
    }
    uAxis.normalize();
    vAxis = uAxis.cross(normal);

    float height = atmosphere.height / radius;

    glBegin(GL_QUAD_STRIP);
    int divisions = 180;
    for (int i = 0; i <= divisions; i++)
    {
        float theta = (float) i / (float) divisions * 2 * (float) PI;
        Vector3f v = (float) cos(theta) * uAxis + (float) sin(theta) * vAxis;
        Vector3f base = atmCenter + v * atmRadius;
        Vector3f toCenter = base - center;

        float cosSunAngle = toCenter.dot(sunDirection) / radius;
        float brightness = 1.0f;
        float botColor[3];
        float topColor[3];
        botColor[0] = atmosphere.lowerColor.red();
        botColor[1] = atmosphere.lowerColor.green();
        botColor[2] = atmosphere.lowerColor.blue();
        topColor[0] = atmosphere.upperColor.red();
        topColor[1] = atmosphere.upperColor.green();
        topColor[2] = atmosphere.upperColor.blue();

        if (cosSunAngle < 0.2f && lit)
        {
            if (cosSunAngle < -0.2f)
            {
                brightness = 0;
            }
            else
            {
                float t = (0.2f + cosSunAngle) * 2.5f;
                brightness = t;
                botColor[0] = Mathf::lerp(t, 1.0f, botColor[0]);
                botColor[1] = Mathf::lerp(t, 0.3f, botColor[1]);
                botColor[2] = Mathf::lerp(t, 0.0f, botColor[2]);
                topColor[0] = Mathf::lerp(t, 1.0f, topColor[0]);
                topColor[1] = Mathf::lerp(t, 0.3f, topColor[1]);
                topColor[2] = Mathf::lerp(t, 0.0f, topColor[2]);
            }
        }

        glColor4f(botColor[0], botColor[1], botColor[2],
                  0.85f * fade * brightness + ambientColor.red());
        glVertex(base - toCenter * height * 0.05f);
        glColor4f(topColor[0], topColor[1], topColor[2], 0.0f);
        glVertex(base + toCenter * height);
    }
    glEnd();
}


template <typename T> static Matrix<T, 3, 1>
ellipsoidTangent(const Matrix<T, 3, 1>& recipSemiAxes,
                 const Matrix<T, 3, 1>& w,
                 const Matrix<T, 3, 1>& e,
                 const Matrix<T, 3, 1>& e_,
                 T ee)
{
    // We want to find t such that -E(1-t) + Wt is the direction of a ray
    // tangent to the ellipsoid.  A tangent ray will intersect the ellipsoid
    // at exactly one point.  Finding the intersection between a ray and an
    // ellipsoid ultimately requires using the quadratic formula, which has
    // one solution when the discriminant (b^2 - 4ac) is zero.  The code below
    // computes the value of t that results in a discriminant of zero.
    Matrix<T, 3, 1> w_ = w.cwise() * recipSemiAxes;//(w.x * recipSemiAxes.x, w.y * recipSemiAxes.y, w.z * recipSemiAxes.z);
    T ww = w_.dot(w_);
    T ew = w_.dot(e_);

    // Before elimination of terms:
    // double a =  4 * square(ee + ew) - 4 * (ee + 2 * ew + ww) * (ee - 1.0f);
    // double b = -8 * ee * (ee + ew)  - 4 * (-2 * (ee + ew) * (ee - 1.0f));
    // double c =  4 * ee * ee         - 4 * (ee * (ee - 1.0f));

    // Simplify the below expression and eliminate the ee^2 terms; this
    // prevents precision errors, as ee tends to be a very large value.
    //T a =  4 * square(ee + ew) - 4 * (ee + 2 * ew + ww) * (ee - 1);
    T a =  4 * (square(ew) - ee * ww + ee + 2 * ew + ww);
    T b = -8 * (ee + ew);
    T c =  4 * ee;

    T t = 0;
    T discriminant = b * b - 4 * a * c;

    if (discriminant < 0)
        t = (-b + (T) sqrt(-discriminant)) / (2 * a); // Bad!
    else
        t = (-b + (T) sqrt(discriminant)) / (2 * a);

    // V is the direction vector.  We now need the point of intersection,
    // which we obtain by solving the quadratic equation for the ray-ellipse
    // intersection.  Since we already know that the discriminant is zero,
    // the solution is just -b/2a
    Matrix<T, 3, 1> v = -e * (1 - t) + w * t;
    Matrix<T, 3, 1> v_ = v.cwise() * recipSemiAxes;
    T a1 = v_.dot(v_);
    T b1 = (T) 2 * v_.dot(e_);
    T t1 = -b1 / (2 * a1);

    return e + v * t1;
}


void Renderer::renderEllipsoidAtmosphere(const Atmosphere& atmosphere,
                                         const Vector3f& center,
                                         const Quaternionf& orientation,
                                         const Vector3f& semiAxes,
                                         const Vector3f& sunDirection,
                                         const LightingState& ls,
                                         float pixSize,
                                         bool lit)
{
    if (atmosphere.height == 0.0f)
        return;

    glDepthMask(GL_FALSE);

    // Gradually fade in the atmosphere if it's thickness on screen is just
    // over one pixel.
    float fade = clamp(pixSize - 2);

    Matrix3f rot = orientation.toRotationMatrix();
    Matrix3f irot = orientation.conjugate().toRotationMatrix();

    Vector3f eyePos = Vector3f::Zero();
    float radius = semiAxes.maxCoeff();
    Vector3f eyeVec = center - eyePos;
    eyeVec = rot * eyeVec;
    double centerDist = eyeVec.norm();

    float height = atmosphere.height / radius;
    Vector3f recipSemiAxes = semiAxes.cwise().inverse();

    Vector3f recipAtmSemiAxes = recipSemiAxes / (1.0f + height);
    // ellipDist is not the true distance from the surface unless the
    // planet is spherical.  Computing the true distance requires finding
    // the roots of a sixth degree polynomial, and isn't actually what we
    // want anyhow since the atmosphere region is just the planet ellipsoid
    // multiplied by a uniform scale factor.  The value that we do compute
    // is the distance to the surface along a line from the eye position to
    // the center of the ellipsoid.
    float ellipDist = (eyeVec.cwise() * recipSemiAxes).norm() - 1.0f;
    bool within = ellipDist < height;

    // Adjust the tesselation of the sky dome/ring based on distance from the
    // planet surface.
    int nSlices = MaxSkySlices;
    if (ellipDist < 0.25f)
    {
        nSlices = MinSkySlices + max(0, (int) ((ellipDist / 0.25f) * (MaxSkySlices - MinSkySlices)));
        nSlices &= ~1;
    }

    int nRings = min(1 + (int) pixSize / 5, 6);
    int nHorizonRings = nRings;
    if (within)
        nRings += 12;

    float horizonHeight = height;
    if (within)
    {
        if (ellipDist <= 0.0f)
            horizonHeight = 0.0f;
        else
            horizonHeight *= max((float) pow(ellipDist / height, 0.33f), 0.001f);
    }

    Vector3f e = -eyeVec;
    Vector3f e_ = e.cwise() * recipSemiAxes;//(e.x * recipSemiAxes.x, e.y * recipSemiAxes.y, e.z * recipSemiAxes.z);
    float ee = e_.dot(e_);

    // Compute the cosine of the altitude of the sun.  This is used to compute
    // the degree of sunset/sunrise coloration.
    float cosSunAltitude = 0.0f;
    {
        // Check for a sun either directly behind or in front of the viewer
        float cosSunAngle = (float) (sunDirection.dot(e) / centerDist);
        if (cosSunAngle < -1.0f + 1.0e-6f)
        {
            cosSunAltitude = 0.0f;
        }
        else if (cosSunAngle > 1.0f - 1.0e-6f)
        {
            cosSunAltitude = 0.0f;
        }
        else
        {
            Vector3f v = (rot * -sunDirection) * (float) centerDist;
            Vector3f tangentPoint = center +
                irot * ellipsoidTangent(recipSemiAxes,
                                        v,
                                        e, e_, ee);
            Vector3f tangentDir = (tangentPoint - eyePos).normalized();
            cosSunAltitude = sunDirection.dot(tangentDir);
        }
    }

    Vector3f normal = eyeVec;
    normal = normal / (float) centerDist;

    Vector3f uAxis, vAxis;
    if (abs(normal.x()) < abs(normal.y()) && abs(normal.x()) < abs(normal.z()))
    {
        uAxis = Vector3f::UnitX().cross(normal);
    }
    else if (abs(eyeVec.y()) < abs(normal.z()))
    {
        uAxis = Vector3f::UnitY().cross(normal);
    }
    else
    {
        uAxis = Vector3f::UnitZ().cross(normal);
    }
    uAxis.normalize();
    vAxis = uAxis.cross(normal);

    // Compute the contour of the ellipsoid
    int i;
    for (i = 0; i <= nSlices; i++)
    {
        // We want rays with an origin at the eye point and tangent to the the
        // ellipsoid.
        float theta = (float) i / (float) nSlices * 2 * (float) PI;
        Vector3f w = (float) cos(theta) * uAxis + (float) sin(theta) * vAxis;
        w = w * (float) centerDist;

        Vector3f toCenter = ellipsoidTangent(recipSemiAxes, w, e, e_, ee);
        skyContour[i].v = irot * toCenter;
        skyContour[i].centerDist = skyContour[i].v.norm();
        skyContour[i].eyeDir = skyContour[i].v + (center - eyePos);
        skyContour[i].eyeDist = skyContour[i].eyeDir.norm();
        skyContour[i].eyeDir.normalize();

        float skyCapDist = (float) sqrt(square(skyContour[i].eyeDist) +
                                        square(horizonHeight * radius));
        skyContour[i].cosSkyCapAltitude = skyContour[i].eyeDist /
            skyCapDist;
    }


    Vector3f botColor = atmosphere.lowerColor.toVector3();
    Vector3f topColor = atmosphere.upperColor.toVector3();
    Vector3f sunsetColor = atmosphere.sunsetColor.toVector3();
    
    if (within)
    {
        Vector3f skyColor = atmosphere.skyColor.toVector3();
        if (ellipDist < 0.0f)
            topColor = skyColor;
        else
            topColor = skyColor + (topColor - skyColor) * (ellipDist / height);
    }

    if (ls.nLights == 0 && lit)
    {
        botColor = topColor = sunsetColor = Vector3f::Zero();
    }

    Vector3f zenith = (skyContour[0].v + skyContour[nSlices / 2].v);
    zenith.normalize();
    zenith *= skyContour[0].centerDist * (1.0f + horizonHeight * 2.0f);

    float minOpacity = within ? (1.0f - ellipDist / height) * 0.75f : 0.0f;
    float sunset = cosSunAltitude < 0.9f ? 0.0f : (cosSunAltitude - 0.9f) * 10.0f;

    // Build the list of vertices
    SkyVertex* vtx = skyVertices;
    for (i = 0; i <= nRings; i++)
    {
        float h = min(1.0f, (float) i / (float) nHorizonRings);
        float hh = (float) sqrt(h);
        float u = i <= nHorizonRings ? 0.0f :
            (float) (i - nHorizonRings) / (float) (nRings - nHorizonRings);
        float r = Mathf::lerp(h, 1.0f - (horizonHeight * 0.05f), 1.0f + horizonHeight);
        float atten = 1.0f - hh;

        for (int j = 0; j < nSlices; j++)
        {
            Vector3f v;
            if (i <= nHorizonRings)
                v = skyContour[j].v * r;
            else
                v = (skyContour[j].v * (1.0f - u) + zenith * u) * r;
            Vector3f p = center + v;

            Vector3f viewDir = p.normalized();
            float cosSunAngle = viewDir.dot(sunDirection);
            float cosAltitude = viewDir.dot(skyContour[j].eyeDir);
            float brightness = 1.0f;
            float coloration = 0.0f;
            if (lit)
            {
                if (sunset > 0.0f && cosSunAngle > 0.7f && cosAltitude > 0.98f)
                {
                    coloration =  (1.0f / 0.30f) * (cosSunAngle - 0.70f);
                    coloration *= 50.0f * (cosAltitude - 0.98f);
                    coloration *= sunset;
                }

                cosSunAngle = skyContour[j].v.dot(sunDirection) / skyContour[j].centerDist;
                if (cosSunAngle > -0.2f)
                {
                    if (cosSunAngle < 0.3f)
                        brightness = (cosSunAngle + 0.2f) * 2.0f;
                    else
                        brightness = 1.0f;
                }
                else
                {
                    brightness = 0.0f;
                }
            }

            vtx->x = p.x();
            vtx->y = p.y();
            vtx->z = p.z();

            atten = 1.0f - hh;
            Vector3f color = (1.0f - hh) * botColor + hh * topColor;
            brightness *= minOpacity + (1.0f - minOpacity) * fade * atten;
            if (coloration != 0.0f)
                color = (1.0f - coloration) * color + coloration * sunsetColor;

#ifdef HDR_COMPRESS
            brightness *= 0.5f;
#endif
            Color(brightness * color.x(),
                  brightness * color.y(),
                  brightness * color.z(),
                  fade * (minOpacity + (1.0f - minOpacity)) * atten).get(vtx->color);
            vtx++;
        }
    }

    // Create the index list
    int index = 0;
    for (i = 0; i < nRings; i++)
    {
        int baseVertex = i * nSlices;
        for (int j = 0; j < nSlices; j++)
        {
            skyIndices[index++] = baseVertex + j;
            skyIndices[index++] = baseVertex + nSlices + j;
        }
        skyIndices[index++] = baseVertex;
        skyIndices[index++] = baseVertex + nSlices;
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(SkyVertex), &skyVertices[0].x);
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(SkyVertex),
                   static_cast<void*>(&skyVertices[0].color));

    for (i = 0; i < nRings; i++)
    {
        glDrawElements(GL_QUAD_STRIP,
                       (nSlices + 1) * 2,
                       GL_UNSIGNED_INT,
                       &skyIndices[(nSlices + 1) * 2 * i]);
    }

    glDisableClientState(GL_COLOR_ARRAY);
}


static void setupNightTextureCombine()
{
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_PRIMARY_COLOR_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_ONE_MINUS_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
}


static void setupBumpTexenv()
{
    // Set up the texenv_combine extension to do DOT3 bump mapping.
    // No support for ambient light yet.
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);

    // The primary color contains the light direction in surface
    // space, and texture0 is a normal map.  The lighting is
    // calculated by computing the dot product.
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_DOT3_RGB_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_PRIMARY_COLOR_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);

    // In the final stage, modulate the lighting value by the
    // base texture color.
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PREVIOUS_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);
    glEnable(GL_TEXTURE_2D);

    glActiveTextureARB(GL_TEXTURE0_ARB);
}


#if 0
static void setupBumpTexenvAmbient(Color ambientColor)
{
    float texenvConst[4];
    texenvConst[0] = ambientColor.red();
    texenvConst[1] = ambientColor.green();
    texenvConst[2] = ambientColor.blue();
    texenvConst[3] = ambientColor.alpha();

    // Set up the texenv_combine extension to do DOT3 bump mapping.
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);

    // The primary color contains the light direction in surface
    // space, and texture0 is a normal map.  The lighting is
    // calculated by computing the dot product.
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_DOT3_RGB_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_PRIMARY_COLOR_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);

    // Add in the ambient color
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, texenvConst);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_ADD);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_PREVIOUS_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_CONSTANT_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);
    glEnable(GL_TEXTURE_2D);

    // In the final stage, modulate the lighting value by the
    // base texture color.
    glActiveTextureARB(GL_TEXTURE2_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_PREVIOUS_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);
    glEnable(GL_TEXTURE_2D);

    glActiveTextureARB(GL_TEXTURE0_ARB);
}
#endif


static void setupTexenvAmbient(Color ambientColor)
{
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);

    // The primary color contains the light direction in surface
    // space, and texture0 is a normal map.  The lighting is
    // calculated by computing the dot product.
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, ambientColor.toVector4().data());
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_CONSTANT_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);
    glEnable(GL_TEXTURE_2D);
}


static void setupTexenvGlossMapAlpha()
{
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_PRIMARY_COLOR_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_ALPHA);

}


static void setLightParameters_VP(VertexProcessor& vproc,
                                  const LightingState& ls,
                                  Color materialDiffuse,
                                  Color materialSpecular)
{
    Vector3f diffuseColor = materialDiffuse.toVector3();
#ifdef HDR_COMPRESS
    Vector3f specularColor = materialSpecular.toVector3() * 0.5f;
#else
    Vector3f specularColor = materialSpecular.toVector3();
#endif
    for (unsigned int i = 0; i < ls.nLights; i++)
    {
        const DirectionalLight& light = ls.lights[i];

        Vector3f lightColor = light.color.toVector3() * light.irradiance;
        Vector3f diffuse = diffuseColor.cwise() * lightColor;
        Vector3f specular = specularColor.cwise() * lightColor;
        
        // Just handle two light sources for now
        if (i == 0)
        {
            vproc.parameter(vp::LightDirection0, ls.lights[0].direction_obj);
            vproc.parameter(vp::DiffuseColor0, diffuse);
            vproc.parameter(vp::SpecularColor0, specular);
        }
        else if (i == 1)
        {
            vproc.parameter(vp::LightDirection1, ls.lights[1].direction_obj);
            vproc.parameter(vp::DiffuseColor1, diffuse);
            vproc.parameter(vp::SpecularColor1, specular);
        }
    }
}


static void renderModelDefault(Geometry* geometry,
                               const RenderInfo& ri,
                               bool lit,
                               ResourceHandle texOverride)
{
    FixedFunctionRenderContext rc;
    Material m;

    rc.setLighting(lit);

    if (ri.baseTex == NULL)
    {
        glDisable(GL_TEXTURE_2D);
    }
    else
    {
        glEnable(GL_TEXTURE_2D);
        ri.baseTex->bind();
    }

    glColor(ri.color);

    if (ri.baseTex != NULL)
    {
        m.diffuse = Material::Color(ri.color.red(), ri.color.green(), ri.color.blue());
        m.specular = Material::Color(ri.specularColor.red(), ri.specularColor.green(), ri.specularColor.blue());
        m.specularPower = ri.specularPower;

        CelestiaTextureResource textureResource(texOverride);
        m.maps[Material::DiffuseMap] = &textureResource;

        rc.setMaterial(&m);
        rc.lock();
    }

    geometry->render(rc);
    if (geometry->usesTextureType(Material::EmissiveMap))
    {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        rc.setRenderPass(RenderContext::EmissivePass);
        rc.setMaterial(NULL);

        geometry->render(rc);

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    m.maps[Material::DiffuseMap] = NULL; // prevent Material destructor from deleting the texture resource

    // Reset the material
    float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float zero = 0.0f;
    glColor4fv(black);
    glMaterialfv(GL_FRONT, GL_EMISSION, black);
    glMaterialfv(GL_FRONT, GL_SPECULAR, black);
    glMaterialfv(GL_FRONT, GL_SHININESS, &zero);
}


static void renderSphereDefault(const RenderInfo& ri,
                                const Frustum& frustum,
                                bool lit,
                                const GLContext& context)
{
    if (lit)
        glEnable(GL_LIGHTING);
    else
        glDisable(GL_LIGHTING);

    if (ri.baseTex == NULL)
    {
        glDisable(GL_TEXTURE_2D);
    }
    else
    {
        glEnable(GL_TEXTURE_2D);
        ri.baseTex->bind();
    }

    glColor(ri.color);

    g_lodSphere->render(context,
                        LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                        frustum, ri.pixWidth,
                        ri.baseTex);
    if (ri.nightTex != NULL && ri.useTexEnvCombine)
    {
        ri.nightTex->bind();
#ifdef USE_HDR
#ifdef HDR_COMPRESS
        Color nightColor(ri.color.red()   * 2.f,
                         ri.color.green() * 2.f,
                         ri.color.blue()  * 2.f,
                         ri.nightLightScale);  // Modulate brightness using alpha
#else
        Color nightColor(ri.color.red(),
                         ri.color.green(),
                         ri.color.blue(),
                         ri.nightLightScale);  // Modulate brightness using alpha
#endif
        glColor(nightColor);
#endif
        setupNightTextureCombine();
        glEnable(GL_BLEND);
#ifdef USE_HDR
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
#else
        glBlendFunc(GL_ONE, GL_ONE);
#endif
        glAmbientLightColor(Color::Black); // Disable ambient light
        g_lodSphere->render(context,
                            LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                            frustum, ri.pixWidth,
                            ri.nightTex);
        glAmbientLightColor(ri.ambientColor);
#ifdef USE_HDR
        glColor(ri.color);
#endif
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    if (ri.overlayTex != NULL)
    {
        ri.overlayTex->bind();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        g_lodSphere->render(context,
                            LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                            frustum, ri.pixWidth,
                            ri.overlayTex);
        glBlendFunc(GL_ONE, GL_ONE);
    }
}


// DEPRECATED -- renderSphere_Combiners_VP should be used instead; only
// very old drivers don't support vertex programs.
static void renderSphere_Combiners(const RenderInfo& ri,
                                   const Frustum& frustum,
                                   const GLContext& context)
{
    glDisable(GL_LIGHTING);

    if (ri.baseTex == NULL)
    {
        glDisable(GL_TEXTURE_2D);
    }
    else
    {
        glEnable(GL_TEXTURE_2D);
        ri.baseTex->bind();
    }

    glColor(ri.color * ri.sunColor);

    // Don't use a normal map if it's a dxt5nm map--only the GLSL path
    // can handle them.
    if (ri.bumpTex != NULL &&
        (ri.bumpTex->getFormatOptions() & Texture::DXT5NormalMap) == 0)
    {
        renderBumpMappedMesh(context,
                             *(ri.baseTex),
                             *(ri.bumpTex),
                             ri.sunDir_eye,
                             ri.orientation,
                             ri.ambientColor,
                             frustum,
                             ri.pixWidth);
    }
    else if (ri.baseTex != NULL)
    {
        renderSmoothMesh(context,
                         *(ri.baseTex),
                         ri.sunDir_eye,
                         ri.orientation,
                         ri.ambientColor,
                         ri.pixWidth,
                         frustum);
    }
    else
    {
        glEnable(GL_LIGHTING);
        g_lodSphere->render(context, frustum, ri.pixWidth, NULL, 0);
    }

    if (ri.nightTex != NULL)
    {
        ri.nightTex->bind();
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        renderSmoothMesh(context,
                         *(ri.nightTex),
                         ri.sunDir_eye,
                         ri.orientation,
                         Color::Black,
                         ri.pixWidth,
                         frustum,
                         true);
    }

    if (ri.overlayTex != NULL)
    {
        glEnable(GL_LIGHTING);
        ri.overlayTex->bind();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        g_lodSphere->render(context,
                            LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                            frustum, ri.pixWidth,
                            ri.overlayTex);
#if 0
        renderSmoothMesh(context,
                         *(ri.overlayTex),
                         ri.sunDir_eye,
                         ri.orientation,
                         ri.ambientColor,
                         ri.pixWidth,
                         frustum);
#endif
        glBlendFunc(GL_ONE, GL_ONE);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}


static void renderSphere_DOT3_VP(const RenderInfo& ri,
                                 const LightingState& ls,
                                 const Frustum& frustum,
                                 const GLContext& context)
{
    VertexProcessor* vproc = context.getVertexProcessor();
    assert(vproc != NULL);

    if (ri.baseTex == NULL)
    {
        glDisable(GL_TEXTURE_2D);
    }
    else
    {
        glEnable(GL_TEXTURE_2D);
        ri.baseTex->bind();
    }

    vproc->enable();
    vproc->parameter(vp::EyePosition, ri.eyePos_obj);
    setLightParameters_VP(*vproc, ls, ri.color, ri.specularColor);

    Color ambient(ri.ambientColor * ri.color);
#ifdef USE_HDR
    ambient = ri.ambientColor;
#endif
    vproc->parameter(vp::AmbientColor, ambient);
    vproc->parameter(vp::SpecularExponent, 0.0f, 1.0f, 0.5f, ri.specularPower);

    // Don't use a normal map if it's a dxt5nm map--only the GLSL path
    // can handle them.
    if (ri.bumpTex != NULL &&
        (ri.bumpTex->getFormatOptions() & Texture::DXT5NormalMap) == 0 &&
        ri.baseTex != NULL)
    {
        // We don't yet handle the case where there's a bump map but no
        // base texture.
#ifdef HDR_COMPRESS
        vproc->use(vp::diffuseBumpHDR);
#else
        vproc->use(vp::diffuseBump);
#endif
        if (ri.ambientColor != Color::Black)
        {
            // If there's ambient light, we'll need to render in two passes:
            // one for the ambient light, and the second for light from the star.
            // We could do this in a single pass using three texture stages, but
            // this isn't won't work with hardware that only supported two
            // texture stages.

            // Render the base texture modulated by the ambient color
            setupTexenvAmbient(ambient);
            g_lodSphere->render(context,
                                LODSphereMesh::TexCoords0 | LODSphereMesh::VertexProgParams,
                                frustum, ri.pixWidth,
                                ri.baseTex);

            // Add the light from the sun
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            setupBumpTexenv();
            g_lodSphere->render(context,
                                LODSphereMesh::Normals | LODSphereMesh::Tangents |
                                LODSphereMesh::TexCoords0 | LODSphereMesh::VertexProgParams,
                                frustum, ri.pixWidth,
                                ri.bumpTex, ri.baseTex);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glDisable(GL_BLEND);
        }
        else
        {
            glActiveTextureARB(GL_TEXTURE1_ARB);
            ri.baseTex->bind();
            glActiveTextureARB(GL_TEXTURE0_ARB);
            ri.bumpTex->bind();
            setupBumpTexenv();
            g_lodSphere->render(context,
                                LODSphereMesh::Normals | LODSphereMesh::Tangents |
                                LODSphereMesh::TexCoords0 | LODSphereMesh::VertexProgParams,
                                frustum, ri.pixWidth,
                                ri.bumpTex, ri.baseTex);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        }
    }
    else
    {
        if (ls.nLights > 1)
            vproc->use(vp::diffuse_2light);
        else
            vproc->use(vp::diffuse);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        g_lodSphere->render(context,
                            LODSphereMesh::Normals | LODSphereMesh::TexCoords0 |
                            LODSphereMesh::VertexProgParams,
                            frustum, ri.pixWidth,
                            ri.baseTex);
    }

    // Render a specular pass; can't be done in one pass because
    // specular needs to be modulated with a gloss map.
    if (ri.specularColor != Color::Black)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        vproc->use(vp::glossMap);

        if (ri.glossTex != NULL)
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        else
            setupTexenvGlossMapAlpha();

        g_lodSphere->render(context,
                            LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                            frustum, ri.pixWidth,
                            ri.glossTex != NULL ? ri.glossTex : ri.baseTex);

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glDisable(GL_BLEND);
    }

    if (ri.nightTex != NULL)
    {
        ri.nightTex->bind();
#ifdef USE_HDR
        // Scale night light intensity
#ifdef HDR_COMPRESS
        Color nightColor0(ls.lights[0].color.red()  *ri.color.red()  *2.f,
                          ls.lights[0].color.green()*ri.color.green()*2.f,
                          ls.lights[0].color.blue() *ri.color.blue() *2.f,
                          ri.nightLightScale); // Modulate brightness with alpha
#else
        Color nightColor0(ls.lights[0].color.red()  *ri.color.red(),
                          ls.lights[0].color.green()*ri.color.green(),
                          ls.lights[0].color.blue() *ri.color.blue(),
                          ri.nightLightScale); // Modulate brightness with alpha
#endif
        vproc->parameter(vp::DiffuseColor0, nightColor0);
#endif
        if (ls.nLights > 1)
        {
#ifdef USE_HDR
#ifdef HDR_COMPRESS
            Color nightColor1(ls.lights[1].color.red()  *ri.color.red()  *2.f,
                              ls.lights[1].color.green()*ri.color.green()*2.f,
                              ls.lights[1].color.blue() *ri.color.blue() *2.f,
                              ri.nightLightScale);
#else
            Color nightColor1(ls.lights[1].color.red()  *ri.color.red(),
                              ls.lights[1].color.green()*ri.color.green(),
                              ls.lights[1].color.blue() *ri.color.blue(),
                              ri.nightLightScale);
#endif
            vproc->parameter(vp::DiffuseColor0, nightColor1);
#endif
#ifdef HDR_COMPRESS
            vproc->use(vp::nightLights_2lightHDR);
#else
            vproc->use(vp::nightLights_2light);
#endif
        }
        else
        {
#ifdef HDR_COMPRESS
            vproc->use(vp::nightLightsHDR);
#else
            vproc->use(vp::nightLights);
#endif
        }
#ifdef USE_HDR
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
#else
        setupNightTextureCombine();
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
#endif
        g_lodSphere->render(context,
                            LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                            frustum, ri.pixWidth,
                            ri.nightTex);
#ifdef USE_HDR
        vproc->parameter(vp::DiffuseColor0, ls.lights[0].color * ri.color);
        if (ls.nLights > 1)
            vproc->parameter(vp::DiffuseColor1, ls.lights[1].color * ri.color);
#endif
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    if (ri.overlayTex != NULL)
    {
        ri.overlayTex->bind();
        vproc->use(vp::diffuse);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        g_lodSphere->render(context,
                            LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                            frustum, ri.pixWidth,
                            ri.overlayTex);
        glBlendFunc(GL_ONE, GL_ONE);
    }

    vproc->disable();
}


static void renderSphere_Combiners_VP(const RenderInfo& ri,
                                      const LightingState& ls,
                                      const Frustum& frustum,
                                      const GLContext& context)
{
    Texture* textures[4];
    VertexProcessor* vproc = context.getVertexProcessor();
    assert(vproc != NULL);

    if (ri.baseTex == NULL)
    {
        glDisable(GL_TEXTURE_2D);
    }
    else
    {
        glEnable(GL_TEXTURE_2D);
        ri.baseTex->bind();
    }

    // Set up the fog parameters if the haze density is non-zero
    float hazeDensity = ri.hazeColor.alpha();
#ifdef HDR_COMPRESS
    Color hazeColor(ri.hazeColor.red()   * 0.5f,
                    ri.hazeColor.green() * 0.5f,
                    ri.hazeColor.blue()  * 0.5f,
                    hazeDensity);
#else
    Color hazeColor = ri.hazeColor;
#endif

    if (hazeDensity > 0.0f && !buggyVertexProgramEmulation)
    {
        glEnable(GL_FOG);
        float fogColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        fogColor[0] = hazeColor.red();
        fogColor[1] = hazeColor.green();
        fogColor[2] = hazeColor.blue();
        glFogfv(GL_FOG_COLOR, fogColor);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogf(GL_FOG_START, 0.0);
        glFogf(GL_FOG_END, 1.0f / hazeDensity);
    }

    vproc->enable();

    vproc->parameter(vp::EyePosition, ri.eyePos_obj);
    setLightParameters_VP(*vproc, ls, ri.color, ri.specularColor);

    vproc->parameter(vp::SpecularExponent, 0.0f, 1.0f, 0.5f, ri.specularPower);
    vproc->parameter(vp::AmbientColor, ri.ambientColor * ri.color);
    vproc->parameter(vp::HazeColor, hazeColor);

    // Don't use a normal map if it's a dxt5nm map--only the GLSL path
    // can handle them.
    if (ri.bumpTex != NULL &&
        (ri.bumpTex->getFormatOptions() & Texture::DXT5NormalMap) == 0)
    {
        if (hazeDensity > 0.0f)
        {
#ifdef HDR_COMPRESS
            vproc->use(vp::diffuseBumpHazeHDR);
#else
            vproc->use(vp::diffuseBumpHaze);
#endif
        }
        else
        {
#ifdef HDR_COMPRESS
            vproc->use(vp::diffuseBumpHDR);
#else
            vproc->use(vp::diffuseBump);
#endif
        }
        SetupCombinersDecalAndBumpMap(*(ri.bumpTex),
                                      ri.ambientColor * ri.color,
                                      ri.sunColor * ri.color);
        g_lodSphere->render(context,
                            LODSphereMesh::Normals | LODSphereMesh::Tangents |
                            LODSphereMesh::TexCoords0 | LODSphereMesh::VertexProgParams,
                            frustum, ri.pixWidth,
                            ri.baseTex, ri.bumpTex);
        DisableCombiners();

        // Render a specular pass
        if (ri.specularColor != Color::Black)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            glEnable(GL_COLOR_SUM_EXT);
            vproc->use(vp::specular);

            // Disable ambient and diffuse
            vproc->parameter(vp::AmbientColor, Color::Black);
            vproc->parameter(vp::DiffuseColor0, Color::Black);
            SetupCombinersGlossMap(ri.glossTex != NULL ? GL_TEXTURE0_ARB : 0);

            textures[0] = ri.glossTex != NULL ? ri.glossTex : ri.baseTex;
            g_lodSphere->render(context,
                                LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                                frustum, ri.pixWidth,
                                textures, 1);

            // re-enable diffuse
            vproc->parameter(vp::DiffuseColor0, ri.sunColor * ri.color);

            DisableCombiners();
            glDisable(GL_COLOR_SUM_EXT);
            glDisable(GL_BLEND);
        }
    }
    else if (ri.specularColor != Color::Black)
    {
        glEnable(GL_COLOR_SUM_EXT);
        if (ls.nLights > 1)
            vproc->use(vp::specular_2light);
        else
            vproc->use(vp::specular);
        SetupCombinersGlossMapWithFog(ri.glossTex != NULL ? GL_TEXTURE1_ARB : 0);
        unsigned int attributes = LODSphereMesh::Normals | LODSphereMesh::TexCoords0 |
            LODSphereMesh::VertexProgParams;
        g_lodSphere->render(context,
                            attributes, frustum, ri.pixWidth,
                            ri.baseTex, ri.glossTex);
        DisableCombiners();
        glDisable(GL_COLOR_SUM_EXT);
    }
    else
    {
        if (ls.nLights > 1)
        {
            if (hazeDensity > 0.0f)
                vproc->use(vp::diffuseHaze_2light);
            else
                vproc->use(vp::diffuse_2light);
        }
        else
        {
            if (hazeDensity > 0.0f)
                vproc->use(vp::diffuseHaze);
            else
                vproc->use(vp::diffuse);
        }

        g_lodSphere->render(context,
                            LODSphereMesh::Normals | LODSphereMesh::TexCoords0 |
                            LODSphereMesh::VertexProgParams,
                            frustum, ri.pixWidth,
                            ri.baseTex);
    }

    if (hazeDensity > 0.0f)
        glDisable(GL_FOG);

    if (ri.nightTex != NULL)
    {
        ri.nightTex->bind();
#ifdef USE_HDR
        // Scale night light intensity
#ifdef HDR_COMPRESS
        Color nightColor0(ls.lights[0].color.red()  *ri.color.red()  *2.f,
                          ls.lights[0].color.green()*ri.color.green()*2.f,
                          ls.lights[0].color.blue() *ri.color.blue() *2.f,
                          ri.nightLightScale); // Modulate brightness with alpha
#else
        Color nightColor0(ls.lights[0].color.red()  *ri.color.red(),
                          ls.lights[0].color.green()*ri.color.green(),
                          ls.lights[0].color.blue() *ri.color.blue(),
                          ri.nightLightScale); // Modulate brightness with alpha
#endif
        vproc->parameter(vp::DiffuseColor0, nightColor0);
#endif
        if (ls.nLights > 1)
        {
#ifdef USE_HDR
#ifdef HDR_COMPRESS
            Color nightColor1(ls.lights[1].color.red()  *ri.color.red()  *2.f,
                              ls.lights[1].color.green()*ri.color.green()*2.f,
                              ls.lights[1].color.blue() *ri.color.blue() *2.f,
                              ri.nightLightScale);
#else
            Color nightColor1(ls.lights[1].color.red()  *ri.color.red(),
                              ls.lights[1].color.green()*ri.color.green(),
                              ls.lights[1].color.blue() *ri.color.blue(),
                              ri.nightLightScale);
#endif
            vproc->parameter(vp::DiffuseColor0, nightColor1);
#endif
#ifdef HDR_COMPRESS
            vproc->use(vp::nightLights_2lightHDR);
#else
            vproc->use(vp::nightLights_2light);
#endif
        }
        else
        {
#ifdef HDR_COMPRESS
            vproc->use(vp::nightLightsHDR);
#else
            vproc->use(vp::nightLights);
#endif
        }
#ifdef USE_HDR
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
#else
        setupNightTextureCombine();
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
#endif
        g_lodSphere->render(context,
                            LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                            frustum, ri.pixWidth,
                            ri.nightTex);
#ifdef USE_HDR
        vproc->parameter(vp::DiffuseColor0, ri.sunColor * ri.color);
#endif
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    if (ri.overlayTex != NULL)
    {
        ri.overlayTex->bind();
        vproc->use(vp::diffuse);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        g_lodSphere->render(context,
                            LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                            frustum, ri.pixWidth,
                            ri.overlayTex);
        glBlendFunc(GL_ONE, GL_ONE);
    }

    vproc->disable();
}


// Render a planet sphere using both fragment and vertex programs
static void renderSphere_FP_VP(const RenderInfo& ri,
                               const Frustum& frustum,
                               const GLContext& context)
{
    Texture* textures[4];
    VertexProcessor* vproc = context.getVertexProcessor();
    FragmentProcessor* fproc = context.getFragmentProcessor();
    assert(vproc != NULL && fproc != NULL);

    if (ri.baseTex == NULL)
    {
        glDisable(GL_TEXTURE_2D);
    }
    else
    {
        glEnable(GL_TEXTURE_2D);
        ri.baseTex->bind();
    }

    // Compute the half angle vector required for specular lighting
    Vector3f halfAngle_obj = ri.eyeDir_obj + ri.sunDir_obj;
    if (halfAngle_obj.norm() != 0.0f)
        halfAngle_obj.normalize();

    // Set up the fog parameters if the haze density is non-zero
    float hazeDensity = ri.hazeColor.alpha();

    if (hazeDensity > 0.0f)
    {
        glEnable(GL_FOG);
        float fogColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        fogColor[0] = ri.hazeColor.red();
        fogColor[1] = ri.hazeColor.green();
        fogColor[2] = ri.hazeColor.blue();
        glFogfv(GL_FOG_COLOR, fogColor);
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogf(GL_FOG_START, 0.0);
        glFogf(GL_FOG_END, 1.0f / hazeDensity);
    }

    vproc->enable();

    vproc->parameter(vp::EyePosition, ri.eyePos_obj);
    vproc->parameter(vp::LightDirection0, ri.sunDir_obj);
    vproc->parameter(vp::DiffuseColor0, ri.sunColor * ri.color);
    vproc->parameter(vp::SpecularExponent, 0.0f, 1.0f, 0.5f, ri.specularPower);
    vproc->parameter(vp::SpecularColor0, ri.sunColor * ri.specularColor);
    vproc->parameter(vp::AmbientColor, ri.ambientColor * ri.color);
    vproc->parameter(vp::HazeColor, ri.hazeColor);

    if (ri.bumpTex != NULL)
    {
        fproc->enable();

        if (hazeDensity > 0.0f)
            vproc->use(vp::diffuseBumpHaze);
        else
            vproc->use(vp::diffuseBump);
        fproc->use(fp::texDiffuseBump);
        g_lodSphere->render(context,
                            LODSphereMesh::Normals | LODSphereMesh::Tangents |
                            LODSphereMesh::TexCoords0 | LODSphereMesh::VertexProgParams,
                            frustum, ri.pixWidth,
                            ri.baseTex, ri.bumpTex);
        fproc->disable();

        // Render a specular pass
        if (ri.specularColor != Color::Black)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            glEnable(GL_COLOR_SUM_EXT);
            vproc->use(vp::specular);

            // Disable ambient and diffuse
            vproc->parameter(vp::AmbientColor, Color::Black);
            vproc->parameter(vp::DiffuseColor0, Color::Black);
            SetupCombinersGlossMap(ri.glossTex != NULL ? GL_TEXTURE0_ARB : 0);

            textures[0] = ri.glossTex != NULL ? ri.glossTex : ri.baseTex;
            g_lodSphere->render(context,
                                LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                                frustum, ri.pixWidth,
                                textures, 1);

            // re-enable diffuse
            vproc->parameter(vp::DiffuseColor0, ri.sunColor * ri.color);

            DisableCombiners();
            glDisable(GL_COLOR_SUM_EXT);
            glDisable(GL_BLEND);
        }
    }
    else if (ri.specularColor != Color::Black)
    {
        fproc->enable();
        if (ri.glossTex == NULL)
        {
            vproc->use(vp::perFragmentSpecularAlpha);
            fproc->use(fp::texSpecularAlpha);
        }
        else
        {
            vproc->use(vp::perFragmentSpecular);
            fproc->use(fp::texSpecular);
        }
        fproc->parameter(fp::DiffuseColor, ri.sunColor * ri.color);
        fproc->parameter(fp::SunDirection, ri.sunDir_obj);
        fproc->parameter(fp::SpecularColor, ri.specularColor);
        fproc->parameter(fp::SpecularExponent, ri.specularPower, 0.0f, 0.0f, 0.0f);
        fproc->parameter(fp::AmbientColor, ri.ambientColor);

        unsigned int attributes = LODSphereMesh::Normals |
                                  LODSphereMesh::TexCoords0 |
                                  LODSphereMesh::VertexProgParams;
        g_lodSphere->render(context,
                            attributes, frustum, ri.pixWidth,
                            ri.baseTex, ri.glossTex);
        fproc->disable();
    }
    else
    {
        fproc->enable();
        if (hazeDensity > 0.0f)
            vproc->use(vp::diffuseHaze);
        else
            vproc->use(vp::diffuse);
        fproc->use(fp::texDiffuse);
        g_lodSphere->render(context,
                            LODSphereMesh::Normals | LODSphereMesh::TexCoords0 |
                            LODSphereMesh::VertexProgParams,
                            frustum, ri.pixWidth,
                            ri.baseTex);
        fproc->disable();
    }

    if (hazeDensity > 0.0f)
        glDisable(GL_FOG);

    if (ri.nightTex != NULL)
    {
        ri.nightTex->bind();
        vproc->use(vp::nightLights);
        setupNightTextureCombine();
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        g_lodSphere->render(context,
                            LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                            frustum, ri.pixWidth,
                            ri.nightTex);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    if (ri.overlayTex != NULL)
    {
        ri.overlayTex->bind();
        vproc->use(vp::diffuse);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        g_lodSphere->render(context,
                            LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                            frustum, ri.pixWidth,
                            ri.overlayTex);
        glBlendFunc(GL_ONE, GL_ONE);
    }

    vproc->disable();
}


static void renderShadowedGeometryDefault(Geometry* geometry,
                                          const RenderInfo& ri,
                                          const Frustum& frustum,
                                          const Vector4f& texGenS,
                                          const Vector4f& texGenT,
                                          const Vector3f& lightDir,
                                          bool useShadowMask,
                                          const GLContext& context)
{
    glEnable(GL_TEXTURE_GEN_S);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, texGenS.data());
    //texGenPlane(GL_S, GL_OBJECT_PLANE, sPlane);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, texGenT.data());

    if (useShadowMask)
    {
        glActiveTextureARB(GL_TEXTURE1_ARB);
        glEnable(GL_TEXTURE_GEN_S);
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        glTexGenfv(GL_S, GL_OBJECT_PLANE,
                    Vector4f(lightDir.x(), lightDir.y(), lightDir.z(), 0.5f).data());
        glActiveTextureARB(GL_TEXTURE0_ARB);
    }

    glColor4f(1, 1, 1, 1);
    glDisable(GL_LIGHTING);

    if (geometry == NULL)
    {
        g_lodSphere->render(context,
                            LODSphereMesh::Normals | LODSphereMesh::Multipass,
                            frustum, ri.pixWidth, NULL);
    }
    else
    {
        FixedFunctionRenderContext rc;
        geometry->render(rc);
    }
    glEnable(GL_LIGHTING);

    if (useShadowMask)
    {
        glActiveTextureARB(GL_TEXTURE1_ARB);
        glDisable(GL_TEXTURE_GEN_S);
        glActiveTextureARB(GL_TEXTURE0_ARB);
    }
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
}


static void renderShadowedGeometryVertexShader(const RenderInfo& ri,
                                               const Frustum& frustum,
                                               const Vector4f& texGenS,
                                               const Vector4f& texGenT,
                                               const Vector3f& lightDir,
                                               const GLContext& context)
{
    VertexProcessor* vproc = context.getVertexProcessor();
    assert(vproc != NULL);

    vproc->enable();
    vproc->parameter(vp::LightDirection0, lightDir);
    vproc->parameter(vp::TexGen_S, texGenS);
    vproc->parameter(vp::TexGen_T, texGenT);
    vproc->use(vp::shadowTexture);

    g_lodSphere->render(context,
                        LODSphereMesh::Normals | LODSphereMesh::Multipass, frustum,
                        ri.pixWidth, NULL);

    vproc->disable();
}


static void renderRings(RingSystem& rings,
                        RenderInfo& ri,
                        float planetRadius,
                        float planetOblateness,
                        unsigned int textureResolution,
                        bool renderShadow,
                        const GLContext& context,
                        unsigned int nSections)
{
    float inner = rings.innerRadius / planetRadius;
    float outer = rings.outerRadius / planetRadius;

    // Ring Illumination:
    // Since a ring system is composed of millions of individual
    // particles, it's not at all realistic to model it as a flat
    // Lambertian surface.  We'll approximate the llumination
    // function by assuming that the ring system contains Lambertian
    // particles, and that the brightness at some point in the ring
    // system is proportional to the illuminated fraction of a
    // particle there.  In fact, we'll simplify things further and
    // set the illumination of the entire ring system to the same
    // value, computing the illuminated fraction of a hypothetical
    // particle located at the center of the planet.  This
    // approximation breaks down when you get close to the planet.
    float ringIllumination = 0.0f;
    {
        float illumFraction = (1.0f + ri.eyeDir_obj.dot(ri.sunDir_obj)) / 2.0f;
        // Just use the illuminated fraction for now . . .
        ringIllumination = illumFraction;
    }

    GLContext::VertexPath vpath = context.getVertexPath();
    VertexProcessor* vproc = context.getVertexProcessor();
    FragmentProcessor* fproc = context.getFragmentProcessor();

    if (vproc != NULL)
    {
        vproc->enable();
        vproc->use(vp::ringIllum);
        vproc->parameter(vp::LightDirection0, ri.sunDir_obj);
        vproc->parameter(vp::DiffuseColor0, ri.sunColor * rings.color);
        vproc->parameter(vp::AmbientColor, ri.ambientColor * ri.color);
        vproc->parameter(vp::Constant0, Vector3f(0.0f, 0.5f, 1.0f));
    }

    // If we have multi-texture support, we'll use the second texture unit
    // to render the shadow of the planet on the rings.  This is a bit of
    // a hack, and assumes that the planet is ellipsoidal in shape,
    // and only works for a planet illuminated by a single sun where the
    // distance to the sun is very large relative to its diameter.
    if (renderShadow)
    {
        glActiveTextureARB(GL_TEXTURE1_ARB);
        glEnable(GL_TEXTURE_2D);
        shadowTex->bind();

        // Compute the projection vectors based on the sun direction.
        // I'm being a little careless here--if the sun direction lies
        // along the y-axis, this will fail.  It's unlikely that a
        // planet would ever orbit underneath its sun (an orbital
        // inclination of 90 degrees), but this should be made
        // more robust anyway.
        Vector3f axis = Vector3f::UnitY().cross(ri.sunDir_obj);
        float cosAngle = Vector3f::UnitY().dot(ri.sunDir_obj);
        axis.normalize();

        float sScale = 1.0f;
        float tScale = 1.0f;
        if (fproc == NULL)
        {
            // When fragment programs aren't used, we render shadows with circular
            // textures.  We scale up the texture slightly to account for the
            // padding pixels near the texture borders.
            sScale *= ShadowTextureScale;
            tScale *= ShadowTextureScale;
        }

        if (planetOblateness != 0.0f)
        {
            // For oblate planets, the size of the shadow volume will vary based
            // on the light direction.

            // A vertical slice of the planet is an ellipse
            float a = 1.0f;                          // semimajor axis
            float b = a * (1.0f - planetOblateness); // semiminor axis
            float ecc2 = 1.0f - (b * b) / (a * a);   // square of eccentricity

            // Calculate the radius of the ellipse at the incident angle of the
            // light on the ring plane + 90 degrees.
            float r = a * (float) sqrt((1.0f - ecc2) /
                                       (1.0f - ecc2 * square(cosAngle)));

            tScale *= a / r;
        }

        // The s axis is perpendicular to the shadow axis in the plane of the
        // of the rings, and the t axis completes the orthonormal basis.
        Vector3f sAxis = axis * 0.5f;
        Vector3f tAxis = axis.cross(ri.sunDir_obj) * 0.5f * tScale;

        Vector4f sPlane;
        Vector4f tPlane;
        sPlane.start<3>() = sAxis;
        sPlane[3] = 0.5f;
        tPlane.start<3>() = tAxis;
        tPlane[3] = 0.5f;

        if (vproc != NULL)
        {
            vproc->parameter(vp::TexGen_S, sPlane);
            vproc->parameter(vp::TexGen_T, tPlane);
        }
        else
        {
            glEnable(GL_TEXTURE_GEN_S);
            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
            glTexGenfv(GL_S, GL_EYE_PLANE, sPlane.data());
            glEnable(GL_TEXTURE_GEN_T);
            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
            glTexGenfv(GL_T, GL_EYE_PLANE, tPlane.data());
        }

        glActiveTextureARB(GL_TEXTURE0_ARB);

        if (fproc != NULL)
        {
            float r0 = 0.24f;
            float r1 = 0.25f;
            float bias = 1.0f / (1.0f - r1 / r0);
            float scale = -bias / r0;

            fproc->enable();
            fproc->use(fp::sphereShadowOnRings);
            fproc->parameter(fp::ShadowParams0, scale, bias, 0.0f, 0.0f);
            fproc->parameter(fp::AmbientColor, ri.ambientColor * ri.color);
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Texture* ringsTex = rings.texture.find(textureResolution);

    if (ringsTex != NULL)
    {
        glEnable(GL_TEXTURE_2D);
        ringsTex->bind();
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
    }

    // Perform our own lighting for the rings.
    // TODO: Don't forget about light source color (required when we
    // paying attention to star color.)
    if (vpath == GLContext::VPath_Basic)
    {
        glDisable(GL_LIGHTING);
        Vector3f litColor = rings.color.toVector3();
        litColor = litColor * ringIllumination + ri.ambientColor.toVector3();
        glColor4f(litColor.x(), litColor.y(), litColor.z(), 1.0f);
    }

    // This gets tricky . . .  we render the rings in two parts.  One
    // part is potentially shadowed by the planet, and we need to
    // render that part with the projected shadow texture enabled.
    // The other part isn't shadowed, but will appear so if we don't
    // first disable the shadow texture.  The problem is that the
    // shadow texture will affect anything along the line between the
    // sun and the planet, regardless of whether it's in front or
    // behind the planet.

    // Compute the angle of the sun projected on the ring plane
    float sunAngle = std::atan2(ri.sunDir_obj.z(), ri.sunDir_obj.x());

    // If there's a fragment program, it will handle the ambient term--make
    // sure that we don't add it both in the fragment and vertex programs.
    if (vproc != NULL && fproc != NULL)
        glAmbientLightColor(Color::Black);

    renderRingSystem(inner, outer,
                     (float) (sunAngle + PI / 2),
                     (float) (sunAngle + 3 * PI / 2),
                     nSections / 2);
    renderRingSystem(inner, outer,
                     (float) (sunAngle +  3 * PI / 2),
                     (float) (sunAngle + PI / 2),
                     nSections / 2);

    if (vproc != NULL && fproc != NULL)
        glAmbientLightColor(ri.ambientColor * ri.color);

    // Disable the second texture unit if it was used
    if (renderShadow)
    {
        glActiveTextureARB(GL_TEXTURE1_ARB);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
        glActiveTextureARB(GL_TEXTURE0_ARB);

        if (fproc != NULL)
            fproc->disable();
    }

    // Render the unshadowed side
    renderRingSystem(inner, outer,
                     (float) (sunAngle - PI / 2),
                     (float) (sunAngle + PI / 2),
                     nSections / 2);
    renderRingSystem(inner, outer,
                     (float) (sunAngle + PI / 2),
                     (float) (sunAngle - PI / 2),
                     nSections / 2);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    if (vproc != NULL)
        vproc->disable();
}


static void
renderEclipseShadows(Geometry* geometry,
                     LightingState::EclipseShadowVector& eclipseShadows,
                     RenderInfo& ri,
                     float planetRadius,
                     Quaternionf& planetOrientation,
                     Frustum& viewFrustum,
                     const GLContext& context)
{
    Matrix3f planetTransform = planetOrientation.toRotationMatrix();

    // Eclipse shadows on mesh objects are only supported in
    // the OpenGL 2.0 path.
    if (geometry != NULL)
        return;

    for (LightingState::EclipseShadowVector::iterator iter = eclipseShadows.begin();
         iter != eclipseShadows.end(); iter++)
    {
        EclipseShadow shadow = *iter;

        // Determine which eclipse shadow texture to use.  This is only
        // a very rough approximation to reality.  Since there are an
        // infinite number of possible eclipse volumes, what we should be
        // doing is generating the eclipse textures on the fly using
        // render-to-texture.  But for now, we'll just choose from a fixed
        // set of eclipse shadow textures based on the relative size of
        // the umbra and penumbra.
        Texture* eclipseTex = NULL;
        float umbra = shadow.umbraRadius / shadow.penumbraRadius;
        if (umbra < 0.1f)
            eclipseTex = eclipseShadowTextures[0];
        else if (umbra < 0.35f)
            eclipseTex = eclipseShadowTextures[1];
        else if (umbra < 0.6f)
            eclipseTex = eclipseShadowTextures[2];
        else if (umbra < 0.9f)
            eclipseTex = eclipseShadowTextures[3];
        else
            eclipseTex = shadowTex;

        // Compute the transformation to use for generating texture
        // coordinates from the object vertices.
        Vector3f origin = planetTransform * shadow.origin;
        Vector3f dir    = planetTransform * shadow.direction;
        float scale = planetRadius / shadow.penumbraRadius;
        Quaternionf shadowRotation;
        shadowRotation.setFromTwoVectors(Vector3f::UnitY(), dir);
        Matrix3f m = shadowRotation.toRotationMatrix();

        Vector3f sAxis = m * Vector3f::UnitX() * (0.5f * scale);
        Vector3f tAxis = m * Vector3f::UnitZ() * (0.5f * scale);

        Vector4f texGenS;
        Vector4f texGenT;
        texGenS.start<3>() = sAxis;
        texGenT.start<3>() = tAxis;
        texGenS[3] = -origin.dot(sAxis) / planetRadius + 0.5f;
        texGenT[3] = -origin.dot(tAxis) / planetRadius + 0.5f;

        // TODO: Multiple eclipse shadows should be rendered in a single
        // pass using multitexture.
        if (eclipseTex != NULL)
            eclipseTex->bind();
        // shadowMaskTexture->bind();
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);

        // If the ambient light level is greater than zero, reduce the
        // darkness of the shadows.
        if (ri.useTexEnvCombine)
        {
            float color[4] = { ri.ambientColor.red(), ri.ambientColor.green(),
                               ri.ambientColor.blue(), 1.0f };
            glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_CONSTANT_EXT);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_TEXTURE);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_ADD);

            // The second texture unit has the shadow 'mask'
            glActiveTextureARB(GL_TEXTURE1_ARB);
            glEnable(GL_TEXTURE_2D);
            shadowMaskTexture->bind();
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_ADD);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_PREVIOUS_EXT);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_TEXTURE);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);
            glActiveTextureARB(GL_TEXTURE0_ARB);
        }

        // Since invariance between nVidia's vertex programs and the
        // standard transformation pipeline isn't guaranteed, we have to
        // make sure to use the same transformation engine on subsequent
        // rendering passes as we did on the initial one.
        if (context.getVertexPath() != GLContext::VPath_Basic && geometry == NULL)
        {
            renderShadowedGeometryVertexShader(ri, viewFrustum,
                                               texGenS, texGenT,
                                               dir,
                                               context);
        }
        else
        {
            renderShadowedGeometryDefault(geometry, ri, viewFrustum,
                                          texGenS, texGenT,
                                          dir,
                                          ri.useTexEnvCombine,
                                          context);
        }

        if (ri.useTexEnvCombine)
        {
            // Disable second texture unit
            glActiveTextureARB(GL_TEXTURE1_ARB);
            glDisable(GL_TEXTURE_2D);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glActiveTextureARB(GL_TEXTURE0_ARB);

            float color[4] = { 0, 0, 0, 0 };
            glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDisable(GL_BLEND);
    }
}


static void
renderEclipseShadows_Shaders(Geometry* geometry,
                             LightingState::EclipseShadowVector& eclipseShadows,
                             RenderInfo& ri,
                             float planetRadius,
                             const Quaternionf& planetOrientation,
                             Frustum& viewFrustum,
                             const GLContext& context)
{
    Matrix3f planetTransform = planetOrientation.toRotationMatrix();
    
    // Eclipse shadows on mesh objects are only implemented in the GLSL path
    if (geometry != NULL)
        return;

    glEnable(GL_TEXTURE_2D);
    penumbraFunctionTexture->bind();

    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);

    Vector4f texGenS[4];
    Vector4f texGenT[4];
    Vector4f shadowParams[4];

    int n = 0;
    for (LightingState::EclipseShadowVector::iterator iter = eclipseShadows.begin();
         iter != eclipseShadows.end() && n < 4; iter++, n++)
    {
        EclipseShadow shadow = *iter;

        float R2 = 0.25f;
        float umbra = shadow.umbraRadius / shadow.penumbraRadius;
        umbra = umbra * umbra;
        if (umbra < 0.0001f)
            umbra = 0.0001f;
        else if (umbra > 0.99f)
            umbra = 0.99f;

        float umbraRadius = R2 * umbra;
        float penumbraRadius = R2;
        float shadowBias = 1.0f / (1.0f - penumbraRadius / umbraRadius);
        float shadowScale = -shadowBias / umbraRadius;

        shadowParams[n][0] = shadowScale;
        shadowParams[n][1] = shadowBias;
        shadowParams[n][2] = 0.0f;
        shadowParams[n][3] = 0.0f;

        // Compute the transformation to use for generating texture
        // coordinates from the object vertices.
        Vector3f origin = planetTransform * shadow.origin;
        Vector3f dir    = planetTransform * shadow.direction;
        float scale = planetRadius / shadow.penumbraRadius;
        Quaternionf shadowRotation;
        shadowRotation.setFromTwoVectors(Vector3f::UnitY(), dir);
        Matrix3f m = shadowRotation.toRotationMatrix();

        Vector3f sAxis = m * Vector3f::UnitX() * (0.5f * scale);
        Vector3f tAxis = m * Vector3f::UnitZ() * (0.5f * scale);

        texGenS[n].start<3>() = sAxis;
        texGenT[n].start<3>() = tAxis;
        texGenS[n][3] = -origin.dot(sAxis) / planetRadius + 0.5f;
        texGenT[n][3] = -origin.dot(tAxis) / planetRadius + 0.5f;
    }


    VertexProcessor* vproc = context.getVertexProcessor();
    FragmentProcessor* fproc = context.getFragmentProcessor();

    vproc->enable();
    vproc->use(vp::multiShadow);

    fproc->enable();
    if (n == 1)
        fproc->use(fp::eclipseShadow1);
    else
        fproc->use(fp::eclipseShadow2);

    fproc->parameter(fp::ShadowParams0, shadowParams[0]);
    vproc->parameter(vp::TexGen_S, texGenS[0]);
    vproc->parameter(vp::TexGen_T, texGenT[0]);
    if (n >= 2)
    {
        fproc->parameter(fp::ShadowParams1, shadowParams[1]);
        vproc->parameter(vp::TexGen_S2, texGenS[1]);
        vproc->parameter(vp::TexGen_T2, texGenT[1]);
    }
    if (n >= 3)
    {
        //fproc->parameter(fp::ShadowParams2, shadowParams[2]);
        vproc->parameter(vp::TexGen_S3, texGenS[2]);
        vproc->parameter(vp::TexGen_T3, texGenT[2]);
    }
    if (n >= 4)
    {
        //fproc->parameter(fp::ShadowParams3, shadowParams[3]);
        vproc->parameter(vp::TexGen_S4, texGenS[3]);
        vproc->parameter(vp::TexGen_T4, texGenT[3]);
    }

    //vproc->parameter(vp::LightDirection0, lightDir);

    g_lodSphere->render(context,
                        LODSphereMesh::Normals | LODSphereMesh::Multipass,
                        viewFrustum,
                        ri.pixWidth, NULL);

    vproc->disable();
    fproc->disable();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDisable(GL_BLEND);
}


static void
renderRingShadowsVS(Geometry* /*geometry*/,           //TODO: Remove unused parameters??
                    const RingSystem& rings,
                    const Vector3f& /*sunDir*/,
                    RenderInfo& ri,
                    float planetRadius,
                    float /*oblateness*/,
                    Matrix4f& /*planetMat*/,
                    Frustum& viewFrustum,
                    const GLContext& context)
{
    // Compute the transformation to use for generating texture
    // coordinates from the object vertices.
    float ringWidth = rings.outerRadius - rings.innerRadius;
    float s = ri.sunDir_obj.y();
    float scale = (abs(s) < 0.001f) ? 1000.0f : 1.0f / s;

    if (abs(s) > 1.0f - 1.0e-4f)
    {
        // Planet is illuminated almost directly from above, so
        // no ring shadow will be cast on the planet.  Conveniently
        // avoids some potential division by zero when ray-casting.
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);

    // If the ambient light level is greater than zero, reduce the
    // darkness of the shadows.
    float color[4] = { ri.ambientColor.red(), ri.ambientColor.green(),
                       ri.ambientColor.blue(), 1.0f };
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_CONSTANT_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_ADD);

    // Tweak the texture--set clamp to border and a border color with
    // a zero alpha.  If a graphics card doesn't support clamp to border,
    // it doesn't get to play.  It's possible to get reasonable behavior
    // by turning off mipmaps and assuming transparent rows of pixels for
    // the top and bottom of the ring textures . . . maybe later.
    float bc[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bc);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_ARB);

    // Ring shadows look strange if they're always completely black.  Vary
    // the darkness of the shadow based on the angle between the sun and the
    // ring plane.  There's some justification for this--the larger the angle
    // between the sun and the ring plane (normal), the more ring material
    // there is to travel through.
    //float alpha = (1.0f - abs(ri.sunDir_obj.y)) * 1.0f;
    // ...but, images from Cassini are showing very dark ring shadows, so we'll
    // go with that.
    float alpha = 1.0f;

    VertexProcessor* vproc = context.getVertexProcessor();
    assert(vproc != NULL);

    vproc->enable();
    vproc->use(vp::ringShadow);
    vproc->parameter(vp::LightDirection0, ri.sunDir_obj);
    vproc->parameter(vp::DiffuseColor0, 1, 1, 1, alpha); // color = white
    vproc->parameter(vp::TexGen_S,
                     rings.innerRadius / planetRadius,
                     1.0f / (ringWidth / planetRadius),
                     0.0f, 0.5f);
    vproc->parameter(vp::TexGen_T, scale, 0, 0, 0);
    g_lodSphere->render(context, LODSphereMesh::Multipass,
                        viewFrustum, ri.pixWidth, NULL);
    vproc->disable();

    // Restore the texture combiners
    if (ri.useTexEnvCombine)
    {
        float color[4] = { 0, 0, 0, 0 };
        glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDisable(GL_BLEND);
}


void Renderer::renderLocations(const Body& body,
                               const Vector3d& bodyPosition,
                               const Quaterniond& bodyOrientation)
{
    const vector<Location*>* locations = body.getLocations();
    if (locations == NULL)
        return;
    
    Vector3f semiAxes = body.getSemiAxes();
    
    float nearDist = getNearPlaneDistance();
    double boundingRadius = semiAxes.maxCoeff();

    Vector3d bodyCenter = bodyPosition;
    Vector3d viewRayOrigin = bodyOrientation * -bodyCenter;
    double labelOffset = 0.0001;

    Vector3f vn  = getCameraOrientation().conjugate() * -Vector3f::UnitZ();
    Vector3d viewNormal = vn.cast<double>();
    
    Ellipsoidd bodyEllipsoid(semiAxes.cast<double>());
    
    Matrix3d bodyMatrix = bodyOrientation.conjugate().toRotationMatrix();
    
    for (vector<Location*>::const_iterator iter = locations->begin();
         iter != locations->end(); iter++)
    {
        const Location& location = **iter;
        
        if (location.getFeatureType() & locationFilter)
        {
            // Get the position of the location with respect to the planet center
            Vector3f ppos = location.getPosition();
            
            // Compute the bodycentric position of the location
            Vector3d locPos = ppos.cast<double>();
            
            // Get the planetocentric position of the label.  Add a slight scale factor
            // to keep the point from being exactly on the surface.
            Vector3d pcLabelPos = locPos * (1.0 + labelOffset);
            
            // Get the camera space label position
            Vector3d labelPos = bodyCenter + bodyMatrix * locPos;
            
            float effSize = location.getImportance();
            if (effSize < 0.0f)
                effSize = location.getSize();
            
            float pixSize = effSize / (float) (labelPos.norm() * pixelSize);
            
            if (pixSize > minFeatureSize && labelPos.dot(viewNormal) > 0.0)
            {
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
                Ray3d testRay(viewRayOrigin, pcLabelPos - viewRayOrigin);
                bool hit = testIntersection(testRay, bodyEllipsoid, t);

                if (!hit || t >= 1.0)
                {                    
                    // Calculate the intersection of the eye-to-label ray with the plane perpendicular to
                    // the view normal that touches the front of the object's bounding sphere
                    double planetZ = viewNormal.dot(bodyCenter) - boundingRadius;
                    if (planetZ < -nearDist * 1.001)
                        planetZ = -nearDist * 1.001;
                    double z = viewNormal.dot(labelPos);
                    labelPos *= planetZ / z;
                                        
                    uint32 featureType = location.getFeatureType();
                    MarkerRepresentation* locationMarker = NULL;
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

                    Color labelColor = location.isLabelColorOverridden() ? location.getLabelColor() : LocationLabelColor;
                    addObjectAnnotation(locationMarker,
                                        location.getName(true),
                                        labelColor,
                                        labelPos.cast<float>());
                }
            }
        }
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
    float cosTheta = (float) (radius / d);
    if (cosTheta > 0.999f)
        cosTheta = 0.999f;

    // Phi is the angle between the light vector and receiver-to-reflector vector.
    // cos(phi) is thus the illumination at the sub-point. The horizon points are
    // at phi+theta and phi-theta.
    float cosPhi = (float) (toSun.dot(toObject) / (d * toSun.norm()));

    // Use a trigonometric identity to compute cos(phi +/- theta):
    //   cos(phi + theta) = cos(phi) * cos(theta) - sin(phi) * sin(theta)

    // s = sin(phi) * sin(theta)
    float s = (float) sqrt((1.0f - cosPhi * cosPhi) * (1.0f - cosTheta * cosTheta));
    
    float cosPhi1 = cosPhi * cosTheta - s;  // cos(phi + theta)
    float cosPhi2 = cosPhi * cosTheta + s;  // cos(phi - theta)

    // Calculate a weighted average of illumination at the three points
    return (2.0f * max(cosPhi, 0.0f) + max(cosPhi1, 0.0f) + max(cosPhi2, 0.0f)) * 0.25f;
}


static void
setupObjectLighting(const vector<LightSource>& suns,
                    const vector<SecondaryIlluminator>& secondaryIlluminators,
                    const Quaternionf& objOrientation,
                    const Vector3f& objScale,
                    const Vector3f& objPosition_eye,
                    bool isNormalized,
#ifdef USE_HDR
                    const float faintestMag,
                    const float saturationMag,
                    const float appMag,
#endif
                    LightingState& ls)
{
    unsigned int nLights = min(MaxLights, (unsigned int) suns.size());
    if (nLights == 0)
        return;

#ifdef USE_HDR
    float exposureFactor = (faintestMag - appMag)/(faintestMag - saturationMag + 0.001f);
#endif

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
        unsigned int maxIrrSource = 0;
        Vector3d objpos = objPosition_eye.cast<double>();

        // Only account for light from the brightest secondary source
        for (vector<SecondaryIlluminator>::const_iterator iter = secondaryIlluminators.begin();
             iter != secondaryIlluminators.end(); iter++)
        {
            Vector3d toIllum = iter->position_v - objpos;  // reflector-to-object vector
            float distSquared = (float) toIllum.squaredNorm() / square(iter->radius);

            if (distSquared > 0.01f)
            {
                // Irradiance falls off with distance^2
                float irr = iter->reflectedIrradiance / distSquared;

                // Phase effects will always leave the irradiance unaffected or reduce it;
                // don't bother calculating them if we've already found a brighter secondary
                // source.
                if (irr > maxIrr)
                {
                    // Account for the phase
                    Vector3d toSun = objpos - suns[0].position;
                    irr *= estimateReflectedLightFraction(toSun, toIllum, iter->radius);              
                    if (irr > maxIrr)
                    {
                        maxIrr = irr;
                        maxIrrSource = iter - secondaryIlluminators.begin();
                    }
                }
            }
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
        sort(ls.lights, ls.lights + nLights, LightIrradiancePredicate());
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
    float gamma = (float) (log(minDisplayableValue) / log(minVisibleFraction));
    float minVisibleIrradiance = minVisibleFraction * totalIrradiance;

    Matrix3f m = objOrientation.toRotationMatrix();

    // Gamma scale and normalize the light sources; cull light sources that
    // aren't bright enough to contribute the final pixels rendered into the
    // frame buffer.
    ls.nLights = 0;
    for (i = 0; i < nLights && ls.lights[i].irradiance > minVisibleIrradiance; i++)
    {
#ifdef USE_HDR
        ls.lights[i].irradiance *= exposureFactor / totalIrradiance;
#else
        ls.lights[i].irradiance =
            (float) pow(ls.lights[i].irradiance / totalIrradiance, gamma);
#endif

        // Compute the direction of the light in object space
        ls.lights[i].direction_obj = m * ls.lights[i].direction_eye;

        ls.nLights++;
    }

    Matrix3f invScale = objScale.cwise().inverse().asDiagonal();
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
                            double now,
                            const Quaternionf& cameraOrientation,
                            float nearPlaneDistance,
                            float farPlaneDistance,
                            RenderProperties& obj,
                            const LightingState& ls)
{
    RenderInfo ri;

    float altitude = distance - obj.radius;
    float discSizeInPixels = obj.radius / (max(nearPlaneDistance, altitude) * pixelSize);

    ri.sunDir_eye = Vector3f::UnitY();
    ri.sunDir_obj = Vector3f::UnitY();
    ri.sunColor = Color(0.0f, 0.0f, 0.0f);
    if (ls.nLights > 0)
    {
        ri.sunDir_eye = ls.lights[0].direction_eye;
        ri.sunDir_obj = ls.lights[0].direction_obj;
        ri.sunColor   = ls.lights[0].color;// * ls.lights[0].intensity;
    }

    // Enable depth buffering
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    glDisable(GL_BLEND);

    // Get the object's geometry; NULL indicates that object is an
    // ellipsoid.
    Geometry* geometry = NULL;
    if (obj.geometry != InvalidResource)
    {
        // This is a model loaded from a file
        geometry = GetGeometryManager()->find(obj.geometry);
    }

    // Get the textures . . .
    if (obj.surface->baseTexture.tex[textureResolution] != InvalidResource)
        ri.baseTex = obj.surface->baseTexture.find(textureResolution);
    if ((obj.surface->appearanceFlags & Surface::ApplyBumpMap) != 0 &&
        context->bumpMappingSupported() &&
        obj.surface->bumpTexture.tex[textureResolution] != InvalidResource)
        ri.bumpTex = obj.surface->bumpTexture.find(textureResolution);
    if ((obj.surface->appearanceFlags & Surface::ApplyNightMap) != 0 &&
        (renderFlags & ShowNightMaps) != 0)
        ri.nightTex = obj.surface->nightTexture.find(textureResolution);
    if ((obj.surface->appearanceFlags & Surface::SeparateSpecularMap) != 0)
        ri.glossTex = obj.surface->specularTexture.find(textureResolution);
    if ((obj.surface->appearanceFlags & Surface::ApplyOverlay) != 0)
        ri.overlayTex = obj.surface->overlayTexture.find(textureResolution);

    // Apply the modelview transform for the object
    glPushMatrix();
    glTranslate(pos);
    glRotate(obj.orientation.conjugate());

    // Scaling will be nonuniform for nonspherical planets. As long as the
    // deviation from spherical isn't too large, the nonuniform scale factor
    // shouldn't mess up the lighting calculations enough to be noticeable
    // (and we turn on renormalization anyhow, which most graphics cards
    // support.)
    float radius = obj.radius;
    Vector3f scaleFactors;
    float geometryScale;
    if (geometry == NULL || geometry->isNormalized())
    {
        geometryScale = obj.radius;
        scaleFactors = obj.radius * obj.semiAxes;
        ri.pointScale = 2.0f * obj.radius / pixelSize;
    }
    else
    {
        geometryScale = obj.geometryScale;
        scaleFactors = Vector3f::Constant(geometryScale);
        ri.pointScale = 2.0f * geometryScale / pixelSize;
    }
    glScale(scaleFactors);

    Matrix3f planetRotation = obj.orientation.toRotationMatrix();
    Matrix4f planetMat = Matrix4f::Identity();
    planetMat.corner<3, 3>(TopLeft) = planetRotation;

    ri.eyeDir_obj = -(planetRotation * pos).normalized();
    ri.eyePos_obj = -(planetRotation * (pos.cwise() / scaleFactors));

    ri.orientation = cameraOrientation * obj.orientation.conjugate();

    ri.pixWidth = discSizeInPixels;

    // Set up the colors
    if (ri.baseTex == NULL ||
        (obj.surface->appearanceFlags & Surface::BlendTexture) != 0)
    {
        ri.color = obj.surface->color;
    }

    ri.ambientColor = ambientColor;
    ri.hazeColor = obj.surface->hazeColor;
    ri.specularColor = obj.surface->specularColor;
    ri.specularPower = obj.surface->specularPower;
    ri.useTexEnvCombine = context->getRenderPath() != GLContext::GLPath_Basic;
    ri.lunarLambert = obj.surface->lunarLambert;
#ifdef USE_HDR
    ri.nightLightScale = obj.surface->nightLightRadiance * exposure * 1.e5f * .5f;
#endif

    // See if the surface should be lit
    bool lit = (obj.surface->appearanceFlags & Surface::Emissive) == 0;

    // Set the OpenGL light state
    unsigned int i;
    for (i = 0; i < ls.nLights; i++)
    {
        const DirectionalLight& light = ls.lights[i];

        glLightDirection(GL_LIGHT0 + i, ls.lights[i].direction_obj);

        // RANT ALERT!
        // This sucks, but it's necessary.  glScale is used to scale a unit
        // sphere up to planet size.  Since normals are transformed by the
        // inverse transpose of the model matrix, this means they end up
        // getting scaled by a factor of 1.0 / planet radius (in km).  This
        // has terrible effects on lighting: the planet appears almost
        // completely dark.  To get around this, the GL_rescale_normal
        // extension was introduced and eventually incorporated into into the
        // OpenGL 1.2 standard.  Of course, not everyone implemented this
        // incredibly simple and essential little extension.  Microsoft is
        // notorious for half-assed support of OpenGL, but 3dfx should have
        // known better: no Voodoo 1/2/3 drivers seem to support this
        // extension.  The following is an attempt to get around the problem by
        // scaling the light brightness by the planet radius.  According to the
        // OpenGL spec, this should work fine, as clamping of colors to [0, 1]
        // occurs *after* lighting.  It works fine on my GeForce3 when I
        // disable EXT_rescale_normal, but I'm not certain whether other
        // drivers are as well behaved as nVidia's.
        //
        // Addendum: Unsurprisingly, using color values outside [0, 1] produces
        // problems on Savage4 cards.

        Vector3f lightColor = light.color.toVector3() * light.irradiance;
        if (useRescaleNormal)
        {
            glLightColor(GL_LIGHT0 + i, GL_DIFFUSE, lightColor);
            glLightColor(GL_LIGHT0 + i, GL_SPECULAR, lightColor);
        }
        else
        {
            glLightColor(GL_LIGHT0 + i, GL_DIFFUSE, lightColor * radius);
        }
        glEnable(GL_LIGHT0 + i);
    }

    // Compute the inverse model/view matrix
    // TODO: This code uses the old vector classes, but will be eliminated when the new planet rendering code
    // is adopted. The new planet renderer doesn't require the inverse transformed view frustum.
#if 0
    Transform3f invModelView = cameraOrientation.conjugate() *
                               Translation3f(-pos) *
                               obj.orientation *
                               Scaling3f(1.0f / radius);
#endif
    Mat4f planetMat_old(Vec4f(planetMat.col(0).x(), planetMat.col(0).y(), planetMat.col(0).z(), planetMat.col(0).w()),
                        Vec4f(planetMat.col(1).x(), planetMat.col(1).y(), planetMat.col(1).z(), planetMat.col(1).w()),
                        Vec4f(planetMat.col(2).x(), planetMat.col(2).y(), planetMat.col(2).z(), planetMat.col(2).w()),
                        Vec4f(planetMat.col(3).x(), planetMat.col(3).y(), planetMat.col(3).z(), planetMat.col(3).w()));

    Mat4f invMV = (fromEigen(cameraOrientation).toMatrix4() *
                   Mat4f::translation(Point3f(-pos.x() / radius, -pos.y() / radius, -pos.z() / radius)) *
                   planetMat_old);

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
            frustumFarPlane = (float) sqrt(square(d) - square(eradius)) * 1.1f;
        }
        else
        {
            // We're inside the bounding sphere; leave the far plane alone
        }

        if (obj.atmosphere != NULL)
        {
            float atmosphereHeight = max(obj.atmosphere->cloudHeight,
                                         obj.atmosphere->mieScaleHeight * (float) -log(AtmosphereExtinctionThreshold));
            if (atmosphereHeight > 0.0f)
            {
                // If there's an atmosphere, we need to move the far plane
                // out so that the clouds and atmosphere shell aren't clipped.
                float atmosphereRadius = eradius + atmosphereHeight;
                frustumFarPlane += (float) sqrt(square(atmosphereRadius) -
                                                square(eradius));
            }
        }
    }

    // Transform the frustum into object coordinates using the
    // inverse model/view matrix. The frustum is scaled to a
    // normalized coordinate system where the 1 unit = 1 planet
    // radius (for an ellipsoidal planet, radius is taken to be
    // largest semiaxis.)
    Frustum viewFrustum(degToRad(fov),
                        (float) windowWidth / (float) windowHeight,
                        nearPlaneDistance / radius, frustumFarPlane / radius);
    viewFrustum.transform(invMV);

    // Get cloud layer parameters
    Texture* cloudTex       = NULL;
    Texture* cloudNormalMap = NULL;
    float cloudTexOffset    = 0.0f;
    if (obj.atmosphere != NULL)
    {
        Atmosphere* atmosphere = const_cast<Atmosphere*>(obj.atmosphere); // Ugly cast required because MultiResTexture::find() is non-const
        if ((renderFlags & ShowCloudMaps) != 0)
        {
            if (atmosphere->cloudTexture.tex[textureResolution] != InvalidResource)
                cloudTex = atmosphere->cloudTexture.find(textureResolution);
            if (atmosphere->cloudNormalMap.tex[textureResolution] != InvalidResource)
                cloudNormalMap = atmosphere->cloudNormalMap.find(textureResolution);
        }
        if (atmosphere->cloudSpeed != 0.0f)
            cloudTexOffset = (float) (-pfmod(now * atmosphere->cloudSpeed / (2 * PI), 1.0));
    }

    if (obj.geometry == InvalidResource)
    {
        // A null model indicates that this body is a sphere
        if (lit)
        {
            switch (context->getRenderPath())
            {
            case GLContext::GLPath_GLSL:
                renderEllipsoid_GLSL(ri, ls,
                                     const_cast<Atmosphere*>(obj.atmosphere), cloudTexOffset,
                                     scaleFactors,
                                     textureResolution,
                                     renderFlags,
                                     obj.orientation, viewFrustum, *context);
                break;

            case GLContext::GLPath_NV30:
                renderSphere_FP_VP(ri, viewFrustum, *context);
                break;

            case GLContext::GLPath_NvCombiner_ARBVP:
            case GLContext::GLPath_NvCombiner_NvVP:
                renderSphere_Combiners_VP(ri, ls, viewFrustum, *context);
                break;

            case GLContext::GLPath_NvCombiner:
                renderSphere_Combiners(ri, viewFrustum, *context);
                break;

            case GLContext::GLPath_DOT3_ARBVP:
                renderSphere_DOT3_VP(ri, ls, viewFrustum, *context);
                break;

            default:
                renderSphereDefault(ri, viewFrustum, true, *context);
            }
        }
        else
        {
            renderSphereDefault(ri, viewFrustum, false, *context);
        }
    }
    else
    {
        if (geometry != NULL)
        {
            ResourceHandle texOverride = obj.surface->baseTexture.tex[textureResolution];

            if (context->getRenderPath() == GLContext::GLPath_GLSL)
            {
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
                                        astro::daysToSecs(now - astro::J2000));
                }
                else
                {
                    renderGeometry_GLSL_Unlit(geometry,
                                              ri,
                                              texOverride,
                                              geometryScale,
                                              renderFlags,
                                              obj.orientation,
                                              astro::daysToSecs(now - astro::J2000));
                }

                for (unsigned int i = 1; i < 8;/*context->getMaxTextures();*/ i++)
                {
                    glActiveTextureARB(GL_TEXTURE0_ARB + i);
                    glDisable(GL_TEXTURE_2D);
                }
                glActiveTextureARB(GL_TEXTURE0_ARB);
                glEnable(GL_TEXTURE_2D);
                glUseProgramObjectARB(0);
            }
            else
            {
                renderModelDefault(geometry, ri, lit, texOverride);
            }
        }
    }

    if (obj.rings != NULL && distance <= obj.rings->innerRadius)
    {
        if (context->getRenderPath() == GLContext::GLPath_GLSL)
        {
            renderRings_GLSL(*obj.rings, ri, ls,
                             radius, 1.0f - obj.semiAxes.y(),
                             textureResolution,
                             (renderFlags & ShowRingShadows) != 0 && lit,
                             detailOptions.ringSystemSections);
        }
        else
        {
            renderRings(*obj.rings, ri, radius, 1.0f - obj.semiAxes.y(),
                        textureResolution,
                        context->getMaxTextures() > 1 &&
                        (renderFlags & ShowRingShadows) != 0 && lit,
                        *context,
                        detailOptions.ringSystemSections);
        }
    }

    if (obj.atmosphere != NULL)
    {
        Atmosphere* atmosphere = const_cast<Atmosphere *>(obj.atmosphere);

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
            fade = clamp(thicknessInPixels - 2);
        }
        else
        {
            fade = 1.0f;
        }

        if (fade > 0 && (renderFlags & ShowAtmospheres) != 0)
        {
            // Only use new atmosphere code in OpenGL 2.0 path when new style parameters are defined.
            // TODO: convert old style atmopshere parameters
            if (context->getRenderPath() == GLContext::GLPath_GLSL &&
                atmosphere->mieScaleHeight > 0.0f)
            {
                float atmScale = 1.0f + atmosphere->height / radius;

                renderAtmosphere_GLSL(ri, ls,
                                      atmosphere,
                                      radius * atmScale,
                                      obj.orientation,
                                      viewFrustum,
                                      *context);
            }
            else
            {
                glPushMatrix();
                glLoadIdentity();
                glDisable(GL_LIGHTING);
                glDisable(GL_TEXTURE_2D);
                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

                glRotate(cameraOrientation);

                renderEllipsoidAtmosphere(*atmosphere,
                                          pos,
                                          obj.orientation,
                                          scaleFactors,
                                          ri.sunDir_eye,
                                          ls,
                                          thicknessInPixels,
                                          lit);
                glEnable(GL_TEXTURE_2D);
                glPopMatrix();
            }
        }

        // If there's a cloud layer, we'll render it now.
        if (cloudTex != NULL)
        {
            glPushMatrix();

            float cloudScale = 1.0f + atmosphere->cloudHeight / radius;
            glScalef(cloudScale, cloudScale, cloudScale);

            // If we're beneath the cloud level, render the interior of
            // the cloud sphere.
            if (distance - radius < atmosphere->cloudHeight)
                glFrontFace(GL_CW);

            if (atmosphere->cloudSpeed != 0.0f)
            {
                // Make the clouds appear to rotate above the surface of
                // the planet.  This is easier to do with the texture
                // matrix than the model matrix because changing the
                // texture matrix doesn't require us to compute a second
                // set of model space rendering parameters.
                glMatrixMode(GL_TEXTURE);
                glTranslatef(cloudTexOffset, 0.0f, 0.0f);
                glMatrixMode(GL_MODELVIEW);
            }

            glEnable(GL_LIGHTING);
            glDepthMask(GL_FALSE);
            cloudTex->bind();
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#ifdef HDR_COMPRESS
            glColor4f(0.5f, 0.5f, 0.5f, 1);
#else
            glColor4f(1, 1, 1, 1);
#endif

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
                if (context->getRenderPath() == GLContext::GLPath_GLSL)
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
                                      *context);
                }
                else
                {
                    VertexProcessor* vproc = context->getVertexProcessor();
                    if (vproc != NULL)
                    {
                        vproc->enable();
                        vproc->parameter(vp::AmbientColor, ri.ambientColor * ri.color);
                        vproc->parameter(vp::TextureTranslation,
                                         cloudTexOffset, 0.0f, 0.0f, 0.0f);
                        if (ls.nLights > 1)
                            vproc->use(vp::diffuseTexOffset_2light);
                        else
                            vproc->use(vp::diffuseTexOffset);
                        setLightParameters_VP(*vproc, ls, ri.color, Color::Black);
                    }

                    g_lodSphere->render(*context,
                                        LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                                        viewFrustum,
                                        ri.pixWidth,
                                        cloudTex);

                    if (vproc != NULL)
                        vproc->disable();
                }
            }
            else
            {
                glDisable(GL_LIGHTING);
                g_lodSphere->render(*context,
                                    LODSphereMesh::Normals | LODSphereMesh::TexCoords0,
                                    viewFrustum,
                                    ri.pixWidth,
                                    cloudTex);
                glEnable(GL_LIGHTING);
            }

            glDisable(GL_POLYGON_OFFSET_FILL);

            // Reset the texture matrix
            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
            glMatrixMode(GL_MODELVIEW);

            glDepthMask(GL_TRUE);
            glFrontFace(GL_CCW);

            glPopMatrix();
        }
    }

    // No separate shadow rendering pass required for GLSL path
    if (ls.shadows[0] != NULL &&
        ls.shadows[0]->size() != 0 &&
        (obj.surface->appearanceFlags & Surface::Emissive) == 0 &&
        context->getRenderPath() != GLContext::GLPath_GLSL)
    {
        if (context->getVertexProcessor() != NULL &&
            context->getFragmentProcessor() != NULL)
        {
            renderEclipseShadows_Shaders(geometry,
                                         *ls.shadows[0],
                                         ri,
                                         radius, obj.orientation, viewFrustum,
                                         *context);
        }
        else
        {
            renderEclipseShadows(geometry,
                                 *ls.shadows[0],
                                 ri,
                                 radius, obj.orientation, viewFrustum,
                                 *context);
        }
    }

    if (obj.rings != NULL &&
        (obj.surface->appearanceFlags & Surface::Emissive) == 0 &&
        (renderFlags & ShowRingShadows) != 0)
    {
        Texture* ringsTex = obj.rings->texture.find(textureResolution);
        if (ringsTex != NULL)
        {
            Vector3f sunDir = pos.normalized();

            glEnable(GL_TEXTURE_2D);
            ringsTex->bind();

            if (useClampToBorder &&
                context->getVertexPath() != GLContext::VPath_Basic &&
                context->getRenderPath() != GLContext::GLPath_GLSL)
            {
                renderRingShadowsVS(geometry,
                                    *obj.rings,
                                    sunDir,
                                    ri,
                                    radius, 1.0f - obj.semiAxes.y(),
                                    planetMat, viewFrustum,
                                    *context);
            }
        }
    }

    if (obj.rings != NULL && distance > obj.rings->innerRadius)
    {
        glDepthMask(GL_FALSE);
        if (context->getRenderPath() == GLContext::GLPath_GLSL)
        {
            renderRings_GLSL(*obj.rings, ri, ls,
                             radius, 1.0f - obj.semiAxes.y(),
                             textureResolution,
                             (renderFlags & ShowRingShadows) != 0 && lit,
                             detailOptions.ringSystemSections);
        }
        else
        {
            renderRings(*obj.rings, ri, radius, 1.0f - obj.semiAxes.y(),
                        textureResolution,
                        (context->hasMultitexture() &&
                         (renderFlags & ShowRingShadows) != 0 && lit),
                        *context,
                        detailOptions.ringSystemSections);
        }
    }

    // Disable all light sources other than the first
    for (i = 0; i < ls.nLights; i++)
        glDisable(GL_LIGHT0 + i);

    glPopMatrix();
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
}


bool Renderer::testEclipse(const Body& receiver,
                           const Body& caster,
                           LightingState& lightingState,
                           unsigned int lightIndex,
                           double now)
{
    const DirectionalLight& light = lightingState.lights[lightIndex];
    LightingState::EclipseShadowVector& shadows = *lightingState.shadows[lightIndex];
    bool isReceiverShadowed = false;
    
    // Ignore situations where the shadow casting body is much smaller than
    // the receiver, as these shadows aren't likely to be relevant.  Also,
    // ignore eclipses where the caster is not an ellipsoid, since we can't
    // generate correct shadows in this case.
    if (caster.getRadius() >= receiver.getRadius() * MinRelativeOccluderRadius &&
        caster.hasVisibleGeometry() &&
        caster.extant(now) &&
        caster.isEllipsoid())
    {
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
        //assert(sun != NULL);
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

        double dist = distance(posReceiver,
                               Ray3d(posCaster, lightToCasterDir));
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
            shadow.maxDepth = std::min(1.0f, square(appOccluderRadius / appSunRadius));
            shadow.caster = &caster;

            // Ignore transits that don't produce a visible shadow.
            if (shadow.maxDepth > 1.0f / 256.0f)
                shadows.push_back(shadow);

            isReceiverShadowed = true;
        }
        
        // If the caster has a ring system, see if it casts a shadow on the receiver.
        // Ring shadows are only supported in the OpenGL 2.0 path.
        if (caster.getRings() && context->getRenderPath() == GLContext::GLPath_GLSL)
        {
            bool shadowed = false;
            
            // The shadow volume of the rings is an oblique circular cylinder
            if (dist < caster.getRings()->outerRadius + receiver.getRadius())
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
                        caster.getRings()->outerRadius * ringPlaneNormal.dot(shadowDirection);
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
                    shadow.ringSystem = caster.getRings();
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
                            const Quaternionf& cameraOrientation,
                            float nearPlaneDistance,
                            float farPlaneDistance)
{
    double now = observer.getTime();
    float altitude = distance - body.getRadius();
    float discSizeInPixels = body.getRadius() /
        (max(nearPlaneDistance, altitude) * pixelSize);

    if (discSizeInPixels > 1 && body.hasVisibleGeometry())
    {
        RenderProperties rp;

        if (displayedSurface.empty())
        {
            rp.surface = const_cast<Surface*>(&body.getSurface());
        }
        else
        {
            rp.surface = body.getAlternateSurface(displayedSurface);
            if (rp.surface == NULL)
                rp.surface = const_cast<Surface*>(&body.getSurface());
        }
        rp.atmosphere = body.getAtmosphere();
        rp.rings = body.getRings();
        rp.radius = body.getRadius();
        rp.geometry = body.getGeometry();
        rp.semiAxes = body.getSemiAxes() * (1.0f / rp.radius);
        rp.geometryScale = body.getGeometryScale();

        Quaterniond q = body.getRotationModel(now)->spin(now) *
                        body.getEclipticToEquatorial(now);

        rp.orientation = body.getGeometryOrientation() * q.cast<float>();

        if (body.getLocations() != NULL && (labelMode & LocationLabels) != 0)
            body.computeLocations();

        Vector3f scaleFactors;
        bool isNormalized = false;
        Geometry* geometry = NULL;
        if (rp.geometry != InvalidResource)
            geometry = GetGeometryManager()->find(rp.geometry);
        if (geometry == NULL || geometry->isNormalized())
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
#ifdef USE_HDR
                            faintestMag,
                            DEFAULT_EXPOSURE + brightPlus, //exposure + brightPlus,
                            appMag,
#endif
                            lights);
        assert(lights.nLights <= MaxLights);

        lights.ambientColor = Vector3f(ambientColor.red(),
                                       ambientColor.green(),
                                       ambientColor.blue());

        {
            // Clear out the list of eclipse shadows
            for (unsigned int li = 0; li < lights.nLights; li++)
            {
                eclipseShadows[li].clear();
                lights.shadows[li] = &eclipseShadows[li];
            }
        }


        // Add ring shadow records for each light
        if (body.getRings() && ShowRingShadows)
        {
            for (unsigned int li = 0; li < lights.nLights; li++)
            {
                lights.ringShadows[li].ringSystem = body.getRings();
                lights.ringShadows[li].casterOrientation = q.cast<float>();
                lights.ringShadows[li].origin = Vector3f::Zero();
                lights.ringShadows[li].direction = -lights.lights[li].position.normalized().cast<float>();
            }
        }
        
        // Calculate eclipse circumstances
        if ((renderFlags & ShowEclipseShadows) != 0 &&
            body.getSystem() != NULL)
        {
            PlanetarySystem* system = body.getSystem();

            if (system->getPrimaryBody() == NULL &&
                body.getSatellites() != NULL)
            {
                // The body is a planet.  Check for eclipse shadows
                // from all of its satellites.
                PlanetarySystem* satellites = body.getSatellites();
                if (satellites != NULL)
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
            else if (system->getPrimaryBody() != NULL)
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
                        Body* planet = system->getPrimaryBody();
                        while (planet != NULL)
                        {
                            testEclipse(body, *planet, lights, li, now);
                            if (planet->getSystem() != NULL)
                                planet = planet->getSystem()->getPrimaryBody();
                            else
                                planet = NULL;
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
            if (lights.ringShadows[li].ringSystem != NULL)
            {
                RingSystem* rings = lights.ringShadows[li].ringSystem;

                // Use the first set of ring shadows found (shadowing the brightest light
                // source.)
                if (lights.shadowingRingSystem == NULL)
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
                Texture* ringsTex = rings->texture.find(textureResolution);
                if (ringsTex)
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
                    float areaLightLod = celmath::log2(max(relativeFeatureSize, 1.0f));

                    // Compute the LOD that would be automatically used by the GPU.
                    float texelToPixelRatio = ringTextureWidth / projectedRingSizeInPixels;
                    float gpuLod = celmath::log2(texelToPixelRatio);

                    //float lod = max(areaLightLod, log(texelToPixelRatio) / log(2.0f));
                    float lod = max(areaLightLod, gpuLod);

                    // maxLOD is the index of the smallest mipmap (or close to it for non-power-of-two
                    // textures.) We can't make the lod larger than this.
                    float maxLod = celmath::log2((float) ringsTex->getWidth());
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
                    float lodBias = max(0.0f, lod - gpuLod);

                    if (GLEW_ARB_shader_texture_lod)
                    {
                        lights.ringShadows[li].texLod = lod;
                    }
                    else
                    {
                        lights.ringShadows[li].texLod = lodBias;
                    }
                }
                else
                {
                    lights.ringShadows[li].texLod = 0.0f;
                }
            }
        }

        renderObject(pos, distance, now,
                     cameraOrientation, nearPlaneDistance, farPlaneDistance,
                     rp, lights);

        if (body.getLocations() != NULL && (labelMode & LocationLabels) != 0)
        {
            // Set up location markers for this body
            mountainRep    = MarkerRepresentation(MarkerRepresentation::Triangle, 8.0f, LocationLabelColor);
            craterRep      = MarkerRepresentation(MarkerRepresentation::Circle,   8.0f, LocationLabelColor);
            observatoryRep = MarkerRepresentation(MarkerRepresentation::Plus,     8.0f, LocationLabelColor);
            cityRep        = MarkerRepresentation(MarkerRepresentation::X,        3.0f, LocationLabelColor);
            genericLocationRep = MarkerRepresentation(MarkerRepresentation::Square, 8.0f, LocationLabelColor);
            
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glDisable(GL_BLEND);

            // We need a double precision body-relative position of the
            // observer, otherwise location labels will tend to jitter.
            Vector3d posd = body.getPosition(observer.getTime()).offsetFromKm(observer.getPosition());
            renderLocations(body, posd, q);

            glDisable(GL_DEPTH_TEST);
        }
    }

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
#ifdef USE_HDR
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
#endif

    if (body.isVisibleAsPoint())
    {
        if (useNewStarRendering)
        {
            renderObjectAsPoint(pos,
                                body.getRadius(),
                                appMag,
                                faintestMag,
                                discSizeInPixels,
                                body.getSurface().color,
                                cameraOrientation,
                                false, false);
        }
        else
        {
            renderObjectAsPoint_nosprite(pos,
                                 body.getRadius(),
                                 appMag,
                                 faintestMag,
                                 discSizeInPixels,
                                 body.getSurface().color,
                                 cameraOrientation,
                                 false);
        }
    }
#ifdef USE_HDR
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#endif
}


void Renderer::renderStar(const Star& star,
                          const Vector3f& pos,
                          float distance,
                          float appMag,
                          const Quaternionf& cameraOrientation,
                          double now,
                          float nearPlaneDistance,
                          float farPlaneDistance)
{
    if (!star.getVisibility())
        return;

    Color color = colorTemp->lookupColor(star.getTemperature());
    float radius = star.getRadius();
    float discSizeInPixels = radius / (distance * pixelSize);

    if (discSizeInPixels > 1)
    {
        Surface surface;
        RenderProperties rp;

        surface.color = color;

        MultiResTexture mtex = star.getTexture();
        if (mtex.tex[textureResolution] != InvalidResource)
        {
            surface.baseTexture = mtex;
        }
        else
        {
            surface.baseTexture = InvalidResource;
        }
        surface.appearanceFlags |= Surface::ApplyBaseTexture;
        surface.appearanceFlags |= Surface::Emissive;

        rp.surface = &surface;
        rp.rings = NULL;
        rp.radius = star.getRadius();
        rp.semiAxes = star.getEllipsoidSemiAxes();
        rp.geometry = star.getGeometry();

#ifndef USE_HDR
        Atmosphere atmosphere;
        Color atmColor(color.red() * 0.5f, color.green() * 0.5f, color.blue() * 0.5f);
        atmosphere.height = radius * CoronaHeight;
        atmosphere.lowerColor = atmColor;
        atmosphere.upperColor = atmColor;
        atmosphere.skyColor = atmColor;

        // Use atmosphere effect to give stars a fuzzy fringe
        if (rp.geometry == InvalidResource)
            rp.atmosphere = &atmosphere;
        else
#endif
        rp.atmosphere = NULL;

        rp.orientation = star.getRotationModel()->orientationAtTime(now).cast<float>();

        renderObject(pos, distance, now,
                     cameraOrientation, nearPlaneDistance, farPlaneDistance,
                     rp, LightingState());
    }

    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
#ifdef USE_HDR
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
#endif

#ifndef USE_HDR
    if (useNewStarRendering)
    {
#endif
        renderObjectAsPoint(pos,
                            star.getRadius(),
                            appMag,
                            faintestMag,
                            discSizeInPixels,
                            color,
                            cameraOrientation,
                            true, true);
#ifndef USE_HDR
    }
    else
    {
        renderObjectAsPoint_nosprite(pos,
                                     star.getRadius(),
                                     appMag,
                                     faintestPlanetMag,
                                     discSizeInPixels,
                                     color,
                                     cameraOrientation,
                                     true);
    }
#endif
#ifdef USE_HDR
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#endif
}


static const int MaxCometTailPoints = 120;
static const int CometTailSlices = 48;
struct CometTailVertex
{
    Vector3f point;
    Vector3f normal;
    float brightness;
};

static CometTailVertex cometTailVertices[CometTailSlices * MaxCometTailPoints];

static void ProcessCometTailVertex(const CometTailVertex& v,
                                   const Vector3f& viewDir,
                                   float fadeDistFromSun)
{
    // If fadeDistFromSun = x/x0 >= 1.0, comet tail starts fading,
    // i.e. fadeFactor quickly transits from 1 to 0.

    float fadeFactor = 0.5f - 0.5f * (float) tanh(fadeDistFromSun - 1.0f / fadeDistFromSun);
    float shade = abs(viewDir.dot(v.normal) * v.brightness * fadeFactor);
    glColor4f(0.5f, 0.5f, 0.75f, shade);
    glVertex(v.point);
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


// TODO: Remove unused parameters??
void Renderer::renderCometTail(const Body& body,
                               const Vector3f& pos,
                               double now,
                               float discSizeInPixels)
{
    Vector3f cometPoints[MaxCometTailPoints];
    Vector3d pos0 = body.getOrbit(now)->positionAtTime(now);
    Vector3d pos1 = body.getOrbit(now)->positionAtTime(now - 0.01);
    Vector3d vd = pos1 - pos0;
    double t = now;

    float distanceFromSun, irradiance_max = 0.0f;
    unsigned int li_eff = 0;    // Select the first sun as default to
                                // shut up compiler warnings

    // Adjust the amount of triangles used for the comet tail based on
    // the screen size of the comet.
    float lod = min(1.0f, max(0.2f, discSizeInPixels / 1000.0f));
    int nTailPoints = (int) (MaxCometTailPoints * lod);
    int nTailSlices = (int) (CometTailSlices * lod);

    // Find the sun with the largest irrradiance of light onto the comet
    // as function of the comet's position;
    // irradiance = sun's luminosity / square(distanceFromSun);

    for (unsigned int li = 0; li < lightSourceList.size(); li++)
    {
        distanceFromSun = (float) (pos.cast<double>() - lightSourceList[li].position).norm();
        float irradiance = lightSourceList[li].luminosity / square(distanceFromSun);
        if (irradiance > irradiance_max)
        {
            li_eff = li;
            irradiance_max = irradiance;
        }
    }
    float fadeDistance = 1.0f / (float) (COMET_TAIL_ATTEN_DIST_SOL * sqrt(irradiance_max));

    // direction to sun with dominant light irradiance:
    Vector3f sunDir = (pos.cast<double>() - lightSourceList[li_eff].position).cast<float>().normalized();

    float dustTailLength = cometDustTailLength((float) pos0.norm(), body.getRadius());
    float dustTailRadius = dustTailLength * 0.1f;

    Vector3f origin = -sunDir * (body.getRadius() * 100);

    int i;
    for (i = 0; i < nTailPoints; i++)
    {
        float alpha = (float) i / (float) nTailPoints;
        alpha = alpha * alpha;
        cometPoints[i] = origin + sunDir * (dustTailLength * alpha);
    }

    // We need three axes to define the coordinate system for rendering the
    // comet.  The first axis is the sun-to-comet direction, and the other
    // two are chose orthogonal to each other and the primary axis.
    Vector3f v = (cometPoints[1] - cometPoints[0]).normalized();
    Quaternionf q = body.getEclipticToEquatorial(t).cast<float>();
    Vector3f u = v.unitOrthogonal();
    Vector3f w = u.cross(v);

    glColor4f(0.0f, 1.0f, 1.0f, 0.5f);
    glPushMatrix();
    glTranslate(pos);

    // glActiveTextureARB(GL_TEXTURE0_ARB);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    for (i = 0; i < nTailPoints; i++)
    {
        float brightness = 1.0f - (float) i / (float) (nTailPoints - 1);
        Vector3f v0, v1;
        float sectionLength;
        if (i != 0 && i != nTailPoints - 1)
        {
            v0 = cometPoints[i] - cometPoints[i - 1];
            v1 = cometPoints[i + 1] - cometPoints[i];
            sectionLength = v0.norm();
            v0.normalize();
            v1.normalize();
            q.setFromTwoVectors(v0, v1);
            Matrix3f m = q.toRotationMatrix();
            u = m * u;
            v = m * v;
            w = m * w;
        }
        else if (i == 0)
        {
            v0 = cometPoints[i + 1] - cometPoints[i];
            sectionLength = v0.norm();
            v0.normalize();
            v1 = v0;
        }
        else
        {
            v0 = v1 = cometPoints[i] - cometPoints[i - 1];
            sectionLength = v0.norm();
            v0.normalize();
            v1 = v0;
        }

        float radius = (float) i / (float) nTailPoints *
            dustTailRadius;
        float dr = (dustTailRadius / (float) nTailPoints) /
            sectionLength;

        float w0 = (float) atan(dr);
        float d = std::sqrt(1.0f + w0 * w0);
        float w1 = 1.0f / d;
        w0 = w0 / d;

        // Special case the first vertex in the comet tail
        if (i == 0)
        {
            w0 = 1;
            w1 = 0.0f;
        }

        for (int j = 0; j < nTailSlices; j++)
        {
            float theta = (float) (2 * PI * (float) j / nTailSlices);
            float s = (float) sin(theta);
            float c = (float) cos(theta);
            CometTailVertex& vtx = cometTailVertices[i * nTailSlices + j];
            vtx.normal = u * (s * w1) + w * (c * w1) + v * w0;
            s *= radius;
            c *= radius;

            vtx.point = cometPoints[i] + u * s + w * c;
            vtx.brightness = brightness;
        }
    }

    Vector3f viewDir = pos.normalized();

    glDisable(GL_CULL_FACE);
    for (i = 0; i < nTailPoints - 1; i++)
    {
        glBegin(GL_QUAD_STRIP);
        int n = i * nTailSlices;
        for (int j = 0; j < nTailSlices; j++)
        {
            ProcessCometTailVertex(cometTailVertices[n + j], viewDir, fadeDistance);
            ProcessCometTailVertex(cometTailVertices[n + j + nTailSlices],
                                   viewDir, fadeDistance);
        }
        ProcessCometTailVertex(cometTailVertices[n], viewDir, fadeDistance);
        ProcessCometTailVertex(cometTailVertices[n + nTailSlices],
                               viewDir, fadeDistance);
        glEnd();
    }
    glEnable(GL_CULL_FACE);

    glBegin(GL_LINE_STRIP);
    for (i = 0; i < nTailPoints; i++)
    {
        glVertex(cometPoints[i]);
    }
    glEnd();

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glPopMatrix();
}


// Render a reference mark
void Renderer::renderReferenceMark(const ReferenceMark& refMark,
                                   const Vector3f& pos,
                                   float distance,
                                   double now,
                                   float nearPlaneDistance)
{
    float altitude = distance - refMark.boundingSphereRadius();
    float discSizeInPixels = refMark.boundingSphereRadius() /
        (max(nearPlaneDistance, altitude) * pixelSize);

    if (discSizeInPixels <= 1)
        return;

    // Apply the modelview transform for the object
    glPushMatrix();
    glTranslate(pos);

    refMark.render(this, pos, discSizeInPixels, now);

    glPopMatrix();

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
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
    double irradiance = power / sphereArea(distanceFromSun * 1000);

    // Compute the total energy hitting the planet; assume an albedo of 1.0, so
    // reflected energy = incident energy.
    double incidentEnergy = irradiance * circleArea(objRadius * 1000);
    
    // Compute the luminosity (i.e. power relative to solar power)
    return (float) (incidentEnergy / astro::SOLAR_POWER);
}


void Renderer::addRenderListEntries(RenderListEntry& rle,
                                    Body& body,
                                    bool isLabeled)
{
    bool visibleAsPoint = rle.appMag < faintestPlanetMag && body.isVisibleAsPoint();

    if (rle.discSizeInPixels > 1 || visibleAsPoint || isLabeled)
    {
        rle.renderableType = RenderListEntry::RenderableBody;
        rle.body = &body;

        if (body.getGeometry() != InvalidResource && rle.discSizeInPixels > 1)
        {
            Geometry* geometry = GetGeometryManager()->find(body.getGeometry());
            if (geometry == NULL)
                rle.isOpaque = true;
            else
                rle.isOpaque = geometry->isOpaque();
        }
        else
        {
            rle.isOpaque = true;
        }
        rle.radius = body.getRadius();
        renderList.push_back(rle);
    }

    if (body.getClassification() == Body::Comet && (renderFlags & ShowCometTails) != 0)
    {
        float radius = cometDustTailLength(rle.sun.norm(), body.getRadius());
        float discSize = (radius / (float) rle.distance) / pixelSize;
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

    const list<ReferenceMark*>* refMarks = body.getReferenceMarks();
    if (refMarks != NULL)
    {
        for (list<ReferenceMark*>::const_iterator iter = refMarks->begin();
            iter != refMarks->end(); ++iter)
        {
            const ReferenceMark* rm = *iter;

            rle.renderableType = RenderListEntry::RenderableReferenceMark;
            rle.refMark = rm;
            rle.isOpaque = rm->isOpaque();
            rle.radius = rm->boundingSphereRadius();
            renderList.push_back(rle);
        }
    }
}


void Renderer::buildRenderLists(const Vector3d& astrocentricObserverPos,
                                const Frustum& viewFrustum,
                                const Vector3d& viewPlaneNormal,
                                const Vector3d& frameCenter,
                                const FrameTree* tree,
                                const Observer& observer,
                                double now)
{
    int labelClassMask = translateLabelModeToClassMask(labelMode);

    Matrix3f viewMat = observer.getOrientationf().toRotationMatrix();
    Vector3f viewMatZ = viewMat.row(2);
    double invCosViewAngle = 1.0 / cosViewConeAngle;
    double sinViewAngle = sqrt(1.0 - square(cosViewConeAngle));   

    unsigned int nChildren = tree != NULL ? tree->childCount() : 0;
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
        ReferenceFrame* frame = phase->orbitFrame();
        Vector3d pos_s = frameCenter + frame->getOrientation(now).conjugate() * p;

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
            for (unsigned int li = 0; li < lightSourceList.size(); li++)
            {
                Vector3d sunPos = pos_v - lightSourceList[li].position;
                appMag = min(appMag, body->getApparentMagnitude(lightSourceList[li].luminosity, sunPos, pos_v));
            }

            bool visibleAsPoint = appMag < faintestPlanetMag && body->isVisibleAsPoint();
            bool isLabeled = (body->getOrbitClassification() & labelClassMask) != 0;
            bool visible = body->isVisible();

            if ((discSize > 1 || visibleAsPoint || isLabeled) && visible)
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
        if (subtree != NULL)
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
            float minPossibleDistance = (float) (dist_v - subtree->boundingSphereRadius());
            float brightestPossible = 0.0;
            float largestPossible = 0.0;

            // If the viewer is not within the subtree bounding sphere, see if we can cull it because
            // it contains no objects brighter than the limiting magnitude and no objects that will
            // be larger than one pixel in size.
            if (minPossibleDistance > 1.0f)
            {
                // Figure out the magnitude of the brightest possible object in the subtree.

                // Compute the luminosity from reflected light of the largest object in the subtree
                float lum = 0.0f;                
                for (unsigned int li = 0; li < lightSourceList.size(); li++)
                {
                    Vector3d sunPos = pos_v - lightSourceList[li].position;
                    lum += luminosityAtOpposition(lightSourceList[li].luminosity, (float) sunPos.norm(), (float) subtree->maxChildRadius());
                }
                brightestPossible = astro::lumToAppMag(lum, astro::kilometersToLightYears(minPossibleDistance));
                largestPossible = (float) subtree->maxChildRadius() / (float) minPossibleDistance / pixelSize;
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
                if (viewFrustum.testSphere(pos_v.cast<float>(), (float) subtree->boundingSphereRadius()) != Frustum::Outside)
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
                float influenceRadius = (float) (subtree->boundingSphereRadius() +
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
                               const Frustum& viewFrustum,
                               const FrameTree* tree,
                               double now)
{
    Matrix3d viewMat = observerOrientation.toRotationMatrix();
    Vector3d viewMatZ = viewMat.row(2);

    unsigned int nChildren = tree != NULL ? tree->childCount() : 0;
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
             (orbitVis == Body::UseClassVisibility && (body->getOrbitClassification() & orbitMask) != 0)))
        {
            Vector3d orbitOrigin = Vector3d::Zero();
            Selection centerObject = phase->orbitFrame()->getCenter();
            if (centerObject.body() != NULL)
            {
                orbitOrigin = centerObject.body()->getAstrocentricPosition(now);
            }

            // Calculate the origin of the orbit relative to the observer
            Vector3d relOrigin = orbitOrigin - astrocentricObserverPos;

            // Compute the size of the orbit in pixels
            double originDistance = pos_v.norm();
            double boundingRadius = body->getOrbit(now)->getBoundingRadius();
            float orbitRadiusInPixels = (float) (boundingRadius / (originDistance * pixelSize));

            if (orbitRadiusInPixels > minOrbitSize)
            {
                // Add the orbit of this body to the list of orbits to be rendered
                OrbitPathListEntry path;
                path.body = body;
                path.star = NULL;
                path.centerZ = (float) relOrigin.dot(viewMatZ);
                path.radius = (float) boundingRadius;
                path.origin = relOrigin;
                path.opacity = sizeFade(orbitRadiusInPixels, minOrbitSize, 2.0f);
                orbitPathList.push_back(path);
            }
        }

        const FrameTree* subtree = body->getFrameTree();
        if (subtree != NULL)
        {
            // Only try to render orbits of child objects when:
            //   - The apparent size of the subtree bounding sphere is large enough that
            //     orbit paths will be visible, and
            //   - The subtree bounding sphere isn't outside the view frustum
            double dist_v = pos_v.norm();
            float distanceToBoundingSphere = (float) (dist_v - subtree->boundingSphereRadius());
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
                if (viewFrustum.testSphere(pos_v.cast<float>(), (float) subtree->boundingSphereRadius()) != Frustum::Outside)
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


void Renderer::buildLabelLists(const Frustum& viewFrustum,
                               double now)
{
    int labelClassMask = translateLabelModeToClassMask(labelMode);
    Body* lastPrimary = NULL;
    Sphered primarySphere;

    for (vector<RenderListEntry>::const_iterator iter = renderList.begin();
         iter != renderList.end(); iter++)
    {
        int classification = iter->body->getOrbitClassification();

        if (iter->renderableType == RenderListEntry::RenderableBody &&
            (classification & labelClassMask)                       &&
            viewFrustum.testSphere(iter->position, iter->radius) != Frustum::Outside)
        {
            const Body* body = iter->body;
            Vector3f pos = iter->position;

            float boundingRadiusSize = (float) (body->getOrbit(now)->getBoundingRadius() / iter->distance) / pixelSize;
            if (boundingRadiusSize > minOrbitSize)
            {
                Color labelColor;
                float opacity = sizeFade(boundingRadiusSize, minOrbitSize, 2.0f);

                switch (classification)
                {
                case Body::Planet:
                    labelColor = PlanetLabelColor;
                    break;
                case Body::DwarfPlanet:
                    labelColor = DwarfPlanetLabelColor;
                    break;
                case Body::Moon:
                    labelColor = MoonLabelColor;
                    break;
                case Body::MinorMoon:
                    labelColor = MinorMoonLabelColor;
                    break;
                case Body::Asteroid:
                    labelColor = AsteroidLabelColor;
                    break;
                case Body::Comet:
                    labelColor = CometLabelColor;
                    break;
                case Body::Spacecraft:
                    labelColor = SpacecraftLabelColor;
                    break;
                default:
                    labelColor = Color::Black;
                    break;
                }

                labelColor = Color(labelColor, opacity * labelColor.alpha());

                if (!body->getName().empty())
                {
                    bool isBehindPrimary = false;

                    const TimelinePhase* phase = body->getTimeline()->findPhase(now);
                    Body* primary = phase->orbitFrame()->getCenter().body();
                    if (primary != NULL && (primary->getClassification() & Body::Invisible) != 0)
                    {
                        Body* parent = phase->orbitFrame()->getCenter().body();
                        if (parent != NULL)
                            primary = parent;
                    }

                    // Position the label slightly in front of the object along a line from
                    // object center to viewer.
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
                    if (primary != NULL && primary->isEllipsoid())
                    {
                        // In the typical case, we're rendering labels for many
                        // objects that orbit the same primary. Avoid repeatedly
                        // calling getPosition() by caching the last primary
                        // position.
                        if (primary != lastPrimary)
                        {
                            Vector3d p = phase->orbitFrame()->getOrientation(now).conjugate() *
                                         phase->orbit()->positionAtTime(now);
                            Vector3d v = iter->position.cast<double>() - p;

                            primarySphere = Sphered(v, primary->getRadius());
                            lastPrimary = primary;
                        }

                        Ray3d testRay(Vector3d::Zero(), pos.cast<double>());

                        // Test the viewer-to-labeled object ray against
                        // the primary sphere (TODO: handle ellipsoids)
                        double t = 0.0;
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
                            Vec3d primaryVec(primarySphere.center.x(),
                                             primarySphere.center.y(),
                                             primarySphere.center.z());
                            double distToPrimary = primaryVec.length();
                            Planed primaryTangentPlane(primaryVec, primaryVec * (primaryVec * (1.0 - primarySphere.radius / distToPrimary)));

                            // Compute the intersection of the viewer-to-labeled
                            // object ray with the tangent plane.
                            float u = (float) (primaryTangentPlane.d / (primaryTangentPlane.normal * Vec3d(pos.x(), pos.y(), pos.z())));

                            // If the intersection point is closer to the viewer
                            // than the label, then project the label onto the
                            // tangent plane.
                            if (u < 1.0f && u > 0.0f)
                            {
                                pos = pos * u;
                            }
                        }
                    }

                    addSortedAnnotation(NULL, body->getName(true), labelColor, pos);
                }
            }
        }
    } // for each render list entry      
}


// Add a star orbit to the render list
void Renderer::addStarOrbitToRenderList(const Star& star,
                                        const Observer& observer,
                                        double now)
{
    // If the star isn't fixed, add its orbit to the render list
    if ((renderFlags & ShowOrbits) != 0 &&
        ((orbitMask & Body::Stellar) != 0 || highlightObject.star() == &star))
    {
        Matrix3d viewMat = observer.getOrientation().toRotationMatrix();
        Vector3d viewMatZ = viewMat.row(2);

        if (star.getOrbit() != NULL)
        {
            // Get orbit origin relative to the observer
            Vector3d orbitOrigin = star.getOrbitBarycenterPosition(now).offsetFromKm(observer.getPosition());

            // Compute the size of the orbit in pixels
            double originDistance = orbitOrigin.norm();
            double boundingRadius = star.getOrbit()->getBoundingRadius();
            float orbitRadiusInPixels = (float) (boundingRadius / (originDistance * pixelSize));

            if (orbitRadiusInPixels > minOrbitSize)
            {
                // Add the orbit of this body to the list of orbits to be rendered
                OrbitPathListEntry path;
                path.star = &star;
                path.body = NULL;
                path.centerZ = (float) orbitOrigin.dot(viewMatZ);
                path.radius = (float) boundingRadius;
                path.origin = orbitOrigin;
                path.opacity = sizeFade(orbitRadiusInPixels, minOrbitSize, 2.0f);
                orbitPathList.push_back(path);
            }
        }
    }
}


template <class OBJ, class PREC> class ObjectRenderer : public OctreeProcessor<OBJ, PREC>
{
 public:
    ObjectRenderer(const PREC distanceLimit);

    void process(const OBJ&, PREC, float) {};

 public:
    const Observer* observer;

    GLContext* context;
    Renderer*  renderer;

    Eigen::Vector3f viewNormal;

    float fov;
    float size;
    float pixelSize;
    float faintestMag;
    float faintestMagNight;
    float saturationMag;
#ifdef USE_HDR
    float exposure;
#endif
    float brightnessScale;
    float brightnessBias;
    float distanceLimit;

    // Objects brighter than labelThresholdMag will be labeled
    float labelThresholdMag;

    // These are not fully used by this template's descendants
    // but we place them here just in case a more sophisticated
    // rendering scheme is implemented:
    int nRendered;
    int nClose;
    int nBright;
    int nProcessed;
    int nLabelled;

    int renderFlags;
    int labelMode;
};


template <class OBJ, class PREC>
ObjectRenderer<OBJ, PREC>::ObjectRenderer(const PREC _distanceLimit) :
    distanceLimit((float) _distanceLimit),
#ifdef USE_HDR
    exposure     (0.0f),
#endif
    nRendered    (0),
    nClose       (0),
    nBright      (0),
    nProcessed   (0),
    nLabelled    (0)
{
}


class StarRenderer : public ObjectRenderer<Star, float>
{
 public:
    StarRenderer();

    void process(const Star& star, float distance, float appMag);

 public:
    Vector3d obsPos;

    vector<Renderer::Particle>*      glareParticles;
    vector<RenderListEntry>*         renderList;
    StarVertexBuffer*      starVertexBuffer;
    PointStarVertexBuffer* pointStarVertexBuffer;

    const StarDatabase* starDB;

    bool   useScaledDiscs;
    GLenum starPrimitive;
    float  maxDiscSize;

    float cosFOV;

    const ColorTemperatureTable* colorTemp;
#ifdef DEBUG_HDR_ADAPT
    float minMag;
    float maxMag;
    float minAlpha;
    float maxAlpha;
    float maxSize;
    float above;
    unsigned long countAboveN;
    unsigned long total;
#endif
};


StarRenderer::StarRenderer() :
    ObjectRenderer<Star, float>(STAR_DISTANCE_LIMIT),
    starVertexBuffer     (NULL),
    pointStarVertexBuffer(NULL),
    useScaledDiscs       (false),
    maxDiscSize          (1.0f),
    cosFOV               (1.0f),
    colorTemp            (NULL)
{
}


void StarRenderer::process(const Star& star, float distance, float appMag)
{
    nProcessed++;

    Vector3f starPos = star.getPosition();

    // Calculate the difference at double precision *before* converting to float.
    // This is very important for stars that are far from the origin.
    Vector3f relPos = (starPos.cast<double>() - obsPos).cast<float>();
    float   orbitalRadius = star.getOrbitalRadius();
    bool    hasOrbit = orbitalRadius > 0.0f;

    if (distance > distanceLimit)
        return;

    if (relPos.dot(viewNormal) > 0.0f || relPos.x() * relPos.x() < 0.1f || hasOrbit)
    {
#ifdef HDR_COMPRESS
        Color starColorFull = colorTemp->lookupColor(star.getTemperature());
        Color starColor(starColorFull.red()   * 0.5f,
                        starColorFull.green() * 0.5f,
                        starColorFull.blue()  * 0.5f);
#else
        Color starColor = colorTemp->lookupColor(star.getTemperature());
#endif
        float renderDistance = distance;
        float s = renderDistance * size;
        float discSizeInPixels = 0.0f;
        float orbitSizeInPixels = 0.0f;

        if (hasOrbit)
            orbitSizeInPixels = orbitalRadius / (distance * pixelSize);

        // Special handling for stars less than one light year away . . .
        // We can't just go ahead and render a nearby star in the usual way
        // for two reasons:
        //   * It may be clipped by the near plane
        //   * It may be large enough that we should render it as a mesh
        //     instead of a particle
        // It's possible that the second condition might apply for stars
        // further than one light year away if the star is huge, the fov is
        // very small and the resolution is high.  We'll ignore this for now
        // and use the most inexpensive test possible . . .
        if (distance < 1.0f || orbitSizeInPixels > 1.0f)
        {
            // Compute the position of the observer relative to the star.
            // This is a much more accurate (and expensive) distance
            // calculation than the previous one which used the observer's
            // position rounded off to floats.
            Vector3d hPos = astrocentricPosition(observer->getPosition(),
                                                 star,
                                                 observer->getTime());
            relPos =  hPos.cast<float>() * -astro::kilometersToLightYears(1.0f),
            distance = relPos.norm();

            // Recompute apparent magnitude using new distance computation
            appMag = astro::absToAppMag(star.getAbsoluteMagnitude(), distance);

            float f        = RenderDistance / distance;
            renderDistance = RenderDistance;
            starPos        = obsPos.cast<float>() + relPos * f;

            float radius = star.getRadius();
            discSizeInPixels = radius / astro::lightYearsToKilometers(distance) / pixelSize;
            ++nClose;
        }

        // Place labels for stars brighter than the specified label threshold brightness
        if ((labelMode & Renderer::StarLabels) && appMag < labelThresholdMag)
        {
            Vector3f starDir = relPos;
            starDir.normalize();
            if (starDir.dot(viewNormal) > cosFOV)
            {
                char nameBuffer[Renderer::MaxLabelLength];
                starDB->getStarName(star, nameBuffer, sizeof(nameBuffer), true);
                float distr = 3.5f * (labelThresholdMag - appMag)/labelThresholdMag;
                if (distr > 1.0f)
                    distr = 1.0f;
                renderer->addBackgroundAnnotation(NULL, nameBuffer,
                                                  Color(Renderer::StarLabelColor, distr * Renderer::StarLabelColor.alpha()),
                                                  relPos);
                nLabelled++;
            }
        }

        // Stars closer than the maximum solar system size are actually
        // added to the render list and depth sorted, since they may occlude
        // planets.
        if (distance > MaxSolarSystemSize)
        {
#ifdef USE_HDR
            float alpha = exposure*(faintestMag - appMag)/(faintestMag - saturationMag + 0.001f);
#else
            float alpha = (faintestMag - appMag) * brightnessScale + brightnessBias;
#endif
#ifdef DEBUG_HDR_ADAPT
            minMag = max(minMag, appMag);
            maxMag = min(maxMag, appMag);
            minAlpha = min(minAlpha, alpha);
            maxAlpha = max(maxAlpha, alpha);
            ++total;
            if (alpha > above)
            {
                ++countAboveN;
            }
#endif
            float pointSize;

            if (useScaledDiscs)
            {
                float discSize = size;
                if (alpha < 0.0f)
                {
                    alpha = 0.0f;
                }
                else if (alpha > 1.0f)
                {
                    discSize = min(discSize * (2.0f * alpha - 1.0f), maxDiscSize);
                    alpha = 1.0f;
                }
                pointSize = discSize;
            }
            else
            {
                alpha = clamp(alpha);
                pointSize = size;
#ifdef DEBUG_HDR_ADAPT
                maxSize = max(maxSize, pointSize);
#endif
            }

            if (starPrimitive == GL_POINTS)
            {
                pointStarVertexBuffer->addStar(relPos,
                                               Color(starColor, alpha),
                                               pointSize);
            }
            else
            {
                starVertexBuffer->addStar(relPos,
                                          Color(starColor, alpha),
                                          pointSize * renderDistance);
            }

            ++nRendered;

            // If the star is brighter than the saturation magnitude, add a
            // halo around it to make it appear more brilliant.  This is a
            // hack to compensate for the limited dynamic range of monitors.
            if (appMag < saturationMag)
            {
                Renderer::Particle p;
                p.center = relPos;
                p.size = size;
                p.color = Color(starColor, alpha);

                alpha = GlareOpacity * clamp((appMag - saturationMag) * -0.8f);
                s = renderDistance * 0.001f * (3 - (appMag - saturationMag)) * 2;
                if (s > p.size * 3)
                {
                    p.size = s * 2.0f/(1.0f + FOV/fov);
                }
                else
                {
                    if (s > p.size * 3)
                        p.size = s * 2.0f; //2.0f/(1.0f +FOV/fov);
                    else
                        p.size = p.size * 3;
                    p.size *= 1.6f;
                }

                p.color = Color(starColor, alpha);
                glareParticles->insert(glareParticles->end(), p);
                ++nBright;
            }
        }
        else
        {
            Matrix3f viewMat = observer->getOrientationf().toRotationMatrix();
            Vector3f viewMatZ = viewMat.row(2);

            RenderListEntry rle;
            rle.renderableType = RenderListEntry::RenderableStar;
            rle.star = &star;
            rle.isOpaque = true;

            // Objects in the render list are always rendered relative to
            // a viewer at the origin--this is different than for distant
            // stars.
            float scale = astro::lightYearsToKilometers(1.0f);
            rle.position = relPos * scale;
            rle.centerZ = rle.position.dot(viewMatZ);
            rle.distance = rle.position.norm();
            rle.radius = star.getRadius();
            rle.discSizeInPixels = discSizeInPixels;
            rle.appMag = appMag;
            renderList->push_back(rle);
        }
    }
}


class PointStarRenderer : public ObjectRenderer<Star, float>
{
 public:
    PointStarRenderer();

    void process(const Star& star, float distance, float appMag);

 public:
    Vector3d obsPos;

    vector<RenderListEntry>*         renderList;
    PointStarVertexBuffer* starVertexBuffer;
    PointStarVertexBuffer* glareVertexBuffer;

    const StarDatabase* starDB;

    bool   useScaledDiscs;
    GLenum starPrimitive;
    float  maxDiscSize;

    float cosFOV;

    const ColorTemperatureTable* colorTemp;
#ifdef DEBUG_HDR_ADAPT
    float minMag;
    float maxMag;
    float minAlpha;
    float maxAlpha;
    float maxSize;
    float above;
    unsigned long countAboveN;
    unsigned long total;
#endif
};


PointStarRenderer::PointStarRenderer() :
    ObjectRenderer<Star, float>(STAR_DISTANCE_LIMIT),
    starVertexBuffer     (NULL),
    useScaledDiscs       (false),
    maxDiscSize          (1.0f),
    cosFOV               (1.0f),
    colorTemp            (NULL)
{
}


void PointStarRenderer::process(const Star& star, float distance, float appMag)
{
    nProcessed++;

    Vector3f starPos = star.getPosition();

    // Calculate the difference at double precision *before* converting to float.
    // This is very important for stars that are far from the origin.
    Vector3f relPos = (starPos.cast<double>() - obsPos).cast<float>();
    float   orbitalRadius = star.getOrbitalRadius();
    bool    hasOrbit = orbitalRadius > 0.0f;

    if (distance > distanceLimit)
        return;


    // A very rough check to see if the star may be visible: is the star in
    // front of the viewer? If the star might be close (relPos.x^2 < 0.1) or
    // is moving in an orbit, we'll always regard it as potentially visible.
    // TODO: consider normalizing relPos and comparing relPos*viewNormal against
    // cosFOV--this will cull many more stars than relPos*viewNormal, at the
    // cost of a normalize per star.
    if (relPos.dot(viewNormal) > 0.0f || relPos.x() * relPos.x() < 0.1f || hasOrbit)
    {
#ifdef HDR_COMPRESS
        Color starColorFull = colorTemp->lookupColor(star.getTemperature());
        Color starColor(starColorFull.red()   * 0.5f,
                        starColorFull.green() * 0.5f,
                        starColorFull.blue()  * 0.5f);
#else
        Color starColor = colorTemp->lookupColor(star.getTemperature());
#endif
        float renderDistance = distance;
        /*float s = renderDistance * size;      Unused*/
        float discSizeInPixels = 0.0f;
        float orbitSizeInPixels = 0.0f;

        if (hasOrbit)
            orbitSizeInPixels = orbitalRadius / (distance * pixelSize);

        // Special handling for stars less than one light year away . . .
        // We can't just go ahead and render a nearby star in the usual way
        // for two reasons:
        //   * It may be clipped by the near plane
        //   * It may be large enough that we should render it as a mesh
        //     instead of a particle
        // It's possible that the second condition might apply for stars
        // further than one light year away if the star is huge, the fov is
        // very small and the resolution is high.  We'll ignore this for now
        // and use the most inexpensive test possible . . .
        if (distance < 1.0f || orbitSizeInPixels > 1.0f)
        {
            // Compute the position of the observer relative to the star.
            // This is a much more accurate (and expensive) distance
            // calculation than the previous one which used the observer's
            // position rounded off to floats.
            Vector3d hPos = astrocentricPosition(observer->getPosition(),
                                                 star,
                                                 observer->getTime());
            relPos = hPos.cast<float>() * -astro::kilometersToLightYears(1.0f),
            distance = relPos.norm();

            // Recompute apparent magnitude using new distance computation
            appMag = astro::absToAppMag(star.getAbsoluteMagnitude(), distance);

            float f        = RenderDistance / distance;
            renderDistance = RenderDistance;
            starPos        = obsPos.cast<float>() + relPos * f;

            float radius = star.getRadius();
            discSizeInPixels = radius / astro::lightYearsToKilometers(distance) / pixelSize;
            ++nClose;
        }

        // Place labels for stars brighter than the specified label threshold brightness
        if ((labelMode & Renderer::StarLabels) && appMag < labelThresholdMag)
        {
            Vector3f starDir = relPos;
            starDir.normalize();
            if (starDir.dot(viewNormal) > cosFOV)
            {
                char nameBuffer[Renderer::MaxLabelLength];
                starDB->getStarName(star, nameBuffer, sizeof(nameBuffer), true);
                float distr = 3.5f * (labelThresholdMag - appMag)/labelThresholdMag;
                if (distr > 1.0f)
                    distr = 1.0f;
                renderer->addBackgroundAnnotation(NULL, nameBuffer,
                                                  Color(Renderer::StarLabelColor, distr * Renderer::StarLabelColor.alpha()),
                                                  relPos);
                nLabelled++;
            }
        }

        // Stars closer than the maximum solar system size are actually
        // added to the render list and depth sorted, since they may occlude
        // planets.
        if (distance > MaxSolarSystemSize)
        {
#ifdef USE_HDR
            float satPoint = saturationMag;
            float alpha = exposure*(faintestMag - appMag)/(faintestMag - saturationMag + 0.001f);
#else
            float satPoint = faintestMag - (1.0f - brightnessBias) / brightnessScale; // TODO: precompute this value
            float alpha = (faintestMag - appMag) * brightnessScale + brightnessBias;
#endif
#ifdef DEBUG_HDR_ADAPT
            minMag = max(minMag, appMag);
            maxMag = min(maxMag, appMag);
            minAlpha = min(minAlpha, alpha);
            maxAlpha = max(maxAlpha, alpha);
            ++total;
            if (alpha > above)
            {
                ++countAboveN;
            }
#endif

            if (useScaledDiscs)
            {
                float discSize = size;
                if (alpha < 0.0f)
                {
                    alpha = 0.0f;
                }
                else if (alpha > 1.0f)
                {
                    float discScale = min(MaxScaledDiscStarSize, (float) pow(2.0f, 0.3f * (satPoint - appMag)));
                    discSize *= discScale;

                    float glareAlpha = min(0.5f, discScale / 4.0f);
                    glareVertexBuffer->addStar(relPos, Color(starColor, glareAlpha), discSize * 3.0f);

                    alpha = 1.0f;
                }
                starVertexBuffer->addStar(relPos, Color(starColor, alpha), discSize);
            }
            else
            {
                if (alpha < 0.0f)
                {
                    alpha = 0.0f;
                }
                else if (alpha > 1.0f)
                {
                    float discScale = min(100.0f, satPoint - appMag + 2.0f);
                    float glareAlpha = min(GlareOpacity, (discScale - 2.0f) / 4.0f);
                    glareVertexBuffer->addStar(relPos, Color(starColor, glareAlpha), 2.0f * discScale * size);
#ifdef DEBUG_HDR_ADAPT
                    maxSize = max(maxSize, 2.0f * discScale * size);
#endif
                }
                starVertexBuffer->addStar(relPos, Color(starColor, alpha), size);
            }

            ++nRendered;
        }
        else
        {
            Matrix3f viewMat = observer->getOrientationf().toRotationMatrix();
            Vector3f viewMatZ = viewMat.row(2);

            RenderListEntry rle;
            rle.renderableType = RenderListEntry::RenderableStar;
            rle.star = &star;

            // Objects in the render list are always rendered relative to
            // a viewer at the origin--this is different than for distant
            // stars.
            float scale = astro::lightYearsToKilometers(1.0f);
            rle.position = relPos * scale;
            rle.centerZ = rle.position.dot(viewMatZ);
            rle.distance = rle.position.norm();
            rle.radius = star.getRadius();
            rle.discSizeInPixels = discSizeInPixels;
            rle.appMag = appMag;
            rle.isOpaque = true;
            renderList->insert(renderList->end(), rle);
        }
    }
}


// Calculate the maximum field of view (from top left corner to bottom right) of
// a frustum with the specified aspect ratio (width/height) and vertical field of
// view. We follow the convention used elsewhere and use units of degrees for
// the field of view angle.
static double calcMaxFOV(double fovY_degrees, double aspectRatio)
{
    double l = 1.0 / tan(degToRad(fovY_degrees / 2.0));
    return radToDeg(atan(sqrt(aspectRatio * aspectRatio + 1.0) / l)) * 2.0;
}


void Renderer::renderStars(const StarDatabase& starDB,
                           float faintestMagNight,
                           const Observer& observer)
{
    StarRenderer starRenderer;
    Vector3d obsPos = observer.getPosition().toLy();


    starRenderer.context          = context;
    starRenderer.renderer         = this;
    starRenderer.starDB           = &starDB;
    starRenderer.observer         = &observer;
    starRenderer.obsPos           = obsPos;
    starRenderer.viewNormal       = observer.getOrientationf().conjugate() * -Vector3f::UnitZ();
    starRenderer.glareParticles   = &glareParticles;
    starRenderer.renderList       = &renderList;
    starRenderer.starVertexBuffer = starVertexBuffer;
    starRenderer.pointStarVertexBuffer = pointStarVertexBuffer;
    starRenderer.fov              = fov;
    starRenderer.cosFOV            = (float) cos(degToRad(calcMaxFOV(fov, (float) windowWidth / (float) windowHeight)) / 2.0f);

    // size/pixelSize =0.86 at 120deg, 1.43 at 45deg and 1.6 at 0deg.
    starRenderer.size             = pixelSize * 1.6f / corrFac;
    starRenderer.pixelSize        = pixelSize;
    starRenderer.brightnessScale  = brightnessScale * corrFac;
    starRenderer.brightnessBias   = brightnessBias;
    starRenderer.faintestMag      = faintestMag;
    starRenderer.faintestMagNight = faintestMagNight;
    starRenderer.saturationMag    = saturationMag;
#ifdef USE_HDR
    starRenderer.exposure         = exposure + brightPlus;
#endif
#ifdef DEBUG_HDR_ADAPT
    starRenderer.minMag = -100.f;
    starRenderer.maxMag =  100.f;
    starRenderer.minAlpha = 1.f;
    starRenderer.maxAlpha = 0.f;
    starRenderer.maxSize  = 0.f;
    starRenderer.above    = 1.f;
    starRenderer.countAboveN = 0L;
    starRenderer.total       = 0L;
#endif
    starRenderer.distanceLimit    = distanceLimit;
    starRenderer.labelMode        = labelMode;

    // = 1.0 at startup
    float effDistanceToScreen = mmToInches((float) REF_DISTANCE_TO_SCREEN) * pixelSize * getScreenDpi();
    starRenderer.labelThresholdMag = max(1.0f, (faintestMag - 4.0f) * (1.0f - 0.5f * (float) log10(effDistanceToScreen)));

    if (starStyle == PointStars || useNewStarRendering)
    {
        starRenderer.starPrimitive = GL_POINTS;
        //starRenderer.size          = 3.2f;
    }
    else
    {
        starRenderer.starPrimitive = GL_QUADS;
    }

    if (starStyle == ScaledDiscStars)
    {
        starRenderer.useScaledDiscs = true;
        starRenderer.brightnessScale *= 2.0f;
        starRenderer.maxDiscSize = starRenderer.size * MaxScaledDiscStarSize;
    }

    starRenderer.colorTemp = colorTemp;

    glareParticles.clear();

    starVertexBuffer->setBillboardOrientation(observer.getOrientationf());

    glEnable(GL_TEXTURE_2D);

    if (useNewStarRendering)
        gaussianDiscTex->bind();
    else
        starTex->bind();
    if (starRenderer.starPrimitive == GL_POINTS)
    {
        // Point primitives (either real points or point sprites)
        if (starStyle == PointStars)
            starRenderer.pointStarVertexBuffer->startPoints(*context);
        else
            starRenderer.pointStarVertexBuffer->startSprites(*context);
    }
    else
    {
        // Use quad primitives
        starRenderer.starVertexBuffer->start();
    }
    starDB.findVisibleStars(starRenderer,
                            obsPos.cast<float>(),
                            observer.getOrientationf(),
                            degToRad(fov),
                            (float) windowWidth / (float) windowHeight,
                            faintestMagNight);
#ifdef DEBUG_HDR_ADAPT
  HDR_LOG <<
      "* minMag = "    << starRenderer.minMag << ", " <<
      "maxMag = "      << starRenderer.maxMag << ", " <<
      "percent above " << starRenderer.above << " = " <<
      (100.0*(double)starRenderer.countAboveN/(double)starRenderer.total) << endl;
#endif

    if (starRenderer.starPrimitive == GL_POINTS)
        starRenderer.pointStarVertexBuffer->finish();
    else
        starRenderer.starVertexBuffer->finish();

    gaussianGlareTex->bind();
    renderParticles(glareParticles, observer.getOrientationf());
}


void Renderer::renderPointStars(const StarDatabase& starDB,
                                float faintestMagNight,
                                const Observer& observer)
{
    Vector3d obsPos = observer.getPosition().toLy();

    PointStarRenderer starRenderer;
    starRenderer.context           = context;
    starRenderer.renderer          = this;
    starRenderer.starDB            = &starDB;
    starRenderer.observer          = &observer;
    starRenderer.obsPos            = obsPos;
    starRenderer.viewNormal        = observer.getOrientationf().conjugate() * -Vector3f::UnitZ();
    starRenderer.renderList        = &renderList;
    starRenderer.starVertexBuffer  = pointStarVertexBuffer;
    starRenderer.glareVertexBuffer = glareVertexBuffer;
    starRenderer.fov               = fov;
    starRenderer.cosFOV            = (float) cos(degToRad(calcMaxFOV(fov, (float) windowWidth / (float) windowHeight)) / 2.0f);

    starRenderer.pixelSize         = pixelSize;
    starRenderer.brightnessScale   = brightnessScale * corrFac;
    starRenderer.brightnessBias    = brightnessBias;
    starRenderer.faintestMag       = faintestMag;
    starRenderer.faintestMagNight  = faintestMagNight;
    starRenderer.saturationMag     = saturationMag;
#ifdef USE_HDR
    starRenderer.exposure          = exposure + brightPlus;
#endif
#ifdef DEBUG_HDR_ADAPT
    starRenderer.minMag = -100.f;
    starRenderer.maxMag =  100.f;
    starRenderer.minAlpha = 1.f;
    starRenderer.maxAlpha = 0.f;
    starRenderer.maxSize  = 0.f;
    starRenderer.above    = 1.0f;
    starRenderer.countAboveN = 0L;
    starRenderer.total       = 0L;
#endif
    starRenderer.distanceLimit     = distanceLimit;
    starRenderer.labelMode         = labelMode;

    // = 1.0 at startup
    float effDistanceToScreen = mmToInches((float) REF_DISTANCE_TO_SCREEN) * pixelSize * getScreenDpi();
    starRenderer.labelThresholdMag = 1.2f * max(1.0f, (faintestMag - 4.0f) * (1.0f - 0.5f * (float) log10(effDistanceToScreen)));

    starRenderer.size          = BaseStarDiscSize;
    if (starStyle == ScaledDiscStars)
    {
        starRenderer.useScaledDiscs = true;
        starRenderer.brightnessScale *= 2.0f;
        starRenderer.maxDiscSize = starRenderer.size * MaxScaledDiscStarSize;
    }
    else if (starStyle == FuzzyPointStars)
    {
        starRenderer.brightnessScale *= 1.0f;
    }

    starRenderer.colorTemp = colorTemp;

    glEnable(GL_TEXTURE_2D);
    gaussianDiscTex->bind();
    starRenderer.starVertexBuffer->setTexture(gaussianDiscTex);
    starRenderer.glareVertexBuffer->setTexture(gaussianGlareTex);

    starRenderer.glareVertexBuffer->startSprites(*context);
    if (starStyle == PointStars)
        starRenderer.starVertexBuffer->startPoints(*context);
    else
        starRenderer.starVertexBuffer->startSprites(*context);

    starDB.findVisibleStars(starRenderer,
                            obsPos.cast<float>(),
                            observer.getOrientationf(),
                            degToRad(fov),
                            (float) windowWidth / (float) windowHeight,
                            faintestMagNight);

    starRenderer.starVertexBuffer->render();
    starRenderer.glareVertexBuffer->render();
    starRenderer.starVertexBuffer->finish();
    starRenderer.glareVertexBuffer->finish();
}


class DSORenderer : public ObjectRenderer<DeepSkyObject*, double>
{
 public:
    DSORenderer();

    void process(DeepSkyObject* const &, double, float);

 public:
    Vector3d     obsPos;
    DSODatabase* dsoDB;
    Frustum      frustum;

    Matrix3f orientationMatrix;

    int wWidth;
    int wHeight;

    double avgAbsMag;

    unsigned int dsosProcessed;
};


DSORenderer::DSORenderer() :
    ObjectRenderer<DeepSkyObject*, double>(DSO_OCTREE_ROOT_SIZE),
    frustum(degToRad(45.0f), 1.0f, 1.0f),
    dsosProcessed(0)
{
}


void DSORenderer::process(DeepSkyObject* const & dso,
                          double distanceToDSO,
                          float  absMag)
{
    if (distanceToDSO > distanceLimit)
        return;
    
    Vector3d dsoPos = dso->getPosition();
    Vector3f relPos = (dsoPos - obsPos).cast<float>();

    Vector3f center = orientationMatrix.transpose() * relPos;
	
	double enhance = 4.0, pc10 = 32.6167;
	
	// The parameter 'enhance' adjusts the DSO brightness as viewed from "inside" 
	// (e.g. MilkyWay as seen from Earth). It provides an enhanced apparent  core  
	// brightness appMag  ~ absMag - enhance. 'enhance' thus serves to uniformly  
	// enhance the too low sprite luminosity at close distance.

    float  appMag = (distanceToDSO >= pc10)? (float) astro::absToAppMag((double) absMag, distanceToDSO): absMag + (float) (enhance * tanh(distanceToDSO/pc10 - 1.0));
    
	// Test the object's bounding sphere against the view frustum. If we
    // avoid this stage, overcrowded octree cells may hit performance badly:
    // each object (even if it's not visible) would be sent to the OpenGL
    // pipeline.
    
    if (dso->isVisible())
    {
        double dsoRadius = dso->getBoundingSphereRadius(); 
        bool inFrustum = frustum.testSphere(center, (float) dsoRadius) != Frustum::Outside;

        if (inFrustum)
        {
            if ((renderFlags & dso->getRenderMask()))
            {
                dsosProcessed++;

                // Input: display looks satisfactory for 0.2 < brightness < O(1.0)
                // Ansatz: brightness = a - b * appMag(distanceToDSO), emulating eye sensitivity...
                // determine a,b such that
                // a - b * absMag = absMag / avgAbsMag ~ 1; a - b * faintestMag = 0.2.
                // The 2nd eq. guarantees that the faintest galaxies are still visible.            
                
			    if(!strcmp(dso->getObjTypeName(),"globular"))		
				    avgAbsMag =  -6.86;    // average over 150  globulars in globulars.dsc.
			    else if (!strcmp(dso->getObjTypeName(),"galaxy"))
				    avgAbsMag = -19.04;    // average over 10937 galaxies in galaxies.dsc.
                
    			
                float r   = absMag / (float) avgAbsMag;			  
			    float brightness = r - (r - 0.2f) * (absMag - appMag) / (absMag - faintestMag);
    	
			    // obviously, brightness(appMag = absMag) = r and 
			    // brightness(appMag = faintestMag) = 0.2, as desired.

			    brightness = 2.3f * brightness * (faintestMag - 4.75f) / renderer->getFaintestAM45deg();

#ifdef USE_HDR
                brightness *= exposure;
#endif
                if (brightness < 0)
				    brightness = 0;
    						
			    if (dsoRadius < 1000.0)
                {
                    // Small objects may be prone to clipping; give them special
                    // handling.  We don't want to always set the projection
                    // matrix, since that could be expensive with large galaxy
                    // catalogs.
                    float nearZ = (float) (distanceToDSO / 2);
                    float farZ  = (float) (distanceToDSO + dsoRadius * 2 * CubeCornerToCenterDistance);
                    if (nearZ < dsoRadius * 0.001)
                    {
                        nearZ = (float) (dsoRadius * 0.001);
                        farZ  = nearZ * 10000.0f;
                    }

                    glMatrixMode(GL_PROJECTION);
                    glPushMatrix();
                    glLoadIdentity();
                    gluPerspective(fov,
                                   (float) wWidth / (float) wHeight,
                                   nearZ,
                                   farZ);
                    glMatrixMode(GL_MODELVIEW);
                }

                glPushMatrix();
                glTranslate(relPos);

                dso->render(*context,
                            relPos,
                            observer->getOrientationf(),
                            (float) brightness,
                            pixelSize);
                glPopMatrix();

    #if 1
                if (dsoRadius < 1000.0)
                {
                    glMatrixMode(GL_PROJECTION);
                    glPopMatrix();
                    glMatrixMode(GL_MODELVIEW);
                }
    #endif
            } // renderFlags check

            // Only render those labels that are in front of the camera:
            // Place labels for DSOs brighter than the specified label threshold brightness
            //
            unsigned int labelMask = dso->getLabelMask();

            if ((labelMask & labelMode))
            {
                Color labelColor;
                float appMagEff = 6.0f;
                float step = 6.0f;
                float symbolSize = 0.0f;
                MarkerRepresentation* rep = NULL;

                // Use magnitude based fading for galaxies, and distance based
                // fading for nebulae and open clusters.
                switch (labelMask)
                {
                case Renderer::NebulaLabels:
                    rep = &renderer->nebulaRep;
                    labelColor = Renderer::NebulaLabelColor;
                    appMagEff = astro::absToAppMag(-7.5f, (float) distanceToDSO);
                    symbolSize = (float) (dso->getRadius() / distanceToDSO) / pixelSize;
                    step = 6.0f;
                    break;
                case Renderer::OpenClusterLabels:
                    rep = &renderer->openClusterRep;
                    labelColor = Renderer::OpenClusterLabelColor;
                    appMagEff = astro::absToAppMag(-6.0f, (float) distanceToDSO);
                    symbolSize = (float) (dso->getRadius() / distanceToDSO) / pixelSize;
                    step = 4.0f;
                    break;
                case Renderer::GalaxyLabels:
                    labelColor = Renderer::GalaxyLabelColor;
                    appMagEff = appMag;
                    step = 6.0f;
                    break;
		        case Renderer::GlobularLabels:
                    labelColor = Renderer::GlobularLabelColor;
                    appMagEff = appMag;
                    step = 3.0f;
                    break;	        			
                default:
                    // Unrecognized object class
                    labelColor = Color::White;
                    appMagEff = appMag;
                    step = 6.0f;
                    break;
                }

                if (appMagEff < labelThresholdMag)
                {
                    // introduce distance dependent label transparency.
                    float distr = step * (labelThresholdMag - appMagEff) / labelThresholdMag;
                    if (distr > 1.0f)
                        distr = 1.0f;

                    renderer->addBackgroundAnnotation(rep,
                                                      dsoDB->getDSOName(dso, true),
                                                      Color(labelColor, distr * labelColor.alpha()),
                                                      relPos,
                                                      Renderer::AlignLeft, Renderer::VerticalAlignCenter, symbolSize);
                }
            } // labels enabled
        } // in frustum
    } // isVisible()
}


void Renderer::renderDeepSkyObjects(const Universe&  universe,
                                    const Observer& observer,
                                    const float     faintestMagNight)
{
    DSORenderer dsoRenderer;

    Vector3d obsPos    = observer.getPosition().toLy();

    DSODatabase* dsoDB  = universe.getDSOCatalog();

    dsoRenderer.context          = context;
    dsoRenderer.renderer         = this;
    dsoRenderer.dsoDB            = dsoDB;
    dsoRenderer.orientationMatrix = observer.getOrientationf().conjugate().toRotationMatrix();
    dsoRenderer.observer          = &observer;
    dsoRenderer.obsPos            = obsPos;
    dsoRenderer.viewNormal        = observer.getOrientationf().conjugate() * -Vector3f::UnitZ();
    dsoRenderer.fov              = fov;
    // size/pixelSize =0.86 at 120deg, 1.43 at 45deg and 1.6 at 0deg.
    dsoRenderer.size             = pixelSize * 1.6f / corrFac;
    dsoRenderer.pixelSize        = pixelSize;
    dsoRenderer.brightnessScale  = brightnessScale * corrFac;
    dsoRenderer.brightnessBias   = brightnessBias;
    dsoRenderer.avgAbsMag        = dsoDB->getAverageAbsoluteMagnitude();
    dsoRenderer.faintestMag      = faintestMag;
    dsoRenderer.faintestMagNight = faintestMagNight;
    dsoRenderer.saturationMag    = saturationMag;
#ifdef USE_HDR
    dsoRenderer.exposure         = exposure + brightPlus;
#endif
    dsoRenderer.renderFlags      = renderFlags;
    dsoRenderer.labelMode        = labelMode;
    dsoRenderer.wWidth           = windowWidth;
    dsoRenderer.wHeight          = windowHeight;

    dsoRenderer.frustum = Frustum(degToRad(fov),
                        (float) windowWidth / (float) windowHeight,
                        MinNearPlaneDistance);
    // Use pixelSize * screenDpi instead of FoV, to eliminate windowHeight dependence.
    // = 1.0 at startup
    float effDistanceToScreen = mmToInches((float) REF_DISTANCE_TO_SCREEN) * pixelSize * getScreenDpi();

    dsoRenderer.labelThresholdMag = 2.0f * max(1.0f, (faintestMag - 4.0f) * (1.0f - 0.5f * (float) log10(effDistanceToScreen)));

    galaxyRep      = MarkerRepresentation(MarkerRepresentation::Triangle, 8.0f, GalaxyLabelColor);
    nebulaRep      = MarkerRepresentation(MarkerRepresentation::Square,   8.0f, NebulaLabelColor);
    openClusterRep = MarkerRepresentation(MarkerRepresentation::Circle,   8.0f, OpenClusterLabelColor);
    globularRep    = MarkerRepresentation(MarkerRepresentation::Circle,   8.0f, OpenClusterLabelColor);

    // Render any line primitives with smooth lines
    // (mostly to make graticules look good.)
    if ((renderFlags & ShowSmoothLines) != 0)
        enableSmoothLines();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    dsoDB->findVisibleDSOs(dsoRenderer,
                           obsPos,
                           observer.getOrientationf(),
                           degToRad(fov),
                           (float) windowWidth / (float) windowHeight,
                           2 * faintestMagNight);

    // clog << "DSOs processed: " << dsoRenderer.dsosProcessed << endl;

    if ((renderFlags & ShowSmoothLines) != 0)
        disableSmoothLines();
}


static Vector3d toStandardCoords(const Vector3d& v)
{
    return Vector3d(v.x(), -v.z(), v.y());
}


void Renderer::renderSkyGrids(const Observer& observer)
{
    if (renderFlags & ShowCelestialSphere)
    {
        SkyGrid grid;
        grid.setOrientation(Quaterniond(AngleAxis<double>(astro::J2000Obliquity, Vector3d::UnitX())));
        grid.setLineColor(EquatorialGridColor);
        grid.setLabelColor(EquatorialGridLabelColor);
        grid.render(*this, observer, windowWidth, windowHeight);
    }

    if (renderFlags & ShowGalacticGrid)
    {
        SkyGrid galacticGrid;
        galacticGrid.setOrientation((astro::eclipticToEquatorial() * astro::equatorialToGalactic()).conjugate());
        galacticGrid.setLineColor(GalacticGridColor);
        galacticGrid.setLabelColor(GalacticGridLabelColor);
        galacticGrid.setLongitudeUnits(SkyGrid::LongitudeDegrees);
        galacticGrid.render(*this, observer, windowWidth, windowHeight);
    }

    if (renderFlags & ShowEclipticGrid)
    {
        SkyGrid grid;
        grid.setOrientation(Quaterniond::Identity());
        grid.setLineColor(EclipticGridColor);
        grid.setLabelColor(EclipticGridLabelColor);
        grid.setLongitudeUnits(SkyGrid::LongitudeDegrees);
        grid.render(*this, observer, windowWidth, windowHeight);
    }

    if (renderFlags & ShowHorizonGrid)
    {
        double tdb = observer.getTime();
        const ObserverFrame* frame = observer.getFrame();
        Body* body = frame->getRefObject().body();
        
        if (body)
        {
            SkyGrid grid;
            grid.setLineColor(HorizonGridColor);
            grid.setLabelColor(HorizonGridLabelColor);
            grid.setLongitudeUnits(SkyGrid::LongitudeDegrees);
            grid.setLongitudeDirection(SkyGrid::IncreasingClockwise);
            
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
                grid.setOrientation(Quaterniond(m));
                
                grid.render(*this, observer, windowWidth, windowHeight);
            }
        }
    }

    if (renderFlags & ShowEcliptic)
    {
        // Draw the J2000.0 ecliptic; trivial, since this forms the basis for
        // Celestia's coordinate system.
        const int subdivision = 200;
        glColor(EclipticColor);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < subdivision; i++)
        {
            double theta = (double) i / (double) subdivision * 2 * PI;
            glVertex3f((float) cos(theta) * 1000.0f, 0.0f, (float) sin(theta) * 1000.0f);
        }
        glEnd();
    }
}


/*! Draw an arrow at the view border pointing to an offscreen selection. This method
 *  should only be called when the selection lies outside the view frustum.
 */
void Renderer::renderSelectionPointer(const Observer& observer,
                                      double now,
                                      const Frustum& viewFrustum,
                                      const Selection& sel)
{
    const float cursorDistance = 20.0f;
    if (sel.empty())
        return;

    Matrix3f cameraMatrix = observer.getOrientationf().conjugate().toRotationMatrix();
    Vector3f u = cameraMatrix * Vector3f::UnitX();
    Vector3f v = cameraMatrix * Vector3f::UnitY();

    // Get the position of the cursor relative to the eye
    Vector3d position = sel.getPosition(now).offsetFromKm(observer.getPosition());
    double distance = position.norm();
    bool isVisible = viewFrustum.testSphere(position, sel.radius()) != Frustum::Outside;
    position *= cursorDistance / distance;

#ifdef USE_HDR
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
#endif
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!isVisible)
    {
        double viewAspectRatio = (double) windowWidth / (double) windowHeight;
        double vfov = observer.getFOV();
        float h = (float) tan(vfov / 2);
        float w = (float) (h * viewAspectRatio);
        float diag = std::sqrt(h * h + w * w);

        Vector3f posf = position.cast<float>();
        posf *= (1.0f / cursorDistance);
        float x = u.dot(posf);
        float y = v.dot(posf);
        float angle = std::atan2(y, x);
        float c = std::cos(angle);
        float s = std::sin(angle);

        float t = 1.0f;
        float x0 = c * diag;
        float y0 = s * diag;
        if (std::abs(x0) < w)
            t = h / std::abs(y0);
        else
            t = w / std::abs(x0);
        x0 *= t;
        y0 *= t;
        glColor(SelectionCursorColor, 0.6f);
        Vector3f center = -cameraMatrix * Vector3f::UnitZ();

        glPushMatrix();
        glTranslatef(center.x(), center.y(), center.z());

        Vector3f p0(Vector3f::Zero());
        Vector3f p1(-20.0f * pixelSize,  6.0f * pixelSize, 0.0f);
        Vector3f p2(-20.0f * pixelSize, -6.0f * pixelSize, 0.0f);

        glBegin(GL_TRIANGLES);
        glVertex((p0.x() * c - p0.y() * s + x0) * u + (p0.x() * s + p0.y() * c + y0) * v); 
        glVertex((p1.x() * c - p1.y() * s + x0) * u + (p1.x() * s + p1.y() * c + y0) * v); 
        glVertex((p2.x() * c - p2.y() * s + x0) * u + (p2.x() * s + p2.y() * c + y0) * v); 
        glEnd();

        glPopMatrix();
    }

#ifdef USE_HDR
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#endif

    glEnable(GL_TEXTURE_2D);
}


void Renderer::labelConstellations(const AsterismList& asterisms,
                                   const Observer& observer)
{
    Vector3f observerPos = observer.getPosition().toLy().cast<float>();

    for (AsterismList::const_iterator iter = asterisms.begin();
         iter != asterisms.end(); iter++)
    {
        Asterism* ast = *iter;
        if (ast->getChainCount() > 0 && ast->getActive())
        {
            const Asterism::Chain& chain = ast->getChain(0);

            if (chain.size() > 0)
            {
                // The constellation label is positioned at the average
                // position of all stars in the first chain.  This usually
                // gives reasonable results.
                Vector3f avg = Vector3f::Zero();
                for (Asterism::Chain::const_iterator iter = chain.begin();
                     iter != chain.end(); iter++)
                    avg += *iter;

                avg = avg / (float) chain.size();

                // Draw all constellation labels at the same distance
                avg.normalize();
                avg = avg * 1.0e4f;

                Vector3f rpos = avg - observerPos;

                if ((observer.getOrientationf() * rpos).z() < 0)
                {
                    // We'll linearly fade the labels as a function of the
                    // observer's distance to the origin of coordinates:
                    float opacity = 1.0f;
                    float dist = observerPos.norm();
                    if (dist > MaxAsterismLabelsConstDist)
                    {
                        opacity = clamp((MaxAsterismLabelsConstDist - dist) /
                                        (MaxAsterismLabelsDist - MaxAsterismLabelsConstDist) + 1);
                    }

                    // Use the default label color unless the constellation has an
                    // override color set.
                    Color labelColor = ConstellationLabelColor;
                    if (ast->isColorOverridden())
                        labelColor = ast->getOverrideColor();

                    addBackgroundAnnotation(NULL,
                                            ast->getName((labelMode & I18nConstellationLabels) != 0),
                                            Color(labelColor, opacity),
                                            rpos,
                                            AlignCenter, VerticalAlignCenter);
                }
            }
        }
    }
}


void Renderer::renderParticles(const vector<Particle>& particles,
                               const Quaternionf& orientation)
{
    int nParticles = particles.size();

    {
        Matrix3f m = orientation.conjugate().toRotationMatrix();
        Vector3f v0 = m * Vector3f(-1, -1, 0);
        Vector3f v1 = m * Vector3f( 1, -1, 0);
        Vector3f v2 = m * Vector3f( 1,  1, 0);
        Vector3f v3 = m * Vector3f(-1,  1, 0);

        glBegin(GL_QUADS);
        for (int i = 0; i < nParticles; i++)
        {
            Vector3f center = particles[i].center;
            float size = particles[i].size;

            glColor(particles[i].color);
            glTexCoord2f(0, 1);
            glVertex(center + (v0 * size));
            glTexCoord2f(1, 1);
            glVertex(center + (v1 * size));
            glTexCoord2f(1, 0);
            glVertex(center + (v2 * size));
            glTexCoord2f(0, 0);
            glVertex(center + (v3 * size));
        }
        glEnd();
    }
}


static void renderCrosshair(float pixelSize, double tsec)
{
    const float cursorMinRadius = 6.0f;
    const float cursorRadiusVariability = 4.0f;
    const float minCursorWidth = 7.0f;
    const float cursorPulsePeriod = 1.5f;

    float selectionSizeInPixels = pixelSize;
    float cursorRadius = selectionSizeInPixels + cursorMinRadius;
    cursorRadius += cursorRadiusVariability * (float) (0.5 + 0.5 * std::sin(tsec * 2 * PI / cursorPulsePeriod));

    // Enlarge the size of the cross hair sligtly when the selection
    // has a large apparent size
    float cursorGrow = max(1.0f, min(2.5f, (selectionSizeInPixels - 10.0f) / 100.0f));

    float h = 2.0f * cursorGrow;
    float cursorWidth = minCursorWidth * cursorGrow;
    float r0 = cursorRadius;
    float r1 = cursorRadius + cursorWidth;

    const unsigned int markCount = 4;
    Vector3f p0(r0, 0.0f, 0.0f);
    Vector3f p1(r1, -h, 0.0f);
    Vector3f p2(r1,  h, 0.0f);

    glBegin(GL_TRIANGLES);
    for (unsigned int i = 0; i < markCount; i++)
    {
        float theta = (float) (PI / 4.0) + (float) i / (float) markCount * (float) (2 * PI);
        Matrix3f rotation = AngleAxisf(theta, Vector3f::UnitZ()).toRotationMatrix();
        glVertex(rotation * p0);
        glVertex(rotation * p1);
        glVertex(rotation * p2);
    }
    glEnd();
}


void Renderer::renderAnnotations(const vector<Annotation>& annotations, FontStyle fs)
{
    if (font[fs] == NULL)
        return;

    // Enable line smoothing for rendering symbols
    if ((renderFlags & ShowSmoothLines) != 0)
        enableSmoothLines();
    
#ifdef USE_HDR
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
#endif
    glEnable(GL_TEXTURE_2D);
    font[fs]->bind();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    for (int i = 0; i < (int) annotations.size(); i++)
    {
        if (annotations[i].markerRep != NULL)
        {
            glPushMatrix();
            const MarkerRepresentation& markerRep = *annotations[i].markerRep;

            float size = markerRep.size();
            if (annotations[i].size > 0.0f)
            {
                size = annotations[i].size;
            }

            glColor(annotations[i].color);
            glTranslatef((GLfloat) (int) annotations[i].position.x(),
                         (GLfloat) (int) annotations[i].position.y(), 0.0f);

            glDisable(GL_TEXTURE_2D);
            if (markerRep.symbol() == MarkerRepresentation::Crosshair)
                renderCrosshair(size, realTime);
            else
                markerRep.render(size);
            glEnable(GL_TEXTURE_2D);
            
            if (!markerRep.label().empty())
            {
                int labelOffset = (int) markerRep.size() / 2;
                glTranslatef(labelOffset + PixelOffset, -labelOffset - font[fs]->getHeight() + PixelOffset, 0.0f);
                font[fs]->render(markerRep.label(), 0.0f, 0.0f);
            }  
            glPopMatrix();
        }

        if (annotations[i].labelText[0] != '\0')
        {
            glPushMatrix();
            int labelWidth = 0;
            int hOffset = 2;
            int vOffset = 0;
            
            switch (annotations[i].halign)
            {
            case AlignCenter:
                labelWidth = (font[fs]->getWidth(annotations[i].labelText));
                hOffset = -labelWidth / 2;
                break;
            
            case AlignRight:
                labelWidth = (font[fs]->getWidth(annotations[i].labelText));
                hOffset = -(labelWidth + 2);
                break;
            
            case AlignLeft:
                if (annotations[i].markerRep != NULL)
                    hOffset = 2 + (int) annotations[i].markerRep->size() / 2;
                break;
            }
            
            switch (annotations[i].valign)
            {
            case AlignCenter:
                vOffset = -font[fs]->getHeight() / 2;
                break;
            case VerticalAlignTop:
                vOffset = -font[fs]->getHeight();
                break;
            case VerticalAlignBottom:
                vOffset = 0;
                break;
            }
            
            glColor(annotations[i].color);
            glTranslatef((int) annotations[i].position.x() + hOffset + PixelOffset,
                         (int) annotations[i].position.y() + vOffset + PixelOffset, 0.0f);
            // EK TODO: Check where to replace (see '_(' above)
            font[fs]->render(annotations[i].labelText, 0.0f, 0.0f);
            glPopMatrix();
        }
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
#ifdef USE_HDR
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#endif

    if ((renderFlags & ShowSmoothLines) != 0)
        disableSmoothLines();
}


void
Renderer::renderBackgroundAnnotations(FontStyle fs)
{
    glEnable(GL_DEPTH_TEST);
    renderAnnotations(backgroundAnnotations, fs);
    glDisable(GL_DEPTH_TEST);
    
    clearAnnotations(backgroundAnnotations);
}


void
Renderer::renderForegroundAnnotations(FontStyle fs)
{
    glDisable(GL_DEPTH_TEST);
    renderAnnotations(foregroundAnnotations, fs);
    
    clearAnnotations(foregroundAnnotations);
}


vector<Renderer::Annotation>::iterator
Renderer::renderSortedAnnotations(vector<Annotation>::iterator iter,
                                  float nearDist,
                                  float farDist,
                                  FontStyle fs)
{
    if (font[fs] == NULL)
        return iter;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    font[fs]->bind();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Precompute values that will be used to generate the normalized device z value;
    // we're effectively just handling the projection instead of OpenGL. We use an orthographic
    // projection matrix in order to get the label text position exactly right but need to mimic
    // the depth coordinate generation of a perspective projection.
    float d1 = -(farDist + nearDist) / (farDist - nearDist);
    float d2 = -2.0f * nearDist * farDist / (farDist - nearDist);

    for (; iter != depthSortedAnnotations.end() && iter->position.z() > nearDist; iter++)
    {
        // Compute normalized device z
        float ndc_z = d1 + d2 / -iter->position.z();
        ndc_z = min(1.0f, max(-1.0f, ndc_z)); // Clamp to [-1,1]

        // Offsets to left align label
        int labelHOffset = 0;
        int labelVOffset = 0;

        glPushMatrix();
        if (iter->markerRep != NULL)
        {
            const MarkerRepresentation& markerRep = *iter->markerRep;
            float size = markerRep.size();
            if (iter->size > 0.0f)
            {
                size = iter->size;
            }
            
            glTranslatef((GLfloat) (int) iter->position.x(), (GLfloat) (int) iter->position.y(), ndc_z);
            glColor(iter->color);
                        
            glDisable(GL_TEXTURE_2D);
            if (markerRep.symbol() == MarkerRepresentation::Crosshair)
                renderCrosshair(size, realTime);
            else
                markerRep.render(size);
            glEnable(GL_TEXTURE_2D);            
            
            if (!markerRep.label().empty())
            {
                int labelOffset = (int) markerRep.size() / 2;
                glTranslatef(labelOffset + PixelOffset, -labelOffset - font[fs]->getHeight() + PixelOffset, 0.0f);
                font[fs]->render(markerRep.label(), 0.0f, 0.0f);
            }
        }
        else
        {
            glTranslatef((int) iter->position.x() + PixelOffset + labelHOffset,
                         (int) iter->position.y() + PixelOffset + labelVOffset,
                         ndc_z);
            glColor(iter->color);
            font[fs]->render(iter->labelText, 0.0f, 0.0f);
        }
        glPopMatrix();
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_DEPTH_TEST);

    return iter;
}


vector<Renderer::Annotation>::iterator
Renderer::renderAnnotations(vector<Annotation>::iterator startIter,
                            vector<Annotation>::iterator endIter,
                            float nearDist,
                            float farDist,
                            FontStyle fs)
{
    if (font[fs] == NULL)
        return endIter;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    font[fs]->bind();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Precompute values that will be used to generate the normalized device z value;
    // we're effectively just handling the projection instead of OpenGL. We use an orthographic
    // projection matrix in order to get the label text position exactly right but need to mimic
    // the depth coordinate generation of a perspective projection.
    float d1 = -(farDist + nearDist) / (farDist - nearDist);
    float d2 = -2.0f * nearDist * farDist / (farDist - nearDist);

    vector<Annotation>::iterator iter = startIter;
    for (; iter != endIter && iter->position.z() > nearDist; iter++)
    {
        // Compute normalized device z
        float ndc_z = d1 + d2 / -iter->position.z();
        ndc_z = min(1.0f, max(-1.0f, ndc_z)); // Clamp to [-1,1]

        // Offsets to left align label
        int labelHOffset = 0;
        int labelVOffset = 0;

        if (iter->markerRep != NULL)
        {
            glPushMatrix();
            const MarkerRepresentation& markerRep = *iter->markerRep;
            float size = markerRep.size();
            if (iter->size > 0.0f)
            {
                size = iter->size;
            }
            
            glTranslatef((GLfloat) (int) iter->position.x(), (GLfloat) (int) iter->position.y(), ndc_z);
            glColor(iter->color);
                        
            glDisable(GL_TEXTURE_2D);
            if (markerRep.symbol() == MarkerRepresentation::Crosshair)
                renderCrosshair(size, realTime);
            else
                markerRep.render(size);
            glEnable(GL_TEXTURE_2D);            
            
            if (!markerRep.label().empty())
            {
                int labelOffset = (int) markerRep.size() / 2;
                glTranslatef(labelOffset + PixelOffset, -labelOffset - font[fs]->getHeight() + PixelOffset, 0.0f);
                font[fs]->render(markerRep.label(), 0.0f, 0.0f);
            }
            glPopMatrix();
        }
        
        if (iter->labelText[0] != '\0')
        {
            if (iter->markerRep != NULL)
                labelHOffset += (int) iter->markerRep->size() / 2 + 3;

            glPushMatrix();
            glTranslatef((int) iter->position.x() + PixelOffset + labelHOffset,
                         (int) iter->position.y() + PixelOffset + labelVOffset,
                         ndc_z);
            glColor(iter->color);
            font[fs]->render(iter->labelText, 0.0f, 0.0f);
            glPopMatrix();
        }
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_DEPTH_TEST);

    return iter;
}


void Renderer::renderMarkers(const MarkerList& markers,
                             const UniversalCoord& cameraPosition,
			                 const Quaterniond& cameraOrientation,
                             double jd)
{
    // Calculate the cosine of half the maximum field of view. We'll use this for
    // fast testing of marker visibility. The stored field of view is the
    // vertical field of view; we want the field of view as measured on the
    // diagonal between viewport corners.
    double h = tan(degToRad(fov / 2));
    double diag = sqrt(1.0 + square(h) + square(h * (double) windowWidth / (double) windowHeight));
    double cosFOV = 1.0 / diag;
    
    Vector3d viewVector = cameraOrientation.conjugate() * -Vector3d::UnitZ();
    
    for (MarkerList::const_iterator iter = markers.begin(); iter != markers.end(); iter++)
    {
        Vector3d offset = iter->position(jd).offsetFromKm(cameraPosition);
                
        // Only render those markers that lie withing the field of view.
        if ((offset.dot(viewVector)) > cosFOV * offset.norm())
        {
            double distance = offset.norm();
            float symbolSize = 0.0f;
            if (iter->sizing() == DistanceBasedSize)
            {
                symbolSize = (float) (iter->representation().size() / distance) / pixelSize;
            }

            if (iter->occludable())
            {
                // If the marker is occludable, add it to the sorted annotation list if it's relatively
                // nearby, and to the background list if it's very distant.
                if (distance < astro::lightYearsToKilometers(1.0))
                {
                    // Modify the marker position so that it is always in front of the marked object.
                    double boundingRadius;
                    if (iter->object().body() != NULL)
                        boundingRadius = iter->object().body()->getBoundingRadius();
                    else
                        boundingRadius = iter->object().radius();                
                    offset *= (1.0 - boundingRadius * 1.01 / distance);
                
                    addSortedAnnotation(&(iter->representation()), EMPTY_STRING, iter->representation().color(),
                                        offset.cast<float>(),
                                        AlignLeft, VerticalAlignTop, symbolSize);
                }
                else
                {
                    addAnnotation(backgroundAnnotations,
                                  &(iter->representation()), EMPTY_STRING, iter->representation().color(),
                                  offset.cast<float>(),
                                  AlignLeft, VerticalAlignTop, symbolSize);
                }
            }
            else
            {
                addAnnotation(foregroundAnnotations,
                              &(iter->representation()), EMPTY_STRING, iter->representation().color(),
                              offset.cast<float>(),
                              AlignLeft, VerticalAlignTop, symbolSize);
            }
        }
    }
}


void Renderer::setStarStyle(StarStyle style)
{
    starStyle = style;
    markSettingsChanged();
}


Renderer::StarStyle Renderer::getStarStyle() const
{
    return starStyle;
}


void Renderer::loadTextures(Body* body)
{
    Surface& surface = body->getSurface();

    if (surface.baseTexture.tex[textureResolution] != InvalidResource)
        surface.baseTexture.find(textureResolution);
    if ((surface.appearanceFlags & Surface::ApplyBumpMap) != 0 &&
        context->bumpMappingSupported() &&
        surface.bumpTexture.tex[textureResolution] != InvalidResource)
        surface.bumpTexture.find(textureResolution);
    if ((surface.appearanceFlags & Surface::ApplyNightMap) != 0 &&
        (renderFlags & ShowNightMaps) != 0)
        surface.nightTexture.find(textureResolution);
    if ((surface.appearanceFlags & Surface::SeparateSpecularMap) != 0 &&
        surface.specularTexture.tex[textureResolution] != InvalidResource)
        surface.specularTexture.find(textureResolution);

    if ((renderFlags & ShowCloudMaps) != 0 &&
        body->getAtmosphere() != NULL &&
        body->getAtmosphere()->cloudTexture.tex[textureResolution] != InvalidResource)
    {
        body->getAtmosphere()->cloudTexture.find(textureResolution);
    }

    if (body->getRings() != NULL &&
        body->getRings()->texture.tex[textureResolution] != InvalidResource)
    {
        body->getRings()->texture.find(textureResolution);
    }
    
    if (body->getGeometry() != InvalidResource)
    {
        Geometry* geometry = GetGeometryManager()->find(body->getGeometry());
        if (geometry)
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
    assert(watcher != NULL);
    watchers.insert(watchers.end(), watcher);
}

void Renderer::removeWatcher(RendererWatcher* watcher)
{
    list<RendererWatcher*>::iterator iter =
        find(watchers.begin(), watchers.end(), watcher);
    if (iter != watchers.end())
        watchers.erase(iter);
}

void Renderer::notifyWatchers() const
{
    for (list<RendererWatcher*>::const_iterator iter = watchers.begin();
         iter != watchers.end(); iter++)
    {
        (*iter)->notifyRenderSettingsChanged(this);
    }
}
