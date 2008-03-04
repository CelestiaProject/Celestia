// precession.h
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

namespace astro
{

// PA and QA are the location of the pole of the ecliptic of date
// with respect to the fixed ecliptic of J2000.0
struct EclipticAngles
{
    double PA;
    double QA;
};


// epsA is the obliquity with respect to the ecliptic of date. pA is
// the general precession---the angle between the ascending node of the
// equator on the ecliptic of date, and the ascending node of the ecliptic
// of date on the J2000.0 ecliptic.
struct PrecessionAngles
{
    double pA;     // precession
    double epsA;   // obliquity
};


extern EclipticAngles EclipticPrecession_P03LP(double T);
extern PrecessionAngles PrecObliquity_P03LP(double T);

};
