// scattertable.cpp
//
// Copyright (C) 2010, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// A utility for generating atmospheric transmittance and scattering
// tables for use in real time 3D rendering.
//
// Using a 3D texture to store precomputed inscattering values was described
// in:
// Schafhitzel T., Falk M., Ertl T.: "Real-time rendering of planets with
// atmospheres." In WSCG International Conference in Central Europe on
// Computer Graphics, Visualization and Computer Vision (2007).
//
// The approach in Schafhitzel et al was extended in Bruneton E., Neyret F.:
// "Precomputed Atmospheric Scattering." Eurographics Symposium on
// Rendering 2008. Bruneton and Neyret made three key improvements:
// * Extended the tables to 4D in order to incorporate the angle between
//   viewer and the sun (resulting in the 'twilight wedge' phenomenon)
// * Added extra steps in the table generation to simulate the effects
//   of multiple scattering
// * Optimized the parameterizations of height, view angle, and sun angle
//   in order to reduce artifacts and use less storage. Further improvements
//   to the parameterization were made in GLSL and code available on the
//   web at http://evasion.inrialpes.fr/~Eric.Bruneton/
//
// The inscatter table produced by the scattertable utility currently only
// incorporates single scattering and is 3D, not 4D. The parameterization of
// view angle is an optimized version of the 'steep sigmoid' function
// used by Bruneton (which involved an expensive inverse trig operation
// in the shader.)

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <numbers>
#include <optional>
#include <string_view>

#include <Eigen/Core>

#include <celmath/mathlib.h>
#include <celutil/bytes.h>

using namespace std::string_view_literals;

namespace math = celestia::math;
namespace util = celestia::util;

namespace
{

constexpr unsigned int HeightSamples = 32;
constexpr unsigned int ViewAngleSamples = 256;
constexpr unsigned int SunAngleSamples = 32;

// Values settable via the command line
struct Options
{
    unsigned int ScatteringIntegrationSteps{ 25 };
    std::filesystem::path ConfigFileName;
    std::filesystem::path OutputFileName{ "out.atm" };
};

const Eigen::Vector3f RGBWavelengths(680.0f, 550.0f, 440.0f);

/** @param lambda - wavelength in nm
 *  @param n - index of refraction
 *  @param N - particles per meter^3
 */
constexpr float
RayleighScatteringCoeff(float lambda, float n, float N)
{
    float lambdaM4 = math::square(math::square(lambda * 1.0e-9f));
    return (8.0f * math::cube(std::numbers::pi_v<float>) * math::square(n * n - 1.0f)) / (3.0f * N * lambdaM4);
}

Eigen::Vector3f
RayleighScatteringCoeff(float n, float N)
{
    return Eigen::Vector3f(RayleighScatteringCoeff(RGBWavelengths.x(), n, N),
                           RayleighScatteringCoeff(RGBWavelengths.y(), n, N),
                           RayleighScatteringCoeff(RGBWavelengths.z(), n, N));
}

struct ParameterSet
{
    ParameterSet()
    {
        // Compute default rayleigh coefficients index of refraction and
        // molecular densities for Earth's atmosphere.
        Eigen::Vector3f rayleighCoeff = RayleighScatteringCoeff(1.00027712f, 2.5470e25f);
        RayleighRed   = rayleighCoeff.x() * 1000.0f;
        RayleighGreen = rayleighCoeff.y() * 1000.0f;
        RayleighBlue  = rayleighCoeff.z() * 1000.0f;
    }

    float RayleighScaleHeight{ 7.94f };
    float RayleighRed;
    float RayleighGreen;
    float RayleighBlue;
    float MieScaleHeight{ 1.2f };
    float Mie{ 2.1e-3f };
    float AbsorbRed{ 0.0f };
    float AbsorbGreen{ 0.0f };
    float AbsorbBlue{ 0.0f };
    float Radius{ 6378.0f };
};

struct Atmosphere
{
    static constexpr unsigned int TransmittanceSampleCount = HeightSamples * ViewAngleSamples;
    static constexpr unsigned int InscatterSampleCount = HeightSamples * ViewAngleSamples * SunAngleSamples;

    using TransmittanceTable = std::array<Eigen::Vector3f, TransmittanceSampleCount>;
    using InscatterTable = std::array<Eigen::Vector4f, InscatterSampleCount>;

