// url.cpp
//
// Copyright (C) 2002-present, the Celestia Development Team
// Original version written by Chris Teyssier (chris@tux.teyssier.org)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <fmt/printf.h>
#include <celcompat/charconv.h>
#include <celutil/bigfix.h>
#include <celutil/gettext.h>
#include <celutil/stringutils.h>
#include "celestiacore.h"
#include "url.h"

namespace
{

std::string getBodyName(Universe* universe, Body* body)
{
    std::string name = body->getName();
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
            break;
        parentBody = parentSystem->getPrimaryBody();
    }

    auto *star = body->getSystem()->getStar();
    if (star != nullptr)
        name = universe->getStarCatalog()->getStarName(*star) + ":" + name;

    return name;
}

// We use std::string here because we pass result to C API (gettext())
std::string getBodyShortName(const std::string &body)
{
    if (!body.empty())
    {
        auto pos = body.rfind(":");
        if (pos != std::string_view::npos)
            return body.substr(pos + 1);
    }
    return body;
}

std::string_view
getCoordSysName(ObserverFrame::CoordinateSystem mode)
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

} // anon namespace

Url::Url(CelestiaCore *core) :
    m_appCore(core)
{
}

Url::Url(const CelestiaState &appState, int version, Url::TimeSource timeSource) :
    m_appCore(appState.m_appCore),
    m_state(appState),
    m_version(version),
    m_timeSource(timeSource)
{
    assert(version == 3);
    std::ostringstream u;

    switch (m_state.m_coordSys)
    {
    case ObserverFrame::Universal:
        m_nBodies = 0;
        break;
    case ObserverFrame::PhaseLock:
        m_nBodies = 2;
        break;
    default:
        m_nBodies = 1;
    }

    u << Url::proto() << getCoordSysName(m_state.m_coordSys);

    if (appState.m_coordSys != ObserverFrame::Universal)
    {
        u << '/' << m_state.m_refBodyName;
        if (appState.m_coordSys == ObserverFrame::PhaseLock)
            u << '/' << m_state.m_targetBodyName;
    }

    m_date = astro::Date(m_state.m_tdb);
    u << '/' << m_date.toCStr(astro::Date::ISO8601);

    // observer position
    u << fmt::format("?x={}&y={}&z={}",
                     m_state.m_observerPosition.x.toString(),
                     m_state.m_observerPosition.y.toString(),
                     m_state.m_observerPosition.z.toString());

    // observer orientation
    u << fmt::format("&ow={}&ox={}&oy={}&oz={}",
                     m_state.m_observerOrientation.w(),
                     m_state.m_observerOrientation.x(),
                     m_state.m_observerOrientation.y(),
                     m_state.m_observerOrientation.z());

    if (!m_state.m_trackedBodyName.empty())
        u << "&track=" << m_state.m_trackedBodyName;
    if (!m_state.m_selectedBodyName.empty())
        u << "&select=" << m_state.m_selectedBodyName;

    u << fmt::format("&fov={}&ts={}&ltd={}&p={}",
                     m_state.m_fieldOfView,
                     m_state.m_timeScale,
                     m_state.m_lightTimeDelay ? 1 : 0,
                     m_state.m_pauseState ? 1 : 0);


    // ShowTintedIllumination == 0x04000000, the last 1.6 parameter
    // we keep only old parameters and clear new ones
    int rf = static_cast<int>(m_state.m_renderFlags & 0x04ffffffull);
    // 1.6 uses ShowPlanets to control display of all types of solar
    // system objects. So set it if any one is displayed.
    if ((m_state.m_renderFlags & Renderer::ShowSolarSystemObjects) != 0)
        rf |= static_cast<int>(Renderer::ShowPlanets);
    // But we need to store actual value of the bit which controls
    // planets display. 27th bit is unused in 1.6.
    if ((m_state.m_renderFlags & Renderer::ShowPlanets) != 0)
        rf |= (1<<27);
    int nrf = static_cast<int>(m_state.m_renderFlags >> 27);

    u << fmt::format("&rf={}&nrf={}&lm={}",
                     rf, nrf, m_state.m_labelMode);

    // Append the url settings: time source and version
    u << "&tsrc=" << (int) m_timeSource;
    u << "&ver=" << m_version;

    m_url = u.str();
    m_valid = true;
    evalName();
}

