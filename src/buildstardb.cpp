// buildstardb.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <assert.h>
#include "stardb.h"

using namespace std;


static string MainDatabaseFile("hip_main.dat");
static string ComponentDatabase("h_dm_com.dat");
static string OrbitalDatabase("hip_dm_o.dat");

static const int HipStarRecordLength = 451;

static uint32 NullCCDMIdentifier = 0xffffffff;
static uint32 NullCatalogNumber = 0xffffffff;


class HipparcosStar
{
public:
    HipparcosStar();

    void write(ostream&);
  
    uint32 HIPCatalogNumber;
    uint32 HDCatalogNumber;
    float ascension;
    float declination;
    float parallax;
    float appMag;
    StellarClass stellarClass;

    uint32 CCDMIdentifier;
    uint8 starsWithCCDM;
    uint8 parallaxError;
};

HipparcosStar::HipparcosStar() :
    HIPCatalogNumber(NullCatalogNumber),
    HDCatalogNumber(NullCatalogNumber),
    ascension(0.0f),
    declination(0.0f),
    parallax(0.0f),
    appMag(0.0f),
    CCDMIdentifier(NullCCDMIdentifier),
    starsWithCCDM(0),
    parallaxError(0)
{
}

template<class T> void binwrite(ostream& out, T x)
{
    out.write(reinterpret_cast<char*>(&x), sizeof(T));
}

void HipparcosStar::write(ostream& out)
{
    if (HDCatalogNumber != NullCatalogNumber)
        binwrite(out, HDCatalogNumber);
    else
        binwrite(out, HIPCatalogNumber | 0x10000000);
    binwrite(out, ascension);
    binwrite(out, declination);
    binwrite(out, parallax);
    binwrite(out, (short) (appMag * 256));
    binwrite(out, stellarClass);
    binwrite(out, parallaxError);
}


class MultistarSystem
{
public:
    int nStars; // Never greater than four in the HIPPARCOS catalog
    HipparcosStar* stars[4];
};


vector<HipparcosStar> stars;

typedef map<uint32, MultistarSystem*> MultistarSystemCatalog;
MultistarSystemCatalog starSystems;


StellarClass ParseStellarClass(char *starType)
{
    StellarClass::StarType type = StellarClass::NormalStar;
    StellarClass::SpectralClass specClass = StellarClass::Spectral_A;
    StellarClass::LuminosityClass lum = StellarClass::Lum_V;
    unsigned short number = 5;
    int i = 0;

    // Subdwarves (luminosity class VI) are prefixed with sd
    if (starType[i] == 's' && starType[i + 1] == 'd')
    {
        lum = StellarClass::Lum_VI;
        i += 2;
    }

    switch (starType[i])
    {
    case 'O':
        specClass = StellarClass::Spectral_O;
        break;
    case 'B':
        specClass = StellarClass::Spectral_B;
        break;
    case 'A':
        specClass = StellarClass::Spectral_A;
        break;
    case 'F':
        specClass = StellarClass::Spectral_F;
        break;
    case 'G':
        specClass = StellarClass::Spectral_G;
        break;
    case 'K':
        specClass = StellarClass::Spectral_K;
        break;
    case 'M':      
        specClass = StellarClass::Spectral_M;
        break;
    case 'R':
        specClass = StellarClass::Spectral_R;
        break;
    case 'N':
        specClass = StellarClass::Spectral_S;
        break;
    case 'S':
        specClass = StellarClass::Spectral_N;
        break;
    case 'W':
	i++;
	if (starType[i] == 'C')
 	    specClass = StellarClass::Spectral_WC;
	else if (starType[i] == 'N')
	    specClass = StellarClass::Spectral_WN;
	else
	    i--;
	break;

    case 'D':
        type = StellarClass::WhiteDwarf;
        return StellarClass(type, specClass, 0, lum);

    default:
        specClass = StellarClass::Spectral_Unknown;
        break;
    }

    i++;
    if (starType[i] >= '0' && starType[i] <= '9')
    {
        number = starType[i] - '0';
    }
    else
    {
        // No number given for spectral class; assume it's a 5 unless
        // the star is type O, as O5 stars are exceedingly rare.
        if (specClass == StellarClass::Spectral_O)
            number = 9;
        else
            number = 5;
    }

    if (lum != StellarClass::Lum_VI)
    {
        i++;
        lum = StellarClass::Lum_V;
        while (i < 13 && starType[i] != '\0') {
            if (starType[i] == 'I') {
                if (starType[i + 1] == 'I') {
                    if (starType[i + 2] == 'I') {
                        lum = StellarClass::Lum_III;
                    } else {
                        lum = StellarClass::Lum_II;
                    }
                } else if (starType[i + 1] == 'V') {
                    lum = StellarClass::Lum_IV;
                } else if (starType[i + 1] == 'a') {
                    if (starType[i + 2] == '0')
                        lum = StellarClass::Lum_Ia0;
                    else
                        lum = StellarClass::Lum_Ia;
                } else if (starType[i + 1] == 'b') {
                    lum = StellarClass::Lum_Ib;
                } else {
                    lum = StellarClass::Lum_Ib;
                }
                break;
            } else if (starType[i] == 'V') {
                if (starType[i + 1] == 'I')
                    lum = StellarClass::Lum_VI;
                else
                    lum = StellarClass::Lum_V;
                break;
            }
            i++;
        }
    }

    return StellarClass(type, specClass, number, lum);
}


