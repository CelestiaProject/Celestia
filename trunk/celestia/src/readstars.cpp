// readstars.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include <iostream>

#define MAX_STARS 120000

#define PI 3.1415926535898

#include "basictypes.h"
#include "stellarclass.h"
#include "star.h"


int main(int argc, char *argv[])
{
    Star *stars = new Star[MAX_STARS];
    int nStars = 0;
    float brightest = 100000;

    while (nStars < MAX_STARS) {
	uint32 catNo = 0;
	float RA = 0, dec = 0, parallax = 0;
	int16 appMag;
	uint16 stellarClass;

	cin.read((void *) &catNo, sizeof catNo);
	cin.read((void *) &RA, sizeof RA);
	cin.read((void *) &dec, sizeof dec);
	cin.read((void *) &parallax, sizeof parallax);
	cin.read((void *) &appMag, sizeof appMag);
	cin.read((void *) &stellarClass, sizeof stellarClass);
	if (!cin.good())
	    break;

	Star *star = &stars[nStars];

	// Compute distance based on parallax
	double distance = 3.26 / (parallax > 0.0 ? parallax / 1000.0 : 1e-6);

	// Convert from RA, dec spherical to cartesian coordinates
	double theta = RA / 24.0 * PI * 2;
	double phi = (1.0 - dec / 90.0) * PI / 2;
	double x = cos(theta) * sin(phi) * distance;
	double y = cos(phi) * distance;
	double z = sin(theta) * sin(phi) * distance;
	star->setPosition((float) x, (float) y, (float) z);

	// Use apparent magnitude and distance to determine the absolute
	// magnitude of the star.
	star->setAbsoluteMagnitude((float) (appMag / 256.0 + 5 -
					    5 * log10(distance / 3.26)));

	StellarClass sc((StellarClass::StarType) (stellarClass >> 12),
			(StellarClass::SpectralClass)(stellarClass >> 8 & 0xf),
			(unsigned int) (stellarClass >> 4 & 0xf),
			(StellarClass::LuminosityClass) (stellarClass & 0xf));
	star->setStellarClass(sc);

	star->setCatalogNumber(catNo);

	if (parallax > 0.0 && star->getAbsoluteMagnitude() < brightest)
        {
	    brightest = star->getAbsoluteMagnitude();
	    cout << brightest << ' ' << sc << '\n';
	}

	nStars++;
    }

    cout << nStars << '\n';
    cout << sizeof(StellarClass) << '\n';
    cout << sizeof(stars[0]) << '\n';
}
