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
#include "stardb.h"
#include "observer.h"
#include "solarsys.h"
#include "galaxy.h"
#include "asterism.h"
#include "selection.h"
#include "texturefont.h"


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
                const StarDatabase&,
                float faintestVisible,
                SolarSystem*,
                GalaxyList*,
                const Selection& sel,
                double now);
    
    // Convert window coordinates to a ray for picking
    Vec3f getPickRay(int winX, int winY);

    enum {
        NoLabels = 0,
        StarLabels = 1,
        MajorPlanetLabels = 2,
        MinorPlanetLabels = 4,
        ConstellationLabels = 8,
    };
    enum {
        ShowNothing         =   0,
        ShowStars           =   1,
        ShowPlanets         =   2,
        ShowGalaxies        =   4,
        ShowDiagrams        =   8,
        ShowCloudMaps       =  16,
        ShowOrbits          =  32,
        ShowCelestialSphere =  64,
        ShowNightMaps       = 128,
    };
    int getRenderFlags() const;
    void setRenderFlags(int);
    int getLabelMode() const;
    void setLabelMode(int);
    void addLabelledStar(Star*);
    void clearLabelledStars();
    float getAmbientLightLevel() const;
    void setAmbientLightLevel(float);

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

    void showAsterisms(AsterismList*);

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
        float nearZ;
        float farZ;
        float discSizeInPixels;
        float appMag;

        bool operator<(const _RenderListEntry& r) const
        {
            return distance < r.distance;
            // return z > r.z;
        }
    } RenderListEntry;

    class StarVertexBuffer
    {
    public:
        StarVertexBuffer(unsigned int _capacity);
        ~StarVertexBuffer();
        void render();
        void addStar(const Point3f&, const Color&, float);
        void setBillboardOrientation(const Quatf&);

    private:
        unsigned int capacity;
        unsigned int nStars;
        float* vertices;
        float* texCoords;
        unsigned char* colors;
        Vec3f v0, v1, v2, v3;
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
                              float discSizeInPixels,
                              Color color,
                              const Quatf& orientation,
                              float renderDistance,
                              bool useHaloes);
    void labelStars(const std::vector<Star*>& stars,
                    const StarDatabase& starDB,
                    const Observer& observer);
    void labelConstellations(const AsterismList& asterisms,
                             const Observer& observer);
    void renderParticles(const std::vector<Particle>& particles,
                         Quatf orientation);
    void renderLabels();

    
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
    float saturationMag;
    float brightnessBias;

    float brightnessScale;
    float faintestMag;

    Color ambientColor;

    StarVertexBuffer* starVertexBuffer;
    std::vector<RenderListEntry> renderList;
    std::vector<Particle> glareParticles;
    std::vector<Label> labels;

    std::vector<Star*> labelledStars;

    AsterismList* asterisms;

    double modelMatrix[16];
    double projMatrix[16];

    int nSimultaneousTextures;
    bool useTexEnvCombine;
    bool useRegisterCombiners;
    bool useCubeMaps;
    bool useCompressedTextures;
    bool useVertexPrograms;
    bool useRescaleNormal;
};

#endif // _RENDER_H_
