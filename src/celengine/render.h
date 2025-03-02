// render.h
//
// Copyright (C) 2001-2008, Celestia Development Team
// Contact: Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <Eigen/Core>

#include <celengine/body.h>
#include <celengine/lightenv.h>
#include <celengine/multitexture.h>
#include <celengine/universe.h>
#include <celengine/selection.h>
#include <celengine/shadermanager.h>
#include <celengine/starcolors.h>
#include <celengine/projectionmode.h>
#include <celengine/rendcontext.h>
#include <celengine/renderlistentry.h>
#include <celengine/textlayout.h>
#include <celimage/pixelformat.h>
#include <celrender/rendererfwd.h>
#include "renderflags.h"

class RendererWatcher;
class FrameTree;
class ReferenceMark;
class CurvePlot;
class PointStarVertexBuffer;
class Observer;
class Surface;
class TextureFont;
class FramebufferObject;

namespace celestia
{
class Rect;

namespace gl
{
class Buffer;
class VertexObject;
}

namespace math
{
class Frustum;
class InfiniteFrustum;
}
}

struct Matrices
{
    const Eigen::Matrix4f *projection;
    const Eigen::Matrix4f *modelview;
};


struct LightSource
{
    Eigen::Vector3d position;
    Color color;
    float luminosity;
    float radius;
};


struct SecondaryIlluminator
{
    const Body*     body;
    Eigen::Vector3d position_v;       // viewer relative position
    float           radius;           // radius in km
    float           reflectedIrradiance;  // albedo times total irradiance from direct sources
};


enum class RenderMode
{
    Fill = 0,
    Line = 1
};

class Renderer
{
 public:
    Renderer();
    ~Renderer();

    struct PipelineState
    {
        bool blending       { false };
        bool scissor        { false };
        bool multisample    { false };
        bool depthMask      { false };
        bool depthTest      { false };
        bool smoothLines    { false };

        struct
        {
            GLenum src      { GL_NONE };
            GLenum dst      { GL_NONE };
        } blendFunc;
    };

    struct DetailOptions
    {
        DetailOptions();
        unsigned int orbitPathSamplePoints{ 100 };
        unsigned int shadowTextureSize{ 256 };
        unsigned int eclipseTextureSize{ 128 };
        double orbitWindowEnd{ 0.5 };
        double orbitPeriodsShown{ 1.0 };
        double linearFadeFraction{ 0.0 };
#ifndef GL_ES
        bool useMesaPackInvert{ true };
#endif
    };

    bool init(int, int, const DetailOptions&);
    void shutdown() {};
    void resize(int, int);
    float getAspectRatio() const;

    void setFaintestAM45deg(float);
    float getFaintestAM45deg() const;
    void setRTL(bool);
    bool isRTL() const;

    void setRenderMode(RenderMode);
    void autoMag(float& faintestMag, float zoom);
    void render(const Observer&,
                const Universe&,
                float faintestVisible,
                const Selection& sel);

    bool getInfo(std::map<std::string, std::string>& info) const;

    RenderFlags getRenderFlags() const;
    void setRenderFlags(RenderFlags);
    RenderLabels getLabelMode() const;
    void setLabelMode(RenderLabels);
    std::shared_ptr<celestia::engine::ProjectionMode> getProjectionMode() const;
    void setProjectionMode(std::shared_ptr<celestia::engine::ProjectionMode>);
    float getAmbientLightLevel() const;
    void setAmbientLightLevel(float);
    float getTintSaturation() const;
    void setTintSaturation(float);
    float getMinimumOrbitSize() const;
    void setMinimumOrbitSize(float);
    float getMinimumFeatureSize() const;
    void setMinimumFeatureSize(float);
    float getDistanceLimit() const;
    void setDistanceLimit(float);
    BodyClassification getOrbitMask() const;
    void setOrbitMask(BodyClassification);
    int getScreenDpi() const;
    void setScreenDpi(int);
    int getWindowWidth() const;
    int getWindowHeight() const;

    float getScaleFactor() const;
    float getPointWidth() const;
    float getPointHeight() const;

    // GL wrappers
    void getViewport(int* x, int* y, int* w, int* h) const;
    void getViewport(std::array<int, 4>& viewport) const;
    void setViewport(int x, int y, int w, int h);
    void setViewport(const std::array<int, 4>& viewport);
    void setScissor(int x, int y, int w, int h);
    void removeScissor();

    void enableMSAA() noexcept;
    void disableMSAA() noexcept;
    bool isMSAAEnabled() const noexcept;

