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
#include <celutil/basictypes.h>
#include <celutil/bytes.h>
#include <celengine/astro.h>
#include <celengine/stellarclass.h>


using namespace std;


static string inputFilename;
static string outputFilename;


void Usage()
{
    cerr << "Usage: makestardb <input file> <output star database>\n";
}


bool parseCommandLine(int argc, char* argv[])
{
    int i = 1;
    int fileCount = 0;

    while (i < argc)
    {
        if (argv[i][0] == '-')
        {
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


bool WriteStarDatabase(istream& in, ostream& out)
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

        string scString;
        in >> scString;
        StellarClass sc = StellarClass::parse(scString);

        uint16 scData = (((uint16) sc.getStarType() << 12) |
                         ((uint16) sc.getSpectralClass() << 8) |
                         ((uint16) sc.getSpectralSubclass() << 4) |
                         ((uint16) sc.getLuminosityClass()));

        writeUint(out, catalogNumber);
        writeFloat(out, x);
        writeFloat(out, y);
        writeFloat(out, z);
        writeShort(out, (int16) (absMag * 256.0f));
        writeUshort(out, scData);
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

    bool success = WriteStarDatabase(inputFile, stardbFile);

    return success ? 0 : 1;
}
