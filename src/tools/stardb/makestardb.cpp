// makestardb.cpp
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// Convert a file with ASCII star records to a Celestia star database

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cctype>
#include <cassert>
#include <celutil/basictypes.h>
#include <celutil/bytes.h>
#include <celengine/astro.h>
#include <celengine/star.h>

using namespace std;


static string inputFilename;
static string outputFilename;
static bool useSphericalCoords = false;


void Usage()
{
    cerr << "Usage: makestardb [options] <input file> <output star database>\n";
    cerr << "  Options:\n";
    cerr << "    --spherical (or -s) : input file has spherical coords (RA/dec/distance\n";
}


bool parseCommandLine(int argc, char* argv[])
{
    int i = 1;
    int fileCount = 0;

    while (i < argc)
    {
        if (argv[i][0] == '-')
        {
            if (!strcmp(argv[i], "--spherical") || !strcmp(argv[i], "-s"))
            {
                useSphericalCoords = true;
            }
            else
            {
                cerr << "Unknown command line switch: " << argv[i] << '\n';
                return false;
            }
            i++;
        }
        else
        {
            if (fileCount == 0)
            {
                // input filename first
                inputFilename = string(argv[i]);
                fileCount++;
            }
            else if (fileCount == 1)
            {
                // output filename second
                outputFilename = string(argv[i]);
                fileCount++;
            }
            else
            {
                // more than two filenames on the command line is an error
                return false;
            }
            i++;
        }
    }

    return true;
}


static void writeUint(ostream& out, uint32 n)
{
    LE_TO_CPU_INT32(n, n);
    out.write(reinterpret_cast<char*>(&n), sizeof n);
}

static void writeFloat(ostream& out, float f)
{
    LE_TO_CPU_FLOAT(f, f);
    out.write(reinterpret_cast<char*>(&f), sizeof f);
}

static void writeUshort(ostream& out, uint16 n)
{
    LE_TO_CPU_INT16(n, n);
    out.write(reinterpret_cast<char*>(&n), sizeof n);
}

static void writeShort(ostream& out, int16 n)
{
    LE_TO_CPU_INT16(n, n);
    out.write(reinterpret_cast<char*>(&n), sizeof n);
}


// The following code implements a state machine for parsing spectral
// types.  It is a very forgiving parser, returning unknown for any of the
// spectral type fields it can't find, and silently ignoring any extra
// characters in the spectral type.  The parser is written this way because
// the spectral type strings from the Hipparcos catalog are quite irregular.
enum ParseState
{
    BeginState,
    EndState,
    NormalStarState,
    WolfRayetTypeState,
    NormalStarClassState,
    NormalStarSubclassState,
    NormalStarSubclassDecimalState,
    NormalStarSubclassFinalState,
    LumClassBeginState,
    LumClassIState,
    LumClassIIState,
    LumClassVState,
    LumClassIdashState,
    LumClassIaState,
    WDTypeState,
    WDExtendedTypeState,
    WDSubclassState,
    SubdwarfPrefixState,
};

