// star.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celmath/mathlib.h>
#include "celestia.h"
#include "astro.h"
#include "star.h"


// The value of the temperature of the sun is actually 5780, but the
// stellar class tables list the temperature of a G2V star as 5860.  We
// use the latter value so that the radius of the sun is computed correctly
// as one times SOLAR_RADIUS . . .  the high metallicity of the Sun is
// probably what accounts for the discrepancy in temperature.
// #define SOLAR_TEMPERATURE    5780.0f
#define SOLAR_TEMPERATURE    5860.0f
#define SOLAR_RADIUS         696000

// Star temperature data from Carroll and Ostlie's
// _Modern Astrophysics_ (1996), p. A-13 - A-18.  Temperatures from
// missing (an typically not used) types in those tables were just
// interpolated.

// TODO
// These temperatures are valid for main sequence stars . . . add
// tables for giants and supergiants
static float tempO[10] =
{
    50000, 50000, 50000, 50000, 47000,
    44500, 41000, 38000, 35800, 33000
};

static float tempB[10] =
{
    30000, 25400, 22000, 18700, 17000,
    15400, 14000, 13000, 11900, 10500
};

static float tempA[10] =
{
    9520, 9230, 8970, 8720, 8460, 8200, 8020, 7850, 7580, 7390
};

static float tempF[10] =
{
    7200, 7050, 6890, 6740, 6590, 6440, 6360, 6280, 6200, 6110
};

static float tempG[10] =
{
    6030, 5940, 5860, 5830, 5800, 5770, 5700, 5630, 5570, 5410
};

static float tempK[10] =
{
    5250, 5080, 4900, 4730, 4590, 4350, 4200, 4060, 3990, 3920
};

static float tempM[10] =
{
    3850, 3720, 3580, 3470, 3370, 3240, 3050, 2940, 2640, 2000
};

static float tempL[10] =
{
    1960, 1930, 1900, 1850, 1800, 1740, 1680, 1620, 1560, 1500
};

static float tempT[10] =
{
    1425, 1350, 1275, 1200, 1140, 1080, 1020, 900, 800, 750
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
    -8.9f, -9.6f, -10.8f, -11.9f, -13.1f, -14.4f, -16.1f, -17.9f, -19.6f, -19.6f,
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


// Return the radius of the star in kilometers
float Star::getRadius() const
{
#ifdef NO_BOLOMETRIC_MAGNITUDE_CORRECTION
    // Use the Stefan-Boltzmann law to estimate the radius of a
    // star from surface temperature and luminosity
    return SOLAR_RADIUS * (float) sqrt(getLuminosity()) *
        square(SOLAR_TEMPERATURE / getTemperature());
#else
    // Calculate the luminosity of the star from the bolometric, not the
    // visual magnitude of the star.
    float solarBMag = SOLAR_ABSMAG + bmag_correctionG[0][2];
    float bmag = getBolometricMagnitude();
    float boloLum = (float) exp((solarBMag - bmag) / LN_MAG);

    // Use the Stefan-Boltzmann law to estimate the radius of a
    // star from surface temperature and luminosity
    return SOLAR_RADIUS * (float) sqrt(boloLum) *
        square(SOLAR_TEMPERATURE / getTemperature());
#endif
}


// Return the temperature in Kelvin
float Star::getTemperature() const
{
    if (stellarClass.getStarType() == StellarClass::NormalStar)
    {
        unsigned int specSubClass = stellarClass.getSpectralSubclass();
        switch (stellarClass.getSpectralClass())
        {
        case StellarClass::Spectral_O:
            return tempO[specSubClass];
        case StellarClass::Spectral_B:
            return tempB[specSubClass];
        case StellarClass::Spectral_Unknown:
        case StellarClass::Spectral_A:
            return tempA[specSubClass];
        case StellarClass::Spectral_F:
            return tempF[specSubClass];
        case StellarClass::Spectral_G:
            return tempG[specSubClass];
        case StellarClass::Spectral_K:
            return tempK[specSubClass];
        case StellarClass::Spectral_M:
            return tempM[specSubClass];
        case StellarClass::Spectral_R:
            return tempK[specSubClass];
        case StellarClass::Spectral_S:
            return tempM[specSubClass];
        case StellarClass::Spectral_N:
            return tempM[specSubClass];
        case StellarClass::Spectral_WN:
        case StellarClass::Spectral_WC:
            return tempO[specSubClass];
        case StellarClass::Spectral_L:
            return tempL[specSubClass];
        case StellarClass::Spectral_T:
            return tempT[specSubClass];
        default:
            // TODO: Bad spectral class
            return -1;
        }
    }
    else if (stellarClass.getStarType() == StellarClass::WhiteDwarf)
    {
        // TODO
        // Return a better temperature estimate for white dwarf
        // and neutron stars.
        return 10000;
    }
    else if (stellarClass.getStarType() == StellarClass::NeutronStar)
    {
        return 10000;
    }
    else if (stellarClass.getStarType() == StellarClass::BlackHole)
    {
        return 0;
    }
    else
    {
        // TODO: Bad star type
        return -1;
    }
}


// Estimate the bolometric magnitude of the star from the absolute visual
// magnitude and the spectral class.
float Star::getBolometricMagnitude() const
{
    float bolometricCorrection = 0.0f;

    if (stellarClass.getStarType() == StellarClass::NormalStar)
    {
        unsigned int specSubClass = stellarClass.getSpectralSubclass();
        unsigned int lumIndex = 0;
        
        switch (stellarClass.getLuminosityClass())
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
            lumIndex = 0;
            break;
        }

        switch (stellarClass.getSpectralClass())
        {
        case StellarClass::Spectral_O:
            bolometricCorrection = bmag_correctionO[lumIndex][specSubClass];
            break;
        case StellarClass::Spectral_B:
            bolometricCorrection = bmag_correctionB[lumIndex][specSubClass];
            break;
        case StellarClass::Spectral_Unknown:
        case StellarClass::Spectral_A:
            bolometricCorrection = bmag_correctionA[lumIndex][specSubClass];
            break;
        case StellarClass::Spectral_F:
            bolometricCorrection = bmag_correctionF[lumIndex][specSubClass];
            break;
        case StellarClass::Spectral_G:
            bolometricCorrection = bmag_correctionG[lumIndex][specSubClass];
            break;
        case StellarClass::Spectral_K:
            bolometricCorrection = bmag_correctionK[lumIndex][specSubClass];
            break;
        case StellarClass::Spectral_M:
            bolometricCorrection = bmag_correctionM[lumIndex][specSubClass];
            break;

        case StellarClass::Spectral_R:
        case StellarClass::Spectral_S:
        case StellarClass::Spectral_N:
            bolometricCorrection = bmag_correctionM[lumIndex][8];
            break;

        case StellarClass::Spectral_WC:
        case StellarClass::Spectral_WN:
            // Treat Wolf-Rayet stars like O
            bolometricCorrection = bmag_correctionO[lumIndex][specSubClass];
            break;

        // Brown dwarf types
        case StellarClass::Spectral_L:
            bolometricCorrection = bmag_correctionL[specSubClass];
            break;
        case StellarClass::Spectral_T:
            bolometricCorrection = bmag_correctionT[specSubClass];
            break;
        }
    }
    else if (stellarClass.getStarType() == StellarClass::WhiteDwarf)
    {
        bolometricCorrection = 0.0f;
    }
    else if (stellarClass.getStarType() == StellarClass::NeutronStar)
    {
        bolometricCorrection = 0.0f;
    }

    return getAbsoluteMagnitude() + bolometricCorrection;
}


