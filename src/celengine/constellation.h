// constellation.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CONSTELLATION_H_
#define _CONSTELLATION_H_

#include <string>

class Constellation
{
public:
    static Constellation *getConstellation(unsigned int);
    static Constellation *getConstellation(const std::string&);

    std::string getName();
    std::string getGenitive();
    std::string getAbbreviation();

private:
    Constellation(const char *_name, const char *_genitive, const char *_abbrev);
    static void initialize();

    std::string name;
    std::string genitive;
    std::string abbrev;
};

#endif // _CONSTELLATION_H_


