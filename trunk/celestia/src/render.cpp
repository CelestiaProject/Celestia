// render.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include "gl.h"
#include "astro.h"
#include "glext.h"
#include "vecgl.h"
#include "perlin.h"
#include "spheremesh.h"
#include "texfont.h"
#include "console.h"
#include "gui.h"
#include "regcombine.h"
#include "render.h"

using namespace std;

#define FOV           45.0f
#define NEAR_DIST      0.5f
#define FAR_DIST   10000000.0f

#define RENDER_DISTANCE 50.0f

#define FAINTEST_MAGNITUDE  5.5f

static const float PixelOffset = 0.375f;

// Static meshes and textures used by all instances of Simulation

static bool commonDataInitialized = false;

#define SPHERE_LODS 5

static SphereMesh* sphereMesh[SPHERE_LODS];
static SphereMesh* asteroidMesh = NULL;

static CTexture* normalizationTex = NULL;
static CTexture* diffuseLightTex = NULL;

static CTexture* starTex = NULL;
static CTexture* glareTex = NULL;
static CTexture* galaxyTex = NULL;
static CTexture* shadowTex = NULL;
static CTexture* veilTex = NULL;

static TexFont* font = NULL;


Renderer::Renderer() :
    windowWidth(0),
    windowHeight(0),
    fov(FOV),
    renderMode(GL_FILL),
    asterisms(NULL),
    renderFlags(ShowStars | ShowPlanets),
    labelMode(NoLabels),
    ambientLightLevel(0.1f),
    brightnessScale(1.0f / 6.0f),
    brightnessBias(0.0f),
    perPixelLightingEnabled(false),
    console(NULL),
    nSimultaneousTextures(1),
    useRegisterCombiners(false),
    useCubeMaps(false)
{
    textureManager = new TextureManager("textures");
    meshManager = new MeshManager("models");
    console = new Console(30, 100);
}


Renderer::~Renderer()
{
}


static void BlueTextureEval(float u, float v, float w,
                            unsigned char *pixel)
{
    pixel[0] = 128;
    pixel[1] = 128;
    pixel[2] = 255;
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

static void VeilTextureEval(float u, float v, float w,
                            unsigned char* pixel)
{
    float t = 0.0f;
    if (w > 0)
    {
        t = 1.0f - clamp(abs(w - 0.04f) * 25);
    }
        
    pixel[0] = 0;
    pixel[1] = 0;
    pixel[2] = 255;
    pixel[3] = (int) (128.99f * t);
    // pixel[3] = (int) (128.99f * (1 - (float) pow(abs(w), 0.5f)));
}

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
    return 2 * NEAR_DIST * (float) tan(degToRad(fovY / 2.0)) / (float) windowHeight;
}


