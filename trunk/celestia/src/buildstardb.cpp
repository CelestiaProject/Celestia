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
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <assert.h>
#include <unistd.h>
#include "celengine/stardb.h"

using namespace std;


#ifdef _WIN32
static string MainDatabaseFile("hip_main.dat");
static string TychoDatabaseFile("tyc_main.dat");
static string ComponentDatabaseFile("h_dm_com.dat");
static string OrbitalDatabase("hip_dm_o.dat");
#else
#ifndef MACOSX_PB
#include <config.h>
#endif /*MACOSX_PB*/
static string MainDatabaseFile(HIP_DATA_DIR "/hip_main.dat");
static string TychoDatabaseFile(HIP_DATA_DIR "/tyc_main.dat");
static string ComponentDatabaseFile(HIP_DATA_DIR "/h_dm_com.dat");
static string OrbitalDatabase(HIP_DATA_DIR "/hip_dm_o.dat");
#endif

static const int HipStarRecordLength = 451;
static const int HipComponentRecordLength = 239;
static const int TycStarRecordLength = 351;

static uint32 NullCCDMIdentifier = 0xffffffff;
static uint32 NullCatalogNumber = 0xffffffff;
static int verbose, dropstars;


class HipparcosStar
{
public:
    HipparcosStar();

    void write(ostream&);
    void analyze();

    uint32 HIPCatalogNumber;
    uint32 HDCatalogNumber;
    float ascension;
    float declination;
    float parallax;
    float appMag;
    StellarClass stellarClass;

    uint32 CCDMIdentifier;
    uint8 starsWithCCDM;
    uint8 nComponents;
    uint8 parallaxError;
    uint tycline;
    uint status;
    float e_RA;  // Errors in Right Ascension,
    float e_DE;  // Declination,
    float e_Mag; // and Magnitude
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
    nComponents(1),
    parallaxError(0),
    tycline(0),
    e_RA(0.0f),
    e_DE(0.0f),
    e_Mag(0.0f)
{
}

template<class T> void binwrite(ostream& out, T x)
{
    out.write(reinterpret_cast<char*>(&x), sizeof(T));
}

void HipparcosStar::write(ostream& out)
{
    if (status>=2)
        return;
#if 0
    if (HDCatalogNumber != NullCatalogNumber)
        binwrite(out, HDCatalogNumber);
    else
        binwrite(out, HIPCatalogNumber | 0x10000000);
#endif
    binwrite(out, HIPCatalogNumber);
    binwrite(out, HDCatalogNumber);
    binwrite(out, ascension);
    binwrite(out, declination);
    binwrite(out, parallax);
    binwrite(out, (short) (appMag * 256));
    binwrite(out, stellarClass);
    binwrite(out, parallaxError);
}

                    // Statistic Values:
float s_er;         // Sum of Error in RA
float s_erq;        // Sum of Squares of Errors in RA
unsigned int n_er;  // Number of Error Values
float s_ed, s_edq;  // Ditto for Declination
unsigned int n_ed;
unsigned int n_drop,n_dub; // number of stars to drop, number dubious

void HipparcosStar::analyze()
{
    int dubious=0;
    status=0;
    if ((parallax) <= 0.4 && ((dropstars==0) || (dropstars==1 && appMag<6.0)))
        parallax=0.4;  /* fix strange paralaxes so the stars aren't *way*
                          out there. A parallax of 0.4 will put them at
                          just a touch above 8154 LY away. */
    if (parallax <= 0.0)
        dubious+=400;
    else if (parallax<0.2)
        dubious+=4;
    else if (parallax<0.4)
        dubious+=2;
    if (parallax<=parallaxError)
        dubious+=2;
    else if (parallax<=(2*parallaxError))
        dubious++;
    if (e_RA<1000.0)
    {
        s_er+=e_RA;
        s_erq+=square(e_RA);
        n_er++;
        if (e_RA>20.0)
            dubious+=100;
        else if (e_RA>15.0)
            dubious+=2;
        else if (e_RA>10.0)
            dubious++;
    }
    else
        dubious+=4;  /* No error given, assume it's rather dubious */

    if (e_DE<1000.0)
    {
        s_ed+=e_DE;
        s_edq+=square(e_DE);
        n_ed++;
        if (e_DE>20.0)
            dubious+=100;
        else if (e_DE>15.0)
            dubious+=2;
        else if (e_DE>10.0)
            dubious++;
    }
    else
        dubious+=4;  /* No error given, assume it's rather dubious */

    if (dubious>3)
    {
        status=1;
        n_dub++;
    }
    if ((dubious>5) && (dropstars) && (!(dropstars==1 && appMag<6.0)))
    {
        status=2;
        n_drop++;
    }
}


bool operator<(const HipparcosStar& a, const HipparcosStar& b)
{
    return a.HIPCatalogNumber < b.HIPCatalogNumber;
}

struct HIPCatalogComparePredicate
{
    HIPCatalogComparePredicate() : dummy(0)
    {
    }

