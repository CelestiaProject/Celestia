// render.h
//
// Copyright (C) 2001-2008, Celestia Development Team
// Contact: Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_RENDER_H_
#define _CELENGINE_RENDER_H_

#include <Eigen/Core>
#include <celmath/frustum.h>
#include <celengine/universe.h>
#include <celengine/observer.h>
#include <celengine/selection.h>
#ifdef USE_GLCONTEXT
#include <celengine/glcontext.h>
#endif
#include <celengine/processoctree.h>
#include <celengine/starcolors.h>
#include <celengine/rendcontext.h>
#include <celtxf/texturefont.h>
#include <vector>
#include <list>
#include <string>
#include "vertexobject.h"


class RendererWatcher;
class FrameTree;
class ReferenceMark;
class CurvePlot;
class AsterismList;

struct LightSource
{
    Eigen::Vector3d position;
    Color color;
    float luminosity;
    float radius;
};


struct RenderListEntry
{
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    enum RenderableType
    {
        RenderableStar,
        RenderableBody,
        RenderableCometTail,
        RenderableReferenceMark,
    };

    union
    {
        const Star* star;
        Body* body;
        const ReferenceMark* refMark;
    };

    Eigen::Vector3f position;
    Eigen::Vector3f sun;
    float distance;
    float radius;
    float centerZ;
    float nearZ;
    float farZ;
    float discSizeInPixels;
    float appMag;
    RenderableType renderableType;
    bool isOpaque;
};


struct SecondaryIlluminator
{
    const Body*     body;
    Eigen::Vector3d position_v;       // viewer relative position
    float           radius;           // radius in km
    float           reflectedIrradiance;  // albedo times total irradiance from direct sources
};


class PointStarVertexBuffer;

class Renderer
{
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    Renderer();
    ~Renderer();

    struct DetailOptions
    {
        DetailOptions();
        unsigned int orbitPathSamplePoints;
        unsigned int shadowTextureSize;
        unsigned int eclipseTextureSize;
        double orbitWindowEnd;
        double orbitPeriodsShown;
        double linearFadeFraction;
    };

#ifdef USE_GLCONTEXT
    bool init(GLContext*, int, int, DetailOptions&);
#else
    bool init(int, int, DetailOptions&);
#endif
    void shutdown() {};
    void resize(int, int);

    float calcPixelSize(float fovY, float windowHeight);
    void setFaintestAM45deg(float);
    float getFaintestAM45deg() const;

    void setRenderMode(int);
    void autoMag(float& faintestMag);
    void render(const Observer&,
                const Universe&,
                float faintestVisible,
                const Selection& sel);
    void draw(const Observer&,
              const Universe&,
              float faintestVisible,
              const Selection& sel);

    enum {
        NoLabels            = 0x000,
        StarLabels          = 0x001,
        PlanetLabels        = 0x002,
        MoonLabels          = 0x004,
        ConstellationLabels = 0x008,
        GalaxyLabels        = 0x010,
        AsteroidLabels      = 0x020,
        SpacecraftLabels    = 0x040,
        LocationLabels      = 0x080,
        CometLabels         = 0x100,
        NebulaLabels        = 0x200,
        OpenClusterLabels   = 0x400,
        I18nConstellationLabels = 0x800,
        DwarfPlanetLabels   = 0x1000,
        MinorMoonLabels     = 0x2000,
        GlobularLabels      = 0x4000,
        BodyLabelMask       = (PlanetLabels | DwarfPlanetLabels | MoonLabels | MinorMoonLabels | AsteroidLabels | SpacecraftLabels | CometLabels),
    };

