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

#include <cassert>
#include <cinttypes>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <fmt/printf.h>
#include <celutil/util.h>
#include <celengine/astro.h>
#include "celestiacore.h"
#include "url.h"

using namespace Eigen;
using namespace std;

constexpr const uint64_t NewRenderFlags = Renderer::ShowDwarfPlanets |
                                          Renderer::ShowMoons        |
                                          Renderer::ShowMinorMoons   |
                                          Renderer::ShowAsteroids    |
                                          Renderer::ShowComets       |
                                          Renderer::ShowSpacecrafts;


inline const string
DCO2Str(const astro::Date& date, UniversalCoord& coord, const Quaternionf& orientation)
{
    return fmt::sprintf("/%04d-%02d-%02dT%02d:%02d:%08.5f?x=%s&y=%s&z=%s&ow=%f&ox=%f&oy=%f&oz=%f",
                        date.year, date.month, date.day, date.hour, date.minute, date.seconds,
                        coord.x.toString(), coord.y.toString(), coord.z.toString(),
                        orientation.w(), orientation.x(), orientation.y(), orientation.z());
}


inline const string
Render2Str(unsigned version, uint64_t renderFlags, int labelMode)
{
    if (version >= 4)
        return fmt::sprintf("rf=%" PRIu64 "&lm=%d", renderFlags, labelMode);

    return fmt::sprintf("rf=%d&lm=%d", (int) (renderFlags & 0xffffffff), labelMode);
}


inline const string
View2Str(float fieldOfView, float timeScale, bool lightTimeDelay, bool pauseState)
{
    return fmt::sprintf("&fov=%f&ts=%f&ltd=%c&p=%c&",
                        fieldOfView, timeScale,
                        lightTimeDelay ? '1' : '0',
                        pauseState ? '1' : '0');
}


static string fromUrlPath(const string& str)
{
    string out = str;

    for (string::size_type pos = 0; pos != string::npos; )
    {
        pos = str.find(':', pos + 1);
        if (pos != string::npos)
            out[pos] = '/';
    }
    return out;
}


const string getEncodedObjectName(const Selection& sel, const CelestiaCore* appCore);

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

    Quaterniond q = sim->getObserver().getOrientation();
    q = frame->convertFromUniversal(q, tdb);
    observerOrientation = q.cast<float>();

    trackedBodyName  = getEncodedObjectName(sim->getTrackedObject(), appCore);
    selectedBodyName = getEncodedObjectName(sim->getSelection(), appCore);
    fieldOfView      = radToDeg(sim->getActiveObserver()->getFOV());
    timeScale        = static_cast<float>(sim->getTimeScale());
    pauseState       = sim->getPauseState();
    lightTimeDelay   = appCore->getLightDelayActive();
    renderFlags      = renderer->getRenderFlags();
    labelMode        = renderer->getLabelMode();
}


#if 0
bool CelestiaState::loadState(ParamsMap params)
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

    if (params.count("select") != 0)
        selectedBodyName = params["select"];
    if (params.count("track") != 0)
        trackedBodyName = params["track"];
    if (params.count("ltd") != 0)
        lightTimeDelay = (strcmp(params["ltd"].c_str(), "1") == 0);
    else
        lightTimeDelay = false;

    if (params.count("fov") != 0)
    {
        if (sscanf(params["fov"].c_str(), "%f", &fieldOfView) != 1.0f)
            return false;
    }

    if (params.count("ts") != 0)
    {
        if (sscanf(params["ts"].c_str(), "%f", &timeScale) != 1.0f)
            return false;
    }

    int paused = 0;
    if (params.count("p") != 0)
    {
        if (sscanf(params["p"].c_str(), "%d", &paused) != 1)
            return false;
        if (paused != 0 && paused != 1)
            return false;
        pauseState = paused == 1;
    }

    // Render settings
    if (params.count("rf") != 0)
    {
        if (sscanf(params["rf"].c_str(), "%d", &renderFlags) != 1)
            return false;
    }
    if (params.count("lm") != 0)
    {
        if (sscanf(params["lm"].c_str(), "%d", &labelMode) != 1)
            return false;
    }
}
#endif