static StellarClass
parseSpectralType(const string& st)
{
    uint32 i = 0;
    ParseState state = BeginState;
    StellarClass::StarType starType = StellarClass::NormalStar;
    StellarClass::SpectralClass specClass = StellarClass::Spectral_Unknown;
    StellarClass::LuminosityClass lumClass = StellarClass::Lum_Unknown;
    unsigned int subclass = StellarClass::Subclass_Unknown;

    while (state != EndState)
    {
        char c;
        if (i < st.length())
            c = st[i];
        else
            c = '\0';

        switch (state)
        {
        case BeginState:
            switch (c)
            {
            case 'Q':
                starType = StellarClass::NeutronStar;
                state = EndState;
                break;
            case 'X':
                starType = StellarClass::BlackHole;
                state = EndState;
                break;
            case 'D':
                starType = StellarClass::WhiteDwarf;
                specClass = StellarClass::Spectral_D;
                state = WDTypeState;
                i++;
                break;
            case 's':
                // Hipparcos uses sd prefix for stars with luminosity
                // class VI ('subdwarfs')
                state = SubdwarfPrefixState;
                i++;
                break;
            case '?':
                state = EndState;
                break;
            default:
                state = NormalStarClassState;
                break;
            }
            break;

        case WolfRayetTypeState:
            switch (c)
            {
            case 'C':
                specClass = StellarClass::Spectral_WC;
                state = NormalStarSubclassState;
                i++;
                break;
            case 'N':
                specClass = StellarClass::Spectral_WN;
                state = NormalStarSubclassState;
                i++;
                break;
            default:
                specClass = StellarClass::Spectral_WC;
                state = NormalStarSubclassState;
                break;
            }
            break;

        case SubdwarfPrefixState:
            if (c == 'd')
            {
                lumClass = StellarClass::Lum_VI;
                state = NormalStarClassState;
                i++;
                break;
            }
            else
            {
                state = EndState;
            }
            break;

        case NormalStarClassState:
            switch (c)
            {
            case 'W':
                state = WolfRayetTypeState;
                break;
            case 'O':
                specClass = StellarClass::Spectral_O;
                state = NormalStarSubclassState;
                break;               
            case 'B':
                specClass = StellarClass::Spectral_B;
                state = NormalStarSubclassState;
                break;               
            case 'A':
                specClass = StellarClass::Spectral_A;
                state = NormalStarSubclassState;
                break;
            case 'F':
                specClass = StellarClass::Spectral_F;
                state = NormalStarSubclassState;
                break;
            case 'G':
                specClass = StellarClass::Spectral_G;
                state = NormalStarSubclassState;
                break;
            case 'K':
                specClass = StellarClass::Spectral_K;
                state = NormalStarSubclassState;
                break;
            case 'M':
                specClass = StellarClass::Spectral_M;
                state = NormalStarSubclassState;
                break;
            case 'R':
                specClass = StellarClass::Spectral_R;
                state = NormalStarSubclassState;
                break;
            case 'S':
                specClass = StellarClass::Spectral_S;
                state = NormalStarSubclassState;
                break;
            case 'N':
                specClass = StellarClass::Spectral_N;
                state = NormalStarSubclassState;
                break;
            case 'L':
                specClass = StellarClass::Spectral_L;
                state = NormalStarSubclassState;
                break;
            case 'T':
                specClass = StellarClass::Spectral_T;
                state = NormalStarSubclassState;
                break;
            case 'C':
                specClass = StellarClass::Spectral_C;
                state = NormalStarSubclassState;
                break;
            default:
                state = EndState;
                break;
            }
            i++;
            break;

        case NormalStarSubclassState:
            if (isdigit(c))
            {
                subclass = (unsigned int) c - (unsigned int) '0';
                state = NormalStarSubclassDecimalState;
                i++;
            }
            else
            {
                state = LumClassBeginState;
            }
            break;

        case NormalStarSubclassDecimalState:
            if (c == '.')
            {
                state = NormalStarSubclassFinalState;
                i++;
            }
            else
            {
                state = LumClassBeginState;
            }
            break;

        case NormalStarSubclassFinalState:
            if (isdigit(c))
                state = LumClassBeginState;
            else
                state = EndState;
            i++;
            break;

        case LumClassBeginState:
            switch (c)
            {
            case 'I':
                state = LumClassIState;
                break;
            case 'V':
                state = LumClassVState;
                break;
            default:
                state = EndState;
                break;
            }
            i++;
            break;

        case LumClassIState:
            switch (c)
            {
            case 'I':
                state = LumClassIIState;
                break;
            case 'V':
                lumClass = StellarClass::Lum_IV;
                state = EndState;
                break;
            case 'a':
                state = LumClassIaState;
                break;
            case 'b':
                lumClass = StellarClass::Lum_Ib;
                state = EndState;
                break;
            case '-':
                state = LumClassIdashState;
                break;
            default:
                lumClass = StellarClass::Lum_Ib;
                state = EndState;
                break;
            }
            i++;
            break;

        case LumClassIIState:
            switch (c)
            {
            case 'I':
                lumClass = StellarClass::Lum_III;
                state = EndState;
                break;
            default:
                lumClass = StellarClass::Lum_II;
                state = EndState;
                break;
            }
            break;

        case LumClassIdashState:
            switch (c)
            {
            case 'a':
                state = LumClassIaState;
                break;
            case 'b':
                lumClass = StellarClass::Lum_Ib;
                state = EndState;
                break;
            default:
                lumClass = StellarClass::Lum_Ib;
                state = EndState;
                break;
            }
            break;

        case LumClassIaState:
            switch (c)
            {
            case '0':
                lumClass = StellarClass::Lum_Ia0;
                state = EndState;
                break;
            default:
                lumClass = StellarClass::Lum_Ia;
                state = EndState;
                break;
            }
            break;

        case LumClassVState:
            switch (c)
            {
            case 'I':
                lumClass = StellarClass::Lum_VI;
                state = EndState;
                break;
            default:
                lumClass = StellarClass::Lum_V;
                state = EndState;
                break;
            }
            break;

        case WDTypeState:
            switch (c)
            {
            case 'A':
                specClass = StellarClass::Spectral_DA;
                i++;
                break;
            case 'B':
                specClass = StellarClass::Spectral_DB;
                i++;
                break;
            case 'C':
                specClass = StellarClass::Spectral_DC;
                i++;
                break;
            case 'O':
                specClass = StellarClass::Spectral_DO;
                i++;
                break;
            case 'Q':
                specClass = StellarClass::Spectral_DQ;
                i++;
                break;
            case 'Z':
                specClass = StellarClass::Spectral_DZ;
                i++;
                break;
            default:
                specClass = StellarClass::Spectral_D;
                break;
            }
            state = WDExtendedTypeState;
            break;

        case WDExtendedTypeState:
            switch (c)
            {
            case 'A':
            case 'B':
            case 'C':
            case 'O':
            case 'Q':
            case 'Z':
            case 'V': // variable
            case 'P': // magnetic stars with polarized light
            case 'H': // magnetic stars without polarized light
            case 'E': // emission lines
                i++;
                break;
            default:
                state = WDSubclassState;
                break;
            }
            break;

        case WDSubclassState:
            if (isdigit(c))
            {
                subclass = (unsigned int) c - (unsigned int) '0';
                i++;
            }
            state = EndState;
            break;

        default:
            assert(0);
            state = EndState;
            break;
        }
    }

    return StellarClass(starType, specClass, subclass, lumClass);
}


