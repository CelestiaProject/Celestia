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

#include <map>
#include <string>
#include <Eigen/Geometry>
#include <celengine/observer.h>

class CelestiaCore;

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
    CelestiaState() = default;
    CelestiaState(CelestiaCore* appCore);
    ~CelestiaState() = default;
    // FIXME

    bool loadState(std::map<std::string, std::string> &params);
    void saveState(std::map<std::string, std::string> &params);
    void captureState();

 private:
    // Observer frame, position, and orientation. For multiview, there needs
    // be one instance of these parameters per view saved.
    ObserverFrame::CoordinateSystem m_coordSys              { ObserverFrame::Universal };
    UniversalCoord                  m_observerPosition      { 0.0, 0.0, 0.0 };
    Eigen::Quaternionf              m_observerOrientation   { Eigen::Quaternionf::Identity() };
    float                           m_fieldOfView           { 45.0f };

    // Time parameters
    double                          m_tdb                   { 0.0 };
    float                           m_timeScale             { 1.0f };
    bool                            m_pauseState            { false };
    bool                            m_lightTimeDelay        { false };

    std::string                     m_refBodyName;
    std::string                     m_targetBodyName;
    std::string                     m_trackedBodyName;
    std::string                     m_selectedBodyName;

    int                             m_labelMode             { 0 };
    uint64_t                        m_renderFlags           { 0 };

    CelestiaCore                   *m_appCore               { nullptr };

    friend class Url;
};
