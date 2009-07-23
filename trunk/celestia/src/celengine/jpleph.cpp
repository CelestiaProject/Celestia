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

#include <fstream>
#include <iomanip>
#include <cstdio>
#include <cassert>
#include <celutil/bytes.h>
#include <celutil/basictypes.h>
#include "jpleph.h"

using namespace Eigen;
using namespace std;

static const unsigned int DE200RecordSize    =  826;
static const unsigned int DE405RecordSize    = 1018;
static const unsigned int DE406RecordSize    =  728;

static const unsigned int NConstants         =  400;
static const unsigned int ConstantNameLength =  6;

static const unsigned int MaxChebyshevCoeffs = 32;

static const int LabelSize = 84;


// Read a big-endian 32-bit unsigned integer
static int32 readUint(istream& in)
{
    int32 ret;
    in.read((char*) &ret, sizeof(int32));
    BE_TO_CPU_INT32(ret, ret);
    return (uint32) ret;
}

// Read a big-endian 64-bit IEEE double--if the native double format isn't
// IEEE 754, there will be troubles.
static double readDouble(istream& in)
{
    double d;
    in.read((char*) &d, sizeof(double));
    BE_TO_CPU_DOUBLE(d, d);
    return d;
}


JPLEphRecord::~JPLEphRecord()
{
    if (coeffs != NULL)
	delete coeffs;
}



JPLEphemeris::JPLEphemeris()
{
}


JPLEphemeris::~JPLEphemeris()
{
}


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


// Return the position of an object relative to the solar system barycenter
// or the Earth (in the case of the Moon) at a specified TDB Julian date tjd.
// If tjd is outside the span covered by the ephemeris it is clamped to a
// valid time.
Vector3d JPLEphemeris::getPlanetPosition(JPLEphemItem planet, double tjd) const
{
    // Solar system barycenter is the origin
    if (planet == JPLEph_SSB)
    {
        return Vector3d::Zero();
    }

    // The position of the Earth must be computed from the positions of the
    // Earth-Moon barycenter and Moon
    if (planet == JPLEph_Earth)
    {
        Vector3d embPos = getPlanetPosition(JPLEph_EarthMoonBary, tjd);

        // Get the geocentric position of the Moon
        Vector3d moonPos = getPlanetPosition(JPLEph_Moon, tjd);

        return embPos - moonPos * (1.0 / (earthMoonMassRatio + 1.0));
    }

    // Clamp time to [ startDate, endDate ]
    if (tjd < startDate)
        tjd = startDate;
    else if (tjd > endDate)
	tjd = endDate;

    // recNo is always >= 0:
    unsigned int recNo = (unsigned int) ((tjd - startDate) / daysPerInterval);
    // Make sure we don't go past the end of the array if t == endDate
    if (recNo >= records.size())
        recNo = records.size() - 1;
    const JPLEphRecord* rec = &records[recNo];

    assert(coeffInfo[planet].nGranules >= 1);
    assert(coeffInfo[planet].nGranules <= 32);
    assert(coeffInfo[planet].nCoeffs <= MaxChebyshevCoeffs);

    // u is the normalized time (in [-1, 1]) for interpolating
    // coeffs is a pointer to the Chebyshev coefficients
    double u = 0.0;
    double* coeffs = NULL;

    // nGranules is unsigned int so it will be compared against FFFFFFFF:
    if (coeffInfo[planet].nGranules == (unsigned int) -1)
    {
    	coeffs = rec->coeffs + coeffInfo[planet].offset;
    	u = 2.0 * (tjd - rec->t0) / daysPerInterval - 1.0;
    }
    else
    {
	double daysPerGranule = daysPerInterval / coeffInfo[planet].nGranules;
	int granule = (int) ((tjd - rec->t0) / daysPerGranule);
	double granuleStartDate = rec->t0 + daysPerGranule * (double) granule;
	coeffs = rec->coeffs + coeffInfo[planet].offset +
            granule * coeffInfo[planet].nCoeffs * 3;
	u = 2.0 * (tjd - granuleStartDate) / daysPerGranule - 1.0;
    }

    // Evaluate the Chebyshev polynomials
    double sum[3];
    double cc[MaxChebyshevCoeffs];
    unsigned int nCoeffs = coeffInfo[planet].nCoeffs;
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

    return Vector3d(sum[0], sum[1], sum[2]);
}


JPLEphemeris* JPLEphemeris::load(istream& in)
{
    JPLEphemeris* eph = NULL;

    // Skip past three header labels
    in.ignore(LabelSize * 3);
    if (!in.good())
        return NULL;

    // Skip past the constant names
    in.ignore(NConstants * ConstantNameLength);
    if (!in.good())
        return NULL;

    eph = new JPLEphemeris();
    if (eph == NULL)
        return NULL;

    // Read the start time, end time, and time interval
    eph->startDate = readDouble(in);
    eph->endDate = readDouble(in);
    eph->daysPerInterval = readDouble(in);
    if (!in.good())
    {
        delete eph;
        return NULL;
    }

    // Number of constants with valid values; not useful for us
    (void) readUint(in);

    eph->au = readDouble(in);     // kilometers per astronomical unit
    eph->earthMoonMassRatio = readDouble(in);

    // Read the coefficient information for each item in the ephemeris
    unsigned int i;
    for (i = 0; i < JPLEph_NItems; i++)
    {
        eph->coeffInfo[i].offset = readUint(in) - 3;
        eph->coeffInfo[i].nCoeffs = readUint(in);
        eph->coeffInfo[i].nGranules = readUint(in);
    }
    if (!in.good())
    {
        delete eph;
        return NULL;
    }

    eph->DENum = readUint(in);

    switch (eph->DENum)
    {
    case 200:
        eph->recordSize = DE200RecordSize;
        break;
    case 405:
        eph->recordSize = DE405RecordSize;
        break;
    case 406:
        eph->recordSize = DE406RecordSize;
        break;
    default:
        delete eph;
        return NULL;
    }

    eph->librationCoeffInfo.offset        = readUint(in);
    eph->librationCoeffInfo.nCoeffs       = readUint(in);
    eph->librationCoeffInfo.nGranules     = readUint(in);
    if (!in.good())
    {
        delete eph;
        return NULL;
    }

    // Skip past the rest of the record
    in.ignore(eph->recordSize * 8 - 2856);
    // The next record contains constant values (which we don't need)
    in.ignore(eph->recordSize * 8);
    if (!in.good())
    {
        delete eph;
        return NULL;
    }

    unsigned int nRecords = (unsigned int) ((eph->endDate - eph->startDate) /
					    eph->daysPerInterval);
    eph->records.resize(nRecords);
    for (i = 0; i < nRecords; i++)
    {
    	eph->records[i].t0 = readDouble(in);
    	eph->records[i].t1 = readDouble(in);

    	// Allocate coefficient array for this record; the first two
    	// 'coefficients' are actually the start and end time (t0 and t1)
    	eph->records[i].coeffs = new double[eph->recordSize - 2];
    	for (unsigned int j = 0; j < eph->recordSize - 2; j++)
    	    eph->records[i].coeffs[j] = readDouble(in);

    	// Make sure that we read this record successfully
    	if (!in.good())
    	{
    	    delete eph;
    	    return NULL;
    	}
    }

    return eph;
}
