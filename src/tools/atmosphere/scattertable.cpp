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

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <map>
#include <cassert>
#include <Eigen/Core>
#include <Eigen/Array>

using namespace Eigen;
using namespace std;


const unsigned int HeightSamples = 32;
const unsigned int ViewAngleSamples = 256;
const unsigned int SunAngleSamples = 32;

// Values settable via the command line
static unsigned int ScatteringIntegrationSteps = 25;

typedef map<string, double> ParameterSet;

static const Vector3f RGBWavelengths(680.0f, 550.0f, 440.0f);

/** @param lambda - wavelength in nm
 *  @param n - index of refraction
 *  @param N - particles per meter^3
 */
static float RayleighScatteringCoeff(float lambda, float n, float N)
{
    float lambdaM = lambda * 1.0e-9f;
    return (8.0f * pow(float(M_PI), 3.0f) * pow(n * n - 1.0f, 2.0f)) / (3.0f * N * pow(lambdaM, 4.0f));
}

static Vector3f RayleighScatteringCoeff(float n, float N)
{
    return Vector3f(RayleighScatteringCoeff(RGBWavelengths.x(), n, N),
                    RayleighScatteringCoeff(RGBWavelengths.y(), n, N),
                    RayleighScatteringCoeff(RGBWavelengths.z(), n, N));
}


class Atmosphere
{
public:
    Atmosphere()
    {
    }

    float shellRadius() const
    {
        return planetRadius + max(mieScaleHeight, rayleighScaleHeight) * 8.0f;
    }

    Vector3f* computeTransmittanceTable() const;
    Vector4f* computeInscatterTable() const;

public:
    float planetRadius;
    float rayleighScaleHeight;
    float mieScaleHeight;
    Vector3f rayleighCoeff;
    float mieCoeff;
    Vector3f absorptionCoeff;
    float mieAsymmetry;
};


// Check to see if a floating point value is a NaN. Useful for debugging.
static inline bool isNaN(float x)
{
    return x != x;
}


static inline float sign(float x)
{
    if (x < 0.0f)
        return -1.0f;
    else if (x > 0.0f)
        return 1.0f;
    else
        return 0.0f;
}


// Based on approximation from E. Bruneton and F. Neyret,
//     - r is distance of eye from planet center
//     - mu is the cosine of the view angle (view angle dot zenith direction)
//     - pathLength is the distance that the ray travels through the atmosphere
//     - H is the scale height
//     - R is the planet radius
float opticalDepth(float r, float mu, float l, float H, float R)
{
    float a = sqrt(r * 0.5 / H);
    float bx = a * mu;
    float by = a * (mu + l / r);
    float sbx = sign(bx);
    float sby = sign(by);
    float x = sby > sbx ? exp(bx * bx) : 0.0f;
    float yx = sbx / (2.3193f * abs(bx) + sqrt(1.52f * bx * bx + 4.0f));
    float yy = sby / (2.3193f * abs(by) + sqrt(1.52f * by * by + 4.0f)) *
        exp(-l / H * (l / (2.0f * r) + mu));

    return sqrt(6.2831f * H * r) * exp((R - r) / H) * (x + yx - yy);
}


Vector3f transmittance(float r, float mu, float l, const Atmosphere& atm)
{
    float depthR = opticalDepth(r, mu, l, atm.rayleighScaleHeight, atm.planetRadius);
    float depthM = opticalDepth(r, mu, l, atm.mieScaleHeight, atm.planetRadius);
    return (-depthR * atm.rayleighCoeff
            - depthM * Vector3f::Constant(atm.mieCoeff)
            - depthM * atm.absorptionCoeff).cwise().exp();
}


// Parameters:
//   h   - height of the viewpoint above the planet surface
//   mu  - cosine of the angle between the view direction and zenith
//   muS - cosine of the angle between the sun direction and zenith

// Map a unorm to the cosine of the view angle
static inline float toMu(float u)
{
    float x = u * 2.0f - 1.0f;
    float signX = x < 0.0f ? 1.0f : -1.0f;
    return (x * (0.1f - 0.15f * signX) - 0.165f) / (signX * x + 1.1f);
}

