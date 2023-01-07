// qlobular.cpp
//
// Copyright (C) 2008, Celestia Development Team
// Initial code by Dr. Fridger Schrempp <fridger.schrempp@desy.de>
//
// Simulation of globular clusters, theoretical framework by
// Ivan King, Astron. J. 67 (1962) 471; ibid. 71 (1966) 64
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <optional>
#include <utility>

#include <fmt/printf.h>

#include <celmath/ellipsoid.h>
#include <celmath/intersect.h>
#include <celmath/randutils.h>
#include <celmath/ray.h>
#include <celutil/color.h>
#include <celutil/gettext.h>
#include "globular.h"
#include "glsupport.h"
#include "pixelformat.h"
#include "render.h"
#include "texture.h"
#include "vecgl.h"

namespace vecgl = celestia::vecgl;
using celestia::render::VertexObject;

struct GlobularForm
{
    struct Blob
    {
        Eigen::Vector3f position;
        unsigned int colorIndex;
        float radius_2d;
    };

    std::vector<Blob> gblobs{ };
    Eigen::Vector3f scale{ };
};


namespace
{

constexpr const int cntrTexWidth  = 512;
constexpr const int cntrTexHeight = 512;
constexpr const int starTexWidth  = 128;
constexpr const int starTexHeight = 128;

Color colorTable[256];

constexpr const unsigned int GLOBULAR_POINTS  = 8192;

constexpr const float LumiShape = 3.0f;

// min/max c-values of globular cluster data
constexpr const float MinC = 0.50f;
constexpr const float MaxC = 2.58f;
constexpr std::size_t GlobularBuckets = 8;
constexpr const float BinWidth = (MaxC - MinC) / static_cast<float>(GlobularBuckets) + 0.02f;

// P1 determines the zoom level, where individual cluster stars start to appear.
// The smaller P2 (< 1), the faster stars show up when resolution increases.
constexpr const float P1 = 65.0f, P2 = 0.75f;

constexpr const float RRatio_min_exponent = 1.7f;

constexpr const float RADIUS_CORRECTION = 0.025f;

float CBin, RRatio, XI, Rr = 1.0f, Gg = 1.0f, Bb = 1.0f;

void globularTextureEval(float u, float v, float /*w*/, std::uint8_t *pixel)
{
    // use an exponential luminosity shape for the individual stars
    // giving sort of a halo for the brighter (i.e.bigger) stars.

    static const float Lumi0 = std::exp(-LumiShape);
    float lumi = std::exp(-LumiShape * std::sqrt(u * u + v * v)) - Lumi0;

    if (lumi <= 0.0f)
        lumi = 0.0f;

    auto pixVal = static_cast<std::uint8_t>(lumi * 255.99f);
    pixel[0] = 255;
    pixel[1] = 255;
    pixel[2] = 255;
    pixel[3] = pixVal;
}

float relStarDensity(float eta)
{
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

    static const float RRatio_min = std::pow(10.0f, RRatio_min_exponent);
    float rRatio = std::max(RRatio_min, RRatio);
    float Xi = 1.0f / std::sqrt(1.0f + rRatio * rRatio);
    float XI2 = Xi * Xi;
    float rho2 = 1.0001f + eta * eta * rRatio * rRatio; //add 1e-4 as regulator near rho=0

    return ((std::log(rho2) + 4.0f * (1.0f - std::sqrt(rho2)) * Xi) / (rho2 - 1.0f) + XI2) / (1.0f - 2.0f * Xi + XI2);
}

void centerCloudTexEval(float u, float v, float /*w*/, std::uint8_t *pixel)
{
    /*! For reasons of speed, calculate central "cloud" texture only for
     *  8 bins of King_1962 concentration, c = CBin, XI(CBin), RRatio(CBin).
     */

    // Skyplane projected King_1962 profile at center (rho = eta = 0):
    float c2d = 1.0f - XI;

    float eta = std::sqrt(u * u + v * v);  // u,v = (-1..1)

    // eta^2 = u * u  + v * v = 1 is the biggest circle fitting into the quadratic
    // procedural texture. Hence clipping

    if (eta >= 1.0f)
        eta  = 1.0f;

    // eta = 1 corresponds to tidalRadius:

    float rho   = eta  * RRatio;
    float rho2  = 1.0f + rho * rho;

    // Skyplane projected King_1962 profile (Eq.(14)), vanishes for eta = 1:
    // i.e. absolutely no globular stars for r > tidalRadius:

    float profile_2d = (1.0f / std::sqrt(rho2) - 1.0f)/c2d + 1.0f;
    profile_2d = profile_2d * profile_2d;

    pixel[0] = 255;
    pixel[1] = 255;
    pixel[2] = 255;
    pixel[3] = static_cast<std::uint8_t>(relStarDensity(eta) * profile_2d * 255.99f);
}

void initGlobularData(VertexObject& vo,
                      const std::vector<GlobularForm::Blob>& points,
                      GLint sizeLoc,
                      GLint etaLoc)
{
    struct GlobularVtx
    {
        Eigen::Vector3f position;
        Color color;
        float starSize;
        float eta;
    };
    std::vector<GlobularVtx> globularVtx;
    globularVtx.reserve(4 + points.size());

    // Reuse the buffer for a tidal
    globularVtx.push_back({{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0.0f, 0.0f});
    globularVtx.push_back({{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1.0f, 0.0f});
    globularVtx.push_back({{ 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1.0f, 1.0f});
    globularVtx.push_back({{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0.0f, 1.0f});

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
            starSize /= 1.25f;
        }

        const GlobularForm::Blob& b = points[i];
        GlobularVtx vtx;
        vtx.starSize = starSize;
        vtx.position = b.position;
        vtx.eta      = b.radius_2d;

        /* Colors of normal globular stars are given by color profile.
         * Associate orange "Red Giant" stars with the largest sprite
         * sizes (while pow2 = 128).
         */

        vtx.color = (pow2 < 256) ? colorTable[255] : colorTable[b.colorIndex];

        globularVtx.push_back(vtx);
    }

    vo.allocate(globularVtx.size() * sizeof(GlobularVtx), globularVtx.data());
    vo.setVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex,
                            3, GL_FLOAT, false, sizeof(GlobularVtx), 0);
    vo.setVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex,
                            2, GL_FLOAT, false, sizeof(GlobularVtx), offsetof(GlobularVtx, starSize)); //HACK!!! used only for tidal
    vo.setVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex,
                            4, GL_UNSIGNED_BYTE, true, sizeof(GlobularVtx), offsetof(GlobularVtx, color));
    vo.setVertexAttribArray(sizeLoc, 1, GL_FLOAT, false, sizeof(GlobularVtx), offsetof(GlobularVtx, starSize));
    vo.setVertexAttribArray(etaLoc,  1, GL_FLOAT, false, sizeof(GlobularVtx), offsetof(GlobularVtx, eta));
}

