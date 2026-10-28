// celestiastate.h
//
// Copyright (C) 2002-present, the Celestia Development Team
// Original version written by Chris Teyssier (chris@tux.teyssier.org)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include <Eigen/Geometry>

#include <celengine/observer.h>
#include <celengine/renderflags.h>
#include <celengine/selection.h>

class CelestiaCore;

namespace celestia
{

/*! The TimeSource specifies what the time will be set to when the user
 *  activates the URL.
 *  - UseUrlTime indicates that the simulation time should be set to whatever
 *    value was stored in the URL.
 *  - UseSimulationTime means that the simulation time at activation is not
 *    changed.
 *  - UseSystemTime means that the simulation time will be set to whatever the
 *    current system time is when the URL is activated.
 */
enum class UrlTimeSource : int
{
    UseUrlTime        = 0,
    UseSimulationTime = 1,
    UseSystemTime     = 2,
    _Max,
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
    static CelestiaState capture(CelestiaCore& appCore);
    static std::optional<CelestiaState> parseURL(CelestiaCore& appCore, std::string_view urlStr);

    std::string createURL(UrlTimeSource timeSource = UrlTimeSource::UseUrlTime) const;
    void apply() const;

private:
    class Parser;

    struct UrlTime { double urlTime{ 0.0 }; };
    struct SimulationTime {};
    struct SystemTime {};

    explicit CelestiaState(CelestiaCore&);

    // Observer frame, position, and orientation. For multiview, there needs
    // be one instance of these parameters per view saved.
    ObserverFrame::CoordinateSystem m_coordSys              { ObserverFrame::CoordinateSystem::Universal };
    UniversalCoord                  m_observerPosition      { 0.0, 0.0, 0.0 };
    Eigen::Quaternionf              m_observerOrientation   { Eigen::Quaternionf::Identity() };
    float                           m_fieldOfView           { 45.0f };

    // Time parameters
    std::variant<SimulationTime, UrlTime, SystemTime> m_time;
    float                           m_timeScale             { 1.0f };
    bool                            m_pauseState            { false };
    bool                            m_lightTimeDelay        { false };

    Selection                       m_refBody;
    Selection                       m_targetBody;
    Selection                       m_trackedBody;
    Selection                       m_selectedBody;

    RenderLabels                    m_labelMode             { RenderLabels::NoLabels };
    RenderFlags                     m_renderFlags           { RenderFlags::ShowNothing };

    CelestiaCore& m_appCore;
};

} // end namespace celestia
