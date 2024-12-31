// url.cpp
//
// Copyright (C) 2002-present, the Celestia Development Team
// Original version written by Chris Teyssier (chris@tux.teyssier.org)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "url.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <sstream>
#include <utility>

#include <Eigen/Geometry>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <celcompat/charconv.h>
#include <celutil/r128util.h>
#include <celengine/body.h>
#include <celengine/location.h>
#include <celengine/render.h>
#include <celengine/simulation.h>
#include <celengine/universe.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/stringutils.h>
#include "celestiacore.h"

using namespace std::string_view_literals;

using celestia::util::GetLogger;
using celestia::util::DecodeFromBase64;
using celestia::util::EncodeAsBase64;

namespace astro = celestia::astro;
namespace math = celestia::math;

namespace
{

constexpr std::string_view PROTOCOL = "cel://"sv;

// First new render flag in version 1.7
// ShowDwarfPlanets = bit 27 (0x0000000008000000)
constexpr unsigned int NEW_FLAG_BIT_1_7 = 27;
constexpr std::uint64_t NEW_SHOW_PLANETS_BIT_MASK = (UINT64_C(1) << (NEW_FLAG_BIT_1_7 - 1));
constexpr std::uint64_t RF_MASK = NEW_SHOW_PLANETS_BIT_MASK - 1;

std::string_view
getCoordSysName(ObserverFrame::CoordinateSystem mode)
{
    switch (mode)
    {
    case ObserverFrame::CoordinateSystem::Universal:
        return "Freeflight"sv;
    case ObserverFrame::CoordinateSystem::Ecliptical:
        return "Follow"sv;
    case ObserverFrame::CoordinateSystem::BodyFixed:
        return "SyncOrbit"sv;
    case ObserverFrame::CoordinateSystem::Chase:
        return "Chase"sv;
    case ObserverFrame::CoordinateSystem::PhaseLock:
        return "PhaseLock"sv;
    case ObserverFrame::CoordinateSystem::Equatorial:
        return "Unknown"sv;
    case ObserverFrame::CoordinateSystem::ObserverLocal:
        return "Unknown"sv;
    default:
        return "Unknown"sv;
    }
}

std::map<std::string_view, std::string>
parseURLParams(std::string_view paramsStr)
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
            GetLogger()->error(_("URL parameter must look like key=value\n"));
            break;
        }
        params[kv.substr(0, vpos)] = Url::decodeString(kv.substr(vpos + 1));
        if (pos == npos)
            break;
        iter.remove_prefix(pos + 1);
    }

    return params;
}

struct Mode
{
    std::string_view                modeStr;
    ObserverFrame::CoordinateSystem mode;
    int                             nBodies;
};

constexpr std::array modes
{
    Mode{ "Freeflight"sv, ObserverFrame::CoordinateSystem::Universal,   0 },
    Mode{ "Follow"sv,     ObserverFrame::CoordinateSystem::Ecliptical,  1 },
    Mode{ "SyncOrbit"sv,  ObserverFrame::CoordinateSystem::BodyFixed,   1 },
    Mode{ "Chase"sv,      ObserverFrame::CoordinateSystem::Chase,       1 },
    Mode{ "PhaseLock"sv,  ObserverFrame::CoordinateSystem::PhaseLock,   2 },
};

} // end unnamed namespace

Url::Url(CelestiaCore *core) :
    m_appCore(core)
{
}