bool WriteStarDatabase(istream& in, ostream& out, bool sphericalCoords)
{
    unsigned int record = 0;
    unsigned int nStarsInFile = 0;

    in >> nStarsInFile;
    if (!in.good())
    {
        cerr << "Error reading star count at beginning of input file.\n";
        return 1;
    }

    // Write the header
    out.write("CELSTARS", 8);

    // Write the version
    writeShort(out, 0x0100);

    writeUint(out, nStarsInFile);

    for (unsigned int record = 0; record < nStarsInFile; record++)
    {
        unsigned int catalogNumber;
        float x, y, z;
        float absMag;

        in >> catalogNumber;
        if (in.eof())
            return true;

        if (!in.good())
        {
            cerr << "Error parsing catalog number for record #" << record << '\n';
            return false;
        }

        if (sphericalCoords)
        {
            float RA, dec, distance;
            float appMag;

            in >> RA >> dec >> distance;
            if (!in.good())
            {
                cerr << "Error parsing position of star " << catalogNumber << '\n';
                return false;
            }

            in >> appMag;
            if (!in.good())
            {
                cerr << "Error parsing magnitude of star " << catalogNumber << '\n';
                return false;
            }

            Point3d pos =
                astro::equatorialToCelestialCart((double) RA * 24.0 / 360.0,
                                                 (double) dec,
                                                 (double) distance);
            x = (float) pos.x;
            y = (float) pos.y;
            z = (float) pos.z;
            absMag = (float) (appMag + 5 - 5 * log10(distance / 3.26));
        }
        else
        {
            in >> x >> y >> z;
            if (!in.good())
            {
                cerr << "Error parsing position of star " << catalogNumber << '\n';
                return false;
            }

            in >> absMag;
            if (!in.good())
            {
                cerr << "Error parsing magnitude of star " << catalogNumber << '\n';
                return false;
            }
        }

        string scString;
        in >> scString;
        StellarClass sc = parseSpectralType(scString);
#if 0
        StarDetails* details = StarDetails::GetStarDetails(sc);
        if (details == NULL)
        {
            cerr << "Error parsing spectral type of star " << catalogNumber << '\n';
            return false;
        }

        // For spectral type parser debugging . . .
        cout << scString << ' ' << details->getSpectralType() << '\n';
#endif

        writeUint(out, catalogNumber);
        writeFloat(out, x);
        writeFloat(out, y);
        writeFloat(out, z);
        writeShort(out, (int16) (absMag * 256.0f));
        writeUshort(out, sc.pack());
    }

    return true;
}


int main(int argc, char* argv[])
{
    if (!parseCommandLine(argc, argv) || inputFilename.empty())
    {
        Usage();
        return 1;
    }

    ifstream inputFile(inputFilename.c_str(), ios::in);
    if (!inputFile.good())
    {
        cerr << "Error opening input file " << inputFilename << '\n';
        return 1;
    }

    ofstream stardbFile(outputFilename.c_str(), ios::out | ios::binary);
    if (!stardbFile.good())
    {
        cerr << "Error opening star database file " << outputFilename << '\n';
        return 1;
    }

    bool success = WriteStarDatabase(inputFile, stardbFile, useSphericalCoords);

    return success ? 0 : 1;
}
