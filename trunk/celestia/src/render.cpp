// render.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cstdio>
#include <cstring>
#include "gl.h"
#include "astro.h"
#include "glext.h"
#include "vecgl.h"
#include "frustum.h"
#include "perlin.h"
#include "spheremesh.h"
#include "lodspheremesh.h"
#include "regcombine.h"
#include "vertexprog.h"
#include "texmanager.h"
#include "meshmanager.h"
#include "render.h"

using namespace std;

#define FOV           45.0f
#define NEAR_DIST      0.5f
#define FAR_DIST   10000000.0f

#define RENDER_DISTANCE 50.0f

#define FAINTEST_MAGNITUDE  5.5f

static const float PixelOffset = 0.375f;

static const int StarVertexListSize = 1024;

// These two values constrain the near and far planes of the view frustum
// when rendering planet and object meshes.  The near plane will never be
// closer than MinNearPlaneDistance, and the far plane is set so that far/near
// will not exceed MaxFarNearRatio.
static const float MinNearPlaneDistance = 0.0001f; // km
static const float MaxFarNearRatio      = 10000.0f;

// Static meshes and textures used by all instances of Simulation

static bool commonDataInitialized = false;

#define SPHERE_LODS 5

static LODSphereMesh* lodSphere = NULL;
static SphereMesh* asteroidMesh = NULL;

static Texture* normalizationTex = NULL;

static Texture* starTex = NULL;
static Texture* glareTex = NULL;
static Texture* galaxyTex = NULL;
static Texture* shadowTex = NULL;

static Texture* starTexB = NULL;
static Texture* starTexA = NULL;
static Texture* starTexG = NULL;
static Texture* starTexM = NULL;

static bool isGF3 = false;

struct SphericalCoordLabel
{
    string label;
    float ra;
    float dec;

    SphericalCoordLabel() : ra(0), dec(0) {};
    SphericalCoordLabel(float _ra, float _dec) : ra(_ra), dec(_dec)
    {
    }
};

static int nCoordLabels = 32;
static SphericalCoordLabel* coordLabels = NULL;


Renderer::Renderer() :
    windowWidth(0),
    windowHeight(0),
    fov(FOV),
    renderMode(GL_FILL),
    labelMode(NoLabels),
    renderFlags(ShowStars | ShowPlanets),
    ambientLightLevel(0.1f),
    fragmentShaderEnabled(false),
    vertexShaderEnabled(false),
    brightnessBias(0.0f),
    brightnessScale(1.0f / 6.0f),
    starVertexBuffer(NULL),
    asterisms(NULL),
    nSimultaneousTextures(1),
    useTexEnvCombine(false),
    useRegisterCombiners(false),
    useCubeMaps(false),
    useVertexPrograms(false),
    useRescaleNormal(false)
{
    starVertexBuffer = new StarVertexBuffer(2048);
}


Renderer::~Renderer()
{
    if (starVertexBuffer != NULL)
        delete starVertexBuffer;
}


static void StarTextureEval(float u, float v, float w,
                            unsigned char *pixel)
{
    float r = 1 - (float) sqrt(u * u + v * v);
    if (r < 0)
    {
        r = 0;
    }
    else if (r < 0.25f)
    {
        r = 4.0f * r;
    }
    else 
    {
        r = 1;
    }

    int pixVal = (int) (r * 255.99f);
    pixel[0] = pixVal;
    pixel[1] = pixVal;
    pixel[2] = pixVal;
}

static void GlareTextureEval(float u, float v, float w,
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

static void ShadowTextureEval(float u, float v, float w,
                              unsigned char *pixel)
{
    float r = (float) sqrt(u * u + v * v);

    int pixVal = r < 0.95f ? 0 : 255;
    pixel[0] = pixVal;
    pixel[1] = pixVal;
    pixel[2] = pixVal;
}


#if 0
static void BoxTextureEval(float u, float v, float w,
                            unsigned char* pixel)
{
    int r = 0, g = 0, b = 0;
    if (abs(u) > abs(v))
    {
        if (abs(u) > abs(w))
            r = 255;
        else
            b = 255;
    }
    else
    {
        if (abs(v) > abs(w))
            g = 255;
        else
            b = 255;
    }

    pixel[0] = r;
    pixel[1] = g;
    pixel[2] = b;
    pixel[3] = 80;
}
#endif

static void IllumMapEval(float x, float y, float z,
                         unsigned char* pixel)
{
    Vec3f v(x, y, z);
    Vec3f n(0, 0, 1);
    Vec3f u(0, 0, 1);
    float c = v * n;
    if (c < 0.0f)
    {
        u = v;
    }
    else
    {
        c = (1 - ((1 - c))) * 1.0f;
        u = v + (c * n);
        u.normalize();
    }

    pixel[0] = 128 + (int) (127 * u.x);
    pixel[1] = 128 + (int) (127 * u.y);
    pixel[2] = 128 + (int) (127 * u.z);
}


static float AsteroidDisplacementFunc(float u, float v, void* info)
{
    float theta = u * (float) PI * 2;
    float phi = (v - 0.5f) * (float) PI;
    float x = (float) (cos(phi) * cos(theta));
    float y = (float) sin(phi);
    float z = (float) (cos(phi) * sin(theta));

    // return 0.5f;
    return fractalsum(Point3f(x + 10, y + 10, z + 10), 1) * 0.75f;
}


static float calcPixelSize(float fovY, float windowHeight)
{
    return 2 * (float) tan(degToRad(fovY / 2.0)) / (float) windowHeight;
}


bool Renderer::init(int winWidth, int winHeight)
{
    // Initialize static meshes and textures common to all instances of Renderer
    if (!commonDataInitialized)
    {
        lodSphere = new LODSphereMesh();
        asteroidMesh = new SphereMesh(Vec3f(0.7f, 1.1f, 1.0f),
                                      21, 20,
                                      AsteroidDisplacementFunc,
                                      NULL);

        starTex = CreateProceduralTexture(64, 64, GL_RGB, StarTextureEval);
        starTex->bindName();

        galaxyTex = CreateProceduralTexture(128, 128, GL_RGBA, GlareTextureEval);
        galaxyTex->bindName();

        glareTex = CreateJPEGTexture("textures/flare.jpg");
        if (glareTex == NULL)
            glareTex = CreateProceduralTexture(64, 64, GL_RGB, GlareTextureEval);
        glareTex->bindName();

        shadowTex = CreateProceduralTexture(256, 256, GL_RGB, ShadowTextureEval);
        shadowTex->bindName();

        starTexB = CreateJPEGTexture("textures/bstar.jpg");
        if (starTexB != NULL)
            starTexB->bindName();
        starTexA = CreateJPEGTexture("textures/astar.jpg");
        if (starTexA != NULL)
            starTexA->bindName();
        starTexG = CreateJPEGTexture("textures/gstar.jpg");
        if (starTexG != NULL)
            starTexG->bindName();
        starTexM = CreateJPEGTexture("textures/mstar.jpg");
        if (starTexM != NULL)
            starTexM->bindName();

        // Initialize GL extensions
        if (ExtensionSupported("GL_ARB_multitexture"))
            InitExtMultiTexture();
        if (ExtensionSupported("GL_NV_register_combiners"))
            InitExtRegisterCombiners();
        if (ExtensionSupported("GL_NV_vertex_program"))
            InitExtVertexProgram();
        if (ExtensionSupported("GL_EXT_texture_cube_map"))
        {
            // normalizationTex = CreateNormalizationCubeMap(64);
            normalizationTex = CreateProceduralCubeMap(64, GL_RGB, IllumMapEval);
            normalizationTex->bindName();
        }

        char* glRenderer = (char*) glGetString(GL_RENDERER);
        if (glRenderer != NULL && strstr(glRenderer, "GeForce3") != NULL)
            isGF3 = true;

        // Create labels for celestial sphere
        {
            char buf[10];
            int i;

            coordLabels = new SphericalCoordLabel[nCoordLabels];
            for (i = 0; i < 12; i++)
            {
                coordLabels[i].ra = i * 2;
                coordLabels[i].dec = 0;
                sprintf(buf, "%dh", i * 2);
                coordLabels[i].label = string(buf);
            }

            coordLabels[12] = SphericalCoordLabel(0, -75);
            coordLabels[13] = SphericalCoordLabel(0, -60);
            coordLabels[14] = SphericalCoordLabel(0, -45);
            coordLabels[15] = SphericalCoordLabel(0, -30);
            coordLabels[16] = SphericalCoordLabel(0, -15);
            coordLabels[17] = SphericalCoordLabel(0,  15);
            coordLabels[18] = SphericalCoordLabel(0,  30);
            coordLabels[19] = SphericalCoordLabel(0,  45);
            coordLabels[20] = SphericalCoordLabel(0,  60);
            coordLabels[21] = SphericalCoordLabel(0,  75);
            for (i = 22; i < nCoordLabels; i++)
            {
                coordLabels[i].ra = 12;
                coordLabels[i].dec = coordLabels[i - 10].dec;
            }

            for (i = 12; i < nCoordLabels; i++)
            {
                char buf[10];
                sprintf(buf, "%d", (int) coordLabels[i].dec);
                coordLabels[i].label = string(buf);
            }
        }

        commonDataInitialized = true;
    }

    cout << "GL extensions supported:\n";
    cout << glGetString(GL_EXTENSIONS) << '\n';

    // Get GL extension information
    if (ExtensionSupported("GL_ARB_multitexture"))
    {
        DPRINTF("Renderer: multi-texture supported.\n");
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &nSimultaneousTextures);
    }
    if (ExtensionSupported("GL_EXT_texture_env_combine"))
    {
        useTexEnvCombine = true;
        DPRINTF("Renderer: texture env combine supported.\n");
    }
    if (ExtensionSupported("GL_NV_register_combiners"))
    {
        DPRINTF("Renderer: nVidia register combiners supported.\n");
        useRegisterCombiners = true;
    }
    if (ExtensionSupported("GL_NV_vertex_program"))
    {
        DPRINTF("Renderer: nVidia vertex programs supported.\n");
        useVertexPrograms = vp::init();
    }
    if (ExtensionSupported("GL_EXT_texture_cube_map"))
    {
        DPRINTF("Renderer: cube texture maps supported.\n");
        useCubeMaps = true;
    }
    if (ExtensionSupported("GL_EXT_rescale_normal"))
    {
        // We need this enabled because we use glScale, but only
        // with uniform scale factors.
        DPRINTF("Renderer: EXT_rescale_normal supported.\n");
        useRescaleNormal = true;
        glEnable(GL_RESCALE_NORMAL_EXT);
    }

    cout << "Simultaneous textures supported: " << nSimultaneousTextures << '\n';

    glLoadIdentity();

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);

    // LEQUAL rather than LESS required for multipass rendering
    glDepthFunc(GL_LEQUAL);

    resize(winWidth, winHeight);

    return true;
}


