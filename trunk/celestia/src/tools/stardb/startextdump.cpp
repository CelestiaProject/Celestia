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
#include <celutil/basictypes.h>
#include <celutil/bytes.h>
#include <celengine/astro.h>
#include <celengine/stellarclass.h>

using namespace std;


void Usage()
{
    cerr << "Usage: startextdump [options] <star database file> [output file]\n";
}


static uint32 readUint(istream& in)
{
    uint32 n;
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

static int16 readShort(istream& in)
{
    int16 n;
    in.read(reinterpret_cast<char*>(&n), sizeof n);
    LE_TO_CPU_INT16(n, n);
    return n;
}

static uint16 readUshort(istream& in)
{
    uint16 n;
    in.read(reinterpret_cast<char*>(&n), sizeof n);
    LE_TO_CPU_INT16(n, n);
    return n;
}

static uint8 readUbyte(istream& in)
{
    uint8 n;
    in.read((char*) &n, 1);
    return n;
}


void printStellarClass(uint16 sc, ostream& out)
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
    else
    {
        out << '?';
    }
}


bool DumpStarDatabase(istream& in, ostream& out)
{
    uint32 nStarsInFile = readUint(in);
    if (!in.good())
    {
        cerr << "Error reading count of stars from database.\n";
        return false;
    }

    for (uint32 i = 0; i < nStarsInFile; i++)
    {
        if (!in.good())
        {
            cerr << "Error reading from star database at record " << i << endl;
            return false;
        }

        uint32 catalogNum    = readUint(in);
        uint32 HDCatalogNum  = readUint(in);
        float  RA            = readFloat(in);
        float  dec           = readFloat(in);
        float  parallax      = readFloat(in);
        int16  appMag        = readShort(in);
        uint16 stellarClass  = readUshort(in);
        uint8  parallaxError = readUbyte(in);

	// Compute distance based on parallax
	double distance = LY_PER_PARSEC / (parallax > 0.0 ? parallax / 1000.0 : 1e-6);
        Point3d pos = astro::equatorialToCelestialCart((double) RA, (double) dec, distance);
	float absMag = (float) (appMag / 256.0 + 5 -
                                5 * log10(distance / 3.26));
        out << catalogNum << ' ';
        out << setprecision(7);
        out << (float) pos.x << ' ' << (float) pos.y << ' ' << (float) pos.z << ' ';
        out << setprecision(4);
        out << absMag << ' ';
        printStellarClass(stellarClass, out);
        out << '\n';
    }

    return true;
}


int main(int argc, char* argv[])
{
    if (argc < 2 || argc > 3)
    {
        Usage();
        return 1;
    }

    ifstream stardbFile(argv[1], ios::in | ios::binary);
    if (!stardbFile.good())
    {
        cerr << "Error opening star database file " << argv[1] << '\n';
        return 1;
    }

    bool success;
    if (argc > 2)
    {
        ofstream out(argv[2], ios::out);
        if (!out.good())
        {
            cerr << "Error opening output file " << argv[2] << '\n';
            return 1;
        }

        success = DumpStarDatabase(stardbFile, out);
    }
    else
    {
        success = DumpStarDatabase(stardbFile, cout);
    }

    return success ? 0 : 1;
}