    bool operator()(const HipparcosStar* star0, const HipparcosStar* star1) const
    {
        return star0->HIPCatalogNumber < star1->HIPCatalogNumber;
    }

    bool operator()(const HipparcosStar* star0, uint32 hip)
    {
        return star0->HIPCatalogNumber < hip;
    }

    int dummy;
};


class MultistarSystem
{
public:
    int nStars; // Never greater than four in the HIPPARCOS catalog
    HipparcosStar* stars[4];
};


class HipparcosComponent
{
public:
    HipparcosComponent();

    HipparcosStar* star;
    char componentID;
    char refComponentID;
    float ascension;
    float declination;
    float appMag;
    float bMag;
    float vMag;
    bool hasBV;
    float positionAngle;
    float separation;
};

HipparcosComponent::HipparcosComponent() :
    star(NULL),
    componentID('A'),
    refComponentID('A'),
    appMag(0.0f),
    bMag(0.0f),
    vMag(0.0f),
    hasBV(false),
    positionAngle(0.0f),
    separation(0.0f)
{
}


vector<HipparcosStar> stars;
vector<HipparcosStar> companions;
vector<HipparcosComponent> components;
vector<HipparcosStar*> starIndex;

typedef map<uint32, MultistarSystem*> MultistarSystemCatalog;
MultistarSystemCatalog starSystems;


HipparcosStar* findStar(uint32 hip)
{
    HIPCatalogComparePredicate pred;

    vector<HipparcosStar*>::iterator iter = lower_bound(starIndex.begin(),
                                                        starIndex.end(),
                                                        hip, pred);
    if (iter == starIndex.end())
        return NULL;
    HipparcosStar* star = *iter;
    if (star->HIPCatalogNumber == hip)
        return star;
    else
        return NULL;
}


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
    star.HIPCatalogNumber = 0;
    star.ascension = 0.0f;
    star.declination = 0.0f;
    star.parallax = 1000000.0f;
    star.appMag = -15.17f;
    star.stellarClass = StellarClass(StellarClass::NormalStar,
                                     StellarClass::Spectral_G, 2,
                                     StellarClass::Lum_V);

    return star;
}


StellarClass guessSpectralType(float colorIndex, float /*absMag*/)
{
    StellarClass::SpectralClass specClass = StellarClass::Spectral_Unknown;
    float subclass = 0.0f;

    if (colorIndex < -0.25f)
    {
        specClass = StellarClass::Spectral_O;
        subclass = (colorIndex + 0.5f) / 0.25f;
    }
    else if (colorIndex < 0.0f)
    {
        specClass = StellarClass::Spectral_B;
        subclass = (colorIndex + 0.25f) / 0.25f;
    }
    else if (colorIndex < 0.25f)
    {
        specClass = StellarClass::Spectral_A;
        subclass = (colorIndex - 0.0f) / 0.25f;
    }
    else if (colorIndex < 0.6f)
    {
        specClass = StellarClass::Spectral_F;
        subclass = (colorIndex - 0.25f) / 0.35f;
    }
    else if (colorIndex < 0.85f)
    {
        specClass = StellarClass::Spectral_G;
        subclass = (colorIndex - 0.6f) / 0.25f;
    }
    else if (colorIndex < 1.4f)
    {
        specClass = StellarClass::Spectral_K;
        subclass = (colorIndex - 0.85f) / 0.55f;
    }
    else
    {
        specClass = StellarClass::Spectral_M;
        subclass = (colorIndex - 1.4f) / 1.0f;
    }

    if (subclass < 0.0f)
        subclass = 0.0f;
    else if (subclass > 1.0f)
        subclass = 1.0f;

    return StellarClass(StellarClass::NormalStar,
                        specClass,
                        (unsigned int) (subclass * 9.99f),
                        StellarClass::Lum_V);
}



static unsigned int okStars, lineno, changes, tested;

