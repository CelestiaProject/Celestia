// render.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _RENDER_H_
#define _RENDER_H_

#include <vector>
#include <string>
#include <celengine/observer.h>
#include <celengine/universe.h>
#include <celengine/selection.h>
#include <celtxf/texturefont.h>


class Renderer
{
 public:
    Renderer();
    ~Renderer();
    
    bool init(int, int);
    void shutdown() {};
    void resize(int, int);

    float getFieldOfView();
    void setFieldOfView(float);

    void setRenderMode(int);

    void render(const Observer&,
                const Universe&,
                float faintestVisible,
                const Selection& sel,
                double now);
    
    // Convert window coordinates to a ray for picking
    Vec3f getPickRay(int winX, int winY);

    enum {
        NoLabels            = 0x00,
        StarLabels          = 0x01,
        PlanetLabels        = 0x02,
        MoonLabels          = 0x04,
        ConstellationLabels = 0x08,
        GalaxyLabels        = 0x10,
        AsteroidLabels      = 0x20,
        SpacecraftLabels    = 0x40,
        BodyLabelMask       = (PlanetLabels | MoonLabels | AsteroidLabels | SpacecraftLabels),
    };

    enum {
        ShowNothing         = 0x0000,
        ShowStars           = 0x0001,
        ShowPlanets         = 0x0002,
        ShowGalaxies        = 0x0004,
        ShowDiagrams        = 0x0008,
        ShowCloudMaps       = 0x0010,
        ShowOrbits          = 0x0020,
        ShowCelestialSphere = 0x0040,
        ShowNightMaps       = 0x0080,
        ShowAtmospheres     = 0x0100,
        ShowSmoothLines     = 0x0200,
        ShowEclipseShadows  = 0x0400,
        ShowStarsAsPoints   = 0x0800,
        ShowRingShadows     = 0x1000,
    };

    int getRenderFlags() const;
    void setRenderFlags(int);
    int getLabelMode() const;
    void setLabelMode(int);
    void addLabelledStar(Star*);
    void clearLabelledStars();
    float getAmbientLightLevel() const;
    void setAmbientLightLevel(float);
    void setMinimumOrbitSize(float);

    bool getFragmentShaderEnabled() const;
    void setFragmentShaderEnabled(bool);
    bool fragmentShaderSupported() const;
    bool getVertexShaderEnabled() const;
    void setVertexShaderEnabled(bool);
    bool vertexShaderSupported() const;

    float getSaturationMagnitude() const;
    void setSaturationMagnitude(float);
    float getBrightnessBias() const;
    void setBrightnessBias(float);
    void setResolution(unsigned int resolution);
    unsigned int getResolution();

    typedef struct {
        std::string text;
        Color color;
        Point3f position;
    } Label;

    void addLabel(std::string, Color, Point3f, float depth = -1);
    void clearLabels();

    void setFont(TextureFont*);
    TextureFont* getFont() const;

 public:
    // Internal types
    // TODO: Figure out how to make these private.  Even with a friend
    // 
    struct Particle
    {
        Point3f center;
        float size;
        Color color;
        float pad0, pad1, pad2;
    };

    typedef struct _RenderListEntry
    {
        const Star* star;
        Body* body;
        Point3f position;
        Vec3f sun;
        float distance;
        float radius;
        float nearZ;
        float farZ;
        float discSizeInPixels;
        float appMag;

        bool operator<(const _RenderListEntry& r) const
        {
            return distance - radius < r.distance - r.radius;
            // return z > r.z;
        }
    } RenderListEntry;

    struct EclipseShadow
    {
        Point3f origin;
        Vec3f direction;
        float penumbraRadius;
        float umbraRadius;
    };

    struct RenderProperties
    {
        RenderProperties() :
            surface(NULL),
            atmosphere(NULL),
            rings(NULL),
            radius(1.0f),
            oblateness(0.0f),
            mesh(InvalidResource),
            orientation(1.0f),
            eclipseShadows(NULL)
        {};

