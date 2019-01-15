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
#include <celengine/astro.h>
#include <Eigen/Core>
#include <Eigen/Geometry>

class CelestiaCore;
class CelestiaState;


class Url
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    using ParamsMap = std::map<std::string, std::string>;

    enum class UrlType
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
    enum class TimeSource
    {
        UseUrlTime        = 0,
        UseSimulationTime = 1,
        UseSystemTime     = 2,
        TimeSourceCount   = 3,
    };

    Url() = default;

    // parses str
    Url(std::string str, CelestiaCore *core);
    // current url of appCore
    Url(CelestiaCore* appCore, UrlType type = UrlType::Absolute);
    Url(const CelestiaState& appState,
        unsigned _version = CurrentVersion,
        TimeSource _timeSource = TimeSource::UseUrlTime);
    ~Url() = default;

    std::string getAsString() const;
    std::string toString() const;
    std::string getName() const;
    void goTo();

    constexpr static unsigned CurrentVersion{ 4 };

    static std::string decodeString(const std::string& str);
    static std::string encodeString(const std::string& str);

private:
    void initVersion2(ParamsMap& params, const std::string& timeString);
    void initVersion3(ParamsMap& params, const std::string& timeString);

    void initDatePos(ParamsMap& params, const std::string& timeString);
    void initSimCommon(ParamsMap& params);
    void initRenderFlags(ParamsMap& params);

private:
    std::string urlStr;
    std::string name;
    std::string modeStr;
    std::array<std::string, 2> bodies;
    std::string selectedStr;
    std::string trackedStr;

    CelestiaCore *appCore;

    ObserverFrame ref;
    Selection selected;
    Selection tracked;

    ObserverFrame::CoordinateSystem mode{ ObserverFrame::Universal };
    int        nbBodies{ -1 };
    float      fieldOfView{ 0.0f };
    float      timeScale{ 0.0f };
    uint64_t   renderFlags{ 0 };
    int        labelMode{ 0 };
    bool       lightTimeDelay{ false };
    bool       pauseState{ false };
    UrlType    type{ UrlType::Absolute };
    TimeSource timeSource{ TimeSource::UseUrlTime };
    unsigned   version{ CurrentVersion };

    ParamsMap parseUrlParams(const std::string& url) const;
    std::string getCoordSysName(ObserverFrame::CoordinateSystem mode) const;
    std::string getBodyShortName(const std::string& body) const;
    std::string getEncodedObjectName(const Selection& selection);

    void evalName();

    // Variables specific to Global Urls
    UniversalCoord coord;
    astro::Date date;
    Eigen::Quaternionf orientation;

    // Variables specific to Relative Urls
    double distance{ 0.0};
    double longitude{ 0.0 };
    double latitude{ 0.0 };
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
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    CelestiaState() = default;

    bool loadState(Url::ParamsMap& params);
    void saveState(Url::ParamsMap& params);
    void captureState(CelestiaCore* appCore);

    // Observer frame, position, and orientation. For multiview, there needs
    // be one instance of these parameters per view saved.
    ObserverFrame::CoordinateSystem coordSys{ ObserverFrame::Universal };
    string refBodyName;
    string targetBodyName;
    string trackedBodyName;
    UniversalCoord observerPosition{ 0.0, 0.0, 0.0 };
    Eigen::Quaternionf observerOrientation{ Eigen::Quaternionf::Identity() };
    float fieldOfView{ 45.0f };

    // Time parameters
    double tdb{ 0.0 };
    float  timeScale{ 1.0f };
    bool   pauseState{ false };
    bool   lightTimeDelay{ false };

    std::string selectedBodyName;

    int      labelMode{ 0 };
    uint64_t renderFlags{ 0 };
};

#endif