Url::Url(string str, CelestiaCore *core):
    urlStr(move(str)),
    appCore(core)
{
    string::size_type pos, endPrevious;
    Simulation *sim = appCore->getSimulation();
    ParamsMap params = parseUrlParams(urlStr);

    if (urlStr.compare(0, 6, "cel://") != 0)
    {
        urlStr.clear();
        return;
    }

    // Version labelling of cel URLs was only added in Celestia 1.5, cel URL
    // version 2. Assume any URL without a version is version 1.
    version = params.count("ver") == 0 ? 1 : (unsigned) stoi(params["ver"]);

    pos = urlStr.find('/', 6);
    if (pos == string::npos)
        pos = urlStr.find('?', 6);

    if (pos == string::npos)
        modeStr = urlStr.substr(6);
    else
        modeStr = decodeString(urlStr.substr(6, pos - 6));

    if (!compareIgnoringCase(modeStr, "Freeflight"))
    {
        mode = ObserverFrame::Universal;
        nbBodies = 0;
    }
    else if (!compareIgnoringCase(modeStr, "Follow"))
    {
        mode = ObserverFrame::Ecliptical;
        nbBodies = 1;
    }
    else if (!compareIgnoringCase(modeStr, "SyncOrbit"))
    {
        mode = ObserverFrame::BodyFixed;
        nbBodies = 1;
    }
    else if (!compareIgnoringCase(modeStr, "Chase"))
    {
        mode = ObserverFrame::Chase;
        nbBodies = 1;
    }
    else if (!compareIgnoringCase(modeStr, "PhaseLock"))
    {
        mode = ObserverFrame::PhaseLock;
        nbBodies = 2;
    }
    else if (!compareIgnoringCase(modeStr, "Settings"))
    {
        type = UrlType::Settings;
        nbBodies = 0;
    }

    if (nbBodies == -1)
    {
        urlStr.clear();
        return; // Mode not recognized
    }

    endPrevious = pos;
    int i;
    for (i = 0; i < nbBodies && endPrevious != string::npos; i++)
    {
        string bodyName;
        pos = urlStr.find('/', endPrevious + 1);
        if (pos == string::npos)
            pos = urlStr.find('?', endPrevious + 1);
        if (pos == string::npos)
            bodyName = urlStr.substr(endPrevious + 1);
        else
            bodyName = urlStr.substr(endPrevious + 1, pos - endPrevious - 1);
        endPrevious = pos;

        bodies[i] = decodeString(bodyName);
    }

    if (i != nbBodies)
    {
        urlStr.clear();
        return; // Number of bodies in Url doesn't match Mode
    }

    switch (nbBodies)
    {
    case 0:
        ref = ObserverFrame();
        break;
    case 1:
        ref = ObserverFrame(mode, sim->findObjectFromPath(fromUrlPath(bodies[0])));
        break;
    case 2:
        ref = ObserverFrame(mode,
                            sim->findObjectFromPath(fromUrlPath(bodies[0])),
                            sim->findObjectFromPath(fromUrlPath(bodies[1])));
    }

    string time;
    pos = urlStr.find('?', endPrevious + 1);
    if (pos == string::npos)
        time = urlStr.substr(endPrevious + 1);
    else
        time = urlStr.substr(endPrevious + 1, pos - endPrevious - 1);
    time = decodeString(time);

    switch (version)
    {
    case 1:
    case 2:
        initVersion2(params, time);
        break;
    case 3:
    case 4:
        // Version 4 has only render flags defined as uint64_t
        initVersion3(params, time);
        break;
    default:
        urlStr.clear();
        return;
    }

    evalName();
}


