// star.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celmath/mathlib.h>
#include <celengine/selection.h>
#include <cassert>
#include <fmt/printf.h>
#include <config.h>
#include "astro.h"
#include "star.h"
#include "texmanager.h"
#include "celephem/orbit.h"
#include "meshmanager.h"
#include "astrodb.h"

using namespace Eigen;
using namespace std;
using namespace celmath;


// The value of the temperature of the sun is actually 5780, but the
// stellar class tables list the temperature of a G2V star as 5860.  We
// use the latter value so that the radius of the sun is computed correctly
// as one times SOLAR_RADIUS . . .  the high metallicity of the Sun is
// probably what accounts for the discrepancy in temperature.
// #define SOLAR_TEMPERATURE    5780.0f
#define SOLAR_TEMPERATURE    5780.0f
#define SOLAR_BOLOMETRIC_MAG 4.75f

// moved the following to astro.h
// #define SOLAR_RADIUS         696000


struct SpectralTypeInfo
{
    char* name;
    float temperature;
    float rotationPeriod;
};


static StarDetails** normalStarDetails = nullptr;
static StarDetails** whiteDwarfDetails = nullptr;
static StarDetails*  neutronStarDetails = nullptr;
static StarDetails*  blackHoleDetails = nullptr;
static StarDetails*  barycenterDetails = nullptr;

StarDetails::StarTextureSet StarDetails::starTextures;

// Star temperature data from Lang's _Astrophysical Data: Planets and Stars_
// Temperatures from missing (and typically not used) types in those
// tables were just interpolated.
static float tempO[3][10] =
{
    { 52500, 52500, 52500, 52500, 48000, 44500, 41000, 38000, 35800, 33000 },
    { 50000, 50000, 50000, 50000, 45500, 42500, 39500, 37000, 34700, 32000 },
    { 47300, 47300, 47300, 47300, 44100, 42500, 39500, 37000, 34700, 32000 },
};

static float tempB[3][10] =
{
    { 30000, 25400, 22000, 18700, 17000, 15400, 14000, 13000, 11900, 10500 },
    { 29000, 24000, 20300, 17100, 16000, 15000, 14100, 13200, 12400, 11000 },
    { 26000, 20800, 18500, 16200, 15100, 13600, 13000, 12200, 11200, 10300 },
};

static float tempA[3][10] =
{
    {  9520, 9230, 8970, 8720, 8460, 8200, 8020, 7850, 7580, 7390 },
    { 10100, 9480, 9000, 8600, 8300, 8100, 7850, 7650, 7450, 7250 },
    {  9730, 9230, 9080, 8770, 8610, 8510, 8310, 8150, 7950, 7800 },
};

static float tempF[3][10] =
{
    { 7200, 7050, 6890, 6740, 6590, 6440, 6360, 6280, 6200, 6110 },
    { 7150, 7000, 6870, 6720, 6570, 6470, 6350, 6250, 6150, 6080 },
    { 7700, 7500, 7350, 7150, 7000, 6900, 6500, 6300, 6100, 5800 },
};

static float tempG[3][10] =
{
    { 6030, 5940, 5860, 5830, 5800, 5770, 5700, 5630, 5570, 5410 },
    { 5850, 5650, 5450, 5350, 5250, 5150, 5050, 5070, 4900, 4820 },
    { 5550, 5350, 5200, 5050, 4950, 4850, 4750, 4660, 4600, 4500 },
};

static float tempK[3][10] =
{
    { 5250, 5080, 4900, 4730, 4590, 4350, 4200, 4060, 3990, 3920 },
    { 4750, 4600, 4420, 4200, 4000, 3950, 3900, 3850, 3830, 3810 },
    { 4420, 4330, 4250, 4080, 3950, 3850, 3760, 3700, 3680, 3660 },
};

static float tempM[3][10] =
{
    { 3850, 3720, 3580, 3470, 3370, 3240, 3050, 2940, 2640, 2000 },
    { 3800, 3720, 3620, 3530, 3430, 3330, 3240, 3240, 3240, 3240 },
    { 3650, 3550, 3450, 3200, 2980, 2800, 2600, 2600, 2600, 2600 },
};

// Wolf-Rayet temperatures.  From Lang's Astrophysical Data: Planets and
// Stars.
static float tempWN[10] =
{
    50000, 50000, 50000, 50000, 47000, 43000, 39000, 32000, 29000, 29000
};

static float tempWC[10] =
{
    60000, 60000, 60000, 60000, 60000, 60000, 60000, 54000, 46000, 38000
};

// Brown dwarf temperatures
static float tempL[10] =
{
    1960, 1930, 1900, 1850, 1800, 1740, 1680, 1620, 1560, 1500
};

static float tempT[10] =
{
    1425, 1350, 1275, 1200, 1140, 1080, 1020, 900, 750, 500
};

// For Y type we just extrapolate the L & T data
static float tempY[10] =
{
    400, 350, 300, 250, 200, 150, 100, 50, 3, 3
};

// White dwarf temperaturs
static float tempWD[10] =
{
    100000.0f, 50400.0f, 25200.0f, 16800.0f, 12600.0f,
    10080.0f, 8400.0f, 7200.0f, 6300.0f, 5600.0f,
};


// Tables with adjustments for estimating absolute bolometric magnitude from
// visual magnitude, from Lang's "Astrophysical Data: Planets and Stars".
// Gaps in the tables from unused spectral classes were filled in with linear
// interpolation--not accurate, but these shouldn't appear in real catalog
// data anyway.
static float bmag_correctionO[3][10] =
{
    // Lum class V (main sequence)
    {
        -4.75f, -4.75f, -4.75f, -4.75f, -4.45f,
        -4.40f, -3.93f, -3.68f, -3.54f, -3.33f,
    },
    // Lum class III
    {
        -4.58f, -4.58f, -4.58f, -4.58f, -4.28f,
        -4.05f, -3.80f, -3.58f, -3.39f, -3.13f,
    },
    // Lum class I
    {
        -4.41f, -4.41f, -4.41f, -4.41f, -4.17f,
        -3.87f, -3.74f, -3.48f, -3.35f, -3.18f,
    }
};

