// celestiastate.cpp
//
// Copyright (C) 2002-present, the Celestia Development Team
// Original version written by Chris Teyssier (chris@tux.teyssier.org)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celestiastate.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <optional>
#include <system_error>

#include <fmt/format.h>

#include <celastro/date.h>
#include <celcompat/charconv.h>
#include <celengine/body.h>
#include <celengine/dsodb.h>
#include <celengine/location.h>
#include <celengine/simulation.h>
#include <celengine/stardb.h>
#include <celengine/univcoord.h>
#include <celengine/universe.h>
#include <celmath/mathlib.h>
#include <celutil/flag.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/r128util.h>
#include "celestiacore.h"

using namespace std::string_view_literals;
using celestia::util::GetLogger;

class DeepSkyObject;
class Star;

namespace celestia
{
namespace
{
constexpr std::string_view PROTOCOL = "cel://"sv;
constexpr int UrlVersion = 3;

// First new render flag in version 1.7
// ShowDwarfPlanets = bit 27 (0x0000000008000000)
constexpr unsigned int NEW_FLAG_BIT_1_7 = 27;
constexpr auto NEW_SHOW_PLANETS_BIT_MASK = static_cast<RenderFlags>(UINT64_C(1) << (NEW_FLAG_BIT_1_7 - 1U));
constexpr auto RF_MASK = static_cast<RenderFlags>(static_cast<std::uint64_t>(NEW_SHOW_PLANETS_BIT_MASK) - UINT64_C(1));

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

constexpr std::string_view
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

template<std::size_t N>
bool
decodeString(std::string_view encodedString, fmt::basic_memory_buffer<char, N>& buffer, bool convertPath = false)
{
    buffer.clear();
    auto pos = convertPath ? encodedString.find_first_of(":%"sv) : encodedString.find('%');
    if (pos == std::string_view::npos)
        return true;

    for (;;)
    {
        std::ranges::copy(encodedString.substr(0, pos), std::back_inserter(buffer));
        if (pos == std::string_view::npos)
            return true;

        char ch = encodedString[pos];
        encodedString.remove_prefix(pos + 1);
        if (ch == ':')
        {
            buffer.push_back('/');
        }
        else if (encodedString.size() < 2)
        {
            GetLogger()->warn(_("Invalid escape sequence in URL\n"));
            return false;
        }
        else
        {
            // decode escape sequence
            std::uint8_t cval;
            const auto endPtr = encodedString.data() + 2;
            if (auto [ptr, ec] = compat::from_chars(encodedString.data(), endPtr, cval, 16);
                ec != std::errc{} || ptr != endPtr)
            {
                GetLogger()->warn(_("Invalid escape sequence in URL\n"));
                return false;
            }

            buffer.push_back(static_cast<char>(cval));
            encodedString.remove_prefix(2);
        }

        pos = convertPath ? encodedString.find_first_of(":%"sv) : encodedString.find('%');
    }
}

Selection
findObjectFromEncodedPath(const Simulation& simulation, std::string_view path)
{
    fmt::basic_memory_buffer<char, 256> buffer;
    if (!decodeString(path, buffer, true))
        return {};

    if (buffer.size() > 0)
        path = std::string_view(buffer.data(), buffer.size());
    return simulation.findObjectFromPath(path);
}

struct EncodedName
{
    EncodedName(const Selection& sel, const Universe& u)
        : selection(sel), universe(u)
    {
    }

    const Selection& selection;
    const Universe& universe;
};

class QueryFields
{
public:
    explicit QueryFields(const CelestiaCore& appCore) : m_appCore(appCore) {}

    bool parse(std::string_view);

    auto x() const { return getFieldPtr(m_x, Field::x); }
    auto y() const { return getFieldPtr(m_y, Field::y); }
    auto z() const { return getFieldPtr(m_z, Field::z); }
    auto ow() const { return getField(m_ow, Field::ow); }
    auto ox() const { return getField(m_ox, Field::ox); }
    auto oy() const { return getField(m_oy, Field::oy); }
    auto oz() const { return getField(m_oz, Field::oz); }
    auto select() const { return getSelection(m_select, Field::select); }
    auto track() const { return getSelection(m_track, Field::track); }
    auto fov() const { return getField(m_fov, Field::fov); }
    auto ts() const { return getField(m_ts, Field::ts); }
    auto nrf() const { return getField(m_nrf, Field::nrf); }
    auto rf() const { return getField(m_rf, Field::rf); }
    auto lm() const { return getField(m_lm, Field::lm); }
    auto tsrc() const { return getField(m_tsrc, Field::tsrc); }
    auto ver() const { return getField(m_ver, Field::ver); }
    auto p() const { return getField(m_p, Field::p); }
    auto ltd() const { return getField(m_ltd, Field::ltd); }

private:
    struct Parsers;