    void setPipelineState(const PipelineState &ps) noexcept;

    celestia::engine::PixelFormat getPreferredCaptureFormat() const noexcept;

    void drawRectangle(const celestia::Rect& r,
                       FisheyeOverrideMode fishEyeOverrideMode,
                       const Eigen::Matrix4f& p,
                       const Eigen::Matrix4f& m = Eigen::Matrix4f::Identity()) const;
    void setRenderRegion(int x, int y, int width, int height, bool withScissor = true);

    ColorTableType getStarColorTable() const;
    void setStarColorTable(ColorTableType);
    [[deprecated]] bool getVideoSync() const;
    [[deprecated]] void setVideoSync(bool);
    void setSolarSystemMaxDistance(float);
    void setShadowMapSize(unsigned);

    bool captureFrame(int, int, int, int, celestia::engine::PixelFormat format, unsigned char*) const;

    void renderMarker(celestia::MarkerRepresentation::Symbol symbol,
                      float size,
                      const Color &color,
                      const Matrices &m);

    celestia::util::array_view<const Star*> getNearStars() const
    {
        return nearStars;
    }

    const Eigen::Matrix4f& getModelViewMatrix() const
    {
        return m_modelMatrix;
    }

    const Eigen::Matrix4f& getProjectionMatrix() const
    {
        return m_projMatrix;
    }

    const Eigen::Matrix4f& getOrthoProjectionMatrix() const
    {
        return m_orthoProjMatrix;
    }

    const Eigen::Matrix4f& getCurrentModelViewMatrix() const
    {
        return *m_modelViewPtr;
    }

    void setCurrentModelViewMatrix(const Eigen::Matrix4f& m)
    {
        m_modelViewPtr = &m;
    }

    void setDefaultModelViewMatrix()
    {
        m_modelViewPtr = &m_modelMatrix;
    }

    const Eigen::Matrix4f& getCurrentProjectionMatrix() const
    {
        return *m_projectionPtr;
    }

    void setCurrentProjectionMatrix(const Eigen::Matrix4f& m)
    {
        m_projectionPtr = &m;
    }

    void setDefaultProjectionMatrix()
    {
        m_projectionPtr = &m_projMatrix;
    }

    void buildProjectionMatrix(Eigen::Matrix4f &mat, float nearZ, float farZ, float zoom) const;

    void setStarStyle(StarStyle);
    StarStyle getStarStyle() const;
    void setResolution(TextureResolution resolution);
    TextureResolution getResolution() const;
    void enableSelectionPointer();
    void disableSelectionPointer();

    void loadTextures(Body*);

    // Label related methods
    enum class LabelHorizontalAlignment : std::uint8_t
    {
        Center,
        Start,
        End,
    };

    enum class LabelVerticalAlignment : std::uint8_t
    {
        Center,
        Bottom,
        Top,
    };

    struct Annotation
    {
        std::string labelText;
        const celestia::MarkerRepresentation* markerRep;
        Color color;
        Eigen::Vector3f position;
        LabelHorizontalAlignment halign : 3;
        LabelVerticalAlignment valign : 3;
        float size;

        bool operator<(const Annotation&) const;
    };

    void addForegroundAnnotation(const celestia::MarkerRepresentation* markerRep,
                                 std::string_view labelText,
                                 Color color,
                                 const Eigen::Vector3f& position,
                                 LabelHorizontalAlignment halign = LabelHorizontalAlignment::Start,
                                 LabelVerticalAlignment valign = LabelVerticalAlignment::Bottom,
                                 float size = 0.0f);
    void addBackgroundAnnotation(const celestia::MarkerRepresentation* markerRep,
                                 std::string_view labelText,
                                 Color color,
                                 const Eigen::Vector3f& position,
                                 LabelHorizontalAlignment halign = LabelHorizontalAlignment::Start,
                                 LabelVerticalAlignment valign = LabelVerticalAlignment::Bottom,
                                 float size = 0.0f);
    void addSortedAnnotation(const celestia::MarkerRepresentation* markerRep,
                             std::string_view labelText,
                             Color color,
                             const Eigen::Vector3f& position,
                             LabelHorizontalAlignment halign = LabelHorizontalAlignment::Start,
                             LabelVerticalAlignment valign = LabelVerticalAlignment::Bottom,
                             float size = 0.0f);

    ShaderManager& getShaderManager() const { return *shaderManager; }

