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

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include <celutil/bytes.h>


static std::string inputFilename;
static std::string outputFilename;


void Usage()
{
    std::cerr << "Usage: makexindex [input file] [output file]\n";
}


bool parseCommandLine(int argc, char* argv[])
{
    int i = 1;
    int fileCount = 0;

    while (i < argc)
    {
        if (argv[i][0] == '-')
        {
            std::cerr << "Unknown command line switch: " << argv[i] << '\n';
            return false;
        }
        else
        {
            if (fileCount == 0)
            {
                // input filename first
                inputFilename = std::string(argv[i]);
                fileCount++;
            }
            else if (fileCount == 1)
            {
                // output filename second
                outputFilename = std::string(argv[i]);
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


static void writeUint(std::ostream& out, std::uint32_t n)
{
    LE_TO_CPU_INT32(n, n);
    out.write(reinterpret_cast<const char*>(&n), sizeof n);
}


static void writeShort(std::ostream& out, std::int16_t n)
{
    LE_TO_CPU_INT16(n, n);
    out.write(reinterpret_cast<const char*>(&n), sizeof n);
}


bool WriteCrossIndex(std::istream& in, std::ostream& out)
{
    // Write the header
    out.write("CELINDEX", 8);

    // Write the version
    writeShort(out, 0x0100);

    unsigned int record = 0;
    while (!in.eof())
    {
        std::uint32_t catalogNumber;
        std::uint32_t celCatalogNumber;

        in >> catalogNumber;
        if (in.eof())
            return true;

        in >> celCatalogNumber;
        if (!in.good())
        {
            std::cerr << "Error parsing record #" << record << '\n';
            return false;
        }

        writeUint(out, catalogNumber);
        writeUint(out, celCatalogNumber);

        record++;
    }

    return true;
}


int main(int argc, char* argv[])
{
    if (!parseCommandLine(argc, argv)/* || inputFilename.empty()*/)
    {
        Usage();
        return 1;
    }

    std::istream* inputFile = &std::cin;
    std::ifstream fin;
    if (!inputFilename.empty())
    {
        fin.open(inputFilename, std::ios::in);
        if (!fin.good())
        {
            std::cerr << "Error opening input file " << inputFilename << '\n';
            return 1;
        }
        inputFile = &fin;
    }

    std::ostream* outputFile = &std::cout;
    std::ofstream fout;
    if (!outputFilename.empty())
    {
        fout.open(outputFilename, std::ios::out | std::ios::binary);
        if (!fout.good())
        {
            std::cerr << "Error opening output file " << outputFilename << '\n';
            return 1;
        }
        outputFile = &fout;
    }

    bool success = WriteCrossIndex(*inputFile, *outputFile);

    return success ? 0 : 1;
}
