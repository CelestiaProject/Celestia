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

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string_view>
#include <Eigen/Geometry>
#include <celutil/logger.h>
#include <celmath/mathlib.h>
#include <celengine/astro.h>
#include <celengine/parser.h>
#include <celutil/stringutils.h>
#include <celutil/tokenizer.h>
#include <celestia/audiosession.h>
#include "cmdparser.h"

using namespace std;
using namespace celmath;
using namespace celestia;
using namespace celestia::scripts;
using celestia::util::GetLogger;
using celestia::util::DecodeFromBase64;


static uint64_t parseRenderFlags(const string&, const FlagMap64&);
static int parseLabelFlags(const string&, const FlagMap&);
static int parseOrbitFlags(const string&, const FlagMap&);
static int parseConstellations(CommandConstellations* cmd, const string &s, int act);
int parseConstellationColor(CommandConstellationColor* cmd, const string &s, Eigen::Vector3d *col, int act);

CommandParser::CommandParser(istream& in, const shared_ptr<ScriptMaps> &sm) :
    scriptMaps(sm)
{
    tokenizer = new Tokenizer(&in);
    parser = new Parser(tokenizer);
}

CommandParser::CommandParser(Tokenizer& tok, const shared_ptr<ScriptMaps> &sm) :
    scriptMaps(sm)
{
    tokenizer = &tok;
    parser = new Parser(tokenizer);
}

CommandParser::~CommandParser()
{
    delete parser;
    delete tokenizer;
}


CommandSequence* CommandParser::parse()
{
    CommandSequence* seq = new CommandSequence();

    if (tokenizer->nextToken() != Tokenizer::TokenBeginGroup)
    {
        error("'{' expected at start of script.");
        delete seq;
        return nullptr;
    }

    Tokenizer::TokenType ttype = tokenizer->nextToken();
    while (ttype != Tokenizer::TokenEnd && ttype != Tokenizer::TokenEndGroup)
    {
        tokenizer->pushBack();
        Command* cmd = parseCommand();
        if (cmd == nullptr)
        {
            for_each(seq->begin(), seq->end(), [](Command* cmd) { delete cmd; });
            delete seq;
            return nullptr;
        }
        else
        {
            seq->push_back(cmd);
        }

        ttype = tokenizer->nextToken();
    }

    if (ttype != Tokenizer::TokenEndGroup)
    {
        error("Missing '}' at end of script.");
        for_each(seq->begin(), seq->end(), [](Command* cmd) { delete cmd; });
        delete seq;
        return nullptr;
    }

    return seq;
}


auto CommandParser::getErrors() const
    -> celestia::util::array_view<std::string>
{
    return errorList;
}


void CommandParser::error(string errMsg)
{
    errorList.emplace_back(std::move(errMsg));
}


static ObserverFrame::CoordinateSystem parseCoordinateSystem(const string& name)
{
    // 'geographic' is a deprecated name for the bodyfixed coordinate system,
    // maintained here for compatibility with older scripts.
    if (compareIgnoringCase(name, "observer") == 0)
        return ObserverFrame::ObserverLocal;
    else if (compareIgnoringCase(name, "bodyfixed") == 0)
        return ObserverFrame::BodyFixed;
    else if (compareIgnoringCase(name, "geographic") == 0)
        return ObserverFrame::BodyFixed;
    else if (compareIgnoringCase(name, "equatorial") == 0)
        return ObserverFrame::Equatorial;
    else if (compareIgnoringCase(name, "ecliptical") == 0)
        return ObserverFrame::Ecliptical;
    else if (compareIgnoringCase(name, "universal") == 0)
        return ObserverFrame::Universal;
    else if (compareIgnoringCase(name, "lock") == 0)
        return ObserverFrame::PhaseLock;
    else if (compareIgnoringCase(name, "chase") == 0)
        return ObserverFrame::Chase;
    else
        return ObserverFrame::ObserverLocal;
}


