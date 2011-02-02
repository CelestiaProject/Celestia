/***************************************************************************
                          url.cpp  -  description
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

#include <string>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <sstream>
#include <iomanip>
#include "celestiacore.h"
#include "celutil/util.h"
#include "celengine/astro.h"
#include "url.h"

using namespace std;

const unsigned int Url::CurrentVersion = 3;


const string getEncodedObjectName(const Selection& sel, const CelestiaCore* appCore);

CelestiaState::CelestiaState() :
    coordSys(ObserverFrame::Universal),
    observerPosition(0.0, 0.0, 0.0),
    observerOrientation(1.0f),
    fieldOfView(45.0f),
    tdb(0.0),
    timeScale(1.0f),
    pauseState(false),
    lightTimeDelay(false),
    labelMode(0),
    renderFlags(0)
{
}


void
CelestiaState::captureState(CelestiaCore* appCore)
{
    Simulation *sim = appCore->getSimulation();
    Renderer *renderer = appCore->getRenderer();
    
    const ObserverFrame* frame = sim->getFrame();
    
    coordSys = frame->getCoordinateSystem();
    if (coordSys != ObserverFrame::Universal)
    {
        refBodyName = getEncodedObjectName(frame->getRefObject(), appCore);
        if (coordSys == ObserverFrame::PhaseLock) 
        {
            targetBodyName = getEncodedObjectName(frame->getTargetObject(), appCore);
        }
    }
    
    tdb = sim->getTime();
    
    // Store the position and orientation of the observer in the current
    // frame.
    observerPosition = sim->getObserver().getPosition();
    observerPosition = frame->convertFromUniversal(observerPosition, tdb);
        
    Quatd q = sim->getObserver().getOrientation();
    q = frame->convertFromUniversal(q, tdb);
    observerOrientation = Quatf((float) q.w, (float) q.x, (float) q.y, (float) q.z);
        
    Selection tracked = sim->getTrackedObject();
    trackedBodyName = getEncodedObjectName(tracked, appCore);
    Selection selected = sim->getSelection();
    selectedBodyName = getEncodedObjectName(selected, appCore);
    fieldOfView = radToDeg(sim->getActiveObserver()->getFOV());
    timeScale = (float) sim->getTimeScale();
    pauseState = sim->getPauseState();
    lightTimeDelay = appCore->getLightDelayActive();
    renderFlags = renderer->getRenderFlags();
    labelMode = renderer->getLabelMode();
}


#if 0
bool CelestiaState::loadState(std::map<std::string, std::string> params)
{
    sscanf(timeString.c_str(), "%d-%d-%dT%d:%d:%lf",
           &date.year, &date.month, &date.day,
           &date.hour, &date.minute, &date.seconds);
    
    observerPosition = UniversalCoord(BigFix(params["x"]),
                                      BigFix(params["y"]),
                                      BigFix(params["z"]));
    
    float ow = 0.0f;
    float ox = 0.0f;
    float oy = 0.0f;
    float oz = 0.0f;
    if (sscanf(params["ow"].c_str(), "%f", &ow) != 1 ||
        sscanf(params["ox"].c_str(), "%f", &ox) != 1 ||
        sscanf(params["oy"].c_str(), "%f", &oy) != 1 ||
        sscanf(params["oz"].c_str(), "%f", &oz) != 1)
    {
        return false;
    }
    
    orientation = Quatf(ow, ox, oy, oz);
    
    if (params["select"] != "")
        selectedBodyName = params["select"];
    if (params["track"] != "")
        trackedBodyName = params["track"];
    if (params["ltd"] != "")
        lightTimeDelay = (strcmp(params["ltd"].c_str(), "1") == 0);
    else
        lightTimeDelay = false;
    
    if (params["fov"] != "")
    {
        if (sscanf(params["fov"].c_str(), "%f", &fieldOfView) != 1.0f)
            return false;
    }
    
    if (params["ts"] != "") 
    {
        if (sscanf(params["ts"].c_str(), "%f", &timeScale) != 1.0f)
            return false;
    }
    
    int paused = 0;
    if (params["p"] != "")
    {
        if (sscanf(params["p"].c_str(), "%d", &paused) != 1)
            return false;
        if (paused != 0 && paused != 1)
            return false;
        pauseState = paused == 1;
    }
    
    // Render settings
    if (params["rf"] != "")
    {
        if (sscanf(params["rf"].c_str(), "%d", &renderFlags) != 1)
            return false;
    }
    if (params["lm"] != "")
    {
        if (sscanf(params["lm"].c_str(), "%d", &labelMode) != 1)
            return false;
    }
}
#endif


Url::Url()
{};

Url::Url(const std::string& str, CelestiaCore *core):
    urlStr(str),
    appCore(core),
    pauseState(false),
    timeSource(UseUrlTime),
    version(2)
{
    std::string::size_type pos, endPrevious;
    std::vector<Selection> bodies;
    Simulation *sim = appCore->getSimulation();
    std::map<std::string, std::string> params = parseUrlParams(urlStr);

    if (urlStr.substr(0, 6) != "cel://")
    {
        urlStr = "";
        return;
    }

    // Version labelling of cel URLs was only added in Celestia 1.5, cel URL
    // version 2. Assume any URL without a version is version 1.
    if (params["ver"] != "")
    {
        sscanf(params["ver"].c_str(), "%u", &version);
    }
    else
    {
        version = 1;
    }
    
    pos = urlStr.find("/", 6);
    if (pos == std::string::npos)
        pos = urlStr.find("?", 6);

    if (pos == std::string::npos)
        modeStr = urlStr.substr(6);
    else
        modeStr = decodeString(urlStr.substr(6, pos - 6));

    if (!compareIgnoringCase(modeStr, std::string("Freeflight")))
    {
        mode = ObserverFrame::Universal;
        nbBodies = 0;
    }
    else if (!compareIgnoringCase(modeStr, std::string("Follow")))
    {
        mode = ObserverFrame::Ecliptical;
        nbBodies = 1;
    }
    else if (!compareIgnoringCase(modeStr, std::string("SyncOrbit")))
    {
        mode = ObserverFrame::BodyFixed;
        nbBodies = 1;
    }
    else if (!compareIgnoringCase(modeStr, std::string("Chase")))
    {
        mode = ObserverFrame::Chase;
        nbBodies = 1;
    }
    else if (!compareIgnoringCase(modeStr, std::string("PhaseLock")))
    {
        mode = ObserverFrame::PhaseLock;
        nbBodies = 2;
    }
    else if (!compareIgnoringCase(modeStr, std::string("Settings")))
    {
        type = Settings;
        nbBodies = 0;
    }
    
    if (nbBodies == -1)
    {
        urlStr = "";
        return; // Mode not recognized
    }
    
    endPrevious = pos;
    int nb = nbBodies, i=1;
    while (nb != 0 && endPrevious != std::string::npos) {
        std::string bodyName="";
        pos = urlStr.find("/", endPrevious + 1);
        if (pos == std::string::npos) pos = urlStr.find("?", endPrevious + 1);
        if (pos == std::string::npos) bodyName = urlStr.substr(endPrevious + 1);
        else bodyName = urlStr.substr(endPrevious + 1, pos - endPrevious - 1);
        endPrevious = pos;
        
        bodyName = decodeString(bodyName);
        pos = 0;
        if (i==1) body1 = bodyName;
        if (i==2) body2 = bodyName;
        while(pos != std::string::npos) {
            pos = bodyName.find(":", pos + 1);
            if (pos != std::string::npos) bodyName[pos]='/';
        }
        
        bodies.push_back(sim->findObjectFromPath(bodyName));
        
        nb--;
        i++;
    }
    
    if (nb != 0) {
        urlStr = "";
        return; // Number of bodies in Url doesn't match Mode
    }
    
    if (nbBodies == 0) ref = ObserverFrame();
    if (nbBodies == 1) ref = ObserverFrame(mode, bodies[0]);
    if (nbBodies == 2) ref = ObserverFrame(mode, bodies[0], bodies[1]);
    fromString = true;
        
    std::string time="";
    pos = urlStr.find("?", endPrevious + 1);
    if (pos == std::string::npos)
        time = urlStr.substr(endPrevious + 1);
    else time = urlStr.substr(endPrevious + 1, pos - endPrevious -1);
        time = decodeString(time);
        
    switch (version)
    {
    case 1:
    case 2:
        initVersion2(params, time);
        break;
    case 3:
        initVersion3(params, time);
        break;
    default:
        urlStr = "";
        return;
    }

    evalName();
}


Url::Url(CelestiaCore* core, UrlType type)
{
    appCore = core;
    version = 2;
    timeSource = UseUrlTime;
    
    Simulation *sim = appCore->getSimulation();
    Renderer *renderer = appCore->getRenderer();

    this->type = type;

    modeStr = getCoordSysName(sim->getFrame()->getCoordinateSystem());
    if (type == Settings) modeStr = "Settings";
    ref = *sim->getFrame();
    urlStr += "cel://" + modeStr;
    if (type != Settings && sim->getFrame()->getCoordinateSystem() != ObserverFrame::Universal) {
        body1 = getEncodedObjectName(sim->getFrame()->getRefObject());
        urlStr += "/" + body1;
        if (sim->getFrame()->getCoordinateSystem() == ObserverFrame::PhaseLock) {
            body2 = getEncodedObjectName(sim->getFrame()->getTargetObject());
            urlStr += "/" + body2;
        }
    }

    double simTime = sim->getTime();
    
    char date_str[50];
    date = astro::Date(simTime);
    char buff[255];

    switch (type) {
    case Absolute:
        snprintf(date_str, sizeof(date_str), "%04d-%02d-%02dT%02d:%02d:%08.5f",
            date.year, date.month, date.day, date.hour, date.minute, date.seconds);

        coord = sim->getObserver().getPosition();
        urlStr += std::string("/") + date_str + "?x=" + coord.x.toString();
        urlStr += "&y=" +  coord.y.toString();
        urlStr += "&z=" +  coord.z.toString();

        orientation = sim->getObserver().getOrientationf();
        sprintf(buff, "&ow=%f&ox=%f&oy=%f&oz=%f", orientation.w, orientation.x, orientation.y, orientation.z);
        urlStr += buff;
        break;
    case Relative:
        sim->getSelectionLongLat(distance, longitude, latitude);
        sprintf(buff, "dist=%f&long=%f&lat=%f", distance, longitude, latitude);
        urlStr += std::string("/?") + buff;
        break;
    case Settings:
        urlStr += std::string("/?");
        break;
    }

    switch (type) {
    case Absolute: // Intentional Fall-Through
    case Relative:
        tracked = sim->getTrackedObject();
        trackedStr = getEncodedObjectName(tracked);
        if (trackedStr != "") urlStr += "&track=" + trackedStr;

        selected = sim->getSelection();
        selectedStr = getEncodedObjectName(selected);
        if (selectedStr != "") urlStr += "&select=" + selectedStr;

        fieldOfView = radToDeg(sim->getActiveObserver()->getFOV());
        timeScale = (float) sim->getTimeScale();
        pauseState = sim->getPauseState();
        lightTimeDelay = appCore->getLightDelayActive();
        sprintf(buff, "&fov=%f&ts=%f&ltd=%c&p=%c&", fieldOfView,
            timeScale, lightTimeDelay?'1':'0', pauseState?'1':'0');
        urlStr += buff;
    case Settings: // Intentional Fall-Through
        renderFlags = renderer->getRenderFlags();
        labelMode = renderer->getLabelMode();
        sprintf(buff, "rf=%d&lm=%d", renderFlags, labelMode);
        urlStr += buff;
        break;
    }

    // Append the Celestia URL version
    {
        char buf[32];
        sprintf(buf, "&ver=%u", version);
        urlStr += buf;
    }

    evalName();
}


/*! Construct a new cel URL from a saved CelestiaState object. This method may
 *  may only be called to create a version 3 or later url.
 */