void Renderer::resize(int width, int height)
{
    windowWidth = width;
    windowHeight = height;
    // glViewport(windowWidth, windowHeight);
}


float Renderer::getFieldOfView()
{
    return fov;
}


void Renderer::setFieldOfView(float _fov)
{
    fov = _fov;
}


TextureFont* Renderer::getFont() const
{
    return font;
}

void Renderer::setFont(TextureFont* txf)
{
    font = txf;
}


void Renderer::setRenderMode(int _renderMode)
{
    renderMode = _renderMode;
}


Vec3f Renderer::getPickRay(int winX, int winY)
{
    float aspectRatio = (float) windowWidth / (float) windowHeight;
    float nearPlaneHeight = 2 * NEAR_DIST * (float) tan(degToRad(fov / 2.0));
    float nearPlaneWidth = nearPlaneHeight * aspectRatio;

    float x = nearPlaneWidth * ((float) winX / (float) windowWidth - 0.5f);
    float y = nearPlaneHeight * (0.5f - (float) winY / (float) windowHeight);
    Vec3f pickDirection = Vec3f(x, y, -NEAR_DIST);
    pickDirection.normalize();

    return pickDirection;
}


int Renderer::getRenderFlags() const
{
    return renderFlags;
}

void Renderer::setRenderFlags(int _renderFlags)
{
    renderFlags = _renderFlags;
}

int Renderer::getLabelMode() const
{
    return labelMode;
}

void Renderer::setLabelMode(int _labelMode)
{
    labelMode = _labelMode;
}


void Renderer::addLabelledStar(Star* star)
{
    labelledStars.insert(labelledStars.end(), star);
}


void Renderer::clearLabelledStars()
{
    labelledStars.clear();
}


void Renderer::showAsterisms(AsterismList* a)
{
    asterisms = a;
}


float Renderer::getAmbientLightLevel() const
{
    return ambientLightLevel;
}


void Renderer::setAmbientLightLevel(float level)
{
    ambientLightLevel = level;
}


bool Renderer::getFragmentShaderEnabled() const
{
    return fragmentShaderEnabled;
}

void Renderer::setFragmentShaderEnabled(bool enable)
{
    fragmentShaderEnabled = enable && fragmentShaderSupported();
}

bool Renderer::fragmentShaderSupported() const
{
    return useCubeMaps && useRegisterCombiners;
}

bool Renderer::getVertexShaderEnabled() const
{
    return vertexShaderEnabled;
}

void Renderer::setVertexShaderEnabled(bool enable)
{
    vertexShaderEnabled = enable && vertexShaderSupported();
}

bool Renderer::vertexShaderSupported() const
{
    return useVertexPrograms;
}



void Renderer::addLabel(string text, Color color, Point3f pos)
{
    double winX, winY, winZ;
    int view[4] = { 0, 0, 0, 0 };
    view[0] = -windowWidth / 2;
    view[1] = -windowHeight / 2;
    view[2] = windowWidth;
    view[3] = windowHeight;
    if (gluProject(pos.x, pos.y, pos.z,
                   modelMatrix,
                   projMatrix,
                   view,
                   &winX, &winY, &winZ) != GL_FALSE)
    {
        Label l;
        l.text = text;
        l.color = color;
        l.position = Point3f((float) winX, (float) winY, (float) winZ);
        labels.insert(labels.end(), l);
    }
}


void Renderer::clearLabels()
{
    labels.clear();
}


