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
struct EclipticPole
{
    double PA;
    double QA;
};


// piA and PiA are angles that transform the J2000 ecliptic to the
// ecliptic of date. They are related to the ecliptic pole coordinates
// PA and QA:
//   PA = sin(piA)*sin(PiA)
//   QA = sin(piA)*cos(PiA)
//
// PiA is the angle along the J2000 ecliptic between the J2000 equinox
// and the intersection of the J2000 ecliptic and ecliptic of date.
struct EclipticAngles
{
    double piA;
    double PiA;
};


// epsA is the angle between the ecliptic and mean equator of date. pA is the
// general precession: the difference between angles L and PiA. L is the angle
// along the mean ecliptic of date from the equinox of date to the
// intersection of the J2000 ecliptic and ecliptic of date.
struct PrecessionAngles
{
    double pA;     // precession
    double epsA;   // obliquity
};


struct EquatorialPrecessionAngles
{
    double zetaA;
    double zA;
    double thetaA;
};


extern EclipticPole EclipticPrecession_P03LP(double T);
extern PrecessionAngles PrecObliquity_P03LP(double T);

extern EclipticPole EclipticPrecession_P03(double T);
extern EclipticAngles EclipticPrecessionAngles_P03(double T);
extern PrecessionAngles PrecObliquity_P03(double T);
extern EquatorialPrecessionAngles EquatorialPrecessionAngles_P03(double T);

};
