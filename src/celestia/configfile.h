// configfile.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CONFIGFILE_H_
#define _CONFIGFILE_H_

#include <string>
#include <vector>

struct CelestiaConfig
{
    std::string starDatabaseFile;
    std::string starNamesFile;
    std::vector<std::string> catalogXrefFiles;
    std::vector<std::string> solarSystemFiles;
    std::string extrasDir;
    std::string galaxyCatalog;
    std::vector<std::string> labelledStars;
    std::string asterismsFile;
    float faintestVisible;
    std::string favoritesFile;
    std::string initScriptFile;
    std::string demoScriptFile;
    std::string destinationsFile;
    std::string mainFont;
    std::string labelFont;
    std::string titleFont;
    std::string logoTextureFile;
};


CelestiaConfig* ReadCelestiaConfig(std::string filename);

#endif // _CONFIGFILE_H_