static float bmag_correctionB[3][10] =
{
    // Lum class V (main sequence)
    {
        -3.16f, -2.70f, -2.35f, -1.94f, -1.70f,
        -1.46f, -1.21f, -1.02f, -0.80f, -0.51f,
    },
    // Lum class III
    {
        -2.88f, -2.43f, -2.02f, -1.60f, -1.45f,
        -1.30f, -1.13f, -0.97f, -0.82f, -0.71f,
    },
    // Lum class I
    {
        -2.49f, -1.87f, -1.58f, -1.26f, -1.11f,
        -0.95f, -0.88f, -0.78f, -0.66f, -0.52f,
    }
};

static float bmag_correctionA[3][10] =
{
    // Lum class V (main sequence)
    {
        -0.30f, -0.23f, -0.20f, -0.17f, -0.16f,
        -0.15f, -0.13f, -0.12f, -0.10f, -0.09f,
    },
    // Lum class III
    {
        -0.42f, -0.29f, -0.20f, -0.17f, -0.15f,
        -0.14f, -0.12f, -0.10f, -0.10f, -0.10f,
    },
    // Lum class I
    {
        -0.41f, -0.32f, -0.28f, -0.21f, -0.17f,
        -0.13f, -0.09f, -0.06f, -0.03f, -0.02f,
    }
};

static float bmag_correctionF[3][10] =
{
    // Lum class V (main sequence)
    {
        -0.09f, -0.10f, -0.11f, -0.12f, -0.13f,
        -0.14f, -0.14f, -0.15f, -0.16f, -0.17f,
    },
    // Lum class III
    {
        -0.11f, -0.11f, -0.11f, -0.12f, -0.13f,
        -0.13f, -0.15f, -0.15f, -0.16f, -0.18f,
    },
    // Lum class I
    {
        -0.01f,  0.00f,  0.00f, -0.01f, -0.02f,
        -0.03f, -0.05f, -0.07f, -0.09f, -0.12f,
    }
};

static float bmag_correctionG[3][10] =
{
    // Lum class V (main sequence)
    {
        -0.18f, -0.19f, -0.20f, -0.20f, -0.21f,
        -0.21f, -0.27f, -0.33f, -0.40f, -0.36f,
    },
    // Lum class III
    {
        -0.20f, -0.24f, -0.27f, -0.29f, -0.32f,
        -0.34f, -0.37f, -0.40f, -0.42f, -0.46f,
    },
    // Lum class I
    {
        -0.15f, -0.18f, -0.21f, -0.25f, -0.29f,
        -0.33f, -0.36f, -0.39f, -0.42f, -0.46f,
    }
};

static float bmag_correctionK[3][10] =
{
    // Lum class V (main sequence)
    {
        -0.31f, -0.37f, -0.42f, -0.50f, -0.55f,
        -0.72f, -0.89f, -1.01f, -1.13f, -1.26f,
    },
    // Lum class III
    {
        -0.50f, -0.55f, -0.61f, -0.76f, -0.94f,
        -1.02f, -1.09f, -1.17f, -1.20f, -1.22f,
    },
    // Lum class I
    {
        -0.50f, -0.56f, -0.61f, -0.75f, -0.90f,
        -1.01f, -1.10f, -1.20f, -1.23f, -1.26f,
    }
};

static float bmag_correctionM[3][10] =
{
    // Lum class V (main sequence)
    {
        -1.38f, -1.62f, -1.89f, -2.15f, -2.38f,
        -2.73f, -3.21f, -3.46f, -4.10f, -4.40f,
    },
    // Lum class III
    {
        -1.25f, -1.44f, -1.62f, -1.87f, -2.22f,
        -2.48f, -2.73f, -2.73f, -2.73f, -2.73f,
    },
    // Lum class I
    {
        -1.29f, -1.38f, -1.62f, -2.13f, -2.75f,
        -3.47f, -3.90f, -3.90f, -3.90f, -3.90f,
    }
};

// Brown dwarf data from Grant Hutchison
static float bmag_correctionL[10] =
{
    -4.6f, -4.9f, -5.0f, -5.2f, -5.4f, -5.9f, -6.1f, -6.7f, -7.4f, -8.2f,
};

static float bmag_correctionT[10] =
{
    -8.9f, -9.6f, -10.8f, -11.9f, -13.1f, -14.4f, -16.1f, -17.9f, -19.6f, -21.7f,
};

// Bolometric correction for Brown dwarf Y is an extrapolation of the L & T data
static float bmag_correctionY[10] =
{
    -23.9f, -26.2f, -28.8f, -31.5f, -34.5f, -37.6f, -41.0f, -44.6f, -48.4f, -52.5f,
};


// White dwarf data from Grant Hutchison; value for hypothetical
// 0 subclass is just duplicated from subclass 1.
static float bmag_correctionWD[10] =
{
    -4.15f, -4.15f, -2.22f, -1.24f, -0.67f,
    -0.32f, -0.13f, -0.04f, -0.03f, -0.09f,
};


// Stellar rotation by spectral and luminosity class.
// Tables from Grant Hutchison:
// "Most data are from Lang's _Astrophysical Data: Planets and Stars_ (I
// calculated from theoretical radii and observed rotation velocities), but
// with some additional information gleaned from elsewhere.
// A big scatter in rotation periods, of course, particularly in the K and
// early M dwarfs. I'm not hugely happy with the supergiant and giant rotation
// periods for K and M, either - they may be considerably slower yet, but it's
// obviously difficult to come by the data when the rotation velocity is too
// slow to obviously affect the spectra."
//
// I add missing values by interpolating linearly--certainly not the best
// technique, but adequate for our purposes.  The rotation rate of the Sun
// was used for spectral class G2.

