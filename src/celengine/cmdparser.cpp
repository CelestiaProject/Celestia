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
#include <cstdio>

#include "celestia.h"
// Ugh . . . the C++ standard says that stringstream should be in
// sstream, but the GNU C++ compiler uses strstream instead.
#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif // HAVE_SSTREAM

#include <celutil/util.h>
#include <celutil/debug.h>
#include <celmath/mathlib.h>
#include <celengine/astro.h>
#include "astro.h"
#include "cmdparser.h"
#include "glcontext.h"

using namespace std;


static int parseRenderFlags(string);
static int parseLabelFlags(string);
static int parseOrbitFlags(string);

CommandParser::CommandParser(istream& in)
{
    tokenizer = new Tokenizer(&in);
    parser = new Parser(tokenizer);
}

CommandParser::CommandParser(Tokenizer& tok)
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
        return NULL;
    }

    Tokenizer::TokenType ttype = tokenizer->nextToken();
    while (ttype != Tokenizer::TokenEnd && ttype != Tokenizer::TokenEndGroup)
    {
        tokenizer->pushBack();
        Command* cmd = parseCommand();
        if (cmd == NULL)
        {
            for (CommandSequence::const_iterator iter = seq->begin();
                 iter != seq->end();
                 iter++)
            {
                delete *iter;
            }
            delete seq;
            return NULL;
        }
        else
        {
            seq->insert(seq->end(), cmd);
        }

        ttype = tokenizer->nextToken();
    }

    if (ttype != Tokenizer::TokenEndGroup)
    {
        error("Missing '}' at end of script.");
        for_each(seq->begin(), seq->end(), deleteFunc<Command*>());;
        delete seq;
        return NULL;
    }

    return seq;
}


const vector<string>* CommandParser::getErrors() const
{
    return &errorList;
}


void CommandParser::error(const string errMsg)
{
    errorList.insert(errorList.end(), errMsg);
}


static astro::CoordinateSystem parseCoordinateSystem(const string& name)
{
    if (compareIgnoringCase(name, "observer") == 0)
        return astro::ObserverLocal;
    else if (compareIgnoringCase(name, "geographic") == 0)
        return astro::Geographic;
    else if (compareIgnoringCase(name, "equatorial") == 0)
        return astro::Equatorial;
    else if (compareIgnoringCase(name, "ecliptical") == 0)
        return astro::Ecliptical;
    else if (compareIgnoringCase(name, "universal") == 0)
        return astro::Universal;
    else if (compareIgnoringCase(name, "lock") == 0)
        return astro::PhaseLock;
    else if (compareIgnoringCase(name, "chase") == 0)
        return astro::Chase;
    else
        return astro::ObserverLocal;
}