    // Callbacks for renderables; these belong in a special renderer interface
    // only visible in object's render methods.
    void beginObjectAnnotations();
    void addObjectAnnotation(const celestia::MarkerRepresentation* markerRep,
                             std::string_view labelText,
                             Color,
                             const Eigen::Vector3f&,
                             LabelHorizontalAlignment halign,
                             LabelVerticalAlignment valign);
    void endObjectAnnotations();
    Eigen::Quaternionf getCameraOrientationf() const;
    Eigen::Quaterniond getCameraOrientation() const;
    Eigen::Matrix3d getCameraTransform() const;
    void setCameraTransform(const Eigen::Matrix3d&);

    float getNearPlaneDistance() const;

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

    void setFont(FontStyle, const std::shared_ptr<TextureFont>&);
    std::shared_ptr<TextureFont> getFont(FontStyle) const;

    bool settingsHaveChanged() const;
    void markSettingsChanged();

    void addWatcher(RendererWatcher*);
    void removeWatcher(RendererWatcher*);
    void notifyWatchers() const;

    FramebufferObject* getShadowFBO(int) const;

 public:
    struct RenderProperties
    {
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

    struct DepthBufferPartition
    {
        int index;
        float nearZ;
        float farZ;
    };

 private:
    void setFieldOfView(float);
    void renderPointStars(const StarDatabase& starDB,
                          float faintestVisible,
                          const Observer& observer);
    void renderDeepSkyObjects(const Universe&,
                              const Observer&,
                              float faintestMagNight);
    void renderSkyGrids(const Observer& observer);
    void renderSelectionPointer(const Observer& observer,
                                double now,
                                const celestia::math::InfiniteFrustum& viewFrustum,
                                const Selection& sel);

    void renderAsterisms(const Universe&, float, const Matrices&);
    void renderBoundaries(const Universe&, float, const Matrices&);
    void renderCrosshair(float size, double tsec, const Color &color, const Matrices &m);

    void buildNearSystemsLists(const Universe &universe,
                               const Observer &observer,
                               const celestia::math::InfiniteFrustum &xfrustum,
                               double jd);

    void buildRenderLists(const Eigen::Vector3d& astrocentricObserverPos,
                          const celestia::math::InfiniteFrustum& viewFrustum,
                          const Eigen::Vector3d& viewPlaneNormal,
                          const Eigen::Vector3d& frameCenter,
                          const FrameTree* tree,
                          const Observer& observer,
                          double now);
    void buildOrbitLists(const Eigen::Vector3d& astrocentricObserverPos,
                         const Eigen::Quaterniond& observerOrientation,
                         const celestia::math::InfiniteFrustum& viewFrustum,
                         const FrameTree* tree,
                         double now);
    void buildLabelLists(const celestia::math::InfiniteFrustum& viewFrustum,
                         double now);
    int buildDepthPartitions();


    void addRenderListEntries(RenderListEntry& rle,
                              Body& body,
                              bool isLabeled);

    void addStarOrbitToRenderList(const Star& star,
                                  const Observer& observer,
                                  double now);

    void removeInvisibleItems(const celestia::math::InfiniteFrustum &frustum);

    void renderObject(const Eigen::Vector3f& pos,
                      float distance,
                      const Observer& observer,
                      float nearPlaneDistance,
                      float farPlaneDistance,
                      RenderProperties& obj,
                      const LightingState&,
                      const Matrices&);

    void renderPlanet(Body& body,
                      const Eigen::Vector3f& pos,
                      float distance,
                      float appMag,
                      const Observer& observer,
                      float, float,
                      const Matrices&);

    void renderStar(const Star& star,
                    const Eigen::Vector3f& pos,
                    float distance,
                    float appMag,
                    const Observer& observer,
                    float, float,
                    const Matrices&);

    void renderReferenceMark(const ReferenceMark& refMark,
                             const Eigen::Vector3f& pos,
                             float distance,
                             double now,
                             float nearPlaneDistance,
                             const Matrices&);

    void renderCometTail(const Body& body,
                         const Eigen::Vector3f& pos,
                         const Observer& observer,
                         float dustTailLength,
                         float discSizeInPixels,
                         const Matrices&);

    void calculatePointSize(float appMag,
                            float size,
                            float &discSize,
                            float &alpha,
                            float &glareSize,
                            float &glareAlpha) const;

    void renderObjectAsPoint(const Eigen::Vector3f& center,
                             float radius,
                             float appMag,
                             float discSizeInPixels,
                             const Color& color,
                             bool useHalos,
                             bool emissive,
                             const Matrices&);

    void locationsToAnnotations(const Body& body,
                                const Eigen::Vector3d& bodyPosition,
                                const Eigen::Quaterniond& bodyOrientation);