Url::Url(const CelestiaState& appState, unsigned int _version, TimeSource _timeSource)
{
    ostringstream u;
    
    appCore = NULL;
    
    assert(_version >= 3);
    version = _version;
    timeSource = _timeSource;
    type = Absolute;
    
    modeStr      = getCoordSysName(appState.coordSys);
    body1        = appState.refBodyName;
    body2        = appState.targetBodyName;
    selectedStr  = appState.selectedBodyName;
    trackedStr   = appState.trackedBodyName;
    
    coord        = appState.observerPosition;
    orientation  = appState.observerOrientation;
    
    //ref =
    //selected =
    //tracked =
    nbBodies = 1;
    if (appState.coordSys == ObserverFrame::Universal)
        nbBodies = 0;
    else if (appState.coordSys ==  ObserverFrame::PhaseLock)
        nbBodies = 2;
    
    fieldOfView      = appState.fieldOfView;
    renderFlags      = appState.renderFlags;
    labelMode        = appState.labelMode;
    
    date             = astro::Date(appState.tdb);
    timeScale        = appState.timeScale;
    pauseState       = appState.pauseState;
    lightTimeDelay   = appState.lightTimeDelay;    
    
    u << "cel://" << modeStr;
    
    if (appState.coordSys != ObserverFrame::Universal)
    {
        u << "/" << appState.refBodyName;
        if (appState.coordSys == ObserverFrame::PhaseLock)
        {
            u << "/" << appState.targetBodyName;
        }
    }
        
    char date_str[50];
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02dT%02d:%02d:%08.5f",
             date.year, date.month, date.day, date.hour, date.minute, date.seconds);
    u << "/" << date_str;
        
    // observer position
    u << "?x=" << coord.x.toString() << "&y=" << coord.y.toString() << "&z=" << coord.z.toString();
    
    // observer orientation
    u << "&ow=" << orientation.w
      << "&ox=" << orientation.x
      << "&oy=" << orientation.y
      << "&oz=" << orientation.z;
    
    if (trackedStr != "") 
        u << "&track=" << trackedStr;
    if (selectedStr != "")
        u << "&select=" << selectedStr;

    u << "&fov=" << fieldOfView;
    u << "&ts=" << timeScale;
    u << "&ltd=" << (lightTimeDelay ? 1 : 0);
    u << "&p=" << (pauseState ? 1 : 0);

    u << "&rf=" << renderFlags;
    u << "&lm=" << labelMode;

    // Append the url settings: time source and version
    u << "&tsrc=" << (int) timeSource;
    u << "&ver=" << version;

    urlStr = u.str();

    evalName();
}