HipparcosStar TheSun()
{
    HipparcosStar star;

    star.HDCatalogNumber = 0;
    star.ascension = 0.0f;
    star.declination = 0.0f;
    star.parallax = 1000000.0f;
    star.appMag = -15.17f;
    star.stellarClass = StellarClass(StellarClass::NormalStar,
                                     StellarClass::Spectral_G, 2,
                                     StellarClass::Lum_V);

    return star;
}


bool ReadStarRecord(istream& in)
{
    HipparcosStar star;
    char buf[HipStarRecordLength];

    in.read(buf, HipStarRecordLength);
    
    if (sscanf(buf + 2, "%d", &star.HIPCatalogNumber) != 1)
    {
        cout << "Error reading catalog number.\n";
        return false;
    }

    sscanf(buf + 390, "%d", &star.HDCatalogNumber);

    if (sscanf(buf + 41, "%f", &star.appMag) != 1)
    {
        cout << "Error reading magnitude.\n";
        return false;
    }

    if (sscanf(buf + 79, "%f", &star.parallax) != 1)
    {
        // cout << "Error reading parallax.\n";
    }

    int hh = 0;
    int mm = 0;
    float seconds;
    if (sscanf(buf + 17, "%d %d %f", &hh, &mm, &seconds) != 3)
    {
        cout << "Error reading ascension.\n";
        return false;
    }
    star.ascension = hh + (float) mm / 60.0f + (float) seconds / 3600.0f;
    
    char decSign;
    int deg;
    if (sscanf(buf + 29, "%c%d %d %f", &decSign, &deg, &mm, &seconds) != 4)
    {
        cout << "Error reading declination.\n";
        return false;
    }
    star.declination = deg + (float) mm / 60.0f + (float) seconds / 3600.0f;
    if (decSign == '-')
        star.declination = -star.declination;

    char* spectralType = buf + 435;
    spectralType[12] = '\0';
    star.stellarClass = ParseStellarClass(spectralType);

    int asc = 0;
    int dec = 0;
    if (sscanf(buf + 327, "%d%c%d", &asc, &decSign, &dec) == 3)
    {
        if (decSign == '-')
            dec = -dec;
        star.CCDMIdentifier = asc << 16 | (dec & 0xffff);

        int n = 1;
        sscanf(buf + 340, "%d", &n);
        star.starsWithCCDM = (uint8) n;
    }

    float parallaxError = 0.0f;
    if (sscanf(buf + 119, "%f", &parallaxError) != 0)
    {
        if (star.parallax < 0.0f || parallaxError / star.parallax > 1.0f)
            star.parallaxError = (int8) 255;
        else
            star.parallaxError = (int8) (parallaxError / star.parallax * 200);
    }
    
    
    stars.insert(stars.end(), star);

    return true;
}


void BuildMultistarSystemCatalog()
{
    for (vector<HipparcosStar>::iterator iter = stars.begin();
         iter != stars.end(); iter++)
    {
        if (iter->CCDMIdentifier != NullCCDMIdentifier)
        {
            MultistarSystemCatalog::iterator it =
                starSystems.find(iter->CCDMIdentifier);
            if (it == starSystems.end())
            {
                MultistarSystem* multiSystem = new MultistarSystem();
                multiSystem->nStars = 1;
                multiSystem->stars[0] = iter;
                starSystems.insert(MultistarSystemCatalog::value_type(iter->CCDMIdentifier, multiSystem));
            }
            else
            {
                MultistarSystem* multiSystem = it->second;
                if (multiSystem->nStars == 4)
                {
                    cout << "Number of stars in system exceeds 4\n";
                }
                else
                {
                    multiSystem->stars[multiSystem->nStars] = iter;
                    multiSystem->nStars++;
                }
            }
        }
    }
}


void CorrectComponentParallaxes()
{
    for (MultistarSystemCatalog::iterator iter = starSystems.begin();
         iter != starSystems.end(); iter++)
    {
        MultistarSystem* multiSystem = iter->second;
        if (multiSystem->nStars > 1)
        {
            for (int i = 1; i < multiSystem->nStars; i++)
                multiSystem->stars[i]->parallax = multiSystem->stars[0]->parallax;
        }
    }
}


int main(int argc, char* argv[])
{
    assert(sizeof(StellarClass) == 2);

    ifstream mainDatabase(MainDatabaseFile.c_str(), ios::in | ios::binary);
    if (!mainDatabase.good())
    {
        cout << "Error opening " << MainDatabaseFile << '\n';
        exit(1);
    }

    cout << "Reading HIPPARCOS data set.\n";
    while (mainDatabase.good())
    {
        ReadStarRecord(mainDatabase);
        if (stars.size() % 10000 == 0)
            cout << stars.size() << " records.\n";
    }

    cout << "Read " << stars.size() << " stars from main database.\n";

    stars.insert(stars.end(), TheSun());

    cout << "Building catalog of multiple star systems.\n";
    BuildMultistarSystemCatalog();
    
    int nMultipleSystems = starSystems.size();
    cout << "Stars in multiple star systems: " << nMultipleSystems << '\n';

    CorrectComponentParallaxes();

    char* outputFile = "stars.dat";
    if (argc > 1)
        outputFile = argv[1];

    cout << "Writing processed star records to " << outputFile << '\n';
    ofstream out(outputFile, ios::out | ios::binary);
    if (!out.good())
    {
        cout << "Error opening " << outputFile << '\n';
        exit(1);
    }

    binwrite(out, stars.size());
    for (vector<HipparcosStar>::iterator iter = stars.begin();
         iter != stars.end(); iter++)
    {
        iter->write(out);
    }

    return 0;
}