// Map a unorm to the cosine of the sun angle
static inline float toMuS(float u)
{
    // Original version from Bruneton paper:
    //return (-1.0f / 3.0f) * (log(1.0f - u * (1.0f - exp(-3.6f))) + 0.6f);

    // Modifier version has a wider range, allowing more negative angles. This
    // eliminates the faint but persistent illumination that appears even
    // when the sun is far below the horizon. Even the adjusted function may
    // still not be adequate when very large scale heights are used.
    return (-1.0f / 2.0f) * (log(1.0f - u * (1.0f - exp(-2.6f))) + 0.6f);
}


Vector3f*
Atmosphere::computeTransmittanceTable() const
{
    float Rg = planetRadius;
    float Rg2 = Rg * Rg;
    float Rt = shellRadius();
    float Rt2 = Rt * Rt;

    unsigned int sampleCount = HeightSamples * ViewAngleSamples;
    Vector3f* transmittanceTable = new Vector3f[sampleCount];

    // Avoid numerical precision problems by choosing a first viewer
    // position just *above* the planet surface.
    float baseHeight = Rg * 1.0e-6f;

    for (unsigned int i = 0; i < HeightSamples; ++i)
    {
        float v = float(i) / float(HeightSamples);
        float h = v * v * (Rt - Rg) + baseHeight;
        float r = Rg + h;
        float r2 = r * r;

        for (unsigned int j = 0; j < ViewAngleSamples; ++j)
        {
            float u = float(j) / float(ViewAngleSamples - 1);
            float mu = max(-1.0f, min(1.0f, toMu(u)));
            float cosTheta = mu;
            float sinTheta2 = 1.0f - cosTheta * cosTheta;

            float pathLength;
            float d = Rg2 - r2 * sinTheta2;
            if (d > 0.0f && -r * cosTheta - sqrt(d) > 0.0f)
            {
                pathLength = -r * cosTheta - sqrt(Rg2 - r2 * sinTheta2);
            }
            else
            {
                pathLength = -r * cosTheta + sqrt(Rt2 - r2 * sinTheta2);
            }

            unsigned int index = i * ViewAngleSamples + j;
            transmittanceTable[index] = transmittance(r, mu, pathLength, *this);

            // Warning messages
            if (isNaN(transmittanceTable[index].x()))
            {
                cout << "NaN in transmittance table at (" << j << ", " << i << ")\n";
                cout << transmittanceTable[index].x() << endl;
                cout << "r=" << r << ", mu=" << mu << ", l=" << pathLength << endl;
                exit(1);
            }

            if (transmittanceTable[index].x() > 1.0f)
            {
                cout << "Non-physical transmittance " << transmittanceTable[index].x() << endl;
            }
        }
    }

    return transmittanceTable;
}


