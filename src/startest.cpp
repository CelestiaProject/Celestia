// startest.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <math.h>
#include "celestia.h"
#include "stardb.h"
#include "visstars.h"
#include "mathlib.h"


int main(int argc, char *argv[])
{
    if (argc != 3)
    {
	cerr << "usage: " << argv[0] << " <stars file> <star names file>\n";
	exit(1);
    }

    ifstream starFile(argv[1], ios::in | ios::binary);
    if (!starFile.good())
    {
	cerr << "Error opening " << argv[1] << '\n';
	exit(1);
    }

    ifstream starNamesFile(argv[2], ios::in);
    if (!starNamesFile.good())
    {
	cerr << "Error opening " << argv[2] << '\n';
	exit(1);
    }

    StarDatabase* starDB = StarDatabase::read(starFile);
    if (starDB == NULL)
    {
	cerr << "Error reading stars file\n";
	exit(1);
    }

    StarNameDatabase* starNameDB = StarDatabase::readNames(starNamesFile);
    if (starNameDB == NULL)
    {
        cerr << "Error reading star names file\n";
	exit(1);
    }

    cout << "nNames = " << starNameDB->size() << '\n';
#if 0
    for (;;)
    {
	int hd;

	if (!(cin >> hd))
	    break;

	StarNameDatabase::iterator iter = starNameDB->find(hd);

	if (iter == starNameDB->end())
        {
	    cout << "No name found.\n";
	}
	else
	{
	    cout << iter->second->getName() << '\n';
	}

	if (!cin.good())
	    break;
    }
#endif
    uint d10 = 0, d20 = 0, d30 = 0, bright = 0;
    uint subdwarves = 0;

    for (uint32 i = 0; i < starDB->size(); i++)
    {
        Star *star = starDB->getStar(i);
        Point3f pos = star->getPosition();
        float distance = (float) sqrt(pos.x * pos.x + pos.y * pos.y +
                                      pos.z * pos.z);
        float appMag = star->getAbsoluteMagnitude() - 5 + 5 *
            (float) log10(distance / 3.26);
        if (appMag < 6)
            bright++;
        if (star->getStellarClass().getLuminosityClass() == StellarClass::Lum_VI)
            subdwarves++;
        if (distance < 30)
        {
            d30++;
            if (distance < 20)
            {
                uint32 catNo = star->getCatalogNumber();
                StarNameDatabase::iterator iter = starNameDB->find(catNo);
                if (iter != starNameDB->end())
                {
                    cout << iter->second->getName() << " : " << distance << " : " << star->getStellarClass() << " : " << star->getLuminosity() << '\n';
                }
                else
                {
                    cout << catNo << " : " << distance << " : " << star->getStellarClass() << " : " << star->getLuminosity() << '\n';
                }
                d20++;
                if (distance < 10)
                {
                    d10++;
                }
            }
        }
    }

    cout << "subdwarves (Type VI):" << subdwarves << '\n';
    cout << "mag < 6.0: " << bright << '\n';
    cout << "closer than 10 ly: " << d10 << '\n';
    cout << "closer than 20 ly: " << d20 << '\n';
    cout << "closer than 30 ly: " << d30 << '\n';

    {
        Observer obs;
        VisibleStarSet visset(starDB);

        visset.setCloseDistance(20);
        visset.setLimitingMagnitude(5.5f);
        visset.updateAll(obs);

        vector<uint32> *starvec = visset.getVisibleSet();
        cout << "visible: " << starvec->size() << "\n";
    }

    return 0;
}