void Url::initVersion2(std::map<std::string, std::string>& params,
                       const std::string& timeString) 
{
    if (type != Settings)
    {
        if (params["dist"] != "")
            type = Relative;
        else
            type = Absolute;
    }
    
    switch (type) {
        case Absolute:
            date = astro::Date(0.0);
            sscanf(timeString.c_str(), "%d-%d-%dT%d:%d:%lf",
                   &date.year, &date.month, &date.day,
                   &date.hour, &date.minute, &date.seconds);
            
            coord = UniversalCoord(BigFix(params["x"]),
                                   BigFix(params["y"]),
                                   BigFix(params["z"]));
            
            float ow, ox, oy, oz;
            sscanf(params["ow"].c_str(), "%f", &ow);
            sscanf(params["ox"].c_str(), "%f", &ox);
            sscanf(params["oy"].c_str(), "%f", &oy);
            sscanf(params["oz"].c_str(), "%f", &oz);
            
            orientation = Quatf(ow, ox, oy, oz);
            
            // Intentional Fall-Through
        case Relative:
            if (params["dist"] != "") {
                sscanf(params["dist"].c_str(), "%lf", &distance);
            }
            if (params["long"] != "") {
                sscanf(params["long"].c_str(), "%lf", &longitude);
            }
            if (params["lat"] != "") {
                sscanf(params["lat"].c_str(), "%lf", &latitude);
            }
            if (params["select"] != "") {
                selectedStr = params["select"];
            }
            if (params["track"] != "") {
                trackedStr = params["track"];
            }
            if (params["ltd"] != "") {
                lightTimeDelay = (strcmp(params["ltd"].c_str(), "1") == 0);
            } else {
                lightTimeDelay = false;
            }
            if (params["fov"] != "") {
                sscanf(params["fov"].c_str(), "%f", &fieldOfView);
            }
            if (params["ts"] != "") {
                sscanf(params["ts"].c_str(), "%f", &timeScale);
            }
            if (params["p"] != "") {
                int pauseInt = 0;
                sscanf(params["p"].c_str(), "%d", &pauseInt);
                pauseState = pauseInt == 1;
            }
            break;
        case Settings:
            break;
    }
    
    if (params["rf"] != "") {
        sscanf(params["rf"].c_str(), "%d", &renderFlags);
    }
    if (params["lm"] != "") {
        sscanf(params["lm"].c_str(), "%d", &labelMode);
    }
    
}


