// jpleph.h
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

#pragma once

#include <array>
#include <cstddef>
#include <iosfwd>
#include <vector>

#include <Eigen/Core>

namespace celestia::ephem
{

enum class JPLEphemItem
{
    Mercury       =  0,
    Venus         =  1,
    EarthMoonBary =  2,
    Mars          =  3,
    Jupiter       =  4,
    Saturn        =  5,
    Uranus        =  6,
    Neptune       =  7,
    Pluto         =  8,
    Moon          =  9,
    Sun           = 10,
    Earth         = 11,
    SSB           = 12,
};


struct JPLEphCoeffInfo
{
    unsigned int offset;
    unsigned int nCoeffs;
    unsigned int nGranules;
};


struct JPLEphRecord
{
    JPLEphRecord() = default;
    ~JPLEphRecord() = default;

    double t0{ 0.0 };
    double t1{ 0.0 };
    std::vector<double> coeffs{ };
};


class JPLEphemeris
{
private:
    JPLEphemeris() = default;

public:
    static constexpr std::size_t JPLEph_NItems = 12;

    ~JPLEphemeris() = default;

    Eigen::Vector3d getPlanetPosition(JPLEphemItem, double t) const;

    static JPLEphemeris* load(std::istream&);

    unsigned int getDENumber() const;
    double getStartDate() const;
    double getEndDate() const;
    bool getByteSwap() const;
    unsigned int getRecordSize() const;

private:
    std::array<JPLEphCoeffInfo, JPLEph_NItems> coeffInfo;
    JPLEphCoeffInfo librationCoeffInfo;

    double startDate;
    double endDate;
    double daysPerInterval;

    double au;
    double earthMoonMassRatio;

    unsigned int DENum;       // ephemeris version
    unsigned int recordSize;  // number of doubles per record
    bool swapBytes;

    std::vector<JPLEphRecord> records;
};

} // end namespace celestia::ephem