void Renderer::render(const Observer& observer,
                      const StarDatabase& starDB,
                      float faintestVisible,
                      SolarSystem* solarSystem,
                      GalaxyList* galaxies,
                      const Selection& sel,
                      double now)
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // Compute the size of a pixel
    pixelSize = calcPixelSize(fov, windowHeight);

    // Set up the projection we'll use for rendering stars.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov,
                   (float) windowWidth / (float) windowHeight,
                   NEAR_DIST, FAR_DIST);
    
    // Set the modelview matrix
    glMatrixMode(GL_MODELVIEW);

    // Create the ambient light source.  For realistic space rendering
    // this should be black.
    {
        float ambientColor[4];
        ambientColor[0] = ambientColor[1] = ambientColor[2] = ambientLightLevel;
        ambientColor[3] = 1.0f;
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);
    }

    // Set up the camera
    Point3f observerPos = (Point3f) observer.getPosition();
    glPushMatrix();
    glRotate(observer.getOrientation());

    // Get the model matrix *before* translation.  We'll use this for positioning
    // star and planet labels.
    glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);

    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);

    clearLabels();
    renderList.clear();

    if ((renderFlags & ShowCelestialSphere) != 0)
    {
        glColor4f(0.5f, 0.0, 0.7f, 0.5f);
        glDisable(GL_TEXTURE_2D);
        renderCelestialSphere(observer);
        glEnable(GL_TEXTURE_2D);
    }

    if ((renderFlags & ShowGalaxies) != 0 && galaxies != NULL)
        renderGalaxies(*galaxies, observer);

    // Translate the camera before rendering the stars
    glPushMatrix();
    glTranslatef(-observerPos.x, -observerPos.y, -observerPos.z);
    // Render stars
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    if ((renderFlags & ShowStars) != 0)
        renderStars(starDB, faintestVisible, observer);

    // Render asterisms
    if ((renderFlags & ShowDiagrams) != 0 && asterisms != NULL)
    {
        glColor4f(0.5f, 0.0, 1.0f, 0.5f);
        glDisable(GL_TEXTURE_2D);
        for (AsterismList::const_iterator iter = asterisms->begin();
             iter != asterisms->end(); iter++)
        {
            Asterism* ast = *iter;

            for (int i = 0; i < ast->getChainCount(); i++)
            {
                const Asterism::Chain& chain = ast->getChain(i);

                glBegin(GL_LINE_STRIP);
                for (Asterism::Chain::const_iterator iter = chain.begin();
                     iter != chain.end(); iter++)
                    glVertex(*iter);
                glEnd();
            }
        }
    }

    if ((labelMode & StarLabels) != 0)
        labelStars(labelledStars, starDB, observer);
    if ((labelMode & ConstellationLabels) != 0 && asterisms != NULL)
        labelConstellations(*asterisms, observer);

    glPopMatrix();

    glPolygonMode(GL_FRONT, (GLenum) renderMode);
    glPolygonMode(GL_BACK, (GLenum) renderMode);

    // Render planets, moons, asteroids, etc.  Stars close and large enough
    // to have discernible surface detail are also placed in renderList.
    const Star* sun = NULL;
    if (solarSystem != NULL)
        sun = solarSystem->getStar();

    if (sun != NULL)
    {
        renderPlanetarySystem(*sun,
                              *solarSystem->getPlanets(),
                              observer,
                              Mat4d::identity(), now,
                              (labelMode & (MinorPlanetLabels | MajorPlanetLabels)) != 0);
        starTex->bind();
    }

    {
        Frustum frustum(degToRad(fov),
                        (float) windowWidth / (float) windowHeight,
                        MinNearPlaneDistance);
        Mat3f viewMat = conjugate(observer.getOrientation()).toMatrix3();

        // Remove objects from the render list that lie completely outside the
        // view frustum.
        vector<RenderListEntry>::iterator notCulled = renderList.begin();
        for (vector<RenderListEntry>::iterator iter = renderList.begin();
             iter != renderList.end(); iter++)
        {
            Point3f center = iter->position * viewMat;

            bool convex = true;
            float radius = 1.0f;
            if (iter->body != NULL)
            {
                radius = iter->body->getRadius();
                if (iter->body->getRings() != NULL)
                {
                    radius = iter->body->getRings()->outerRadius;
                    convex = false;
                }
            }
            else if (iter->star != NULL)
            {
                radius = iter->star->getRadius();
            }

            // Test the object's bounding sphere against the view frustum
            if (frustum.testSphere(center, radius) != Frustum::Outside)
            {
                float nearZ = center.distanceFromOrigin() - radius;
                float maxSpan = (float) sqrt(square((float) windowWidth) +
                                             square((float) windowHeight));
                nearZ = -nearZ / (float) cos(degToRad(fov)) *
                    ((float) windowHeight / maxSpan);
                
                if (nearZ > -MinNearPlaneDistance)
                    iter->nearZ = -MinNearPlaneDistance;
                else
                    iter->nearZ = nearZ;
                iter->farZ = center.z - radius;

                if (!convex)
                {
                    if (iter->farZ / iter->nearZ > MaxFarNearRatio)
                        iter->nearZ = iter->farZ / MaxFarNearRatio;
                }

                *notCulled = *iter;
                notCulled++;
            }
        }

        renderList.resize(notCulled - renderList.begin());
        
        // The calls to renderSolarSystem/renderStars filled renderList
        // with visible planetary bodies.  Sort it by distance, then
        // render each entry.
        sort(renderList.begin(), renderList.end());

        int nEntries = renderList.size();

        // Determine how to split up the depth buffer.  Each body with an
        // apparent size greater than one pixel is allocated its own
        // depth buffer range.  This means that overlapping objects
        // may not be handled correctly, but such an occurrence would be
        // extremely rare, unless we expand the simulation to include
        // docking spaceships etc.  In that case, this technique would have
        // to be modified to let overlapping objects share a depth buffer
        // range.
        int nDepthBuckets = 1;
        int i;
        for (i = 0; i < nEntries; i++)
        {
            if (renderList[i].discSizeInPixels > 1)
                nDepthBuckets++;
        }
        float depthRange = 1.0f / (float) nDepthBuckets;

        int depthBucket = nDepthBuckets - 1;
        i = nEntries - 1;

        // Set up the depth bucket.
        glDepthRange(depthBucket * depthRange, (depthBucket + 1) * depthRange);

        // Set the initial near and far plane distance; any reasonable choice
        // for these will do, since different values will be chosen as soon
        // as we need to render a body as a mesh.
        float nearPlaneDistance = 1.0f;
        float farPlaneDistance = 10.0f;
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(fov, (float) windowWidth / (float) windowHeight,
                       nearPlaneDistance, farPlaneDistance);
        glMatrixMode(GL_MODELVIEW);

        // Render all the bodies in the render list.
        for (i = nEntries - 1; i >= 0; i--)
        {
            if (renderList[i].discSizeInPixels > 1)
            {
                float radius = 1.0f;
                if (renderList[i].body != NULL)
                    radius = renderList[i].body->getRadius();
                else if (renderList[i].star != NULL)
                    radius = renderList[i].star->getRadius();

                nearPlaneDistance = renderList[i].nearZ * -0.9f;
                farPlaneDistance = renderList[i].farZ * -1.1f;
                if (nearPlaneDistance < MinNearPlaneDistance)
                    nearPlaneDistance = MinNearPlaneDistance;
                if (farPlaneDistance / nearPlaneDistance > MaxFarNearRatio)
                    farPlaneDistance = nearPlaneDistance * MaxFarNearRatio;

                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                gluPerspective(fov, (float) windowWidth / (float) windowHeight,
                               nearPlaneDistance, farPlaneDistance);
                glMatrixMode(GL_MODELVIEW);
            }

            if (renderList[i].body != NULL)
            {
                renderPlanet(*renderList[i].body,
                             renderList[i].position,
                             renderList[i].sun,
                             renderList[i].distance,
                             renderList[i].appMag,
                             now,
                             observer.getOrientation(),
                             nearPlaneDistance, farPlaneDistance);
            }
            else if (renderList[i].star != NULL)
            {
                renderStar(*renderList[i].star,
                           renderList[i].position,
                           renderList[i].distance,
                           renderList[i].appMag,
                           observer.getOrientation(),
                           now,
                           nearPlaneDistance, farPlaneDistance);
            }

            // If this body is larger than a pixel, we rendered it as a mesh
            // instead of just a particle.  We move to the next depth buffer 
            // bucket before proceeding with further rendering.
            if (renderList[i].discSizeInPixels > 1)
            {
                depthBucket--;
                glDepthRange(depthBucket * depthRange, (depthBucket + 1) * depthRange);
            }
        }

        // reset the depth range
        glDepthRange(0, 1);

        if ((renderFlags & ShowOrbits) != 0 && solarSystem != NULL)
        {
            // At this point, we're not rendering into the depth buffer
            // so we'll set the far plane to be way out there.  If we don't
            // do this, the orbits either suffer from clipping by the far
            // plane, or else get clipped to close to the viewer.
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            gluPerspective(fov,
                           (float) windowWidth / (float) windowHeight,
                           NEAR_DIST, 1000000);
    
            // Set the modelview matrix
            glMatrixMode(GL_MODELVIEW);

            const Star* sun = solarSystem->getStar();
            Point3f starPos = sun->getPosition();
            // Compute the position of the observer relative to the star
            Vec3d opos = observer.getPosition() - Point3d((double) starPos.x,
                                                          (double) starPos.y,
                                                          (double) starPos.z);

            // At the solar system scale, we'll handle all calculations in
            // AU instead of light years.
            opos = Vec3d(astro::lightYearsToAU(opos.x) * 100,
                         astro::lightYearsToAU(opos.y) * 100,
                         astro::lightYearsToAU(opos.z) * 100);
            glPushMatrix();
            glTranslated(-opos.x, -opos.y, -opos.z);
        
            glDisable(GL_LIGHTING);
            glDisable(GL_TEXTURE_2D);

            // Render orbits
            PlanetarySystem* planets = solarSystem->getPlanets();
            int nBodies = planets->getSystemSize();
            for (i = 0; i < nBodies; i++)
            {
                Body* body = planets->getBody(i);

                // Only show orbits for major bodies or selected objects
                if (body->getRadius() > 1000 || body == sel.body)
                {
                    if (body == sel.body)
                        glColor4f(1, 0, 0, 1);
                    else
                        glColor4f(0, 0, 1, 1);
                    glBegin(GL_LINE_LOOP);
                    int nSteps = 100;
                    double dt = body->getOrbit()->getPeriod() / (double) nSteps;
                    for (int j = 0; j < nSteps; j++)
                    {
                        Point3d p = body->getOrbit()->positionAtTime(j * dt);
                        glVertex3f(astro::kilometersToAU((float) p.x * 100),
                                   astro::kilometersToAU((float) p.y * 100),
                                   astro::kilometersToAU((float) p.z * 100));
                    }
                    glEnd();
                }
            }

#ifdef ECLIPTIC_AXES
            // Render axes in plane of the ecliptic for debugging
            glBegin(GL_LINES);
            glColor4f(1, 0, 0, 1);
            glVertex3f(3000, 0, 0);
            glVertex3f(-3000, 0, 0);
            glVertex3f(2800, 0, 200);
            glVertex3f(3000, 0, 0);
            glVertex3f(2800, 0, -200);
            glVertex3f(3000, 0, 0);
            glColor4f(0, 1, 0, 1);
            glVertex3f(0, 0, 3000);
            glVertex3f(0, 0, -3000);
            glVertex3f(200, 0, 2800);
            glVertex3f(0, 0, 3000);
            glVertex3f(-200, 0, 2800);
            glVertex3f(0, 0, 3000);
            glEnd();
#endif

            glPopMatrix();
        }
    }

    glPopMatrix();

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_FILL);

    renderLabels();

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
}


static void renderRingSystem(float innerRadius,
                             float outerRadius,
                             float beginAngle,
                             float endAngle,
                             int nSections)
{
    float angle = endAngle - beginAngle;

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= nSections; i++)
    {
        float t = (float) i / (float) nSections;
        float theta = beginAngle + t * angle;
        float s = (float) sin(theta);
        float c = (float) cos(theta);
        glTexCoord2f(0, 0);
        glVertex3f(c * innerRadius, 0, s * innerRadius);
        glTexCoord2f(1, 0);
        glVertex3f(c * outerRadius, 0, s * outerRadius);
    }
    glEnd();
}