        Surface* surface;
        const Atmosphere* atmosphere;
        RingSystem* rings;
        RotationElements re;
        float radius;
        float oblateness;
        ResourceHandle mesh;
        Quatf orientation;
        std::vector<EclipseShadow>* eclipseShadows;
    };

    class StarVertexBuffer
    {
    public:
        StarVertexBuffer(unsigned int _capacity);
        ~StarVertexBuffer();
        void start(bool _usePoints);
        void render();
        void finish();
        void addStar(const Point3f&, const Color&, float);
        void setBillboardOrientation(const Quatf&);

    private:
        unsigned int capacity;
        unsigned int nStars;
        float* vertices;
        float* texCoords;
        unsigned char* colors;
        Vec3f v0, v1, v2, v3;
        bool usePoints;
    };

 private:
    void renderStars(const StarDatabase& starDB,
                     float faintestVisible,
                     const Observer& observer);
    void renderGalaxies(const GalaxyList& galaxies,
                        const Observer& observer);
    void renderCelestialSphere(const Observer& observer);
    void renderPlanetarySystem(const Star& sun,
                               const PlanetarySystem& solSystem,
                               const Observer& observer,
                               const Mat4d& frame,
                               double now,
                               bool showLabels = false);

    void renderObject(Point3f pos,
                      float distance,
                      double now,
                      Quatf cameraOrientation,
                      float nearPlaneDistance,
                      float farPlaneDistance,
                      Vec3f sunDirection,
                      Color sunColor,
                      RenderProperties& obj);

    void renderPlanet(const Body& body,
                      Point3f pos,
                      Vec3f sunDirection,
                      float distance,
                      float appMag,
                      double now,
                      Quatf orientation,
                      float, float);

    void renderStar(const Star& star,
                    Point3f pos,
                    float distance,
                    float appMag,
                    Quatf orientation,
                    double now,
                    float, float);

    void renderBodyAsParticle(Point3f center,
                              float appMag,
                              float _faintestMag,
                              float discSizeInPixels,
                              Color color,
                              const Quatf& orientation,
                              float renderDistance,
                              bool useHaloes);

    bool testEclipse(const Body&, const Body&, double now);

    void labelGalaxies(const GalaxyList& galaxies,
                    const Observer& observer);
    void labelStars(const std::vector<Star*>& stars,
                    const StarDatabase& starDB,
                    const Observer& observer);
    void labelConstellations(const AsterismList& asterisms,
                             const Observer& observer);
    void renderParticles(const std::vector<Particle>& particles,
                         Quatf orientation);
    void renderLabels();

    void renderOrbit(Body*, double);
    void renderOrbits(PlanetarySystem*, const Selection&, double,
                      const Point3d&, const Point3d&);
    
 private:
    int windowWidth;
    int windowHeight;
    float fov;
    float pixelSize;

    TextureFont* font;

    int renderMode;
    int labelMode;
    int renderFlags;
    float ambientLightLevel;
    bool fragmentShaderEnabled;
    bool vertexShaderEnabled;
    float brightnessBias;

    float brightnessScale;
    float faintestMag;
    float faintestPlanetMag;
    float saturationMagNight;
    float saturationMag;

    Color ambientColor;

    StarVertexBuffer* starVertexBuffer;
    std::vector<RenderListEntry> renderList;
    std::vector<Particle> glareParticles;
    std::vector<Label> labels;
    std::vector<EclipseShadow> eclipseShadows;

    std::vector<Star*> labelledStars;

    double modelMatrix[16];
    double projMatrix[16];

    int nSimultaneousTextures;
    bool useTexEnvCombine;
    bool useRegisterCombiners;
    bool useCubeMaps;
    bool useCompressedTextures;
    bool useVertexPrograms;
    bool useRescaleNormal;
    unsigned int textureResolution;

    struct CachedOrbit
    {
        Body* body;
        std::vector<Point3f> trajectory;
        bool keep;
    };
    std::vector<CachedOrbit*> orbitCache;

    float minOrbitSize;
};

#endif // _RENDER_H_
