// config.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <string>
#include <vector>

struct CelestiaConfig
{
    std::string starDatabaseFile;
    std::string starNamesFile;
    std::vector<std::string> solarSystemFiles;
    std::vector<std::string> labelledStars;
};


CelestiaConfig* ReadCelestiaConfig(string filename);

