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
#include "stardb.h"

using namespace std;


static string MainDatabaseFile("hip_main.dat");
static string ComponentDatabaseFile("h_dm_com.dat");
static string OrbitalDatabase("hip_dm_o.dat");

static const int HipStarRecordLength = 451;
static const int HipComponentRecordLength = 239;

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
    uint8 nComponents;
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
    nComponents(1),
    parallaxError(0)
{
}

template<class T> void binwrite(ostream& out, T x)
{
    out.write(reinterpret_cast<char*>(&x), sizeof(T));
}

void HipparcosStar::write(ostream& out)
{
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
    }

    char* spectralType = buf + 435;
    spectralType[12] = '\0';
    star.stellarClass = ParseStellarClass(spectralType);

    int asc = 0;
    int dec = 0;
    char decSign = ' ';
    if (sscanf(buf + 327, "%d%c%d", &asc, &decSign, &dec) == 3)
    {
        if (decSign == '-')
            dec = -dec;
        star.CCDMIdentifier = asc << 16 | (dec & 0xffff);

        int n = 1;
        sscanf(buf + 340, "%d", &n);
        star.starsWithCCDM = (uint8) n;
        sscanf(buf + 343, "%d", &n);
        star.nComponents = (uint8) n;
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
        sscanf(buf + 69, "%f", &component.vMag) != 1)
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


StellarClass guessSpectralType(float colorIndex, float absMag)
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


int main(int argc, char* argv[])
{
    assert(sizeof(StellarClass) == 2);

    // Read star records from the primary HIPPARCOS catalog
    {
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
    }
    cout << "Read " << stars.size() << " stars from main database.\n";

    cout << "Adding the Sun...\n";
    stars.insert(stars.end(), TheSun());

    cout << "Sorting stars...\n";
    {
        starIndex.reserve(stars.size());
        for (vector<HipparcosStar>::iterator iter = stars.begin();
             iter != stars.end(); iter++)
        {
            starIndex.insert(starIndex.end(), iter);
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
        if (!componentDatabase.good())
        {
            cout << "Error opening " << ComponentDatabaseFile << '\n';
            exit(1);
        }

        cout << "Reading HIPPARCOS component database.\n";
        while (componentDatabase.good())
        {
            ReadComponentRecord(componentDatabase);
        }
    }
    cout << "Read " << components.size() << " components.\n";

    {    
        int aComp = 0, bComp = 0, cComp = 0, dComp = 0, eComp = 0, otherComp = 0;
        int bvComp = 0;
        for (int i = 0; i < components.size(); i++)
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
        
        cout << "A:" << aComp << "  B:" << bComp << "  C:" << cComp << "  D:" << dComp << "  E:" << eComp << '\n';
        cout << "Components with B-V mag: " << bvComp << '\n';
    }

    cout << "Building catalog of multiple star systems.\n";
    BuildMultistarSystemCatalog();
    
    int nMultipleSystems = starSystems.size();
    cout << "Stars in multiple star systems: " << nMultipleSystems << '\n';

    ConstrainComponentParallaxes();

    CorrectErrors();

    // CreateCompanionList();
    cout << "Companion stars: " << companions.size() << '\n';
    cout << "Total stars: " << stars.size() + companions.size() << '\n';

    ShowStarsWithComponents();

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

    binwrite(out, stars.size() + companions.size());
    {
        vector<HipparcosStar>::iterator iter;
        for (iter = stars.begin(); iter != stars.end(); iter++)
            iter->write(out);
        for (iter = companions.begin(); iter != companions.end(); iter++)
            iter->write(out);
    }

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
