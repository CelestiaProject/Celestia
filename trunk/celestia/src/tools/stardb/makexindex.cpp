// makexindex.cpp
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// Convert an ASCII cross index to binary

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <celutil/basictypes.h>
#include <celutil/bytes.h>

using namespace std;


static string inputFilename;
static string outputFilename;


void Usage()
{
    cerr << "Usage: makexindex [input file] [output file]\n";
}


bool parseCommandLine(int argc, char* argv[])
{
    int i = 1;
    int fileCount = 0;

    while (i < argc)
    {
        if (argv[i][0] == '-')
        {
            cerr << "Unknown command line switch: " << argv[i] << '\n';
            return false;
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


static void writeShort(ostream& out, int16 n)
{
    LE_TO_CPU_INT16(n, n);
    out.write(reinterpret_cast<char*>(&n), sizeof n);
}


bool WriteCrossIndex(istream& in, ostream& out)
{
    // Write the header
    out.write("CELINDEX", 8);

    // Write the version
    writeShort(out, 0x0100);
    
    unsigned int record = 0;
    while (!in.eof())
    {
        unsigned int catalogNumber;
        unsigned int celCatalogNumber;

        in >> catalogNumber;
        if (in.eof())
            return true;

        in >> celCatalogNumber;
        if (!in.good())
        {
            cerr << "Error parsing record #" << record << '\n';
            return false;
        }

        writeUint(out, (uint32) catalogNumber);
        writeUint(out, (uint32) celCatalogNumber);

        record++;
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

    istream* inputFile = &cin;
    if (!inputFilename.empty())
    {
        inputFile = new ifstream(inputFilename.c_str(), ios::in);
        if (!inputFile->good())
        {
            cerr << "Error opening input file " << inputFilename << '\n';
            return 1;
        }
    }

    ostream* outputFile = &cout;
    if (!outputFilename.empty())
    {
        outputFile = new ofstream(outputFilename.c_str(), ios::out | ios::binary);
        if (!outputFile->good())
        {
            cerr << "Error opening output file " << outputFilename << '\n';
            return 1;
        }
    }

    bool success = WriteCrossIndex(*inputFile, *outputFile);

    return success ? 0 : 1;
}