Url::Url(CelestiaCore* core, UrlType _type) :
    appCore(core),
    type(_type),
    timeSource(TimeSource::UseUrlTime)
{
    ostringstream u("cel://");

    Simulation *sim = appCore->getSimulation();
    ref = *sim->getFrame();

    if (type == UrlType::Settings)
        u << "Settings";
    else
        u << getCoordSysName(sim->getFrame()->getCoordinateSystem());

    if (type != UrlType::Settings && sim->getFrame()->getCoordinateSystem() != ObserverFrame::Universal)
    {
        bodies[0] = getEncodedObjectName(sim->getFrame()->getRefObject());
        u << "/" << bodies[0];
        if (sim->getFrame()->getCoordinateSystem() == ObserverFrame::PhaseLock)
        {
            bodies[1] = getEncodedObjectName(sim->getFrame()->getTargetObject());
            u << "/" << bodies[1];
        }
    }

    switch (type)
    {
    case UrlType::Absolute:
        orientation = sim->getObserver().getOrientationf();
        coord = sim->getObserver().getPosition();
        u << DCO2Str(astro::Date(sim->getTime()), coord, orientation);
        break;

    case UrlType::Relative:
        sim->getSelectionLongLat(distance, longitude, latitude);
        u << fmt::sprintf("/?dist=%f&long=%f&lat=%f", distance, longitude, latitude);
        break;

    case UrlType::Settings:
        u << string("/?");
        break;
    }

    Renderer *renderer = appCore->getRenderer();
    switch (type)
    {
    case UrlType::Absolute: // Intentional Fall-Through
    case UrlType::Relative:
        tracked = sim->getTrackedObject();
        if (!trackedStr.empty())
            u << "&track=" << getEncodedObjectName(tracked);

        selected = sim->getSelection();
        if (!selectedStr.empty())
            u << "&select=" << getEncodedObjectName(selected);

        fieldOfView    = radToDeg(sim->getActiveObserver()->getFOV());
        timeScale      = (float) sim->getTimeScale();
        pauseState     = sim->getPauseState();
        lightTimeDelay = appCore->getLightDelayActive();

        u << View2Str(fieldOfView, timeScale, lightTimeDelay, pauseState);

    case UrlType::Settings: // Intentional Fall-Through
        renderFlags = renderer->getRenderFlags();
        labelMode = renderer->getLabelMode();
        u << Render2Str(version, renderFlags, labelMode);
        break;
    }

    // Append the Celestia URL version
    u << "&ver=" << version;

    urlStr = u.str();
    evalName();
}


/*! Construct a new cel URL from a saved CelestiaState object. This method may
 *  may only be called to create a version 3 or later url.
 */
Url::Url(const CelestiaState& appState, unsigned int _version, TimeSource _timeSource) :
    appCore(nullptr),
    version(_version),
    timeSource(_timeSource),
    type(UrlType::Absolute)
{
    assert(_version >= 3);

    ostringstream u;

    modeStr      = getCoordSysName(appState.coordSys);
    bodies[0]        = appState.refBodyName;
    bodies[1]        = appState.targetBodyName;
    selectedStr  = appState.selectedBodyName;
    trackedStr   = appState.trackedBodyName;

    coord        = appState.observerPosition;
    orientation  = appState.observerOrientation;

    //ref =
    //selected =
    //tracked =
    switch (appState.coordSys)
    {
    case ObserverFrame::Universal:
        nbBodies = 0;
        break;
    case ObserverFrame::PhaseLock:
        nbBodies = 2;
        break;
    default:
        nbBodies = 1;
    }

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
            u << "/" << appState.targetBodyName;
    }

    u << DCO2Str(date, coord, orientation);

    if (!trackedStr.empty())
        u << "&track=" << trackedStr;
    if (!selectedStr.empty())
        u << "&select=" << selectedStr;

    u << View2Str(fieldOfView, timeScale, lightTimeDelay, pauseState);
    u << Render2Str(version, renderFlags, labelMode);

    // Append the url settings: time source and version
    u << "&tsrc=" << (int) timeSource;
    u << "&ver=" << version;

    urlStr = u.str();

    evalName();
}


void Url::initDatePos(ParamsMap& params, const std::string& timeString)
{
    date = astro::Date(0.0);
    sscanf(timeString.c_str(), "%d-%d-%dT%d:%d:%lf",
           &date.year, &date.month, &date.day,
           &date.hour, &date.minute, &date.seconds);

    coord = UniversalCoord(BigFix(params["x"]),
                           BigFix(params["y"]),
                           BigFix(params["z"]));

    float ow, ox, oy, oz;
    ow = stof(params["ow"]);
    ox = stof(params["ox"]);
    oy = stof(params["oy"]);
    oz = stof(params["oz"]);

    orientation = Quaternionf(ow, ox, oy, oz);
}


void Url::initSimCommon(ParamsMap& params)
{
    if (params.count("select") != 0)
        selectedStr = params["select"];

    if (params.count("track") != 0)
        trackedStr = params["track"];

    if (params.count("ltd") != 0)
        lightTimeDelay = params["ltd"] == "1";

    if (params.count("fov") != 0)
        fieldOfView = stof(params["fov"]);

    if (params.count("ts") != 0)
        timeScale = stof(params["ts"]);

    if (params.count("p") != 0)
        pauseState = params["p"] == "1";
}