GlobularForm buildGlobularForm(float c)
{
    GlobularForm::Blob b{};
    std::vector<GlobularForm::Blob> globularPoints;

    float rRatio = std::pow(10.0f, c); //  = r_t / r_c
    float prob;
    float cc =  1.0f + rRatio * rRatio;
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
    while (i < GLOBULAR_POINTS)
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

        prob = (1.0f - 1.0f / Z) / prob0;
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

            b.colorIndex = static_cast<unsigned int>(Z * 254);

            globularPoints.push_back(b);
            i++;
        }
    }

    // Check for efficiency of sprite-star generation => close to 100 %!
    //cout << "c =  "<< c <<"  i =  " << i - 1 <<"  k =  " << k - 1 << "  Efficiency:  " << 100.0f * i / (float)k<<"%" << endl;

    GlobularForm globularForm;
    globularForm.gblobs = std::move(globularPoints);
    globularForm.scale  = Eigen::Vector3f::Ones();

    return globularForm;
}

class GlobularInfoManager
{
 public:
    GlobularInfoManager()
    {
        initializeForms();
        centerTex.fill(nullptr);
    }

    const GlobularForm* getForm(unsigned int) const;
    Texture* getCenterTex(unsigned int);
    Texture* getGlobularTex();

 private:
    void initializeForms();