    float shellRadius() const
    {
        return planetRadius + std::max(mieScaleHeight, rayleighScaleHeight) * 8.0f;
    }

    std::unique_ptr<TransmittanceTable> computeTransmittanceTable() const;
    std::unique_ptr<InscatterTable> computeInscatterTable(unsigned int scatteringIntegrationSteps) const;

    float planetRadius;
    float rayleighScaleHeight;
    float mieScaleHeight;
    Eigen::Vector3f rayleighCoeff;
    float mieCoeff;
    Eigen::Vector3f absorptionCoeff;
    float mieAsymmetry;
};

// Based on approximation from E. Bruneton and F. Neyret,
//     - r is distance of eye from planet center
//     - mu is the cosine of the view angle (view angle dot zenith direction)
//     - pathLength is the distance that the ray travels through the atmosphere
//     - H is the scale height
//     - R is the planet radius
float
opticalDepth(float r, float mu, float l, float H, float R)
{
    float a = std::sqrt(r * 0.5 / H);
    float bx = a * mu;
    float by = a * (mu + l / r);
    float sbx = math::sign(bx);
    float sby = math::sign(by);
    float x = sby > sbx ? std::exp(bx * bx) : 0.0f;
    float yx = sbx / (2.3193f * std::abs(bx) + std::sqrt(1.52f * bx * bx + 4.0f));
    float yy = sby / (2.3193f * std::abs(by) + std::sqrt(1.52f * by * by + 4.0f)) *
        std::exp(-l / H * (l / (2.0f * r) + mu));

    return std::sqrt(6.2831f * H * r) * std::exp((R - r) / H) * (x + yx - yy);
}

Eigen::Vector3f
transmittance(float r, float mu, float l, const Atmosphere& atm)
{
    float depthR = opticalDepth(r, mu, l, atm.rayleighScaleHeight, atm.planetRadius);
    float depthM = opticalDepth(r, mu, l, atm.mieScaleHeight, atm.planetRadius);
    return (-depthR * atm.rayleighCoeff
            - depthM * Eigen::Vector3f::Constant(atm.mieCoeff)
            - depthM * atm.absorptionCoeff).array().exp();
}

// Parameters:
//   h   - height of the viewpoint above the planet surface
//   mu  - cosine of the angle between the view direction and zenith
//   muS - cosine of the angle between the sun direction and zenith

// Map a unorm to the cosine of the view angle
constexpr float
toMu(float u)
{
    float x = u * 2.0f - 1.0f;
    float signX = x < 0.0f ? 1.0f : -1.0f;
    return (x * (0.1f - 0.15f * signX) - 0.165f) / (signX * x + 1.1f);
}

// Map a unorm to the cosine of the sun angle
inline float
toMuS(float u)
{
    // Original version from Bruneton paper:
    //return (-1.0f / 3.0f) * (log(1.0f - u * (1.0f - exp(-3.6f))) + 0.6f);

    // Modifier version has a wider range, allowing more negative angles. This
    // eliminates the faint but persistent illumination that appears even
    // when the sun is far below the horizon. Even the adjusted function may
    // still not be adequate when very large scale heights are used.
    return (-1.0f / 2.0f) * (std::log(1.0f - u * (1.0f - std::exp(-2.6f))) + 0.6f);
}

std::unique_ptr<Atmosphere::TransmittanceTable>
Atmosphere::computeTransmittanceTable() const
{
    float Rg = planetRadius;
    float Rg2 = Rg * Rg;
    float Rt = shellRadius();
    float Rt2 = Rt * Rt;

    auto transmittanceTable = std::make_unique<TransmittanceTable>();

    // Avoid numerical precision problems by choosing a first viewer
    // position just *above* the planet surface.
    float baseHeight = Rg * 1.0e-6f;

    for (unsigned int i = 0; i < HeightSamples; ++i)
    {
        float v = static_cast<float>(i) / static_cast<float>(HeightSamples);
        float h = v * v * (Rt - Rg) + baseHeight;
        float r = Rg + h;
        float r2 = r * r;

        for (unsigned int j = 0; j < ViewAngleSamples; ++j)
        {
            float u = static_cast<float>(j) / static_cast<float>(ViewAngleSamples - 1);
            float mu = std::clamp(toMu(u), -1.0f, 1.0f);
            float cosTheta = mu;
            float sinTheta2 = 1.0f - cosTheta * cosTheta;

            float pathLength;
            float d = Rg2 - r2 * sinTheta2;
            if (d > 0.0f && -r * cosTheta - std::sqrt(d) > 0.0f)
            {
                pathLength = -r * cosTheta - std::sqrt(Rg2 - r2 * sinTheta2);
            }
            else
            {
                pathLength = -r * cosTheta + std::sqrt(Rt2 - r2 * sinTheta2);
            }

            unsigned int index = i * ViewAngleSamples + j;
            (*transmittanceTable)[index] = transmittance(r, mu, pathLength, *this);

            // Warning messages
            if (std::isnan((*transmittanceTable)[index].x()))
            {
                std::cout << "NaN in transmittance table at (" << j << ", " << i << ")\n"
                          << (*transmittanceTable)[index].x() << "\n"
                             "r=" << r << ", mu=" << mu << ", l=" << pathLength << '\n';
                std::exit(EXIT_FAILURE);
            }

            if ((*transmittanceTable)[index].x() > 1.0f)
            {
                std::cout << "Non-physical transmittance " << (*transmittanceTable)[index].x() << '\n';
            }
        }
    }

    return transmittanceTable;
}

std::unique_ptr<Atmosphere::InscatterTable>
Atmosphere::computeInscatterTable(unsigned int scatteringIntegrationSteps) const
{
    // Rg - "ground radius"
    // Rt - "transparent radius", i.e. radius of the atmosphere at some point
    //      where it is visually undetectable.

    float Rg = planetRadius;
    float Rg2 = Rg * Rg;
    float Rt = shellRadius();
    float Rt2 = Rt * Rt;

    // Avoid numerical precision problems by choosing a first viewer
    // position just *above* the planet surface.
    float baseHeight = Rg * 1.0e-6f;

    auto inscatter = std::make_unique<InscatterTable>();

    for (unsigned int i = 0; i < HeightSamples; ++i)
    {
        float w = static_cast<float>(i) / static_cast<float>(HeightSamples);
        float h = w * w * (Rt - Rg) + baseHeight;
        float r = Rg + h;
        float r2 = r * r;

        std::cout << "layer " << i << ", height=" << h << "km\n";

        Eigen::Vector2f eye(0.0f, r);

        for (unsigned int j = 0; j < ViewAngleSamples; ++j)
        {
            float v = static_cast<float>(j) / static_cast<float>(ViewAngleSamples - 1);
            float mu = std::clamp(toMu(v), -1.0f, 1.0f);
            float cosTheta = mu;
            float sinTheta2 = 1.0f - cosTheta * cosTheta;
            float sinTheta = std::sqrt(sinTheta2);
            Eigen::Vector2f view(sinTheta, cosTheta);

            float pathLength;
            float d = Rg2 - r2 * sinTheta2;
            if (d > 0.0f && -r * cosTheta - std::sqrt(d) > 0.0f)
            {
                // Ray hits the planet
                pathLength = -r * cosTheta - std::sqrt(Rg2 - r2 * sinTheta2);
            }
            else
            {
                // Ray hits the sky
                pathLength = -r * cosTheta + std::sqrt(Rt2 - r2 * sinTheta2);
            }

            for (unsigned int k = 0; k < SunAngleSamples; ++k)
            {
                float w = static_cast<float>(k) / static_cast<float>(SunAngleSamples - 1);
                float muS = toMuS(w);
                float cosPhi = muS;
                float sinPhi = std::sqrt(std::max(0.0f, 1.0f - cosPhi * cosPhi));
                Eigen::Vector2f sun(sinPhi, cosPhi);

                float stepLength = pathLength / static_cast<float>(scatteringIntegrationSteps);
                Eigen::Vector2f step = view * stepLength;

                Eigen::Vector3f rayleigh = Eigen::Vector3f::Zero();
                float mie = 0.0f;

                for (unsigned int m = 0; m < scatteringIntegrationSteps; ++m)
                {
                    Eigen::Vector2f x = eye + step * m;
                    float distanceToViewer = stepLength * m;
                    float rx2 = x.squaredNorm();
                    float rx = std::sqrt(rx2);

                    // Compute the transmittance along the path to the viewer
                    Eigen::Vector3f viewPathTransmittance = transmittance(r, mu, distanceToViewer, *this);

                    // Compute the cosine and sine of the angle between the
                    // sun direction and zenith at the current sample.
                    float c = x.dot(sun) / rx;
                    float s2 = 1.0f - c * c;

                    // Compute the transmittance along the path to the sun
                    // and the total transmittance t.
                    Eigen::Vector3f t;
                    if (Rg2 - rx2 * s2 < 0.0f || -rx * c - std::sqrt(Rg2 - rx2 * s2) < 0.0f)
                    {
                        // Compute the distance through the atmosphere
                        // in the direction of the sun
                        float sunPathLength = -rx * c + std::sqrt(Rt2 - rx2 * s2);
                        Eigen::Vector3f sunPathTransmittance = transmittance(rx, c, sunPathLength, *this);

                        t = viewPathTransmittance.cwiseProduct(sunPathTransmittance);
                    }
                    else
                    {
                        // Ray to sun intersects the planet; no inscattered
                        // light at this point.
                        t = Eigen::Vector3f::Zero();
                    }

                    // Accumulate Rayleigh and Mie scattering
                    float hx = rx - Rg;
                    rayleigh += (std::exp(-hx / rayleighScaleHeight) * stepLength) * t;
                    mie += std::exp(-hx / mieScaleHeight) * stepLength * t.x();
                }

                unsigned int index = (i * ViewAngleSamples + j) * SunAngleSamples + k;
                (*inscatter)[index] << rayleigh.cwiseProduct(rayleighCoeff),
                                       mie * mieCoeff;
                if (i == HeightSamples - 1 && k == 0)
                {
                    std::cout << std::acos(muS) * 180.0/std::numbers::pi << ", "
                              << std::acos(mu) * 180.0/std::numbers::pi << ", "
                              << (*inscatter)[index].transpose() << '\n';
                }

#if 0
                // Emit warnings about NaNs in scatter table
                if (std::isnan(rayleigh.x()))
                {
                    std::cout << "NaN in inscatter table at (" << k << ", " << j << ", " << i << ")\n";
                }
#endif
            }
        }
    }

    return inscatter;
}

void
usage()
{
    std::cerr << "Usage: scattertable [options] <config file>\n"
                 "   --output <filename> (or -o) : set filename of output image\n"
                 "           (default is out.atm)\n"
                 "   --scattersteps <value> (or -s)\n"
                 "           set the number of integration steps for scattering\n";
}

/*
 * Theory:
 * Atmospheres are assumed to be composed of two different populations of
 * particles: Rayleigh scattering and Mie scattering. The density
 * of each population decreases exponentially with height above the planet
 * surface to a degree determined by a scale height:
 *
 *     density(height) = e^(-height/scaleHeight)
 *
 * Rayleigh scattering is wavelength dependent, with a fixed phase function.
 *
 * Mie scattering is wavelength independent, with a phase function determined
 * by a single parameter g (the asymmetry parameter). Mie scattering aerosols
 * may also be assigned wavelength dependent absorption coefficients.
 */

Eigen::Vector3f
computeRayleighCoeffs(const Eigen::Vector3f& wavelengths)
{
    return wavelengths.array().pow(-4.0f);
}

void
SetAtmosphereParameters(Atmosphere& atm, ParameterSet& params)
{
    atm.rayleighScaleHeight   = params.RayleighScaleHeight;
    atm.rayleighCoeff.x()     = params.RayleighRed;
    atm.rayleighCoeff.y()     = params.RayleighGreen;
    atm.rayleighCoeff.z()     = params.RayleighBlue;

    atm.mieScaleHeight        = params.MieScaleHeight;
    atm.mieCoeff              = params.Mie;

    atm.absorptionCoeff.x()   = params.AbsorbRed;
    atm.absorptionCoeff.y()   = params.AbsorbGreen;
    atm.absorptionCoeff.z()   = params.AbsorbBlue;

    atm.planetRadius          = params.Radius;
}

bool
LoadParameterSet(ParameterSet& params, const std::filesystem::path& filename)
{
    std::ifstream in(filename);

    if (!in.good())
    {
        std::cerr << "Error opening config file " << filename << '\n';
        return false;
    }

    std::string name;
    while (in.good())
    {
        name.clear();
        in >> name;

        float numValue = 0.0f;
        in >> numValue;
        if (!in.good())
            break;

        if (name == "RayleighScaleHeight"sv)
            params.RayleighScaleHeight = numValue;
        else if (name == "RayleighRed"sv)
            params.RayleighRed = numValue;
        else if (name == "RayleighGreen"sv)
            params.RayleighGreen = numValue;
        else if (name == "RayleighBlue"sv)
            params.RayleighBlue = numValue;
        else if (name == "MieScaleHeight"sv)
            params.MieScaleHeight = numValue;
        else if (name == "Mie"sv)
            params.Mie = numValue;
        else if (name == "AbsorbRed"sv)
            params.AbsorbRed = numValue;
        else if (name == "AbsorbGreen"sv)
            params.AbsorbGreen = numValue;
        else if (name == "AbsorbBlue"sv)
            params.AbsorbBlue = numValue;
        else if (name == "Radius"sv)
            params.Radius = numValue;
    }

    if (in.bad())
    {
        std::cerr << "Error in scene config file " << filename << '\n';
        return false;
    }

    return true;
}

std::optional<Options>
parseCommandLine(int argc, char* argv[])
{
    int i = 1;
    int fileCount = 0;

    auto result = std::make_optional<Options>();
    auto& options = *result;

    while (i < argc)
    {
        std::string_view arg{ argv[i] };
        if (arg.starts_with('-'))
        {
            if (arg == "-s"sv || arg == "--scattersteps"sv)
            {
                if (i == argc - 1)
                    return std::nullopt;

                if (std::sscanf(argv[i + 1], " %u", &options.ScatteringIntegrationSteps) != 1)
                    return std::nullopt;
                ++i;
            }
            else if (arg == "-o"sv || arg == "--output"sv)
            {
                if (i == argc - 1)
                    return std::nullopt;

                options.OutputFileName = std::filesystem::path(arg);
                ++i;
            }
            else
            {
                return std::nullopt;
            }
            ++i;
        }
        else
        {
            if (fileCount == 0)
            {
                // input filename first
                options.ConfigFileName = std::filesystem::path(arg);
                ++fileCount;
            }
            else
            {
                // more than one filenames on the command line is an error
                return std::nullopt;
            }
            ++i;
        }
    }

    return result;
}

// Convert a single precision floating point value to half precision
constexpr std::uint16_t
floatToHalf(float f)
{
    auto fi = std::bit_cast<std::uint32_t>(f);

    auto signBit = uint16_t((fi & UINT32_C(0x80000000)) >> 16);

    if (f > 65504.0f)
    {
        // overflow
        return 0x7c00;
    }
    if (f < -65504.0f)
    {
        // overflow
        return 0xfc00;
    }

    int exponent = static_cast<int>((fi >> 23) & UINT32_C(0xff)) - 127 + 15;
    std::uint32_t significand = fi & UINT32_C(0x007fffff);

    if (exponent < -9)
    {
        // Value is too small even to represent as a subnormal
        return signBit;
    }
    if (exponent <= 0)
    {
        // Convert to a subnormal
        return signBit + (significand >> (13 - exponent));
    }
    else if (exponent + 127 - 15 == 0xff)
    {
        // Special values: infinities and NaNs
        if (significand == 0)
        {
            // Infinity
            return signBit + 0x7c00;
        }

        // NaN - preserve bits, but make sure that we don't
        // make the significand zero, as that would indicate
        // an infinity, not a NaN
        auto nanBits = static_cast<std::uint16_t>(significand >> 13);
        if (nanBits == 0)
        {
            nanBits = 1;
        }
        return signBit + UINT16_C(0x7c00) + nanBits;

    }
    else if (exponent > 30)
    {
        // Overflow; return infinity
        return signBit + 0x7c00;
    }
    else
    {
        // Normal value
        return signBit + (exponent << 10) +
               ((significand + 0x00001000) >> 13); // round
    }
}

struct DDSPixelFormat
{
    util::LE<std::uint32_t> dwSize{sizeof(DDSPixelFormat)};
    util::LE<std::uint32_t> dwFlags{0};
    util::LE<std::uint32_t> dwFourCC{0};
    util::LE<std::uint32_t> dwRGBBitCount{0};
    util::LE<std::uint32_t> dwRBitMask{0};
    util::LE<std::uint32_t> dwGBitMask{0};
    util::LE<std::uint32_t> dwBBitMask{0};
    util::LE<std::uint32_t> dwABitMask{0};
};

// Header for Microsoft DDS file format
struct DDSHeader
{
    DDSHeader()
    {
        std::ranges::fill(dwReserved1, 0);
    }
    static constexpr std::uint32_t CAPS_COMPLEX = 0x000008;
    static constexpr std::uint32_t CAPS_MIPMAP  = 0x400000;
    static constexpr std::uint32_t CAPS_TEXTURE = 0x001000;

