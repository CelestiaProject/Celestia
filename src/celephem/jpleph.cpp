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

#include <array>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <istream>
#include <type_traits>

#include <celutil/bytes.h>
#include "jpleph.h"

namespace celestia::ephem
{

namespace
{

inline void getMaybeSwapUint32(std::uint32_t& dest, const char* ptr, bool swapBytes)
{
    std::memcpy(&dest, ptr, sizeof(std::uint32_t));
    if (swapBytes)
    {
        dest = bswap_32(dest);
    }
}


inline void getMaybeSwapDouble(double& dest, const char* ptr, bool swapBytes)
{
    if (swapBytes)
    {
        static_assert(sizeof(double) == sizeof(std::uint64_t));
        // Because of potential issues with handling NaN representations
        // we do the swap operation on an integer, then create the double
        std::uint64_t reversed;
        std::memcpy(&reversed, ptr, sizeof(double));
        reversed = bswap_64(reversed);
        std::memcpy(&dest, &reversed, sizeof(double));
    }
    else
    {
        std::memcpy(&dest, ptr, sizeof(double));
    }
}

constexpr unsigned int NConstants         =  400;
constexpr unsigned int ConstantNameLength =  6;

constexpr unsigned int MaxChebyshevCoeffs = 32;

constexpr unsigned int LabelSize = 84;

constexpr unsigned int INPOP_DE_COMPATIBLE = 100;
constexpr unsigned int DE200 = 200;

// Read a big-endian or little endian 32-bit unsigned integer
std::uint32_t readUint(std::istream& in, bool swap)
{
    std::uint32_t ret;
    in.read((char*) &ret, sizeof(std::uint32_t));
    return swap ? bswap_32(ret) : ret;
}

// Read a big-endian or little endian 64-bit IEEE double.
// If the native double format isn't IEEE 754, there will be troubles.
double readDouble(std::istream& in, bool swap)
{
    double d;
    in.read((char*) &d, sizeof(double));
    return swap ? bswap_double(d) : d;
}

#pragma pack(push, 1)

// These packed structs are only used for offset calculations, they should
// not be instantiated directly.

struct JPLECoeff
{
    JPLECoeff() = delete;

    std::uint32_t offset;
    std::uint32_t nCoeffs;
    std::uint32_t nGranules;
};

struct JPLEFileHeader
{
    JPLEFileHeader() = delete;

    //  Three header labels
    char headerLabels[3][LabelSize];

    // Constant names
    char constantNames[NConstants][ConstantNameLength];

    // Start time, end time, and time interval
    double startDate, endDate, daysPerInterval;

    // Number of constants with valid values
    std::uint32_t nConstants;

    // km per AU
    double au;
    // Earth-Moon mass ratio
    double earthMoonMassRatio;

    // Coefficient information for each item in the ephemeris
    JPLECoeff coeffInfo[JPLEphemeris::JPLEph_NItems];

    // DE number
    std::uint32_t deNum;

    // Libration coefficient information
    JPLECoeff librationCoeffInfo;
};

#pragma pack(pop)

static_assert(std::is_standard_layout_v<JPLECoeff>);
static_assert(std::is_standard_layout_v<JPLEFileHeader>);

} // end unnamed namespace


unsigned int JPLEphemeris::getDENumber() const
{
    return DENum;
}

double JPLEphemeris::getStartDate() const
{
    return startDate;
}

double JPLEphemeris::getEndDate() const
{
    return endDate;
}

unsigned int JPLEphemeris::getRecordSize() const
{
    return recordSize;
}

bool JPLEphemeris::getByteSwap() const
{
    return swapBytes;
}

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
    auto recNo = (unsigned int) ((tjd - startDate) / daysPerInterval);
    // Make sure we don't go past the end of the array if t == endDate
    if (recNo >= records.size())
        recNo = records.size() - 1;
    const JPLEphRecord* rec = &records[recNo];

    auto planetIdx = static_cast<std::size_t>(planet);

    assert(coeffInfo[planetIdx].nGranules >= 1);
    assert(coeffInfo[planetIdx].nGranules <= 32);
    assert(coeffInfo[planetIdx].nCoeffs <= MaxChebyshevCoeffs);

    // u is the normalized time (in [-1, 1]) for interpolating
    // coeffs is a pointer to the Chebyshev coefficients
    double u = 0.0;
    const double* coeffs = nullptr;

    // nGranules is unsigned int so it will be compared against FFFFFFFF:
    if (coeffInfo[planetIdx].nGranules == (unsigned int) -1)
    {
        coeffs = rec->coeffs.data() + coeffInfo[planetIdx].offset;
        u = 2.0 * (tjd - rec->t0) / daysPerInterval - 1.0;
    }
    else
    {
        double daysPerGranule = daysPerInterval / coeffInfo[planetIdx].nGranules;
        auto granule = (int) ((tjd - rec->t0) / daysPerGranule);
        double granuleStartDate = rec->t0 + daysPerGranule * (double) granule;
        coeffs = rec->coeffs.data() + coeffInfo[planetIdx].offset +
                 granule * coeffInfo[planetIdx].nCoeffs * 3;
        u = 2.0 * (tjd - granuleStartDate) / daysPerGranule - 1.0;
    }