    enum class Field
    {
        x, y, z,
        ow, ox, oy, oz,
        select, track,
        fov, ts,
        nrf, rf, lm,
        tsrc, ver,
        p, ltd,
        _Max,
    };

    static bool isPathProperty(Field field)
    {
        return field == Field::select || field == Field::track;
    }

    template<typename T>
    std::optional<T> getField(const T& data, Field field) const
    {
        return m_fields[static_cast<std::size_t>(field)] ? std::make_optional(data) : std::nullopt;
    }

    template<typename T>
    const T* getFieldPtr(const T& data, Field field) const
    {
        return m_fields[static_cast<std::size_t>(field)] ? &data : nullptr;
    }

    Selection getSelection(const Selection& data, Field field) const
    {
        return m_fields[static_cast<std::size_t>(field)] ? data : Selection{};
    }

    const CelestiaCore& m_appCore;

    R128 m_x{ 0, 0 };
    R128 m_y{ 0, 0 };
    R128 m_z{ 0, 0 };
    float m_ow{ 0.0f };
    float m_ox{ 0.0f };
    float m_oy{ 0.0f };
    float m_oz{ 0.0f };
    Selection m_select;
    Selection m_track;
    float m_fov{ 0.0f };
    float m_ts{ 0.0f };
    std::uint64_t m_nrf{ 0 };
    std::uint64_t m_rf{ 0 };
    std::uint32_t m_lm{ 0 };
    int m_tsrc{ 0 };
    int m_ver{ 0 };
    bool m_p{ false };
    bool m_ltd{ false };

    std::bitset<static_cast<std::size_t>(Field::_Max)> m_fields;
};

struct QueryFields::Parsers
{
    struct FieldParser
    {
        std::string_view key;
        Field field;
        bool (*parser)(QueryFields&, std::string_view, std::string_view);
    };

    template<typename T>
        requires std::integral<T> || std::floating_point<T>
    static bool parseNumber(const QueryFields&, std::string_view key, std::string_view value, T& out)
    {
        const auto endPtr = value.data() + value.size();
        if (auto [ptr, ec] = compat::from_chars(value.data(), endPtr, out);
            ec != std::errc{} || ptr != endPtr)
        {
            GetLogger()->warn(_("Failed to parse field {}\n"), key);
            return false;
        }

        return true;
    }

    static bool parseBool(const QueryFields&, std::string_view key, std::string_view value, bool& out)
    {
        if (value == "0"sv)
        {
            out = false;
            return true;
        }

        if (value == "1"sv)
        {
            out = true;
            return true;
        }

        GetLogger()->warn(_("Failed to parse field {}\n"), key);
        return false;
    }

    static bool parseR128(const QueryFields&, std::string_view key, std::string_view value, R128& out)
    {
        auto result = util::DecodeFromBase64(value);
        if (!result.has_value())
        {
            GetLogger()->warn(_("Failed to parse field {}\n"), key);
            return false;
        }

        out = *result;
        return true;
    }

    static bool parseBody(const QueryFields& q, std::string_view key, std::string_view value, Selection& out)
    {
        out = q.m_appCore.getSimulation()->findObjectFromPath(value);
        if (out.empty())
        {
            GetLogger()->warn(_("Unknown body in field {}\n"), key);
            return false;
        }

        return true;
    }