static float rotperiod_O[3][10] =
{
    { 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f },
    { 6.3f, 6.3f, 6.3f, 6.3f, 6.3f, 6.3f, 6.3f, 6.3f, 6.3f, 6.3f },
    { 15.0f, 15.0f, 15.0f, 15.0f, 15.0f, 15.0f, 15.0f, 15.0f, 15.0f, 15.0f },
};

static float rotperiod_B[3][10] =
{
    { 2.0f, 1.8f, 1.6f, 1.4f, 1.1f, 0.8f, 0.8f, 0.8f, 0.8f, 0.7f },
    { 6.3f, 5.6f, 5.0f, 4.3f, 3.7f, 3.1f, 2.9f, 2.8f, 2.7f, 2.6f },
    { 15.0f, 24.0f, 33.0f, 42.0f, 52.0f, 63.0f, 65.0f, 67.0f, 70.0f, 72.0f },
};

static float rotperiod_A[3][10] =
{
    { 0.7f, 0.7f, 0.6f, 0.6f, 0.5f, 0.5f, 0.5f, 0.6f, 0.6f, 0.7f },
    { 2.5f, 2.3f, 2.1f, 1.9f, 1.7f, 1.6f, 1.6f, 1.7f, 1.7f, 1.8f },
    { 75.0f, 77.0f, 80.0f, 82.0f, 85.0f, 87.0f, 95.0f, 104.0f, 115.0f, 125.0f },
};

static float rotperiod_F[3][10] =
{
    { 0.7f, 0.7f, 0.6f, 0.6f, 0.5f, 0.5f, 0.5f, 0.6f, 0.6f, 0.7f },
    { 1.9f, 2.5f, 3.0f, 3.5f, 4.0f, 4.6f, 5.6f, 6.7f, 7.8f, 8.9f },
    { 135.0f, 141.0f, 148.0f, 155.0f, 162.0f, 169.0f, 175.0f, 182.0f, 188.0f, 195.0f },
};

static float rotperiod_G[3][10] =
{
    { 11.1f, 18.2f, 25.4f, 24.7f, 24.0f, 23.3f, 23.0f, 22.7f, 22.3f, 21.9f },
    { 10.0f, 13.0f, 16.0f, 19.0f, 22.0f, 25.0f, 28.0f, 31.0f, 33.0f, 35.0f },
    { 202.0f, 222.0f, 242.0f, 262.0f, 282.0f,
      303.0f, 323.0f, 343.0f, 364.0f, 384.0f },
};

static float rotperiod_K[3][10] =
{
    { 21.5f, 20.8f, 20.2f, 19.4f, 18.8f, 18.2f, 17.6f, 17.0f, 16.4f, 15.8f },
    { 38.0f, 43.0f, 48.0f, 53.0f, 58.0f, 63.0f, 71.0f, 78.0f, 86.0f, 93.0f },
    { 405.0f, 526.0f, 648.0f, 769.0f, 891.0f,
      1012.0f, 1063.0f, 1103.0f, 1154.0f, 1204.0f },
};

static float rotperiod_M[3][10] =
{
    { 15.2f, 12.4f, 9.6f, 6.8f, 4.0f, 1.3f, 1.0f, 0.7f, 0.4f, 0.2f },
    { 101.0f, 101.0f, 101.0f, 101.0f, 101.0f, 101.0f, 101.0f, 101.0f, 101.0f, 101.0f },
    { 1265.0f, 1265.0f, 1265.0f, 1265.0f, 1265.0f,
      1265.0f, 1265.0f, 1265.0f, 1265.0f, 1265.0f },
};


const char* LumClassNames[StellarClass::Lum_Count] = {
    "I-a0", "I-a", "I-b", "II", "III", "IV", "V", "VI", ""
};

const char* SubclassNames[11] = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ""
};

const char* SpectralClassNames[StellarClass::NormalClassCount] = {
    "O", "B", "A", "F", "G", "K", "M", "R",
    "S", "N", "WC", "WN", "?", "L", "T", "Y", "C",
};

const char* WDSpectralClassNames[StellarClass::WDClassCount] = {
    "DA", "DB", "DC", "DO", "DQ", "DZ", "D", "DX",
};


StarDetails*
StarDetails::GetStarDetails(const StellarClass& sc)
{
    switch (sc.getStarType())
    {
    case StellarClass::NormalStar:
        return GetNormalStarDetails(sc.getSpectralClass(),
                                    sc.getSubclass(),
                                    sc.getLuminosityClass());

    case StellarClass::WhiteDwarf:
        return GetWhiteDwarfDetails(sc.getSpectralClass(),
                                    sc.getSubclass());
    case StellarClass::NeutronStar:
        return GetNeutronStarDetails();
    case StellarClass::BlackHole:
        return GetBlackHoleDetails();
    default:
        return nullptr;
    }
}


StarDetails*
StarDetails::CreateStandardStarType(const std::string& specTypeName,
                                    float _temperature,
                                    float _rotationPeriod)

{
    auto* details = new StarDetails();

    details->setTemperature(_temperature);
    details->setSpectralType(specTypeName);

    details->setRotationModel(new UniformRotationModel(_rotationPeriod,
                                                       0.0f,
                                                       astro::J2000,
                                                       0.0f,
                                                       0.0f));

    return details;
}