    // Evaluate the Chebyshev polynomials
    double sum[3];
    double cc[MaxChebyshevCoeffs];
    unsigned int nCoeffs = coeffInfo[planetIdx].nCoeffs;
    for (int i = 0; i < 3; i++)
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


JPLEphemeris* JPLEphemeris::load(std::istream& in)
{
    std::array<char, sizeof(JPLEFileHeader)> fh;
    in.read(fh.data(), fh.size()); /* Flawfinder: ignore */
    if (!in.good())
        return nullptr;

    decltype(JPLEFileHeader::deNum) deNum;
    std::memcpy(&deNum, fh.data() + offsetof(JPLEFileHeader, deNum), sizeof(deNum));
    std::uint32_t deNum2 = bswap_32(deNum);

    bool swapBytes;
    if (deNum == INPOP_DE_COMPATIBLE)
    {
        // INPOP ephemeris with same endianess as CPU
        swapBytes = false;
    }
    else if (deNum2 == INPOP_DE_COMPATIBLE)
    {
        // INPOP ephemeris with different endianess
        swapBytes = true;
        deNum = bswap_32(deNum);
    }
    else if ((deNum > (1u << 15)) && (deNum2 >= DE200))
    {
        // DE ephemeris with different endianess
        swapBytes = true;
        deNum = deNum2;
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

    auto *eph = new JPLEphemeris();
    eph->swapBytes = swapBytes;
    eph->DENum = deNum;

    // Read the start time, end time, and time interval
    getMaybeSwapDouble(eph->startDate,          fh.data() + offsetof(JPLEFileHeader, startDate),          swapBytes);
    getMaybeSwapDouble(eph->endDate,            fh.data() + offsetof(JPLEFileHeader, endDate),            swapBytes);
    getMaybeSwapDouble(eph->daysPerInterval,    fh.data() + offsetof(JPLEFileHeader, daysPerInterval),    swapBytes);
    // kilometers per astronomical unit
    getMaybeSwapDouble(eph->au,                 fh.data() + offsetof(JPLEFileHeader, au),                 swapBytes);
    getMaybeSwapDouble(eph->earthMoonMassRatio, fh.data() + offsetof(JPLEFileHeader, earthMoonMassRatio), swapBytes);

    // Read the coefficient information for each item in the ephemeris
    eph->recordSize = 0;
    for (unsigned int i = 0; i < JPLEph_NItems; i++)
    {
        const char* coeffInfo = fh.data() + offsetof(JPLEFileHeader, coeffInfo) + i * sizeof(JPLECoeff);
        getMaybeSwapUint32(eph->coeffInfo[i].offset,    coeffInfo + offsetof(JPLECoeff, offset),    swapBytes);
        getMaybeSwapUint32(eph->coeffInfo[i].nCoeffs,   coeffInfo + offsetof(JPLECoeff, nCoeffs),   swapBytes);
        getMaybeSwapUint32(eph->coeffInfo[i].nGranules, coeffInfo + offsetof(JPLECoeff, nGranules), swapBytes);
        eph->coeffInfo[i].offset -= 3;
        // last item is the nutation ephemeris (only 2 components)
        unsigned nRecords = i == JPLEph_NItems - 1 ? 2 : 3;
        eph->recordSize += eph->coeffInfo[i].nCoeffs * eph->coeffInfo[i].nGranules * nRecords;
    }

    const char* librationCoeffInfo = fh.data() + offsetof(JPLEFileHeader, librationCoeffInfo);
    getMaybeSwapUint32(eph->librationCoeffInfo.offset,    librationCoeffInfo + offsetof(JPLECoeff, offset),    swapBytes);
    getMaybeSwapUint32(eph->librationCoeffInfo.nCoeffs,   librationCoeffInfo + offsetof(JPLECoeff, nCoeffs),   swapBytes);
    getMaybeSwapUint32(eph->librationCoeffInfo.nGranules, librationCoeffInfo + offsetof(JPLECoeff, nGranules), swapBytes);
    eph->recordSize += eph->librationCoeffInfo.nCoeffs * eph->librationCoeffInfo.nGranules * 3;
    eph->recordSize += 2;   // record start and end time

    // if INPOP ephemeris, read record size
    if (deNum == INPOP_DE_COMPATIBLE)
    {
       eph->recordSize = readUint(in, eph->swapBytes);
       // Skip past the rest of the record
       in.ignore(eph->recordSize * 8 - sizeof(JPLEFileHeader) - sizeof(uint32_t));
    }
    else
    {
        // Skip past the rest of the record
        in.ignore(eph->recordSize * 8 - sizeof(JPLEFileHeader));
    }

    // The next record contains constant values (which we don't need)
    in.ignore(eph->recordSize * 8);
    if (!in.good())
    {
        delete eph;
        return nullptr;
    }

    auto nRecords = (unsigned int) ((eph->endDate - eph->startDate) /
                        eph->daysPerInterval);
    eph->records.resize(nRecords);
    for (unsigned int i = 0; i < nRecords; i++)
    {
        eph->records[i].t0 = readDouble(in, eph->swapBytes);
        eph->records[i].t1 = readDouble(in, eph->swapBytes);

        // Allocate coefficient array for this record; the first two
        // 'coefficients' are actually the start and end time (t0 and t1)
        eph->records[i].coeffs.reserve(eph->recordSize - 2);
        for (unsigned int j = 0; j < eph->recordSize - 2; j++)
            eph->records[i].coeffs.push_back(readDouble(in, eph->swapBytes));

        // Make sure that we read this record successfully
        if (!in.good())
        {
            delete eph;
            return nullptr;
        }
    }

    return eph;
}

} // end namespace celestia::ephem
