// starname.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _STARNAME_H_
#define _STARNAME_H_
#include <string>
#include "basictypes.h"
#include "constellation.h"

using std::string;

class StarName
{
public:
    StarName(string _name, string _designation, Constellation *_constel);

    string getName();
    string getDesignation(); // get Bayer or Flamsteed designation
    Constellation *getConstellation();

private:
    string name;
    string designation; // Bayer or Flamsteed designation
    Constellation *constellation;
};
#endif // _STARNAME_H_