bool CheckStarRecord(istream& in)
{
    HipparcosStar star,*hipstar;
    char buf[HipStarRecordLength];
    bool ok=true;

    in.read(buf, TycStarRecordLength);
    lineno++;

    if (sscanf(buf + 210, "%d", &star.HIPCatalogNumber) != 1)
    {
        // Not in Hipparcos, skip it.
        if (verbose>1)
            cout << "Error reading HIPPARCOS catalog number.\n";
        return false;
    }

    if (verbose>1)
        cout << "Testing HIP " << star.HIPCatalogNumber << " from Line " << lineno << " ." << endl;
    if (!(hipstar=findStar(star.HIPCatalogNumber)))
    {
        cout << "Error finding HIP " << star.HIPCatalogNumber << " from Line " << lineno << " ." << endl;
        return false;
    }

    if (hipstar->tycline)
    {
        if (verbose>0)
            cout << "Duplicate Tycho Line for HIP " << star.HIPCatalogNumber << " from Line " << lineno << ", earlier Line at " << hipstar->tycline << " ." << endl;
    }
    else
        hipstar->tycline=lineno;

    tested++;
    sscanf(buf + 309, "%d", &star.HDCatalogNumber);

    if (sscanf(buf + 224, "%f", &star.e_Mag) != 1)
        /* Tycho Database gives no error in for VMag, so we use error on BTmag
           instead.*/
    {
            /* no standard Error given even in BTmag, give it a large value so
               CheckStarRecord() will use the Tycho value if possible */
            star.e_Mag = 1000.0f;
    }
    else if (star.e_Mag >1000.0)
    {
        cout << "Huge BTmag error for HIP " << star.HIPCatalogNumber << " from Line " << lineno << " ." << endl;
    }

    if (sscanf(buf + 41, "%f", &star.appMag) != 1)
    {
        if (verbose>0)
            cout << "Error reading magnitude for HIP " << star.HIPCatalogNumber << " from Line " << lineno << " ." << endl;
    }
    else if (star.e_Mag < hipstar->e_Mag)
    {
        hipstar->appMag=star.appMag;
        hipstar->e_Mag=star.e_Mag;
        changes++;
        if (verbose > 2)
            cout << "  Change Mag.\n";
    }

    float parallaxError = 0.0f;
    if (sscanf(buf + 119, "%f", &parallaxError) != 0)
    {
        if (star.parallax < 0.0f || parallaxError / star.parallax > 1.0f)
            star.parallaxError = 255u;
        else
            star.parallaxError = (uint8) (parallaxError / star.parallax * 200);
    }

    if (sscanf(buf + 79, "%f", &star.parallax) != 1)
    {
        if (verbose>0)
            cout << "Error reading parallax for HIP " << star.HIPCatalogNumber << " from Line " << lineno << " ." << endl;
        ok=false;
    }
    else if (star.parallax< 0.0)
    {
        if (hipstar->parallax< 0.0)
        {
            if (verbose>0)
                cout << "Negative parallax for HIP " << star.HIPCatalogNumber << " from Line " << lineno << " ." << endl;
        ok=false;
        }
    }
    else if (star.parallaxError < hipstar->parallaxError)
    {
        hipstar->parallax=star.parallax;
        hipstar->parallaxError=star.parallaxError;
        changes++;
        if (verbose > 2)
            cout << "  Change Parallax.\n";
    }


    if (sscanf(buf + 105, "%f", &star.e_RA) != 1)
    {
            /* no standard Error givenfor Right Ascension , give it a large
               value so original HIPPARCOS value will be used */
            if (verbose>0)
                cout << "No RA error for HIP " << star.HIPCatalogNumber << " from Line " << lineno << " ." << endl;
            star.e_RA = 2000.0f;
    }
    else if (star.e_RA >1000.0)
    {
        cout << "Huge RA error for HIP " << star.HIPCatalogNumber << " from Line " << lineno << " ." << endl;
    }
    if (sscanf(buf + 112, "%f", &star.e_DE) != 1)
    {
            /* no standard Error given for Declination, give it a large value
               so original HIPPARCOS value will be used. */
            if (verbose>0)
                cout << "No DE error for HIP " << star.HIPCatalogNumber << " from Line " << lineno << " ." << endl;
            star.e_DE = 2000.0f;
    }
    else if (star.e_DE >1000.0)
    {
        cout << "Huge DE error for HIP " << star.HIPCatalogNumber << " from Line " << lineno << " ." << endl;
    }

    bool coordReadError = false;
    if (sscanf(buf + 51, "%f", &star.ascension) != 1)
        coordReadError = true;
    if (sscanf(buf + 64, "%f", &star.declination) != 1)
        coordReadError = true;
    star.ascension = (float) (star.ascension * 24.0 / 360.0);

    // Read the lower resolution coordinates in hhmmss form if we failed
    // to read the coordinates in degrees.  Not sure why the high resolution
    // coordinates are occasionally missing . . .
    if (coordReadError)
    {
        int hh = 0;
        int mm = 0;
        float seconds;
        coordReadError=false;
        if (sscanf(buf + 17, "%d %d %f", &hh, &mm, &seconds) != 3)
        {
            cout << "Error reading ascension for HIP " << star.HIPCatalogNumber << " from Line " << lineno << " ." << endl;
            coordReadError=true;;
        }
        else
        {
            star.ascension = hh + (float) mm / 60.0f + (float) seconds / 3600.0f;

            char decSign;
            int deg;
            if (sscanf(buf + 29, "%c%d %d %f", &decSign, &deg, &mm, &seconds) != 4)
            {
                cout << "Error reading declination for HIP " << star.HIPCatalogNumber << " from Line " << lineno << " ." << endl;
                coordReadError=true;;
            }
            else
            {
                star.declination = deg + (float) mm / 60.0f + (float) seconds / 3600.0f;
                if (decSign == '-')
                    star.declination = -star.declination;
            }
        }
    }

    if (!((coordReadError) || ((star.ascension==hipstar->ascension) && (star.declination==hipstar->declination))))
    {
        float ast=star.e_RA * star.e_DE;
        float ahi=hipstar->e_RA * hipstar->e_DE;
        if ((ast<ahi) || ((ast==ahi) && ((star.e_RA + star.e_DE) < (hipstar->e_RA + hipstar->e_DE))))
        //if ((star.e_RA * star.e_DE) < (hipstar->e_RA * hipstar->e_DE))
        {
            // Error on the Tycho value is smaller, use it.
            hipstar->ascension=star.ascension;
            hipstar->declination=star.declination;
            hipstar->e_RA=star.e_RA;
            hipstar->e_DE=star.e_DE;
            changes++;
            if (verbose > 2)
                cout << "  Change Position.\n";
        }
    }

    int asc = 0;
    int dec = 0;
    char decSign = ' ';
    if (sscanf(buf + 299, "%d%c%d", &asc, &decSign, &dec) == 3)
    {
        if (decSign == '-')
            dec = -dec;
        star.CCDMIdentifier = asc << 16 | (dec & 0xffff);

        if (star.CCDMIdentifier != hipstar->CCDMIdentifier)
        {
            cout << "Diffrence in CCDM Identifier for HIP " << star.HIPCatalogNumber << " from Line " << lineno << " ." << endl;
        ok=false;
        }
    }

    if (ok)
        okStars++;

    return true;
}