bool
Url::goTo()
{
    if (!m_valid)
        return false;

    assert(m_appCore != nullptr);
    auto *sim = m_appCore->getSimulation();
    auto *renderer = m_appCore->getRenderer();

    sim->update(0.0);
    sim->setFrame(m_ref.getCoordinateSystem(), m_ref.getRefObject(), m_ref.getTargetObject());
    sim->getActiveObserver()->setFOV(celmath::degToRad(m_state.m_fieldOfView));
    m_appCore->setZoomFromFOV();
    sim->setTimeScale(m_state.m_timeScale);
    sim->setPauseState(m_state.m_pauseState);
    m_appCore->setLightDelayActive(m_state.m_lightTimeDelay);
    if (!m_state.m_selectedBodyName.empty())
    {
        auto body = m_state.m_selectedBodyName;
        std::replace(body.begin(), body.end(), ':', '/');
        auto sel = sim->findObjectFromPath(body);
        sim->setSelection(sel);
    }
    else
    {
        sim->setSelection(Selection());
    }

    if (!m_state.m_trackedBodyName.empty())
    {
        auto body = m_state.m_trackedBodyName;
        std::replace(body.begin(), body.end(), ':', '/');
        auto sel = sim->findObjectFromPath(body);
        sim->setTrackedObject(sel);
    }
    else
    {
        if (!sim->getTrackedObject().empty())
            sim->setTrackedObject(Selection());
    }

    renderer->setRenderFlags(m_state.m_renderFlags);
    renderer->setLabelMode(m_state.m_labelMode);

    switch (m_timeSource)
    {
    case UseUrlTime:
        sim->setTime(m_state.m_tdb);
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
    auto coord = sim->getObserver().getFrame()->convertToUniversal(m_state.m_observerPosition, m_state.m_tdb);
    Eigen::Quaterniond q = m_state.m_observerOrientation.cast<double>();
    q = sim->getObserver().getFrame()->convertToUniversal(q, m_state.m_tdb);
    sim->setObserverPosition(coord);
    sim->setObserverOrientation(q.cast<float>());

    return true;
}

std::string
Url::getAsString() const
{
    return m_url;
}

std::string
Url::getEncodedObjectName(const Selection& selection, const CelestiaCore* appCore)
{
    auto *universe = appCore->getSimulation()->getUniverse();
    std::string name;
    Body* parentBody = nullptr;

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
        return {};
    }

    return Url::encodeString(name);
}

std::string
Url::decodeString(std::string_view str)
{
    std::string_view::size_type a = 0, b = 0;
    std::string out;

    b = str.find('%');
    while (b != std::string_view::npos && a < str.length())
    {
        auto s = str.substr(a, b - a);
        out.append(s.data(), s.length());
        auto c_code = str.substr(b + 1, 2);
        uint8_t c;
        if (to_number(c_code, c, 16))
        {
            out += static_cast<std::string::value_type>(c);
        }
        else
        {
            fmt::fprintf(std::cerr, _("Incorrect hex value \"%s\"\n"), c_code);
            out += '%';
            out.append(c_code.data(), c_code.length());
        }
        a = b + 1 + c_code.length();
        b = str.find('%', a);
    }
    if (a < str.length())
    {
        auto s = str.substr(a);
        out.append(s.data(), s.length());
    }

    return out;
}

std::string
Url::encodeString(std::string_view str)
{
    std::ostringstream enc;

    for (const auto _ch : str)
    {
        auto ch = static_cast<unsigned char>(_ch);
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
            enc << fmt::sprintf("%%%02x", ch);
        else
            enc << _ch;
    }

    return enc.str();
}

struct Mode
{
    std::string_view                modeStr;
    ObserverFrame::CoordinateSystem mode;
    int                             nBodies;
};

static Mode modes[] =
{
    { "Freeflight", ObserverFrame::Universal,   0 },
    { "Follow",     ObserverFrame::Ecliptical,  1 },
    { "SyncOrbit",  ObserverFrame::BodyFixed,   1 },
    { "Chase",      ObserverFrame::Chase,       1 },
    { "PhaseLock",  ObserverFrame::PhaseLock,   2 },
};

auto ParseURLParams(std::string_view paramsStr)
    -> std::map<std::string_view, std::string>;

