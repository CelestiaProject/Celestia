/***************************************************************************
                          url.h  -  description
                             -------------------
    begin                : Wed Aug 7 2002
    copyright            : (C) 2002 by chris
    email                : chris@tux.teyssier.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _URL_H_
#define _URL_H_

#include <string>
#include "celestiacore.h"
#include "celengine/astro.h"

class CelestiaCore;

class Url
{
public:
    // parses str
    Url(const std::string& str, CelestiaCore *core);
    // current url of appCore
    Url(CelestiaCore* appCore);
    ~Url();

    std::string getAsString() const;
    std::string getName() const;
    void goTo();

private:
    std::string urlStr, name;
    std::string modeStr;
    std::string body1, body2, selectedStr, trackedStr;


    CelestiaCore *appCore;

    FrameOfReference ref;
    Selection selected;
    Selection tracked;

    astro::CoordinateSystem mode;
    int nbBodies;
    UniversalCoord coord;
    Quatf orientation;
    float fieldOfView;
    float timeScale;
    int renderFlags;
    int labelMode;
    astro::Date date;

    std::map<std::string, std::string> parseUrlParams(const std::string& url) const;
    std::string getCoordSysName(astro::CoordinateSystem mode) const;
    std::string getSelectionName(const Selection& selection) const;
    std::string getBodyShortName(const std::string& body) const;
    static std::string decode_string(const std::string& str);

    bool fromString;

};

#endif