bool Renderer::init(int winWidth, int winHeight)
{
    // Initialize static meshes and textures common to all instances of Renderer
    if (!commonDataInitialized)
    {
        sphereMesh[0] = new SphereMesh(1.0f, 11, 10);
        sphereMesh[1] = new SphereMesh(1.0f, 21, 40);
        sphereMesh[2] = new SphereMesh(1.0f, 31, 60);
        sphereMesh[3] = new SphereMesh(1.0f, 41, 80);
        sphereMesh[4] = new SphereMesh(1.0f, 61, 120);
        asteroidMesh = new SphereMesh(Vec3f(0.7f, 1.1f, 1.0f),
                                      21, 20,
                                      AsteroidDisplacementFunc,
                                      NULL);

        starTex = CreateProceduralTexture(64, 64, GL_RGB, StarTextureEval);
        starTex->bindName();

        galaxyTex = CreateProceduralTexture(128, 128, GL_RGBA, GlareTextureEval);
        galaxyTex->bindName();

        glareTex = CreateJPEGTexture("textures\\flare.jpg");
        if (glareTex == NULL)
            glareTex = CreateProceduralTexture(64, 64, GL_RGB, GlareTextureEval);
        glareTex->bindName();

        shadowTex = CreateProceduralTexture(256, 256, GL_RGB, ShadowTextureEval);
        shadowTex->bindName();

        // font = txfLoadFont("fonts\\helvetica_14b.txf");
        font = txfLoadFont("fonts\\default.txf");
        if (font != NULL)
            txfEstablishTexture(font, 0, GL_FALSE);

        // Initialize GL extensions
        if (ExtensionSupported("GL_ARB_multitexture"))
            InitExtMultiTexture();
        if (ExtensionSupported("GL_NV_register_combiners"))
            InitExtRegisterCombiners();
        if (ExtensionSupported("GL_EXT_texture_cube_map"))
        {
            normalizationTex = CreateNormalizationCubeMap(64);
            // diffuseLightTex = CreateDiffuseLightCubeMap(64);
        }

        commonDataInitialized = true;
    }

#if 0
    {
        Galaxy* g = new Galaxy();
        g->setName("Milky Way");
        g->setPosition(Point3d(28000, 20, 0));
        Quatf q(1);
        q.setAxisAngle(Vec3f(1, 0, 0), degToRad(50.0f));
        g->setOrientation(q);
        g->setRadius(50000.0f);
        g->setType(Galaxy::SBa);
        galaxies.insert(galaxies.end(), g);

        g = new Galaxy();
        g->setName("Andromeda");
        g->setPosition(Point3d(2000000, 1000000, 2000000));
        g->setRadius(95000.0f);
        g->setType(Galaxy::Sb);
        galaxies.insert(galaxies.end(), g);

        g = new Galaxy();
        g->setName("LMC");
        g->setPosition(Point3d(100000, -100000, 100000));
        g->setRadius(15000.0f);
        g->setType(Galaxy::Irr);
        galaxies.insert(galaxies.end(), g);

        g = new Galaxy();
        g->setName("SMC");
        g->setPosition(Point3d(50000, -150000, 80000));
        g->setRadius(12500.0f);
        g->setType(Galaxy::Irr);
        galaxies.insert(galaxies.end(), g);
    }
#endif

    cout << "GL extensions supported:\n";
    cout << glGetString(GL_EXTENSIONS) << '\n';

    // Get GL extension information
    if (ExtensionSupported("GL_ARB_multitexture"))
    {
        DPRINTF("Renderer: multi-texture supported.\n");
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &nSimultaneousTextures);
    }
    if (ExtensionSupported("GL_NV_register_combiners"))
    {
        DPRINTF("Renderer: nVidia register combiners supported.\n");
        useRegisterCombiners = true;
    }
    if (ExtensionSupported("GL_EXT_texture_cube_map"))
    {
        DPRINTF("Renderer: cube texture maps supported.\n");
        useCubeMaps = true;
    }

    cout << "Simultaneous textures supported: " << nSimultaneousTextures << '\n';

    if (useCubeMaps)
    {
        // Initialize texture use for rendering atmospheric veils
        veilTex = CreateProceduralCubeMap(128, GL_RGBA, VeilTextureEval);
        veilTex->bindName();
    }


    glLoadIdentity();

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);

    // LEQUAL rather than LESS required for multipass rendering
    glDepthFunc(GL_LEQUAL);

    // We need this enabled because we use glScale, but only
    // with uniform scale factors
    // TODO: Do a proper check for this extension before enabling
    glEnable(GL_RESCALE_NORMAL_EXT);

    console->setFont(font);

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