Vector4f*
Atmosphere::computeInscatterTable() const
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

    unsigned int sampleCount = HeightSamples * ViewAngleSamples * SunAngleSamples;
    Vector4f* inscatter = new Vector4f[sampleCount];

    for (unsigned int i = 0; i < HeightSamples; ++i)
    {
        float w = float(i) / float(HeightSamples);
        float h = w * w * (Rt - Rg) + baseHeight;
        float r = Rg + h;
        float r2 = r * r;

        cout << "layer " << i << ", height=" << h << "km\n";

        Vector2f eye(0.0f, r);

        for (unsigned int j = 0; j < ViewAngleSamples; ++j)
        {
            float v = float(j) / float(ViewAngleSamples - 1);
            float mu = max(-1.0f, min(1.0f, toMu(v)));
            float cosTheta = mu;
            float sinTheta2 = 1.0f - cosTheta * cosTheta;
            float sinTheta = sqrt(sinTheta2);
            Vector2f view(sinTheta, cosTheta);

            float pathLength;
            float d = Rg2 - r2 * sinTheta2;
            if (d > 0.0f && -r * cosTheta - sqrt(d) > 0.0f)
            {
                // Ray hits the planet
                pathLength = -r * cosTheta - sqrt(Rg2 - r2 * sinTheta2);
            }
            else
            {
                // Ray hits the sky
                pathLength = -r * cosTheta + sqrt(Rt2 - r2 * sinTheta2);
            }

            for (unsigned int k = 0; k < SunAngleSamples; ++k)
            {
                float w = float(k) / float(SunAngleSamples - 1);
                float muS = toMuS(w);
                float cosPhi = muS;
                float sinPhi = sqrt(max(0.0f, 1.0f - cosPhi * cosPhi));
                Vector2f sun(sinPhi, cosPhi);

                float stepLength = pathLength / float(ScatteringIntegrationSteps);
                Vector2f step = view * stepLength;

                Vector3f rayleigh = Vector3f::Zero();
                float mie = 0.0f;

                for (unsigned int m = 0; m < ScatteringIntegrationSteps; ++m)
                {
                    Vector2f x = eye + step * m;
                    float distanceToViewer = stepLength * m;
                    float rx2 = x.squaredNorm();
                    float rx = sqrt(rx2);
                    
                    // Compute the transmittance along the path to the viewer
                    Vector3f viewPathTransmittance = transmittance(r, mu, distanceToViewer, *this);

                    // Compute the cosine and sine of the angle between the
                    // sun direction and zenith at the current sample.
                    float c = x.dot(sun) / rx;
                    float s2 = 1.0f - c * c;

                    // Compute the transmittance along the path to the sun
                    // and the total transmittance t.
                    Vector3f t;
                    if (Rg2 - rx2 * s2 < 0.0f || -rx * c - sqrt(Rg2 - rx2 * s2) < 0.0f)
                    {
                        // Compute the distance through the atmosphere
                        // in the direction of the sun
                        float sunPathLength = -rx * c + sqrt(Rt2 - rx2 * s2);
                        Vector3f sunPathTransmittance = transmittance(rx, c, sunPathLength, *this);

                        t = viewPathTransmittance.cwise() * sunPathTransmittance;
                    }
                    else
                    {
                        // Ray to sun intersects the planet; no inscattered
                        // light at this point.
                        t = Vector3f::Zero();
                    }

                    // Accumulate Rayleigh and Mie scattering
                    float hx = rx - Rg;
                    rayleigh += (exp(-hx / rayleighScaleHeight) * stepLength) * t;
                    mie += exp(-hx / mieScaleHeight) * stepLength * t.x();
                }

                unsigned int index = (i * ViewAngleSamples + j) * SunAngleSamples + k;
                inscatter[index] << rayleigh.cwise() * rayleighCoeff,
                                    mie * mieCoeff;
                if (i == HeightSamples - 1 && k == 0)
                {
                    cout << acos(muS) * 180.0/M_PI << ", "
                         << acos(mu) * 180.0/M_PI << ", "
                         << inscatter[index].transpose() << endl;
                }

#if 0
                // Emit warnings about NaNs in scatter table
                if (isNaN(rayleigh.x()))
                {
                    cout << "NaN in inscatter table at (" << k << ", " << j << ", " << i << ")\n";
                }
#endif
            }
        }
    }

    return inscatter;
}



