// cmdparser.cpp
//
// Parse Celestia command files and turn them into CommandSequences.
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "cmdparser.h"

#ifdef USE_MINIAUDIO
#include <algorithm>
#endif
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <istream>
#include <map>
#include <optional>
#include <sstream>
#include <variant>

#include <Eigen/Core>
#include <fmt/format.h>

#include <celengine/hash.h>
#include <celengine/parser.h>
#include <celengine/render.h>
#include <celengine/value.h>
#ifdef USE_MINIAUDIO
#include <celestia/audiosession.h>
#endif
#include <celmath/mathlib.h>
#include <celscript/common/scriptmaps.h>
#include <celutil/logger.h>
#include <celutil/r128util.h>
#include <celutil/stringutils.h>
#include <celutil/tokenizer.h>

using namespace std::string_view_literals;
using celestia::util::GetLogger;
using celestia::util::DecodeFromBase64;

namespace celestia::scripts
{
namespace
{

std::uint64_t
parseRenderFlags(const std::string &s, const celestia::scripts::FlagMap64& RenderFlagMap)
{
    std::istringstream in(s);

    Tokenizer tokenizer(&in);
    std::uint64_t flags = 0;

    Tokenizer::TokenType ttype = tokenizer.nextToken();
    while (ttype != Tokenizer::TokenEnd)
    {
        if (auto tokenValue = tokenizer.getNameValue(); tokenValue.has_value())
        {
            if (RenderFlagMap.count(*tokenValue) == 0)
                GetLogger()->warn("Unknown render flag: {}\n", *tokenValue);
            else
                flags |= RenderFlagMap.at(*tokenValue);

            ttype = tokenizer.nextToken();
            if (ttype == Tokenizer::TokenBar)
                ttype = tokenizer.nextToken();
        }
    }

    return flags;
}


int
parseLabelFlags(const std::string &s, const celestia::scripts::FlagMap &LabelFlagMap)
{
    std::istringstream in(s);

    Tokenizer tokenizer(&in);
    int flags = 0;

    Tokenizer::TokenType ttype = tokenizer.nextToken();
    while (ttype != Tokenizer::TokenEnd)
    {
        if (auto tokenValue = tokenizer.getNameValue(); tokenValue.has_value())
        {
            if (LabelFlagMap.count(*tokenValue) == 0)
                GetLogger()->warn("Unknown label flag: {}\n", *tokenValue);
            else
                flags |= LabelFlagMap.at(*tokenValue);

            ttype = tokenizer.nextToken();
            if (ttype == Tokenizer::TokenBar)
                ttype = tokenizer.nextToken();
        }
    }

    return flags;
}


int
parseOrbitFlags(const std::string &s, const celestia::scripts::FlagMap &BodyTypeMap)
{
    std::istringstream in(s);

    Tokenizer tokenizer(&in);
    int flags = 0;

    Tokenizer::TokenType ttype = tokenizer.nextToken();
    while (ttype != Tokenizer::TokenEnd)
    {
        if (auto tokenValue = tokenizer.getNameValue(); tokenValue.has_value())
        {
            std::string name(*tokenValue);
            name[0] = std::toupper(static_cast<unsigned char>(name[0]));

            if (BodyTypeMap.count(name) == 0)
                GetLogger()->warn("Unknown orbit flag: {}\n", name);
            else
                flags |= BodyTypeMap.at(name);

            ttype = tokenizer.nextToken();
            if (ttype == Tokenizer::TokenBar)
                ttype = tokenizer.nextToken();
        }
    }

    return flags;
}


int
parseConstellations(CommandConstellations& cmd, const std::string &s, bool act)
{
    std::istringstream in(s);

    Tokenizer tokenizer(&in);
    int flags = 0;

    Tokenizer::TokenType ttype = tokenizer.nextToken();
    while (ttype != Tokenizer::TokenEnd)
    {
        auto tokenValue = tokenizer.getNameValue();
        if (!tokenValue.has_value())
        {
            GetLogger()->error("Command Parser: error parsing render flags\n");
            return 0;
        }

        if (compareIgnoringCase(*tokenValue, "all"sv) == 0)
        {
            if (act)
                cmd.flags.all = true;
            else
                cmd.flags.none = true;
        }
        else
            cmd.setValues(*tokenValue, act);

        ttype = tokenizer.nextToken();
        if (ttype == Tokenizer::TokenBar)
            ttype = tokenizer.nextToken();
    }

    return flags;
}


int
parseConstellationColor(CommandConstellationColor& cmd,
                        const std::string &s,
                        const Eigen::Vector3d &col,
                        bool act)
{
    std::istringstream in(s);

    Tokenizer tokenizer(&in);
    int flags = 0;

    if (act)
        cmd.setColor(static_cast<float>(col.x()),
                     static_cast<float>(col.y()),
                     static_cast<float>(col.z()));
    else
        cmd.unsetColor();

    Tokenizer::TokenType ttype = tokenizer.nextToken();
    while (ttype != Tokenizer::TokenEnd)
    {
        auto tokenValue = tokenizer.getNameValue();
        if (!tokenValue.has_value())
        {
            GetLogger()->error("Command Parser: error parsing render flags\n");
            return 0;
        }

        if (compareIgnoringCase(*tokenValue, "all"sv) == 0)
        {
            if (act)
                cmd.flags.all = true;
            else
                cmd.flags.none = true;
        }
        else
            cmd.setConstellations(*tokenValue);

        ttype = tokenizer.nextToken();
        if (ttype == Tokenizer::TokenBar)
            ttype = tokenizer.nextToken();
    }

    return flags;
}


ObserverFrame::CoordinateSystem parseCoordinateSystem(std::string_view name)
{
    if (compareIgnoringCase("observer"sv, name) == 0)
        return ObserverFrame::ObserverLocal;
    if (compareIgnoringCase("bodyfixed"sv, name) == 0 ||
        // 'geographic' is a deprecated name for the bodyfixed coordinate
        // system, maintained here for compatibility with older scripts.
        compareIgnoringCase("geographic"sv, name) == 0)
        return ObserverFrame::BodyFixed;
    if (compareIgnoringCase("equatorial"sv, name) == 0)
        return ObserverFrame::Equatorial;
    if (compareIgnoringCase("ecliptical"sv, name) == 0)
        return ObserverFrame::Ecliptical;
    if (compareIgnoringCase("universal"sv, name) == 0)
        return ObserverFrame::Universal;
    if (compareIgnoringCase("lock"sv, name) == 0)
        return ObserverFrame::PhaseLock;
    if (compareIgnoringCase("chase"sv, name) == 0)
        return ObserverFrame::Chase;
    return ObserverFrame::ObserverLocal;
}


using ParseResult = std::variant<std::unique_ptr<Command>, std::string>;


template<typename F>
class ParseResultVisitor
{
 public:
    explicit ParseResultVisitor(F _f) : f(_f) {}

