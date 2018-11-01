// constellation.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>

class Constellation
{
public:
    Constellation(const char *_name, const char *_genitive, const char *_abbrev);

    static Constellation *getConstellation(unsigned int);
    static Constellation *getConstellation(const std::string&);

    const std::string getName() const;
    const std::string getGenitive() const;
    const std::string getAbbreviation() const;

private:
    const char* name;
    const char* genitive;
    const char* abbrev;
};