// If the an object occupies a pixel or less of screen space, we don't
// render its mesh at all and just display a starlike point instead.
// Switching between the particle and mesh renderings of an object is
// jarring, however . . . so we'll blend in the particle view of the
// object to smooth things out, making it dimmer as the disc size approaches
// 4 pixels.
void Renderer::renderBodyAsParticle(Point3f position,
                                    float appMag,
                                    float discSizeInPixels,
                                    Color color,
                                    const Quatf& orientation,
                                    float renderZ,
                                    bool useHaloes)
{
    if (discSizeInPixels < 4 || useHaloes)
    {
        float a = 1;

        if (discSizeInPixels > 1)
        {
            a = 0.5f * (4 - discSizeInPixels);
            if (a > 1)
                a = 1;
        }
        else
        {
            a = clamp(1.0f - appMag * brightnessScale + brightnessBias);
        }

        // We scale up the particle by a factor of 1.5 so that it's more
        // visible--the texture we use has fuzzy edges, and if we render it
        // in just one pixel, it's likely to disappear.  Also, the render
        // distance is scaled by a factor of 0.1 so that we're rendering in
        // front of any mesh that happens to be sharing this depth bucket.
        // What we really want is to render the particle with the frontmost
        // z value in this depth bucket, and scaling the render distance is
        // just hack to accomplish this.  There are cases where it will fail
        // and a more robust method should be implemented.
        float size = pixelSize * 1.5f * renderZ;
        float posScale = abs(renderZ / (position * conjugate(orientation).toMatrix3()).z);

        Point3f center(position.x * posScale,
                       position.y * posScale,
                       position.z * posScale);
        Mat3f m = orientation.toMatrix3();
        Vec3f v0 = Vec3f(-1, -1, 0) * m;
        Vec3f v1 = Vec3f( 1, -1, 0) * m;
        Vec3f v2 = Vec3f( 1,  1, 0) * m;
        Vec3f v3 = Vec3f(-1,  1, 0) * m;

        starTex->bind();
        glColor(color, a);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex(center + (v0 * size));
        glTexCoord2f(1, 0);
        glVertex(center + (v1 * size));
        glTexCoord2f(1, 1);
        glVertex(center + (v2 * size));
        glTexCoord2f(0, 1);
        glVertex(center + (v3 * size));
        glEnd();

        // If the object is brighter than magnitude 1, add a halo around it to
        // make it appear more brilliant.  This is a hack to compensate for the
        // limited dynamic range of monitors.
        //
        // TODO: Currently, this is extremely broken.  Stars look fine,
        // but planets look ridiculous with bright haloes.
        if (useHaloes && appMag < 1.0f)
        {
            a = 0.4f * clamp((appMag - 1) * -0.8f);
            float s = renderZ * 0.001f * (3 - (appMag - 1)) * 2;
            if (s > size * 3)
                size = s;
            else
                size = size * 3;
            float realSize = discSizeInPixels * pixelSize * renderZ;
            if (size < realSize * 10)
                size = realSize * 10;
            glareTex->bind();
            glColor(color, a);
            glBegin(GL_QUADS);
            glTexCoord2f(0, 0);
            glVertex(center + (v0 * size));
            glTexCoord2f(1, 0);
            glVertex(center + (v1 * size));
            glTexCoord2f(1, 1);
            glVertex(center + (v2 * size));
            glTexCoord2f(0, 1);
            glVertex(center + (v3 * size));
            glEnd();
        }
    }
}


static void renderBumpMappedMesh(Mesh& mesh,
                                 Texture& bumpTexture,
                                 Vec3f lightDirection,
                                 Quatf orientation,
                                 Color ambientColor,
                                 float lod)
{
    // We're doing our own per-pixel lighting, so disable GL's lighting
    glDisable(GL_LIGHTING);

    // Render the base texture on the first pass . . .  The base
    // texture and color should have been set up already by the
    // caller.
    mesh.render(lod);

    // The 'default' light vector for the bump map is (0, 0, 1).  Determine
    // a rotation transformation that will move the sun direction to
    // this vector.
    Quatf lightOrientation;
    {
        Vec3f zeroLightDirection(0, 0, 1);
        Vec3f axis = lightDirection ^ zeroLightDirection;
        float cosAngle = zeroLightDirection * lightDirection;
        float angle = 0.0f;
        float epsilon = 1e-5f;

        if (cosAngle + 1 < epsilon)
        {
            axis = Vec3f(0, 1, 0);
            angle = (float) PI;
        }
        else if (cosAngle - 1 > -epsilon)
        {
            axis = Vec3f(0, 1, 0);
            angle = 0.0f;
        }
        else
        {
            axis.normalize();
            angle = (float) acos(cosAngle);
        }
        lightOrientation.setAxisAngle(axis, angle);
    }

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
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_EXT);
    glEnable(GL_TEXTURE_GEN_S);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_EXT);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_EXT);

    // Set up the texture transformation--the light direction and the
    // viewer orientation both need to be considered.
    glMatrixMode(GL_TEXTURE);
    glRotate(lightOrientation * ~orientation);
    glMatrixMode(GL_MODELVIEW);
    glActiveTextureARB(GL_TEXTURE0_ARB);

    mesh.render(lod);

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


static void renderSmoothMesh(Mesh& mesh,
                             Texture& baseTexture,
                             Vec3f lightDirection,
                             Quatf orientation,
                             Color ambientColor,
                             float lod,
                             bool invert = false)
{
    // We're doing our own per-pixel lighting, so disable GL's lighting
    glDisable(GL_LIGHTING);

    // The 'default' light vector for the bump map is (0, 0, 1).  Determine
    // a rotation transformation that will move the sun direction to
    // this vector.
    Quatf lightOrientation;
    {
        Vec3f zeroLightDirection(0, 0, 1);
        Vec3f axis = lightDirection ^ zeroLightDirection;
        float cosAngle = zeroLightDirection * lightDirection;
        float angle = 0.0f;
        float epsilon = 1e-5f;

        if (cosAngle + 1 < epsilon)
        {
            axis = Vec3f(0, 1, 0);
            angle = (float) PI;
        }
        else if (cosAngle - 1 > -epsilon)
        {
            axis = Vec3f(0, 1, 0);
            angle = 0.0f;
        }
        else
        {
            axis.normalize();
            angle = (float) acos(cosAngle);
        }
        lightOrientation.setAxisAngle(axis, angle);
    }

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
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_EXT);
    glEnable(GL_TEXTURE_GEN_S);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_EXT);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_EXT);

    // Set up the texture transformation--the light direction and the
    // viewer orientation both need to be considered.
    glMatrixMode(GL_TEXTURE);
    glRotate(lightOrientation * ~orientation);
    glMatrixMode(GL_MODELVIEW);
    glActiveTextureARB(GL_TEXTURE0_ARB);

    mesh.render(lod);

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


struct RenderInfo
{
    Mesh* mesh;
    Color color;
    Texture* baseTex;
    Texture* bumpTex;
    Texture* cloudTex;
    Texture* nightTex;
    Color hazeColor;
    Color specularColor;
    float specularPower;
    Vec3f sunDir_eye;
    Vec3f sunDir_obj;
    Vec3f eyeDir_obj;
    Point3f eyePos_obj;
    Color sunColor;
    Color ambientColor;
    Quatf orientation;
    float lod;
    bool useTexEnvCombine;

    RenderInfo() : mesh(NULL),
                   color(1.0f, 1.0f, 1.0f),
                   baseTex(NULL),
                   bumpTex(NULL),
                   cloudTex(NULL),
                   nightTex(NULL),
                   hazeColor(0.0f, 0.0f, 0.0f),
                   specularColor(0.0f, 0.0f, 0.0f),
                   specularPower(0.0f),
                   sunDir_eye(0.0f, 0.0f, 1.0f),
                   sunDir_obj(0.0f, 0.0f, 1.0f),
                   eyeDir_obj(0.0f, 0.0f, 1.0f),
                   eyePos_obj(0.0f, 0.0f, 0.0f),
                   sunColor(1.0f, 1.0f, 1.0f),
                   ambientColor(0.0f, 0.0f, 0.0f),
                   orientation(1.0f, 0.0f, 0.0f, 0.0f),
                   lod(0.0f),
                   useTexEnvCombine(false)
    {};
};


static void renderMeshDefault(const RenderInfo& ri, float scale,
                              bool rescaleNormalSupported)
{
    // Set up the light source for the sun
    glEnable(GL_LIGHTING);
    glLightDirection(GL_LIGHT0, ri.sunDir_obj);

    // RANT ALERT!
    // This sucks, but it's necessary.  glScale is used to scale a unit sphere
    // up to planet size.  Since normals are transformed by the inverse
    // transpose of the model matrix, this means they end up getting scaled
    // by a factor of 1.0 / planet radius (in km).  This has terrible effects
    // on lighting: the planet appears almost completely dark.  To get around
    // this, the GL_rescale_normal extension was introduced and eventually
    // incorporated into into the OpenGL 1.2 standard.  Of course, not everyone
    // implemented this incredibly simple and useful little extension.
    // Microsoft is notorious for half-assed support of OpenGL, but 3dfx
    // should have known better: no Voodoo 1/2/3 drivers seem to support this
    // extension.  The following is an attempt to get around the problem by
    // scaling the light brightness by the planet radius.  According to the
    // OpenGL spec, this should work fine, as clamping of colors to [0, 1]
    // occurs *after* lighting.  It works fine on my GeForce3 when I disable
    // EXT_rescale_normal, but I'm not certain whether other drivers
    // are as well behaved as nVidia's.
    if (rescaleNormalSupported)
    {
        glLightColor(GL_LIGHT0, GL_DIFFUSE, ri.sunColor);
    }
    else
    {
        glLightColor(GL_LIGHT0, GL_DIFFUSE,
                     Vec3f(ri.sunColor.red(), ri.sunColor.green(), ri.sunColor.blue()) * scale);
    }

    glEnable(GL_LIGHT0);

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

    ri.mesh->render(ri.lod);
    if (ri.nightTex != NULL && ri.useTexEnvCombine)
    {
        ri.nightTex->bind();
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_PRIMARY_COLOR_EXT);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_ONE_MINUS_SRC_COLOR);
        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_TEXTURE);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);
        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        ri.mesh->render(ri.lod);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    if (ri.cloudTex != NULL)
    {
        ri.cloudTex->bind();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        ri.mesh->render(ri.lod);
    }
}