    std::unique_ptr<Command> operator()(std::unique_ptr<Command> p) const { return p; }
    std::unique_ptr<Command> operator()(std::string&& s)
    {
        f(std::move(s));
        return nullptr;
    }

 private:
    F f;
};


inline ParseResult makeError(const char* message)
{
    return ParseResult{ std::in_place_type<std::string>, message };
}


template<typename T>
ParseResult parseParameterlessCommand(const Hash&, const ScriptMaps&)
{
    return std::make_unique<T>();
}


ParseResult parseWaitCommand(const Hash& paramList, const ScriptMaps&)
{
    auto duration = paramList.getNumber<double>("duration").value_or(1.0);
    return std::make_unique<CommandWait>(duration);
}


ParseResult parseSetCommand(const Hash& paramList, const ScriptMaps&)
{
    double value = 0.0;
    const std::string* name = paramList.getString("name");
    if (name == nullptr)
        return makeError("Missing name");

    if (auto valueOpt = paramList.getNumber<double>("value"); valueOpt.has_value())
        value = *valueOpt;
    else if (const std::string* valstr = paramList.getString("value"); valstr != nullptr)
    {
        // Some values may be specified via strings
        if (compareIgnoringCase(*valstr, "fuzzypoints") == 0)
            value = static_cast<double>(Renderer::FuzzyPointStars);
        else if (compareIgnoringCase(*valstr, "points") == 0)
            value = static_cast<double>(Renderer::PointStars);
        else if (compareIgnoringCase(*valstr, "scaleddiscs") == 0)
            value = static_cast<double>(Renderer::ScaledDiscStars);
    }

    return std::make_unique<CommandSet>(*name, value);
}


ParseResult parseSelectCommand(const Hash& paramList, const ScriptMaps&)
{
    const std::string* objStr = paramList.getString("object");
    return objStr == nullptr
        ? makeError("Missing object parameter to select")
        : std::make_unique<CommandSelect>(*objStr);
}


ParseResult parseSetFrameCommand(const Hash& paramList, const ScriptMaps&)
{
    const std::string* refName = paramList.getString("ref");
    if (refName == nullptr)
        return makeError("Missing ref parameter to setframe");

    const std::string* targetName = paramList.getString("target");
    if (targetName == nullptr)
        return makeError("Missing target parameter to setframe");

    ObserverFrame::CoordinateSystem coordSys;
    if (const std::string* coordSysName = paramList.getString("coordsys"); coordSysName == nullptr)
        coordSys = ObserverFrame::Universal;
    else
        coordSys = parseCoordinateSystem(*coordSysName);

    return std::make_unique<CommandSetFrame>(coordSys, *refName, *targetName);
}


ParseResult parseSetSurfaceCommand(const Hash& paramList, const ScriptMaps&)
{
    const std::string* name = paramList.getString("name");
    return name == nullptr
        ? makeError("Missing name parameter to setsurface")
        : std::make_unique<CommandSetSurface>(*name);
}


ParseResult parseGotoCommand(const Hash& paramList, const ScriptMaps&)
{
    auto t = paramList.getNumber<double>("time").value_or(1.0);
    auto distance = paramList.getNumber<double>("distance").value_or(5.0);

    ObserverFrame::CoordinateSystem upFrame = ObserverFrame::ObserverLocal;
    if (const std::string* frameString = paramList.getString("upframe"); frameString != nullptr)
        upFrame = parseCoordinateSystem(*frameString);

    auto up = paramList.getVector3<double>("up").value_or(Eigen::Vector3d::UnitY());

    return std::make_unique<CommandGoto>(t,
                                         distance,
                                         up.cast<float>(),
                                         upFrame);
}


ParseResult parseGotoLongLatCommand(const Hash& paramList, const ScriptMaps&)
{
    auto t         = paramList.getNumber<double>("time").value_or(1.0);
    auto distance  = paramList.getNumber<double>("distance").value_or(5.0);
    auto up        = paramList.getVector3<double>("up").value_or(Eigen::Vector3d::UnitY());
    auto longitude = paramList.getNumber<double>("longitude").value_or(0.0);
    auto latitude  = paramList.getNumber<double>("latitude").value_or(0.0);

    return std::make_unique<CommandGotoLongLat>(t,
                                                distance,
                                                static_cast<float>(celmath::degToRad(longitude)),
                                                static_cast<float>(celmath::degToRad(latitude)),
                                                up.cast<float>());
}


ParseResult parseGotoLocCommand(const Hash& paramList, const ScriptMaps&)
{
    auto t = paramList.getNumber<double>("time").value_or(1.0);

    if (auto pos = paramList.getVector3<double>("position"); pos.has_value())
    {
        *pos *= astro::kilometersToMicroLightYears(1.0);
        auto xrot = paramList.getNumber<double>("xrot").value_or(0.0);
        auto yrot = paramList.getNumber<double>("yrot").value_or(0.0);
        auto zrot = paramList.getNumber<double>("zrot").value_or(0.0);
        auto rotation = Eigen::Quaterniond(Eigen::AngleAxisd(celmath::degToRad(xrot), Eigen::Vector3d::UnitX()) *
                                           Eigen::AngleAxisd(celmath::degToRad(yrot), Eigen::Vector3d::UnitY()) *
                                           Eigen::AngleAxisd(celmath::degToRad(zrot), Eigen::Vector3d::UnitZ()));
        return std::make_unique<CommandGotoLocation>(t, *pos, rotation);
    }

    const std::string* x = paramList.getString("x");
    const std::string* y = paramList.getString("y");
    const std::string* z = paramList.getString("z");
    double xpos = x == nullptr ? 0.0 : static_cast<double>(DecodeFromBase64(*x));
    double ypos = y == nullptr ? 0.0 : static_cast<double>(DecodeFromBase64(*y));
    double zpos = z == nullptr ? 0.0 : static_cast<double>(DecodeFromBase64(*z));
    auto ow = paramList.getNumber<double>("ow").value_or(0.0);
    auto ox = paramList.getNumber<double>("ox").value_or(0.0);
    auto oy = paramList.getNumber<double>("oy").value_or(0.0);
    auto oz = paramList.getNumber<double>("oz").value_or(0.0);
    Eigen::Quaterniond orientation(ow, ox, oy, oz);
    return std::make_unique<CommandGotoLocation>(t, Eigen::Vector3d(xpos, ypos, zpos), orientation);
}


ParseResult parseSetUrlCommand(const Hash& paramList, const ScriptMaps&)
{
    const std::string* url = paramList.getString("url");
    return url == nullptr
        ? makeError("Missing url parameter to seturl")
        : std::make_unique<CommandSetUrl>(*url);
}


ParseResult parseCenterCommand(const Hash& paramList, const ScriptMaps&)
{
    auto t = paramList.getNumber<double>("time").value_or(1.0);
    return std::make_unique<CommandCenter>(t);
}


ParseResult parsePrintCommand(const Hash& paramList, const ScriptMaps&)
{
    const std::string* text = paramList.getString("text");
    if (text == nullptr)
        return makeError("Missing text parameter to print");

    auto duration = paramList.getNumber<double>("duration").value_or(1e9);
    auto voff     = paramList.getNumber<int>("row").value_or(0);
    auto hoff     = paramList.getNumber<int>("column").value_or(0);
    int horig = -1;
    int vorig = -1;
    if (const std::string* origin = paramList.getString("origin"); origin != nullptr && !origin->empty())
    {
        constexpr std::string_view top    = "top"sv;
        constexpr std::string_view bottom = "bottom"sv;
        constexpr std::string_view left   = "left"sv;
        constexpr std::string_view right  = "right"sv;
        constexpr std::string_view center = "center"sv;

        if (*origin == center)
        {
            horig = 0;
            vorig = 0;
        }
        else
        {
            // (bottom|top)(left|right)? or (left|right)
            std::string_view originSv = *origin;
            if (compareIgnoringCase(originSv, bottom, bottom.size()) == 0)
            {
                vorig = -1;
                originSv = originSv.substr(bottom.size());
            }
            else if (compareIgnoringCase(originSv, top, top.size()) == 0)
            {
                vorig = 1;
                originSv = originSv.substr(top.size());
            }
            else
                vorig = 0;

            if (originSv.empty())
                horig = 0; // was either "top" or "bottom" exactly
            else if (compareIgnoringCase(originSv, right) == 0)
                horig = 1;
            else if (compareIgnoringCase(originSv, left) == 0)
                horig = -1;
            else
            {
                // Could not parse the origin
                horig = -1;
                vorig = -1;
            }
        }
    }

    return std::make_unique<CommandPrint>(*text,
                                          horig, vorig, hoff, -voff,
                                          duration);
}


ParseResult parseTimeCommand(const Hash& paramList, const ScriptMaps&)
{
    double jd = 2451545.0;
    if (auto jdVal = paramList.getNumber<double>("jd"); jdVal.has_value())
    {
        jd = *jdVal;
    }
    else
    {
        const std::string* utc = paramList.getString("utc");
        if (utc == nullptr)
            return makeError("Missing either time or utc parameter to time");

        astro::Date date = astro::Date(0.0);
        std::sscanf(utc->c_str(), "%d-%d-%dT%d:%d:%lf",
                    &date.year, &date.month, &date.day,
                    &date.hour, &date.minute, &date.seconds);
        jd = static_cast<double>(date);
    }

    return std::make_unique<CommandSetTime>(jd);
}


ParseResult parseTimeRateCommand(const Hash& paramList, const ScriptMaps&)
{
    auto rate = paramList.getNumber<double>("rate").value_or(1.0);
    return std::make_unique<CommandSetTimeRate>(rate);
}


ParseResult parseChangeDistanceCommand(const Hash& paramList, const ScriptMaps&)
{
    auto rate     = paramList.getNumber<double>("rate").value_or(0.0);
    auto duration = paramList.getNumber<double>("duration").value_or(1.0);
    return std::make_unique<CommandChangeDistance>(duration, rate);
}


ParseResult parseOrbitCommand(const Hash& paramList, const ScriptMaps&)
{
    auto duration = paramList.getNumber<double>("duration").value_or(1.0);
    auto rate     = paramList.getNumber<double>("rate").value_or(0.0);
    auto axis     = paramList.getVector3<double>("axis").value_or(Eigen::Vector3d::Zero());
    return std::make_unique<CommandOrbit>(duration,
                                          axis.cast<float>(),
                                          static_cast<float>(celmath::degToRad(rate)));
}


ParseResult parseRotateCommand(const Hash& paramList, const ScriptMaps&)
{
    auto duration = paramList.getNumber<double>("duration").value_or(1.0);
    auto rate     = paramList.getNumber<double>("rate").value_or(0.0);
    auto axis     = paramList.getVector3<double>("axis").value_or(Eigen::Vector3d::Zero());
    return std::make_unique<CommandRotate>(duration,
                                           axis.cast<float>(),
                                           static_cast<float>(celmath::degToRad(rate)));
}


ParseResult parseMoveCommand(const Hash& paramList, const ScriptMaps&)
{
    auto duration = paramList.getNumber<double>("duration").value_or(1.0);
    auto velocity = paramList.getVector3<double>("velocity").value_or(Eigen::Vector3d::Zero());
    return std::make_unique<CommandMove>(duration, velocity * astro::kilometersToMicroLightYears(1.0));
}


ParseResult parseSetPositionCommand(const Hash& paramList, const ScriptMaps&)
{
    // Base position in light years, offset in kilometers
    if (auto base = paramList.getVector3<float>("base"); base.has_value())
    {
        auto offset = paramList.getVector3<double>("offset").value_or(Eigen::Vector3d::Zero());
        UniversalCoord basePosition = UniversalCoord::CreateLy(base->cast<double>());
        return std::make_unique<CommandSetPosition>(basePosition.offsetKm(offset));
    }

    const std::string* x = paramList.getString("x");
    const std::string* y = paramList.getString("y");
    const std::string* z = paramList.getString("z");
    return x == nullptr || y == nullptr || z == nullptr
        ? makeError("Missing x, y, z or base, offset parameters to setposition")
        : std::make_unique<CommandSetPosition>(UniversalCoord(DecodeFromBase64(*x),
                                                              DecodeFromBase64(*y),
                                                              DecodeFromBase64(*z)));
}


ParseResult parseSetOrientationCommand(const Hash& paramList, const ScriptMaps&)
{
    if (auto angle = paramList.getNumber<double>("angle"); angle.has_value())
    {
        auto axis = paramList.getVector3<double>("axis").value_or(Eigen::Vector3d::Zero());
        auto orientation = Eigen::Quaternionf(Eigen::AngleAxisf(static_cast<float>(celmath::degToRad(*angle)),
                                                                axis.cast<float>().normalized()));
        return std::make_unique<CommandSetOrientation>(orientation);
    }

    auto ow = paramList.getNumber<double>("ow").value_or(0.0);
    auto ox = paramList.getNumber<double>("ox").value_or(0.0);
    auto oy = paramList.getNumber<double>("oy").value_or(0.0);
    auto oz = paramList.getNumber<double>("oz").value_or(0.0);
    Eigen::Quaternionf orientation(ow, ox, oy, oz);
    return std::make_unique<CommandSetOrientation>(orientation);
}


ParseResult parseRenderFlagsCommand(const Hash& paramList, const ScriptMaps& scriptMaps)
{
    std::uint64_t setFlags = 0;
    std::uint64_t clearFlags = 0;

    if (const std::string* s = paramList.getString("set"); s != nullptr)
        setFlags = parseRenderFlags(*s, scriptMaps.RenderFlagMap);
    if (const std::string* s = paramList.getString("clear"); s != nullptr)
        clearFlags = parseRenderFlags(*s, scriptMaps.RenderFlagMap);

    return std::make_unique<CommandRenderFlags>(setFlags, clearFlags);
}


ParseResult parseLabelsCommand(const Hash& paramList, const ScriptMaps& scriptMaps)
{
    int setFlags = 0;
    int clearFlags = 0;

    if (const std::string* s = paramList.getString("set"); s != nullptr)
        setFlags = parseLabelFlags(*s, scriptMaps.LabelFlagMap);
    if (const std::string* s = paramList.getString("clear"); s != nullptr)
        clearFlags = parseLabelFlags(*s, scriptMaps.LabelFlagMap);

    return std::make_unique<CommandLabels>(setFlags, clearFlags);
}


ParseResult parseOrbitFlagsCommand(const Hash& paramList, const ScriptMaps& scriptMaps)
{
    int setFlags = 0;
    int clearFlags = 0;

    if (const std::string* s = paramList.getString("set"); s != nullptr)
        setFlags = parseOrbitFlags(*s, scriptMaps.OrbitVisibilityMap);
    if (const std::string* s = paramList.getString("clear"); s != nullptr)
        clearFlags = parseOrbitFlags(*s, scriptMaps.OrbitVisibilityMap);

    return std::make_unique<CommandOrbitFlags>(setFlags, clearFlags);
}


ParseResult parseConstellationsCommand(const Hash& paramList, const ScriptMaps&)
{
    auto cmdcons = std::make_unique<CommandConstellations>();

    if (const std::string* s = paramList.getString("set"); s != nullptr)
        parseConstellations(*cmdcons, *s, true);
    if (const std::string* s = paramList.getString("clear"); s != nullptr)
        parseConstellations(*cmdcons, *s, false);
    return ParseResult(std::move(cmdcons));
}


ParseResult parseConstellationColorCommand(const Hash& paramList, const ScriptMaps&)
{
    auto cmdconcol = std::make_unique<CommandConstellationColor>();

    auto colorv = paramList.getVector3<double>("color").value_or(Eigen::Vector3d::UnitX());

    if (const std::string* s = paramList.getString("set"); s != nullptr)
        parseConstellationColor(*cmdconcol, *s, colorv, true);
    if (const std::string* s = paramList.getString("clear"); s != nullptr)
        parseConstellationColor(*cmdconcol, *s, colorv, false);
    return ParseResult(std::move(cmdconcol));
}


ParseResult parseSetVisibilityLimitCommand(const Hash& paramList, const ScriptMaps&)
{
    auto mag = paramList.getNumber<double>("magnitude").value_or(6.0);
    return std::make_unique<CommandSetVisibilityLimit>(mag);
}


ParseResult parseSetFaintestAutoMag45DegCommand(const Hash& paramList, const ScriptMaps&)
{
    auto mag = paramList.getNumber<double>("magnitude").value_or(8.5);
    return std::make_unique<CommandSetFaintestAutoMag45deg>(mag);
}


ParseResult parseSetAmbientLightCommand(const Hash& paramList, const ScriptMaps&)
{
    auto brightness = paramList.getNumber<float>("brightness").value_or(0.0f);
    return std::make_unique<CommandSetAmbientLight>(brightness);
}


ParseResult parseSetGalaxyLightGainCommand(const Hash& paramList, const ScriptMaps&)
{
    auto gain = paramList.getNumber<float>("gain").value_or(0.0f);
    return std::make_unique<CommandSetGalaxyLightGain>(gain);
}


ParseResult parseSetTextureResolutionCommand(const Hash& paramList, const ScriptMaps&)
{
    unsigned int res = 1;
    if (const std::string* textureRes = paramList.getString("resolution"); textureRes != nullptr)
    {
        if (compareIgnoringCase(*textureRes, "low") == 0)
            res = 0;
        else if (compareIgnoringCase(*textureRes, "medium") == 0)
            res = 1;
        else if (compareIgnoringCase(*textureRes, "high") == 0)
            res = 2;
    }

    return std::make_unique<CommandSetTextureResolution>(res);
}


ParseResult parsePreloadTexCommand(const Hash& paramList, const ScriptMaps&)
{
    const std::string* object = paramList.getString("object");
    return object == nullptr
        ? makeError("Missing object parameter to preloadtex")
        : std::make_unique<CommandPreloadTextures>(*object);
}


ParseResult parseMarkCommand(const Hash& paramList, const ScriptMaps&)
{
    const std::string* object = paramList.getString("object");
    if (object == nullptr)
        return makeError("Missing object parameter to mark");

    auto size   = paramList.getNumber<double>("size").value_or(10.0);
    auto colorv = paramList.getVector3<float>("color").value_or(Eigen::Vector3f::UnitX());
    auto alpha  = paramList.getNumber<float>("alpha").value_or(0.9f);
    Color color(colorv.x(), colorv.y(), colorv.z(), alpha);

    celestia::MarkerRepresentation::Symbol symbol = celestia::MarkerRepresentation::Diamond;
    if (const std::string* symbolString = paramList.getString("symbol"); symbolString != nullptr)
    {
        if (compareIgnoringCase("diamond"sv, *symbolString) == 0)
            symbol = celestia::MarkerRepresentation::Diamond;
        else if (compareIgnoringCase("triangle"sv, *symbolString) == 0)
            symbol = celestia::MarkerRepresentation::Triangle;
        else if (compareIgnoringCase("square"sv, *symbolString) == 0)
            symbol = celestia::MarkerRepresentation::Square;
        else if (compareIgnoringCase("filledsquare"sv, *symbolString) == 0)
            symbol = celestia::MarkerRepresentation::FilledSquare;
        else if (compareIgnoringCase("plus"sv, *symbolString) == 0)
            symbol = celestia::MarkerRepresentation::Plus;
        else if (compareIgnoringCase("x"sv, *symbolString) == 0)
            symbol = celestia::MarkerRepresentation::X;
        else if (compareIgnoringCase("leftarrow"sv, *symbolString) == 0)
            symbol = celestia::MarkerRepresentation::LeftArrow;
        else if (compareIgnoringCase("rightarrow"sv, *symbolString) == 0)
            symbol = celestia::MarkerRepresentation::RightArrow;
        else if (compareIgnoringCase("uparrow"sv, *symbolString) == 0)
            symbol = celestia::MarkerRepresentation::UpArrow;
        else if (compareIgnoringCase("downarrow"sv, *symbolString) == 0)
            symbol = celestia::MarkerRepresentation::DownArrow;
        else if (compareIgnoringCase("circle"sv, *symbolString) == 0)
            symbol = celestia::MarkerRepresentation::Circle;
        else if (compareIgnoringCase("disk"sv, *symbolString) == 0)
            symbol = celestia::MarkerRepresentation::Disk;
    }

    celestia::MarkerRepresentation rep{symbol};

    rep.setSize(static_cast<float>(size));
    rep.setColor(color);
    if (const std::string* label = paramList.getString("label"); label != nullptr)
        rep.setLabel(*label);

    bool occludable = paramList.getBoolean("occludable").value_or(true);

    return std::make_unique<CommandMark>(*object, rep, occludable);
}


ParseResult parseUnmarkCommand(const Hash& paramList, const ScriptMaps&)
{
    const std::string* object = paramList.getString("object");
    return object == nullptr
        ? makeError("Missing object parameter to unmark")
        : std::make_unique<CommandUnmark>(*object);
}


ParseResult parseCaptureCommand(const Hash& paramList, const ScriptMaps&)
{
    const std::string* filename = paramList.getString("filename");
    if (filename == nullptr)
        return makeError("Missing filename parameter to capture");

    std::string type;
    if (const std::string* typeStr = paramList.getString("type"); typeStr != nullptr)
    {
        type = *typeStr;
    }

    return std::make_unique<CommandCapture>(type, *filename);
}


ParseResult parseSplitViewCommand(const Hash& paramList, const ScriptMaps&)
{
    auto view = paramList.getNumber<unsigned int>("view").value_or(1u);
    std::string splitType;
    if (const std::string* splitVal = paramList.getString("type"); splitVal != nullptr)
    {
        splitType = *splitVal;
    }

    auto splitPos = paramList.getNumber<float>("position").value_or(0.5f);
    return std::make_unique<CommandSplitView>(view, splitType, splitPos);
}


ParseResult parseDeleteViewCommand(const Hash& paramList, const ScriptMaps&)
{
    auto view = paramList.getNumber<unsigned int>("view").value_or(1u);
    return std::make_unique<CommandDeleteView>(view);
}


ParseResult parseSetActiveViewCommand(const Hash& paramList, const ScriptMaps&)
{
    auto view = paramList.getNumber<unsigned int>("view").value_or(1u);
    return std::make_unique<CommandSetActiveView>(view);
}


ParseResult parseSetRadiusCommand(const Hash& paramList, const ScriptMaps&)
{
    const std::string* object = paramList.getString("object");
    if (object == nullptr)
        return makeError("Missing object parameter to setradius");

    auto radius = paramList.getNumber<float>("radius").value_or(1.0f);
    return std::make_unique<CommandSetRadius>(*object, radius);
}


ParseResult parseSetLineColorCommand(const Hash& paramList, const ScriptMaps&)
{
    const std::string* item = paramList.getString("item");
    if (item == nullptr)
        return makeError("Missing item parameter to setlinecolor");

    auto colorv = paramList.getVector3<float>("color").value_or(Eigen::Vector3f::UnitX());
    Color color(colorv.x(), colorv.y(), colorv.z());
    return std::make_unique<CommandSetLineColor>(*item, color);
}


ParseResult parseSetLabelColorCommand(const Hash& paramList, const ScriptMaps&)
{
    const std::string* item = paramList.getString("item");
    if (item == nullptr)
        return makeError("Missing item parameter to setlabelcolor");

    auto colorv = paramList.getVector3<float>("color").value_or(Eigen::Vector3f::UnitX());
    Color color(colorv.x(), colorv.y(), colorv.z());
    return std::make_unique<CommandSetLabelColor>(*item, color);
}


ParseResult parseSetTextColorCommand(const Hash& paramList, const ScriptMaps&)
{
    auto colorv = paramList.getVector3<float>("color").value_or(Eigen::Vector3f::Ones());
    Color color(colorv.x(), colorv.y(), colorv.z());
    return std::make_unique<CommandSetTextColor>(color);
}


ParseResult parsePlayCommand(const Hash& paramList, const ScriptMaps&)
{
#ifdef USE_MINIAUDIO
    int channel = std::max(paramList.getNumber<int>("channel").value_or(celestia::defaultAudioChannel),
                           celestia::minAudioChannel);

    auto optionalVolume = paramList.getNumber<float>("volume");
    if (optionalVolume.has_value())
        optionalVolume = std::clamp(*optionalVolume, celestia::minAudioVolume, celestia::maxAudioVolume);
    float pan = std::clamp(paramList.getNumber<float>("pan").value_or(celestia::defaultAudioPan),
                           celestia::minAudioPan, celestia::maxAudioPan);
    int nopause = paramList.getNumber<int>("nopause").value_or(0);
    std::optional<bool> optionalLoop = std::nullopt;
    if (auto loop = paramList.getNumber<int>("loop"); loop.has_value())
        optionalLoop = *loop == 1;
    std::optional<std::string> optionalFilename = std::nullopt;
    if (auto filename = paramList.getString("filename"); filename != nullptr)
        optionalFilename = *filename;

    return std::make_unique<CommandPlay>(channel, optionalVolume, pan, optionalLoop, optionalFilename, nopause == 1);
#else
    return std::make_unique<CommandNoOp>();
#endif
}


ParseResult parseOverlayCommand(const Hash& paramList, const ScriptMaps&)
{
    auto duration = paramList.getNumber<float>("duration").value_or(3.0f);
    auto xoffset  = paramList.getNumber<float>("xoffset").value_or(0.0f);
    auto yoffset  = paramList.getNumber<float>("yoffset").value_or(0.0f);
    std::optional<float> alpha = paramList.getNumber<float>("alpha");

    const std::string* filename = paramList.getString("filename");
    if (filename == nullptr)
        return makeError("Missing filename parameter to overlay");

    bool fitscreen = false;
    // The below triggers a Sonar rule to use value_or in the outer condition,
    // however this would be eagerly-evaluated and cause unnecessary map
    // lookups in the case that the boolean lookup succeeds.
    if (auto fitscreenValue = paramList.getBoolean("fitscreen"); fitscreenValue.has_value()) // NOSONAR
        fitscreen = *fitscreenValue;
    else
        fitscreen = paramList.getNumber<int>("fitscreen").value_or(0) != 0;

    std::array<Color, 4> colors;
    Color color = paramList.getColor("color").value_or(Color::White);
    colors.fill(alpha.has_value() ? Color(color, *alpha) : color);

    if (auto colorTop = paramList.getColor("colortop"); colorTop.has_value())
        colors[0] = colors[1] = alpha.has_value() ? Color(*colorTop, *alpha) : *colorTop;
    if (auto colorBottom = paramList.getColor("colorbottom"); colorBottom.has_value())
        colors[2] = colors[3] = alpha.has_value() ? Color(*colorBottom, *alpha) : *colorBottom;
    if (auto colorTopLeft = paramList.getColor("colortopleft"); colorTopLeft.has_value())
        colors[0] = alpha.has_value() ? Color(*colorTopLeft, *alpha) : *colorTopLeft;
    if (auto colorTopRight = paramList.getColor("colortopright"); colorTopRight.has_value())
        colors[1] = alpha.has_value() ? Color(*colorTopRight, *alpha) : *colorTopRight;
    if (auto colorBottomRight = paramList.getColor("colorbottomright"); colorBottomRight.has_value())
        colors[2] = alpha.has_value() ? Color(*colorBottomRight, *alpha) : *colorBottomRight;
    if (auto colorBottomLeft = paramList.getColor("colorbottomleft"); colorBottomLeft.has_value())
        colors[3] = alpha.has_value() ? Color(*colorBottomLeft, *alpha) : *colorBottomLeft;

    auto fadeafter = paramList.getNumber<float>("fadeafter").value_or(duration);

    return std::make_unique<CommandScriptImage>(duration,
                                                fadeafter,
                                                xoffset,
                                                yoffset,
                                                *filename,
                                                fitscreen,
                                                colors);
}


ParseResult parseVerbosityCommand(const Hash& paramList, const ScriptMaps&)
{
    int level = paramList.getNumber<int>("level").value_or(2);
    return std::make_unique<CommandVerbosity>(level);
}


ParseResult parseSetWindowBordersVisibleCommand(const Hash& paramList, const ScriptMaps&)
{
    bool visible = paramList.getBoolean("visible").value_or(true);
    return std::make_unique<CommandSetWindowBordersVisible>(visible);
}


ParseResult parseSetRingsTextureCommand(const Hash& paramList, const ScriptMaps&)
{
    const std::string* object = paramList.getString("object");
    if (object == nullptr)
        return makeError("Missing object parameter to setringstexture");

    const std::string* texture = paramList.getString("texture");
    if (texture == nullptr)
        return makeError("Missing texture parameter to setringstexture");

    std::string path;
    if (const std::string* pathStr = paramList.getString("path"); pathStr != nullptr)
    {
        path = *pathStr;
    }

    return std::make_unique<CommandSetRingsTexture>(*object, *texture, path);
}


ParseResult parseLoadFragmentCommand(const Hash& paramList, const ScriptMaps&)
{
    const std::string* type = paramList.getString("type");
    if (type == nullptr)
        return makeError("Missing type parameter to loadfragment");

    const std::string* fragment = paramList.getString("fragment");
    if (fragment == nullptr)
        return makeError("Missing fragment parameter to loadfragment");

    std::string dir;
    if (const std::string* dirName = paramList.getString("dir"); dirName != nullptr)
        dir = *dirName;

    return std::make_unique<CommandLoadFragment>(*type, *fragment, dir);
}


using ParseCommandPtr = ParseResult (*)(const Hash&, const ScriptMaps&);
using ParseCommandMap = std::map<std::string_view, ParseCommandPtr>;

std::unique_ptr<ParseCommandMap> createParseCommandMap()
{
    return std::make_unique<ParseCommandMap>(
        std::initializer_list<ParseCommandMap::value_type>
        {
            { "wait"sv,                    &parseWaitCommand                              },
            { "set"sv,                     &parseSetCommand                               },
            { "select"sv,                  &parseSelectCommand                            },
            { "setframe"sv,                &parseSetFrameCommand                          },
            { "setsurface"sv,              &parseSetSurfaceCommand                        },
            { "goto"sv,                    &parseGotoCommand                              },
            { "gotolonglat"sv,             &parseGotoLongLatCommand                       },
            { "gotoloc"sv,                 &parseGotoLocCommand                           },
            { "seturl"sv,                  &parseSetUrlCommand                            },
            { "center"sv,                  &parseCenterCommand                            },
            { "follow"sv,                  &parseParameterlessCommand<CommandFollow>      },
            { "synchronous"sv,             &parseParameterlessCommand<CommandSynchronous> },
            { "lock"sv,                    &parseParameterlessCommand<CommandLock>        },
            { "chase"sv,                   &parseParameterlessCommand<CommandChase>       },
            { "track"sv,                   &parseParameterlessCommand<CommandTrack>       },
            { "cancel"sv,                  &parseParameterlessCommand<CommandCancel>      },
            { "exit"sv,                    &parseParameterlessCommand<CommandExit>        },
            { "print"sv,                   &parsePrintCommand                             },
            { "cls"sv,                     &parseParameterlessCommand<CommandClearScreen> },
            { "time"sv,                    &parseTimeCommand                              },
            { "timerate"sv,                &parseTimeRateCommand                          },
            { "changedistance"sv,          &parseChangeDistanceCommand                    },
            { "orbit"sv,                   &parseOrbitCommand                             },
            { "rotate"sv,                  &parseRotateCommand                            },
            { "move"sv,                    &parseMoveCommand                              },
            { "setposition"sv,             &parseSetPositionCommand                       },
            { "setorientation"sv,          &parseSetOrientationCommand                    },
            { "lookback"sv,                &parseParameterlessCommand<CommandLookBack>    },
            { "renderflags"sv,             &parseRenderFlagsCommand                       },
            { "labels"sv,                  &parseLabelsCommand                            },
            { "orbitflags"sv,              &parseOrbitFlagsCommand                        },
            { "constellations"sv,          &parseConstellationsCommand                    },
            { "constellationcolor"sv,      &parseConstellationColorCommand                },
            { "setvisibilitylimit"sv,      &parseSetVisibilityLimitCommand                },
            { "setfaintestautomag45deg"sv, &parseSetFaintestAutoMag45DegCommand           },
            { "setambientlight"sv,         &parseSetAmbientLightCommand                   },
            { "setgalaxylightgain"sv,      &parseSetGalaxyLightGainCommand                },
            { "settextureresolution"sv,    &parseSetTextureResolutionCommand              },
            { "preloadtex"sv,              &parsePreloadTexCommand                        },
            { "mark"sv,                    &parseMarkCommand                              },
            { "unmark"sv,                  &parseUnmarkCommand                            },
            { "unmarkall"sv,               &parseParameterlessCommand<CommandUnmarkAll>   },
            { "capture"sv,                 &parseCaptureCommand                           },
            { "renderpath"sv,              &parseParameterlessCommand<CommandNoOp>        },
            { "splitview"sv,               &parseSplitViewCommand                         },
            { "deleteview"sv,              &parseDeleteViewCommand                        },
            { "singleview"sv,              &parseParameterlessCommand<CommandSingleView>  },
            { "setactiveview"sv,           &parseSetActiveViewCommand                     },
            { "setradius"sv,               &parseSetRadiusCommand                         },
            { "setlinecolor"sv,            &parseSetLineColorCommand                      },
            { "setlabelcolor"sv,           &parseSetLabelColorCommand                     },
            { "settextcolor"sv,            &parseSetTextColorCommand                      },
            { "play"sv,                    &parsePlayCommand                              },
            { "overlay"sv,                 &parseOverlayCommand                           },
            { "verbosity"sv,               &parseVerbosityCommand                         },
            { "setwindowbordersvisible"sv, &parseSetWindowBordersVisibleCommand           },
            { "setringstexture"sv,         &parseSetRingsTextureCommand                   },
            { "loadfragment"sv,            &parseLoadFragmentCommand                      },
        });
}

} // end unnamed namespace


CommandParser::CommandParser(std::istream& in, const std::shared_ptr<ScriptMaps> &sm) :
    tokenizer(std::make_unique<Tokenizer>(&in)),
    scriptMaps(sm)
{
    parser = std::make_unique<Parser>(tokenizer.get());
}


CommandParser::~CommandParser() = default;


CommandSequence CommandParser::parse()
{
    CommandSequence seq;

    if (tokenizer->nextToken() != Tokenizer::TokenBeginGroup)
    {
        error("'{' expected at start of script.");
        return {};
    }

    Tokenizer::TokenType ttype = tokenizer->nextToken();
    while (ttype != Tokenizer::TokenEnd && ttype != Tokenizer::TokenEndGroup)
    {
        tokenizer->pushBack();
        std::unique_ptr<Command> cmd = parseCommand();
        if (cmd == nullptr)
        {
            return {};
        }
        else
        {
            seq.emplace_back(std::move(cmd));
        }

        ttype = tokenizer->nextToken();
    }

    if (ttype != Tokenizer::TokenEndGroup)
    {
        error("Missing '}' at end of script.");
        return {};
    }

    return seq;
}


celestia::util::array_view<std::string> CommandParser::getErrors() const
{
    return errorList;
}


void CommandParser::error(std::string&& errMsg)
{
    errorList.emplace_back(std::move(errMsg));
}


std::unique_ptr<Command> CommandParser::parseCommand()
{
    tokenizer->nextToken();
    std::string commandName;
    if (auto tokenValue = tokenizer->getNameValue(); tokenValue.has_value())
    {
        commandName = *tokenValue;
    }
    else
    {
        error("Invalid command name");
        return nullptr;
    }

    const Value paramListValue = parser->readValue();
    const Hash* paramList = paramListValue.getHash();
    if (paramList == nullptr)
    {
        error("Bad parameter list");
        return nullptr;
    }

    static const ParseCommandMap* parseCommandMap = createParseCommandMap().release();
    auto it = parseCommandMap->find(commandName);
    if (it == parseCommandMap->end())
    {
        error(fmt::format("Unknown command name '{}'", commandName));
        return nullptr;
    }

    return std::visit(ParseResultVisitor([this](std::string&& s) { error(std::move(s)); }),
                      it->second(*paramList, *scriptMaps));
}

} // end namespace celestia::scripts