Command* CommandParser::parseCommand()
{
    if (tokenizer->nextToken() != Tokenizer::TokenName)
    {
        error("Invalid command name");
        return NULL;
    }

    string commandName = tokenizer->getStringValue();
    
    Value* paramListValue = parser->readValue();
    if (paramListValue == NULL || paramListValue->getType() != Value::HashType)
    {
        error("Bad parameter list");
        return NULL;
    }

    Hash* paramList = paramListValue->getHash();
    Command* cmd = NULL;

    if (commandName == "wait")
    {
        double duration = 1.0;
        paramList->getNumber("duration", duration);
        cmd = new CommandWait(duration);
    }
    else if (commandName == "set")
    {
        double value = 0.0;
        string name;
        paramList->getString("name", name);
        if (!paramList->getNumber("value", value))
        {
            // Some values may be specified via strings
            string valstr;
            if (paramList->getString("value", valstr))
            {
                if (compareIgnoringCase(valstr, "fuzzypoints") == 0)
                    value = (double) Renderer::FuzzyPointStars;
                else if (compareIgnoringCase(valstr, "points") == 0)
                    value = (double) Renderer::PointStars;
                else if (compareIgnoringCase(valstr, "scaleddiscs") == 0)
                    value = (double) Renderer::ScaledDiscStars;
            }
        }

        cmd = new CommandSet(name, value);
    }
    else if (commandName == "select")
    {
        string object;
        paramList->getString("object", object);
        cmd = new CommandSelect(object);
    }
    else if (commandName == "setframe")
    {
        string refName;
        paramList->getString("ref", refName);
        string targetName;
        paramList->getString("target", targetName);
        string coordSysName;
        astro::CoordinateSystem coordSys = astro::Universal;
        if (paramList->getString("coordsys", coordSysName))
            coordSys = parseCoordinateSystem(coordSysName);

        cmd = new CommandSetFrame(coordSys, refName, targetName);
    }
    else if (commandName == "setsurface")
    {
        string name;
        paramList->getString("name", name);
        cmd = new CommandSetSurface(name);
    }
    else if (commandName == "goto")
    {
        double t = 1.0;
        paramList->getNumber("time", t);
        double distance = 5.0;
        paramList->getNumber("distance", distance);

        astro::CoordinateSystem upFrame = astro::ObserverLocal;
        string frameString;
        if (paramList->getString("upframe", frameString))
            upFrame = parseCoordinateSystem(frameString);

        Vec3d up(0, 1, 0);
        paramList->getVector("up", up);

        cmd = new CommandGoto(t,
                              distance,
                              Vec3f((float) up.x, (float) up.y, (float) up.z),
                              upFrame);
    }
    else if (commandName == "gotolonglat")
    {
        double t = 1.0;
        paramList->getNumber("time", t);
        double distance = 5.0;
        paramList->getNumber("distance", distance);
        Vec3d up(0, 1, 0);
        paramList->getVector("up", up);
        double longitude;
        paramList->getNumber("longitude", longitude);
        double latitude;
        paramList->getNumber("latitude", latitude);

        cmd = new CommandGotoLongLat(t,
                                     distance,
                                     (float) degToRad(longitude),
                                     (float) degToRad(latitude),
                                     Vec3f((float) up.x, (float) up.y, (float) up.z));
    }
    else if (commandName == "gotoloc")
    {
        double t = 1.0;
        paramList->getNumber("time", t);
        Vec3d pos(0, 1, 0);
        if (paramList->getVector("position", pos))
        {
            pos = pos * astro::kilometersToMicroLightYears(1.0);
            double xrot = 0.0;
            paramList->getNumber("xrot", xrot);
            double yrot = 0.0;
            paramList->getNumber("yrot", yrot);
            double zrot = 0.0;
            paramList->getNumber("zrot", zrot);
            zrot = degToRad(zrot);
            Quatf rotation = Quatf::xrotation((float) degToRad(xrot)) *
                Quatf::yrotation((float) degToRad(yrot)) *
                Quatf::zrotation((float) degToRad(zrot));
            cmd = new CommandGotoLocation(t, Point3d(0.0, 0.0, 0.0) + pos,
                                          rotation);
        }
        else
        {
            std::string x, y, z;
            paramList->getString("x", x);
            paramList->getString("y", y);
            paramList->getString("z", z);
            double ow, ox, oy, oz;
            paramList->getNumber("ow", ow);
            paramList->getNumber("ox", ox);
            paramList->getNumber("oy", oy);
            paramList->getNumber("oz", oz);
            Quatf orientation((float)ow, (float)ox, (float)oy, (float)oz);
            cmd = new CommandGotoLocation(t, Point3d((double)BigFix(x), (double)BigFix(y), (double)BigFix(z)),
                                          orientation);
        }
    }
    else if (commandName == "seturl")
    {
        std::string url;
        paramList->getString("url", url);
        cmd = new CommandSetUrl(url);
    }
    else if (commandName == "center")
    {
        double t = 1.0;
        paramList->getNumber("time", t);
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
        string text;
        string origin;
        int horig = -1;
        int vorig = -1;
        double hoff = 0;
        double voff = 0;
        double duration = 1.0e9;
        paramList->getString("text", text);
        paramList->getString("origin", origin);
        paramList->getNumber("duration", duration);
        paramList->getNumber("row", voff);
        paramList->getNumber("column", hoff);
        if (compareIgnoringCase(origin, "left") == 0)
        {
            horig = -1;
            vorig = 0;
        }
        else if (compareIgnoringCase(origin, "right") == 0)
        {
            horig = 1;
            vorig = 0;
        }
        else if (compareIgnoringCase(origin, "center") == 0)
        {
            horig = 0;
            vorig = 0;
        }
        else if (compareIgnoringCase(origin, "left") == 0)
        {
            horig = -1;
            vorig = 0;
        }
        else if (compareIgnoringCase(origin, "top") == 0)
        {
            horig = 0;
            vorig = 1;
        }
        else if (compareIgnoringCase(origin, "bottom") == 0)
        {
            horig = 0;
            vorig = -1;
        }
        else if (compareIgnoringCase(origin, "topright") == 0)
        {
            horig = 1;
            vorig = 1;
        }
        else if (compareIgnoringCase(origin, "topleft") == 0)
        {
            horig = -1;
            vorig = 1;
        }
        else if (compareIgnoringCase(origin, "bottomleft") == 0)
        {
            horig = -1;
            vorig = -1;
        }
        else if (compareIgnoringCase(origin, "bottomright") == 0)
        {
            horig = 1;
            vorig = -1;
        }

        cmd = new CommandPrint(text,
                               horig, vorig, (int) hoff, (int) -voff,
                               duration);
    }
    else if (commandName == "cls")
    {
        cmd = new CommandClearScreen();
    }
    else if (commandName == "time")
    {
        double jd = 2451545;
        if (!paramList->getNumber("jd", jd))
        {
            std::string utc;
            paramList->getString("utc", utc);
            astro::Date date = astro::Date(0.0);
            sscanf(utc.c_str(), "%d-%d-%dT%d:%d:%lf",
                &date.year, &date.month, &date.day,
                &date.hour, &date.minute, &date.seconds);
            jd = (double)date;
        }
        cmd = new CommandSetTime(jd);
    }
    else if (commandName == "timerate")
    {
        double rate = 1.0;
        paramList->getNumber("rate", rate);
        cmd = new CommandSetTimeRate(rate);
    }
    else if (commandName == "changedistance")
    {
        double rate = 0.0;
        double duration = 1.0;
        paramList->getNumber("rate", rate);
        paramList->getNumber("duration", duration);
        cmd = new CommandChangeDistance(duration, rate);
    }
    else if (commandName == "orbit")
    {
        double rate = 0.0;
        double duration = 1.0;
        Vec3d axis;
        paramList->getNumber("duration", duration);
        paramList->getNumber("rate", rate);
        paramList->getVector("axis", axis);
        cmd = new CommandOrbit(duration,
                               Vec3f((float) axis.x, (float) axis.y, (float) axis.z),
                               (float) degToRad(rate));
    }
    else if (commandName == "rotate")
    {
        double rate = 0.0;
        double duration = 1.0;
        Vec3d axis;
        paramList->getNumber("duration", duration);
        paramList->getNumber("rate", rate);
        paramList->getVector("axis", axis);
        cmd = new CommandRotate(duration,
                                Vec3f((float) axis.x, (float) axis.y, (float) axis.z),
                                (float) degToRad(rate));
    }
    else if (commandName == "move")
    {
        Vec3d velocity;
        double duration;
        paramList->getNumber("duration", duration);
        paramList->getVector("velocity", velocity);
        cmd = new CommandMove(duration, velocity * astro::kilometersToMicroLightYears(1.0));
    }
    else if (commandName == "setposition")
    {
        Vec3d base, offset;
        if (paramList->getVector("base", base))
        {
            paramList->getVector("offset", offset);
            cmd = new CommandSetPosition(astro::universalPosition(Point3d(offset.x, offset.y, offset.z),
                                                                  Point3f((float) base.x, (float) base.y, (float) base.z)));
        }
        else
        {
            std::string x, y, z;
            paramList->getString("x", x);
            paramList->getString("y", y);
            paramList->getString("z", z);
            cmd = new CommandSetPosition(UniversalCoord(BigFix(x), BigFix(y), BigFix(z)));
        }
    }
    else if (commandName == "setorientation")
    {
        Vec3d axis;
        double angle;
        if (paramList->getNumber("angle", angle))
        {
            paramList->getVector("axis", axis);
            cmd = new CommandSetOrientation(Vec3f((float) axis.x, (float) axis.y, (float) axis.z),
                                            (float) degToRad(angle));
        }
        else
        {
            double ow, ox, oy, oz;
            paramList->getNumber("ow", ow);
            paramList->getNumber("ox", ox);
            paramList->getNumber("oy", oy);
            paramList->getNumber("oz", oz);
            Quatf orientation((float)ow, (float)ox, (float)oy, (float)oz);
            Vec3f axis;
            float angle;
            orientation.getAxisAngle(axis, angle);
            cmd = new CommandSetOrientation(axis, angle);
        }
    }
    else if (commandName == "lookback")
    {
        cmd = new CommandLookBack();
    }
    else if (commandName == "renderflags")
    {
        int setFlags = 0;
        int clearFlags = 0;
        string s;

        if (paramList->getString("set", s))
            setFlags = parseRenderFlags(s);
        if (paramList->getString("clear", s))
            clearFlags = parseRenderFlags(s);

        cmd = new CommandRenderFlags(setFlags, clearFlags);
    }
    else if (commandName == "labels")
    {
        int setFlags = 0;
        int clearFlags = 0;
        string s;

        if (paramList->getString("set", s))
            setFlags = parseLabelFlags(s);
        if (paramList->getString("clear", s))
            clearFlags = parseLabelFlags(s);

        cmd = new CommandLabels(setFlags, clearFlags);
    }
    else if (commandName == "orbitflags")
    {
        int setFlags = 0;
        int clearFlags = 0;
        string s;

        if (paramList->getString("set", s))
            setFlags = parseOrbitFlags(s);
        if (paramList->getString("clear", s))
            clearFlags = parseOrbitFlags(s);

        cmd = new CommandOrbitFlags(setFlags, clearFlags);
    }
    else if (commandName == "setvisibilitylimit")
    {
        double mag = 6.0;
        paramList->getNumber("magnitude", mag);
        cmd = new CommandSetVisibilityLimit(mag);
    }
    else if (commandName == "setfaintestautomag45deg")
    {
        double mag = 8.5;
        paramList->getNumber("magnitude", mag);
        cmd = new CommandSetFaintestAutoMag45deg(mag);
    }
    else if (commandName == "setambientlight")
    {
        double brightness = 0.0;
        paramList->getNumber("brightness", brightness);
        cmd = new CommandSetAmbientLight((float) brightness);
    }
    else if (commandName == "preloadtex")
    {
        string object;
        paramList->getString("object", object);
        cmd = new CommandPreloadTextures(object);
    }
    else if (commandName == "mark")
    {
        string object;
        paramList->getString("object", object);
        double size = 10.0f;
        paramList->getNumber("size", size);
        Vec3d colorv(1.0f, 0.0f, 0.0f);
        paramList->getVector("color", colorv);
        Color color((float) colorv.x, (float) colorv.y, (float) colorv.z, 0.9f);

        Marker::Symbol symbol = Marker::Diamond;
        string symbolString;
        if (paramList->getString("symbol", symbolString))
        {
            if (compareIgnoringCase(symbolString, "diamond") == 0)
                symbol = Marker::Diamond;
            else if (compareIgnoringCase(symbolString, "square") == 0)
                symbol = Marker::Square;
            else if (compareIgnoringCase(symbolString, "triangle") == 0)
                symbol = Marker::Triangle;
            else if (compareIgnoringCase(symbolString, "plus") == 0)
                symbol = Marker::Plus;
            else if (compareIgnoringCase(symbolString, "x") == 0)
                symbol = Marker::X;
        }
        
        cmd = new CommandMark(object, color, (float) size, symbol);
    }
    else if (commandName == "unmark")
    {
        string object;
        paramList->getString("object", object);
        cmd = new CommandUnmark(object);
    }
    else if (commandName == "unmarkall")
    {
        cmd = new CommandUnmarkAll();
    }
    else if (commandName == "capture")
    {
        string type, filename;
        paramList->getString("type", type);
        paramList->getString("filename", filename);
		
        cmd = new CommandCapture(type, filename);
    }
    else if (commandName == "renderpath")
    {
        GLContext::GLRenderPath glcpath = GLContext::GLPath_Basic;
        string path;
        paramList->getString("path", path);

        if (compareIgnoringCase(path, "basic") == 0)
            glcpath = GLContext::GLPath_Basic;
        else if (compareIgnoringCase(path, "multitexture") == 0)
            glcpath = GLContext::GLPath_Multitexture;
        else if (compareIgnoringCase(path, "vp") == 0)
            glcpath = GLContext::GLPath_DOT3_ARBVP;
        else if (compareIgnoringCase(path, "vp-nv") == 0)
            glcpath = GLContext::GLPath_NvCombiner_ARBVP;
        else if (compareIgnoringCase(path, "glsl") == 0)
            glcpath = GLContext::GLPath_GLSL;
	
        cmd = new CommandRenderPath(glcpath);
    }
    else
    {
        error("Unknown command name '" + commandName + "'");
        cmd = NULL;
    }

    delete paramListValue;

    return cmd;
}