static void renderMeshFragmentShader(const RenderInfo& ri)
{
    glLightDirection(GL_LIGHT0, ri.sunDir_obj);
    glLightColor(GL_LIGHT0, GL_DIFFUSE, ri.sunColor);
    glEnable(GL_LIGHT0);
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

    if (ri.bumpTex != NULL)
    {
        renderBumpMappedMesh(*(ri.mesh),
                             *(ri.bumpTex),
                             ri.sunDir_eye,
                             ri.orientation,
                             ri.ambientColor,
                             ri.lod);
    }
    else if (ri.baseTex != NULL)
    {
        renderSmoothMesh(*(ri.mesh),
                         *(ri.baseTex),
                         ri.sunDir_eye,
                         ri.orientation,
                         ri.ambientColor,
                         ri.lod);
        if (ri.nightTex != NULL)
        {
            ri.nightTex->bind();
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            renderSmoothMesh(*(ri.mesh),
                             *(ri.nightTex),
                             ri.sunDir_eye, 
                             ri.orientation,
                             ri.ambientColor,
                             ri.lod,
                             true);
        }
    }
    else
    {
        glEnable(GL_LIGHTING);
        ri.mesh->render(ri.lod);
    }

    if (ri.cloudTex != NULL)
    {
        ri.cloudTex->bind();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LIGHTING);
        ri.mesh->render(ri.lod);
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}


static void renderMeshVertexAndFragmentShader(const RenderInfo& ri)
{
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
    Vec3f halfAngle_obj = ri.eyeDir_obj + ri.sunDir_obj;
    if (halfAngle_obj.length() != 0.0f)
        halfAngle_obj.normalize();

    // Set up the fog parameters if the haze density is non-zero
    float hazeDensity = ri.hazeColor.alpha();

    // This is a last minute fix . . . there appears to be a difference in
    // how the fog coordinate is handled by the GeForce3 and the rest of the
    // nVidia cards.  For now, just disable haze if we're running on anything
    // but a GeForce3 :<
    if (!isGF3)
        hazeDensity = 0.0f;

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

    vp::enable();
    vp::parameter(15, ri.eyePos_obj);
    vp::parameter(16, ri.sunDir_obj);
    vp::parameter(17, halfAngle_obj);
    vp::parameter(20, ri.sunColor * ri.color);
    vp::parameter(32, ri.ambientColor * ri.color);
    vp::parameter(33, ri.hazeColor);
    vp::parameter(40, 0.0f, 1.0f, 0.5f, ri.specularPower);

    // Currently, we don't support bump maps and specular reflection
    // simultaneously . . .
    if (ri.bumpTex != NULL)
    {
        vp::enable();
        if (hazeDensity > 0.0f)
            vp::use(vp::diffuseBumpHaze);
        else
            vp::use(vp::diffuseBump);
        SetupCombinersDecalAndBumpMap(*(ri.bumpTex),
                                      ri.ambientColor * ri.color,
                                      ri.sunColor * ri.color);
        ri.mesh->render(Mesh::Normals | Mesh::Tangents | Mesh::TexCoords0 |
                        Mesh::VertexProgParams, ri.lod);
        DisableCombiners();
    }
    else if (ri.specularColor != Color(0.0f, 0.0f, 0.0f))
    {
        vp::parameter(34, ri.sunColor * ri.specularColor);
        vp::use(vp::specular);
        SetupCombinersGlossMapWithFog();
        ri.mesh->render(Mesh::Normals | Mesh::TexCoords0 |
                        Mesh::VertexProgParams, ri.lod);
        DisableCombiners();
    }
    else
    {
        if (hazeDensity > 0.0f)
            vp::use(vp::diffuseHaze);
        else
            vp::use(vp::diffuse);
        ri.mesh->render(Mesh::Normals | Mesh::TexCoords0 |
                        Mesh::VertexProgParams, ri.lod);
    }

    if (hazeDensity > 0.0f)
        glDisable(GL_FOG);

    if (ri.nightTex != NULL && ri.useTexEnvCombine)
    {
        ri.nightTex->bind();
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_PRIMARY_COLOR_EXT);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_ONE_MINUS_SRC_COLOR);
        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_TEXTURE);
        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);
        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        vp::use(vp::diffuse);
        ri.mesh->render(ri.lod);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    if (ri.cloudTex != NULL)
    {
        ri.cloudTex->bind();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        vp::use(vp::diffuse);
        ri.mesh->render(ri.lod);
    }

    vp::disable();
}