Command* CommandParser::parseCommand()
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

    Command* cmd = nullptr;

    if (commandName == "wait")
    {
        auto duration = paramList->getNumber<double>("duration").value_or(1.0);
        cmd = new CommandWait(duration);
    }
    else if (commandName == "set")
    {
        double value = 0.0;
        const std::string* name = paramList->getString("name");
        if (name == nullptr)
        {
            error("Missing name");
            return nullptr;
        }
        if (auto valueOpt = paramList->getNumber<double>("value"); valueOpt.has_value())
        {
            value = *valueOpt;
        }
        else if (const std::string* valstr = paramList->getString("value"); valstr != nullptr)
        {
            // Some values may be specified via strings
            if (compareIgnoringCase(*valstr, "fuzzypoints") == 0)
                value = (double) Renderer::FuzzyPointStars;
            else if (compareIgnoringCase(*valstr, "points") == 0)
                value = (double) Renderer::PointStars;
            else if (compareIgnoringCase(*valstr, "scaleddiscs") == 0)
                value = (double) Renderer::ScaledDiscStars;
        }

        cmd = new CommandSet(*name, value);
    }
    else if (commandName == "select")
    {
        string object;
        if (const std::string* objStr = paramList->getString("object"); objStr != nullptr)
        {
            object = *objStr;
        }

        cmd = new CommandSelect(object);
    }
    else if (commandName == "setframe")
    {
        const std::string* refName = paramList->getString("ref");
        if (refName == nullptr)
        {
            error("Missing ref parameter to setframe");
            return nullptr;
        }

        const std::string* targetName = paramList->getString("target");
        if (targetName == nullptr)
        {
            error("Missing target parameter to setframe");
            return nullptr;
        }

        ObserverFrame::CoordinateSystem coordSys;
        if (const std::string* coordSysName = paramList->getString("coordsys"); coordSysName == nullptr)
            coordSys = ObserverFrame::Universal;
        else
            coordSys = parseCoordinateSystem(*coordSysName);

        cmd = new CommandSetFrame(coordSys, *refName, *targetName);
    }
    else if (commandName == "setsurface")
    {
        const std::string* name = paramList->getString("name");
        if (name == nullptr)
        {
            error("Missing name parameter to setsurface");
            return nullptr;
        }

        cmd = new CommandSetSurface(*name);
    }
    else if (commandName == "goto")
    {
        auto t = paramList->getNumber<double>("time").value_or(1.0);
        auto distance = paramList->getNumber<double>("distance").value_or(5.0);

        ObserverFrame::CoordinateSystem upFrame = ObserverFrame::ObserverLocal;
        if (const std::string* frameString = paramList->getString("upframe"); frameString != nullptr)
            upFrame = parseCoordinateSystem(*frameString);

        auto up = paramList->getVector3<double>("up").value_or(Eigen::Vector3d::UnitY());

        cmd = new CommandGoto(t,
                              distance,
                              up.cast<float>(),
                              upFrame);
    }
    else if (commandName == "gotolonglat")
    {
        auto t = paramList->getNumber<double>("time").value_or(1.0);
        auto distance = paramList->getNumber<double>("distance").value_or(5.0);
        auto up = paramList->getVector3<double>("up").value_or(Eigen::Vector3d::UnitY());
        auto longitude = paramList->getNumber<double>("longitude").value_or(0.0);
        auto latitude = paramList->getNumber<double>("latitude").value_or(0.0);

        cmd = new CommandGotoLongLat(t,
                                     distance,
                                     (float) degToRad(longitude),
                                     (float) degToRad(latitude),
                                     up.cast<float>());
    }
    else if (commandName == "gotoloc")
    {
        auto t = paramList->getNumber<double>("time").value_or(1.0);

        if (auto pos = paramList->getVector3<double>("position"); pos.has_value())
        {
            *pos *= astro::kilometersToMicroLightYears(1.0);
            auto xrot = paramList->getNumber<double>("xrot").value_or(0.0);
            auto yrot = paramList->getNumber<double>("yrot").value_or(0.0);
            auto zrot = paramList->getNumber<double>("zrot").value_or(0.0);
            auto rotation = Eigen::Quaterniond(Eigen::AngleAxisd(degToRad(xrot), Eigen::Vector3d::UnitX()) *
                                               Eigen::AngleAxisd(degToRad(yrot), Eigen::Vector3d::UnitY()) *
                                               Eigen::AngleAxisd(degToRad(zrot), Eigen::Vector3d::UnitZ()));
            cmd = new CommandGotoLocation(t, *pos, rotation);
        }
        else
        {
            const std::string* x = paramList->getString("x");
            const std::string* y = paramList->getString("y");
            const std::string* z = paramList->getString("z");
            double xpos = x == nullptr ? 0.0 : static_cast<double>(DecodeFromBase64(*x));
            double ypos = y == nullptr ? 0.0 : static_cast<double>(DecodeFromBase64(*y));
            double zpos = z == nullptr ? 0.0 : static_cast<double>(DecodeFromBase64(*z));
            auto ow = paramList->getNumber<double>("ow").value_or(0.0);
            auto ox = paramList->getNumber<double>("ox").value_or(0.0);
            auto oy = paramList->getNumber<double>("oy").value_or(0.0);
            auto oz = paramList->getNumber<double>("oz").value_or(0.0);
            Eigen::Quaterniond orientation(ow, ox, oy, oz);
            cmd = new CommandGotoLocation(t, Eigen::Vector3d(xpos, ypos, zpos), orientation);
        }
    }
    else if (commandName == "seturl")
    {
        const std::string* url = paramList->getString("url");
        if (url == nullptr)
        {
            error("Missing url parameter to seturl");
            return nullptr;
        }

        cmd = new CommandSetUrl(*url);
    }
    else if (commandName == "center")
    {
        auto t = paramList->getNumber<double>("time").value_or(1.0);
        cmd = new CommandCenter(t);
    }
    else if (commandName == "follow")
    {
        cmd = new CommandFollow();
    }
    else if (commandName == "synchronous")
    {
        cmd = new CommandSynchronous();
    }
    else if (commandName == "lock")
    {
        cmd = new CommandLock();
    }
    else if (commandName == "chase")
    {
        cmd = new CommandChase();
    }
    else if (commandName == "track")
    {
        cmd = new CommandTrack();
    }
    else if (commandName == "cancel")
    {
        cmd = new CommandCancel();
    }
    else if (commandName == "exit")
    {
        cmd = new CommandExit();
    }
    else if (commandName == "print")
    {
        const std::string* text = paramList->getString("text");
        if (text == nullptr)
        {
            error("Missing text parameter to print");
            return nullptr;
        }

        auto duration = paramList->getNumber<double>("duration").value_or(1e9);
        auto voff = paramList->getNumber<int>("row").value_or(0);
        auto hoff = paramList->getNumber<int>("column").value_or(0);
        int horig = -1;
        int vorig = -1;
        if (const std::string* origin = paramList->getString("origin"); origin != nullptr)
        {
            if (compareIgnoringCase(*origin, "left") == 0)
            {
                horig = -1;
                vorig = 0;
            }
            else if (compareIgnoringCase(*origin, "right") == 0)
            {
                horig = 1;
                vorig = 0;
            }
            else if (compareIgnoringCase(*origin, "center") == 0)
            {
                horig = 0;
                vorig = 0;
            }
            else if (compareIgnoringCase(*origin, "left") == 0)
            {
                horig = -1;
                vorig = 0;
            }
            else if (compareIgnoringCase(*origin, "top") == 0)
            {
                horig = 0;
                vorig = 1;
            }
            else if (compareIgnoringCase(*origin, "bottom") == 0)
            {
                horig = 0;
                vorig = -1;
            }
            else if (compareIgnoringCase(*origin, "topright") == 0)
            {
                horig = 1;
                vorig = 1;
            }
            else if (compareIgnoringCase(*origin, "topleft") == 0)
            {
                horig = -1;
                vorig = 1;
            }
            else if (compareIgnoringCase(*origin, "bottomleft") == 0)
            {
                horig = -1;
                vorig = -1;
            }
            else if (compareIgnoringCase(*origin, "bottomright") == 0)
            {
                horig = 1;
                vorig = -1;
            }
        }

        cmd = new CommandPrint(*text,
                               horig, vorig, hoff, -voff,
                               duration);
    }
    else if (commandName == "cls")
    {
        cmd = new CommandClearScreen();
    }
    else if (commandName == "time")
    {
        double jd = 2451545.0;
        if (auto jdVal = paramList->getNumber<double>("jd"); jdVal.has_value())
        {
            jd = *jdVal;
        }
        else
        {
            const std::string* utc = paramList->getString("utc");
            if (utc == nullptr)
            {
                error("Missing either time or utc parameter to time");
                return nullptr;
            }

            astro::Date date = astro::Date(0.0);
            sscanf(utc->c_str(), "%d-%d-%dT%d:%d:%lf",
                &date.year, &date.month, &date.day,
                &date.hour, &date.minute, &date.seconds);
            jd = (double)date;
        }
        cmd = new CommandSetTime(jd);
    }
    else if (commandName == "timerate")
    {
        auto rate = paramList->getNumber<double>("rate").value_or(1.0);
        cmd = new CommandSetTimeRate(rate);
    }
    else if (commandName == "changedistance")
    {
        auto rate = paramList->getNumber<double>("rate").value_or(0.0);
        auto duration = paramList->getNumber<double>("duration").value_or(1.0);
        cmd = new CommandChangeDistance(duration, rate);
    }
    else if (commandName == "orbit")
    {
        auto duration = paramList->getNumber<double>("duration").value_or(1.0);
        auto rate = paramList->getNumber<double>("rate").value_or(0.0);
        auto axis = paramList->getVector3<double>("axis").value_or(Eigen::Vector3d::Zero());
        cmd = new CommandOrbit(duration,
                               axis.cast<float>(),
                               (float) degToRad(rate));
    }
    else if (commandName == "rotate")
    {
        auto duration = paramList->getNumber<double>("duration").value_or(1.0);
        auto rate = paramList->getNumber<double>("rate").value_or(0.0);
        auto axis = paramList->getVector3<double>("axis").value_or(Eigen::Vector3d::Zero());
        cmd = new CommandRotate(duration,
                                axis.cast<float>(),
                                (float) degToRad(rate));
    }
    else if (commandName == "move")
    {
        auto duration = paramList->getNumber<double>("duration").value_or(1.0);
        auto velocity = paramList->getVector3<double>("velocity").value_or(Eigen::Vector3d::Zero());
        cmd = new CommandMove(duration, velocity * astro::kilometersToMicroLightYears(1.0));
    }
    else if (commandName == "setposition")
    {
        // Base position in light years, offset in kilometers
        if (auto base = paramList->getVector3<double>("base"); base.has_value())
        {
            auto offset = paramList->getVector3<double>("offset").value_or(Eigen::Vector3d::Zero());
            Eigen::Vector3f basef = base->cast<float>();
            UniversalCoord basePosition = UniversalCoord::CreateLy(basef.cast<double>());
            cmd = new CommandSetPosition(basePosition.offsetKm(offset));
        }
        else
        {
            const std::string* x = paramList->getString("x");
            const std::string* y = paramList->getString("y");
            const std::string* z = paramList->getString("z");
            if (x == nullptr || y == nullptr || z == nullptr)
            {
                error("Missing x, y, z or base, offset parameters to setposition");
                return nullptr;
            }
            cmd = new CommandSetPosition(UniversalCoord(DecodeFromBase64(*x),
                                                        DecodeFromBase64(*y),
                                                        DecodeFromBase64(*z)));
        }
    }
    else if (commandName == "setorientation")
    {
        if (auto angle = paramList->getNumber<double>("angle"); angle.has_value())
        {
            auto axis = paramList->getVector3<double>("axis").value_or(Eigen::Vector3d::Zero());
            auto orientation = Eigen::Quaternionf(Eigen::AngleAxisf((float) degToRad(*angle),
                                                                    axis.cast<float>().normalized()));
            cmd = new CommandSetOrientation(orientation);
        }
        else
        {
            auto ow = paramList->getNumber<double>("ow").value_or(0.0);
            auto ox = paramList->getNumber<double>("ox").value_or(0.0);
            auto oy = paramList->getNumber<double>("oy").value_or(0.0);
            auto oz = paramList->getNumber<double>("oz").value_or(0.0);
            Eigen::Quaternionf orientation(ow, ox, oy, oz);
            cmd = new CommandSetOrientation(orientation);
        }
    }
    else if (commandName == "lookback")
    {
        cmd = new CommandLookBack();
    }
    else if (commandName == "renderflags")
    {
        uint64_t setFlags = 0;
        uint64_t clearFlags = 0;

        if (const std::string* s = paramList->getString("set"); s != nullptr)
            setFlags = parseRenderFlags(*s, scriptMaps->RenderFlagMap);
        if (const std::string* s = paramList->getString("clear"); s != nullptr)
            clearFlags = parseRenderFlags(*s, scriptMaps->RenderFlagMap);

        cmd = new CommandRenderFlags(setFlags, clearFlags);
    }
    else if (commandName == "labels")
    {
        int setFlags = 0;
        int clearFlags = 0;

        if (const std::string* s = paramList->getString("set"); s != nullptr)
            setFlags = parseLabelFlags(*s, scriptMaps->LabelFlagMap);
        if (const std::string* s = paramList->getString("clear"); s != nullptr)
            clearFlags = parseLabelFlags(*s, scriptMaps->LabelFlagMap);

        cmd = new CommandLabels(setFlags, clearFlags);
    }
    else if (commandName == "orbitflags")
    {
        int setFlags = 0;
        int clearFlags = 0;

        if (const std::string* s = paramList->getString("set"); s != nullptr)
            setFlags = parseOrbitFlags(*s, scriptMaps->OrbitVisibilityMap);
        if (const std::string* s = paramList->getString("clear"); s != nullptr)
            clearFlags = parseOrbitFlags(*s, scriptMaps->OrbitVisibilityMap);

        cmd = new CommandOrbitFlags(setFlags, clearFlags);
    }
    else if (commandName == "constellations")
    {
        CommandConstellations *cmdcons= new CommandConstellations();

        if (const std::string* s = paramList->getString("set"); s != nullptr)
            parseConstellations(cmdcons, *s, 1);
        if (const std::string* s = paramList->getString("clear"); s != nullptr)
            parseConstellations(cmdcons, *s, 0);
        cmd = cmdcons;
    }

    else if (commandName == "constellationcolor")
    {
        CommandConstellationColor *cmdconcol= new CommandConstellationColor();

        auto colorv = paramList->getVector3<double>("color").value_or(Eigen::Vector3d::UnitX());

        if (const std::string* s = paramList->getString("set"); s != nullptr)
            parseConstellationColor(cmdconcol, *s, &colorv, 1);
        if (const std::string* s = paramList->getString("clear"); s != nullptr)
            parseConstellationColor(cmdconcol, *s, &colorv, 0);
        cmd = cmdconcol;
    }
    else if (commandName == "setvisibilitylimit")
    {
        auto mag = paramList->getNumber<double>("magnitude").value_or(6.0);
        cmd = new CommandSetVisibilityLimit(mag);
    }
    else if (commandName == "setfaintestautomag45deg")
    {
        auto mag = paramList->getNumber<double>("magnitude").value_or(8.5);
        cmd = new CommandSetFaintestAutoMag45deg(mag);
    }
    else if (commandName == "setambientlight")
    {
        auto brightness = paramList->getNumber<float>("brightness").value_or(0.0f);
        cmd = new CommandSetAmbientLight(brightness);
    }
    else if (commandName == "setgalaxylightgain")
    {
        auto gain = paramList->getNumber<float>("gain").value_or(0.0f);
        cmd = new CommandSetGalaxyLightGain(gain);
    }
    else if (commandName == "settextureresolution")
    {
        unsigned int res = 1;
        if (const std::string* textureRes = paramList->getString("resolution"); textureRes != nullptr)
        {
            if (compareIgnoringCase(*textureRes, "low") == 0)
                res = 0;
            else if (compareIgnoringCase(*textureRes, "medium") == 0)
                res = 1;
            else if (compareIgnoringCase(*textureRes, "high") == 0)
                res = 2;
        }
        cmd = new CommandSetTextureResolution(res);
    }
    else if (commandName == "preloadtex")
    {
        const std::string* object = paramList->getString("object");
        if (object == nullptr)
        {
            error("Missing object parameter to preloadtex");
            return nullptr;
        }

        cmd = new CommandPreloadTextures(*object);
    }
    else if (commandName == "mark")
    {
        const std::string* object = paramList->getString("object");
        if (object == nullptr)
        {
            error("Missing object parameter to mark");
            return nullptr;
        }

        auto size = paramList->getNumber<double>("size").value_or(10.0);
        auto colorv = paramList->getVector3<float>("color").value_or(Eigen::Vector3f::UnitX());
        auto alpha = paramList->getNumber<float>("alpha").value_or(0.9f);
        Color color(colorv.x(), colorv.y(), colorv.z(), alpha);

        using namespace celestia;

        MarkerRepresentation rep(MarkerRepresentation::Diamond);
        if (const std::string* symbolString = paramList->getString("symbol"); symbolString != nullptr)
        {
            if (compareIgnoringCase(*symbolString, "diamond") == 0)
                rep = MarkerRepresentation(MarkerRepresentation::Diamond);
            else if (compareIgnoringCase(*symbolString, "triangle") == 0)
                rep = MarkerRepresentation(MarkerRepresentation::Triangle);
            else if (compareIgnoringCase(*symbolString, "square") == 0)
                rep = MarkerRepresentation(MarkerRepresentation::Square);
            else if (compareIgnoringCase(*symbolString, "filledsquare") == 0)
                rep = MarkerRepresentation(MarkerRepresentation::FilledSquare);
            else if (compareIgnoringCase(*symbolString, "plus") == 0)
                rep = MarkerRepresentation(MarkerRepresentation::Plus);
            else if (compareIgnoringCase(*symbolString, "x") == 0)
                rep = MarkerRepresentation(MarkerRepresentation::X);
            else if (compareIgnoringCase(*symbolString, "leftarrow") == 0)
                rep = MarkerRepresentation(MarkerRepresentation::LeftArrow);
            else if (compareIgnoringCase(*symbolString, "rightarrow") == 0)
                rep = MarkerRepresentation(MarkerRepresentation::RightArrow);
            else if (compareIgnoringCase(*symbolString, "uparrow") == 0)
                rep = MarkerRepresentation(MarkerRepresentation::UpArrow);
            else if (compareIgnoringCase(*symbolString, "downarrow") == 0)
                rep = MarkerRepresentation(MarkerRepresentation::DownArrow);
            else if (compareIgnoringCase(*symbolString, "circle") == 0)
                rep = MarkerRepresentation(MarkerRepresentation::Circle);
            else if (compareIgnoringCase(*symbolString, "disk") == 0)
                rep = MarkerRepresentation(MarkerRepresentation::Disk);
        }

        rep.setSize((float) size);
        rep.setColor(color);
        if (const std::string* label = paramList->getString("label"); label != nullptr)
            rep.setLabel(*label);

        bool occludable = paramList->getBoolean("occludable").value_or(true);

        cmd = new CommandMark(*object, rep, occludable);
    }
    else if (commandName == "unmark")
    {
        const std::string* object = paramList->getString("object");
        if (object == nullptr)
        {
            error("Missing object parameter to unmark");
            return nullptr;
        }

        cmd = new CommandUnmark(*object);
    }
    else if (commandName == "unmarkall")
    {
        cmd = new CommandUnmarkAll();
    }
    else if (commandName == "capture")
    {
        const std::string* filename = paramList->getString("filename");
        if (filename == nullptr)
        {
            error("Missing filename parameter to capture");
            return nullptr;
        }

        std::string type;
        if (const std::string* typeStr = paramList->getString("type"); typeStr != nullptr)
        {
            type = *typeStr;
        }

        cmd = new CommandCapture(type, *filename);
    }
    else if (commandName == "renderpath")
    {
        // ignore: renderpath not supported
    }
    else if (commandName == "splitview")
    {
        auto view = paramList->getNumber<unsigned int>("view").value_or(1u);
        string splitType;
        if (const std::string* splitVal = paramList->getString("type"); splitVal != nullptr)
        {
            splitType = *splitVal;
        }

        auto splitPos = paramList->getNumber<float>("position").value_or(0.5f);
        cmd = new CommandSplitView(view, splitType, splitPos);
    }
    else if (commandName == "deleteview")
    {
        auto view = paramList->getNumber<unsigned int>("view").value_or(1u);
        cmd = new CommandDeleteView(view);
    }
    else if (commandName == "singleview")
    {
        cmd = new CommandSingleView();
    }
    else if (commandName == "setactiveview")
    {
        auto view = paramList->getNumber<unsigned int>("view").value_or(1u);
        cmd = new CommandSetActiveView(view);
    }
    else if (commandName == "setradius")
    {
        const std::string* object = paramList->getString("object");
        if (object == nullptr)
        {
            error("Missing object parameter to setradius");
            return nullptr;
        }
        auto radius = paramList->getNumber<float>("radius").value_or(1.0f);
        cmd = new CommandSetRadius(*object, radius);
    }
    else if (commandName == "setlinecolor")
    {
        const std::string* item = paramList->getString("item");
        if (item == nullptr)
        {
            error("Missing item parameter to setlinecolor");
            return nullptr;
        }

        auto colorv = paramList->getVector3<float>("color").value_or(Eigen::Vector3f::UnitX());
        Color color(colorv.x(), colorv.y(), colorv.z());
        cmd = new CommandSetLineColor(*item, color);
    }
    else if (commandName == "setlabelcolor")
    {
        const std::string* item = paramList->getString("item");
        if (item == nullptr)
        {
            error("Missing item parameter to setlabelcolor");
            return nullptr;
        }

        auto colorv = paramList->getVector3<float>("color").value_or(Eigen::Vector3f::UnitX());
        Color color(colorv.x(), colorv.y(), colorv.z());
        cmd = new CommandSetLabelColor(*item, color);
    }
    else if (commandName == "settextcolor")
    {
        auto colorv = paramList->getVector3<float>("color").value_or(Eigen::Vector3f::Ones());
        Color color(colorv.x(), colorv.y(), colorv.z());
        cmd = new CommandSetTextColor(color);
    }
    else if (commandName == "play")
    {
#ifdef USE_MINIAUDIO
        int channel = std::max(paramList->getNumber<int>("channel").value_or(defaultAudioChannel),
                               minAudioChannel);

        auto optionalVolume = paramList->getNumber<float>("volume");
        if (optionalVolume.has_value())
            optionalVolume = clamp(*optionalVolume, minAudioVolume, maxAudioVolume);
        float pan = clamp(paramList->getNumber<float>("pan").value_or(defaultAudioPan),
                          minAudioPan, maxAudioPan);
        int nopause = paramList->getNumber<int>("nopause").value_or(0);
        std::optional<bool> optionalLoop = std::nullopt;
        if (auto loop = paramList->getNumber<int>("loop"); loop.has_value())
            optionalLoop = *loop == 1;
        std::optional<string> optionalFilename = std::nullopt;
        if (auto filename = paramList->getString("filename"); filename != nullptr)
            optionalFilename = *filename;

        cmd = new CommandPlay(channel, optionalVolume, pan, optionalLoop, optionalFilename, nopause == 1);
#else
        cmd = new CommandWait(0);
#endif
    }
    else if (commandName == "overlay")
    {
        auto duration = paramList->getNumber<float>("duration").value_or(3.0f);
        auto xoffset = paramList->getNumber<float>("xoffset").value_or(0.0f);
        auto yoffset = paramList->getNumber<float>("yoffset").value_or(0.0f);
        std::optional<float> alpha = paramList->getNumber<float>("alpha");

        const std::string* filename = paramList->getString("filename");
        if (filename == nullptr)
        {
            error("Missing filename parameter to overlay");
            return nullptr;
        }

        bool fitscreen = false;
        if (auto fitscreenValue = paramList->getBoolean("fitscreen"); fitscreenValue.has_value())
        {
            fitscreen = *fitscreenValue;
        }
        else
        {
            fitscreen = paramList->getNumber<int>("fitscreen").value_or(0) != 0;
        }

        array<Color, 4> colors;
        Color color = paramList->getColor("color").value_or(Color::White);
        colors.fill(alpha.has_value() ? Color(color, *alpha) : color);

        if (auto colorTop = paramList->getColor("colortop"); colorTop.has_value())
            colors[0] = colors[1] = alpha.has_value() ? Color(*colorTop, *alpha) : *colorTop;
        if (auto colorBottom = paramList->getColor("colorbottom"); colorBottom.has_value())
            colors[2] = colors[3] = alpha.has_value() ? Color(*colorBottom, *alpha) : *colorBottom;
        if (auto colorTopLeft = paramList->getColor("colortopleft"); colorTopLeft.has_value())
            colors[0] = alpha.has_value() ? Color(*colorTopLeft, *alpha) : *colorTopLeft;
        if (auto colorTopRight = paramList->getColor("colortopright"); colorTopRight.has_value())
            colors[1] = alpha.has_value() ? Color(*colorTopRight, *alpha) : *colorTopRight;
        if (auto colorBottomRight = paramList->getColor("colorbottomright"); colorBottomRight.has_value())
            colors[2] = alpha.has_value() ? Color(*colorBottomRight, *alpha) : *colorBottomRight;
        if (auto colorBottomLeft = paramList->getColor("colorbottomleft"); colorBottomLeft.has_value())
            colors[3] = alpha.has_value() ? Color(*colorBottomLeft, *alpha) : *colorBottomLeft;

        auto fadeafter = paramList->getNumber<float>("fadeafter").value_or(duration);

        cmd = new CommandScriptImage(duration, fadeafter, xoffset, yoffset, *filename, fitscreen, colors);
    }
    else if (commandName == "verbosity")
    {
        int level = paramList->getNumber<int>("level").value_or(2);
        cmd = new CommandVerbosity(level);
    }
    else if (commandName == "setwindowbordersvisible")
    {
        bool visible = paramList->getBoolean("visible").value_or(true);
        cmd = new CommandSetWindowBordersVisible(visible);
    }
    else if (commandName == "setringstexture")
    {
        const std::string* object = paramList->getString("object");
        if (object == nullptr)
        {
            error("Missing object parameter to setringstexture");
            return nullptr;
        }

        const std::string* texture = paramList->getString("texture");
        if (texture == nullptr)
        {
            error("Missing texture parameter to setringstexture");
            return nullptr;
        }

        std::string path;
        if (const std::string* pathStr = paramList->getString("path"); pathStr != nullptr)
        {
            path = *pathStr;
        }

        cmd = new CommandSetRingsTexture(*object, *texture, path);
    }
    else if (commandName == "loadfragment")
    {
        const std::string* type = paramList->getString("type");
        if (type == nullptr)
        {
            error("Missing type parameter to loadfragment");
            return nullptr;
        }

        const std::string* fragment = paramList->getString("fragment");
        if (fragment == nullptr)
        {
            error("Missing fragment parameter to loadfragment");
            return nullptr;
        }

        std::string dir;
        if (const std::string* dirName = paramList->getString("dir"); dirName != nullptr)
        {
            dir = *dirName;
        }

        cmd = new CommandLoadFragment(*type, *fragment, dir);
    }
    else
    {
        error("Unknown command name '" + commandName + "'");
        cmd = nullptr;
    }

    return cmd;
}


