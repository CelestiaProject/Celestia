// customorbit.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "mathlib.h"
#include "astro.h"
#include "customorbit.h"

using namespace std;

// Epoch values (for 1990.0):
#define CNVRAD	0.017453292
#define MOON_EPOCH 1990			// Epoch for orbital data
#define MOON_EPOCH_JD 2447891.5		// Julian day at epoch
#define MOON_MLO 318.351648		// Moon's mean longitude at epoch
#define MOON_PERIGEE_MLO 36.340410	// Mean longitude of the perigee at epoch
#define MOON_ASCNODE_MLO 318.510107     // Mean longitude of ascending node at epoch
#define MOON_INCLINATION 5.145396	// Inclination of the moon's orbit
#define MOON_ECCENTRICITY 0.054900	// Eccentricity of moon's orbit.
#define MOON_SEMIMAJORAXIS 384401	// Semi-major axis of moon orbit.


static void EclipticToEquatorial(double fEclLat, double fEclLon, double fEclObliq,
                                 double& RA, double& dec)
{
	double x, y;

	//Convert angles to radians
	fEclLat *= CNVRAD;
	fEclLon *= CNVRAD;
	fEclObliq *= CNVRAD;

	dec = asin(sin(fEclLat) * cos(fEclObliq) + cos(fEclLat) * sin(fEclObliq) * sin(fEclLon));
	y = sin(fEclLon) * cos(fEclObliq) - tan(fEclLat) * sin(fEclObliq);
	x = cos(fEclLon);
	RA = atan2(y, x);
}


class LunarOrbit : public Orbit
{
    Point3d positionAtTime(double jd) const
    {
	double D, Mm, N, L, MAS, ES, ELS, NS;
	double Ev, Ae, A3, Ec, A4, V, v, x, y;
	double fEclLat, fEclLon, fEclObliq;

	// Find the epoch day (number of days before or after epoch)
	D = jd - MOON_EPOCH_JD;

	// Get value for ecliptic longitude and mean anomaly for Sun
	NS = pfmod(360 / 365.242191 * D, 360.0);
	MAS = pfmod(NS + 279.403303 - 282.768422, 360.0) * CNVRAD;
	ES = astro::eccentricAnomaly(MAS);
	v = 2*atan(sqrt((1 + 0.016713) / (1 - 0.016713)) * tan(ES/2)) / CNVRAD;
	ELS = pfmod(v + 282.768422, 360.0);

	// Compute moon parameters and corrections
	L = 13.1763966 * D + MOON_MLO;
	L = pfmod(L, 360.0);
	Mm = L - 0.1114041 * D - MOON_PERIGEE_MLO;
	Mm = pfmod(Mm, 360.0);
	N = MOON_ASCNODE_MLO - 0.0529539 * D;
	N = pfmod(N, 360.0);
	Ev = 1.2739 * sin((2*(L - ELS) - Mm) * CNVRAD);
	Ae = 0.1858 * sin(MAS);
	A3 = 0.37 * sin(MAS);
	Mm = Mm + Ev - Ae - A3;
	Ec = 6.2886 * sin(Mm * CNVRAD);
	A4 = 0.214 * sin(2 * Mm * CNVRAD);
	L = L + Ev + Ec - Ae + A4;
	V = 0.6583 * sin(2* (L - ELS) * CNVRAD);
	L = L + V;
	N = N - 0.16 * sin(MAS * CNVRAD);
	y = sin((L - N) * CNVRAD) * cos(MOON_INCLINATION * CNVRAD);
	x = cos((L - N) * CNVRAD);
	
	fEclLon = atan2(y, x) / CNVRAD + N;
	fEclLat = asin(sin((L - N) * CNVRAD) * sin(MOON_INCLINATION * CNVRAD)) / CNVRAD;
	double distance = MOON_SEMIMAJORAXIS * (1 - MOON_ECCENTRICITY * MOON_ECCENTRICITY) / 
		(1 + MOON_ECCENTRICITY * cos((Mm + Ec) * CNVRAD));

	fEclObliq = astro::meanEclipticObliquity(jd);

        double RA, dec;
	EclipticToEquatorial(fEclLat, fEclLon, fEclObliq, RA, dec);
        dec -= PI / 2;
        RA += PI;

        return Point3d(cos(RA) * sin(dec) * distance,
                       cos(dec) * distance,
                       -sin(RA) * sin(dec) * distance);
    };

    double getPeriod() const
    {
        return 27.321661;
    };

private:
    int nonZeroSize;
};


Orbit* GetCustomOrbit(const std::string& name)
{
    if (name == "moon")
        return new LunarOrbit();
    else
        return NULL;
}