void usage()
{
    cerr << "Usage: scattertable [options] <config file>\n";
    cerr << "   --output <filename> (or -o) : set filename of output image\n";
    cerr << "           (default is out.atm)\n";
    cerr << "   --scattersteps <value> (or -s)\n";
    cerr << "           set the number of integration steps for scattering\n";
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

Vector3f computeRayleighCoeffs(const Vector3f& wavelengths)
{
    return wavelengths.cwise().pow(-4.0f);
}


void SetAtmosphereParameters(Atmosphere& atm, ParameterSet& params)
{
    atm.rayleighScaleHeight   = params["RayleighScaleHeight"];
    atm.rayleighCoeff.x()     = params["RayleighRed"];
    atm.rayleighCoeff.y()     = params["RayleighGreen"];
    atm.rayleighCoeff.z()     = params["RayleighBlue"];

    atm.mieScaleHeight        = params["MieScaleHeight"];
    atm.mieCoeff              = params["Mie"];

    atm.absorptionCoeff.x()   = params["AbsorbRed"];
    atm.absorptionCoeff.y()   = params["AbsorbGreen"];
    atm.absorptionCoeff.z()   = params["AbsorbBlue"];

    atm.planetRadius          = params["Radius"];
}


void SetDefaultParameters(ParameterSet& params)
{
    // Compute default rayleigh coefficients index of refraction and
    // molecular densities for Earth's atmosphere.
    Vector3f rayleighCoeff = RayleighScatteringCoeff(1.00027712f, 2.5470e25f);
    float km = 1000.0f;

    params["RayleighScaleHeight"] = 7.94;
    params["RayleighRed"]         = rayleighCoeff.x() * km;
    params["RayleighGreen"]       = rayleighCoeff.y() * km;
    params["RayleighBlue"]        = rayleighCoeff.z() * km;
    
    params["MieScaleHeight"]      = 1.2;
    params["Mie"]                 = 2.1e-6f * km;

    params["AbsorbRed"]           = 0.0;
    params["AbsorbGreen"]         = 0.0;
    params["AbsorbBlue"]          = 0.0;

    params["Radius"]              = 6378.0;
}


bool LoadParameterSet(ParameterSet& params, const string& filename)
{
    ifstream in(filename.c_str());

    if (!in.good())
    {
        cerr << "Error opening config file " << filename << endl;
        return false;
    }

    while (in.good())
    {
        string name;
        in >> name;

        double numValue = 0.0;
        in >> numValue;
        if (in.good())
        {
            params[name] = numValue;
        }
    }

    if (in.bad())
    {
        cerr << "Error in scene config file " << filename << endl;
        return false;
    }
    else
    {
        return true;
    }
}


string ConfigFileName;
string OutputFileName("out.atm");

bool parseCommandLine(int argc, char* argv[])
{
    int i = 1;
    int fileCount = 0;

    while (i < argc)
    {
        if (argv[i][0] == '-')
        {
            if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--scattersteps"))
            {
                if (i == argc - 1)
                {
                    return false;
                }
                else
                {
                    if (sscanf(argv[i + 1], " %u", &ScatteringIntegrationSteps) != 1)
                        return false;
                    i++;
                }
            }
            else if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output"))
            {
                if (i == argc - 1)
                {
                    return false;
                }
                else
                {
                    OutputFileName = string(argv[i + 1]);
                    i++;
                }
            }
            else
            {
                return false;
            }
            i++;
        }
        else
        {
            if (fileCount == 0)
            {
                // input filename first
                ConfigFileName = string(argv[i]);
                fileCount++;
            }
            else
            {
                // more than one filenames on the command line is an error
                return false;
            }
            i++;
        }
    }

    return true;
}


typedef unsigned int uint32;
typedef unsigned short uint16;

union Uint16
{
    char bytes[2];
    uint16 u;
};

union Uint32
{
    char bytes[4];
    uint32 u;
};

union Float
{
    char bytes[4];
    float f;
};

union FloatInt
{
    float f;
    uint32 u;
};


// Convert a single precision floating point value to half precision
uint16 floatToHalf(float f)
{
    FloatInt fi;
    fi.f = f;

    uint16 half = 0;

    uint16 signBit = uint16((fi.u & 0x80000000) >> 16);

    if (f > 65504.0f)
    {
        // overflow
        return 0x7c00;
    }
    else if (f < -65504.0f)
    {
        // overflow
        return 0xfc00;
    }

    int exponent = int((fi.u >> 23) & 0xff) - 127 + 15;
    uint32 significand = fi.u & 0x007fffff;

    if (exponent < -9)
    {
        // Value is too small even to represent as a subnormal
        return signBit;
    }
    else if (exponent <= 0)
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
        else
        {
            // NaN - preserve bits, but make sure that we don't
            // make the significand zero, as that would indicate
            // an infinity, not a NaN
            uint16 nanBits = uint16(significand >> 13);
            if (nanBits == 0)
            {
                nanBits = 1;
            }
            return signBit + 0x7c00 + nanBits;
        }
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
    DDSPixelFormat() :
        dwSize(0),
        dwFlags(0),
        dwFourCC(0),
        dwRGBBitCount(0),
        dwRBitMask(0),
        dwGBitMask(0),
        dwBBitMask(0),
        dwABitMask(0)
    {
    }

    uint32 dwSize;
    uint32 dwFlags;
    uint32 dwFourCC;
    uint32 dwRGBBitCount;
    uint32 dwRBitMask;
    uint32 dwGBitMask;
    uint32 dwBBitMask;
    uint32 dwABitMask;
};


// Header for Microsoft DDS file format
struct DDSHeader
{
    DDSHeader() :
        dwSize(sizeof(DDSHeader)),
        dwFlags(DDSD_PIXELFORMAT),
        dwHeight(0),
        dwWidth(0),
        dwLinearSize(0),
        dwDepth(0),
        dwMipMapCount(0),
        dwCaps(0),
        dwCaps2(0),
        dwCaps3(0),
        dwCaps4(0),
        dwReserved2(0)
    {
        for (unsigned int i = 0; i < sizeof(dwReserved1) / sizeof(uint32); i++)
        {
            dwReserved1[i] = 0;
        }
    }

    static const uint32 CAPS_COMPLEX = 0x000008;
    static const uint32 CAPS_MIPMAP  = 0x400000;
    static const uint32 CAPS_TEXTURE = 0x001000;

    static const uint32 CAPS2_VOLUME = 0x200000;
    static const uint32 CAPS2_CUBEMAP = 0x00000200;
    static const uint32 CAPS2_CUBEMAP_POSITIVEX = 0x00000400;
    static const uint32 CAPS2_CUBEMAP_NEGATIVEX = 0x00000800;
    static const uint32 CAPS2_CUBEMAP_POSITIVEY = 0x00001000;
    static const uint32 CAPS2_CUBEMAP_NEGATIVEY = 0x00002000;
    static const uint32 CAPS2_CUBEMAP_POSITIVEZ = 0x00004000;
    static const uint32 CAPS2_CUBEMAP_NEGATIVEZ = 0x00008000;

    static const uint32 DDSD_CAPS      = 0x1;
    static const uint32 DDSD_HEIGHT    = 0x2;
    static const uint32 DDSD_WIDTH     = 0x4;
    static const uint32 DDSD_PITCH     = 0x8;
    static const uint32 DDSD_PIXELFORMAT = 0x1000;
    static const uint32 DDSD_MIPMAPCOUNT = 0x20000;
    static const uint32 DDSD_LINEARSIZE  = 0x80000;
    static const uint32 DDSD_DEPTH       = 0x800000;

    static const uint32 D3DFMT_A16B16G16R16 =   36;
    static const uint32 D3DFMT_A16B16G16R16F = 113;
    static const uint32 D3DFMT_DXT1 = 0x31545844;
    static const uint32 D3DFMT_DXT3 = 0x33545844;
    static const uint32 D3DFMT_DXT5 = 0x35545844;

    static const uint32 FOURCC      = 0x04;

    uint32          dwSize;
    uint32          dwFlags;
    uint32          dwHeight;
    uint32          dwWidth;
    uint32          dwLinearSize;
    uint32          dwDepth;
    uint32          dwMipMapCount;
    uint32          dwReserved1[11];
    DDSPixelFormat  ddpf;
    uint32          dwCaps;
    uint32          dwCaps2;
    uint32          dwCaps3;
    uint32          dwCaps4;
    uint32          dwReserved2;

    void setTexture()
    {
        dwCaps |= CAPS_TEXTURE;
    }

    void setFourCC(uint32 fcc)
    {
        dwFlags |= FOURCC;
        ddpf.dwFourCC = fcc;
    }

    void setMipMapLevels(uint32 levels)
    {
        dwCaps |= (CAPS_COMPLEX | CAPS_MIPMAP);
        dwFlags |= DDSD_MIPMAPCOUNT;
        dwMipMapCount = levels;
    }

    void setDimensions(uint32 width, uint32 height)
    {
        dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
        dwWidth = width;
        dwHeight = height;
    }

    void setVolumeDimensions(uint32 width, uint32 height, uint32 depth)
    {
        dwCaps |= CAPS_COMPLEX;
        dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT | DDSD_DEPTH);
        dwWidth = width;
        dwHeight = height;
        dwDepth = depth;
    }
};

