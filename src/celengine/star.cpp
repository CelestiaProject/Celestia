// star.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celmath/mathlib.h>
#include <cstring>
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
// missing (and typically not used) types in those tables were just
// interpolated.

// TODO
// These temperatures are valid for main sequence stars . . . add
// tables for giants and supergiants

struct SpectralTypeInfo
{
    char* name;
    float temperature;
    float rotationPeriod;
};

static StarDetails** normalStarDetails = NULL;
static StarDetails** whiteDwarfDetails = NULL;
static StarDetails*  neutronStarDetails = NULL;
static StarDetails*  blackHoleDetails = NULL;

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


char* LumClassNames[StellarClass::Lum_Count] = {
    "I-a0", "I-a", "I-b", "II", "III", "IV", "V", "VI", ""
};

char* SubclassNames[11] = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ""
};

char* SpectralClassNames[StellarClass::NormalClassCount] = {
    "O", "B", "A", "F", "G", "K", "M", "R",
    "S", "N", "WC", "WN", "?", "L", "T", "C",
};

char* WDSpectralClassNames[StellarClass::WDClassCount] = {
    "DA", "DB", "DC", "DO", "DQ", "DZ", "D"
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
        return NULL;
    }
}

StarDetails*
StarDetails::CreateStandardStarType(const std::string& specTypeName,
                                    float _temperature,
                                    float _rotationPeriod)
                                    
{
    StarDetails* details = new StarDetails();

    details->setTemperature(_temperature);
    details->setRotationPeriod(_rotationPeriod);
    details->setSpectralType(specTypeName);

    return details;
}