void Renderer::renderPlanet(const Body& body,
                            Point3f pos,
                            Vec3f sunDirection,
                            float distance,
                            float appMag,
                            double now,
                            Quatf orientation,
                            float nearPlaneDistance,
                            float farPlaneDistance)
{
    float discSizeInPixels = body.getRadius() / (distance * pixelSize);
    if (discSizeInPixels > 1)
    {
        RenderInfo ri;

        // Enable depth buffering
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        glDisable(GL_BLEND);

        // Determine the mesh to use
        if (body.getMesh() == InvalidResource)
        {
            if (body.getRadius() < 50)
                ri.mesh = asteroidMesh;
            else
                ri.mesh = lodSphere;
        }
        else
        {
            ri.mesh = GetMeshManager()->find(body.getMesh());
        }

        const Surface& surface = body.getSurface();

        // Get the textures . . .
        TextureManager* textureManager = GetTextureManager();
        if (surface.baseTexture != InvalidResource)
            ri.baseTex = textureManager->find(surface.baseTexture);
        if ((surface.appearanceFlags & Surface::ApplyBumpMap) != 0 &&
            (fragmentShaderEnabled && useRegisterCombiners && useCubeMaps) &&
            surface.bumpTexture != InvalidResource)
            ri.bumpTex = textureManager->find(surface.bumpTexture);
        if ((surface.appearanceFlags & Surface::ApplyCloudMap) != 0 &&
            (renderFlags & ShowCloudMaps) != 0)
            ri.cloudTex = textureManager->find(surface.cloudTexture);
        if ((surface.appearanceFlags & Surface::ApplyNightMap) != 0 &&
            (renderFlags & ShowNightMaps) != 0)
            ri.nightTex = textureManager->find(surface.nightTexture);

        // Apply the modelview transform for the body
        glPushMatrix();
        glTranslate(pos);
        glRotatef(radToDeg(body.getObliquity()), 1, 0, 0);

        double planetRotation = 0.0;
        // Watch out for the precision limits of floats when computing planet
        // rotation . . .
        {
            double rotations = now / (double) body.getRotationPeriod();
            int wholeRotations = (int) rotations;
            double remainder = rotations - wholeRotations;
            planetRotation = remainder * 2 * PI + body.getRotationPhase();
            glRotatef((float) (remainder * 360.0 + radToDeg(body.getRotationPhase())),
                      0, 1, 0);
        }

        // Apply a scale factor which depends on the size of the planet and
        // its oblateness.  Since the oblateness is usually quite
        // small, the potentially nonuniform scale factor shouldn't mess up
        // the lighting calculations enough to be noticeable.
        // TODO:  Figure out a better way to render ellipsoids than applying
        // a nonunifom scale factor to a sphere . . . it makes me nervous.
        float radius = body.getRadius();
        glScalef(radius, radius * (1.0f - body.getOblateness()), radius);

        // Compute the direction to the eye and light source in object space
        Mat4f planetMat = (Mat4f::xrotation((float) body.getObliquity()) *
                           Mat4f::yrotation((float) planetRotation));
        ri.sunDir_eye = sunDirection;
        ri.sunDir_eye.normalize();
        ri.sunDir_obj = ri.sunDir_eye * planetMat;
        ri.eyeDir_obj = (Point3f(0, 0, 0) - pos) * planetMat;
        ri.eyeDir_obj.normalize();
        ri.eyePos_obj = Point3f(-pos.x, -pos.y, -pos.z) * planetMat;
        ri.orientation = orientation;

        // Determine the appropriate level of detail
        if (discSizeInPixels < 10)
            ri.lod = -3.0f;
        else if (discSizeInPixels < 50)
            ri.lod = -2.0f;
        else if (discSizeInPixels < 100)
            ri.lod = -1.0f;
        else if (discSizeInPixels < 200)
            ri.lod = 0.0f;
        else if (discSizeInPixels < 400)
            ri.lod = 1.0f;
        else if (discSizeInPixels < 800)
            ri.lod = 2.0f;
        else
            ri.lod = 3.0f;

        // Set up the colors
        if (ri.baseTex == NULL ||
            (surface.appearanceFlags & Surface::BlendTexture) != 0)
        {
            ri.color = surface.color;
        }

        ri.sunColor = Color(1.0f, 1.0f, 1.0f);
        {
            // If the star is sufficiently cool, change the light color
            // from white.  Though our sun appears yellow, we still make
            // it and all hotter stars emit white light, as this is the
            // 'natural' light to which our eyes are accustomed.  It may
            // make sense to give a slight bluish tint to light from
            // O and B type stars though.
            PlanetarySystem* system = body.getSystem();
            if (system != NULL)
            {
                const Star* sun = system->getStar();
                switch (sun->getStellarClass().getSpectralClass())
                {
                case StellarClass::Spectral_K:
                    ri.sunColor = Color(1.0f, 0.9f, 0.8f);
                    break;
                case StellarClass::Spectral_M:
                case StellarClass::Spectral_R:
                case StellarClass::Spectral_S:
                case StellarClass::Spectral_N:
                    ri.sunColor = Color(1.0f, 0.7f, 0.7f);
                    break;
                }
            }
        }
            
        ri.ambientColor = Color(ambientLightLevel, ambientLightLevel, ambientLightLevel) * ri.sunColor;
        ri.hazeColor = surface.hazeColor;
        ri.specularColor = surface.specularColor;
        ri.specularPower = surface.specularPower;
        ri.useTexEnvCombine = useTexEnvCombine;

        // Currently, there are three different rendering paths:
        //   1. Generic OpenGL 1.1
        //   2. OpenGL 1.2 + nVidia register combiners
        //   3. OpenGL 1.2 + nVidia register combiners + vertex programs
        // Unfortunately, this means that unless you've got a GeForce card,
        // you'll miss out on a lot of the eye candy . . .
        if (ri.mesh != NULL)
        {
            if (fragmentShaderEnabled && vertexShaderEnabled)
                renderMeshVertexAndFragmentShader(ri);
            else if (fragmentShaderEnabled && !vertexShaderEnabled)
                renderMeshFragmentShader(ri);
            else
                renderMeshDefault(ri, radius, useRescaleNormal);
        }

        // If the planet has a ring system, render it.
        if (body.getRings() != NULL)
        {
            RingSystem* rings = body.getRings();
            float inner = rings->innerRadius / body.getRadius();
            float outer = rings->outerRadius / body.getRadius();
            int nSections = 100;

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
                float illumFraction = (1.0f + ri.eyeDir_obj * ri.sunDir_obj) / 2.0f;
                // Just use the illuminated fraction for now . . .
                ringIllumination = illumFraction;
            }

            // If we have multi-texture support, we'll use the second texture unit
            // to render the shadow of the planet on the rings.  This is a bit of
            // a hack, and assumes that the planet is nearly spherical in shape, and
            // only works for a planet illuminated by a single sun where the distance
            // to the sun is very large relative to its diameter.
            if (nSimultaneousTextures > 1)
            {
                glActiveTextureARB(GL_TEXTURE1_ARB);
                glEnable(GL_TEXTURE_2D);
                shadowTex->bind();

                float sPlane[4] = { 0, 0, 0, 0.5f };
                float tPlane[4] = { 0, 0, 0, 0.5f };

                // Compute the projection vectors based on the sun direction.
                // I'm being a little careless here--if the sun direction lies
                // along the y-axis, this will fail.  It's unlikely that a
                // planet would ever orbit underneath its sun (an orbital
                // inclination of 90 degrees), but this should be made
                // more robust anyway.
                float scale = rings->innerRadius / body.getRadius();
                Vec3f axis = Vec3f(0, 1, 0) ^ ri.sunDir_obj;
                float angle = (float) acos(Vec3f(0, 1, 0) * ri.sunDir_obj);
                Mat4f mat = Mat4f::rotation(axis, -angle);
                Vec3f sAxis = Vec3f(0.5f * scale, 0, 0) * mat;
                Vec3f tAxis = Vec3f(0, 0, 0.5f * scale) * mat;

                sPlane[0] = sAxis.x; sPlane[1] = sAxis.y; sPlane[2] = sAxis.z;
                tPlane[0] = tAxis.x; tPlane[1] = tAxis.y; tPlane[2] = tAxis.z;

                glEnable(GL_TEXTURE_GEN_S);
                glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
                glTexGenfv(GL_S, GL_EYE_PLANE, sPlane);
                glEnable(GL_TEXTURE_GEN_T);
                glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
                glTexGenfv(GL_T, GL_EYE_PLANE, tPlane);

                glActiveTextureARB(GL_TEXTURE0_ARB);
            }

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            Texture* ringsTex = GetTextureManager()->find(rings->texture);

            if (ringsTex != NULL)
                ringsTex->bind();
            else
                glDisable(GL_TEXTURE_2D);
        
            // Perform our own lighting for the rings.
            // TODO: Don't forget about light source color (required when we start paying
            // attention to star color.)
            glDisable(GL_LIGHTING);
            {
                Vec3f litColor(rings->color.red(), rings->color.green(), rings->color.blue());
                litColor = litColor * ringIllumination + Vec3f(1, 1, 1) * ambientLightLevel;
                glColor4f(litColor.x, litColor.y, litColor.z, 1.0f);
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
            float sunAngle = (float) atan2(ri.sunDir_obj.z, ri.sunDir_obj.x);

            renderRingSystem(inner, outer,
                             (float) (sunAngle + PI / 2),
                             (float) (sunAngle + 3 * PI / 2),
                             nSections / 2);
            // glNormal3f(0, -1, 0);
            renderRingSystem(inner, outer,
                             (float) (sunAngle +  3 * PI / 2),
                             (float) (sunAngle + PI / 2),
                             nSections / 2);

            // Disable the second texture unit if it was used
            if (nSimultaneousTextures > 1)
            {
                glActiveTextureARB(GL_TEXTURE1_ARB);
                glDisable(GL_TEXTURE_2D);
                glDisable(GL_TEXTURE_GEN_S);
                glDisable(GL_TEXTURE_GEN_T);
                glActiveTextureARB(GL_TEXTURE0_ARB);
            }

            // Render the unshadowed side
            // glNormal3f(0, 1, 0);
            renderRingSystem(inner, outer,
                             (float) (sunAngle - PI / 2),
                             (float) (sunAngle + PI / 2),
                             nSections / 2);
            // glNormal3f(0, -1, 0);
            renderRingSystem(inner, outer,
                             (float) (sunAngle + PI / 2),
                             (float) (sunAngle - PI / 2),
                             nSections / 2);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        }

        glPopMatrix();
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
    }
    renderBodyAsParticle(pos,
                         appMag,
                         discSizeInPixels,
                         body.getSurface().color,
                         orientation,
                         (nearPlaneDistance + farPlaneDistance) / 2.0f,
                         false);
}


void Renderer::renderStar(const Star& star,
                          Point3f pos,
                          float distance,
                          float appMag,
                          Quatf orientation,
                          double now,
                          float nearPlaneDistance,
                          float farPlaneDistance)
{
    Color color = star.getStellarClass().getApparentColor();
    float radius = star.getRadius();
    float discSizeInPixels = radius / (distance * pixelSize);

    if (discSizeInPixels > 1)
    {
        // Enable depth buffering
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        glDisable(GL_BLEND);

        glPushMatrix();
        glTranslate(pos);
        glScalef(radius, radius, radius);

        glColor(color);

        Texture* tex = NULL;
        switch (star.getStellarClass().getSpectralClass())
        {
        case StellarClass::Spectral_O:
        case StellarClass::Spectral_B:
            tex = starTexB;
            break;
        case StellarClass::Spectral_A:
        case StellarClass::Spectral_F:
            tex = starTexA;
            break;
        case StellarClass::Spectral_G:
        case StellarClass::Spectral_K:
            tex = starTexG;
            break;
        case StellarClass::Spectral_M:
        case StellarClass::Spectral_R:
        case StellarClass::Spectral_S:
        case StellarClass::Spectral_N:
            tex = starTexM;
            break;
        default:
            tex = starTexA;
            break;
        }

        if (tex == NULL)
        {
            glDisable(GL_TEXTURE_2D);
        }
        else
        {
            glEnable(GL_TEXTURE_2D);
            tex->bind();
        }

        // Determine the appropriate level of detail
        float lod = 0.0f;
        if (discSizeInPixels < 10)
            lod = -3.0f;
        else if (discSizeInPixels < 50)
            lod = -2.0f;
        else if (discSizeInPixels < 100)
            lod = -1.0f;
        else if (discSizeInPixels < 200)
            lod = 0.0f;
        else if (discSizeInPixels < 400)
            lod = 1.0f;
        else if (discSizeInPixels < 800)
            lod = 2.0f;
        else
            lod = 3.0f;

        // Rotate the star
        {
            // Use doubles to avoid precision problems here . . .
            double rotations = now / (double) star.getRotationPeriod();
            int wholeRotations = (int) rotations;
            double remainder = rotations - wholeRotations;
            glRotatef((float) (-remainder * 360.0), 0, 1, 0);
        }

        lodSphere->render(lod);;

        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glPopMatrix();
    }

    renderBodyAsParticle(pos,
                         appMag,
                         discSizeInPixels,
                         color,
                         orientation,
                         (nearPlaneDistance + farPlaneDistance) / 2.0f,
                         true);
}