void Url::initRenderFlags(ParamsMap& params)
{
    if (params.count("rf") != 0)
    {
        if (version >= 4)
        {
            renderFlags = static_cast<uint64_t>(stoull(params["rf"]));
        }
        else
        {
            // old renderer flags are int
            renderFlags = static_cast<uint64_t>(stoi(params["rf"]));
            // older celestia versions didn't know about new renderer flags
            if ((renderFlags & Renderer::ShowPlanets) != 0)
                renderFlags |= NewRenderFlags;
        }
    }

    if (params.count("lm") != 0)
        labelMode = stoi(params["lm"]);
}


void Url::initVersion2(ParamsMap& params,
                       const string& timeString)
{
    if (type != UrlType::Settings)
        type = params.count("dist") == 0 ? UrlType::Absolute : UrlType::Relative;

    switch (type)
    {
    case UrlType::Absolute:
        initDatePos(params, timeString);

    // Intentional Fall-Through
    case UrlType::Relative:
        initSimCommon(params);

        distance = stod(params["dist"]);

        if (params.count("long") != 0)
            longitude = stod(params["long"]);

        if (params.count("lat") != 0)
            latitude = stod(params["lat"]);

    // Intentional Fall-Through
    case UrlType::Settings:
        initRenderFlags(params);
    }
}


void Url::initVersion3(ParamsMap& params,
                       const string& timeString)
{
    // Type field not used for version 3 urls; position is always relative
    // to the frame center. Time setting is controlled by the time source.
    type = UrlType::Absolute;

    initDatePos(params, timeString);
    initSimCommon(params);
    initRenderFlags(params);

    int timeSourceInt = 0;
    if (params.count("tsrc") != 0)
        timeSourceInt = stoi(params["tsrc"]);

    if (timeSourceInt >= 0 && timeSourceInt < (int) TimeSource::TimeSourceCount)
        timeSource = (TimeSource) timeSourceInt;
    else
        timeSource = TimeSource::UseUrlTime;
}


string Url::getAsString() const
{
    return urlStr;
}


string Url::getName() const
{
    return name;
}


void Url::evalName()
{
    double lo = longitude, la = latitude;
    const char* los;
    const char* las;
    switch (type)
    {
    case UrlType::Absolute:
        name = _(modeStr.c_str());
        if (!bodies[0].empty())
            name += " " + string(_(getBodyShortName(bodies[0]).c_str()));

        if (!bodies[1].empty())
            name += " " + string(_(getBodyShortName(bodies[1]).c_str()));

        if (!trackedStr.empty())
            name += " -> " + string(_(getBodyShortName(trackedStr).c_str()));

        if (!selectedStr.empty())
            name += " [" + string(_(getBodyShortName(selectedStr).c_str())) + "]";
        break;
    case UrlType::Relative:
        if (!selectedStr.empty())
            name = string(_(getBodyShortName(selectedStr).c_str())) + " ";

        if (lo < 0)
        {
            lo = -lo;
            los = _("W");
        }
        else
        {
            los = _("E");
        }
        if (la < 0)
        {
            la = -la;
            las = _("S");
        }
        else
        {
            las = _("N");
        }

        name += fmt::sprintf("(%.1lf%s, %.1lf%s)", lo, los, la, las);
        break;
    case UrlType::Settings:
        name = _("Settings");
        break;
    }
}


string Url::getBodyShortName(const string& body) const
{
    string::size_type pos;
    if (!body.empty())
    {
        pos = body.rfind(":");
        if (pos != string::npos)
            return body.substr(pos+1);

        return body;
    }
    return "";
}


Url::ParamsMap Url::parseUrlParams(const string& url) const
{
    string::size_type startName, startValue;
    ParamsMap params;

    for (string::size_type pos = url.find("?"); pos != string::npos;)
    {
        startName = pos + 1;
        startValue = url.find('=', startName);
        pos = url.find('&', pos + 1);
        if (startValue != string::npos)
        {
             startValue++;
             if (pos != string::npos)
                 params[url.substr(startName, startValue - startName - 1)] = decodeString(url.substr(startValue, pos - startValue));
             else
                 params[url.substr(startName, startValue - startName - 1)] = decodeString(url.substr(startValue));
        }
    }

    return params;
}


string Url::getCoordSysName(ObserverFrame::CoordinateSystem mode) const
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


