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

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <optional>
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
    std::filesystem::path hdFilename;
    bool useOldFormat{ false };
    bool useSphericalCoords{ false };
};

void
Usage()
{
    std::cerr << "Usage: startextdump [options] <star database file> [output file]\n"
                 "  Options:\n"
                 "    --old (or -o)       : input star database is pre-2.0 format\n"
                 "    --hd                : dump HD catalog cross reference\n"
                 "    --spherical (or -s) : output spherical coordinates (RA/dec/distance\n";
}

void
printStellarClass(std::uint16_t sc, std::ostream& out)
{
    switch (static_cast<StellarClass::StarType>(sc >> 12))
    {
    case StellarClass::NormalStar:
        {
            unsigned int spectralClass = (sc >> 8 & 0xf);
            unsigned int spectralSubclass = (sc >> 4 & 0xf);
            unsigned int luminosityClass = (sc & 0xf);
            if (spectralClass == '?')
            {
                out << '?';
                return;
            }

            constexpr auto spectralTypes = "OBAFGKMRSNWW?LTC"sv;
            out << spectralTypes[spectralClass];
            if (spectralClass == 10)
                out << 'C';
            else if (spectralClass == 11)
                out << 'N';

            if (spectralSubclass < 10)
                out << spectralSubclass;

            switch (luminosityClass)
            {
                case StellarClass::Lum_Ia0:
                    out << "Ia-0";
                    break;
                case StellarClass::Lum_Ia:
                    out << "Ia";
                    break;
                case StellarClass::Lum_Ib:
                    out << "Ib";
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
                    out << 'V';
                    break;
                case StellarClass::Lum_VI:
                    out << "VI";
                    break;
            }
        }
        break;

    case StellarClass::WhiteDwarf:
        {
            unsigned int spectralClass = (sc >> 8 & 0xf);
            unsigned int spectralSubclass = (sc >> 4 & 0xf);
            out << 'D';

            constexpr auto wdTypes = "ABCOQZ?X"sv;
            if (spectralClass < wdTypes.size() && spectralClass != 6)
                out << wdTypes[spectralClass];

            if (spectralSubclass < 10)
                out << spectralSubclass;
        }
        break;

    case StellarClass::NeutronStar:
        out << 'Q';
        break;

    case StellarClass::BlackHole:
        out << 'X';
        break;

    default:
        out << '?';
        break;
    }
}

bool
DumpOldStarDatabase(std::istream& in, std::ostream& out, std::ostream* hdOut,
                    bool spherical)
{
    std::uint32_t nStarsInFile;
    if (util::LE<std::uint32_t> data; in.read(reinterpret_cast<char*>(&data), sizeof(data))) /* Flawfinder: ignore */
    {
        nStarsInFile = data.get();
    }
    else
    {
        std::cerr << "Error reading count of stars from database.\n";
        return false;
    }

    out << nStarsInFile << '\n';

    for (std::uint32_t i = 0; i < nStarsInFile; ++i)
    {
        struct Record
        {
            util::LE<std::uint32_t> catalogNum;
            util::LE<std::uint32_t> hdCatalogNum;
            util::LE<float>         ra;
            util::LE<float>         dec;
            util::LE<float>         parallax;
            util::LE<std::int16_t>  appMag;
            util::LE<std::uint16_t> stellarClass;
            std::byte               parallaxError; // unused
        };

        Record record;
        if (!in.read(reinterpret_cast<char*>(&record), sizeof(record))) /* Flawfinder: ignore */
        {
            std::cerr << "Error reading from star database at record " << i << '\n';
            return false;
        }
        // Compute distance based on parallax
        const auto parallax = record.parallax.get();
        double distance = astro::LY_PER_PARSEC<double> / (parallax > 0.0 ? parallax / 1000.0 : 1e-6);
        const auto catalogNum = record.catalogNum.get();
        out << catalogNum << ' ';
        out << std::setprecision(8);

        if (spherical)
        {
            out << record.ra.get() * 360.0 / 24.0 << ' ' << record.dec.get() << ' ' << distance << ' ';
            out << std::setprecision(6);
            out << static_cast<float>(record.appMag.get()) / 256.0f << ' ';
        }
        else
        {
            Eigen::Vector3d pos = astro::equatorialToCelestialCart(static_cast<double>(record.ra.get()),
                                                                   static_cast<double>(record.dec.get()),
                                                                   distance);
            auto absMag = static_cast<float>(record.appMag.get() / 256.0 + 5.0 -
                                             5.0 * std::log10(distance / astro::LY_PER_PARSEC<double>));
            out << static_cast<float>(pos.x()) << ' ' <<
                   static_cast<float>(pos.y()) << ' ' <<
                   static_cast<float>(pos.z()) << ' ';
            out << std::setprecision(5);
            out << absMag << ' ';
        }
        printStellarClass(record.stellarClass, out);
        out << '\n';

        // Dump HD catalog cross reference
        if (hdOut)
        {
            const auto hdCatalogNum = record.hdCatalogNum.get();
            if (hdCatalogNum != ~UINT32_C(0))
                (*hdOut) << hdCatalogNum << ' ' << catalogNum << '\n';
        }
    }

    return true;
}