Url::Url(const CelestiaState &appState, int version, Url::TimeSource timeSource) :
    m_state(appState),
    m_appCore(appState.m_appCore),
    m_version(version),
    m_timeSource(timeSource)
{
    assert(version == 3);
    std::ostringstream u;

    switch (m_state.m_coordSys)
    {
    case ObserverFrame::CoordinateSystem::Universal:
        m_nBodies = 0;
        break;
    case ObserverFrame::CoordinateSystem::PhaseLock:
        m_nBodies = 2;
        break;
    default:
        m_nBodies = 1;
    }

    u << PROTOCOL << getCoordSysName(m_state.m_coordSys);

    if (appState.m_coordSys != ObserverFrame::CoordinateSystem::Universal)
    {
        u << '/' << m_state.m_refBodyName;
        if (appState.m_coordSys == ObserverFrame::CoordinateSystem::PhaseLock)
            u << '/' << m_state.m_targetBodyName;
    }

    m_date = astro::Date(m_state.m_tdb);
    u << '/' << m_date.toString(std::locale::classic(), astro::Date::ISO8601);

    // observer position
    fmt::print(u, "?x={}&y={}&z={}",
                     EncodeAsBase64(m_state.m_observerPosition.x),
                     EncodeAsBase64(m_state.m_observerPosition.y),
                     EncodeAsBase64(m_state.m_observerPosition.z));

    // observer orientation
    fmt::print(u, "&ow={}&ox={}&oy={}&oz={}",
               m_state.m_observerOrientation.w(),
               m_state.m_observerOrientation.x(),
               m_state.m_observerOrientation.y(),
               m_state.m_observerOrientation.z());

    if (!m_state.m_trackedBodyName.empty())
        u << "&track=" << m_state.m_trackedBodyName;
    if (!m_state.m_selectedBodyName.empty())
        u << "&select=" << m_state.m_selectedBodyName;

    fmt::print(u, "&fov={}&ts={}&ltd={}&p={}",
               m_state.m_fieldOfView,
               m_state.m_timeScale,
               m_state.m_lightTimeDelay ? 1 : 0,
               m_state.m_pauseState ? 1 : 0);


    // ShowEcliptic == 0x02000000, the last 1.6 parameter
    // we keep only old parameters and clear new ones
    auto rf = static_cast<int>(m_state.m_renderFlags & RF_MASK);
    // 1.6 uses ShowPlanets to control display of all types of solar
    // system objects. So set it if any one is displayed.
    if ((m_state.m_renderFlags & Renderer::ShowSolarSystemObjects) != 0)
        rf |= static_cast<int>(Renderer::ShowPlanets);
    // But we need to store actual value of the bit which controls
    // planets display. 26th bit onwards are unused in 1.6.
    if ((m_state.m_renderFlags & Renderer::ShowPlanets) != 0)
        rf |= NEW_SHOW_PLANETS_BIT_MASK;
    auto nrf = static_cast<int>(m_state.m_renderFlags >> NEW_FLAG_BIT_1_7);

    fmt::print(u, "&rf={}&nrf={}&lm={}",
               rf, nrf, m_state.m_labelMode);

    // Append the url settings: time source and version
    u << "&tsrc=" << (int) m_timeSource;
    u << "&ver=" << m_version;

    m_url = u.str();
    m_valid = true;
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
    sim->getActiveObserver()->setFOV(math::degToRad(m_state.m_fieldOfView));
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
    auto coord = sim->getObserver().getFrame()->convertToUniversal(m_state.m_observerPosition, tdb);
    Eigen::Quaterniond q = m_state.m_observerOrientation.cast<double>();
    q = sim->getObserver().getFrame()->convertToUniversal(q, tdb);
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

    switch (selection.getType())
    {
    case SelectionType::Body:
        name = selection.body()->getPath(universe->getStarCatalog(), ':');
        break;

    case SelectionType::Star:
        name = universe->getStarCatalog()->getStarName(*selection.star());
        break;

    case SelectionType::DeepSky:
        name = universe->getDSOCatalog()->getDSOName(selection.deepsky());
        break;

    case SelectionType::Location:
        name = selection.location()->getPath(universe->getStarCatalog(), ':');
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
        out.append(str.substr(a, b - a));
        std::string_view c_code = str.substr(b + 1, 2);
        std::uint8_t c;
        if (to_number(c_code, c, 16))
        {
            out += static_cast<std::string::value_type>(c);
        }
        else
        {
            GetLogger()->warn(_("Incorrect hex value \"{}\"\n"), c_code);
            out += '%';
            out.append(c_code);
        }
        a = b + 1 + c_code.length();
        b = str.find('%', a);
    }

    if (a < str.length())
        out.append(str.substr(a));

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
            fmt::print(enc, "%{:02x}", ch);
        else
            enc << _ch;
    }

    return enc.str();
}