    static constexpr std::array s_parsers
    {
        FieldParser { "x"sv, Field::x, [](QueryFields& q, std::string_view k, std::string_view v) { return parseR128(q, k, v, q.m_x); } },
        FieldParser { "y"sv, Field::y, [](QueryFields& q, std::string_view k, std::string_view v) { return parseR128(q, k, v, q.m_y); } },
        FieldParser { "z"sv, Field::z, [](QueryFields& q, std::string_view k, std::string_view v) { return parseR128(q, k, v, q.m_z); } },
        FieldParser { "ow"sv, Field::ow, [](QueryFields& q, std::string_view k, std::string_view v) { return parseNumber(q, k, v, q.m_ow); } },
        FieldParser { "ox"sv, Field::ox, [](QueryFields& q, std::string_view k, std::string_view v) { return parseNumber(q, k, v, q.m_ox); } },
        FieldParser { "oy"sv, Field::oy, [](QueryFields& q, std::string_view k, std::string_view v) { return parseNumber(q, k, v, q.m_oy); } },
        FieldParser { "oz"sv, Field::oz, [](QueryFields& q, std::string_view k, std::string_view v) { return parseNumber(q, k, v, q.m_oz); } },
        FieldParser { "select"sv, Field::select, [](QueryFields& q, std::string_view k, std::string_view v) { return parseBody(q, k ,v, q.m_select); } },
        FieldParser { "track"sv, Field::track, [](QueryFields& q, std::string_view k, std::string_view v) { return parseBody(q, k ,v, q.m_track); } },
        FieldParser { "fov"sv, Field::fov, [](QueryFields& q, std::string_view k, std::string_view v) { return parseNumber(q, k, v, q.m_fov); } },
        FieldParser { "ts"sv, Field::ts, [](QueryFields& q, std::string_view k, std::string_view v) { return parseNumber(q, k, v, q.m_ts); } },
        FieldParser { "nrf"sv, Field::nrf, [](QueryFields& q, std::string_view k, std::string_view v) { return parseNumber(q, k, v, q.m_nrf); } },
        FieldParser { "rf"sv, Field::rf, [](QueryFields& q, std::string_view k, std::string_view v) { return parseNumber(q, k, v, q.m_rf); } },
        FieldParser { "lm"sv, Field::lm, [](QueryFields& q, std::string_view k, std::string_view v) { return parseNumber(q, k, v, q.m_lm); } },
        FieldParser { "tsrc"sv, Field::tsrc, [](QueryFields& q, std::string_view k, std::string_view v) { return parseNumber(q, k, v, q.m_tsrc); } },
        FieldParser { "ver"sv, Field::ver, [](QueryFields& q, std::string_view k, std::string_view v) { return parseNumber(q, k, v, q.m_ver); } },
        FieldParser { "p"sv, Field::p, [](QueryFields& q, std::string_view k, std::string_view v) { return parseBool(q, k, v, q.m_p); } },
        FieldParser { "ltd"sv, Field::ltd, [](QueryFields& q, std::string_view k, std::string_view v) { return parseBool(q, k, v, q.m_ltd); } },
    };

    static_assert(s_parsers.size() == static_cast<std::size_t>(Field::_Max));
};

bool
QueryFields::parse(std::string_view urlStr)
{
    fmt::basic_memory_buffer<char, 256> decodeBuffer;
    while (!urlStr.empty())
    {
        auto nextPos = urlStr.find('&');
        auto valueStr = urlStr.substr(0, nextPos);
        auto splitPos = valueStr.find('=');
        if (splitPos == std::string_view::npos)
        {
            GetLogger()->warn(_("Query string entries must be in key=value form\n"));
            return false;
        }

        auto key = valueStr.substr(0, splitPos);
        valueStr.remove_prefix(splitPos + 1);
        if (auto parser = std::ranges::find(Parsers::s_parsers, key, [](const auto& p) { return p.key; });
            parser != Parsers::s_parsers.end())
        {
            auto flag = m_fields[static_cast<std::size_t>(parser->field)];
            if (flag)
            {
                GetLogger()->warn(_("Duplicate parameter {} in URL query string\n"), key);
                return false;
            }

            if (!decodeString(valueStr, decodeBuffer, isPathProperty(parser->field)))
                return false;

            if (decodeBuffer.size() > 0)
                valueStr = std::string_view(decodeBuffer.data(), decodeBuffer.size());

            if (!parser->parser(*this, key, valueStr))
                return false;

            flag = true;
        }

        if (nextPos == std::string_view::npos)
            break;

        urlStr.remove_prefix(nextPos + 1);
    }

    return true;
}

} // end unnamed namespace
} // end namespace celestia

