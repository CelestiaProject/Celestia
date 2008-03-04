// precession.cpp
//
// Calculate precession angles for Earth.
//
// Copyright (C) 2008, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include <iostream>
#include <celmath/mathlib.h>
#include <celmath/vecmath.h>
#include "precession.h"

using namespace std;


// Periodic term for the long-period extension of the P03 precession
// model.
struct EclipticPrecessionTerm
{
    double Pc;
    double Qc;
    double Ps;
    double Qs;
    double period;
};


static EclipticPrecessionTerm EclipticPrecessionTerms[] =
{
    {   486.230527, 2559.065245, -2578.462809,   485.116645, 2308.98 },
    {  -963.825784,  247.582718,  -237.405076,  -971.375498, 1831.25 },
    { -1868.737098, -957.399054,  1007.593090, -1930.464338,  687.52 },
    { -1589.172175,  493.021354,  -423.035168, -1634.905683,  729.97 },
    {   429.442489, -328.301413,   337.266785,   429.594383,  492.21 },
    { -2244.742029, -339.969833,   221.240093, -2131.745072,  708.13 },
};


// Periodic term for the long-period extension of the P03 precession
// model.
struct PrecessionTerm
{
    double pc;
    double epsc;
    double ps;
    double epss;
    double period;
};


static PrecessionTerm PrecessionTerms[] =
{
    { -6180.062400,   807.904635, -2434.845716, -2056.455197,  409.90 },
    { -2721.869299,  -177.959383,   538.034071,  -912.727303,  396.15 },
    {  1460.746498,   371.942696, -1245.689351,   447.710000,  536.91 },
    { -1838.488899,  -176.029134,   529.220775,  -611.297411,  402.90 },
    {   949.518077,   -89.154030,   277.195375,   315.900626,  417.15 },
    {    32.701460,  -336.048179,   945.979710,    12.390157,  288.92 },
    {   598.054819,   -17.415730,  -955.163661,   -15.922155, 4042.97 },
    {  -293.145284,   -28.084479,    93.894079,  -102.870153,  304.90 },
    {    66.354942,    21.456146,     0.671968,    24.123484,  281.46 },
    {    18.894136,    30.917011,  -184.663935,     2.512708,  204.38 },
};




/*! Compute the precession of the ecliptic, based on a long-period
 *  extension of the the P03 model, presented in "Long-periodic Precession
 *  Parameters", J. Vondrak (2006)
 *  http://www.astronomy2006.com/files/vondrak.pdf
 *  For an explanation of the angles used in the P03 model, see
 *  "Expressions for IAU2000 precession quantities", N. Capitaine et al,
 *  Astronomy & Astrophysics, v.412, p.567-586 (2003).
 *
 *  6 long-periodic terms, plus a cubic polynomial for longer terms.
 *  The terms are fitted to the P03 model withing 1000 years of J2000.
 *
 *  T is the time in centuries since J2000. The angles returned are
 *  in arcseconds.
 */
astro::EclipticAngles
astro::EclipticPrecession_P03LP(double T)
{
    EclipticAngles angles;

    double T2 = T * T;
    double T3 = T2 * T;

    angles.PA = (5750.804069
                 +  0.1948311 * T
                 -  0.00016739 * T2
                 -  4.8e-8 * T3);
    angles.QA = (-1673.999018
                 +   0.3474459 * T
                 +   0.00011243 * T2
                 -   6.4e-8 * T3);

    unsigned int nTerms = sizeof(EclipticPrecessionTerms) / sizeof(EclipticPrecessionTerms[0]);
    for (unsigned int i = 0; i < nTerms; i++)
    {
        const EclipticPrecessionTerm& p = EclipticPrecessionTerms[i];
        double theta = 2.0 * PI * T / p.period;
        double s = sin(theta);
        double c = cos(theta);
        angles.PA += p.Pc * c + p.Ps * s;
        angles.QA += p.Qc * c + p.Qs * s;
    }

    return angles;
}


/*! Compute the general precession and obliquity, based on the model
 *  presented in "Long-periodic Precession Parameters", J. Vondrak, 2006
 *  http://www.astronomy2006.com/files/vondrak.pdf
 *
 *  T is the time in centuries since J2000. The angles returned are
 *  in arcseconds.
 */
astro::PrecessionAngles
astro::PrecObliquity_P03LP(double T)
{
    PrecessionAngles angles;

    double T2 = T * T;
    double T3 = T2 * T;

    angles.pA   = (  7907.295950
                   + 5044.374034 * T
                   -    0.00713473 * T2
                   +    6e-9 * T3);
    angles.epsA = (  83973.876448
                   -     0.0425899 * T
                   -     0.00000113 * T2);

    unsigned int nTerms = sizeof(PrecessionTerms) / sizeof(PrecessionTerms[0]);
    for (unsigned int i = 0; i < nTerms; i++)
    {
        const PrecessionTerm& p = PrecessionTerms[i];
        double theta = 2.0 * PI * T / p.period;
        double s = sin(theta);
        double c = cos(theta);
        angles.pA   += p.pc * c   + p.ps * s;
        angles.epsA += p.epsc * c + p.epss * s;
    }

    return angles;
}



#define TEST 0

#if TEST

int main(int argc, char* argv[])
{
    for (int i = -100; i <= 100; i++)
    {
	// Get time in Julian centuries from J2000
	double T = (double) i * 100.0;
	
	astro::EclipticAngles ecl = astro::EclipticPrecession_P03LP(T);
	
	Vec3d v(0.0, 0.0, 0.0);
	v.x =  ecl.PA * 2.0 * PI / 1296000;
	v.y = -ecl.QA * 2.0 * PI / 1296000;
	v.z = sqrt(1.0 - v.x * v.x - v.y * v.y);

	// Show the angle between the J2000 ecliptic pole and the ecliptic
	// pole at T centuries from J2000
	clog << i << ": " << radToDeg(acos(v.z)) << endl;
    }

    return 0;
}

#endif // TEST