void Url::initVersion3(std::map<std::string, std::string>& params,
                       const std::string& timeString)
{
    // Type field not used for version 3 urls; position is always relative
    // to the frame center. Time setting is controlled by the time source.
    type = Absolute;
    
    date = astro::Date(0.0);
    sscanf(timeString.c_str(), "%d-%d-%dT%d:%d:%lf",
           &date.year, &date.month, &date.day,
           &date.hour, &date.minute, &date.seconds);
    
    coord = UniversalCoord(BigFix(params["x"]),
                           BigFix(params["y"]),
                           BigFix(params["z"]));
    
    float ow, ox, oy, oz;
    sscanf(params["ow"].c_str(), "%f", &ow);
    sscanf(params["ox"].c_str(), "%f", &ox);
    sscanf(params["oy"].c_str(), "%f", &oy);
    sscanf(params["oz"].c_str(), "%f", &oz);
    
    orientation = Quatf(ow, ox, oy, oz);
    
    if (params["select"] != "")
        selectedStr = params["select"];
    if (params["track"] != "")
        trackedStr = params["track"];
    if (params["ltd"] != "")
        lightTimeDelay = (strcmp(params["ltd"].c_str(), "1") == 0);
    else
        lightTimeDelay = false;
    
    if (params["fov"] != "")
        sscanf(params["fov"].c_str(), "%f", &fieldOfView);
    if (params["ts"] != "") 
        sscanf(params["ts"].c_str(), "%f", &timeScale);
    
    int paused = 0;
    if (params["p"] != "")
        sscanf(params["p"].c_str(), "%d", &paused);
    pauseState = paused == 1;

    // Render settings
    if (params["rf"] != "")
        sscanf(params["rf"].c_str(), "%d", &renderFlags);
    if (params["lm"] != "")
        sscanf(params["lm"].c_str(), "%d", &labelMode);
        
    int timeSourceInt = 0;
    if (params["tsrc"] != "")
        sscanf(params["tsrc"].c_str(), "%d", &timeSourceInt);
    if (timeSourceInt >= 0 && timeSourceInt < TimeSourceCount)
        timeSource = (TimeSource) timeSourceInt;
    else
        timeSource = UseUrlTime;
}