StarDetails*
StarDetails::GetNormalStarDetails(StellarClass::SpectralClass specClass,
                                  unsigned int subclass,
                                  StellarClass::LuminosityClass lumClass)
{
    if (normalStarDetails == nullptr)
    {
        unsigned int nTypes = StellarClass::Spectral_Count * 11 *
            StellarClass::Lum_Count;
        normalStarDetails = new StarDetails*[nTypes];
        for (unsigned int i = 0; i < nTypes; i++)
            normalStarDetails[i] = nullptr;
    }

    if (subclass > StellarClass::Subclass_Unknown)
        subclass = StellarClass::Subclass_Unknown;

    unsigned int index = subclass + (specClass + lumClass * StellarClass::Spectral_Count) * 11;
    if (normalStarDetails[index] == nullptr)
    {
        string name;
        if ((lumClass == StellarClass::Lum_VI) &&
            (specClass >= StellarClass::Spectral_O) && (specClass <= StellarClass::Spectral_A))
        {
            // Hot subdwarfs are prefixed with "sd", while cool subdwarfs use
            // luminosity class VI, per recommendations in arXiv:0805.2567v1
            name = fmt::sprintf("sd%s%s",
                                SpectralClassNames[specClass],
                                SubclassNames[subclass]);
        }
        else
        {
            name = fmt::sprintf("%s%s%s",
                                SpectralClassNames[specClass],
                                SubclassNames[subclass],
                                LumClassNames[lumClass]);
        }

        // Use the same properties for an unknown subclass as for subclass 5
        if (subclass == StellarClass::Subclass_Unknown)
        {
            // Since early O and Wolf-Rayet stars are exceedingly rare,
            // use temperature of the more common late types when the subclass
            // is unspecified in the spectral type.  For other stars, default
            // to subclass 5.
            switch (specClass)
            {
            case StellarClass::Spectral_O:
            case StellarClass::Spectral_WN:
            case StellarClass::Spectral_WC:
                subclass = 9;
                break;
            case StellarClass::Spectral_Y:
                subclass = 0;
                break;
            default:
                subclass = 5;
                break;
            }
        }

        unsigned int lumIndex = 0;
        switch (lumClass)
        {
        case StellarClass::Lum_Ia0:
        case StellarClass::Lum_Ia:
        case StellarClass::Lum_Ib:
        case StellarClass::Lum_II:
            lumIndex = 2;
            break;
        case StellarClass::Lum_III:
        case StellarClass::Lum_IV:
            lumIndex = 1;
            break;
        case StellarClass::Lum_V:
        case StellarClass::Lum_VI:
        case StellarClass::Lum_Unknown:
            lumIndex = 0;
            break;

        default: break;  // Do nothing, but prevent GCC4 warnings (Beware: potentially dangerous)
        }

        float temp = 0.0f;
        switch (specClass)
        {
        case StellarClass::Spectral_O:
            temp = tempO[lumIndex][subclass];
            break;
        case StellarClass::Spectral_B:
            temp = tempB[lumIndex][subclass];
            break;
        case StellarClass::Spectral_Unknown:
        case StellarClass::Spectral_A:
            temp = tempA[lumIndex][subclass];
            break;
        case StellarClass::Spectral_F:
            temp = tempF[lumIndex][subclass];
            break;
        case StellarClass::Spectral_G:
            temp = tempG[lumIndex][subclass];
            break;
        case StellarClass::Spectral_K:
            temp = tempK[lumIndex][subclass];
            break;
        case StellarClass::Spectral_M:
            temp = tempM[lumIndex][subclass];
            break;
        case StellarClass::Spectral_R:
            temp = tempK[lumIndex][subclass];
            break;
        case StellarClass::Spectral_S:
            temp = tempM[lumIndex][subclass];
            break;
        case StellarClass::Spectral_N:
            temp = tempM[lumIndex][subclass];
            break;
        case StellarClass::Spectral_C:
            temp = tempM[lumIndex][subclass];
            break;
        case StellarClass::Spectral_WN:
            temp = tempWN[subclass];
            break;
        case StellarClass::Spectral_WC:
            temp = tempWC[subclass];
            break;
        case StellarClass::Spectral_L:
            temp = tempL[subclass];
            break;
        case StellarClass::Spectral_T:
            temp = tempT[subclass];
            break;
        case StellarClass::Spectral_Y:
            temp = tempY[subclass];
            break;

        default: break;  // Do nothing, but prevent GCC4 warnings (Beware: potentially dangerous)
        }

        float bmagCorrection = 0.0f;
        float period = 1.0f;
        switch (specClass)
        {
        case StellarClass::Spectral_O:
            period = rotperiod_O[lumIndex][subclass];
            bmagCorrection = bmag_correctionO[lumIndex][subclass];
            break;
        case StellarClass::Spectral_B:
            period = rotperiod_B[lumIndex][subclass];
            bmagCorrection = bmag_correctionB[lumIndex][subclass];
            break;
        case StellarClass::Spectral_Unknown:
        case StellarClass::Spectral_A:
            period = rotperiod_A[lumIndex][subclass];
            bmagCorrection = bmag_correctionA[lumIndex][subclass];
            break;
        case StellarClass::Spectral_F:
            period = rotperiod_F[lumIndex][subclass];
            bmagCorrection = bmag_correctionF[lumIndex][subclass];
            break;
        case StellarClass::Spectral_G:
            period = rotperiod_G[lumIndex][subclass];
            bmagCorrection = bmag_correctionG[lumIndex][subclass];
            break;
        case StellarClass::Spectral_K:
            period = rotperiod_K[lumIndex][subclass];
            bmagCorrection = bmag_correctionK[lumIndex][subclass];
            break;
        case StellarClass::Spectral_M:
            period = rotperiod_M[lumIndex][subclass];
            bmagCorrection = bmag_correctionM[lumIndex][subclass];
            break;

        case StellarClass::Spectral_R:
        case StellarClass::Spectral_S:
        case StellarClass::Spectral_N:
        case StellarClass::Spectral_C:
            period = rotperiod_M[lumIndex][subclass];
            bmagCorrection = bmag_correctionM[lumIndex][subclass];
            break;

        case StellarClass::Spectral_WC:
        case StellarClass::Spectral_WN:
            period = rotperiod_O[lumIndex][subclass];
            bmagCorrection = bmag_correctionO[lumIndex][subclass];
            break;

        case StellarClass::Spectral_L:
            // Assume that brown dwarfs are fast rotators like late M dwarfs
            period = 0.2f;
            bmagCorrection = bmag_correctionL[subclass];
            break;

        case StellarClass::Spectral_T:
            // Assume that brown dwarfs are fast rotators like late M dwarfs
            period = 0.2f;
            bmagCorrection = bmag_correctionT[subclass];
            break;

        case StellarClass::Spectral_Y:
            // Assume that brown dwarfs are fast rotators like late M dwarfs
            period = 0.2f;
            bmagCorrection = bmag_correctionY[subclass];
            break;

        default: break;  // Do nothing, but prevent GCC4 warnings (Beware: potentially dangerous)
        }

        normalStarDetails[index] = CreateStandardStarType(name, temp, period);
        normalStarDetails[index]->setBolometricCorrection(bmagCorrection);

        MultiResTexture starTex = starTextures.starTex[specClass];
        if (!starTex.isValid())
            starTex = starTextures.defaultTex;
        normalStarDetails[index]->setTexture(starTex);
    }

    return normalStarDetails[index];
}


