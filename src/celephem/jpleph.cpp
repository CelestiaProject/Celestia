// jpleph.cpp
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// Load JPL's DE200, DE405, and DE406 ephemerides and compute planet
// positions.

#include "jpleph.h"

#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <istream>
#include <type_traits>

#include <celcompat/byteswap.h>
#include <celutil/bytes.h>

namespace celestia::ephem
{

namespace
{

constexpr unsigned int NConstants         =  400;
constexpr unsigned int ConstantNameLength =  6;

constexpr unsigned int MaxChebyshevCoeffs = 32;

constexpr unsigned int LabelSize = 84;

constexpr unsigned int INPOP_DE_COMPATIBLE = 100;
constexpr unsigned int DE200 = 200;

template<std::endian E>
struct JPLECoeff
{
    util::EndianBytes<std::uint32_t, E> offset;
    util::EndianBytes<std::uint32_t, E> nCoeffs;
    util::EndianBytes<std::uint32_t, E> nGranules;
};

template<std::endian E>
struct JPLEFileHeader
{
    //  Three header labels
    std::array<std::array<char, LabelSize>, 3> headerLabels;

    // Constant names
    std::array<std::array<char, ConstantNameLength>, NConstants> constantNames;

    // Start time, end time, and time interval
    util::EndianBytes<double, E> startDate;
    util::EndianBytes<double, E> endDate;
    util::EndianBytes<double, E> daysPerInterval;

    // Number of constants with valid values
    util::EndianBytes<std::uint32_t, E> nConstants;

    // km per AU
    util::EndianBytes<double, E> au;
    // Earth-Moon mass ratio
    util::EndianBytes<double, E> earthMoonMassRatio;

    // Coefficient information for each item in the ephemeris
    std::array<JPLECoeff<E>, JPLEphemeris::JPLEph_NItems> coeffInfo;

    // DE number
    util::EndianBytes<std::uint32_t, E> deNum;

