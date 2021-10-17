// url.h
//
// Copyright (C) 2002-present, the Celestia Development Team
// Original version written by Chris Teyssier (chris@tux.teyssier.org)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <map>
#include <string>
#include <Eigen/Geometry>
#include <celengine/observer.h>
#include <celengine/astro.h>
#include <celcompat/string_view.h>
#include "celestiastate.h"

class CelestiaCore;

class Url
{
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    constexpr static int CurrentVersion = 3;

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

    Url(CelestiaCore *core);
    Url(const CelestiaState& appState,
        int version = CurrentVersion,
        TimeSource timeSource = UseUrlTime);
    Url(const Url&) = default;
    Url(Url&&) = default;
    Url& operator=(const Url&) = default;
    Url& operator=(Url&&) = default;
    ~Url() = default;

    static std::string getEncodedObjectName(const Selection& sel, const CelestiaCore* appCore);
    static constexpr celestia::compat::string_view proto();
    static std::string decodeString(celestia::compat::string_view);
    static std::string encodeString(celestia::compat::string_view);

    bool parse(celestia::compat::string_view);
    bool goTo();
    std::string getAsString() const;

 private:
    bool initVersion3(std::map<celestia::compat::string_view, std::string> &params, celestia::compat::string_view timeStr);
    bool initVersion4(std::map<celestia::compat::string_view, std::string> &params, celestia::compat::string_view timeStr);
    void evalName();

    CelestiaState   m_state;

    std::string     m_url;
    std::string     m_name;
    astro::Date     m_date;
    CelestiaCore   *m_appCore       { nullptr };

    ObserverFrame   m_ref;
    Selection       m_selected;
    Selection       m_tracked;

    int             m_version       { CurrentVersion };
    TimeSource      m_timeSource    { UseUrlTime };

    int             m_nBodies       { -1 };
    bool            m_valid         { false };
};

constexpr celestia::compat::string_view Url::proto()
{
    return "cel://";
}