static string getBodyName(Universe* universe, Body* body)
{
    string name = body->getName();
    PlanetarySystem* parentSystem = body->getSystem();
    const Body* parentBody = nullptr;

    if (parentSystem != nullptr)
        parentBody = parentSystem->getPrimaryBody();
    else
        assert(0);
        // TODO: Figure out why the line below was added.
        //parentBody = body->getOrbitBarycenter();

    while (parentBody != nullptr)
    {
        name = parentBody->getName() + ":" + name;
        parentSystem = parentBody->getSystem();
        if (parentSystem == nullptr)
            parentBody = nullptr;
        else
            parentBody = parentSystem->getPrimaryBody();
    }

    if (body->getSystem()->getStar() != nullptr)
    {
        name = universe->getStarCatalog()->getStarName(*(body->getSystem()->getStar())) + ":" + name;
    }

    return name;
}


void Url::goTo()
{
    if (urlStr.empty())
        return;

    Selection sel;
    Simulation *sim = appCore->getSimulation();
    Renderer *renderer = appCore->getRenderer();

    sim->update(0.0);

    switch (type)
    {
    case UrlType::Absolute:
    // Intentional Fall-Through
    case UrlType::Relative:
        sim->setFrame(ref.getCoordinateSystem(), ref.getRefObject(), ref.getTargetObject());
        sim->getActiveObserver()->setFOV(degToRad(fieldOfView));
        appCore->setZoomFromFOV();
        sim->setTimeScale(timeScale);
        sim->setPauseState(pauseState);
        appCore->setLightDelayActive(lightTimeDelay);

        if (!selectedStr.empty())
            sim->setSelection(sim->findObjectFromPath(fromUrlPath(selectedStr)));
        else
            sim->setSelection(Selection());

        if (!trackedStr.empty())
            sim->setTrackedObject(sim->findObjectFromPath(fromUrlPath(trackedStr)));
        else if (!sim->getTrackedObject().empty())
            sim->setTrackedObject(Selection());

    // Intentional Fall-Through
    case UrlType::Settings:
        renderer->setRenderFlags(renderFlags);
        renderer->setLabelMode(labelMode);
        break;
    }

    if (version >= 3)
    {
        switch (timeSource)
        {
        case TimeSource::UseUrlTime:
            sim->setTime((double) date);
            break;

        case TimeSource::UseSimulationTime:
            // Leave the current simulation time unmodified
            break;

        case TimeSource::UseSystemTime:
            sim->setTime(astro::UTCtoTDB(astro::Date::systemDate()));
            break;

        default:
            break;
        }

        // Position and orientation stored in frame coordinates; convert them
        // to universal and set the observer position.
        double tdb = sim->getTime();
        coord = sim->getObserver().getFrame()->convertToUniversal(coord, tdb);
        Quaterniond q = sim->getObserver().getFrame()->convertToUniversal(orientation.cast<double>(), tdb);
        sim->setObserverPosition(coord);
        sim->setObserverOrientation(q.cast<float>());
    }
    else
    {
        switch (type)
        {
        case UrlType::Absolute:
            sim->setTime((double) date);
            sim->setObserverPosition(coord);
            sim->setObserverOrientation(orientation);
            break;

        case UrlType::Relative:
            sim->gotoSelectionLongLat(0, distance, (float) (longitude * PI / 180), (float) (latitude * PI / 180), Vector3f::UnitY());
            break;

        case UrlType::Settings:
            break;
        }
    }
}


string Url::decodeString(const string& str)
{
    string::size_type a = 0;
    string out;

    for (auto b = str.find("%"); b != string::npos;)
    {
        unsigned int c;
        out += str.substr(a, b-a);
        string c_code = str.substr(b+1, 2);
        sscanf(c_code.c_str(), "%02x", &c);
        out += (char) c;
        a = b + 3;
        b = str.find('%', a);
    }
    out += str.substr(a);

    return out;
}


string Url::encodeString(const string& str)
{
    ostringstream enc;

    for (const auto _ch : str)
    {
        int ch = (unsigned char) _ch;
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
            enc << '%' << setw(2) << hex << ch;
        else
            enc << _ch;
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
    Body* parentBody = nullptr;
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
        parentBody = selection.location()->getParentBody();
        if (parentBody != nullptr)
            name = getBodyName(universe, parentBody) + ":" + name;
        break;

    default:
        return "";
    }

    return Url::encodeString(name);
}