bool Url::parse(std::string_view urlStr)
{
    constexpr auto npos = std::string_view::npos;

    // proper URL string must start with protocol (cel://)
    if (urlStr.compare(0, Url::proto().length(), Url::proto()) != 0)
    {
        fmt::fprintf(std::cerr, _("URL must start with \"%s\"!\n"), Url::proto());
        return false;
    }

    // extract @path and @params from the URL
    auto pos = urlStr.find('?');
    auto pathStr = urlStr.substr(Url::proto().length(), pos - Url::proto().length());
    while (pathStr.back() == '/')
        pathStr.remove_suffix(1);
    std::string_view paramsStr;
    if (pos != npos)
        paramsStr = urlStr.substr(pos + 1);

    pos = pathStr.find('/');
    if (pos == npos)
    {
        std::cerr << _("URL must have at least mode and time!\n");
        return false;
    }
    auto modeStr = pathStr.substr(0, pos);

    int nBodies = -1;
    CelestiaState state;
    auto lambda = [modeStr](Mode &m) { return compareIgnoringCase(modeStr, m.modeStr) == 0; };
    auto it = std::find_if(std::begin(modes), std::end(modes), lambda);
    if (it == std::end(modes))
    {
        fmt::fprintf(std::cerr, _("Unsupported URL mode \"%s\"!\n"), modeStr);
        return false;
    }
    state.m_coordSys = it->mode;
    nBodies = it->nBodies;

    auto timepos = nBodies == 0 ? pos : pathStr.rfind('/');
    auto timeStr = pathStr.substr(timepos + 1);

    Selection bodies[2];
    if (nBodies > 0)
    {
        auto bodiesStr = pathStr.substr(pos + 1, timepos - pos - 1);
        pos = bodiesStr.find('/');
        if (nBodies == 1)
        {
            if (pos != npos)
            {
                std::cerr << _("URL must contain only one body\n");
                return false;
            }
            auto body = Url::decodeString(bodiesStr);
            std::replace(body.begin(), body.end(), ':', '/');
            bodies[0] = m_appCore->getSimulation()->findObjectFromPath(body);
            state.m_refBodyName = std::move(body);
        }
        else if (nBodies == 2)
        {
            if (pos == npos || bodiesStr.find('/', pos + 1) != npos)
            {
                std::cerr << _("URL must contain 2 bodies\n");
                return false;
            }
            auto body = Url::decodeString(bodiesStr.substr(0, pos));
            std::replace(body.begin(), body.end(), ':', '/');
            bodies[0] = m_appCore->getSimulation()->findObjectFromPath(body);
            state.m_refBodyName = std::move(body);

            body = Url::decodeString(bodiesStr.substr(pos + 1));
            std::replace(body.begin(), body.end(), ':', '/');
            bodies[1] = m_appCore->getSimulation()->findObjectFromPath(body);
            state.m_targetBodyName = std::move(body);
        }
    }

    ObserverFrame ref;
    switch (nBodies)
    {
    case 0:
        ref = ObserverFrame();
        break;
    case 1:
        ref = ObserverFrame(state.m_coordSys, bodies[0]);
        break;
    case 2:
        ref = ObserverFrame(state.m_coordSys, bodies[0], bodies[1]);
        break;
    default:
        break;
    }

    auto params = ParseURLParams(paramsStr);

    // Version labelling of cel URLs was only added in Celestia 1.5, cel URL
    // version 2. Assume any URL without a version is version 1.
    int version = 1;
    if (params.count("ver") != 0)
    {
        auto &p = params["ver"];
        if (!to_number(p, version))
        {
            fmt::fprintf(std::cerr, _("Invalid URL version \"%s\"!\n"), p);
            return false;
        }
    }

    if (version != 3 && version != 4)
    {
        fmt::fprintf(std::cerr, _("Unsupported URL version: %i\n"), version);
        return false;
    }

    m_ref = ref;
    m_state = state;
    m_nBodies = nBodies;
    if (version == 4 && !initVersion4(params, timeStr))
        return false;
    else if (!initVersion3(params, timeStr))
        return false;
    m_valid = true;
    evalName();

    return true;
}

auto ParseURLParams(std::string_view paramsStr)
   -> std::map<std::string_view, std::string>
{
    std::map<std::string_view, std::string> params;
    if (paramsStr.empty())
        return params;

    constexpr auto npos = std::string_view::npos;
    for (auto iter = paramsStr;;)
    {
        auto pos = iter.find('&');
        auto kv = iter.substr(0, pos);
        auto vpos = kv.find('=');
        if (vpos == npos)
        {
            std::cerr << _("URL parameter must look like key=value\n");
            break;
        }
        params[kv.substr(0, vpos)] = Url::decodeString(kv.substr(vpos + 1));
        if (pos == npos)
            break;
        iter.remove_prefix(pos + 1);
    }

    return params;
}