int parseRenderFlags(string s)
{
#ifdef HAVE_SSTREAM    
    istringstream in(s);
#else
    istrstream in(s.c_str());
#endif
    Tokenizer tokenizer(&in);
    int flags = 0;

    Tokenizer::TokenType ttype = tokenizer.nextToken();
    while (ttype != Tokenizer::TokenEnd)
    {
        if (ttype == Tokenizer::TokenName)
        {
            string name = tokenizer.getNameValue();
            if (compareIgnoringCase(name, "orbits") == 0)
                flags |= Renderer::ShowOrbits;
            else if (compareIgnoringCase(name, "cloudmaps") == 0)
                flags |= Renderer::ShowCloudMaps;
            else if (compareIgnoringCase(name, "constellations") == 0)
                flags |= Renderer::ShowDiagrams;
            else if (compareIgnoringCase(name, "galaxies") == 0)
                flags |= Renderer::ShowGalaxies;
            else if (compareIgnoringCase(name, "planets") == 0)
                flags |= Renderer::ShowPlanets;
            else if (compareIgnoringCase(name, "stars") == 0)
                flags |= Renderer::ShowStars;
	    else if (compareIgnoringCase(name, "nightmaps") == 0)
		flags |= Renderer::ShowNightMaps;
            else if (compareIgnoringCase(name, "eclipseshadows") == 0)
                flags |= Renderer::ShowEclipseShadows;
            else if (compareIgnoringCase(name, "ringshadows") == 0)
                flags |= Renderer::ShowRingShadows;
            else if (compareIgnoringCase(name, "comettails") == 0)
                flags |= Renderer::ShowCometTails;
            else if (compareIgnoringCase(name, "boundaries") == 0)
                flags |= Renderer::ShowBoundaries;
            else if (compareIgnoringCase(name, "markers") == 0)
                flags |= Renderer::ShowMarkers;
            else if (compareIgnoringCase(name, "automag") == 0)
                flags |= Renderer::ShowAutoMag;
            else if (compareIgnoringCase(name, "atmospheres") == 0)
                flags |= Renderer::ShowAtmospheres;
            else if (compareIgnoringCase(name, "grid") == 0)
                flags |= Renderer::ShowCelestialSphere;
            else if (compareIgnoringCase(name, "partialtrajectories") == 0)
                flags |= Renderer::ShowPartialTrajectories;

            ttype = tokenizer.nextToken();
            if (ttype == Tokenizer::TokenBar)
                ttype = tokenizer.nextToken();
        }
        else
        {
            DPRINTF(0, "Command Parser: error parsing render flags\n");
            return 0;
        }
    }

    return flags;
}