std::string Url::getAsString() const
{
    return urlStr;
}


std::string Url::getName() const
{
    return name;
}


void Url::evalName()
{
    char buff[50];
    double lo = longitude, la = latitude;
    char los = 'E';
    char las = 'N';
    switch(type) {
    case Absolute:
        name = _(modeStr.c_str());
        if (body1 != "") name += " " + std::string(_(getBodyShortName(body1).c_str()));
        if (body2 != "") name += " " + std::string(_(getBodyShortName(body2).c_str()));
        if (trackedStr != "") name += " -> " + std::string(_(getBodyShortName(trackedStr).c_str()));
        if (selectedStr != "") name += " [" + std::string(_(getBodyShortName(selectedStr).c_str())) + "]";
        break;
    case Relative:
        if (selectedStr != "") name = std::string(_(getBodyShortName(selectedStr).c_str())) + " ";
        if (lo < 0) { lo = -lo; los = 'W'; }
        if (la < 0) { la = -la; las = 'S'; }
        sprintf(buff, "(%.1lf%c, %.1lf%c)", lo, los, la, las);
        name += buff;
        break;
    case Settings:
        name = _("Settings");
        break;
    }
}


std::string Url::getBodyShortName(const std::string& body) const
{
    std::string::size_type pos;
    if (body != "") {
        pos = body.rfind(":");
        if (pos != std::string::npos) return body.substr(pos+1);
        else return body;
    }
    return "";
}