Console* Renderer::getConsole() const
{
    return console;
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


bool Renderer::getPerPixelLighting() const
{
    return perPixelLightingEnabled;
}

void Renderer::setPerPixelLighting(bool enable)
{
    perPixelLightingEnabled = enable && perPixelLightingSupported();
}

bool Renderer::perPixelLightingSupported() const
{
    return useCubeMaps && useRegisterCombiners;
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
                      const VisibleStarSet& visset,
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
    glRotate(~observer.getOrientation());

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

    if ((renderFlags & ShowGalaxies) != 0 && galaxies != NULL)
        renderGalaxies(*galaxies, observer);

    // Translate the camera before rendering the stars
    glPushMatrix();
    glTranslatef(-observerPos.x, -observerPos.y, -observerPos.z);
    // Render stars
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    if ((renderFlags & ShowStars) != 0)
        renderStars(starDB, visset, observer);

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

    glPolygonMode(GL_FRONT, renderMode);
    glPolygonMode(GL_BACK, renderMode);

    // Render planets, moons, asteroids, etc.  Stars close and large enough
    // to have discernible surface detail are also placed in renderList.
    // planetParticles.clear();
    Star* sun = NULL;
    if (solarSystem != NULL)
        sun = starDB.find(solarSystem->getStarNumber());
    if (sun != NULL)
    {
        // Change the projection matrix for rendering planets and moons.  Since
        // planets are all rendered at a fixed distance of RENDER_DISTANCE from
        // the viewer, we'll set up the near and far planes to just enclose that
        // range and make the most of our depth buffer resolution
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(fov,
                       (float) windowWidth / (float) windowHeight,
                       NEAR_DIST, RENDER_DISTANCE * 2.0f);
        glMatrixMode(GL_MODELVIEW);

        renderPlanetarySystem(*sun,
                              *solarSystem->getPlanets(),
                              observer,
                              Mat4d::identity(), now,
                              (labelMode & (MinorPlanetLabels | MajorPlanetLabels)) != 0);
        glBindTexture(GL_TEXTURE_2D, starTex->getName());
        // renderParticles(planetParticles, observer.getOrientation());

        // The call to renderSolarSystem filled renderList with visible
        // planetary bodies.  Sort it by distance, then render each entry.
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
        for (int i = 0; i < nEntries; i++)
        {
            if (renderList[i].discSizeInPixels > 1)
                nDepthBuckets++;
        }
        float depthRange = 1.0f / (float) nDepthBuckets;

        int depthBucket = nDepthBuckets - 1;
        i = nEntries - 1;

        // Set up the depth bucket.
        glDepthRange(depthBucket * depthRange, (depthBucket + 1) * depthRange);

        // Render all the bodies in the render list.
        for (i = nEntries - 1; i >= 0; i--)
        {
            if (renderList[i].body != NULL)
            {
                renderPlanet(*renderList[i].body,
                             renderList[i].position,
                             renderList[i].sun,
                             renderList[i].distance,
                             renderList[i].appMag,
                             now,
                             observer.getOrientation());
            }
            else if (renderList[i].star != NULL)
            {
                renderStar(*renderList[i].star,
                           renderList[i].position,
                           renderList[i].distance,
                           renderList[i].appMag,
                           observer.getOrientation(),
                           now);
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

        if ((renderFlags & ShowOrbits) != 0)
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

            Star* sun = starDB.find(solarSystem->getStarNumber());
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
#if 0
            // Render axes in orbital plane for debugging
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

    // #define DISPLAY_AXES
#ifdef DISPLAY_AXES
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    {
        Point3f orig(-0.5f, -0.5f, -2);
        Mat3f m = conjugate(observer.getOrientation()).toMatrix3();

        glBegin(GL_LINES);
        glColor4f(1, 0, 0, 1);
        glVertex(orig);
        glVertex(orig + Vec3f(0.2f, 0, 0) * m);
        glColor4f(0, 1, 0, 1);
        glVertex(orig);
        glVertex(orig + Vec3f(0, 0.2f, 0) * m);
        glColor4f(0, 0, 1, 1);
        glVertex(orig);
        glVertex(orig + Vec3f(0, 0, 0.2f) * m);
        glEnd();
    }
#endif

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_FILL);

    renderLabels();

    glColor4f(0.8f, 0.8f, 1.0f, 1);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(PixelOffset, windowHeight - 20 + PixelOffset, 0);
    console->render();
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
}


static void renderParticle(Point3f& center,
                           Vec3f& v0,
                           Vec3f& v1,
                           Vec3f& v2,
                           Vec3f& v3,
                           float size)
{
    glTexCoord2f(0, 0);
    glVertex(center + (v0 * size));
    glTexCoord2f(1, 0);
    glVertex(center + (v1 * size));
    glTexCoord2f(1, 1);
    glVertex(center + (v2 * size));
    glTexCoord2f(0, 1);
    glVertex(center + (v3 * size));
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
                                    bool useHaloes)
{
    if (discSizeInPixels < 4 || useHaloes)
    {
        float r = 1, g = 1, b = 1;
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

        // We scale up the particle by a factor of 3 so that it's more visible--the
        // texture we use has fuzzy edges, and if we render it in just one pixel,
        // it's likely to disappear.  Also, the render distance is scaled by a factor
        // of 0.1 so that we're rendering in front of any mesh that happens to be
        // sharing this depth bucket.  What we really want is to render the particle
        // with the frontmost z value in this depth bucket, and scaling the render
        // distance is just hack to accomplish this.  There are cases where it will
        // fail and a more robust method should be implemented.
        float size = pixelSize * 3.0f * RENDER_DISTANCE * 0.1f;
        Point3f center(position.x * 0.1f, position.y * 0.1f, position.z * 0.1f);
        Mat3f m = orientation.toMatrix3();
        Vec3f v0 = Vec3f(-1, -1, 0) * m;
        Vec3f v1 = Vec3f( 1, -1, 0) * m;
        Vec3f v2 = Vec3f( 1,  1, 0) * m;
        Vec3f v3 = Vec3f(-1,  1, 0) * m;

        glBindTexture(GL_TEXTURE_2D, starTex->getName());
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
        // TODO: Currently, this is extremely broken.  Stars look fine, but planets
        // look ridiculous with bright haloes.
        if (useHaloes && appMag < 1.0f)
        {
            a = 0.4f * clamp((appMag - 1) * -0.8f);
            // size *= (3 - (appMag - 1)) * 2;
            // size = RENDER_DISTANCE * 0.001f * (3 - (appMag - 1)) * 1;
            // size = discSizeInPixels * 3 * pixelSize;
            // size *= 30.0f;
            float s = RENDER_DISTANCE * 0.0001f * (3 - (appMag - 1)) * 2;
            if (s > size * 3)
                size = s;
            else
                size = size * 3;
            float realSize = discSizeInPixels * (pixelSize / NEAR_DIST) * RENDER_DISTANCE * 0.1f;
            if (size < realSize * 10)
                size = realSize * 10;
            glBindTexture(GL_TEXTURE_2D, glareTex->getName());
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
                                 CTexture& bumpTexture,
                                 Vec3f lightDirection,
                                 Quatf orientation,
                                 Color ambientColor)
{
    // We're doing our own per-pixel lighting, so disable GL's lighting
    glDisable(GL_LIGHTING);

    // Render the base texture on the first pass . . .  The base
    // texture and color should have been set up already by the
    // caller.
    mesh.render();

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
    glRotate(lightOrientation * orientation);
    glMatrixMode(GL_MODELVIEW);
    glActiveTextureARB(GL_TEXTURE0_ARB);

    mesh.render();

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
                             CTexture& baseTexture,
                             Vec3f lightDirection,
                             Quatf orientation,
                             Color ambientColor)
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

    SetupCombinersSmooth(baseTexture, *normalizationTex, ambientColor);

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
    glRotate(lightOrientation * orientation);
    glMatrixMode(GL_MODELVIEW);
    glActiveTextureARB(GL_TEXTURE0_ARB);

    mesh.render();

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



void Renderer::renderPlanet(const Body& body,
                            Point3f pos,
                            Vec3f sunDirection,
                            float distance,
                            float appMag,
                            double now,
                            Quatf orientation)
{
    float discSizeInPixels = body.getRadius() / distance / (pixelSize / NEAR_DIST);

    if (discSizeInPixels > 1)
    {
        // Enable depth buffering
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        sunDirection.normalize();

        // Set up the light source for the sun
        glLightDirection(GL_LIGHT0, sunDirection);
        glLightColor(GL_LIGHT0, GL_DIFFUSE, Vec3f(1.0f, 1.0f, 1.0f));
        glEnable(GL_LIGHT0);

        const Surface& surface = body.getSurface();
        // Get the texture . . .
        CTexture* tex = NULL;
        CTexture* bumpTex = NULL;
        CTexture* cloudTex = NULL;
        if (surface.baseTexture != "")
        {
            if (!textureManager->find(surface.baseTexture, &tex))
            {
                bool compress = ((surface.appearanceFlags & Surface::CompressBaseTexture) != 0);
                tex = textureManager->load(surface.baseTexture, compress);
            }
        }

        // If this renderer can support bump mapping then get the bump texture
        if ((surface.appearanceFlags & Surface::ApplyBumpMap) != 0 &&
            (perPixelLightingEnabled && useRegisterCombiners && useCubeMaps) &&
            surface.bumpTexture != "")
        {
            if (!textureManager->find(surface.bumpTexture, &bumpTex))
                bumpTex = textureManager->loadBumpMap(surface.bumpTexture,
                                                      surface.bumpHeight);
        }

        if ((surface.appearanceFlags & Surface::ApplyCloudMap) != 0 &&
            (renderFlags & ShowCloudMaps) != 0)
        {
            if (!textureManager->find(surface.cloudTexture, &cloudTex))
                cloudTex = textureManager->load(surface.cloudTexture, false);
        }

        if (tex == NULL)
        {
            glDisable(GL_TEXTURE_2D);
        }
        else
        {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, tex->getName());
        }

        if (tex == NULL || (surface.appearanceFlags & Surface::BlendTexture) != 0)
            glColor(surface.color);
        else
            glColor4f(1, 1, 1, 1);

        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
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
            planetRotation = -remainder * 2 * PI;
            glRotatef((float) (-remainder * 360.0), 0, 1, 0);
        }

        float discSize = body.getRadius() / distance * RENDER_DISTANCE;

        // Apply a scale factor which depends on the apparent size of the
        // planet and its oblateness.  Since the oblateness is usually quite
        // small, the potentially nonuniform scale factor shouldn't mess up
        // the lighting calculations enough to cause a problem.
        // TODO:  Figure out a better way to render ellipsoids than applying
        // a nonunifom scale factor to a sphere . . . it makes me nervous.
        glScalef(discSize, discSize * (1.0f - body.getOblateness()), discSize);

        Mesh* mesh = NULL;
        if (body.getMesh() == "")
        {
            int lod = 0;
            if (discSizeInPixels < 10)
                lod = 0;
            else if (discSizeInPixels < 50)
                lod = 1;
            else if (discSizeInPixels < 200)
                lod = 2;
            else if (discSizeInPixels < 400)
                lod = 3;
            else
                lod = 4;
    
            if (body.getRadius() < 50)
                mesh = asteroidMesh;
            else
                mesh = sphereMesh[lod];
        }
        else
        {
            if (!meshManager->find(body.getMesh(), &mesh))
                mesh = meshManager->load(body.getMesh());
        }

        if (mesh != NULL)
        {
            if (perPixelLightingEnabled)
            {
                Color ambientColor(ambientLightLevel, ambientLightLevel, ambientLightLevel);
                if (bumpTex != NULL)
                {
                    renderBumpMappedMesh(*mesh,
                                         *bumpTex,
                                         sunDirection, orientation,
                                         ambientColor);
                }
                else if (tex != NULL)
                {
                    renderSmoothMesh(*mesh,
                                     *tex,
                                     sunDirection, orientation,
                                     ambientColor);
                }
                else
                {
                    mesh->render();
                }

                if (cloudTex != NULL)
                {
                    glBindTexture(GL_TEXTURE_2D, cloudTex->getName());
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if 0
                    // TODO: Enable per-pixel lighting for cloud maps
                    renderSmoothMesh(*mesh, *cloudTex, sunDirection, orientation,
                                     ambientColor);
#else
                    glEnable(GL_LIGHTING);
                    mesh->render();
#endif

#if 0
                    // Attempt rendering an atmospheric veil . . . this needs
                    // several adjustments before it will work.
                    glEnable(GL_TEXTURE_CUBE_MAP_EXT);
                    glBindTexture(GL_TEXTURE_CUBE_MAP_EXT, veilTex->getName());
                    glPushMatrix();
                    glScalef(1.02f, 1.02f, 1.02f);

                    // Set up GL_NORMAL_MAP_EXT texture coordinate generation.  This
                    // mode is part of the cube map extension.
                    glEnable(GL_TEXTURE_GEN_R);
                    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_EXT);
                    glEnable(GL_TEXTURE_GEN_S);
                    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_EXT);
                    glEnable(GL_TEXTURE_GEN_T);
                    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_EXT);

                    mesh->render();

                    glMatrixMode(GL_TEXTURE);
                    glLoadIdentity();
                    glMatrixMode(GL_MODELVIEW);
                    glDisable(GL_TEXTURE_GEN_R);
                    glDisable(GL_TEXTURE_GEN_S);
                    glDisable(GL_TEXTURE_GEN_T);
                    glDisable(GL_TEXTURE_CUBE_MAP_EXT);

                    glPopMatrix();

#if 0
                    // Render an atmospheric halo.  Broken.
                    {
                        int nSections = 400;
                        glDisable(GL_LIGHTING);
                        glPushMatrix();
                        glRotate(~orientation);
                        glBegin(GL_QUAD_STRIP);
                        for (int i = 0; i <= nSections; i++)
                        {
                            float theta = (float) i / (float) nSections * 2 * PI;
                            float c = (float) cos(theta);
                            float s = (float) sin(theta);
                            glColor4f(0, 0, 1, 0);
                            glVertex3f(c * 1.01f, s * 1.01f, 0);
                            glColor4f(0, 0, 1, 1);
                            glVertex3f(c, s, 0);
                        }
                        glEnd();
                        glPopMatrix();
                    }
#endif
#endif
                }
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            }
            else
            {
                mesh->render();
                if (cloudTex != NULL)
                {
                    glBindTexture(GL_TEXTURE_2D, cloudTex->getName());
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    mesh->render();
                }
            }
        }

        // If the planet has a ring system, render it.
        if (body.getRings() != NULL)
        {
            RingSystem* rings = body.getRings();
            float inner = rings->innerRadius / body.getRadius();
            float outer = rings->outerRadius / body.getRadius();
            int nSections = 100;

            // Convert the sun direction to the planet's coordinate system
            Mat4f planetMat = Mat4f::xrotation((float) body.getObliquity()) *
                Mat4f::yrotation((float) planetRotation);
            Vec3f localSunDir = sunDirection * planetMat;

            // Ring Illumination
            // Since a ring system is composed of millions of individual particles,
            // it's not at all realistic to model it as a flat Lambertian surface.
            // We'll approximate the llumination function by assuming that the ring
            // system contains Lambertian particles, and that the brightness at some
            // point in the ring system is proportional to the illuminated fraction
            // of a particle there.  In fact, we'll simplify things further and set
            // the illumination of the entire ring system to the same value, computing
            // the illuminated fraction of a hypothetical particle located at the center
            // of the planet.  This approximation breaks down when you get close to the
            // planet . . .
            float ringIllumination = 0.0f;
            {
                // Compute the direction from the viewer to the planet in the
                // planet's coordinate system
                Vec3f viewVec = (Point3f(0, 0, 0) - pos) * planetMat;
                viewVec.normalize();
                float illumFraction = (1.0f + viewVec * localSunDir) / 2.0f;

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
                glBindTexture(GL_TEXTURE_2D, shadowTex->getName());

                float sPlane[4] = { 0, 0, 0, 0.5f };
                float tPlane[4] = { 0, 0, 0, 0.5f };

            // Compute the projection vectors based on the sun direction.
            // I'm being a little careless here--if the sun direction lies
            // along the y-axis, this will fail.  It's unlikely that a planet
            // would ever orbit underneath its sun (an orbital inclination of
            // 90 degrees), but this should be made more robust anyway.
                Vec3f axis = Vec3f(0, 1, 0) ^ localSunDir;
                float angle = (float) acos(Vec3f(0, 1, 0) * localSunDir);
                Mat4f mat = Mat4f::rotation(axis, -angle);
                Vec3f sAxis = Vec3f(0.5f, 0, 0) * mat;
                Vec3f tAxis = Vec3f(0, 0, 0.5f) * mat;

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

            CTexture* ringsTex = NULL;
            if (rings->texture != "")
            {
                if (!textureManager->find(rings->texture, &ringsTex))
                    ringsTex = textureManager->load(rings->texture);
            }

            if (ringsTex != NULL)
                glBindTexture(GL_TEXTURE_2D, ringsTex->getName());
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

            // This gets tricky . . .  we render the rings in two parts.  One part
            // is potentially shadowed by the planet, and we need to render that part
            // with the projected shadow texture enabled.  The other part isn't shadowed,
            // but will appear so if we don't first disable the shadow texture.  The problem
            // is that the shadow texture will affect anything along the line between the
            // sun and the planet, regardless of whether it's in front or behing the planet.

            // Compute the angle of the sun projected on the ring plane
            float sunAngle = (float) atan2(localSunDir.z, localSunDir.x);

            // Render the potentially shadowed side
            // glNormal3f(0, 1, 0);
            renderRingSystem(inner, outer,
                             (float) (sunAngle + PI / 2), (float) (sunAngle + 3 * PI / 2),
                             nSections / 2);
            // glNormal3f(0, -1, 0);
            renderRingSystem(inner, outer,
                             (float) (sunAngle +  3 * PI / 2), (float) (sunAngle + PI / 2),
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
                             (float) (sunAngle - PI / 2), (float) (sunAngle + PI / 2),
                             nSections / 2);
            // glNormal3f(0, -1, 0);
            renderRingSystem(inner, outer,
                             (float) (sunAngle + PI / 2), (float) (sunAngle - PI / 2),
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
                         false);
#if 0
    // If the size of the planetary disc is under a pixel, we don't
    // render the mesh for the planet and just display a starlike point instead.
    // Switching between the particle and mesh renderings of a body is
    // jarring, however . . . so we'll blend in the particle view of the
    // body to smooth things out, making it dimmer as the disc size approaches
    // 4 pixels.
    if (discSizeInPixels < 4)
    {
        float r = 1, g = 1, b = 1;
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

        // We scale up the particle by a factor of 3 so that it's more visible--the
        // texture we use has fuzzy edges, and if we render it in just one pixel,
        // it's likely to disappear.  Also, the render distance is scaled by a factor
        // of 0.1 so that we're rendering in front of any mesh that happens to be
        // sharing this depth bucket.  What we really want is to render the particle
        // with the frontmost z value in this depth bucket, and scaling the render
        // distance is just hack to accomplish this.  There are cases where it will
        // fail and a more robust method should be implemented.
        float size = pixelSize * 3.0f * RENDER_DISTANCE * 0.1f;
        Point3f center(pos.x * 0.1f, pos.y * 0.1f, pos.z * 0.1f);
        Mat3f m = orientation.toMatrix3();
        Vec3f v0 = Vec3f(-1, -1, 0) * m;
        Vec3f v1 = Vec3f( 1, -1, 0) * m;
        Vec3f v2 = Vec3f( 1,  1, 0) * m;
        Vec3f v3 = Vec3f(-1,  1, 0) * m;

        glBindTexture(GL_TEXTURE_2D, starTex->getName());
        glColor(body.getColor(), a);
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
#endif
}


#if 1
void Renderer::renderStar(const Star& star,
                          Point3f pos,
                          float distance,
                          float appMag,
                          Quatf orientation,
                          double now)
{
    Color color = star.getStellarClass().getApparentColor();
    float radius = star.getRadius();
    float discSizeInPixels = radius / distance / (pixelSize / NEAR_DIST);
    float discSize = radius / distance * RENDER_DISTANCE;

    if (discSizeInPixels > 1)
    {
        // Enable depth buffering
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        glDisable(GL_BLEND);
        glPushMatrix();
        glTranslate(pos);

        glColor(color);
        glScalef(discSize, discSize, discSize);

        char* textureName = NULL;
        switch (star.getStellarClass().getSpectralClass())
        {
        case StellarClass::Spectral_O:
        case StellarClass::Spectral_B:
            textureName = "bstar.jpg";
            break;
        case StellarClass::Spectral_A:
        case StellarClass::Spectral_F:
            textureName = "astar.jpg";
            break;
        case StellarClass::Spectral_G:
        case StellarClass::Spectral_K:
            textureName = "gstar.jpg";
            break;
        case StellarClass::Spectral_M:
        case StellarClass::Spectral_R:
        case StellarClass::Spectral_S:
        case StellarClass::Spectral_N:
            textureName = "mstar.jpg";
            break;
        default:
            textureName = "astar.jpg";
            break;
        }

        CTexture* tex = NULL;
        if (!textureManager->find(textureName, &tex))
            tex = textureManager->load(textureName);
        if (tex == NULL)
        {
            glDisable(GL_TEXTURE_2D);
        }
        else
        {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, tex->getName());
        }

        int lod = 0;
        if (discSizeInPixels < 10)
            lod = 0;
        else if (discSizeInPixels < 50)
            lod = 1;
        else if (discSizeInPixels < 200)
            lod = 2;
        else
            lod = 3;

        // Rotate the star
        {
            // Use doubles to avoid precision problems here . . .
            double rotations = now / (double) star.getRotationPeriod();
            int wholeRotations = (int) rotations;
            double remainder = rotations - wholeRotations;
            glRotatef((float) (-remainder * 360.0), 0, 1, 0);
        }

        sphereMesh[lod]->render();;

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
                         true);
}
#endif


void Renderer::renderPlanetarySystem(const Star& sun,
                                     const PlanetarySystem& solSystem,
                                     const Observer& observer,
                                     Mat4d& frame,
                                     double now,
                                     bool showLabels)
{
    float size = pixelSize * 3.0f;

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
        double distanceFromSun = bodyPos.distanceFromOrigin();
        
        // We now have the positions of the observer and the planet relative
        // to the sun.  From these, compute the position of the planet
        // relative to the observer.
        Vec3d posd = bodyPos - observerPos;

        double distanceFromObserver = posd.length();
        float appMag = body->getApparentMagnitude(sun,
                                                  bodyPos - Point3d(0, 0, 0),
                                                  posd);

        // Scale the position of the body so that it lies at RENDER_DISTANCE units
        // from the observer.
        if (distanceFromObserver > 0.0)
            posd *= (RENDER_DISTANCE / distanceFromObserver);
        Vec3f pos((float) posd.x, (float) posd.y, (float) posd.z);

        float s = (float) RENDER_DISTANCE * size;

        // Compute the size of the planet/moon disc in pixels
        float discSize = (body->getRadius() / (float) distanceFromObserver) / (pixelSize / NEAR_DIST);

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
            if (body->getRadius() >= 500.0 && (labelMode & MajorPlanetLabels) != 0)

            {
                addLabel(body->getName(),
                         Color(0.0f, 1.0f, 0.0f),
                         Point3f(pos.x, pos.y, pos.z));
            }
            else if (body->getRadius() < 500.0 && (labelMode & MinorPlanetLabels) != 0)
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


void Renderer::renderStars(const StarDatabase& starDB,
                           const VisibleStarSet& visset,
                           const Observer& observer)
{
    vector<uint32>* vis = visset.getVisibleSet();
    int nStars = vis->size();
    Point3f observerPos = (Point3f) observer.getPosition();
    Vec3f viewNormal = Vec3f(0, 0, -1) * observer.getOrientation().toMatrix3();

    float size = pixelSize * 3.0f;

    starParticles.clear();
    glareParticles.clear();

    for (int i = 0; i < nStars; i++)
    {
        Star* star = starDB.getStar((*vis)[i]);
        Point3f pos = star->getPosition();
        Vec3f relPos = pos - observerPos;
        if (relPos * viewNormal > 0 || relPos.x * relPos.x < 0.1f)
        {
            float distance = relPos.length();

            Color starColor = star->getStellarClass().getApparentColor();
            float alpha = 0.0f;
            float renderDistance = distance;
            float s = renderDistance * size;
            float discSizeInPixels = 0.0f;

            // Special handling for stars less than one light year away . . .
            // We can't just go ahead and render a nearby star in the usual way
            // for two reasons:
            //   * It may be clipped by the near plane
            //   * It may be large enough that we should render it as a mesh instead
            //     of a particle
            // It's possible that the second condition might apply for stars further
            // than one light year away if the star is huge, the fov is very small,
            // and the resolution is high.  We'll ignore this for now and use the
            // most inexpensive test possible . . .
            if (distance < 1.0f)
            {
                // Compute the position of the observer relative to the star.
                // This is a much more accurate (and expensive) distance
                // calculation than the previous one which used the observer's
                // position rounded off to floats.
                relPos = pos - observer.getPosition();
                distance = relPos.length();

                float f = RENDER_DISTANCE / distance;
                renderDistance = RENDER_DISTANCE;
                pos = observerPos + relPos * f;

                float radius = star->getRadius();
                // s = renderDistance * size + astro::kilometersToLightYears(radius * f);
                discSizeInPixels = radius / astro::lightYearsToKilometers(distance) /
                    (pixelSize / NEAR_DIST);
            }

            float appMag = astro::lumToAppMag(star->getLuminosity(), distance);
        
            alpha = clamp(1.0f - appMag * brightnessScale + brightnessBias);

            if (discSizeInPixels <= 1)
            {
                Particle p;
                p.center = pos;
                p.size = renderDistance * size;
                p.color = Color(starColor, alpha);
                starParticles.insert(starParticles.end(), p);

                // If the star is brighter than magnitude 1, add a halo around it to
                // make it appear more brilliant.  This is a hack to compensate for the
                // limited dynamic range of monitors.
                if (appMag < 1.0f)
                {
                    alpha = 0.4f * clamp((appMag - 1) * -0.8f);
                    s = renderDistance * 0.001f * (3 - (appMag - 1)) * 2;

                    if (s > p.size * 3)
                        p.size = s;
                    else
                        p.size = p.size * 3;
                    p.color = Color(starColor, alpha);
                    glareParticles.insert(glareParticles.end(), p);
                }
            }
            else
            {
                RenderListEntry rle;
                rle.star = star;
                rle.body = NULL;

                // Objects in the render list are always rendered relative to a viewer
                // at the origin--this is different than for distance stars.
                float scale = RENDER_DISTANCE / distance;
                rle.position = Point3f(relPos.x * scale, relPos.y * scale, relPos.z * scale);

                rle.distance = astro::lightYearsToKilometers(distance);
                rle.discSizeInPixels = discSizeInPixels;
                rle.appMag = appMag;
                renderList.insert(renderList.end(), rle);
            }
        }
    }

    glBindTexture(GL_TEXTURE_2D, starTex->getName());
    renderParticles(starParticles, observer.getOrientation());
    glBindTexture(GL_TEXTURE_2D, glareTex->getName());
    renderParticles(glareParticles, observer.getOrientation());
}


void Renderer::renderGalaxies(const GalaxyList& galaxies,
                              const Observer& observer)
{
    Vec3f viewNormal = Vec3f(0, 0, -1) * observer.getOrientation().toMatrix3();
    Point3d observerPos = (Point3d) observer.getPosition();
    Mat3f viewMat = observer.getOrientation().toMatrix3();
    Vec3f v0 = Vec3f(-1, -1, 0) * viewMat;
    Vec3f v1 = Vec3f( 1, -1, 0) * viewMat;
    Vec3f v2 = Vec3f( 1,  1, 0) * viewMat;
    Vec3f v3 = Vec3f(-1,  1, 0) * viewMat;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, galaxyTex->getName());

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
        float minimumFeatureSize = pixelSize * distanceToGalaxy;

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

            glBegin(GL_QUADS);
            for (int i = 0; i < points->size(); i++)
            {
                Point3f p = (*points)[i] * m;
                Vec3f relPos = p - offset;

                if ((i & pow2) != 0)
                {
                    pow2 <<= 1;
                    size /= 1.5f;
                    if (size < minimumFeatureSize)
                    {
                        // cout << galaxy->getName() << ": Quitting after " << i << " particles.\n";
                        break;
                    }
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
        float appMag = astro::lumToAppMag(star->getLuminosity(), distance);
        
        if (appMag < 6.0f)
        {
            Vec3f rpos = pos - observerPos;
            if ((rpos * conjugate(observer.getOrientation()).toMatrix3()).z < 0)
            {
                addLabel(starDB.getStarName(star->getCatalogNumber()),
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
    float xscale = 2.0f / (float) windowWidth;
    float yscale = 2.0f / (float) windowHeight;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, font->texobj);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef((int) (windowWidth / 2), (int) (windowHeight / 2), 0);

    for (int i = 0; i < labels.size(); i++)
    {
        glColor(labels[i].color);
        glPushMatrix();
        glTranslatef((int) labels[i].position.x + PixelOffset,
                     (int) labels[i].position.y + PixelOffset,
                     0);
        txfRenderString(font, labels[i].text);
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