StarDetails*
StarDetails::GetNormalStarDetails(StellarClass::SpectralClass specClass,
                                  unsigned int subclass,
                                  StellarClass::LuminosityClass lumClass)
{
    if (normalStarDetails == NULL)
    {
        unsigned int nTypes = StellarClass::Spectral_Count * 11 * 
            StellarClass::Lum_Count;
        normalStarDetails = new StarDetails*[nTypes];
        for (unsigned int i = 0; i < nTypes; i++)
            normalStarDetails[i] = NULL;
    }

    if (subclass > StellarClass::Subclass_Unknown)
        subclass = StellarClass::Subclass_Unknown;

    uint index = subclass + (specClass + lumClass * StellarClass::Spectral_Count) * 11;
    if (normalStarDetails[index] == NULL)
    {
        char name[16];
        sprintf(name, "%s%s%s",
                SpectralClassNames[specClass],
                SubclassNames[subclass],
                LumClassNames[lumClass]);

        // Use the same properties for an unknown subclass as for subclass 5
        if (subclass == StellarClass::Subclass_Unknown)
            subclass = 5;

        float temp = 0.0f;
        switch (specClass)
        {
        case StellarClass::Spectral_O:
            temp = tempO[subclass];
            break;
        case StellarClass::Spectral_B:
            temp = tempB[subclass];
            break;
        case StellarClass::Spectral_Unknown:
        case StellarClass::Spectral_A:
            temp = tempA[subclass];
            break;
        case StellarClass::Spectral_F:
            temp = tempF[subclass];
            break;
        case StellarClass::Spectral_G:
            temp = tempG[subclass];
            break;
        case StellarClass::Spectral_K:
            temp = tempK[subclass];
            break;
        case StellarClass::Spectral_M:
            temp = tempM[subclass];
            break;
        case StellarClass::Spectral_R:
            temp = tempK[subclass];
            break;
        case StellarClass::Spectral_S:
            temp = tempM[subclass];
            break;
        case StellarClass::Spectral_N:
            temp = tempM[subclass];
            break;
        case StellarClass::Spectral_C:
            temp = tempM[subclass];
            break;
        case StellarClass::Spectral_WN:
        case StellarClass::Spectral_WC:
            temp = tempO[subclass];
            break;
        case StellarClass::Spectral_L:
            temp = tempL[subclass];
            break;
        case StellarClass::Spectral_T:
            temp = tempT[subclass];
            break;
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
        }

        normalStarDetails[index] = CreateStandardStarType(name, temp, period);
        normalStarDetails[index]->setBolometricCorrection(bmagCorrection);
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

    if (whiteDwarfDetails == NULL)
    {
        unsigned int nTypes = 
            StellarClass::WDClassCount * StellarClass::SubclassCount;
        whiteDwarfDetails = new StarDetails*[nTypes];
        for (unsigned int i = 0; i < nTypes; i++)
            whiteDwarfDetails[i] = NULL;
    }

    if (subclass > StellarClass::Subclass_Unknown)
        subclass = StellarClass::Subclass_Unknown;

    uint index = subclass + (scIndex * StellarClass::SubclassCount);
    if (whiteDwarfDetails[index] == NULL)
    {
        char name[16];
        sprintf(name, "%s%s",
                WDSpectralClassNames[scIndex],
                SubclassNames[subclass]);

        float temp;
        if (subclass == 0)
            temp = 100000.0f;
        if (subclass == StellarClass::Subclass_Unknown)
            temp = 10080.0f;  // Treat unknown as subclass 5
        else
            temp = 50400.0f / subclass;

        // Assign white dwarfs a rotation period of half an hour; very
        // rough, as white rotation rates vary a lot.
        float period = 1.0f / 48.0f;
        
        whiteDwarfDetails[index] = CreateStandardStarType(name, temp, period);
    }

    return whiteDwarfDetails[index];
}


StarDetails*
StarDetails::GetNeutronStarDetails()
{
    if (neutronStarDetails == NULL)
    {
        // The default neutron star has a rotation period of one second,
        // surface temperature of five million K.
        neutronStarDetails = CreateStandardStarType("Q", 5000000.0f,
                                                    1.0f / 86400.0f);
        neutronStarDetails->setRadius(10.0f);
    }

    return neutronStarDetails;
}


StarDetails*
StarDetails::GetBlackHoleDetails()
{
    if (blackHoleDetails == NULL)
    {
        // Default black hole parameters are based on a one solar mass
        // black hole.
        // The temperature is computed from the equation:
        //      T=h_bar c^3/(8 pi G k m)
        blackHoleDetails = CreateStandardStarType("X", 6.15e-8f,
                                                  1.0f / 86400.0f);
        blackHoleDetails->setRadius(2.9f);
    }

    return blackHoleDetails;
}


StarDetails::StarDetails() :
    radius(0.0f),
    temperature(0.0f),
    bolometricCorrection(0.0f),
    rotationPeriod(1.0f),
    knowledge(0u),
    model(InvalidResource),
    orbit(NULL),
    orbitalRadius(0.0f)
{
    spectralType[0] = '\0';
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
StarDetails::setRotationPeriod(float _rotationPeriod)
{
    rotationPeriod = _rotationPeriod;
}


void
StarDetails::setSpectralType(const std::string& s)
{
    strncpy(spectralType, s.c_str(), sizeof(spectralType));
    spectralType[sizeof(spectralType) - 1] = '\0';
}


void
StarDetails::setKnowledge(uint32 _knowledge)
{
    knowledge = _knowledge;
}


void
StarDetails::addKnowledge(uint32 _knowledge)
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
StarDetails::setModel(ResourceHandle rh)
{
    model = rh;
}


void
StarDetails::setOrbit(Orbit* o)
{
    orbit = o;
}


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


MultiResTexture
Star::getTexture() const
{
    return details->getTexture();
}


ResourceHandle
Star::getModel() const
{
    return details->getModel();
}


void Star::setCatalogNumber(uint32 n)
{
    catalogNumber = n;
}

void Star::setPosition(float x, float y, float z)
{
    position = Point3f(x, y, z);
}

void Star::setPosition(Point3f p)
{
    position = p;
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

void Star::setDetails(StarDetails* sd)
{
    details = sd;
}