static bool ByteSwapRequired = false;
static bool IsLittleEndian()
{
    Uint32 endiannessTest;
    endiannessTest.u = 0x01020304;
    return endiannessTest.bytes[0] == 0x04;
}


// Write out a 16-bit unsigned integer in little-endian order
static void WriteUint16(ostream& out, uint16 u)
{
    assert(sizeof(u) == 2);

    Uint16 ub;
    ub.u = u;
    if (ByteSwapRequired)
    {
        swap(ub.bytes[0], ub.bytes[1]);
    }

    out.write(ub.bytes, sizeof(ub.bytes));
}


// Write out a 32-bit unsigned integer in little-endian order
static void WriteUint32(ostream& out, uint32 u)
{
    assert(sizeof(u) == 4);

    Uint32 ub;
    ub.u = u;
    if (ByteSwapRequired)
    {
        swap(ub.bytes[0], ub.bytes[3]);
        swap(ub.bytes[1], ub.bytes[2]);
    }

    out.write(ub.bytes, sizeof(ub.bytes));
}


// Write out a single precision floating point number
static void WriteFloat(ostream& out, float f)
{
    assert(sizeof(f) == 4);

    Float ub;
    ub.f = f;
    if (ByteSwapRequired)
    {
        swap(ub.bytes[0], ub.bytes[3]);
        swap(ub.bytes[1], ub.bytes[2]);
    }

    out.write(ub.bytes, sizeof(ub.bytes));
}


