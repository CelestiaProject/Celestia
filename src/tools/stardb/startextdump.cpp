// startextdump.cpp
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// Dump the contents of a Celestia star database file in a text
// format that's easy read and edit.

#include <iostream>
#include <fstream>
#include <iomanip>
#include <celutil/bytes.h>
#include <celengine/astro.h>
#include <celengine/stellarclass.h>

using namespace std;


static string inputFilename;
static string outputFilename;
static string hdFilename;
static bool useOldFormat = false;
static bool useSphericalCoords = false;


void Usage()
{
    cerr << "Usage: startextdump [options] <star database file> [output file]\n";
    cerr << "  Options:\n";
    cerr << "    --old (or -o)       : input star database is pre-2.0 format\n";
    cerr << "    --hd                : dump HD catalog cross reference\n";
    cerr << "    --spherical (or -s) : output spherical coordinates (RA/dec/distance\n";
}


static uint32_t readUint(istream& in)
{
    uint32_t n;
    in.read(reinterpret_cast<char*>(&n), sizeof n);
    LE_TO_CPU_INT32(n, n);
    return n;
}

static float readFloat(istream& in)
{
    float f;
    in.read(reinterpret_cast<char*>(&f), sizeof f);
    LE_TO_CPU_FLOAT(f, f);
    return f;
}

static int16_t readShort(istream& in)
{
    int16_t n;
    in.read(reinterpret_cast<char*>(&n), sizeof n);
    LE_TO_CPU_INT16(n, n);
    return n;
}

static uint16_t readUshort(istream& in)
{
    uint16_t n;
    in.read(reinterpret_cast<char*>(&n), sizeof n);
    LE_TO_CPU_INT16(n, n);
    return n;
}

static uint8_t readUbyte(istream& in)
{
    uint8_t n;
    in.read((char*) &n, 1);
    return n;
}


void printStellarClass(uint16_t sc, ostream& out)
{
    StellarClass::StarType st = (StellarClass::StarType) (sc >> 12);

    if (st == StellarClass::WhiteDwarf)
    {
        out << "WD";
    }
    else if (st == StellarClass::NeutronStar)
    {
        out << "Q";
    }
    else if (st == StellarClass::BlackHole)
    {
        out << "X";
    }
    else if (st == StellarClass::NormalStar)
    {
        unsigned int spectralClass = (sc >> 8 & 0xf);
        unsigned int spectralSubclass = (sc >> 4 & 0xf);
        unsigned int luminosityClass = (sc & 0xf);

        if (spectralClass == 12)
        {
            out << '?';
        }
        else
        {
            out << "OBAFGKMRSNWW?LTC"[spectralClass];
            out << "0123456789"[spectralSubclass];
            switch (luminosityClass)
            {
            case StellarClass::Lum_Ia0:
                out << "I-a0";
                break;
            case StellarClass::Lum_Ia:
                out << "I-a";
                break;
            case StellarClass::Lum_Ib:
                out << "I-b";
                break;
            case StellarClass::Lum_II:
                out << "II";
                break;
            case StellarClass::Lum_III:
                out << "III";
                break;
            case StellarClass::Lum_IV:
                out << "IV";
                break;
            case StellarClass::Lum_V:
                out << "V";
                break;
            case StellarClass::Lum_VI:
                out << "VI";
                break;
            }
        }
    }
    else
    {
        out << '?';
    }
}


bool DumpOldStarDatabase(istream& in, ostream& out, ofstream& hdOut,
                         bool spherical)
{
    uint32_t nStarsInFile = readUint(in);
    if (!in.good())
    {
        cerr << "Error reading count of stars from database.\n";
        return false;
    }

    out << nStarsInFile << '\n';

    for (uint32_t i = 0; i < nStarsInFile; i++)
    {
        if (!in.good())
        {
            cerr << "Error reading from star database at record " << i << endl;
            return false;
        }

        uint32_t catalogNum    = readUint(in);
        uint32_t HDCatalogNum  = readUint(in);
        float  RA            = readFloat(in);
        float  dec           = readFloat(in);
        float  parallax      = readFloat(in);
        int16_t  appMag        = readShort(in);
        uint16_t stellarClass  = readUshort(in);
        /*uint8_t  parallaxError = */readUbyte(in); // not used yet

        // Compute distance based on parallax
        double distance = LY_PER_PARSEC / (parallax > 0.0 ? parallax / 1000.0 : 1e-6);
        out << catalogNum << ' ';
        out << setprecision(8);

        if (spherical)
        {
            out << RA * 360.0 / 24.0 << ' ' << dec << ' ' << distance << ' ';
            out << setprecision(6);
            out << (float) appMag / 256.0f << ' ';
        }
        else
        {
            Eigen::Vector3d pos = astro::equatorialToCelestialCart((double) RA, (double) dec, distance);
            float absMag = (float) (appMag / 256.0 + 5 -
                                    5 * log10(distance / LY_PER_PARSEC));
            out << (float) pos.x() << ' ' <<
                   (float) pos.y() << ' ' <<
                   (float) pos.z() << ' ';
            out << setprecision(5);
            out << absMag << ' ';
        }
        printStellarClass(stellarClass, out);
        out << '\n';

        // Dump HD catalog cross reference
        if (hdOut.is_open() && HDCatalogNum != ~0u)
            hdOut << HDCatalogNum << ' ' << catalogNum << '\n';
    }

    return true;
}