    enum RenderFlags : uint64_t
    {
        ShowNothing             = 0x0000000000000000,
        ShowStars               = 0x0000000000000001,
        ShowPlanets             = 0x0000000000000002,
        ShowGalaxies            = 0x0000000000000004,
        ShowDiagrams            = 0x0000000000000008,
        ShowCloudMaps           = 0x0000000000000010,
        ShowOrbits              = 0x0000000000000020,
        ShowCelestialSphere     = 0x0000000000000040,
        ShowNightMaps           = 0x0000000000000080,
        ShowAtmospheres         = 0x0000000000000100,
        ShowSmoothLines         = 0x0000000000000200,
        ShowEclipseShadows      = 0x0000000000000400,
        ShowPlanetRings         = 0x0000000000000800,
        ShowRingShadows         = 0x0000000000001000,
        ShowBoundaries          = 0x0000000000002000,
        ShowAutoMag             = 0x0000000000004000,
        ShowCometTails          = 0x0000000000008000,
        ShowMarkers             = 0x0000000000010000,
        ShowPartialTrajectories = 0x0000000000020000,
        ShowNebulae             = 0x0000000000040000,
        ShowOpenClusters        = 0x0000000000080000,
        ShowGlobulars           = 0x0000000000100000,
        ShowCloudShadows        = 0x0000000000200000,
        ShowGalacticGrid        = 0x0000000000400000,
        ShowEclipticGrid        = 0x0000000000800000,
        ShowHorizonGrid         = 0x0000000001000000,
        ShowEcliptic            = 0x0000000002000000,
        ShowTintedIllumination  = 0x0000000004000000,
        ShowDwarfPlanets        = 0x0000000008000000,
        ShowMoons               = 0x0000000010000000,
        ShowMinorMoons          = 0x0000000020000000,
        ShowAsteroids           = 0x0000000040000000,
        ShowComets              = 0x0000000080000000,
        ShowSpacecrafts         = 0x0000000100000000,
        ShowFadingOrbits        = 0x0000000200000000,
    };

    enum StarStyle
    {
        FuzzyPointStars  = 0,
        PointStars       = 1,
        ScaledDiscStars  = 2,
        StarStyleCount   = 3,
    };

    // Pixel formats for image and video capture.
    // Currently we map them 1:1 to GL
    enum class PixelFormat
    {
        RGB = GL_RGB,
        BGR_EXT = GL_BGR_EXT
    };

    // constants
    constexpr static const uint64_t DefaultRenderFlags =
                                          Renderer::ShowStars          |
                                          Renderer::ShowPlanets        |
                                          Renderer::ShowDwarfPlanets   |
                                          Renderer::ShowMoons          |
                                          Renderer::ShowMinorMoons     |
                                          Renderer::ShowAsteroids      |
                                          Renderer::ShowComets         |
                                          Renderer::ShowSpacecrafts    |
                                          Renderer::ShowGalaxies       |
                                          Renderer::ShowGlobulars      |
                                          Renderer::ShowCloudMaps      |
                                          Renderer::ShowNightMaps      |
                                          Renderer::ShowAtmospheres    |
                                          Renderer::ShowEclipseShadows |
                                          Renderer::ShowPlanetRings    |
                                          Renderer::ShowRingShadows    |
                                          Renderer::ShowCloudShadows   |
                                          Renderer::ShowCometTails     |
                                          Renderer::ShowNebulae        |
                                          Renderer::ShowOpenClusters   |
                                          Renderer::ShowAutoMag        |
                                          Renderer::ShowFadingOrbits   |
                                          Renderer::ShowSmoothLines;

    uint64_t getRenderFlags() const;
    void setRenderFlags(uint64_t);
    int getLabelMode() const;
    void setLabelMode(int);
    float getAmbientLightLevel() const;
    void setAmbientLightLevel(float);
    float getMinimumOrbitSize() const;
    void setMinimumOrbitSize(float);
    float getMinimumFeatureSize() const;
    void setMinimumFeatureSize(float);
    float getDistanceLimit() const;
    void setDistanceLimit(float);
    int getOrbitMask() const;
    void setOrbitMask(int);
    int getScreenDpi() const;
    void setScreenDpi(int);
    void getScreenSize(int* x, int* y, int* w, int* h) const;
    void getScreenSize(std::array<int, 4>& viewport) const;
    const ColorTemperatureTable* getStarColorTable() const;
    void setStarColorTable(const ColorTemperatureTable*);
    bool getVideoSync() const;
    void setVideoSync(bool);
    void setSolarSystemMaxDistance(float);

    bool captureFrame(int, int, int, int, PixelFormat format, unsigned char*, bool = false) const;

    void renderMarker(MarkerRepresentation::Symbol symbol, float size, const Color& color);

#ifdef USE_HDR
    bool getBloomEnabled();
    void setBloomEnabled(bool);
    void increaseBrightness();
    void decreaseBrightness();
    float getBrightness();
#endif

#ifdef USE_GLCONTEXT
    GLContext* getGLContext() { return context; }
#endif

    void setStarStyle(StarStyle);
    StarStyle getStarStyle() const;
    void setResolution(unsigned int resolution);
    unsigned int getResolution() const;

    void loadTextures(Body*);