    std::array<GlobularForm, GlobularBuckets> globularForms{ };
    std::array<Texture*, GlobularBuckets> centerTex{ };
    Texture* globularTex{ nullptr };
};

const GlobularForm* GlobularInfoManager::getForm(unsigned int form) const
{
    assert(form < globularForms.size());
    return &globularForms[form];
}

Texture* GlobularInfoManager::getCenterTex(unsigned int form)
{
    if(centerTex[form] == nullptr)
    {
        centerTex[form] = CreateProceduralTexture(cntrTexWidth, cntrTexHeight,
                                                  celestia::PixelFormat::RGBA,
                                                  centerCloudTexEval);
    }

    assert(centerTex[form] != nullptr);
    return centerTex[form];
}

Texture* GlobularInfoManager::getGlobularTex()
{
    if (globularTex == nullptr)
    {
        globularTex = CreateProceduralTexture(starTexWidth, starTexHeight,
                                              celestia::PixelFormat::RGBA,
                                              globularTextureEval);
    }
    assert(globularTex != nullptr);
    return globularTex;
}

void GlobularInfoManager::initializeForms()
{
    // Build RGB color table, using hue, saturation, value as input.
    // Hue in degrees.

    // Location of hue transition and saturation peak in color index space:
    int i0 = 36, i_satmax = 16;
    // Width of hue transition in color index space:
    int i_width = 3;

    float sat_l = 0.08f, sat_h = 0.1f, hue_r = 27.0f, hue_b = 220.0f;

    // Red Giant star color: i = 255:
    // -------------------------------
    // Convert hue, saturation and value to RGB

    DeepSkyObject::hsv2rgb(&Rr, &Gg, &Bb, 25.0f, 0.65f, 1.0f);
    colorTable[255] = Color(Rr, Gg, Bb);

    // normal stars: i < 255, generic color profile for now, improve later
    // --------------------------------------------------------------------
    // Convert hue, saturation, value to RGB

    for (int i = 254; i >=0; i--)
    {
        // simple qualitative saturation profile:
        // i_satmax is value of i where sat = sat_h + sat_l maximal

        float x = static_cast<float>(i) / static_cast<float>(i_satmax), x2 = x;
        float sat = sat_l + 2 * sat_h /(x2 + 1.0f / x2);

        // Fast transition from hue_r to hue_b at i = i0 within a width
        // i_width in color index space:

        float hue = hue_r + 0.5f * (hue_b - hue_r) * (std::tanh(static_cast<float>(i - i0)
                                                      / static_cast<float>(i_width)) + 1.0f);

        DeepSkyObject::hsv2rgb(&Rr, &Gg, &Bb, hue, sat, 0.85f);
        colorTable[i] = Color(Rr, Gg, Bb);
    }
    // Define globularForms corresponding to 8 different bins of King concentration c

    for (unsigned int ic  = 0; ic < GlobularBuckets; ++ic)
    {
        float CBin = MinC + (static_cast<float>(ic) + 0.5f) * BinWidth;
        globularForms[ic] = buildGlobularForm(CBin);
    }
}

GlobularInfoManager* getGlobularInfoManager()
{
    static GlobularInfoManager* globularInfoManager = new GlobularInfoManager();
    return globularInfoManager;
}

} // end unnamed namespace


Globular::Globular()
{
    recomputeTidalRadius();
}

unsigned int Globular::cSlot(float conc) const
{
    // map the physical range of c, minC <= c <= maxC,
    // to 8 integers (bin numbers), 0 < cSlot <= 7:

    if (conc <= MinC)
        conc  = MinC;
    if (conc >= MaxC)
        conc  = MaxC;

    return static_cast<unsigned int>(std::floor((conc - MinC) / BinWidth));
}

const char* Globular::getType() const
{
    return "Globular";
}

void Globular::setType(const std::string& /*typeStr*/)
{
}

float Globular::getDetail() const
{
    return detail;
}

void Globular::setDetail(float d)
{
    detail = d;
}

float Globular::getCoreRadius() const
{
    return r_c;
}