    // Render an item from the render list
    void renderItem(const RenderListEntry& rle,
                    const Observer& observer,
                    float nearPlaneDistance,
                    float farPlaneDistance,
                    const Matrices&);

    bool testEclipse(const Body& receiver,
                     const Body& caster,
                     LightingState& lightingState,
                     unsigned int lightIndex,
                     double now);

    void labelConstellations(const AsterismList& asterisms,
                             const Observer& observer);

    void getLabelAlignmentInfo(const Annotation &annotation, const TextureFont *font, celestia::engine::TextLayout::HorizontalAlignment &halign, float &hOffset, float &vOffset) const;

    void addAnnotation(std::vector<Annotation>&,
                       const celestia::MarkerRepresentation*,
                       std::string_view labelText,
                       Color color,
                       const Eigen::Vector3f& position,
                       LabelHorizontalAlignment halign = LabelHorizontalAlignment::Start,
                       LabelVerticalAlignment = LabelVerticalAlignment::Bottom,
                       float size = 0.0f,
                       bool special = false);
    void renderAnnotationMarker(const Annotation &a,
                                celestia::engine::TextLayout &layout,
                                float depth,
                                const Matrices&);
    void renderAnnotationLabel(const Annotation &a,
                               celestia::engine::TextLayout &layout,
                               float hOffset,
                               float vOffset,
                               float depth,
                               const Matrices&);
    void renderAnnotations(const std::vector<Annotation>&,
                           FontStyle fs);
    void renderBackgroundAnnotations(FontStyle fs);
    void renderForegroundAnnotations(FontStyle fs);
    std::vector<Annotation>::iterator renderSortedAnnotations(std::vector<Annotation>::iterator,
                                                              float nearDist,
                                                              float farDist,
                                                              FontStyle fs);
    std::vector<Annotation>::iterator renderAnnotations(std::vector<Annotation>::iterator startIter,
                                                        std::vector<Annotation>::iterator endIter,
                                                        float nearDist,
                                                        float farDist,
                                                        FontStyle fs);

    void markersToAnnotations(const celestia::MarkerList &markers,
                              const Observer &observer,
                              double now);

    bool selectionToAnnotation(const Selection &sel,
                               const Observer &observer,
                               const celestia::math::InfiniteFrustum &xfrustum,
                               double now);

    void adjustMagnitudeInsideAtmosphere(float &faintestMag,
                                         float &saturationMag,
                                         double now);

    void renderOrbit(const OrbitPathListEntry&,
                     double now,
                     const Eigen::Quaterniond& cameraOrientation,
                     const celestia::math::Frustum& frustum,
                     float nearDist,
                     float farDist);

    void renderSolarSystemObjects(const Observer &observer,
                                  int nIntervals,
                                  double now);

    void updateBodyVisibilityMask();

    void createShadowFBO();

 private:
    ShaderManager* shaderManager{ nullptr };

    int windowWidth;
    int windowHeight;
    float fov;
    double cosViewConeAngle{ 0.0 };
    int screenDpi;
    float corrFac;
    float pixelSize{ 1.0f };
    float faintestAutoMag45deg;
    std::vector<std::shared_ptr<TextureFont>> fonts{FontCount, nullptr};

    std::shared_ptr<celestia::engine::ProjectionMode> projectionMode{ nullptr };
    int renderMode;
    RenderLabels labelMode{ RenderLabels::NoLabels };
    bool rtl{ false };
    bool showSelectionPointer{ true };
    RenderFlags renderFlags{ RenderFlags::DefaultRenderFlags };
    BodyClassification bodyVisibilityMask{ ~BodyClassification::EmptyMask };
    BodyClassification orbitMask{ BodyClassification::Planet | BodyClassification::Moon | BodyClassification::Stellar };
    float ambientLightLevel{ 0.1f };
    float tintSaturation{ 0.5f };
    float brightnessBias;

    float brightnessScale{ 1.0f };
    float faintestMag{ 0.0f };
    float faintestPlanetMag{ 0.0f };
    float saturationMagNight;
    float saturationMag;
    StarStyle starStyle{ StarStyle::FuzzyPointStars };

    Color ambientColor;
    std::string displayedSurface;