StarDetails*
StarDetails::GetWhiteDwarfDetails(StellarClass::SpectralClass specClass,
                                  unsigned int subclass)
{
    // Hack assumes all WD types are consecutive
    unsigned int scIndex = static_cast<unsigned int>(specClass) -
        StellarClass::FirstWDClass;

    if (whiteDwarfDetails == nullptr)
    {
        unsigned int nTypes =
            StellarClass::WDClassCount * StellarClass::SubclassCount;
        whiteDwarfDetails = new StarDetails*[nTypes];
        for (unsigned int i = 0; i < nTypes; i++)
            whiteDwarfDetails[i] = nullptr;
    }

    if (subclass > StellarClass::Subclass_Unknown)
        subclass = StellarClass::Subclass_Unknown;

    unsigned int index = subclass + (scIndex * StellarClass::SubclassCount);
    if (whiteDwarfDetails[index] == nullptr)
    {
        string name;
        name = fmt::sprintf("%s%s",
                            WDSpectralClassNames[scIndex],
                            SubclassNames[subclass]);

        float temp;
        float bmagCorrection;
        // subclass is always >= 0:
        if (subclass <= 9)
        {
            temp = tempWD[subclass];
            bmagCorrection = bmag_correctionWD[subclass];
        }
        else
        {
            // Treat unknown as subclass 5
            temp = tempWD[5];
            bmagCorrection = bmag_correctionWD[5];
        }

        // Assign white dwarfs a rotation period of half an hour; very
        // rough, as white rotation rates vary a lot.
        float period = 1.0f / 48.0f;

        whiteDwarfDetails[index] = CreateStandardStarType(name, temp, period);
        MultiResTexture starTex = starTextures.starTex[StellarClass::Spectral_D];
        if (!starTex.isValid())
            starTex = starTextures.defaultTex;
        whiteDwarfDetails[index]->setTexture(starTex);
        whiteDwarfDetails[index]->setBolometricCorrection(bmagCorrection);
    }

    return whiteDwarfDetails[index];
}


StarDetails*
StarDetails::GetNeutronStarDetails()
{
    if (neutronStarDetails == nullptr)
    {
        // The default neutron star has a rotation period of one second,
        // surface temperature of five million K.
        neutronStarDetails = CreateStandardStarType("Q", 5000000.0f,
                                                    1.0f / 86400.0f);
        neutronStarDetails->setRadius(10.0f);
        neutronStarDetails->addKnowledge(KnowRadius);
        MultiResTexture starTex = starTextures.neutronStarTex;
        if (!starTex.isValid())
            starTex = starTextures.defaultTex;
        neutronStarDetails->setTexture(starTex);
    }

    return neutronStarDetails;
}


StarDetails*
StarDetails::GetBlackHoleDetails()
{
    if (blackHoleDetails == nullptr)
    {
        // Default black hole parameters are based on a one solar mass
        // black hole.
        // The temperature is computed from the equation:
        //      T=h_bar c^3/(8 pi G k m)
        blackHoleDetails = CreateStandardStarType("X", 6.15e-8f,
                                                  1.0f / 86400.0f);
        blackHoleDetails->setRadius(2.9f);
        blackHoleDetails->addKnowledge(KnowRadius);
    }

    return blackHoleDetails;
}


StarDetails*
StarDetails::GetBarycenterDetails()
{

    if (barycenterDetails == nullptr)
    {
        barycenterDetails = CreateStandardStarType("Bary", 1.0f, 1.0f);
        barycenterDetails->setRadius(0.001f);
        barycenterDetails->addKnowledge(KnowRadius);
        barycenterDetails->setVisibility(false);
    }

    return barycenterDetails;
}


void
StarDetails::SetStarTextures(const StarTextureSet& _starTextures)
{
    starTextures = _starTextures;
}


StarDetails::StarDetails()
{
    spectralType[0] = '\0';
}


StarDetails::StarDetails(const StarDetails& sd) :
    radius(sd.radius),
    temperature(sd.temperature),
    bolometricCorrection(sd.bolometricCorrection),
    knowledge(sd.knowledge),
    visible(sd.visible),
    texture(sd.texture),
    geometry(sd.geometry),
    orbit(sd.orbit),
    orbitalRadius(sd.orbitalRadius),
    barycenter(sd.barycenter),
    rotationModel(sd.rotationModel),
    semiAxes(sd.semiAxes),
    infoURL(sd.infoURL),
    orbitingStars(nullptr),
    isShared(false)
{
    assert(sd.isShared);
    memcpy(spectralType, sd.spectralType, sizeof(spectralType));
}


StarDetails::~StarDetails()
{
    delete orbitingStars;
}


/*! Return the InfoURL. If the InfoURL has not been set, this method
 *  returns an empty string.
 */
const std::string&
StarDetails::getInfoURL() const
{
    return infoURL;
}


void
StarDetails::setRadius(float _radius)
{
    radius = _radius;
}


