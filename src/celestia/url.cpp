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


    if (modeStr == "Freeflight") 
    {
        mode = astro::Universal;
        nbBodies = 0;
    }
    else if (modeStr == "Follow") 
    {
        mode = astro::Ecliptical;
        nbBodies = 1;
    }
    else if (modeStr == "SyncOrbit")
    {
        mode = astro::Geographic;
        nbBodies = 1;
    }
    else if (modeStr == "Chase")
    {
        mode = astro::Chase;
        nbBodies = 1;
    }
    else if (modeStr == "PhaseLock")
    {
        mode = astro::PhaseLock;
        nbBodies = 2;
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

    int t;
    date = astro::Date(0.0);
    sscanf(time.substr(0, 4).c_str(), "%04d", &t);
    date.year=t;
    sscanf(time.substr(5, 2).c_str(), "%02d", &t);
    date.month=t;
    sscanf(time.substr(8, 2).c_str(), "%02d", &t);
    date.day=t;
    sscanf(time.substr(11, 2).c_str(), "%02d", &t);
    date.hour=t;
    sscanf(time.substr(14, 2).c_str(), "%02d", &t);
    date.minute=t;
    float s;
    sscanf(time.substr(17, 5).c_str(), "%f", &s);
    date.seconds=s;

    BigFix x(params["x"].c_str()), y(params["y"].c_str()), z(params["z"].c_str());
    coord = UniversalCoord(x,y,z);

    float ow, ox, oy, oz;
    sscanf(params["ow"].c_str(), "%f", &ow);
    sscanf(params["ox"].c_str(), "%f", &ox);
    sscanf(params["oy"].c_str(), "%f", &oy);
    sscanf(params["oz"].c_str(), "%f", &oz);

    orientation = Quatf(ow, ox, oy, oz);

    if (params["fov"] != "") {
        sscanf(params["fov"].c_str(), "%f", &fieldOfView);
    }
    if (params["ts"] != "") {
        sscanf(params["ts"].c_str(), "%f", &timeScale);
    }
    if (params["rf"] != "") {
        sscanf(params["rf"].c_str(), "%d", &renderFlags);
    }
    if (params["lm"] != "") {
        sscanf(params["lm"].c_str(), "%d", &labelMode);
    }
    if (params["select"] != "") {
        selectedStr = params["select"];
    }
    if (params["track"] != "") {
        trackedStr = params["track"];
    }

    name = modeStr;
    if (body1 != "") name += " " + getBodyShortName(body1);
    if (body2 != "") name += " " + getBodyShortName(body2);
    if (trackedStr != "") name += " -> " + getBodyShortName(trackedStr);
    if (selectedStr != "") name += " [" + getBodyShortName(selectedStr) + "]";
}

Url::Url(CelestiaCore* core) {
    appCore = core;
    Simulation *sim = appCore->getSimulation();
    Renderer *renderer = appCore->getRenderer();

    modeStr = getCoordSysName(sim->getFrame().coordSys);
    ref = sim->getFrame();
    urlStr += "cel://" + modeStr;
    if (sim->getFrame().coordSys != astro::Universal) {
        body1 = getSelectionName(sim->getFrame().refObject);
        urlStr += "/" + body1;
        if (sim->getFrame().coordSys == astro::PhaseLock) {
            body2 = getSelectionName(sim->getFrame().targetObject);
            urlStr += "/" + body2;
        }
    }

    char date_str[30];
    date = astro::Date(sim->getTime());
    sprintf(date_str, "%04d-%02d-%02dT%02d:%02d:%05.2f",
        date.year, date.month, date.day, date.hour, date.minute, date.seconds);

    coord = sim->getObserver().getPosition();
    urlStr += std::string("/") + date_str + "?x=" + coord.x.toString();
    urlStr += "&y=" +  coord.y.toString();
    urlStr += "&z=" +  coord.z.toString();

    orientation = sim->getObserver().getOrientation();
    char buff[255];
    sprintf(buff, "&ow=%f&ox=%f&oy=%f&oz=%f", orientation.w, orientation.x, orientation.y, orientation.z);
    urlStr += buff;

    tracked = sim->getTrackedObject();
    trackedStr = getSelectionName(tracked);
    if (trackedStr != "") urlStr += "&track=" + trackedStr;

    selected = sim->getSelection();
    selectedStr = getSelectionName(selected);
    if (selectedStr != "") urlStr += "&select=" + selectedStr;

    fieldOfView = renderer->getFieldOfView();
    timeScale = sim->getTimeScale();
    renderFlags = renderer->getRenderFlags();
    labelMode = renderer->getLabelMode();
    sprintf(buff, "&fov=%f&ts=%f&rf=%d&lm=%d", fieldOfView,
        timeScale, renderFlags, labelMode);
    urlStr += buff; 

    name = modeStr;
    if (body1 != "") name += " " + getBodyShortName(body1);
    if (body2 != "") name += " " + getBodyShortName(body2);
    if (trackedStr != "") name += " -> " + getBodyShortName(trackedStr);
    if (selectedStr != "") name += " [" + getBodyShortName(selectedStr) + "]";

}

std::string Url::getAsString() const {
    return urlStr;
}

std::string Url::getName() const {
    return name;
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
    }
    return "Unknown";
}


std::string Url::getSelectionName(const Selection& selection) const
{
    Universe *universe = appCore->getSimulation()->getUniverse();
    if (selection.body != 0)
    {
        std::string name=selection.body->getName();
        if (selection.body->getSystem() != 0)
        {
            if (selection.body->getSystem()->getPrimaryBody() != 0)
            {
                name=selection.body->getSystem()->getPrimaryBody()->getName() + ":" + name;
            }
            if (selection.body->getSystem()->getStar() != 0)
            {
                name=universe->getStarCatalog()->getStarName(*(selection.body->getSystem()->getStar()))
                    + ":" + name;
            }
        }

        return name;
    }
    if (selection.star != 0) return universe->getStarCatalog()->getStarName(*selection.star);
    if (selection.galaxy != 0) return selection.galaxy->getName();
    return "";
}

void Url::goTo()
{
    if (urlStr == "")
        return;
    Simulation *sim = appCore->getSimulation();
    Renderer *renderer = appCore->getRenderer();
    std::string::size_type pos;

    sim->setTime((double) date);
    sim->update(0.0);
    sim->setFrame(ref);
    sim->setObserverPosition(coord);
    sim->setObserverOrientation(orientation);
    renderer->setFieldOfView(fieldOfView);
    sim->setTimeScale(timeScale);
    renderer->setRenderFlags(renderFlags);
    renderer->setLabelMode(labelMode);

    pos = 0;
    while(pos != std::string::npos)
    {
        pos = selectedStr.find(":", pos + 1);
        if (pos != std::string::npos) selectedStr[pos]='/';
        Selection sel = sim->findObjectFromPath(selectedStr);
        sim->setSelection(sel);
    }

    pos = 0;
    while(pos != std::string::npos)
    {
        pos = trackedStr.find(":", pos + 1);
        if (pos != std::string::npos) trackedStr[pos]='/';
        Selection sel = sim->findObjectFromPath(trackedStr);
        sim->setTrackedObject(sel);
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
        char c;
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