    Eigen::Quaterniond m_cameraOrientation;
    Eigen::Matrix3d m_cameraTransform{ Eigen::Matrix3d::Identity() };
    PointStarVertexBuffer* pointStarVertexBuffer;
    PointStarVertexBuffer* glareVertexBuffer;
    std::vector<RenderListEntry> renderList;
    std::vector<SecondaryIlluminator> secondaryIlluminators;
    std::vector<DepthBufferPartition> depthPartitions;
    std::vector<Annotation> backgroundAnnotations;
    std::vector<Annotation> foregroundAnnotations;
    std::vector<Annotation> depthSortedAnnotations;
    std::vector<Annotation> objectAnnotations;
    std::vector<OrbitPathListEntry> orbitPathList;
    LightingState::EclipseShadowVector eclipseShadows[MaxLights];
    std::vector<const Star*> nearStars;

    std::vector<LightSource> lightSourceList;

    Eigen::Matrix4f m_modelMatrix;
    Eigen::Matrix4f m_projMatrix;
    Eigen::Matrix4f m_MVPMatrix;
    Eigen::Matrix4f m_orthoProjMatrix;
    const Eigen::Matrix4f *m_modelViewPtr  { &m_modelMatrix };
    const Eigen::Matrix4f *m_projectionPtr { &m_projMatrix };

    bool useCompressedTextures{ false };
    TextureResolution textureResolution{ TextureResolution::medres };
    DetailOptions detailOptions;

    uint32_t frameCount;

    int currentIntervalIndex{ 0 };

    PipelineState m_pipelineState;

    std::array<int, 4> m_viewport { 0, 0, 0, 0 };

    typedef std::map<const celestia::ephem::Orbit*, CurvePlot*> OrbitCache;
    OrbitCache orbitCache;
    uint32_t lastOrbitCacheFlush;

    float minOrbitSize;
    float distanceLimit;
    float minFeatureSize;
    uint64_t locationFilter;

    ColorTemperatureTable starColors{ ColorTableType::Blackbody_D65 };
    ColorTemperatureTable tintColors{ ColorTableType::SunWhite };

    Selection highlightObject;

    bool settingsChanged;

    // True if we're in between a begin/endObjectAnnotations
    bool objectAnnotationSetOpen;

    double realTime{ true };

    // Maximum size of a solar system in light years. Features beyond this distance
    // will not necessarily be rendered correctly. This limit is used for
    // visibility culling of solar systems.
    float SolarSystemMaxDistance{ 1.0f };

    // Size of a texture used in shadow mapping
    unsigned m_shadowMapSize { 0 };
    std::unique_ptr<FramebufferObject> m_shadowFBO;

    std::unique_ptr<celestia::gl::VertexObject> m_markerVO;
    std::unique_ptr<celestia::gl::Buffer> m_markerBO;
    bool m_markerDataInitialized{ false };

    // Saturation magnitude used to calculate a point star size
    float satPoint;

    std::unique_ptr<celestia::render::AsterismRenderer> m_asterismRenderer;
    std::unique_ptr<celestia::render::BoundariesRenderer> m_boundariesRenderer;
    std::unique_ptr<celestia::render::AtmosphereRenderer> m_atmosphereRenderer;
    std::unique_ptr<celestia::render::CometRenderer> m_cometRenderer;
    std::unique_ptr<celestia::render::EclipticLineRenderer> m_eclipticLineRenderer;
    std::unique_ptr<celestia::render::GalaxyRenderer> m_galaxyRenderer;
    std::unique_ptr<celestia::render::GlobularRenderer> m_globularRenderer;
    std::unique_ptr<celestia::render::LargeStarRenderer> m_largeStarRenderer;
    std::unique_ptr<celestia::render::LineRenderer> m_hollowMarkerRenderer;
    std::unique_ptr<celestia::render::NebulaRenderer> m_nebulaRenderer;
    std::unique_ptr<celestia::render::OpenClusterRenderer> m_openClusterRenderer;
    std::unique_ptr<celestia::render::RingRenderer> m_ringRenderer;
    std::unique_ptr<celestia::render::SkyGridRenderer> m_skyGridRenderer;

    // Location markers
 public:
    celestia::MarkerRepresentation mountainRep;
    celestia::MarkerRepresentation craterRep;
    celestia::MarkerRepresentation observatoryRep;
    celestia::MarkerRepresentation cityRep;
    celestia::MarkerRepresentation genericLocationRep;
    celestia::MarkerRepresentation galaxyRep;
    celestia::MarkerRepresentation nebulaRep;
    celestia::MarkerRepresentation openClusterRep;
    celestia::MarkerRepresentation globularRep;

    std::list<RendererWatcher*> watchers;

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

    friend class PointStarRenderer;
};


class RendererWatcher
{
 public:
    RendererWatcher() {};
    virtual ~RendererWatcher() {};

    virtual void notifyRenderSettingsChanged(const Renderer*) = 0;
};