    // Label related methods
    enum LabelAlignment
    {
        AlignCenter,
        AlignLeft,
        AlignRight
    };

    enum LabelVerticalAlignment
    {
        VerticalAlignCenter,
        VerticalAlignBottom,
        VerticalAlignTop,
    };

    struct Annotation
    {
        std::string labelText;
        const MarkerRepresentation* markerRep;
        Color color;
        Eigen::Vector3f position;
        LabelAlignment halign : 3;
        LabelVerticalAlignment valign : 3;
        float size;

        bool operator<(const Annotation&) const;
    };

    void addForegroundAnnotation(const MarkerRepresentation* markerRep,
                                 const std::string& labelText,
                                 Color color,
                                 const Eigen::Vector3f& position,
                                 LabelAlignment halign = AlignLeft,
                                 LabelVerticalAlignment valign = VerticalAlignBottom,
                                 float size = 0.0f);
    void addBackgroundAnnotation(const MarkerRepresentation* markerRep,
                                 const std::string& labelText,
                                 Color color,
                                 const Eigen::Vector3f& position,
                                 LabelAlignment halign = AlignLeft,
                                 LabelVerticalAlignment valign = VerticalAlignBottom,
                                 float size = 0.0f);
    void addSortedAnnotation(const MarkerRepresentation* markerRep,
                             const std::string& labelText,
                             Color color,
                             const Eigen::Vector3f& position,
                             LabelAlignment halign = AlignLeft,
                             LabelVerticalAlignment valign = VerticalAlignBottom,
                             float size = 0.0f);

   ShaderManager& getShaderManager() const { return *shaderManager; }

    // Callbacks for renderables; these belong in a special renderer interface
    // only visible in object's render methods.
    void beginObjectAnnotations();
    void addObjectAnnotation(const MarkerRepresentation* markerRep, const std::string& labelText, Color, const Eigen::Vector3f&);
    void endObjectAnnotations();
    Eigen::Quaternionf getCameraOrientation() const;
    float getNearPlaneDistance() const;

    void clearAnnotations(std::vector<Annotation>&);
    void clearSortedAnnotations();

    void invalidateOrbitCache();

    struct OrbitPathListEntry
    {
        float centerZ;
        float radius;
        Body* body;
        const Star* star;
        Eigen::Vector3d origin;
        float opacity;

        bool operator<(const OrbitPathListEntry&) const;
    };

    enum FontStyle
    {
        FontNormal = 0,
        FontLarge  = 1,
        FontCount  = 2,
    };

    void setFont(FontStyle, TextureFont*);
    TextureFont* getFont(FontStyle) const;

    bool settingsHaveChanged() const;
    void markSettingsChanged();

    void addWatcher(RendererWatcher*);
    void removeWatcher(RendererWatcher*);
    void notifyWatchers() const;

 public:
    // Internal types
    // TODO: Figure out how to make these private.  Even with a friend
    //
    struct Particle
    {
        Eigen::Vector3f center;
        float size;
        Color color;
        float pad0, pad1, pad2;
    };

    struct RenderProperties
    {
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW

        Surface* surface{ nullptr };
        const Atmosphere* atmosphere{ nullptr };
        RingSystem* rings{ nullptr };
        float radius{ 1.0f };
        float geometryScale{ 1.0f };
        Eigen::Vector3f semiAxes{ Eigen::Vector3f::Ones() };
        ResourceHandle geometry{ InvalidResource };
        Eigen::Quaternionf orientation{ Eigen::Quaternionf::Identity() };
        LightingState::EclipseShadowVector* eclipseShadows;
    };

#ifdef OCTREE_DEBUG
    OctreeProcStats m_starProcStats;
    OctreeProcStats m_dsoProcStats;
    Selection m_selected;
#endif

    float m_avgDsoMag;

 private:
    struct SkyVertex
    {
        float x, y, z;
        unsigned char color[4];
    };

    struct SkyContourPoint
    {
        Eigen::Vector3f v;
        Eigen::Vector3f eyeDir;
        float centerDist;
        float eyeDist;
        float cosSkyCapAltitude;
    };

    template <class OBJ> struct ObjectLabel
    {
        OBJ*        obj{ nullptr };
        std::string label;

        ObjectLabel(OBJ* _obj, const std::string& _label) :
            obj  (_obj),
            label(_label)
        {};

        ObjectLabel(const ObjectLabel& objLbl) :
            obj  (objLbl.obj),
            label(objLbl.label)
        {};

        ObjectLabel& operator = (const ObjectLabel& objLbl)
        {
            obj   = objLbl.obj;
            label = objLbl.label;
            return *this;
        };
    };