bool
DumpStarDatabase(std::istream& in, std::ostream& out, bool spherical)
{
    struct Header
    {
        std::array<char, 8>     magic;
        util::LE<std::uint16_t> version;
        util::LE<std::uint32_t> nStarsInFile;
    };

    Header header;
    if (!in.read(reinterpret_cast<char*>(&header), sizeof(header))) /* Flawfinder: ignore */
    {
        std::cerr << "Missing header in star database.\n";
        return false;
    }

    if (std::string_view(header.magic.data(), header.magic.size()) != "CELSTARS"sv)
    {
        std::cerr << "Not a star database file.\n";
        return false;
    }

    if (auto version = header.version.get(); version != UINT16_C(0x0100))
    {
        std::cerr << "Unsupported file version " << (version >> 8) << '.' <<
                     (version & 0xff) << '\n';
        return false;
    }

    auto nStarsInFile = header.nStarsInFile.get();

    out << nStarsInFile << '\n';
    for (std::uint32_t i = 0; i < nStarsInFile; ++i)
    {
        struct Record
        {
            util::LE<std::uint32_t> catalogNum;
            util::LE<float>         x;
            util::LE<float>         y;
            util::LE<float>         z;
            util::LE<std::int16_t>  absMag;
            util::LE<std::uint16_t> stellarClass;
        };

        Record record;
        if (!in.read(reinterpret_cast<char*>(&record), sizeof(record))) /* Flawfinder: ignore */
        {
            std::cerr << "Error reading from star database at record " << i << '\n';
            return false;
        }

        out << record.catalogNum.get() << ' ';
        out << std::setprecision(7);

        if (spherical)
        {
            Eigen::Vector3d eclipticpos{ record.x.get(), record.y.get(), record.z.get() };
            Eigen::Vector3d pos = astro::eclipticToEquatorial(eclipticpos);
            double distance = pos.norm();
            // acos outputs angles in interval [0, pi], use negative sign for interval [-pi, 0]
            double phi = -std::acos(std::clamp(pos.y() / distance, -1.0, 1.0)) * 180.0 / std::numbers::pi;
            double theta = std::atan2(pos.z(), -pos.x()) * 180.0 / std::numbers::pi;
            // atan2 outputs angles in interval [-pi, pi], so we add 360 to fix this
            double ra = theta - 180.0 + 360.0;
            double dec = phi + 90.0;
            auto appMag = static_cast<float>(record.absMag.get() / 256.0 - 5.0 + 5.0 * std::log10(distance / astro::LY_PER_PARSEC<double>));

            out << std::fixed << std::setprecision(9) << static_cast<float>(ra) << ' ' << static_cast<float>(dec) << ' ';
            out << std::setprecision(6) << static_cast<float>(distance) << ' ';
            out << std::setprecision(2);
            out << appMag << ' ';
        }
        else
        {
            out << record.x.get() << ' ' << record.y.get() << ' ' << record.z.get() << ' ';
            out << std::setprecision(4);
            out << (static_cast<float>(record.absMag.get()) / 256.0f) << ' ';
        }

        printStellarClass(record.stellarClass, out);
        out << '\n';
    }

    return true;
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
            if (arg == "--hd"sv)
            {
                if (i == argc - 1)
                    return std::nullopt;

                ++i;
                options.hdFilename = argv[i];
            }
            else if (arg == "-o"sv || arg == "--old"sv)
            {
                options.useOldFormat = true;
            }
            else if (arg == "-s"sv || arg == "--spherical"sv)
            {
                options.useSphericalCoords = true;
            }
            else
            {
                std::cerr << "Unknown command line switch: " << arg << '\n';
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

} // end unnamed namespace

int
main(int argc, char* argv[])
{
    auto options = parseCommandLine(argc, argv);
    if (!options.has_value() || options->inputFilename.empty())
    {
        Usage();
        return EXIT_FAILURE;
    }

    std::ifstream stardbFile(options->inputFilename, std::ios::in | std::ios::binary);
    if (!stardbFile)
    {
        std::cerr << "Error opening star database file " << options->inputFilename << '\n';
        return EXIT_FAILURE;
    }

    std::ostream* hdOut = nullptr;
    std::ofstream hdFile;
    if (options->useOldFormat && !options->hdFilename.empty())
    {
        hdFile.open(options->hdFilename, std::ios::out);
        if (!hdFile)
        {
            std::cerr << "Error opening HD catalog output file " << options->hdFilename << '\n';
            return EXIT_FAILURE;
        }

        hdOut = &hdFile;
    }

    std::ostream* out;
    std::ofstream fout;
    if (options->outputFilename.empty())
    {
        out = &std::cout;
    }
    else
    {
        fout.open(options->outputFilename, std::ios::out);
        if (!fout)
        {
            std::cerr << "Error opening output file " << options->outputFilename << '\n';
            return EXIT_FAILURE;
        }

        out = &fout;
    }

    bool success = options->useOldFormat
        ? DumpOldStarDatabase(stardbFile, *out, hdOut, options->useSphericalCoords)
        : DumpStarDatabase(stardbFile, *out, options->useSphericalCoords);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