bool ReadStarRecord(istream& in)
{
    HipparcosStar star;
    char buf[HipStarRecordLength];

    in.read(buf, HipStarRecordLength);

    if (sscanf(buf + 8, "%d", &star.HIPCatalogNumber) != 1)
    {
        cout << "Error reading catalog number.\n";
        return false;
    }

    sscanf(buf + 390, "%d", &star.HDCatalogNumber);
    star.tycline=0;

    if (sscanf(buf + 41, "%f", &star.appMag) != 1)
    {
        if (verbose>0)
            cout << "Error reading magnitude for HIP " << star.HIPCatalogNumber << " ." << endl;
        return false;
    }

    if (sscanf(buf + 79, "%f", &star.parallax) != 1)
    {
        if (verbose>0)
            cout << "Error reading parallax for HIP " << star.HIPCatalogNumber << " ." << endl;
        return false;
    }
    if (star.parallax< 0.0)
    {
        if (verbose>0)
            cout << "Negative parallax for HIP " << star.HIPCatalogNumber << " ." << endl;
    }

    bool coordReadError = false;
    if (sscanf(buf + 51, "%f", &star.ascension) != 1)
        coordReadError = true;
    if (sscanf(buf + 64, "%f", &star.declination) != 1)
        coordReadError = true;
    star.ascension = (float) (star.ascension * 24.0 / 360.0);

    // Read the lower resolution coordinates in hhmmss form if we failed
    // to read the coordinates in degrees.  Not sure why the high resolution
    // coordinates are occasionally missing . . .
    if (coordReadError)
    {
        int hh = 0;
        int mm = 0;
        float seconds;
        if (sscanf(buf + 17, "%d %d %f", &hh, &mm, &seconds) != 3)
        {
            cout << "Error reading ascension for HIP " << star.HIPCatalogNumber << " ." << endl;
            return false;
        }
        star.ascension = hh + (float) mm / 60.0f + (float) seconds / 3600.0f;

        char decSign;
        int deg;
        if (sscanf(buf + 29, "%c%d %d %f", &decSign, &deg, &mm, &seconds) != 4)
        {
            cout << "Error reading declination for HIP " << star.HIPCatalogNumber << " ." << endl;
            return false;
        }
        star.declination = deg + (float) mm / 60.0f + (float) seconds / 3600.0f;
        if (decSign == '-')
            star.declination = -star.declination;
    }

    int asc = 0;
    int dec = 0;
    char decSign = ' ';
    int n = 1;
    if (sscanf(buf + 327, "%d%c%d", &asc, &decSign, &dec) == 3)
    {
        if (decSign == '-')
            dec = -dec;
        star.CCDMIdentifier = asc << 16 | (dec & 0xffff);

        sscanf(buf + 340, "%d", &n);
        star.starsWithCCDM = (uint8) n;
        sscanf(buf + 343, "%d", &n);
        star.nComponents = (uint8) n;
    }

    char* spectralType = buf + 435;
    spectralType[12] = '\0';
    star.stellarClass = ParseStellarClass(spectralType);
    if (star.stellarClass.getSpectralClass() == StellarClass::Spectral_Unknown)
    {
        float bmag,vmag;
        if ((sscanf(buf + 217, "%f", &bmag) == 1) &&
            (sscanf(buf + 230, "%f", &vmag) == 1))
            {
            if (verbose>0)
                cout << "Guessing Type " << spectralType << "for HIP " << star.HIPCatalogNumber << " ." << endl;
            star.stellarClass = guessSpectralType(bmag - vmag, 0.0f);
            }
        else if (verbose>0)
            cout << "Unparsable stellar class " << spectralType << "for HIP " << star.HIPCatalogNumber << " ." << endl;
    }

    float parallaxError = 0.0f;
    if (sscanf(buf + 119, "%f", &parallaxError) != 0)
    {
        if (star.parallax < 0.0f || parallaxError / star.parallax > 1.0f)
            star.parallaxError = 255u;
        else
            star.parallaxError = (uint8) (parallaxError / star.parallax * 200);
    }

    if (sscanf(buf + 105, "%f", &star.e_RA) != 1)
    {
            /* no standard Error givenfor Right Ascension , give it a large
               value so CheckStarRecord() will use Tycho value if possible */
            star.e_RA = 1000.0f;
    }
    else if (star.e_RA >1000.0)
    {
        cout << "Huge RA error for HIP " << star.HIPCatalogNumber << " from Line " << lineno << " ." << endl;
    }
    if (sscanf(buf + 112, "%f", &star.e_DE) != 1)
    {
            /* no standard Error given for Declination, give it a large value
               so CheckStarRecord() will use the Tycho value if possible */
            star.e_DE = 1000.0f;
    }
    else if (star.e_DE >1000.0)
    {
        cout << "Huge DE error for HIP " << star.HIPCatalogNumber << " from Line " << lineno << " ." << endl;
    }

    if (sscanf(buf + 224, "%f", &star.e_Mag) != 1)
        // No error in HIPPARCOS for VMag, use error on BTmag instead.
    {
            /* no standard Error given give it a large value so CheckStarRecord() will use the Tycho value if possible */
            star.e_Mag = 1000.0f;
    }
    else if (star.e_Mag >1000.0)
    {
        cout << "Huge BTmag error for HIP " << star.HIPCatalogNumber << " from Line " << lineno << " ." << endl;
    }

    stars.insert(stars.end(), star);

    return true;
}