    typedef ObjectLabel<Star>          StarLabel;
    typedef ObjectLabel<DeepSkyObject> DSOLabel;    // currently not used

    struct DepthBufferPartition
    {
        int index;
        float nearZ;
        float farZ;
    };

 private:
    void setFieldOfView(float);
    void renderStars(const AstroDatabase& aDB,
                     float faintestVisible,
                     const Observer& observer);
    void renderPointStars(const AstroDatabase& aDB,
                          float faintestVisible,
                          const Observer& observer);
    void renderDeepSkyObjects(const Universe&,
                              const Observer&,
                              float faintestMagNight);
    void renderSkyGrids(const Observer& observer);
    void renderSelectionPointer(const Observer& observer,
                                double now,
                                const celmath::Frustum& viewFrustum,
                                const Selection& sel);

    void renderAsterisms(const Universe&, float);
    void renderBoundaries(const Universe&, float);

    void buildRenderLists(const Eigen::Vector3d& astrocentricObserverPos,
                          const celmath::Frustum& viewFrustum,
                          const Eigen::Vector3d& viewPlaneNormal,
                          const Eigen::Vector3d& frameCenter,
                          const FrameTree* tree,
                          const Observer& observer,
                          double now);
    void buildOrbitLists(const Eigen::Vector3d& astrocentricObserverPos,
                         const Eigen::Quaterniond& observerOrientation,
                         const celmath::Frustum& viewFrustum,
                         const FrameTree* tree,
                         double now);
    void buildLabelLists(const celmath::Frustum& viewFrustum,
                         double now);

    void addRenderListEntries(RenderListEntry& rle,
                              Body& body,
                              bool isLabeled);

    void addStarOrbitToRenderList(const Star& star,
                                  const Observer& observer,
                                  double now);

    void renderObject(const Eigen::Vector3f& pos,
                      float distance,
                      double now,
                      const Eigen::Quaternionf& cameraOrientation,
                      float nearPlaneDistance,
                      float farPlaneDistance,
                      RenderProperties& obj,
                      const LightingState&);

    void renderPlanet(Body& body,
                      const Eigen::Vector3f& pos,
                      float distance,
                      float appMag,
                      const Observer& observer,
                      const Eigen::Quaternionf& cameraOrientation,
                      float, float);

    void renderStar(const Star& star,
                    const Eigen::Vector3f& pos,
                    float distance,
                    float appMag,
                    const Eigen::Quaternionf& orientation,
                    double now,
                    float, float);

    void renderReferenceMark(const ReferenceMark& refMark,
                             const Eigen::Vector3f& pos,
                             float distance,
                             double now,
                             float nearPlaneDistance);

    void renderCometTail(const Body& body,
                         const Eigen::Vector3f& pos,
                         double now,
                         float discSizeInPixels);

    void renderObjectAsPoint_nosprite(const Eigen::Vector3f& center,
                                      float radius,
                                      float appMag,
                                      float _faintestMag,
                                      float discSizeInPixels,
                                      Color color,
                                      const Eigen::Quaternionf& cameraOrientation,
                                      bool useHalos);
    void renderObjectAsPoint(const Eigen::Vector3f& center,
                             float radius,
                             float appMag,
                             float _faintestMag,
                             float discSizeInPixels,
                             Color color,
                             const Eigen::Quaternionf& cameraOrientation,
                             bool useHalos,
                             bool emissive);

    void renderEllipsoidAtmosphere(const Atmosphere& atmosphere,
                                   const Eigen::Vector3f& center,
                                   const Eigen::Quaternionf& orientation,
                                   const Eigen::Vector3f& semiAxes,
                                   const Eigen::Vector3f& sunDirection,
                                   const LightingState& ls,
                                   float fade,
                                   bool lit);

    void renderLocations(const Body& body,
                         const Eigen::Vector3d& bodyPosition,
                         const Eigen::Quaterniond& bodyOrientation);

    // Render an item from the render list
    void renderItem(const RenderListEntry& rle,
                    const Observer& observer,
                    const Eigen::Quaternionf& cameraOrientation,
                    float nearPlaneDistance,
                    float farPlaneDistance);