std::map<std::string, std::string> Url::parseUrlParams(const std::string& url) const
{
    std::string::size_type pos, startName, startValue;
    std::map<std::string, std::string> params;

    pos = url.find("?");
    while (pos != std::string::npos) {
        startName = pos + 1;
        startValue = url.find("=", startName);
        pos = url.find("&", pos + 1);
        if (startValue != std::string::npos) {
             startValue++;
             if (pos != std::string::npos)
                 params[url.substr(startName, startValue - startName -1)] = decodeString(url.substr(startValue, pos - startValue));
             else
                 params[url.substr(startName, startValue - startName -1)] = decodeString(url.substr(startValue));
        }
    }

    return params;
}


std::string Url::getCoordSysName(ObserverFrame::CoordinateSystem mode) const
{
    switch (mode)
    {
    case ObserverFrame::Universal:
        return "Freeflight";
    case ObserverFrame::Ecliptical:
        return "Follow";
    case ObserverFrame::BodyFixed:
        return "SyncOrbit";
    case ObserverFrame::Chase:
        return "Chase";
    case ObserverFrame::PhaseLock:
        return "PhaseLock";
    case ObserverFrame::Equatorial:
        return "Unknown";
    case ObserverFrame::ObserverLocal:
        return "Unknown";
    default:
        return "Unknown";
    }
}


static std::string getBodyName(Universe* universe, Body* body)
{
    std::string name = body->getName();
    PlanetarySystem* parentSystem = body->getSystem();
    const Body* parentBody = NULL;

    if (parentSystem != NULL)
        parentBody = parentSystem->getPrimaryBody();
    else
        assert(0);
        // TODO: Figure out why the line below was added.
        //parentBody = body->getOrbitBarycenter();  

    while (parentBody != NULL)
    {
        name = parentBody->getName() + ":" + name;
        parentSystem = parentBody->getSystem();
        if (parentSystem == NULL)
            parentBody = NULL;
        else
            parentBody = parentSystem->getPrimaryBody();
    }

    if (body->getSystem()->getStar() != NULL)
    {
        name = universe->getStarCatalog()->getStarName(*(body->getSystem()->getStar())) + ":" + name;
    }

    return name;
}