// Convert a single precision floating point value to half precision
// and write it out.
static void WriteHalfFloat(ostream& out, float f)
{
    WriteUint16(out, floatToHalf(f));
}


void WriteDDSHeader(ostream& out, const DDSHeader& dds)
{
    WriteUint32(out, dds.dwSize);
    WriteUint32(out, dds.dwFlags);
    WriteUint32(out, dds.dwHeight);
    WriteUint32(out, dds.dwWidth);
    WriteUint32(out, dds.dwLinearSize);
    WriteUint32(out, dds.dwDepth);
    WriteUint32(out, dds.dwMipMapCount);
    for (unsigned int i = 0; i < sizeof(dds.dwReserved1) / sizeof(uint32); i++)
    {
        WriteUint32(out, dds.dwReserved1[i]);
    }

    WriteUint32(out, dds.ddpf.dwSize);
    WriteUint32(out, dds.ddpf.dwFlags);
    WriteUint32(out, dds.ddpf.dwFourCC);
    WriteUint32(out, dds.ddpf.dwRGBBitCount);
    WriteUint32(out, dds.ddpf.dwRBitMask);
    WriteUint32(out, dds.ddpf.dwGBitMask);
    WriteUint32(out, dds.ddpf.dwBBitMask);
    WriteUint32(out, dds.ddpf.dwABitMask);

    WriteUint32(out, dds.dwCaps);
    WriteUint32(out, dds.dwCaps2);
    WriteUint32(out, dds.dwCaps3);
    WriteUint32(out, dds.dwCaps4);
    WriteUint32(out, dds.dwReserved2);
}


static void WriteInscatterTableDDS(ostream& out, Vector4f* inscatterTable)
{
    DDSHeader dds;
    dds.setTexture();
    dds.setFourCC(DDSHeader::D3DFMT_A16B16G16R16F);
    dds.setVolumeDimensions(SunAngleSamples, ViewAngleSamples, HeightSamples);
    //dds.setMipMapLevels();

    WriteDDSHeader(out, dds);

    unsigned int sampleCount = SunAngleSamples * ViewAngleSamples * HeightSamples;
    for (unsigned int i = 0; i < sampleCount; ++i)
    {
        const Vector4f& v = inscatterTable[i];
        WriteHalfFloat(out, v.x());
        WriteHalfFloat(out, v.y());
        WriteHalfFloat(out, v.z());
        WriteHalfFloat(out, v.w());
    }
}


static void WriteTransmittanceTableDDS(ostream& out, Vector3f* transmittanceTable)
{
    DDSHeader dds;
    dds.setTexture();
    dds.setFourCC(DDSHeader::D3DFMT_A16B16G16R16F);
    dds.setDimensions(ViewAngleSamples, HeightSamples);
    //dds.setMipMapLevels();

    WriteDDSHeader(out, dds);
    
    unsigned int sampleCount = ViewAngleSamples * HeightSamples;
    for (unsigned int i = 0; i < sampleCount; ++i)
    {
        const Vector3f& v = transmittanceTable[i];
        WriteHalfFloat(out, v.x());
        WriteHalfFloat(out, v.y());
        WriteHalfFloat(out, v.z());
        WriteHalfFloat(out, 0.0f);
    }
}