    static constexpr std::uint32_t DDSD_HEIGHT      = 0x2;
    static constexpr std::uint32_t DDSD_WIDTH       = 0x4;
    static constexpr std::uint32_t DDSD_PIXELFORMAT = 0x1000;
    static constexpr std::uint32_t DDSD_MIPMAPCOUNT = 0x20000;
    static constexpr std::uint32_t DDSD_DEPTH       = 0x800000;

    static constexpr std::uint32_t D3DFMT_A16B16G16R16F = 113;

    static constexpr std::uint32_t FOURCC      = 0x04;

    util::LE<std::uint32_t>                  dwSize{sizeof(DDSHeader)};
    util::LE<std::uint32_t>                  dwFlags{DDSD_PIXELFORMAT};
    util::LE<std::uint32_t>                  dwHeight{0};
    util::LE<std::uint32_t>                  dwWidth{0};
    util::LE<std::uint32_t>                  dwLinearSize{0};
    util::LE<std::uint32_t>                  dwDepth{0};
    util::LE<std::uint32_t>                  dwMipMapCount{0};
    std::array<util::NE<std::uint32_t>, 11>  dwReserved1;
    DDSPixelFormat                           ddpf;
    util::LE<std::uint32_t>                  dwCaps{0};
    util::LE<std::uint32_t>                  dwCaps2{0};
    util::LE<std::uint32_t>                  dwCaps3{0};
    util::LE<std::uint32_t>                  dwCaps4{0};
    util::LE<std::uint32_t>                  dwReserved2{0};