void
StarDetails::setTemperature(float _temperature)
{
    temperature = _temperature;
}


void
StarDetails::setSpectralType(const std::string& s)
{
    strncpy(spectralType, s.c_str(), sizeof(spectralType));
    spectralType[sizeof(spectralType) - 1] = '\0';
}


void
StarDetails::setKnowledge(uint32_t _knowledge)
{
    knowledge = _knowledge;
}


void
StarDetails::addKnowledge(uint32_t _knowledge)
{
    knowledge |= _knowledge;
}


void
StarDetails::setBolometricCorrection(float correction)
{
    bolometricCorrection = correction;
}


void
StarDetails::setTexture(const MultiResTexture& tex)
{
    texture = tex;
}


void
StarDetails::setGeometry(ResourceHandle rh)
{
    geometry = rh;
}


void
StarDetails::setOrbit(Orbit* o)
{
    orbit = o;
    computeOrbitalRadius();
}


void
StarDetails::setOrbitBarycenter(Star* bc)
{
    barycenter = bc;
    computeOrbitalRadius();
}


void
StarDetails::setOrbitalRadius(float r)
{
    if (orbit != nullptr)
        orbitalRadius = r;
}


void
StarDetails::computeOrbitalRadius()
{
    if (orbit == nullptr)
    {
        orbitalRadius = 0.0f;
    }
    else
    {
        orbitalRadius = (float) astro::kilometersToLightYears(orbit->getBoundingRadius());
        if (barycenter != nullptr)
            orbitalRadius += barycenter->getOrbitalRadius();
    }
}


void
StarDetails::setVisibility(bool b)
{
    visible = b;
}


void
StarDetails::setRotationModel(const RotationModel* rm)
{
    rotationModel = rm;
}


/*! Set the InfoURL for this star.
*/
void
StarDetails::setInfoURL(const string& _infoURL)
{
    infoURL = _infoURL;
}

Star::~Star()
{
    // TODO: Implement reference counting for StarDetails objects so that
    // we can enable this.
#if 0
    if (!details->shared())
        delete details;
#endif
}


// Return the radius of the star in kilometers
float Star::getRadius() const
{
    if (details->getKnowledge(StarDetails::KnowRadius))
        return details->getRadius();

#ifdef NO_BOLOMETRIC_MAGNITUDE_CORRECTION
    // Use the Stefan-Boltzmann law to estimate the radius of a
    // star from surface temperature and luminosity
    return SOLAR_RADIUS * (float) sqrt(getLuminosity()) *
        square(SOLAR_TEMPERATURE / getTemperature());
#else
    // Calculate the luminosity of the star from the bolometric, not the
    // visual magnitude of the star.
    float solarBMag = SOLAR_BOLOMETRIC_MAG;
    float bmag = getBolometricMagnitude();
    auto boloLum = (float) exp((solarBMag - bmag) / LN_MAG);

    // Use the Stefan-Boltzmann law to estimate the radius of a
    // star from surface temperature and luminosity
    return SOLAR_RADIUS * (float) sqrt(boloLum) *
        square(SOLAR_TEMPERATURE / getTemperature());
#endif
}


void
StarDetails::setEllipsoidSemiAxes(const Vector3f& v)
{
    semiAxes = v;
}


bool
StarDetails::shared() const
{
    return isShared;
}


void
StarDetails::addOrbitingStar(Star* star)
{
    assert(!shared());
    if (orbitingStars == nullptr)
        orbitingStars = new vector<Star*>();
    orbitingStars->push_back(star);
}


/*! Get the position of the star in the universal coordinate system.
 */
UniversalCoord
Star::getPosition(double t) const
{
    const Orbit* orbit = getOrbit();
    if (orbit == nullptr)
    {
        return UniversalCoord::CreateLy(getPosition().cast<double>());
    }
    else
    {
        const Star* barycenter = getOrbitBarycenter();

        if (barycenter == nullptr)
        {
            UniversalCoord barycenterPos = UniversalCoord::CreateLy(getPosition().cast<double>());
            return UniversalCoord(barycenterPos).offsetKm(orbit->positionAtTime(t));
        }
        else
        {
            if (barycenter == this)
            {
                cout << "Star orbiting around self (" << getIndex() << ")!\n";
                return UniversalCoord::CreateLy(getPosition().cast<double>());
            }
            return barycenter->getPosition(t).offsetKm(orbit->positionAtTime(t));
        }
    }
}


UniversalCoord
Star::getOrbitBarycenterPosition(double t) const
{
    const Star* barycenter = getOrbitBarycenter();

    if (barycenter == nullptr)
    {
        return UniversalCoord::CreateLy(getPosition().cast<double>());
    }
    else
    {
        return barycenter->getPosition(t);
    }
}


/*! Get the velocity of the star in the universal coordinate system.
 */
Vector3d
Star::getVelocity(double t) const
{
    const Orbit* orbit = getOrbit();
    if (orbit == nullptr)
    {
        // The star doesn't have a defined orbit, so the velocity is just
        // zero. (This will change when stellar proper motion is implemented.)
        return Vector3d::Zero();
    }
    else
    {
        const Star* barycenter = getOrbitBarycenter();

        if (barycenter == nullptr)
        {
            // Star orbit is defined around a fixed point, so the total velocity
            // is just the star's orbit velocity.
            return orbit->velocityAtTime(t);
        }
        else
        {
            // Sum the star's orbital velocity and the velocity of the barycenter.
            return barycenter->getVelocity(t) + orbit->velocityAtTime(t);
        }
    }
}


MultiResTexture
Star::getTexture() const
{
    return getDetails()->getTexture();
}


ResourceHandle
Star::getGeometry() const
{
    return getDetails()->getGeometry();
}


/*! Return the InfoURL. If the InfoURL has not been set, this method
*  returns an empty string.
*/
const string&
Star::getInfoURL() const
{
    return getDetails()->getInfoURL();
}