bool Url::initVersion3(std::map<std::string_view, std::string> &params, std::string_view timeStr)
{
    m_version = 3;

    if (!astro::parseDate(std::string(timeStr), m_date))
        return false;
    m_state.m_tdb = (double) m_date;

    if (params.count("x") == 0 || params.count("y") == 0 || params.count("z") == 0)
        return false;
    m_state.m_observerPosition = UniversalCoord(BigFix(params["x"]),
                                                BigFix(params["y"]),
                                                BigFix(params["z"]));

    float ow, ox, oy, oz;
    if (to_number(params["ow"], ow) &&
        to_number(params["ox"], ox) &&
        to_number(params["oy"], oy) &&
        to_number(params["oz"], oz))
        m_state.m_observerOrientation = Eigen::Quaternionf(ow, ox, oy, oz);
    else
        return false;

    if (params.count("select") != 0)
        m_state.m_selectedBodyName = params["select"];
    if (params.count("track") != 0)
        m_state.m_trackedBodyName = params["track"];
    if (params.count("ltd") != 0)
        m_state.m_lightTimeDelay = params["ltd"] != "0";

    if (params.count("fov") != 0 && !to_number(params["fov"], m_state.m_fieldOfView))
        return false;
    if (params.count("ts") != 0 && !to_number(params["ts"], m_state.m_timeScale))
        return false;

    if (params.count("p") != 0)
        m_state.m_pauseState = params["p"] != "0";

    // Render settings
    bool hasNewRenderFlags = false;
    uint64_t newFlags = 0ull, oldFlags = 0ull;
    if (params.count("nrf") != 0)
    {
        hasNewRenderFlags = true;
        int nrf;
        if (!to_number(params["nrf"], nrf))
            return false;
        newFlags = static_cast<uint64_t>(nrf) << 27;
    }
    if (params.count("rf") != 0)
    {
        // old renderer flags are int
        int rf;
        if (!to_number(params["rf"], rf))
            return false;
        // older celestia versions don't know about the new renderer flags
        if (hasNewRenderFlags)
        {

            oldFlags = static_cast<uint64_t>(rf & 0x04ffffff);
            // get actual Renderer::ShowPlanets value in 27th bit
            // clear ShowPlanets if 27th bit is unset
            if ((rf & (1<<27)) == 0)
                oldFlags &= ~Renderer::ShowPlanets;
        }
        else
        {
            oldFlags = static_cast<uint64_t>(rf);
            // new options enabled by default in 1.7
            oldFlags |= Renderer::ShowPlanetRings | Renderer::ShowFadingOrbits;
            // old ShowPlanets == new ShowSolarSystemObjects
            if ((oldFlags & Renderer::ShowPlanets) != 0)
                oldFlags |= Renderer::ShowSolarSystemObjects;
        }
        m_state.m_renderFlags = newFlags | oldFlags;
    }
    if (params.count("lm") != 0 && !to_number(params["lm"], m_state.m_labelMode))
        return false;

    int tsrc = 0;
    if (params.count("tsrc") != 0 && !to_number(params["tsrc"], tsrc))
        return false;
    if (tsrc >= 0 && tsrc < TimeSourceCount)
        m_timeSource = static_cast<TimeSource>(tsrc);

    return true;
}

bool Url::initVersion4(std::map<std::string_view, std::string> &params, std::string_view timeStr)
{
    if (params.count("rf") != 0)
    {
        uint64_t rf;
        if (!to_number(params["rf"], rf))
            return false;
        int nrf = rf >> 27;
        int _rf = rf & 0x07ffffff;
        if ((rf & Renderer::ShowPlanets) != 0)
            _rf |= (1 << 27); // Set the 27th bits to ShowPlanets
        params["nrf"] = std::to_string(nrf);
        params["rf"] = std::to_string(_rf);
    }
    return initVersion3(params, timeStr);
}

void Url::evalName()
{
    std::string name;
    if (!m_state.m_refBodyName.empty())
        name += fmt::sprintf(" %s", _(getBodyShortName(m_state.m_refBodyName).c_str()));
    if (!m_state.m_targetBodyName.empty())
        name += fmt::sprintf(" %s", _(getBodyShortName(m_state.m_targetBodyName).c_str()));
    if (!m_state.m_trackedBodyName.empty())
        name += fmt::sprintf(" -> %s", _(getBodyShortName(m_state.m_trackedBodyName).c_str()));
    if (!m_state.m_selectedBodyName.empty())
        name += fmt::sprintf(" [%s]", _(getBodyShortName(m_state.m_selectedBodyName).c_str()));
    m_name = std::move(name);
}