    // Libration coefficient information
    JPLECoeff<E> librationCoeffInfo;
};

static_assert(std::is_standard_layout_v<JPLEFileHeader<std::endian::native>>);

} // end unnamed namespace

// Return the position of an object relative to the solar system barycenter
// or the Earth (in the case of the Moon) at a specified TDB Julian date tjd.
// If tjd is outside the span covered by the ephemeris it is clamped to a
// valid time.
Eigen::Vector3d JPLEphemeris::getPlanetPosition(JPLEphemItem planet, double tjd) const
{
    // Solar system barycenter is the origin
    if (planet == JPLEphemItem::SSB)
    {
        return Eigen::Vector3d::Zero();
    }

    // The position of the Earth must be computed from the positions of the
    // Earth-Moon barycenter and Moon
    if (planet == JPLEphemItem::Earth)
    {
        Eigen::Vector3d embPos = getPlanetPosition(JPLEphemItem::EarthMoonBary, tjd);

        // Get the geocentric position of the Moon
        Eigen::Vector3d moonPos = getPlanetPosition(JPLEphemItem::Moon, tjd);

        return embPos - moonPos * (1.0 / (earthMoonMassRatio + 1.0));
    }

    // Clamp time to [ startDate, endDate ]
    if (tjd < startDate)
        tjd = startDate;
    else if (tjd > endDate)
        tjd = endDate;

    // recNo is always >= 0:
    auto recNo = static_cast<unsigned int>((tjd - startDate) / daysPerInterval);
    // Make sure we don't go past the end of the array if t == endDate
    if (recNo >= records.size())
        recNo = records.size() - 1;
    const JPLEphRecord& rec = records[recNo];

    auto planetIdx = static_cast<std::size_t>(planet);

    assert(coeffInfo[planetIdx].nGranules >= 1);
    assert(coeffInfo[planetIdx].nGranules <= 32);
    assert(coeffInfo[planetIdx].nCoeffs <= MaxChebyshevCoeffs);

    // u is the normalized time (in [-1, 1]) for interpolating
    // coeffs is a pointer to the Chebyshev coefficients
    double u = 0.0;
    const double* coeffs = nullptr;

    // nGranules is unsigned int so it will be compared against FFFFFFFF:
    if (coeffInfo[planetIdx].nGranules == static_cast<std::uint32_t>(-1))
    {
        coeffs = rec.coeffs.data() + coeffInfo[planetIdx].offset;
        u = 2.0 * (tjd - rec.t0) / daysPerInterval - 1.0;
    }
    else
    {
        double daysPerGranule = daysPerInterval / coeffInfo[planetIdx].nGranules;
        auto granule = static_cast<int>((tjd - rec.t0) / daysPerGranule);
        double granuleStartDate = rec.t0 + daysPerGranule * (double) granule;
        coeffs = rec.coeffs.data() + coeffInfo[planetIdx].offset +
                 granule * coeffInfo[planetIdx].nCoeffs * 3;
        u = 2.0 * (tjd - granuleStartDate) / daysPerGranule - 1.0;
    }

    // Evaluate the Chebyshev polynomials
    std::array<double, 3> sum;
    std::array<double, MaxChebyshevCoeffs> cc;
    unsigned int nCoeffs = coeffInfo[planetIdx].nCoeffs;
    for (int i = 0; i < 3; ++i)
    {
        cc[0] = 1.0;
        cc[1] = u;
        sum[i] = coeffs[i * nCoeffs] + coeffs[i * nCoeffs + 1] * u;
        for (unsigned int j = 2; j < nCoeffs; j++)
        {
            cc[j] = 2.0 * u * cc[j - 1] - cc[j - 2];
            sum[i] += coeffs[i * nCoeffs + j] * cc[j];
        }
    }

    return Eigen::Vector3d(sum[0], sum[1], sum[2]);
}

struct JPLEphemeris::Initializer
{
    template<std::endian E>
    static std::shared_ptr<const JPLEphemeris>
    initialize(std::istream& in)
    {
        JPLEFileHeader<E> header;
        if (!in.seekg(0, std::ios_base::beg) ||
            !in.read(reinterpret_cast<char*>(&header), sizeof(header))) /* Flawfinder: ignore */
        {
            return nullptr;
        }

        auto eph = std::make_shared<JPLEphemeris>(Token{});
        eph->swapBytes = E != std::endian::native;
        eph->DENum = header.deNum;

        // Read the start time, end time, and time interval
        eph->startDate          = header.startDate;
        eph->endDate            = header.endDate;
        if (eph->endDate <= eph->startDate)
            return nullptr;

        eph->daysPerInterval    = header.daysPerInterval;
        // kilometers per astronomical unit
        eph->au                 = header.au;
        eph->earthMoonMassRatio = header.earthMoonMassRatio;

        // Read the coefficient information for each item in the ephemeris
        eph->recordSize = 0;
        for (unsigned int i = 0; i < JPLEph_NItems; i++)
        {
            const auto& headerCoeff = header.coeffInfo[i];
            auto& ephCoeff = eph->coeffInfo[i];

            ephCoeff.offset    = headerCoeff.offset;
            ephCoeff.nCoeffs   = headerCoeff.nCoeffs;
            ephCoeff.nGranules = headerCoeff.nGranules;

            if (ephCoeff.offset < 3)
                return nullptr;

            ephCoeff.offset -= 3;

            // last item is the nutation ephemeris (only 2 components)
            unsigned nRecords = (i == JPLEph_NItems - 1) ? 2 : 3;
            eph->recordSize += ephCoeff.nCoeffs * ephCoeff.nGranules * nRecords;
        }

        eph->librationCoeffInfo.offset = header.librationCoeffInfo.offset;
        eph->librationCoeffInfo.nCoeffs = header.librationCoeffInfo.nCoeffs;
        eph->librationCoeffInfo.nGranules = header.librationCoeffInfo.nGranules;

        eph->recordSize += eph->librationCoeffInfo.nCoeffs * eph->librationCoeffInfo.nGranules * 3;
        eph->recordSize += 2; // record start and end time

        // if INPOP ephemeris, read record size
        if (eph->DENum == INPOP_DE_COMPATIBLE)
        {
            util::EndianBytes<std::uint32_t, E> recordSize;
            if (!in.read(reinterpret_cast<char*>(&recordSize), sizeof(recordSize))) /* Flawfinder: ignore */
                return nullptr;

            eph->recordSize = recordSize;
        }

        // First record is the header
        // The next record contains constant values (which we don't need)
        if (!in.seekg(eph->recordSize * 2 * sizeof(double), std::ios_base::beg))
            return nullptr;

        auto nRecords = static_cast<unsigned int>((eph->endDate - eph->startDate) / eph->daysPerInterval);
        eph->records.resize(nRecords);

        auto recordsData = std::make_unique_for_overwrite<util::EndianBytes<double, E>[]>(nRecords * eph->recordSize);
        if (!in.read(reinterpret_cast<char*>(recordsData.get()), sizeof(double) * nRecords * eph->recordSize)) /* Flawfinde: ignore */
            return nullptr;

        const auto* ptr = recordsData.get();
        for (unsigned int i = 0; i < nRecords; ++i)
        {
            auto& ephRecord = eph->records[i];
            ephRecord.t0 = ptr[0];
            ephRecord.t1 = ptr[1];

            // Allocate coefficient array for this record; the first two
            // 'coefficients' are actually the start and end time (t0 and t1)
            ephRecord.coeffs.reserve(eph->recordSize - 2);
            for (unsigned int j = 0; j < eph->recordSize - 2; ++j)
                ephRecord.coeffs.push_back(ptr[j + 2]);

            ptr += eph->recordSize;
        }

        return eph;
    }
};

std::shared_ptr<const JPLEphemeris>
JPLEphemeris::load(std::istream& in)
{
    std::uint32_t deNum;
    if (constexpr auto deNumOffset = offsetof(JPLEFileHeader<std::endian::native>, deNum);
        !in.seekg(deNumOffset, std::ios_base::beg) ||
        !in.read(reinterpret_cast<char*>(&deNum), sizeof(deNum)))
    {
        return nullptr;
    }

    bool swapBytes;
    if (deNum == INPOP_DE_COMPATIBLE)
    {
        // INPOP ephemeris with same endianess as CPU
        swapBytes = false;
    }
    else if (std::uint32_t deNumSwapped = compat::byteswap(deNum); deNumSwapped == INPOP_DE_COMPATIBLE)
    {
        // INPOP ephemeris with different endianess
        swapBytes = true;
    }
    else if ((deNum > (1u << 15)) && (deNumSwapped >= DE200))
    {
        // DE ephemeris with different endianess
        swapBytes = true;
    }
    else if ((deNum <= (1u << 15)) && (deNum >= DE200))
    {
        // DE ephemeris with same endianess as CPU
        swapBytes = false;
    }
    else
    {
        // something unknown or broken
        return nullptr;
    }

    constexpr auto ReverseEndian = std::endian::native == std::endian::little
        ? std::endian::big
        : std::endian::little;

    return swapBytes
        ? Initializer::initialize<ReverseEndian>(in)
        : Initializer::initialize<std::endian::native>(in);
}

} // end namespace celestia::ephem