float Star::getApparentMagnitude(float ly) const
{
    return astro::absToAppMag(getAbsoluteMagnitude(), ly);
}

float Star::getLuminosity() const
{
    return astro::absMagToLum(getAbsoluteMagnitude());
}

void Star::setLuminosity(float lum)
{
    setAbsoluteMagnitude(astro::lumToAbsMag(lum));
}

StarDetails* Star::getDetails() const
{
    return details;
}

void Star::setDetails(StarDetails* sd)
{
    // TODO: delete existing details if they aren't shared
    details = sd;
}

void Star::setOrbitBarycenter(Star* s)
{
    if (s == this)
    {
        fmt::fprintf(cerr, "Star (%i) going to orbit self!\n", getIndex());
        return;
    }
    if (details->shared())
        details = new StarDetails(*details);
    getDetails()->setOrbitBarycenter(s);
}

void Star::computeOrbitalRadius()
{
    getDetails()->computeOrbitalRadius();
}

void
Star::setRotationModel(const RotationModel* rm)
{
    getDetails()->setRotationModel(rm);
}

void
Star::addOrbitingStar(Star* star)
{
    if (getDetails()->shared())
        details = new StarDetails(*details);
    getDetails()->addOrbitingStar(star);
}

Selection Star::toSelection()
{
//    std::cout << "Star::toSelection()\n";
    return Selection(this);
}