void Globular::setCoreRadius(const float coreRadius)
{
    r_c = coreRadius;
    recomputeTidalRadius();
}

float Globular::getHalfMassRadius() const
{
    // Aproximation to the half-mass radius r_h [ly]
    // (~ 20% accuracy)

    return std::tan(celmath::degToRad(r_c / 60.0f)) * static_cast<float>(getPosition().norm()) * std::pow(10.0f, 0.6f * c - 0.4f);
}

float Globular::getConcentration() const
{
    return c;
}

void Globular::setConcentration(const float conc)
{
    c = conc;
    // For saving time, account for the c dependence via 8 bins only,

    form = getGlobularInfoManager()->getForm(cSlot(conc));
    recomputeTidalRadius();
}

std::string Globular::getDescription() const
{
   return fmt::sprintf(_("Globular (core radius: %4.2f', King concentration: %4.2f)"), r_c, c);
}

const char* Globular::getObjTypeName() const
{
    return "globular";
}

bool Globular::pick(const Eigen::ParametrizedLine<double, 3>& ray,
                    double& distanceToPicker,
                    double& cosAngleToBoundCenter) const
{
    if (!isVisible())
        return false;
    /*
     * The selection ellipsoid should be slightly larger to compensate for the fact
     * that blobs are considered points when globulars are built, but have size
     * when they are drawn.
     */
    Eigen::Vector3d ellipsoidAxes(getRadius() * (form->scale.x() + RADIUS_CORRECTION),
                                  getRadius() * (form->scale.y() + RADIUS_CORRECTION),
                                  getRadius() * (form->scale.z() + RADIUS_CORRECTION));

    Eigen::Vector3d p = getPosition();
    return celmath::testIntersection(celmath::transformRay(Eigen::ParametrizedLine<double, 3>(ray.origin() - p, ray.direction()),
                                                           getOrientation().cast<double>().toRotationMatrix()),
                                     celmath::Ellipsoidd(ellipsoidAxes),
                                     distanceToPicker,
                                     cosAngleToBoundCenter);
}

bool Globular::load(const AssociativeArray* params, const fs::path& resPath)
{
    // Load the basic DSO parameters first

    bool ok = DeepSkyObject::load(params, resPath);
    if (!ok)
        return false;

    if (auto detailVal = params->getNumber<float>("Detail"); detailVal.has_value())
        setDetail(*detailVal);

    if (auto coreRadius = params->getAngle<float>("CoreRadius", 1.0 / MINUTES_PER_DEG); coreRadius.has_value())
        setCoreRadius(*coreRadius);

    if (auto king = params->getNumber<float>("KingConcentration"); king.has_value())
        setConcentration(*king);

    return true;
}