uint64_t parseRenderFlags(const string &s, const FlagMap64& RenderFlagMap)
{
    istringstream in(s);

    Tokenizer tokenizer(&in);
    uint64_t flags = 0;


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


int parseLabelFlags(const string &s, const FlagMap &LabelFlagMap)
{
    istringstream in(s);

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


int parseOrbitFlags(const string &s, const FlagMap &BodyTypeMap)
{
    istringstream in(s);

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


int parseConstellations(CommandConstellations* cmd, const string &s, int act)
{
    istringstream in(s);

    Tokenizer tokenizer(&in);
    int flags = 0;

    Tokenizer::TokenType ttype = tokenizer.nextToken();
    while (ttype != Tokenizer::TokenEnd)
    {
        if (auto tokenValue = tokenizer.getNameValue(); tokenValue.has_value())
        {
            if (compareIgnoringCase(*tokenValue, "all") == 0 && act==1)
                cmd->flags.all = true;
            else if (compareIgnoringCase(*tokenValue, "all") == 0 && act==0)
                cmd->flags.none = true;
            else
                cmd->setValues(*tokenValue, act);

            ttype = tokenizer.nextToken();
            if (ttype == Tokenizer::TokenBar)
                ttype = tokenizer.nextToken();
        }
        else
        {
            GetLogger()->error("Command Parser: error parsing render flags\n");
            return 0;
        }
    }

    return flags;
}


int parseConstellationColor(CommandConstellationColor* cmd, const string &s, Eigen::Vector3d *col, int act)
{
    istringstream in(s);

    Tokenizer tokenizer(&in);
    int flags = 0;

    if(!act)
        cmd->unsetColor();
    else
        cmd->setColor((float)col->x(), (float)col->y(), (float)col->z());

    Tokenizer::TokenType ttype = tokenizer.nextToken();
    while (ttype != Tokenizer::TokenEnd)
    {
        if (auto tokenValue = tokenizer.getNameValue(); tokenValue.has_value())
        {
            if (compareIgnoringCase(*tokenValue, "all") == 0 && act==1)
                cmd->flags.all = true;
            else if (compareIgnoringCase(*tokenValue, "all") == 0 && act==0)
                cmd->flags.none = true;
            else
                cmd->setConstellations(*tokenValue);

            ttype = tokenizer.nextToken();
            if (ttype == Tokenizer::TokenBar)
                ttype = tokenizer.nextToken();
        }
        else
        {
            GetLogger()->error("Command Parser: error parsing render flags\n");
            return 0;
        }
    }

    return flags;
}
