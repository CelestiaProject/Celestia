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
#include <map>
#include "celestiacore.h"
#include "celengine/astro.h"

class CelestiaCore;
class CelestiaState;

class Url
{
public:
    enum UrlType
    {
        Absolute = 0,
        Relative = 1,
        Settings = 2,
    };

    /*! The TimeSource specifies what the time will be set to when the user
     *  activates the URL.
     *  - UseUrlTime indicates that the simulation time should be set to whatever
     *    value was stored in the URL.
     *  - UseSimulationTime means that the simulation time at activation is not
     *    changed.
     *  - UseSystemTime means that the simulation time will be set to whatever the
     *    current system time is when the URL is activated.
     */
    enum TimeSource
    {
        UseUrlTime        = 0,
        UseSimulationTime = 1,
        UseSystemTime     = 2,
        TimeSourceCount   = 3,
    };
    
    Url();
    
    // parses str
    Url(const std::string& str, CelestiaCore *core);
    // current url of appCore
    Url(CelestiaCore* appCore, UrlType type = Absolute);
    Url(const CelestiaState& appState,
        unsigned int version = CurrentVersion,
        TimeSource _timeSource = UseUrlTime);
    ~Url();

    std::string getAsString() const;
    std::string getName() const;
    void goTo();

    static const unsigned int CurrentVersion;

    static std::string decodeString(const std::string& str);
    static std::string encodeString(const std::string& str);
    
private:
    void initVersion2(std::map<std::string, std::string>& params, const std::string& timeString);
    void initVersion3(std::map<std::string, std::string>& params, const std::string& timeString);
    
private:
    std::string urlStr;
    std::string name;
    std::string modeStr;
    std::string body1;
    std::string body2;
    std::string selectedStr;
    std::string trackedStr;

    CelestiaCore *appCore;

    ObserverFrame ref;
    Selection selected;
    Selection tracked;

    ObserverFrame::CoordinateSystem mode;
    int nbBodies;
    float fieldOfView;
    float timeScale;
    int renderFlags;
    int labelMode;
    bool lightTimeDelay;
    bool pauseState;

    std::map<std::string, std::string> parseUrlParams(const std::string& url) const;
    std::string getCoordSysName(ObserverFrame::CoordinateSystem mode) const;
    std::string getBodyShortName(const std::string& body) const;
    std::string getEncodedObjectName(const Selection& selection);

    bool fromString;
    UrlType type;
    TimeSource timeSource;
    unsigned int version;
    
    void evalName();
    
    // Variables specific to Global Urls
    UniversalCoord coord;
    astro::Date date;
    Quatf orientation;
    
    // Variables specific to Relative Urls
    double distance, longitude, latitude;
};


/*! The CelestiaState class holds the current observer position, orientation,
 *  frame, time, and render settings. It is designed to be serialized as a cel
 *  URL, thus strings are stored for bodies instead of Selections.
 *
 *  Some information is *not* stored in cel URLs, including the current
 *  lists of reference marks and markers. Such lists can be arbitrarily long,
 *  and thus not practical to store in a URL.
 */
class CelestiaState
{
public:
    CelestiaState();
    
    bool loadState(std::map<std::string, std::string>& params);
    void saveState(std::map<std::string, std::string>& params);
    void captureState(CelestiaCore* appCore);
    
    // Observer frame, position, and orientation. For multiview, there needs
    // be one instance of these parameters per view saved.
    ObserverFrame::CoordinateSystem coordSys;
    string refBodyName;
    string targetBodyName;
    string trackedBodyName;
    UniversalCoord observerPosition;
    Quatf observerOrientation;
    float fieldOfView;
    
    // Time parameters
    double tdb;
    float timeScale;
    bool pauseState;
    bool lightTimeDelay;
    
    string selectedBodyName;
    
    int labelMode;
    int renderFlags;
};

#endif