template<>
struct fmt::formatter<celestia::EncodedName>
{
    constexpr auto parse(const format_parse_context& ctx) const
    {
        // we should validate the format here but exceptions are disabled
        return ctx.begin();
    }

    auto format(const celestia::EncodedName& encodedName, format_context& ctx) const
    {
        std::string name;
        if (const Body* body = encodedName.selection.body(); body)
            name = body->getPath(encodedName.universe.getStarCatalog(), ':');
        else if (const Star* star = encodedName.selection.star(); star)
            name = encodedName.universe.getStarCatalog()->getStarName(*star);
        else if (const DeepSkyObject* dso = encodedName.selection.deepsky(); dso)
            name = encodedName.universe.getDSOCatalog()->getDSOName(dso);
        else if (const Location* loc = encodedName.selection.location(); loc)
            name = loc->getPath(encodedName.universe.getStarCatalog(), ':');
        else
            return ctx.out();

        auto result = ctx.out();
        for (char ch : name)
        {
            constexpr auto encodeChars = "%?\"#+,=@[]"sv;
            if (ch < 32 || ch >= 127 || encodeChars.find(ch) != std::string_view::npos)
                result = fmt::format_to(result, "%{:02X}", ch);
            else
                *(result++) = ch;
        }

        return result;
    }
};

namespace celestia
{

class CelestiaState::Parser
{
public:
    explicit Parser(CelestiaState& state) : m_state(state) {}
    bool parse(std::string_view) const;

private:
    struct ParsePathResponse
    {
        ParsePathResponse(double _tdb, bool _isValid, bool _hasTime) :
            tdb(_tdb), isValid(_isValid), hasTime(_hasTime)
        {}

        static ParsePathResponse invalid() { return ParsePathResponse(0, false, false); }
        static ParsePathResponse noTime() { return ParsePathResponse(0, true, false); }
        static ParsePathResponse withTime(double tdb) { return ParsePathResponse(tdb, true, true); }

        double tdb;
        bool isValid;
        bool hasTime;
    };

    ParsePathResponse parsePath(std::string_view&) const;
    std::optional<UrlTimeSource> parseQuery(std::string_view) const;
    bool applyRenderFlags(const QueryFields&) const;