bool ReadComponentRecord(istream& in)
{
    HipparcosComponent component;
    char buf[HipComponentRecordLength];

    in.read(buf, HipComponentRecordLength);

    uint32 hip;
    if (sscanf(buf + 42, "%ud", &hip) != 1)
    {
        cout << "Missing HIP catalog number for component.\n";
        return false;
    }

    component.star = findStar(hip);
    if (component.star == NULL)
    {
        cout << "Nonexistent HIP catalog number for component.\n";
        return false;
    }

    if (sscanf(buf + 40, "%c", &component.componentID) != 1)
    {
        cout << "Missing component identifier.\n";
        return false;
    }

    if (sscanf(buf + 175, "%c", &component.refComponentID) != 1)
    {
        cout << "Error reading reference component.\n";
        return false;
    }
    if (component.refComponentID == ' ')
        component.refComponentID = component.componentID;

    // Read astrometric information
    if (sscanf(buf + 88, "%f", &component.ascension) != 1)
    {
        cout << "Missing ascension for component.\n";
        return false;
    }
    component.ascension = (float) (component.ascension * 24.0 / 360.0);

    if (sscanf(buf + 101, "%f", &component.declination) != 1)
    {
        cout << "Missing declination for component.\n";
        return false;
    }

    // Read photometric information
    if (sscanf(buf + 49, "%f", &component.appMag) != 1)
    {
        cout << "Missing magnitude for component.\n";
        return false;
    }

    // vMag and bMag may be necessary to guess the spectral type
    if (sscanf(buf + 62, "%f", &component.bMag) != 1 ||
        sscanf(buf + 75, "%f", &component.vMag) != 1)
    {
        component.bMag = component.vMag = component.appMag;
    }
    else
    {
        component.hasBV = true;
    }

    if (component.componentID != component.refComponentID)
    {
        if (sscanf(buf + 177, "%f", &component.positionAngle) != 1)
        {
            cout << "Missing position angle for component.\n";
            return false;
        }
        if (sscanf(buf + 185, "%f", &component.separation) != 1)
        {
            cout << "Missing separation for component.\n";
            return false;
        }
    }

    components.insert(components.end(), component);

    return true;
};


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
                multiSystem->stars[0] = &*iter;
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
                    multiSystem->stars[multiSystem->nStars] = &*iter;
                    multiSystem->nStars++;
                }
            }
        }
    }
}