bool DumpStarDatabase(istream& in, ostream& out, bool spherical)
{
    char header[8];
    in.read(header, sizeof header);
    if (strncmp(header, "CELSTARS", sizeof header))
    {
        cerr << "Missing header in star database.\n";
        return false;
    }

    uint16_t version = readUshort(in);
    if (version != 0x0100)
    {
        cerr << "Unsupported file version " << (version >> 8) << '.' <<
            (version & 0xff) << '\n';
        return false;
    }

    uint32_t nStarsInFile = readUint(in);
    if (!in.good())
    {
        cerr << "Error reading count of stars from database.\n";
        return false;
    }

    out << nStarsInFile << '\n';

    for (uint32_t i = 0; i < nStarsInFile; i++)
    {
        if (!in.good())
        {
            cerr << "Error reading from star database at record " << i << endl;
            return false;
        }

        uint32_t catalogNum   = readUint(in);
        float x               = readFloat(in);
        float y               = readFloat(in);
        float z               = readFloat(in);
        int16_t  absMag       = readShort(in);
        uint16_t stellarClass = readUshort(in);

        out << catalogNum << ' ';
        out << setprecision(7);

        if (spherical)
        {
            Eigen::Vector3d eclipticpos = {(double) x, (double) y, (double) z};
            Eigen::Vector3d pos = astro::eclipticToEquatorial(eclipticpos);
            double distance = sqrt(x * x + y * y + z * z);
            // acos outputs angles in interval [0, pi], use negative sign for interval [-pi, 0]
            double phi = -acos(pos.y() / distance) * 180 / PI;
            double theta = atan2(pos.z(), -pos.x()) * 180 / PI;
            // atan2 outputs angles in interval [-pi, pi], so we add 360 to fix this
            double ra = theta - 180 + 360;
            double dec = phi + 90;
            float appMag = float (absMag / 256.0 - 5 + 5 * log10(distance / LY_PER_PARSEC));

            out << fixed << setprecision(9) << (float) ra << ' ' << (float) dec << ' ';
            out << setprecision(6) << (float) distance << ' ';
            out << setprecision(2);
            out << appMag << ' ';
        }
        else
        {
            out << x << ' ' << y << ' ' << z << ' ';
            out << setprecision(4);
            out << ((float) absMag / 256.0f) << ' ';
        }

        printStellarClass(stellarClass, out);
        out << '\n';
    }

    return true;
}


bool parseCommandLine(int argc, char* argv[])
{
    int i = 1;
    int fileCount = 0;

    while (i < argc)
    {
        if (argv[i][0] == '-')
        {
            if (!strcmp(argv[i], "--hd"))
            {
                if (i == argc - 1)
                {
                    return false;
                }
                else
                {
                    i++;
                    hdFilename = string(argv[i]);
                }
            }
            else if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--old"))
            {
                useOldFormat = true;
            }
            else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--spherical"))
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


int main(int argc, char* argv[])
{
    if (!parseCommandLine(argc, argv) || inputFilename.empty())
    {
        Usage();
        return 1;
    }

    ifstream stardbFile(inputFilename, ios::in | ios::binary);
    if (!stardbFile.good())
    {
        cerr << "Error opening star database file " << inputFilename << '\n';
        return 1;
    }

    ofstream hdOut;
    if (useOldFormat && !hdFilename.empty())
    {
        hdOut.open(hdFilename, ios::out);
        if (!hdOut.good())
        {
            cerr << "Error opening HD catalog output file " << hdFilename << '\n';
            return 1;
        }
    }

    bool success;
    ostream* out = &cout;
    ofstream fout;
    if (!outputFilename.empty())
    {
        fout.open(outputFilename, ios::out);
        if (!fout.good())
        {
            cerr << "Error opening output file " << outputFilename << '\n';
            return 1;
        }
	out = &fout;
    }

    if (useOldFormat)
    {
        success = DumpOldStarDatabase(stardbFile, *out, hdOut,
                                      useSphericalCoords);
    }
    else
    {
        success = DumpStarDatabase(stardbFile, *out, useSphericalCoords);
    }

    return success ? 0 : 1;
}