bool
Url::parse(std::string_view urlStr)
{
    constexpr auto npos = std::string_view::npos;

    // proper URL string must start with protocol (cel://)
    if (urlStr.compare(0, PROTOCOL.length(), PROTOCOL) != 0)
    {
        GetLogger()->error(_("URL must start with \"{}\"!\n"), PROTOCOL);
        return false;
    }

    // extract @path and @params from the URL
    auto pos = urlStr.find('?');
    auto pathStr = urlStr.substr(PROTOCOL.length(), pos - PROTOCOL.length());
    while (pathStr.back() == '/')
        pathStr.remove_suffix(1);
    std::string_view paramsStr;
    if (pos != npos)
        paramsStr = urlStr.substr(pos + 1);

    pos = pathStr.find('/');
    if (pos == npos)
    {
        GetLogger()->error(_("URL must have at least mode and time!\n"));
        return false;
    }
    auto modeStr = pathStr.substr(0, pos);

    int nBodies = -1;
    CelestiaState state;
    auto it = std::find_if(modes.begin(), modes.end(),
                           [modeStr](const Mode &m) { return compareIgnoringCase(modeStr, m.modeStr) == 0; });
    if (it == modes.end())
    {
        GetLogger()->error(_("Unsupported URL mode \"{}\"!\n"), modeStr);
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
                GetLogger()->error(_("URL must contain only one body\n"));
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
                GetLogger()->error(_("URL must contain 2 bodies\n"));
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

    auto params = parseURLParams(paramsStr);

    // Version labelling of cel URLs was only added in Celestia 1.5, cel URL
    // version 2. Assume any URL without a version is version 1.
    int version = 1;
    if (auto it = params.find("ver"sv); it != params.end() && !to_number(it->second, version))
    {
        GetLogger()->error(_("Invalid URL version \"{}\"!\n"), it->second);
        return false;
    }

    if (version != 3 && version != 4)
    {
        GetLogger()->error(_("Unsupported URL version: {}\n"), version);
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

    return true;
}

bool
Url::initVersion3(const std::map<std::string_view, std::string> &params, std::string_view timeStr)
{
    m_version = 3;

    if (!astro::parseDate(std::string(timeStr), m_date))
        return false;
    m_state.m_tdb = (double) m_date;

    if (auto itx = params.find("x"sv), ity = params.find("y"sv), itz = params.find("z"sv);
        itx == params.end() || ity == params.end() || itz == params.end())
    {
        return false;
    }
    else
    {
        m_state.m_observerPosition = UniversalCoord(DecodeFromBase64(itx->second),
                                                    DecodeFromBase64(ity->second),
                                                    DecodeFromBase64(itz->second));
    }

    float ow, ox, oy, oz;
    if (auto itw = params.find("ow"sv), itx = params.find("ox"sv), ity = params.find("oy"sv), itz = params.find("oz"sv);
        itw != params.end() && to_number(itw->second, ow) &&
        itx != params.end() && to_number(itx->second, ox) &&
        ity != params.end() && to_number(ity->second, oy) &&
        itz != params.end() && to_number(itz->second, oz))
    {
        m_state.m_observerOrientation = Eigen::Quaternionf(ow, ox, oy, oz);
    }
    else
    {
        return false;
    }

    if (auto it = params.find("select"sv); it != params.end())
        m_state.m_selectedBodyName = it->second;
    if (auto it = params.find("track"sv); it != params.end())
        m_state.m_trackedBodyName = it->second;
    if (auto it = params.find("ltd"sv); it != params.end())
        m_state.m_lightTimeDelay = it->second != "0"sv;

    if (auto it = params.find("fov"sv); it != params.end() && !to_number(it->second, m_state.m_fieldOfView))
        return false;
    if (auto it = params.find("ts"sv); it != params.end() && !to_number(it->second, m_state.m_timeScale))
        return false;

    if (auto it = params.find("p"sv); it != params.end())
        m_state.m_pauseState = it->second != "0"sv;

    // Render settings
    bool hasNewRenderFlags = false;
    std::uint64_t newFlags = 0;
    if (auto it = params.find("nrf"sv); it != params.end())
    {
        hasNewRenderFlags = true;
        int nrf;
        if (!to_number(it->second, nrf))
            return false;
        newFlags = static_cast<std::uint64_t>(nrf) << NEW_FLAG_BIT_1_7;
    }
    if (auto it = params.find("rf"sv); it != params.end())
    {
        // old renderer flags are int
        int rf;
        if (!to_number(it->second, rf))
            return false;
        // older celestia versions don't know about the new renderer flags
        std::uint64_t oldFlags;
        if (hasNewRenderFlags)
        {
            oldFlags = static_cast<std::uint64_t>(rf) & RF_MASK;
            // get actual Renderer::ShowPlanets value in 26th bit
            // clear ShowPlanets if 26th bit is unset
            if ((rf & NEW_SHOW_PLANETS_BIT_MASK) == 0)
                oldFlags &= ~Renderer::ShowPlanets;
        }
        else
        {
            oldFlags = static_cast<std::uint64_t>(rf);
            // new options enabled by default in 1.7
            oldFlags |= Renderer::ShowPlanetRings | Renderer::ShowFadingOrbits;
            // old ShowPlanets == new ShowSolarSystemObjects
            if ((oldFlags & Renderer::ShowPlanets) != 0)
                oldFlags |= Renderer::ShowSolarSystemObjects;
        }
        m_state.m_renderFlags = newFlags | oldFlags;
    }
    if (auto it = params.find("lm"sv); it != params.end() && !to_number(it->second, m_state.m_labelMode))
        return false;

    int tsrc = 0;
    if (auto it = params.find("tsrc"sv); it != params.end() && !to_number(it->second, tsrc))
        return false;
    if (tsrc >= 0 && tsrc < TimeSourceCount)
        m_timeSource = static_cast<TimeSource>(tsrc);

    return true;
}

bool
Url::initVersion4(std::map<std::string_view, std::string> &params, std::string_view timeStr)
{
    if (auto it = params.find("rf"sv); it != params.end())
    {
        std::uint64_t rf;
        if (!to_number(it->second, rf))
            return false;
        auto nrf = static_cast<int>(rf >> NEW_FLAG_BIT_1_7);
        int _rf = rf & RF_MASK;
        if ((rf & Renderer::ShowPlanets) != 0)
            _rf |= NEW_SHOW_PLANETS_BIT_MASK; // Set the 26th bit to ShowPlanets
        it->second = fmt::format("{}", _rf);
        params["nrf"] = fmt::format("{}", nrf);
    }
    return initVersion3(params, timeStr);
}
