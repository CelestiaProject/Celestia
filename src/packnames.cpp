// packnames.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <string>
#include "constellation.h"
#include "starname.h"


static char *greekAlphabet[] =
{
    "Alpha",
    "Beta",
    "Gamma",
    "Delta",
    "Epsilon",
    "Zeta",
    "Eta",
    "Theta",
    "Iota",
    "Kappa",
    "Lambda",
    "Mu",
    "Nu",
    "Xi",
    "Omicron",
    "Pi",
    "Rho",
    "Sigma",
    "Tau",
    "Upsilon",
    "Phi",
    "Chi",
    "Psi",
    "Omega"
};


int main(int argc, char *argv[])
{
    string s;

    for (;;)
    {
	unsigned int catalogNumber;
	char sep;

	cin >> catalogNumber;
	if (cin.eof())
        {
	    break;
	}
	else if (cin.bad())
	{
	    cerr << "Error reading names file.\n";
	    break;
	}

	cin >> sep;
	if (sep != ':')
        {
	    cerr < "Missing ':' in names file.\n";
	    break;
	}

	// Get the rest of the line
	getline(cin, s);

	int nextSep = s.find_first_of(':');
	if (nextSep == string::npos)
        {
	    cerr << "Missing ':' in names file.\n";
	    break;
	}

	string common, designation;
	string conAbbr;
	string conName;
	string bayerLetter;
	
	if (nextSep != 0)
	    common = s.substr(0, nextSep);
	designation = s.substr(nextSep + 1, string::npos);

	if (designation != "")
        {
	    nextSep = designation.find_last_of(' ');
	    if (nextSep != string::npos)
	    {
		bayerLetter = designation.substr(0, nextSep);
		conAbbr = designation.substr(nextSep + 1, string::npos);
	    }
	}

	Constellation *constel;
	for (int i = 0; i < 88; i++)
        {
	    constel = Constellation::getConstellation(i);
	    if (constel == NULL)
   	    {
		cerr << "Error getting constellation " << i << '\n';
		break;
  	    }
	    if (constel->getAbbreviation() == conAbbr)
 	    {
		conName = constel->getName();
		break;
	    }
	}

	if (constel != NULL && bayerLetter != "")
        {
	    StarName sn(common, bayerLetter, constel);
	    cout << sn.getDesignation() << ' ' << constel->getAbbreviation() << '\n';
	}

	
    }
}