void Renderer::renderPlanetarySystem(const Star& sun,
                                     const PlanetarySystem& solSystem,
                                     const Observer& observer,
                                     const Mat4d& frame,
                                     double now,
                                     bool showLabels)
{
    Point3f starPos = sun.getPosition();
    Point3d observerPos = astro::heliocentricPosition(observer.getPosition(), starPos);

    int nBodies = solSystem.getSystemSize();
    for (int i = 0; i < nBodies; i++)
    {
        Body* body = solSystem.getBody(i);
        Point3d localPos = body->getOrbit()->positionAtTime(now);
        Mat4d newFrame = Mat4d::xrotation(-body->getObliquity()) * Mat4d::translation(localPos) * frame;
        Point3d bodyPos = Point3d(0, 0, 0) * newFrame;
        bodyPos = body->getHeliocentricPosition(now);
        
        // We now have the positions of the observer and the planet relative
        // to the sun.  From these, compute the position of the planet
        // relative to the observer.
        Vec3d posd = bodyPos - observerPos;

        double distanceFromObserver = posd.length();
        float appMag = body->getApparentMagnitude(sun,
                                                  bodyPos - Point3d(0, 0, 0),
                                                  posd);
        Vec3f pos((float) posd.x, (float) posd.y, (float) posd.z);

        // Compute the size of the planet/moon disc in pixels
        float discSize = (body->getRadius() / (float) distanceFromObserver) / pixelSize;

        if (discSize > 1 || appMag < 1.0f / brightnessScale)
        {
            RenderListEntry rle;
            rle.body = body;
            rle.star = NULL;
            rle.position = Point3f(pos.x, pos.y, pos.z);
            rle.sun = Vec3f((float) -bodyPos.x, (float) -bodyPos.y, (float) -bodyPos.z);
            rle.distance = distanceFromObserver;
            rle.discSizeInPixels = discSize;
            rle.appMag = appMag;
            renderList.insert(renderList.end(), rle);
        }
        
        if (showLabels && (pos * conjugate(observer.getOrientation()).toMatrix3()).z < 0)
        {
            if (body->getRadius() >= 1000.0 && (labelMode & MajorPlanetLabels) != 0)

            {
                addLabel(body->getName(),
                         Color(0.0f, 1.0f, 0.0f),
                         Point3f(pos.x, pos.y, pos.z));
            }
            else if (body->getRadius() < 1000.0 && (labelMode & MinorPlanetLabels) != 0)
            {
                addLabel(body->getName(),
                         Color(0.0f, 0.6f, 0.0f),
                         Point3f(pos.x, pos.y, pos.z));
            }
        }

        if (appMag < 5.0f)
        {
            const PlanetarySystem* satellites = body->getSatellites();
            if (satellites != NULL)
            {
                // Only show labels for satellites if the planet is nearby.
                bool showSatelliteLabels = showLabels && (distanceFromObserver < 25000000);
                renderPlanetarySystem(sun, *satellites, observer, newFrame, now,
                                      showSatelliteLabels);
            }
        }

    }
}


class StarRenderer : public StarHandler
{
public:
    StarRenderer();
    ~StarRenderer() {};
    
    void process(const Star&, float, float);

public:
    const Observer* observer;
    Point3f position;
    Vec3f viewNormal;

    vector<Renderer::Particle>* glareParticles;
    vector<Renderer::RenderListEntry>* renderList;
    Renderer::StarVertexBuffer* starVertexBuffer;

    float faintestVisible;
    float size;
    float pixelSize;
    float brightnessScale;
    float brightnessBias;

    int nProcessed;
    int nRendered;
    int nClose;
    int nBright;
};

StarRenderer::StarRenderer()
{
    nRendered = 0;
    nClose = 0;
    nBright = 0;
    nProcessed = 0;
    starVertexBuffer = NULL;
}

void StarRenderer::process(const Star& star, float distance, float appMag)
{
    nProcessed++;

    Point3f starPos = star.getPosition();
    Vec3f relPos = starPos - position;

    if (relPos * viewNormal > 0 || relPos.x * relPos.x < 0.1f)
    {
        Color starColor = star.getStellarClass().getApparentColor();
        float renderDistance = distance;
        float s = renderDistance * size;
        float discSizeInPixels = 0.0f;

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
        if (distance < 1.0f)
        {
            // Compute the position of the observer relative to the star.
            // This is a much more accurate (and expensive) distance
            // calculation than the previous one which used the observer's
            // position rounded off to floats.
            relPos = starPos - observer->getPosition();
            distance = relPos.length();

            // Recompute apparent magnitude using new distance computation
            appMag = astro::absToAppMag(star.getAbsoluteMagnitude(), distance);

            float f = RENDER_DISTANCE / distance;
            renderDistance = RENDER_DISTANCE;
            starPos = position + relPos * f;

            float radius = star.getRadius();
            discSizeInPixels = radius / astro::lightYearsToKilometers(distance) /
                pixelSize;

            nClose++;
        }

        if (discSizeInPixels <= 1)
        {
            float alpha = clamp(1.0f - appMag * brightnessScale + brightnessBias);

            nRendered++;
            starVertexBuffer->addStar(starPos,
                                      Color(starColor, alpha),
                                      renderDistance * size);

            // If the star is brighter than magnitude 1, add a halo around it
            // to make it appear more brilliant.  This is a hack to compensate
            // for the limited dynamic range of monitors.
            if (appMag < 1.0f)
            {
                Renderer::Particle p;
                p.center = starPos;
                p.size = renderDistance * size;
                p.color = Color(starColor, alpha);

                alpha = 0.4f * clamp((appMag - 1) * -0.8f);
                s = renderDistance * 0.001f * (3 - (appMag - 1)) * 2;

                if (s > p.size * 3)
                    p.size = s;
                else
                    p.size = p.size * 3;
                p.color = Color(starColor, alpha);
                glareParticles->insert(glareParticles->end(), p);
                nBright++;
            }
        }
        else
        {
            Renderer::RenderListEntry rle;
            rle.star = &star;
            rle.body = NULL;
            
            // Objects in the render list are always rendered relative to
            // a viewer at the origin--this is different than for distant
            // stars.
            float scale = astro::lightYearsToKilometers(1.0f);
            rle.position = Point3f(relPos.x * scale, relPos.y * scale, relPos.z * scale);
            rle.distance = rle.position.distanceFromOrigin();
            rle.discSizeInPixels = discSizeInPixels;
            rle.appMag = appMag;
            renderList->insert(renderList->end(), rle);
        }
    }
}

void Renderer::renderStars(const StarDatabase& starDB,
                           float faintestVisible,
                           const Observer& observer)
{
    StarRenderer starRenderer;

    starRenderer.observer = &observer;
    starRenderer.position = (Point3f) observer.getPosition();
    starRenderer.viewNormal = Vec3f(0, 0, -1) * observer.getOrientation().toMatrix3();
    starRenderer.glareParticles = &glareParticles;
    starRenderer.renderList = &renderList;
    starRenderer.starVertexBuffer = starVertexBuffer;
    starRenderer.faintestVisible = faintestVisible;
    starRenderer.size = pixelSize * 1.5f;
    starRenderer.pixelSize = pixelSize;
    starRenderer.brightnessScale = brightnessScale;
    starRenderer.brightnessBias = brightnessBias;

    glareParticles.clear();

    starVertexBuffer->setBillboardOrientation(observer.getOrientation());

    starTex->bind();
    starDB.findVisibleStars(starRenderer,
                            (Point3f) observer.getPosition(),
                            observer.getOrientation(),
                            degToRad(fov),
                            (float) windowWidth / (float) windowHeight,
                            faintestVisible);

    starRenderer.starVertexBuffer->render();
    glareTex->bind();
    renderParticles(glareParticles, observer.getOrientation());
}


void Renderer::renderGalaxies(const GalaxyList& galaxies,
                              const Observer& observer)
{
    // Vec3f viewNormal = Vec3f(0, 0, -1) * observer.getOrientation().toMatrix3();
    Point3d observerPos = (Point3d) observer.getPosition();
    Mat3f viewMat = observer.getOrientation().toMatrix3();
    Vec3f v0 = Vec3f(-1, -1, 0) * viewMat;
    Vec3f v1 = Vec3f( 1, -1, 0) * viewMat;
    Vec3f v2 = Vec3f( 1,  1, 0) * viewMat;
    Vec3f v3 = Vec3f(-1,  1, 0) * viewMat;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    galaxyTex->bind();

    for (GalaxyList::const_iterator iter = galaxies.begin();
         iter != galaxies.end(); iter++)
    {
        Galaxy* galaxy = *iter;
        Point3d pos = galaxy->getPosition();
        float radius = galaxy->getRadius();
        Point3f offset = Point3f((float) (observerPos.x - pos.x),
                                 (float) (observerPos.y - pos.y),
                                 (float) (observerPos.z - pos.z));
        float distanceToGalaxy = offset.distanceFromOrigin() - radius;
        if (distanceToGalaxy < 0)
            distanceToGalaxy = 0;
        float minimumFeatureSize = pixelSize * 0.5f * distanceToGalaxy;

        GalacticForm* form = galaxy->getForm();
        if (form != NULL)
        {
            glPushMatrix();
            glTranslate(Point3f(0, 0, 0) - offset);

            Mat4f m = (galaxy->getOrientation().toMatrix4() *
                       Mat4f::scaling(form->scale) *
                       Mat4f::scaling(radius));
            float size = radius;
            int pow2 = 1;

            vector<Point3f>* points = form->points;
            int nPoints = (int) (points->size() * clamp(galaxy->getDetail()));

            glBegin(GL_QUADS);
            for (int i = 0; i < nPoints; i++)
            {
                Point3f p = (*points)[i] * m;
                Vec3f relPos = p - offset;

                if ((i & pow2) != 0)
                {
                    pow2 <<= 1;
                    size /= 1.5f;
                    if (size < minimumFeatureSize)
                        break;
                }

                // if (relPos * viewNormal > 0)
                {
                    float distance = relPos.length();
                    float screenFrac = size / distance;

                    if (screenFrac < 0.05f)
                    {
                        float a = 8 * (0.05f - screenFrac);
                        glColor4f(1, 1, 1, a);
                        glTexCoord2f(0, 0);
                        glVertex(p + (v0 * size));
                        glTexCoord2f(1, 0);
                        glVertex(p + (v1 * size));
                        glTexCoord2f(1, 1);
                        glVertex(p + (v2 * size));
                        glTexCoord2f(0, 1);
                        glVertex(p + (v3 * size));
                    }
                }
            }
            glEnd();

            glPopMatrix();
        }
    }
}