// Return the rotation period of the star in days.  For normal stars, we use
// the rotation period tables defined above.
float Star::getRotationPeriod() const
{
    float period = 1.0f;

    if (stellarClass.getStarType() == StellarClass::NormalStar)
    {
        unsigned int specSubClass = stellarClass.getSpectralSubclass();
        unsigned int lumIndex = 0;
        
        switch (stellarClass.getLuminosityClass())
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
            lumIndex = 0;
            break;
        }

        switch (stellarClass.getSpectralClass())
        {
        case StellarClass::Spectral_O:
            period = rotperiod_O[lumIndex][specSubClass];
            break;
        case StellarClass::Spectral_B:
            period = rotperiod_B[lumIndex][specSubClass];
            break;
        case StellarClass::Spectral_Unknown:
        case StellarClass::Spectral_A:
            period = rotperiod_A[lumIndex][specSubClass];
            break;
        case StellarClass::Spectral_F:
            period = rotperiod_F[lumIndex][specSubClass];
            break;
        case StellarClass::Spectral_G:
            period = rotperiod_G[lumIndex][specSubClass];
            break;
        case StellarClass::Spectral_K:
            period = rotperiod_K[lumIndex][specSubClass];
            break;
        case StellarClass::Spectral_M:
            period = rotperiod_M[lumIndex][specSubClass];
            break;

        case StellarClass::Spectral_R:
        case StellarClass::Spectral_S:
        case StellarClass::Spectral_N:
            period = rotperiod_M[lumIndex][specSubClass];
            break;

        case StellarClass::Spectral_WC:
        case StellarClass::Spectral_WN:
            period = rotperiod_O[lumIndex][specSubClass];
            break;

        case StellarClass::Spectral_L:
        case StellarClass::Spectral_T:
            // Assume that brown dwarfs are fast rotators like late M dwarfs
            period = 0.2f;
            break;
        }
    }
    else if (stellarClass.getStarType() == StellarClass::WhiteDwarf)
    {
        // Assign white dwarfs a rotation period of half an hour; very
        // rough, as white rotation rates vary a lot.
        return 1.0f / 48.0f;
    }
    else if (stellarClass.getStarType() == StellarClass::NeutronStar)
    {
        // Let all neutron stars have a rotation period of one second
        return 1.0f / 86400.0f;
    }

    return period;
}


void Star::setCatalogNumber(uint32 n)
{
    catalogNumbers[0] = n;
}

void Star::setCatalogNumber(unsigned int which, uint32 n)
{
    catalogNumbers[which] = n;
}

void Star::setPosition(float x, float y, float z)
{
    position = Point3f(x, y, z);
}

void Star::setPosition(Point3f p)
{
    position = p;
}


void Star::setStellarClass(StellarClass sc)
{
    stellarClass = sc;
}


void Star::setAbsoluteMagnitude(float mag)
{
    absMag = mag;
}


float Star::getApparentMagnitude(float ly) const
{
    return astro::absToAppMag(absMag, ly);
}


float Star::getLuminosity() const
{
    return astro::absMagToLum(absMag);
}

void Star::setLuminosity(float lum)
{
    absMag = astro::lumToAbsMag(lum);
}