#if TEST_FLOAT_TO_HALF
int main(void)
{
    while (cin.good())
    {
        float f = 0.0f;
        cin >> f;
        cout << hex << floatToHalf(f) << endl;
    }
    return 0;
}
#endif

int main(int argc, char* argv[])
{
    bool commandLineOK = parseCommandLine(argc, argv);
    if (!commandLineOK || ConfigFileName.empty())
    {
        usage();
        exit(1);
    }

    ParameterSet params;
    SetDefaultParameters(params);
    if (!LoadParameterSet(params, ConfigFileName))
    {
        exit(1);
    }

    Atmosphere atmosphere;
    SetAtmosphereParameters(atmosphere, params);

    cout << "Planet radius: " << atmosphere.planetRadius << "km\n";
    cout << "Rayleigh scale height: " << atmosphere.rayleighScaleHeight << "km\n";
    cout << "Rayleigh coeff: " << atmosphere.rayleighCoeff.transpose() << "m^-1\n";

    cout << "Mie scale height: " << atmosphere.mieScaleHeight << "km\n";
    cout << "Mie coeff: " << atmosphere.mieCoeff << "m^-1\n";
    cout << "Absorption coeff: " << atmosphere.absorptionCoeff.transpose() << "m^-1\n";
    cout << "Using " << ScatteringIntegrationSteps << " integration steps.\n";

    cout << "Generating transmittance table (" << ViewAngleSamples << "x"
         << HeightSamples << ")...\n";
    Vector3f* transmittanceTable = atmosphere.computeTransmittanceTable();

    cout << "Generating inscatter table (" << SunAngleSamples << "x"
         << ViewAngleSamples << "x" << HeightSamples << ")...\n";
        
    Vector4f* inscatterTable = atmosphere.computeInscatterTable();

    ByteSwapRequired = !IsLittleEndian();
    
#if 0
    // Write tables in a single file
    ofstream out(OutputFileName.c_str(), ostream::binary);

    // Header
    out.write("atmscatr", 8);

    // Version
    WriteUint32(out, 1u);

    // Scattering parameters
    WriteFloat(out, atmosphere.rayleighScaleHeight);
    WriteFloat(out, atmosphere.rayleighCoeff.x());
    WriteFloat(out, atmosphere.rayleighCoeff.y());
    WriteFloat(out, atmosphere.rayleighCoeff.z());
    WriteFloat(out, atmosphere.mieScaleHeight);
    WriteFloat(out, atmosphere.mieCoeff);
    WriteFloat(out, atmosphere.mieAsymmetry);
    WriteFloat(out, atmosphere.absorptionCoeff.x());
    WriteFloat(out, atmosphere.absorptionCoeff.y());
    WriteFloat(out, atmosphere.absorptionCoeff.z());
    WriteFloat(out, atmosphere.planetRadius);

    // Transmittance table dimensions
    WriteUint32(out, ViewAngleSamples);
    WriteUint32(out, HeightSamples);

    // Inscatter table dimensions
    WriteUint32(out, SunAngleSamples);
    WriteUint32(out, ViewAngleSamples);
    WriteUint32(out, HeightSamples);

    unsigned int transmittanceTableSamples = ViewAngleSamples * HeightSamples;
    unsigned int inscatterTableSamples = SunAngleSamples * ViewAngleSamples *
        HeightSamples;

    for (unsigned int i = 0; i < transmittanceTableSamples; ++i)
    {
        Vector3f v = transmittanceTable[i];
        WriteFloat(out, v.x());
        WriteFloat(out, v.y());
        WriteFloat(out, v.z());
    }

    for (unsigned int i = 0; i < inscatterTableSamples; ++i)
    {
        Vector4f v = inscatterTable[i];
        WriteFloat(out, v.x());
        WriteFloat(out, v.y());
        WriteFloat(out, v.z());
        WriteFloat(out, v.w());
    }

    out.close();
#else
    // Write tables as separate DDS files
    ofstream transmittanceOut("transmittance.dds", ostream::binary);
    WriteTransmittanceTableDDS(transmittanceOut, transmittanceTable);
    transmittanceOut.close();

    ofstream inscatterOut("inscatter.dds", ostream::binary);
    WriteInscatterTableDDS(inscatterOut, inscatterTable);
    inscatterOut.close();
#endif

    return 0;
}