void Url::goTo()
{
    Selection sel;

    if (urlStr == "")
        return;
    Simulation *sim = appCore->getSimulation();
    Renderer *renderer = appCore->getRenderer();
    std::string::size_type pos;

    sim->update(0.0);

    switch(type) {
    case Absolute:// Intentional Fall-Through
    case Relative:
        sim->setFrame(ref.getCoordinateSystem(), ref.getRefObject(), ref.getTargetObject());
        sim->getActiveObserver()->setFOV(degToRad(fieldOfView));
        appCore->setZoomFromFOV();
        sim->setTimeScale(timeScale);
        sim->setPauseState(pauseState);
        appCore->setLightDelayActive(lightTimeDelay);

        if (selectedStr != "")
        {
            pos = 0;
            while(pos != std::string::npos)
            {
                pos = selectedStr.find(":", pos + 1);
                if (pos != std::string::npos) selectedStr[pos]='/';
            }
            sel = sim->findObjectFromPath(selectedStr);
            sim->setSelection(sel);
        }
        else
        {
            sim->setSelection(Selection());
        }

        if (trackedStr != "")
        {
            pos = 0;
            while(pos != std::string::npos)
            {
                pos = trackedStr.find(":", pos + 1);
                if (pos != std::string::npos) trackedStr[pos]='/';
            }
            sel = sim->findObjectFromPath(trackedStr);
            sim->setTrackedObject(sel);
        }
        else
        {
            if (!sim->getTrackedObject().empty())
                sim->setTrackedObject(Selection());
        }
        // Intentional Fall-Through
    case Settings:
        renderer->setRenderFlags(renderFlags);
        renderer->setLabelMode(labelMode);
        break;
    }

    if (version >= 3)
    {
        switch (timeSource)
        {
            case UseUrlTime:
                sim->setTime((double) date);
                break;
            case UseSimulationTime:
                // Leave the current simulation time unmodified
                break;
            case UseSystemTime:
                sim->setTime(astro::UTCtoTDB(astro::Date::systemDate()));
                break;
            default:
                break;
        }
        
        // Position and orientation stored in frame coordinates; convert them
        // to universal and set the observer position.
        double tdb = sim->getTime();
        coord = sim->getObserver().getFrame()->convertToUniversal(coord, tdb);
        Quatd q(orientation.w, orientation.x, orientation.y, orientation.z);
        q = sim->getObserver().getFrame()->convertToUniversal(q, tdb);
        sim->setObserverPosition(coord);
        sim->setObserverOrientation(Quatf((float) q.w, (float) q.x, (float) q.y, (float) q.z));
    }
    else
    {
        switch(type) {
        case Absolute:
            sim->setTime((double) date);
            sim->setObserverPosition(coord);
            sim->setObserverOrientation(orientation);
            break;
        case Relative:
            sim->gotoSelectionLongLat(0, astro::kilometersToLightYears(distance), (float) (longitude * PI / 180), (float) (latitude * PI / 180), Vec3f(0, 1, 0));
            break;
        case Settings:
            break;
        }
    }
}


Url::~Url()
{
}


std::string Url::decodeString(const std::string& str)
{
    std::string::size_type a=0, b;
    std::string out = "";

    b = str.find("%");
    while (b != std::string::npos)
    {
        unsigned int c;
        out += str.substr(a, b-a);
        std::string c_code = str.substr(b+1, 2);
        sscanf(c_code.c_str(), "%02x", &c);
        out += (char) c;
        a = b + 3;
        b = str.find("%", a);
    }
    out += str.substr(a);

    return out;
}


string Url::encodeString(const string& str)
{
    ostringstream enc;

    for (string::const_iterator iter = str.begin(); iter != str.end(); iter++)
    {
        int ch = (unsigned char) *iter;
        bool encode = false;
        if (ch <= 32 || ch >= 128)
        {
            encode = true;
        }
        else
        {
            switch (ch)
            {
            case '%':
            case '?':
            case '"':
            case '#':
            case '+':
            case ',':
            case '=':
            case '@':
            case '[':
            case ']':
                encode = true;
                break;
            }
        }

        if (encode)
        {
            enc << '%' << setw(2) << hex << ch;
        }
        else
        {
            enc << *iter;
        }
    }

    return enc.str();
}


// Utility function that returns the complete path for a selection.
string
Url::getEncodedObjectName(const Selection& selection)
{
    return ::getEncodedObjectName(selection, appCore);
}


const string
getEncodedObjectName(const Selection& selection, const CelestiaCore* appCore)
{
    Universe *universe = appCore->getSimulation()->getUniverse();
    string name;
    
    switch (selection.getType())
    {
        case Selection::Type_Body:
            name = getBodyName(universe, selection.body());
            break;
            
        case Selection::Type_Star:
            name = universe->getStarCatalog()->getStarName(*selection.star());
            break;

        case Selection::Type_DeepSky:
            name = universe->getDSOCatalog()->getDSOName(selection.deepsky());
            break;

        case Selection::Type_Location:
            name = selection.location()->getName();
            {
                Body* parentBody = selection.location()->getParentBody();
                if (parentBody != NULL)
                    name = getBodyName(universe, parentBody) + ":" + name;
            }
            break;
            
        default:
            return "";
    }

    return Url::encodeString(name);
}