void ConstrainComponentParallaxes()
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
#if 0
        if (multiSystem->nStars > 2)
        {
            cout << multiSystem->nStars << ": ";
            if (multiSystem->stars[0]->HDCatalogNumber != NullCatalogNumber)
                cout << "HD " << multiSystem->stars[0]->HDCatalogNumber;
            else
                cout << "HIP " << multiSystem->stars[0]->HIPCatalogNumber;
            cout << '\n';
        }
#endif
    }
}


void CorrectErrors()
{
    for (vector<HipparcosStar>::iterator iter = stars.begin();
         iter != stars.end(); iter++)
    {
        // Fix the spectral class of Capella, listed for some reason
        // as M1 in the database.
        if (iter->HDCatalogNumber == 34029)
        {
            iter->stellarClass = StellarClass(StellarClass::NormalStar,
                                              StellarClass::Spectral_G, 0,
                                              StellarClass::Lum_III);
        }
    }
}


// Process the vector of star components and insert those that are companions
// of stars in the primary database into the companions vector.
void CreateCompanionList()
{
    for (vector<HipparcosComponent>::iterator iter = components.begin();
         iter != components.end(); iter++)
    {
        // Don't insert the reference component, as this star should already
        // be in the primary database.
        if (iter->componentID != iter->refComponentID)
        {
            int componentNumber = iter->componentID - 'A';
            if (componentNumber > 0 && componentNumber < 8)
            {
                HipparcosStar star;

                star.HDCatalogNumber = NullCatalogNumber;
                star.HIPCatalogNumber = iter->star->HIPCatalogNumber |
                    (componentNumber << 25);

                star.ascension = iter->ascension;
                star.declination = iter->declination;
                star.parallax = iter->star->parallax;
                star.appMag = iter->appMag;
                if (iter->hasBV)
                    star.stellarClass = guessSpectralType(iter->bMag - iter->vMag, 0.0f);
                else
                    star.stellarClass = StellarClass(StellarClass::NormalStar,
                                                     StellarClass::Spectral_Unknown,
                                                     0, StellarClass::Lum_V);

                star.CCDMIdentifier = iter->star->CCDMIdentifier;
                star.parallaxError = iter->star->parallaxError;

                companions.insert(companions.end(), star);
            }
        }
    }
}