    bool testEclipse(const Body& receiver,
                     const Body& caster,
                     LightingState& lightingState,
                     unsigned int lightIndex,
                     double now);

    void labelConstellations(const AsterismList& asterisms,
                             const Observer& observer);
    void renderParticles(const std::vector<Particle>& particles,
                         const Eigen::Quaternionf& orientation);


    void addAnnotation(std::vector<Annotation>&,
                       const MarkerRepresentation*,
                       const std::string& labelText,
                       Color color,
                       const Eigen::Vector3f& position,
                       LabelAlignment halign = AlignLeft,
                       LabelVerticalAlignment = VerticalAlignBottom,
                       float size = 0.0f,
                       bool special = false);
    void renderAnnotations(const std::vector<Annotation>&, FontStyle fs);
    void renderBackgroundAnnotations(FontStyle fs);
    void renderForegroundAnnotations(FontStyle fs);
    std::vector<Annotation>::iterator renderSortedAnnotations(std::vector<Annotation>::iterator,
                                                              float nearDist,
                                                              float farDist,
                                                              FontStyle fs);
    std::vector<Renderer::Annotation>::iterator renderAnnotations(std::vector<Annotation>::iterator startIter,
                                                                  std::vector<Annotation>::iterator endIter,
                                                                  float nearDist,
                                                                  float farDist,
                                                                  FontStyle fs);

    void renderMarkers(const MarkerList&,
                       const UniversalCoord& cameraPosition,
                       const Eigen::Quaterniond& cameraOrientation,
                       double jd);

    void renderOrbit(const OrbitPathListEntry&,
                     double now,
                     const Eigen::Quaterniond& cameraOrientation,
                     const celmath::Frustum& frustum,
                     float nearDist,
                     float farDist);

    void updateBodyVisibilityMask();

#ifdef USE_HDR
 private:
    int sceneTexWidth, sceneTexHeight;
    GLfloat sceneTexWScale, sceneTexHScale;
    GLsizei blurBaseWidth, blurBaseHeight;
    GLuint sceneTexture;
    Texture **blurTextures;
    Texture *blurTempTexture;
    GLuint gaussianLists[4];
    GLint blurFormat;
    bool useLuminanceAlpha;
    bool bloomEnabled;
    float maxBodyMag;
    float exposure, exposurePrev;
    float brightPlus;

    void genBlurTexture(int blurLevel);
    void genBlurTextures();
    void genSceneTexture();
    void renderToBlurTexture(int blurLevel);
    void renderToTexture(const Observer& observer,
                         const Universe& universe,
                         float faintestMagNight,
                         const Selection& sel);
    void drawSceneTexture();
    void drawBlur();
    void drawGaussian3x3(float xdelta, float ydelta, GLsizei width, GLsizei height, float blend);
    void drawGaussian5x5(float xdelta, float ydelta, GLsizei width, GLsizei height, float blend);
    void drawGaussian9x9(float xdelta, float ydelta, GLsizei width, GLsizei height, float blend);
    void drawBlendedVertices(float xdelta, float ydelta, float blend);
#endif

 private:
#ifdef USE_GLCONTEXT
    GLContext* context;
#endif
    ShaderManager* shaderManager{ nullptr };

    int windowWidth;
    int windowHeight;
    float fov;
    double cosViewConeAngle;
    int screenDpi;
    float corrFac;
    float pixelSize{ 1.0f };
    float faintestAutoMag45deg;
    TextureFont* font[FontCount];

    int renderMode;
    int labelMode;
    uint64_t renderFlags;
    int bodyVisibilityMask{ ~0 };
    int orbitMask;
    float ambientLightLevel;
    float brightnessBias;

    float brightnessScale{ 1.0f };
    float faintestMag{ 0.0f };
    float faintestPlanetMag{ 0.0f };
    float saturationMagNight;
    float saturationMag;
    StarStyle starStyle;

    Color ambientColor;
    std::string displayedSurface;

    Eigen::Quaternionf m_cameraOrientation;
    PointStarVertexBuffer* pointStarVertexBuffer;
    PointStarVertexBuffer* glareVertexBuffer;
    std::vector<RenderListEntry> renderList;
    std::vector<SecondaryIlluminator> secondaryIlluminators;
    std::vector<DepthBufferPartition> depthPartitions;
    std::vector<Particle> glareParticles;
    std::vector<Annotation> backgroundAnnotations;
    std::vector<Annotation> foregroundAnnotations;
    std::vector<Annotation> depthSortedAnnotations;
    std::vector<Annotation> objectAnnotations;
    std::vector<OrbitPathListEntry> orbitPathList;
    LightingState::EclipseShadowVector eclipseShadows[MaxLights];
    std::vector<const Star*> nearStars;

