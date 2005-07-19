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
#include <stdio.h>
#include "celestiacore.h"
#include "celengine/astro.h"
#include "url.h"

Url::Url()
{};

Url::Url(const std::string& str, CelestiaCore *core)
{
    urlStr = str;
    appCore = core;
    std::string::size_type pos, endPrevious;
    std::vector<Selection> bodies;
    Simulation *sim = appCore->getSimulation();
    std::map<std::string, std::string> params = parseUrlParams(urlStr);

    if (urlStr.substr(0, 6) != "cel://")
    {
        urlStr = "";
        return;
    }
    
    pos = urlStr.find("/", 6);
    if (pos == std::string::npos) pos = urlStr.find("?", 6);

    if (pos == std::string::npos) modeStr = urlStr.substr(6);
    else modeStr = decode_string(urlStr.substr(6, pos - 6));


    if (!compareIgnoringCase(modeStr, std::string("Freeflight")))
    {
        mode = astro::Universal;
        nbBodies = 0;
    }
    else if (!compareIgnoringCase(modeStr, std::string("Follow")))
    {
        mode = astro::Ecliptical;
        nbBodies = 1;
    }
    else if (!compareIgnoringCase(modeStr, std::string("SyncOrbit")))
    {
        mode = astro::Geographic;
        nbBodies = 1;
    }
    else if (!compareIgnoringCase(modeStr, std::string("Chase")))
    {
        mode = astro::Chase;
        nbBodies = 1;
    }
    else if (!compareIgnoringCase(modeStr, std::string("PhaseLock")))
    {
        mode = astro::PhaseLock;
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

        bodyName = decode_string(bodyName);
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

    if (nbBodies == 0) ref = FrameOfReference();
    if (nbBodies == 1) ref = FrameOfReference(mode, bodies[0]);
    if (nbBodies == 2) ref = FrameOfReference(mode, bodies[0], bodies[1]);
    fromString = true;

    std::string time="";
    pos = urlStr.find("?", endPrevious + 1);
    if (pos == std::string::npos) time = urlStr.substr(endPrevious + 1);
    else time = urlStr.substr(endPrevious + 1, pos - endPrevious -1);
    time = decode_string(time);
    
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
        sscanf(time.c_str(), "%d-%d-%dT%d:%d:%lf",
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

    evalName();
}

Url::Url(CelestiaCore* core, UrlType type) {
    appCore = core;
    Simulation *sim = appCore->getSimulation();
    Renderer *renderer = appCore->getRenderer();

    this->type = type;

    modeStr = getCoordSysName(sim->getFrame().coordSys);
    if (type == Settings) modeStr = "Settings";
    ref = sim->getFrame();
    urlStr += "cel://" + modeStr;
    if (type != Settings && sim->getFrame().coordSys != astro::Universal) {
        body1 = getSelectionName(sim->getFrame().refObject);
        urlStr += "/" + body1;
        if (sim->getFrame().coordSys == astro::PhaseLock) {
            body2 = getSelectionName(sim->getFrame().targetObject);
            urlStr += "/" + body2;
        }
    }

    char date_str[30];
    date = astro::Date(sim->getTime());
    char buff[255];

    switch (type) {
    case Absolute:
        sprintf(date_str, "%04d-%02d-%02dT%02d:%02d:%08.5f",
            date.year, date.month, date.day, date.hour, date.minute, date.seconds);

        coord = sim->getObserver().getPosition();
        urlStr += std::string("/") + date_str + "?x=" + coord.x.toString();
        urlStr += "&y=" +  coord.y.toString();
        urlStr += "&z=" +  coord.z.toString();

        orientation = sim->getObserver().getOrientation();
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
        trackedStr = getSelectionName(tracked);
        if (trackedStr != "") urlStr += "&track=" + trackedStr;

        selected = sim->getSelection();
        selectedStr = getSelectionName(selected);
        if (selectedStr != "") urlStr += "&select=" + selectedStr;

        fieldOfView = radToDeg(sim->getActiveObserver()->getFOV());
        timeScale = sim->getTimeScale();
        lightTimeDelay = appCore->getLightDelayActive();
        sprintf(buff, "&fov=%f&ts=%f&ltd=%c&", fieldOfView,
            timeScale, lightTimeDelay?'1':'0');
        urlStr += buff;
    case Settings: // Intentional Fall-Through
        renderFlags = renderer->getRenderFlags();
        labelMode = renderer->getLabelMode();
        sprintf(buff, "rf=%d&lm=%d", renderFlags, labelMode);
        urlStr += buff;
        break;
    }

    evalName();
}

std::string Url::getAsString() const {
    return urlStr;
}

std::string Url::getName() const {
    return name;
}

void Url::evalName() {
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

std::string Url::getBodyShortName(const std::string& body) const {
    std::string::size_type pos;
    if (body != "") {
        pos = body.rfind(":");
        if (pos != std::string::npos) return body.substr(pos+1);
        else return body;
    }
    return "";
}

std::map<std::string, std::string> Url::parseUrlParams(const std::string& url) const{
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
                 params[url.substr(startName, startValue - startName -1)] = decode_string(url.substr(startValue, pos - startValue));
             else
                 params[url.substr(startName, startValue - startName -1)] = decode_string(url.substr(startValue));
        }
    }

    return params;
}

std::string Url::getCoordSysName(astro::CoordinateSystem mode) const
{
    switch (mode)
    {
    case astro::Universal:
        return "Freeflight";
    case astro::Ecliptical:
        return "Follow";
    case astro::Geographic:
        return "SyncOrbit";
    case astro::Chase:
        return "Chase";
    case astro::PhaseLock:
        return "PhaseLock";
    case astro::Equatorial:
        return "Unknown";
    case astro::ObserverLocal:
        return "Unknown";
    }
    return "Unknown";
}


std::string Url::getSelectionName(const Selection& selection) const
{
    PlanetarySystem* parentSystem;
    Body* parentBody;
    Universe *universe = appCore->getSimulation()->getUniverse();

    switch (selection.getType())
    {
    case Selection::Type_Body:
        {
            std::string name = selection.body()->getName();
            parentSystem = selection.body()->getSystem();
            if (parentSystem != NULL && (parentBody = parentSystem->getPrimaryBody()) != NULL)
            {
                while (parentSystem != NULL && parentBody != NULL)
                {
                    name = parentSystem->getPrimaryBody()->getName() + ":" + name;
                    parentSystem = parentSystem->getPrimaryBody()->getSystem();
                    parentBody = parentSystem->getPrimaryBody();
                }
                if (selection.body()->getSystem()->getStar() != NULL)
                {
                    name=universe->getStarCatalog()->getStarName(*(selection.body()->getSystem()->getStar()))
                        + ":" + name;
                }
            }
            return name;
        }

    case Selection::Type_Star:
        return universe->getStarCatalog()->getStarName(*selection.star());

    case Selection::Type_DeepSky:
        return selection.deepsky()->getName();

    case Selection::Type_Location:
        return "";

    default:
        return "";
    }
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
        sim->setFrame(ref);
        sim->getActiveObserver()->setFOV(degToRad(fieldOfView));
        appCore->setZoomFromFOV();
        sim->setTimeScale(timeScale);
        appCore->setLightDelayActive(lightTimeDelay);

        pos = 0;
        while(pos != std::string::npos)
        {
            pos = selectedStr.find(":", pos + 1);
            if (pos != std::string::npos) selectedStr[pos]='/';
        }
        sel = sim->findObjectFromPath(selectedStr);
        sim->setSelection(sel);

        pos = 0;
        while(pos != std::string::npos)
        {
            pos = trackedStr.find(":", pos + 1);
            if (pos != std::string::npos) trackedStr[pos]='/';
        }
        sel = sim->findObjectFromPath(trackedStr);
        sim->setTrackedObject(sel);
        // Intentional Fall-Through
    case Settings:
        renderer->setRenderFlags(renderFlags);
        renderer->setLabelMode(labelMode);
        break;
    }

    switch(type) {
    case Absolute:
        sim->setTime((double) date);
        sim->setObserverPosition(coord);
        sim->setObserverOrientation(orientation);
        break;
    case Relative:
        sim->gotoSelectionLongLat(0, astro::kilometersToLightYears(distance), longitude * PI / 180, latitude * PI / 180, Vec3f(0, 1, 0));
        break;
    case Settings:
        break;
    }
}

Url::~Url()
{
}

std::string Url::decode_string(const std::string& str)
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
        out += c;
        a = b + 3;
        b = str.find("%", a);
    }
    out += str.substr(a);
                      
    return out;
}





