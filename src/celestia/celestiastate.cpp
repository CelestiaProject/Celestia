// celestiastate.cpp
//
// Copyright (C) 2002-present, the Celestia Development Team
// Original version written by Chris Teyssier (chris@tux.teyssier.org)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celestiacore.h"
#include "celestiastate.h"
#include "url.h"

using namespace celmath;


CelestiaState::CelestiaState(CelestiaCore* appCore) :
    m_appCore(appCore)
{
}

void CelestiaState::captureState()
{
    auto *sim = m_appCore->getSimulation();
    auto *renderer = m_appCore->getRenderer();

    auto frame = sim->getFrame();

    m_coordSys = frame->getCoordinateSystem();
    if (m_coordSys != ObserverFrame::Universal)
    {
        m_refBodyName = Url::getEncodedObjectName(frame->getRefObject(), m_appCore);
        if (m_coordSys == ObserverFrame::PhaseLock)
            m_targetBodyName = Url::getEncodedObjectName(frame->getTargetObject(), m_appCore);
    }

    m_tdb = sim->getTime();

    // Store the position and orientation of the observer in the current
    // frame.
    m_observerPosition = sim->getObserver().getPosition();
    m_observerPosition = frame->convertFromUniversal(m_observerPosition, m_tdb);

    Eigen::Quaterniond q = sim->getObserver().getOrientation();
    q = frame->convertFromUniversal(q, m_tdb);
    m_observerOrientation = q.cast<float>();

    auto tracked = sim->getTrackedObject();
    m_trackedBodyName = Url::getEncodedObjectName(tracked, m_appCore);
    auto selected = sim->getSelection();
    m_selectedBodyName = Url::getEncodedObjectName(selected, m_appCore);
    m_fieldOfView = radToDeg(sim->getActiveObserver()->getFOV());
    m_timeScale = static_cast<float>(sim->getTimeScale());
    m_pauseState = sim->getPauseState();
    m_lightTimeDelay = m_appCore->getLightDelayActive();
    m_renderFlags = renderer->getRenderFlags();
    m_labelMode = renderer->getLabelMode();
}