    std::vector<LightSource> lightSourceList;

    double modelMatrix[16];
    double projMatrix[16];

    bool useCompressedTextures{ false };
    unsigned int textureResolution;
    DetailOptions detailOptions;

    uint32_t frameCount;

    int currentIntervalIndex{ 0 };

    celgl::VertexObject markerVO{ GL_ARRAY_BUFFER, 0, GL_STATIC_DRAW };

 public:
#if 0
    struct OrbitSample
    {
        double t;
        Point3d pos;

        OrbitSample(const Eigen::Vector3d& _pos, double _t) : t(_t), pos(_pos.x(), _pos.y(), _pos.z()) { }
        OrbitSample() { }
    };

    struct OrbitSection
    {
        Capsuled boundingVolume;
        uint32_t firstSample;
    };

    struct CachedOrbit
    {
        std::vector<OrbitSample> trajectory;
        std::vector<OrbitSection> sections;
        uint32_t lastUsed;
    };
#endif

 private:
    typedef std::map<const Orbit*, CurvePlot*> OrbitCache;
    OrbitCache orbitCache;
    uint32_t lastOrbitCacheFlush;

    float minOrbitSize;
    float distanceLimit;
    float minFeatureSize;
    uint64_t locationFilter;

    SkyVertex* skyVertices;
    uint32_t* skyIndices;
    SkyContourPoint* skyContour;

    const ColorTemperatureTable* colorTemp;

    Selection highlightObject;

    bool videoSync;
    bool settingsChanged;

    // True if we're in between a begin/endObjectAnnotations
    bool objectAnnotationSetOpen;

    double realTime{ true };

   // Maximum size of a solar system in light years. Features beyond this distance
   // will not necessarily be rendered correctly. This limit is used for
   // visibility culling of solar systems.
   float SolarSystemMaxDistance{ 1.0f };


    // Location markers
 public:
    MarkerRepresentation mountainRep;
    MarkerRepresentation craterRep;
    MarkerRepresentation observatoryRep;
    MarkerRepresentation cityRep;
    MarkerRepresentation genericLocationRep;
    MarkerRepresentation galaxyRep;
    MarkerRepresentation nebulaRep;
    MarkerRepresentation openClusterRep;
    MarkerRepresentation globularRep;

    std::list<RendererWatcher*> watchers;

 public:
    // Colors for all lines and labels
    static Color StarLabelColor;
    static Color PlanetLabelColor;
    static Color DwarfPlanetLabelColor;
    static Color MoonLabelColor;
    static Color MinorMoonLabelColor;
    static Color AsteroidLabelColor;
    static Color CometLabelColor;
    static Color SpacecraftLabelColor;
    static Color LocationLabelColor;
    static Color GalaxyLabelColor;
    static Color GlobularLabelColor;
    static Color NebulaLabelColor;
    static Color OpenClusterLabelColor;
    static Color ConstellationLabelColor;
    static Color EquatorialGridLabelColor;
    static Color PlanetographicGridLabelColor;
    static Color GalacticGridLabelColor;
    static Color EclipticGridLabelColor;
    static Color HorizonGridLabelColor;

    static Color StarOrbitColor;
    static Color PlanetOrbitColor;
    static Color DwarfPlanetOrbitColor;
    static Color MoonOrbitColor;
    static Color MinorMoonOrbitColor;
    static Color AsteroidOrbitColor;
    static Color CometOrbitColor;
    static Color SpacecraftOrbitColor;
    static Color SelectionOrbitColor;

    static Color ConstellationColor;
    static Color BoundaryColor;
    static Color EquatorialGridColor;
    static Color PlanetographicGridColor;
    static Color PlanetEquatorColor;
    static Color GalacticGridColor;
    static Color EclipticGridColor;
    static Color HorizonGridColor;
    static Color EclipticColor;

    static Color SelectionCursorColor;
};


class RendererWatcher
{
 public:
    RendererWatcher() {};
    virtual ~RendererWatcher() {};

    virtual void notifyRenderSettingsChanged(const Renderer*) = 0;
};


#endif // _CELENGINE_RENDER_H_