    void setTexture()
    {
        dwCaps = dwCaps.get() | CAPS_TEXTURE;
    }

    void setFourCC(std::uint32_t fcc)
    {
        dwFlags = dwFlags.get() | FOURCC;
        ddpf.dwFourCC = fcc;
    }

    void setMipMapLevels(std::uint32_t levels)
    {
        dwCaps = dwCaps.get() | (CAPS_COMPLEX | CAPS_MIPMAP);
        dwFlags = dwFlags.get() | DDSD_MIPMAPCOUNT;
        dwMipMapCount = levels;
    }

    void setDimensions(std::uint32_t width, std::uint32_t height)
    {
        dwFlags = dwFlags.get() | (DDSD_WIDTH | DDSD_HEIGHT);
        dwWidth = width;
        dwHeight = height;
    }

    void setVolumeDimensions(std::uint32_t width, std::uint32_t height, std::uint32_t depth)
    {
        dwCaps = dwCaps.get() | CAPS_COMPLEX;
        dwFlags = dwFlags.get() | (DDSD_WIDTH | DDSD_HEIGHT | DDSD_DEPTH);
        dwWidth = width;
        dwHeight = height;
        dwDepth = depth;
    }
};

bool
WriteDDSHeader(std::ostream& out, const DDSHeader& dds)
{
    out.write(reinterpret_cast<const char*>(&dds), sizeof(dds));
    return out.good();
}

bool
WriteInscatterTableDDS(std::ostream& out, const Atmosphere::InscatterTable& inscatterTable)
{
    DDSHeader dds;
    dds.setTexture();
    dds.setFourCC(DDSHeader::D3DFMT_A16B16G16R16F);
    dds.setVolumeDimensions(SunAngleSamples, ViewAngleSamples, HeightSamples);
    //dds.setMipMapLevels();

    if (!WriteDDSHeader(out, dds))
    {
        std::cerr << "Failed to write inscatter table DDS header.\n";
        return false;
    }

    using ArrayType = std::array<util::LE<std::uint16_t>, Atmosphere::InscatterSampleCount * 4>;
    auto outputData = std::make_unique_for_overwrite<ArrayType>();
    for (unsigned int i = 0; i < Atmosphere::InscatterSampleCount; ++i)
    {
        const auto& v = inscatterTable[i];
        (*outputData)[i * 4]     = floatToHalf(v.x());
        (*outputData)[i * 4 + 1] = floatToHalf(v.y());
        (*outputData)[i * 4 + 2] = floatToHalf(v.z());
        (*outputData)[i * 4 + 3] = floatToHalf(v.w());
    }

    out.write(reinterpret_cast<const char*>(outputData->data()), sizeof(ArrayType));
    return out.good();
}

bool
WriteTransmittanceTableDDS(std::ostream& out, const Atmosphere::TransmittanceTable& transmittanceTable)
{
    DDSHeader dds;
    dds.setTexture();
    dds.setFourCC(DDSHeader::D3DFMT_A16B16G16R16F);
    dds.setDimensions(ViewAngleSamples, HeightSamples);
    //dds.setMipMapLevels();

    if (!WriteDDSHeader(out, dds))
    {
        std::cerr << "Failed to write transmittance table DDS header.\n";
        return false;
    }

    using ArrayType = std::array<util::LE<std::uint16_t>, Atmosphere::TransmittanceSampleCount * 4>;
    auto outputData = std::make_unique_for_overwrite<ArrayType>();
    for (unsigned int i = 0; i < Atmosphere::TransmittanceSampleCount; ++i)
    {
        const Eigen::Vector3f& v = transmittanceTable[i];
        (*outputData)[i * 4]     = floatToHalf(v.x());
        (*outputData)[i * 4 + 1] = floatToHalf(v.y());
        (*outputData)[i * 4 + 2] = floatToHalf(v.z());
        (*outputData)[i * 4 + 3] = floatToHalf(0.0f);
    }

    out.write(reinterpret_cast<const char*>(outputData->data()), sizeof(ArrayType));
    return out.good();
}

} // end unnamed namespace