bool Star::createStar(Star* star,
                            DataDisposition disposition,
                            Hash* starData,
                            const string& path,
                            bool isBarycenter,
                            AstroDatabase *db
                     )
{
    StarDetails* details = nullptr;
    string spectralType;

    // Get the magnitude and spectral type; if the star is actually
    // a barycenter placeholder, these fields are ignored.
    if (isBarycenter)
    {
        details = StarDetails::GetBarycenterDetails();
    }
    else
    {
        if (starData->getString("SpectralType", spectralType))
        {
            StellarClass sc = StellarClass::parse(spectralType);
            details = StarDetails::GetStarDetails(sc);
            if (details == nullptr)
            {
                cerr << _("Invalid star: bad spectral type.\n");
                return false;
            }
        }
        else
        {
            // Spectral type is required for new stars
            if (disposition != DataDisposition::Modify)
            {
                cerr << _("Invalid star: missing spectral type.\n");
                return false;
            }
        }
    }

    bool modifyExistingDetails = false;
    if (disposition == DataDisposition::Modify)
    {
        StarDetails* existingDetails = star->getDetails();

        // If we're modifying an existing star and it already has a
        // customized details record, we'll just modify that.
        if (!existingDetails->shared())
        {
            modifyExistingDetails = true;
            if (details != nullptr)
            {
                // If the spectral type was modified, copy the new data
                // to the custom details record.
                existingDetails->setSpectralType(details->getSpectralType());
                existingDetails->setTemperature(details->getTemperature());
                existingDetails->setBolometricCorrection(details->getBolometricCorrection());
                if ((existingDetails->getKnowledge() & StarDetails::KnowTexture) == 0)
                    existingDetails->setTexture(details->getTexture());
                if ((existingDetails->getKnowledge() & StarDetails::KnowRotation) == 0)
                    existingDetails->setRotationModel(details->getRotationModel());
                existingDetails->setVisibility(details->getVisibility());
            }

            details = existingDetails;
        }
        else if (details == nullptr)
        {
            details = existingDetails;
        }
    }

    string modelName;
    string textureName;
    bool hasTexture = starData->getString("Texture", textureName);
    bool hasModel = starData->getString("Mesh", modelName);

    RotationModel* rm = CreateRotationModel(starData, path, 1.0);
    bool hasRotationModel = (rm != nullptr);

    Vector3d semiAxes = Vector3d::Ones();
    bool hasSemiAxes = starData->getLengthVector("SemiAxes", semiAxes);
    bool hasBarycenter = false;
    Eigen::Vector3f barycenterPosition;

    double radius;
    bool hasRadius = starData->getLength("Radius", radius);

    double temperature = 0.0;
    bool hasTemperature = starData->getNumber("Temperature", temperature);
    // disallow unphysical temperature values
    if (temperature <= 0.0)
    {
        hasTemperature = false;
    }

    double bolometricCorrection;
    bool hasBolometricCorrection = starData->getNumber("BoloCorrection", bolometricCorrection);

    string infoURL;
    bool hasInfoURL = starData->getString("InfoURL", infoURL);

    Orbit* orbit = CreateOrbit(Selection(), starData, path, true);

    if (!modifyExistingDetails)
        star->setDetails(details);

    if (hasTexture              ||
        hasModel                ||
        orbit != nullptr        ||
        hasSemiAxes             ||
        hasRadius               ||
        hasTemperature          ||
        hasBolometricCorrection ||
        hasRotationModel        ||
        hasInfoURL)
    {
        // If the star definition has extended information, clone the
        // star details so we can customize it without affecting other
        // stars of the same spectral type.
        bool free_details = false;
        if (!modifyExistingDetails)
        {
//             clog << "  Cloning star details...\n";
            details = new StarDetails(*details);
            free_details = true;
            star->setDetails(details);
        }

        if (hasTexture)
        {
            details->setTexture(MultiResTexture(textureName, path));
            details->addKnowledge(StarDetails::KnowTexture);
        }

        if (hasModel)
        {
            ResourceHandle geometryHandle = GetGeometryManager()->getHandle(GeometryInfo(modelName, path, Vector3f::Zero(), 1.0f, true));
            details->setGeometry(geometryHandle);
        }

        if (hasSemiAxes)
        {
            details->setEllipsoidSemiAxes(semiAxes.cast<float>());
        }

        if (hasRadius)
        {
            details->setRadius((float) radius);
            details->addKnowledge(StarDetails::KnowRadius);
        }

        if (hasTemperature)
        {
            details->setTemperature((float) temperature);

            if (!hasBolometricCorrection)
            {
                // if we change the temperature, recalculate the bolometric
                // correction using formula from formula for main sequence
                // stars given in B. Cameron Reed (1998), "The Composite
                // Observational-Theoretical HR Diagram", Journal of the Royal
                // Astronomical Society of Canada, Vol 92. p36.

                double logT = log10(temperature) - 4;
                double bc = -8.499 * pow(logT, 4) + 13.421 * pow(logT, 3)
                            - 8.131 * logT * logT - 3.901 * logT - 0.438;

                details->setBolometricCorrection((float) bc);
            }
        }

        if (hasBolometricCorrection)
        {
            details->setBolometricCorrection((float) bolometricCorrection);
        }

        if (hasInfoURL)
        {
            details->setInfoURL(infoURL);
        }

        if (orbit != nullptr)
        {
            details->setOrbit(orbit);

            // See if a barycenter was specified as well
            uint32_t barycenterCatNo = AstroCatalog::InvalidIndex;
            bool barycenterDefined = false;

            string barycenterName;
            if (starData->getString("OrbitBarycenter", barycenterName))
            {
                barycenterCatNo   = db->starnameToIndex(barycenterName, true);
                barycenterDefined = true;
            }
            else if (starData->getNumber("OrbitBarycenter", barycenterCatNo))
            {
                barycenterDefined = true;
            }

            if (barycenterDefined)
            {
                if (barycenterCatNo != AstroCatalog::InvalidIndex)
                {
                    if (barycenterCatNo == star->getIndex())
                    {
                        fmt::fprintf(cerr, "Barycenter index %i same as orbiting star!\n", barycenterCatNo);
                        return false;
                    }
//                     clog << "  requesting barycenter with nr " << barycenterCatNo << endl;
                    Star* barycenter = db->getStar(barycenterCatNo);
                    if (barycenter != nullptr)
                    {
                        if (barycenter == star)
                        {
                            fmt::fprintf(cerr, "Created star %i going to orbit self with nr %i!\n", star->getIndex(), barycenter->getIndex());
                            return false;
                        }
                        hasBarycenter = true;
                        barycenterPosition = barycenter->getPosition().cast<float>();
                        star->setOrbitBarycenter(barycenter);
                        barycenter->addOrbitingStar(star);
                    }
                }

                if (!hasBarycenter)
                {
                    fmt::fprintf(cerr, _("Barycenter %s of star nr %u does not exist.\n"), barycenterName, star->getIndex());
                    delete rm;
                    if (free_details)
                        delete details;
                    return false;
                }
            }
        }

        if (hasRotationModel)
            details->setRotationModel(rm);
    }

/*    if (disposition != DataDisposition::Modify)
        star->setMainIndexNumber(catalogNumber);*/

    // Compute the position in rectangular coordinates.  If a star has an
    // orbit and barycenter, it's position is the position of the barycenter.
    if (hasBarycenter)
    {
        star->setPosition(barycenterPosition.cast<double>());
    }
    else
    {
        double ra = 0.0;
        double dec = 0.0;
        double distance = 0.0;

        if (disposition == DataDisposition::Modify)
        {
            Vector3f pos = star->getPosition().cast<float>();

            // Convert from Celestia's coordinate system
            Vector3f v(pos.x(), -pos.z(), pos.y());
            v = Quaternionf(AngleAxis<float>((float) astro::J2000Obliquity, Vector3f::UnitX())) * v;

            distance = v.norm();
            if (distance > 0.0)
            {
                v.normalize();
                ra = radToDeg(std::atan2(v.y(), v.x())) / DEG_PER_HRA;
                dec = radToDeg(std::asin(v.z()));
            }
        }

        bool modifyPosition = false;
        if (starData->getAngle("RA", ra, DEG_PER_HRA, 1.0))
        {
            modifyPosition = true;
        }
        else
        {
            if (disposition != DataDisposition::Modify)
            {
                cerr << _("Invalid star: missing right ascension\n");
                return false;
            }
        }

        if (starData->getAngle("Dec", dec))
        {
            modifyPosition = true;
        }
        else
        {
            if (disposition != DataDisposition::Modify)
            {
                cerr << _("Invalid star: missing declination.\n");
                return false;
            }
        }

        if (starData->getLength("Distance", distance, KM_PER_LY))
        {
            modifyPosition = true;
        }
        else
        {
            if (disposition != DataDisposition::Modify)
            {
                cerr << _("Invalid star: missing distance.\n");
                return false;
            }
        }

        // Truncate to floats to match behavior of reading from binary file.
        // The conversion to rectangular coordinates is still performed at
        // double precision, however.
        if (modifyPosition)
        {
            float raf = ((float) ra);
            float decf = ((float) dec);
            float distancef = ((float) distance);
            Vector3d pos = astro::equatorialToCelestialCart((double) raf, (double) decf, (double) distancef);
            star->setPosition(pos);
        }
    }

    if (isBarycenter)
    {
        star->setAbsoluteMagnitude(30.0f);
    }
    else
    {
        double magnitude = 0.0;
        bool magnitudeModified = true;
        if (!starData->getNumber("AbsMag", magnitude))
        {
            if (!starData->getNumber("AppMag", magnitude))
            {
                if (disposition != DataDisposition::Modify)
                {
                    clog << _("Invalid star: missing magnitude.\n");
                    return false;
                }
                else
                {
                    magnitudeModified = false;
                }
            }
            else
            {
                float distance = star->getPosition().norm();

                // We can't compute the intrinsic brightness of the star from
                // the apparent magnitude if the star is within a few AU of the
                // origin.
                if (distance < 1e-5f)
                {
                    clog << _("Invalid star: absolute (not apparent) magnitude must be specified for star near origin\n");
                    return false;
                }
                magnitude = astro::appToAbsMag((float) magnitude, distance);
            }
        }

        if (magnitudeModified)
            star->setAbsoluteMagnitude((float) magnitude);
    }

    return true;
}