void ShowStarsWithComponents()
{
    cout << "\nStars with >2 components\n";
    for (vector<HipparcosStar>::iterator iter = stars.begin();
         iter != stars.end(); iter++)
    {
        if (iter->nComponents > 2)
        {
            cout << (int) iter->nComponents << ": ";
            if (iter->HDCatalogNumber != NullCatalogNumber)
                cout << "HD " << iter->HDCatalogNumber;
            else
                cout << "HIP " << iter->HIPCatalogNumber;
            cout << '\n';
        }
    }
}


void CompareTycho()
{
    ifstream tycDatabase(TychoDatabaseFile.c_str(), ios::in | ios::binary);
    if (!tycDatabase.is_open())
    {
        cout << "Error opening " << TychoDatabaseFile << '\n';
        cout << "You may download this file from ftp://cdsarc.u-strasbg.fr/cats/I/239/\n";
        return;
    }

    int recs=0;
    cout << "Comparing Tycho data set.\n";
    okStars=0;
    lineno=0;
    changes=0;
    while (tycDatabase.good())
    {
        CheckStarRecord(tycDatabase);
        if (++recs % 10000 == 0)
            {
            if (verbose>=0)
                cout << recs << " records.\n";
            else
                cout << ".";
                cout.flush();
            }
    }
    if (verbose<0)
        cout << "\n";
    else
        cout << recs  << " records checked, " << tested << " tested,  " << okStars << " checked out OK, and " << changes << " changes were made.\n";
}



