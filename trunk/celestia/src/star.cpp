// star.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celestia.h"
#include "astro.h"
#include "mathlib.h"
#include "star.h"


#define SOLAR_TEMPERATURE    5780.0f
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
    3850, 3720, 3580, 3470, 3370, 3240, 3050, 2940, 2640, 2600
};



// Return the radius of the star in kilometers
float Star::getRadius() const
{
    // Use the Stefan-Boltzmann law to estimate the radius of a
    // star from surface temperature and luminosity
    return SOLAR_RADIUS * (float) sqrt(getLuminosity()) /
        square(getTemperature() / SOLAR_TEMPERATURE);
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
        case StellarClass::Spectral_WC:
            return tempM[specSubClass];
        case StellarClass::Spectral_WN:
            return tempM[specSubClass];
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
    else
    {
        // TODO: Bad star type
        return -1;
    }
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

void Star::setLuminosity(float lum)
{
    luminosity = lum;
}

void Star::setStellarClass(StellarClass sc)
{
    stellarClass = sc;
}


float Star::getAbsoluteMagnitude() const
{
    return astro::lumToAbsMag(luminosity);
}

void Star::setAbsoluteMagnitude(float mag)
{
    luminosity = astro::absMagToLum(mag);
}