int parseLabelFlags(string s)
{
#ifdef HAVE_SSTREAM
    istringstream in(s);
#else
    istrstream in(s.c_str());
#endif
    Tokenizer tokenizer(&in);
    int flags = 0;

    Tokenizer::TokenType ttype = tokenizer.nextToken();
    while (ttype != Tokenizer::TokenEnd)
    {
        if (ttype == Tokenizer::TokenName)
        {
            string name = tokenizer.getNameValue();
            if (compareIgnoringCase(name, "planets") == 0)
                flags |= Renderer::PlanetLabels;
            else if (compareIgnoringCase(name, "moons") == 0)
                flags |= Renderer::MoonLabels;
            else if (compareIgnoringCase(name, "spacecraft") == 0)
                flags |= Renderer::SpacecraftLabels;
            else if (compareIgnoringCase(name, "asteroids") == 0)
                flags |= Renderer::AsteroidLabels;
	    else if (compareIgnoringCase(name, "comets") == 0)
		flags |= Renderer::CometLabels;
            else if (compareIgnoringCase(name, "constellations") == 0)
                flags |= Renderer::ConstellationLabels;
            else if (compareIgnoringCase(name, "stars") == 0)
                flags |= Renderer::StarLabels;
	    else if (compareIgnoringCase(name, "galaxies") == 0)
		flags |= Renderer::GalaxyLabels;

            ttype = tokenizer.nextToken();
            if (ttype == Tokenizer::TokenBar)
                ttype = tokenizer.nextToken();
        }
        else
        {
            DPRINTF(0, "Command Parser: error parsing label flags\n");
            return 0;
        }
    }

    return flags;
}


int parseOrbitFlags(string s)
{
#ifdef HAVE_SSTREAM    
    istringstream in(s);
#else
    istrstream in(s.c_str());
#endif
    Tokenizer tokenizer(&in);
    int flags = 0;

    Tokenizer::TokenType ttype = tokenizer.nextToken();
    while (ttype != Tokenizer::TokenEnd)
    {
        if (ttype == Tokenizer::TokenName)
        {
            string name = tokenizer.getNameValue();
            if (compareIgnoringCase(name, "planet") == 0)
                flags |= Body::Planet;
            else if (compareIgnoringCase(name, "moon") == 0)
                flags |= Body::Moon;
            else if (compareIgnoringCase(name, "asteroid") == 0)
                flags |= Body::Asteroid;
            else if (compareIgnoringCase(name, "comet") == 0)
                flags |= Body::Comet;
            else if (compareIgnoringCase(name, "spacecraft") == 0)
                flags |= Body::Spacecraft;

            ttype = tokenizer.nextToken();
            if (ttype == Tokenizer::TokenBar)
                ttype = tokenizer.nextToken();
        }
        else
        {
            DPRINTF(0, "Command Parser: error parsing orbit flags\n");
            return 0;
        }
    }

    return flags;
}