void Globular::render(const Eigen::Vector3f& offset,
                      const Eigen::Quaternionf& viewerOrientation,
                      float brightness,
                      float pixelSize,
                      const Matrices& m,
                      Renderer* renderer)
{
    if (form == nullptr)
        return;

    float distanceToDSO = offset.norm() - getRadius();
    if (distanceToDSO < 0)
        distanceToDSO = 0;

    float minimumFeatureSize = 0.5f * pixelSize * distanceToDSO;

    float DiskSizeInPixels = getRadius() / minimumFeatureSize;

    /*
     * Is the globular's apparent size big enough to
     * be noticeable on screen? If it's not, break right here to
     * avoid all the overhead of the matrix transformations and
     * GL state changes:
     */

    if (DiskSizeInPixels < 1.0f)
        return;

    auto *tidalProg = renderer->getShaderManager().getShader("tidal");
    auto *globProg  = renderer->getShaderManager().getShader("globular");
    if (tidalProg == nullptr || globProg == nullptr)
        return;

    /*
     * When resolution (zoom) varies, the blended texture opacity is controlled by the
     * factor 'pixelWeight'. At low resolution, the latter starts at 1, but tends to 0,
     * if the resolution increases sufficiently (DiskSizeInPixels >= P1 pixels)!
     * The smaller P2 (<1), the faster pixelWeight -> 0, for DiskSizeInPixels >= P1.
     */

    float pixelWeight = 1.0f;
    if (DiskSizeInPixels >= P1)
        pixelWeight = 1.0f/(P2 + (1.0f - P2) * DiskSizeInPixels / P1);

    // Use same 8 c-bins as in globularForms below!

    unsigned int ic = cSlot(c);
    CBin = MinC + (static_cast<float>(ic) + 0.5f) * BinWidth; // center value of (ic+1)th c-bin

    RRatio = std::pow(10.0f, CBin);
    XI = 1.0f / std::sqrt(1.0f + RRatio * RRatio);

    GlobularInfoManager* globularInfoManager = getGlobularInfoManager();

#ifndef GL_ES
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif

    float tidalSize = 2.0f * tidalRadius;

    /* Render central cloud sprite (centerTex). It fades away when
     * distance from center or resolution increases sufficiently.
     */

    vo.bind();
    if (!vo.initialized())
    {
        auto i = globProg->attribIndex("starSize");
        auto j = globProg->attribIndex("eta");
        initGlobularData(vo, form->gblobs, i, j);
    }

    tidalProg->use();
    globularInfoManager->getCenterTex(ic)->bind();

    tidalProg->setMVPMatrices(*m.projection, *m.modelview);

    Eigen::Matrix3f viewMat = viewerOrientation.conjugate().toRotationMatrix();
    tidalProg->vec4Param("color") = Eigen::Vector4f(Rr, Gg, Bb, std::min(2 * brightness * pixelWeight, 1.0f));
    tidalProg->floatParam("tidalSize") = tidalSize;
    tidalProg->mat3Param("viewMat") = viewMat;
    tidalProg->samplerParam("tidalTex") = 0;

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.smoothLines = true;
    renderer->setPipelineState(ps);

    vo.draw(GL_TRIANGLE_FAN, 4);

    /*! Next, render globular cluster via distinct "star" sprites (globularTex)
     * for sufficiently large resolution and distance from center of globular.
     *
     * This RGBA texture fades away when resolution decreases (e.g. via automag!),
     * or when distance from globular center decreases.
     */

    GLsizei count = static_cast<GLsizei>(form->gblobs.size() * std::clamp(getDetail(), 0.0f, 1.0f));
    float t = std::pow(2.0f, 1.0f + std::log2(minimumFeatureSize / brightness) / std::log2(1.0f/1.25f));
    count = std::min(count, static_cast<GLsizei>(std::clamp(t, 128.0f, static_cast<float>(std::max(count, 128)))));

    globProg->use();

    globularInfoManager->getGlobularTex()->bind();
    globProg->setMVPMatrices(*m.projection, *m.modelview);
    // TODO: model view matrix should not be reset here
    globProg->ModelViewMatrix = vecgl::translate(*m.modelview, offset);
    Eigen::Matrix3f mx = Eigen::Scaling(form->scale) * getOrientation().toRotationMatrix() * Eigen::Scaling(tidalSize);
    globProg->mat3Param("m")            = mx;
    globProg->vec3Param("offset")       = offset;
    globProg->floatParam("brightness")  = brightness;
    globProg->floatParam("pixelWeight") = pixelWeight;
    globProg->floatParam("RRatio")      = RRatio;
    globProg->floatParam("scale")       = renderer->getScreenDpi() / 25.4f / 3.78f;
    globProg->samplerParam("starTex")   = 0;

    vo.draw(GL_POINTS, count, 4);

    vo.unbind();
#ifndef GL_ES
    glDisable(GL_POINT_SPRITE);
    glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
    // These should be called but stars are broken then
    // TODO: find and fix
    //glDisable(GL_BLEND);
}

std::uint64_t Globular::getRenderMask() const
{
    return Renderer::ShowGlobulars;
}

unsigned int Globular::getLabelMask() const
{
    return Renderer::GlobularLabels;
}

void Globular::recomputeTidalRadius()
{
    // Convert the core radius from arcminutes to light years
    // Compute the tidal radius in light years

    float coreRadiusLy = std::tan(celmath::degToRad(r_c / 60.0f)) * static_cast<float>(getPosition().norm());
    tidalRadius = coreRadiusLy * std::pow(10.0f, c);
}