#if TEST_FLOAT_TO_HALF
int main(void)
{
    while (std::cin.good())
    {
        float f = 0.0f;
        std::cin >> f;
        std::cout << std::hex << floatToHalf(f) << '\n';
    }
    return 0;
}
#else
int main(int argc, char* argv[])
{
    auto options = parseCommandLine(argc, argv);
    if (!options.has_value() || options->ConfigFileName.empty())
    {
        usage();
        return EXIT_FAILURE;
    }

    ParameterSet params;
    if (!LoadParameterSet(params, options->ConfigFileName))
        return EXIT_FAILURE;

    Atmosphere atmosphere;
    SetAtmosphereParameters(atmosphere, params);

    std::cout << "Planet radius: " << atmosphere.planetRadius << "km\n"
                 "Rayleigh scale height: " << atmosphere.rayleighScaleHeight << "km\n"
                 "Rayleigh coeff: " << atmosphere.rayleighCoeff.transpose() << "m^-1\n";

    std::cout << "Mie scale height: " << atmosphere.mieScaleHeight << "km\n"
                 "Mie coeff: " << atmosphere.mieCoeff << "m^-1\n"
                 "Absorption coeff: " << atmosphere.absorptionCoeff.transpose() << "m^-1\n"
                 "Using " << options->ScatteringIntegrationSteps << " integration steps.\n";

    std::cout << "Generating transmittance table (" << ViewAngleSamples << "x"
              << HeightSamples << ")...\n";
    std::unique_ptr<Atmosphere::TransmittanceTable> transmittanceTable = atmosphere.computeTransmittanceTable();

    std::cout << "Generating inscatter table (" << SunAngleSamples << "x"
              << ViewAngleSamples << "x" << HeightSamples << ")...\n";

    std::unique_ptr<Atmosphere::InscatterTable> inscatterTable = atmosphere.computeInscatterTable(options->ScatteringIntegrationSteps);

    // Write tables as separate DDS files
    if (std::ofstream transmittanceOut("transmittance.dds", std::ios_base::out | std::ios_base::binary);
        !WriteTransmittanceTableDDS(transmittanceOut, *transmittanceTable))
    {
        return EXIT_FAILURE;
    }

    if (std::ofstream inscatterOut("inscatter.dds", std::ios_base::out | std::ios_base::binary);
        !WriteInscatterTableDDS(inscatterOut, *inscatterTable))
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
#endif