int main(int argc, char* argv[])
{
    assert(sizeof(StellarClass) == 2);
    verbose=0;
    dropstars=1;
    int c;
    while((c=getopt(argc,argv,"v::qd:"))>-1)
    {
        if (c=='?')
        {
            cout << "Usage: buildstardb [-v[<verbosity_level>] [-q] [-d <drop-level>\n";
            exit(1);
        }
        else if (c=='v')
        {
            if (optarg)
                {
                verbose=(int)atol(optarg);
                if (verbose<-1)
                    verbose=-1;
                else if (verbose>3)
                    verbose=3;
                }
            else
                verbose=1;
        }
        else if (c=='d')
        {   /* Dropstar level. 0 = don't drop stars
                               1 = drop only non-naked eye visible stars
                               2 = drop all stars with strange values */
            dropstars=(int)atol(optarg);
            if (dropstars<0)
                dropstars=0;
            else if (dropstars>2)
                dropstars=2;
        }
        else if (c=='q')
        {
            verbose=-1;
        }
    }

    // Read star records from the primary HIPPARCOS catalog
    {
        ifstream mainDatabase(MainDatabaseFile.c_str(), ios::in | ios::binary);
        if (!mainDatabase.is_open())
        {
            cout << "Error opening " << MainDatabaseFile << '\n';
            cout << "You may download this file from ftp://cdsarc.u-strasbg.fr/cats/I/239/\n";
            exit(1);
        }

        cout << "Reading HIPPARCOS data set.\n";
        while (mainDatabase.good())
        {
            ReadStarRecord(mainDatabase);
            if (stars.size() % 10000 == 0)
                {
                if (verbose>=0)
                    cout << stars.size() << " records.\n";
                else
                    cout << ".";
                    cout.flush();
                }
        }
        if (verbose<0)
            cout << "\n";
    }
    if (verbose>=0)
    {
        cout << "Read " << stars.size() << " stars from main database.\n";
        cout << "Adding the Sun...\n";
    }
    stars.insert(stars.end(), TheSun());

    if (verbose>=0)
        cout << "Sorting stars...\n";
    {
        starIndex.reserve(stars.size());
        for (vector<HipparcosStar>::iterator iter = stars.begin();
             iter != stars.end(); iter++)
        {
            starIndex.insert(starIndex.end(), &*iter);
        }

        HIPCatalogComparePredicate pred;

        // It may not even be necessary to sort the records, if the
        // HIPPARCOS catalog is strictly ordered by catalog number.  I'm not
        // sure about this however,
        random_shuffle(starIndex.begin(), starIndex.end());
        sort(starIndex.begin(), starIndex.end(), pred);
    }

    // Read component records
    {
        ifstream componentDatabase(ComponentDatabaseFile.c_str(),
                                   ios::in | ios::binary);
        if (!componentDatabase.is_open())
        {
            cout << "Error opening " << ComponentDatabaseFile << '\n';
            cout << "You may download this file from ftp://cdsarc.u-strasbg.fr/cats/I/239/\n";
            exit(1);
        }

        if (verbose>=0)
            cout << "Reading HIPPARCOS component database.\n";
        while (componentDatabase.good())
        {
            ReadComponentRecord(componentDatabase);
        }
    }
    if (verbose>=0)
        cout << "Read " << components.size() << " components.\n";

    {
        int aComp = 0, bComp = 0, cComp = 0, dComp = 0, eComp = 0, otherComp = 0;
        int bvComp = 0;
        for (unsigned int i = 0; i < components.size(); i++)
        {
            switch (components[i].componentID)
            {
            case 'A':
                aComp++; break;
            case 'B':
                bComp++; break;
            case 'C':
                cComp++; break;
            case 'D':
                dComp++; break;
            case 'E':
                eComp++; break;
            default:
                otherComp++; break;
            }
            if (components[i].hasBV && components[i].componentID != 'A')
                bvComp++;
        }

        if (verbose>=0)
        {
            cout << "A:" << aComp << "  B:" << bComp << "  C:" << cComp << "  D:" << dComp << "  E:" << eComp << '\n';
            cout << "Components with B-V mag: " << bvComp << '\n';
        }
    }

    if (verbose>=0)
        cout << "Building catalog of multiple star systems.\n";
    BuildMultistarSystemCatalog();

    int nMultipleSystems = starSystems.size();
    if (verbose>=0)
        cout << "Stars in multiple star systems: " << nMultipleSystems << '\n';

    ConstrainComponentParallaxes();

    CorrectErrors();

    CompareTycho();

    // CreateCompanionList();
    if (verbose>=0)
    {
        cout << "Companion stars: " << companions.size() << '\n';
        cout << "Total stars: " << stars.size() + companions.size() << '\n';
    }

    if (verbose>0)
        ShowStarsWithComponents();

    const char* outputFile = "stars.dat";
    if (argv[optind])
        outputFile = argv[optind];

    cout << "Writing processed star records to " << outputFile << '\n';
    ofstream out(outputFile, ios::out | ios::binary);
    if (!out.good())
    {
        cout << "Error opening " << outputFile << '\n';
        exit(1);
    }

    s_er=0.0; // Zero the statistics values
    s_erq=0.0;
    n_er=0;
    s_er=0.0;
    s_erq=0.0;
    n_er=0;
    n_drop=0;
    n_dub=0;
    {
        vector<HipparcosStar>::iterator iter;
        for (iter = stars.begin(); iter != stars.end(); iter++)
            iter->analyze();
        for (iter = companions.begin(); iter != companions.end(); iter++)
            iter->analyze();
        binwrite(out, stars.size() + companions.size() - n_drop);
        float av_r,av_d; // average Right Ascension/Declination
        av_r=s_er/((float)n_er);
        av_d=s_ed/((float)n_ed);
        if (verbose>=0)
        {
            cout << "RA Error average: " << av_r << " with Standard Error: " << sqrt((s_erq+(square(s_er)/n_er) - (2*av_r*s_er))/(n_er-1)) << " .\n";
            cout << "DE Error average: " << av_d << " with Standard Error: " << sqrt((s_edq+(square(s_ed)/n_ed) - (2*av_d*s_ed))/(n_ed-1)) << " .\n";
        }
        for (iter = stars.begin(); iter != stars.end(); iter++)
            iter->write(out);
        for (iter = companions.begin(); iter != companions.end(); iter++)
            iter->write(out);
    }
    cout << "Stars processed: " << stars.size() + companions.size() << "   Number dropped: " << n_drop << "  number dubious: " << n_dub << " .\n";

#if 0
    char* hdOutputFile = "hdxref.dat";

    cout << "Writing out HD cross reference to " << hdOutputFile << '\n';
    ofstream hdout(hdOutputFile, ios::out | ios::binary);
    if (!out.good())
    {
        cout << "Error opening " << hdOutputFile << '\n';
        exit(1);
    }

    {
        int nHD = 0;
        vector<HipparcosStar>::iterator iter;
        for (iter = stars.begin(); iter != stars.end(); iter++)
        {
            if (iter->HDCatalogNumber != NullCatalogNumber)
                nHD++;
        }
        binwrite(hdout, nHD);

        cout << nHD << " stars have HD numbers.\n";

        for (iter = stars.begin(); iter != stars.end(); iter++)
        {
            if (iter->HDCatalogNumber != NullCatalogNumber)
            {
                binwrite(hdout, iter->HDCatalogNumber);
                binwrite(hdout, iter->HIPCatalogNumber);
            }
        }
    }
#endif

    return 0;
}
