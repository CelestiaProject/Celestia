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

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include <Eigen/Core>

#include <celastro/astro.h>
#include <celengine/stellarclass.h>
#include <celutil/bytes.h>

using namespace std::string_view_literals;

namespace astro = celestia::astro;
namespace util = celestia::util;

namespace
{

struct Options
{
    std::filesystem::path inputFilename;
    std::filesystem::path outputFilename;
    bool useSphericalCoords{ false };
};

void
Usage()
{
    std::cerr << "Usage: makestardb [options] <input file> <output star database>\n"
                 "  Options:\n"
                 "    --spherical (or -s) : input file has spherical coords (RA/dec/distance\n";
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
            if (arg == "-s"sv || arg == "--spherical"sv)
            {
                options.useSphericalCoords = true;
            }
            else
            {
                std::cerr << "Unknown command line switch: " << argv[i] << '\n';
                return std::nullopt;
            }
        }
        else
        {
            switch (fileCount)
            {
            case 0:
                // input filename first
                options.inputFilename = arg;
                break;

            case 1:
                // output filename second
                options.outputFilename = arg;
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
WriteStarDatabase(std::istream& in, std::ostream& out, bool sphericalCoords)
{
    struct Header
    {
        std::array<char, 8>     magic{ 'C', 'E', 'L', 'S', 'T', 'A', 'R', 'S' }; // cppcheck-suppress unusedStructMember
        util::LE<std::uint16_t> version{ UINT16_C(0x0100) };                     // cppcheck-suppress unusedStructMember
        util::LE<std::uint32_t> nStarsInFile;
    };

    Header header;

    std::uint32_t nStarsInFile;
    if (!(in >> nStarsInFile))
    {
        std::cerr << "Error reading star count at beginning of input file.\n";
        return false;
    }

    header.nStarsInFile = nStarsInFile;
    if (!out.write(reinterpret_cast<const char*>(&header), sizeof(header)))
    {
        std::cerr << "Failed to write star database header.\n";
        return false;
    }

    std::string scString;
    for (std::uint32_t i = 0; i < nStarsInFile; ++i)
    {
        struct Record
        {
            util::LE<std::uint32_t> catalogNumber;
            util::LE<float>         x;
            util::LE<float>         y;
            util::LE<float>         z;
            util::LE<std::int16_t>  absMag;
            util::LE<std::uint16_t> stellarClass;
        };

        Record record;

        unsigned int catalogNumber;
        if (in >> catalogNumber)
        {
            record.catalogNumber = catalogNumber;
        }
        else
        {
            std::cerr << "Error parsing catalog number for record #" << i << '\n';
            return false;
        }

        if (sphericalCoords)
        {
            float RA, dec, distance;
            float appMag;

            if (!(in >> RA >> dec >> distance))
            {
                std::cerr << "Error parsing position of star " << catalogNumber << '\n';
                return false;
            }

            if (!(in >> appMag))
            {
                std::cerr << "Error parsing magnitude of star " << catalogNumber << '\n';
                return false;
            }

            Eigen::Vector3d pos =
                astro::equatorialToCelestialCart(static_cast<double>(RA) * 24.0 / 360.0,
                                                 static_cast<double>(dec),
                                                 static_cast<double>(distance));
            record.x = static_cast<float>(pos.x());
            record.y = static_cast<float>(pos.y());
            record.z = static_cast<float>(pos.z());
            record.absMag = static_cast<float>(appMag + 5.0 - 5.0 * std::log10(distance / 3.26));
        }
        else
        {
            if (float x, y, z; in >> x >> y >> z)
            {
                record.x = x;
                record.y = y;
                record.z = z;
            }
            else
            {
                std::cerr << "Error parsing position of star " << catalogNumber << '\n';
                return false;
            }

            if (float absMag; in >> absMag)
            {
                record.absMag = static_cast<std::int16_t>(absMag * 256.0f);
            }
            else
            {
                std::cerr << "Error parsing magnitude of star " << catalogNumber << '\n';
                return false;
            }
        }

        scString.clear();
        in >> scString;
        record.stellarClass = StellarClass::parse(scString).packV1();

        if (!out.write(reinterpret_cast<const char*>(&record), sizeof(record)))
        {
            std::cerr << "Failed to write star data record for star " << catalogNumber << '\n';
            return false;
        }
    }

    return true;
}

} // end unnamed namespace

int main(int argc, char* argv[])
{
    auto options = parseCommandLine(argc, argv);
    if (!options.has_value() || options->inputFilename.empty())
    {
        Usage();
        return EXIT_FAILURE;
    }

    std::ifstream inputFile(options->inputFilename, std::ios::in);
    if (!inputFile)
    {
        std::cerr << "Error opening input file " << options->inputFilename << '\n';
        return EXIT_FAILURE;;
    }

    std::ofstream stardbFile(options->outputFilename, std::ios::out | std::ios::binary);
    if (!stardbFile)
    {
        std::cerr << "Error opening star database file " << options->outputFilename << '\n';
        return EXIT_FAILURE;
    }

    bool success = WriteStarDatabase(inputFile, stardbFile, options->useSphericalCoords);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
