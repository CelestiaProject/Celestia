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
    std::vector<std::string> solarSystemFiles;
    std::vector<std::string> starCatalogFiles;
    std::vector<std::string> extrasDirs;
    std::string deepSkyCatalog;
    std::vector<std::string> labelledStars;
    std::string asterismsFile;
    std::string boundariesFile;
    float faintestVisible;
    std::string favoritesFile;
    std::string initScriptFile;
    std::string demoScriptFile;
    std::string destinationsFile;
    std::string mainFont;
    std::string labelFont;
    std::string titleFont;
    std::string logoTextureFile;
    std::string cursor;
    std::vector<std::string> ignoreGLExtensions;
    float rotateAcceleration;
    float mouseRotationSensitivity;
    bool  reverseMouseWheel;
    std::string scriptScreenshotDirectory;
    std::string scriptSystemAccessPolicy;

    std::string HDCrossIndexFile;
    std::string SAOCrossIndexFile;
    std::string GlieseCrossIndexFile;

    // Renderer detail options
    unsigned int shadowTextureSize;
    unsigned int eclipseTextureSize;
    unsigned int ringSystemSections;
    unsigned int orbitPathSamplePoints;

    unsigned int aaSamples;

    bool hdr;
};


CelestiaConfig* ReadCelestiaConfig(std::string filename, CelestiaConfig* config = NULL);

#endif // _CONFIGFILE_H_
