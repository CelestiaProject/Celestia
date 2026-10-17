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

#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string_view>

#include <celutil/bytes.h>

using namespace std::string_view_literals;

namespace util = celestia::util;

namespace
{

struct Options
{
    std::filesystem::path inputFilename;
    std::filesystem::path outputFilename;
};

void
Usage()
{
    std::cerr << "Usage: makexindex [input file] [output file]\n";
}

std::optional<Options>
parseCommandLine(int argc, char* argv[])
{
    int i = 1;
    int fileCount = 0;

    auto result = std::make_optional<Options>();
    auto& options = *result;

    while (i < argc)
    {
        std::string_view arg(argv[i]);
        if (arg.starts_with('-'))
        {
            std::cerr << "Unknown command line switch: " << arg << '\n';
            return std::nullopt;
        }
        else
        {
            switch (fileCount)
            {
            case 0:
                // input filename first
                options.inputFilename = std::string(argv[i]);
                break;

            case 1:
                // output filename second
                options.outputFilename = std::string(argv[i]);
                break;

            default:
                // more than two filenames on the command line is an error
                return std::nullopt;
            }

            ++fileCount;
        }

        ++i;
    }

    return result;
}

bool
WriteCrossIndex(std::istream& in, std::ostream& out)
{
    struct Header
    {
        std::array<char, 8>     magic{ 'C', 'E', 'L', 'I', 'N', 'D', 'E', 'X' }; // cppcheck-suppress unusedStructMember
        util::LE<std::uint16_t> version{ 0x0100 };                               // cppcheck-suppress unusedStructMember
    };

    if (Header header; !out.write(reinterpret_cast<const char*>(&header), sizeof(header)))
    {
        std::cerr << "Failed to write cross index header.\n";
        return false;
    }

    std::uint32_t i = 0;
    for (;;)
    {
        struct Record
        {
            util::LE<std::uint32_t> catalogNumber;
            util::LE<std::uint32_t> celCatalogNumber;
        };

        Record record;

        if (std::uint32_t catalogNumber, celCatalogNumber; in >> catalogNumber >> celCatalogNumber)
        {
            record.catalogNumber = catalogNumber;
            record.celCatalogNumber = celCatalogNumber;
        }
        else if (in.eof())
        {
            break;
        }
        else
        {
            std::cerr << "Error parsing record #" << i << '\n';
            return false;
        }

        if (!out.write(reinterpret_cast<const char*>(&record), sizeof(record)))
        {
            std::cerr << "Failed to write record #" << i << '\n';
            return false;
        }

        ++i;
    }

    return true;
}

} // end unnamed namespace

int main(int argc, char* argv[])
{
    auto options = parseCommandLine(argc, argv);
    if (!options.has_value())
    {
        Usage();
        return EXIT_FAILURE;
    }

    std::istream* in = &std::cin;
    std::ifstream inputFile;
    if (!options->inputFilename.empty())
    {
        inputFile.open(options->inputFilename, std::ios::in);
        if (!inputFile)
        {
            std::cerr << "Error opening input file " << options->inputFilename << '\n';
            return EXIT_FAILURE;
        }

        in = &inputFile;
    }

    std::ostream* out = &std::cout;
    std::ofstream outputFile;
    if (!options->outputFilename.empty())
    {
        outputFile.open(options->outputFilename, std::ios::out | std::ios::binary);
        if (!outputFile)
        {
            std::cerr << "Error opening output file " << options->outputFilename << '\n';
            return EXIT_FAILURE;
        }

        out = &outputFile;
    }

    bool success = WriteCrossIndex(*in, *out);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