void Renderer::renderCelestialSphere(const Observer& observer)
{
    int raDivisions = 12;
    int decDivisions = 12;
    int nSections = 60;
    float radius = 10.0f;

    int i;
    for (i = 0; i < raDivisions; i++)
    {
        float ra = (float) i / (float) raDivisions * 24.0f;

        glBegin(GL_LINE_STRIP);
        for (int j = 0; j <= nSections; j++)
        {
            float dec = ((float) j / (float) nSections) * 180 - 90;
            glVertex(astro::equatorialToCelestialCart(ra, dec, radius));
        }
        glEnd();
    }

    for (i = 1; i < decDivisions; i++)
    {
        float dec = (float) i / (float) decDivisions * 180 - 90;
        glBegin(GL_LINE_LOOP);
        for (int j = 0; j < nSections; j++)
        {
            float ra = (float) j / (float) nSections * 24.0f;
            glVertex(astro::equatorialToCelestialCart(ra, dec, radius));
        }
        glEnd();
    }

    for (i = 0; i < nCoordLabels; i++)
    {
        Point3f pos = astro::equatorialToCelestialCart(coordLabels[i].ra,
                                                       coordLabels[i].dec,
                                                       radius);
        if ((pos * conjugate(observer.getOrientation()).toMatrix3()).z < 0)
        {
            addLabel(coordLabels[i].label, Color(0.0f, 0.0f, 1.0f, 0.7f), pos);
        }
    }
}


void Renderer::labelStars(const vector<Star*>& stars,
                          const StarDatabase& starDB,
                          const Observer& observer)
{
    Point3f observerPos = (Point3f) observer.getPosition();

    for (vector<Star*>::const_iterator iter = stars.begin(); iter != stars.end(); iter++)
    {
        Star* star = *iter;
        Point3f pos = star->getPosition();
        float distance = pos.distanceTo(observerPos);
        float appMag = astro::absToAppMag(star->getAbsoluteMagnitude(), distance);
        
        if (appMag < 6.0f)
        {
            Vec3f rpos = pos - observerPos;
            if ((rpos * conjugate(observer.getOrientation()).toMatrix3()).z < 0)
            {
                addLabel(starDB.getStarName(*star),
                         Color(0.3f, 0.3f, 1.0f),
                         Point3f(rpos.x, rpos.y, rpos.z));
            }
        }
    }
}


void Renderer::labelConstellations(const AsterismList& asterisms,
                                   const Observer& observer)
{
    Point3f observerPos = (Point3f) observer.getPosition();

    for (AsterismList::const_iterator iter = asterisms.begin();
         iter != asterisms.end(); iter++)
    {
        Asterism* ast = *iter;
        if (ast->getChainCount() > 0)
        {
            const Asterism::Chain& chain = ast->getChain(0);

            if (chain.size() > 0)
            {
                // The constellation label is positioned at the average
                // position of all stars in the first chain.  This usually
                // gives reasonable results.
                Vec3f avg(0, 0, 0);
                for (Asterism::Chain::const_iterator iter = chain.begin();
                     iter != chain.end(); iter++)
                    avg += (*iter - Point3f(0, 0, 0));

                avg = avg / (float) chain.size();
                Vec3f rpos = Point3f(avg.x, avg.y, avg.z) - observerPos;
                if ((rpos * conjugate(observer.getOrientation()).toMatrix3()).z < 0) {
                    addLabel(ast->getName(),
                             Color(0.5f, 0.0f, 1.0f, 1.0f),
                             Point3f(rpos.x, rpos.y, rpos.z));
                }
            }
        }
    }
}


void Renderer::renderParticles(const vector<Particle>& particles,
                               Quatf orientation)
{
    int nParticles = particles.size();

    Mat3f m = orientation.toMatrix3();
    Vec3f v0 = Vec3f(-1, -1, 0) * m;
    Vec3f v1 = Vec3f( 1, -1, 0) * m;
    Vec3f v2 = Vec3f( 1,  1, 0) * m;
    Vec3f v3 = Vec3f(-1,  1, 0) * m;

    glBegin(GL_QUADS);
    for (int i = 0; i < nParticles; i++)
    {
        Point3f center = particles[i].center;
        float size = particles[i].size;

        glColor(particles[i].color);
        glTexCoord2f(0, 0);
        glVertex(center + (v0 * size));
        glTexCoord2f(1, 0);
        glVertex(center + (v1 * size));
        glTexCoord2f(1, 1);
        glVertex(center + (v2 * size));
        glTexCoord2f(0, 1);
        glVertex(center + (v3 * size));
    }
    glEnd();
}


void Renderer::renderLabels()
{
    if (font == NULL)
        return;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, font->getTextureName());

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef((int) (windowWidth / 2), (int) (windowHeight / 2), 0);

    for (int i = 0; i < (int) labels.size(); i++)
    {
        glColor(labels[i].color);
        glPushMatrix();
        glTranslatef((int) labels[i].position.x + PixelOffset,
                     (int) labels[i].position.y + PixelOffset,
                     0);
        font->render(labels[i].text);
        glPopMatrix();
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}


float Renderer::getBrightnessScale() const
{
    return brightnessScale;
}

void Renderer::setBrightnessScale(float scale)
{
    brightnessScale = scale;
}

float Renderer::getBrightnessBias() const
{
    return brightnessBias;
}

void Renderer::setBrightnessBias(float bias)
{
    brightnessBias = bias;
}


Renderer::StarVertexBuffer::StarVertexBuffer(unsigned int _capacity) :
    capacity(_capacity),
    vertices(NULL),
    texCoords(NULL),
    colors(NULL)
{
    nStars = 0;
    vertices = new float[capacity * 12];
    texCoords = new float[capacity * 8];
    colors = new unsigned char[capacity * 16];

    // Fill the texture coordinate array now, since it will always have
    // the same contents.
    for (int i = 0; i < capacity; i++)
    {
        int n = i * 8;
        texCoords[n    ] = 0; texCoords[n + 1] = 0;
        texCoords[n + 2] = 1; texCoords[n + 3] = 0;
        texCoords[n + 4] = 1; texCoords[n + 5] = 1;
        texCoords[n + 6] = 0; texCoords[n + 7] = 1;
    }
}

Renderer::StarVertexBuffer::~StarVertexBuffer()
{
    if (vertices != NULL)
        delete vertices;
    if (colors != NULL)
        delete colors;
    if (texCoords != NULL)
        delete texCoords;
}

void Renderer::StarVertexBuffer::render()
{
    if (nStars != 0)
    {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, vertices);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDrawArrays(GL_QUADS, 0, nStars * 4);
        nStars = 0;
    }
}

void Renderer::StarVertexBuffer::addStar(const Point3f& pos,
                                         const Color& color,
                                         float size)
{
    if (nStars < capacity)
    {
        int n = nStars * 12;
        vertices[n + 0]  = pos.x + v0.x * size;
        vertices[n + 1]  = pos.y + v0.y * size;
        vertices[n + 2]  = pos.z + v0.z * size;
        vertices[n + 3]  = pos.x + v1.x * size;
        vertices[n + 4]  = pos.y + v1.y * size;
        vertices[n + 5]  = pos.z + v1.z * size;
        vertices[n + 6]  = pos.x + v2.x * size;
        vertices[n + 7]  = pos.y + v2.y * size;
        vertices[n + 8]  = pos.z + v2.z * size;
        vertices[n + 9]  = pos.x + v3.x * size;
        vertices[n + 10] = pos.y + v3.y * size;
        vertices[n + 11] = pos.z + v3.z * size;
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

void Renderer::StarVertexBuffer::setBillboardOrientation(const Quatf& q)
{
    Mat3f m = q.toMatrix3();
    v0 = Vec3f(-1, -1, 0) * m;
    v1 = Vec3f( 1, -1, 0) * m;
    v2 = Vec3f( 1,  1, 0) * m;
    v3 = Vec3f(-1,  1, 0) * m;
}
                               