    CelestiaState& m_state;
};

bool
CelestiaState::Parser::parse(std::string_view urlStr) const
{
    const auto parsePathResponse = parsePath(urlStr);
    if (!parsePathResponse.isValid)
        return false;

    auto timeSource = parseQuery(urlStr);
    if (!timeSource.has_value())
        return false;

    switch (*timeSource)
    {
    case UrlTimeSource::UseUrlTime:
        if (!parsePathResponse.hasTime)
        {
            GetLogger()->warn(_("Missing time in URL\n"));
            return false;
        }
        else
        {
            m_state.m_time.emplace<UrlTime>(parsePathResponse.tdb);
        }
        break;

    case UrlTimeSource::UseSimulationTime:
        m_state.m_time.emplace<SimulationTime>();
        break;

    case UrlTimeSource::UseSystemTime:
        m_state.m_time.emplace<SystemTime>();
        break;

    default:
        assert(0);
        GetLogger()->warn(_("Invalid time source\n"));
        return false;
    }

    return true;
}

CelestiaState::Parser::ParsePathResponse
CelestiaState::Parser::parsePath(std::string_view& urlStr) const
{
    const Simulation& simulation = *m_state.m_appCore.getSimulation();

    if (!urlStr.starts_with(PROTOCOL))
        return ParsePathResponse::invalid();

    urlStr.remove_prefix(PROTOCOL.size());

    int nBodies;
    if (auto pos = urlStr.find('/'); pos == std::string_view::npos)
    {
        GetLogger()->warn(_("URL does not contain required mode element\n"));
        return ParsePathResponse::invalid();
    }
    else
    {
        const auto& mode = std::ranges::find(modes, urlStr.substr(0, pos), [](const auto &m) { return m.modeStr; });
        if (mode == modes.end())
        {
            GetLogger()->warn(_("Unknown mode in URL\n"));
            return ParsePathResponse::invalid();
        }

        urlStr.remove_prefix(pos + 1);
        m_state.m_coordSys = mode->mode;
        nBodies = mode->nBodies;
    }

    for (int i = 0; i < nBodies; ++i)
    {
        auto pos = urlStr.find('/');
        if (pos == std::string_view::npos)
        {
            GetLogger()->warn(_("Missing body {} of {} in URL\n"), i + 1, nBodies);
            return ParsePathResponse::invalid();
        }

        Selection body = findObjectFromEncodedPath(simulation, urlStr.substr(0, pos));
        if (body.empty())
        {
            GetLogger()->warn(_("Unknwon body in URL\n"));
            return ParsePathResponse::invalid();
        }

        if (i == 0)
            m_state.m_refBody = body;
        else
            m_state.m_targetBody = body;

        urlStr.remove_prefix(pos + 1);
    }

    std::string_view timeStr;
    if (auto pos = urlStr.find('?'); pos == std::string_view::npos)
    {
        GetLogger()->warn(_("Missing query parameters in URL\n"));
        return ParsePathResponse::invalid();
    }
    else
    {
        timeStr = urlStr.substr(0, pos);
        urlStr = urlStr.substr(pos + 1);
    }

    // Ignore trailing slashes
    if (auto pos = timeStr.find_last_not_of('/'); pos == std::string_view::npos)
        return ParsePathResponse::noTime();
    else if (astro::Date date; astro::parseDate(timeStr.substr(0, pos + 1), date))
        return ParsePathResponse::withTime(static_cast<double>(date));

    GetLogger()->warn(_("Could not parse time value in URL\n"));
    return ParsePathResponse::invalid();
}

std::optional<UrlTimeSource>
CelestiaState::Parser::parseQuery(std::string_view urlStr) const
{
    QueryFields queryFields(m_state.m_appCore);
    if (!queryFields.parse(urlStr))
        return std::nullopt;

    if (!applyRenderFlags(queryFields))
        return std::nullopt;

    if (auto x = queryFields.x(), y = queryFields.y(), z = queryFields.z(); x && y && z)
    {
        m_state.m_observerPosition = UniversalCoord(*x, *y, *z);
    }
    else
    {
        GetLogger()->warn(_("URL does not contain position information\n"));
        return std::nullopt;
    }

    if (auto ow = queryFields.ow(), ox = queryFields.ox(), oy = queryFields.oy(), oz = queryFields.oz();
        ow.has_value() && ox.has_value() && oy.has_value() && oz.has_value())
    {
        m_state.m_observerOrientation = Eigen::Quaternionf(*ow, *ox, *oy, *oz).normalized();
    }
    else
    {
        GetLogger()->warn(_("URL does not contain orientation information\n"));
        return std::nullopt;
    }

    if (auto select = queryFields.select(); !select.empty())
        m_state.m_selectedBody = select;
    if (auto track = queryFields.track(); !track.empty())
        m_state.m_trackedBody = track;
    if (auto ltd = queryFields.ltd(); ltd.has_value())
        m_state.m_lightTimeDelay = *ltd;
    if (auto fov = queryFields.fov(); fov.has_value())
        m_state.m_fieldOfView = *fov;
    if (auto ts = queryFields.ts(); ts.has_value())
        m_state.m_timeScale = *ts;
    if (auto p = queryFields.p(); p.has_value())
        m_state.m_pauseState = *p;

    if (auto lm = queryFields.lm(); lm.has_value())
        m_state.m_labelMode = static_cast<RenderLabels>(*lm);

    auto timeSource = UrlTimeSource::UseUrlTime;
    if (auto tsrc = queryFields.tsrc(); tsrc.has_value())
    {
        if (*tsrc < 0 || *tsrc >= static_cast<int>(UrlTimeSource::_Max))
        {
            GetLogger()->warn(_("Invalid time source selection\n"));
            return std::nullopt;
        }

        timeSource = static_cast<UrlTimeSource>(*tsrc);
    }

    return timeSource;
}

bool
CelestiaState::Parser::applyRenderFlags(const QueryFields& queryFields) const
{
    int version = queryFields.ver().value_or(0);
    std::optional<std::uint64_t> rfValue = queryFields.rf();
    std::optional<std::uint64_t> nrf = queryFields.nrf();
    if (version == 4)
    {
        if (rfValue.has_value())
        {
            nrf = *rfValue >> NEW_FLAG_BIT_1_7;
            auto rf = static_cast<RenderFlags>(*rfValue) & RF_MASK;
            if (util::is_set(rf, RenderFlags::ShowPlanets))
                rf |= NEW_SHOW_PLANETS_BIT_MASK;
            rfValue = static_cast<std::uint64_t>(rf);
        }
    }
    else if (version != 3)
    {
        GetLogger()->warn(_("Unsupported URL version\n"));
        return false;
    }

    bool hasNewRenderFlags = false;
    RenderFlags newFlags = RenderFlags::ShowNothing;
    if (nrf.has_value())
    {
        hasNewRenderFlags = true;
        newFlags = static_cast<RenderFlags>(*nrf << NEW_FLAG_BIT_1_7);
    }

    if (rfValue.has_value())
    {
        auto rf = static_cast<RenderFlags>(*rfValue);

        // older celestia versions don't know about the new renderer flags
        RenderFlags oldFlags;
        if (hasNewRenderFlags)
        {
            oldFlags = rf & RF_MASK;
            // get actual Renderer::ShowPlanets value in 26th bit
            // clear ShowPlanets if 26th bit is unset
            if (!util::is_set(rf, NEW_SHOW_PLANETS_BIT_MASK))
                oldFlags &= ~RenderFlags::ShowPlanets;
        }
        else
        {
            oldFlags = rf;
            // new options enabled by default in 1.7
            oldFlags |= RenderFlags::ShowPlanetRings | RenderFlags::ShowFadingOrbits;
            // old ShowPlanets == new ShowSolarSystemObjects
            if (util::is_set(oldFlags, RenderFlags::ShowPlanets))
                oldFlags |= RenderFlags::ShowSolarSystemObjects;
        }
        m_state.m_renderFlags = newFlags | oldFlags;
    }

    return true;
}

CelestiaState::CelestiaState(CelestiaCore& appCore) :
    m_appCore(appCore)
{
}

CelestiaState
CelestiaState::capture(CelestiaCore& appCore)
{
    CelestiaState state(appCore);

    const Simulation* sim = appCore.getSimulation();
    const Renderer* renderer = appCore.getRenderer();

    auto frame = sim->getFrame();

    state.m_coordSys = frame->getCoordinateSystem();
    if (state.m_coordSys != ObserverFrame::CoordinateSystem::Universal)
    {
        state.m_refBody = frame->getRefObject();
        if (state.m_coordSys == ObserverFrame::CoordinateSystem::PhaseLock)
            state.m_targetBody = frame->getTargetObject();
    }

    const auto tdb = sim->getTime();
    state.m_time.emplace<UrlTime>(tdb);

    state.m_observerPosition = frame->convertFromUniversal(sim->getObserver().getPosition(), tdb);
    state.m_observerOrientation = frame->convertFromUniversal(sim->getObserver().getOrientation(), tdb).cast<float>();

    state.m_trackedBody = sim->getTrackedObject();
    state.m_selectedBody = sim->getSelection();
    state.m_fieldOfView = math::radToDeg(sim->getActiveObserver()->getFOV());
    state.m_timeScale = static_cast<float>(sim->getTimeScale());
    state.m_pauseState = sim->getPauseState();
    state.m_lightTimeDelay = appCore.getLightDelayActive();
    state.m_renderFlags = renderer->getRenderFlags();
    state.m_labelMode = renderer->getLabelMode();

    return state;
}

std::string
CelestiaState::createURL(UrlTimeSource timeSource) const
{
    const Universe& universe = *m_appCore.getSimulation()->getUniverse();

    std::string urlStr = fmt::format("{}{}", PROTOCOL, getCoordSysName(m_coordSys));

    if (m_coordSys != ObserverFrame::CoordinateSystem::Universal)
    {
        fmt::format_to(std::back_inserter(urlStr), "/{}", EncodedName(m_refBody, universe));
        if (m_coordSys == ObserverFrame::CoordinateSystem::PhaseLock)
            fmt::format_to(std::back_inserter(urlStr), "/{}", EncodedName(m_targetBody, universe));
    }

    // for backwards compatibility, always write the time component of the URL
    double tdb;
    if (auto time = std::get_if<UrlTime>(&m_time); time)
        tdb = time->urlTime;
    else
        tdb = m_appCore.getSimulation()->getTime();

    fmt::format_to(std::back_inserter(urlStr), "/{}?x={}&y={}&z={}&ow={}&ox={}&oy={}&oz={}",
                   astro::Date(tdb).toString(std::locale::classic(), astro::Date::ISO8601),
                   m_observerPosition.x,
                   m_observerPosition.y,
                   m_observerPosition.z,
                   m_observerOrientation.w(),
                   m_observerOrientation.x(),
                   m_observerOrientation.y(),
                   m_observerOrientation.z());

    if (!m_trackedBody.empty())
        fmt::format_to(std::back_inserter(urlStr), "&track={}", EncodedName(m_trackedBody, universe));
    if (!m_selectedBody.empty())
        fmt::format_to(std::back_inserter(urlStr), "&select={}", EncodedName(m_selectedBody, universe));

    // ShowEcliptic == 0x02000000, the last 1.6 parameter
    // we keep only old parameters and clear new ones
    RenderFlags rf = m_renderFlags & RF_MASK;
    // 1.6 uses ShowPlanets to control display of all types of solar
    // system objects. So set it if any one is displayed.
    if (util::is_set(m_renderFlags, RenderFlags::ShowSolarSystemObjects))
        rf |= RenderFlags::ShowPlanets;
    // But we need to store actual value of the bit which controls
    // planets display. 26th bit onwards are unused in 1.6.
    if (util::is_set(m_renderFlags, RenderFlags::ShowPlanets))
        rf |= NEW_SHOW_PLANETS_BIT_MASK;
    auto nrf = static_cast<std::uint64_t>(m_renderFlags) >> NEW_FLAG_BIT_1_7;

    fmt::format_to(std::back_inserter(urlStr),
                   "&fov={}&ts={}&ltd={:d}&p={:d}&rf={}&nrf={}&lm={}&tsrc={}&ver={}",
                   m_fieldOfView,
                   m_timeScale,
                   m_lightTimeDelay,
                   m_pauseState,
                   static_cast<std::uint64_t>(rf),
                   nrf,
                   static_cast<std::uint32_t>(m_labelMode),
                   static_cast<int>(timeSource),
                   UrlVersion);

    return urlStr;
}

std::optional<CelestiaState>
CelestiaState::parseURL(CelestiaCore& appCore, std::string_view urlStr)
{
    CelestiaState state(appCore);
    if (Parser parser(state); parser.parse(urlStr))
        return state;

    return std::nullopt;
}

void
CelestiaState::apply() const
{
    auto& sim = *m_appCore.getSimulation();
    auto& renderer = *m_appCore.getRenderer();

    sim.update(0.0);
    sim.setFrame(m_coordSys, m_refBody, m_targetBody);
    sim.getActiveObserver()->setFOV(math::degToRad(m_fieldOfView));
    m_appCore.setZoomFromFOV();
    sim.setTimeScale(m_timeScale);
    sim.setPauseState(m_pauseState);
    m_appCore.setLightDelayActive(m_lightTimeDelay);
    sim.setSelection(m_selectedBody);
    if (!m_trackedBody.empty())
        sim.setTrackedObject(m_trackedBody);
    renderer.setRenderFlags(m_renderFlags);
    renderer.setLabelMode(m_labelMode);

    class TimeVisitor
    {
    public:
        explicit TimeVisitor(Simulation& s) : simulation(s) {}

        double operator()(const UrlTime& t) const
        {
            simulation.setTime(t.urlTime);
            return t.urlTime;
        }

        double operator()(const SimulationTime&) const
        {
            return simulation.getTime();
        }

        double operator()(const SystemTime&) const
        {
            auto t = astro::UTCtoTDB(astro::Date::systemDate());
            simulation.setTime(t);
            return t;
        }

    private:
        Simulation& simulation;
    };

    double tdb = std::visit(TimeVisitor{sim}, m_time);
    const ObserverFrame* frame = sim.getObserver().getFrame().get();
    auto coord = frame->convertToUniversal(m_observerPosition, tdb);
    auto q = frame->convertToUniversal(m_observerOrientation.cast<double>(), tdb).cast<float>();
    sim.setObserverPosition(coord);
    sim.setObserverOrientation(q);
}

}
