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
#include <sstream>
#include "util.h"
#include "astro.h"
#include "cmdparser.h"

using namespace std;


static int parseRenderFlags(string);
static int parseLabelFlags(string);

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

    cout << "parsing: " << commandName << '\n';
    if (commandName == "wait")
    {
        double duration = 1.0;
        paramList->getNumber("duration", duration);
        cmd = new CommandWait(duration);
    }
    else if (commandName == "select")
    {
        string object;
        paramList->getString("object", object);
        cmd = new CommandSelect(object);
    }
    else if (commandName == "goto")
    {
        double t = 1.0;
        paramList->getNumber("time", t);
        cmd = new CommandGoto(t);
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
    else if (commandName == "cancel")
    {
        cmd = new CommandCancel();
    }
    else if (commandName == "print")
    {
        string text;
        paramList->getString("text", text);
        cmd = new CommandPrint(text);
    }
    else if (commandName == "cls")
    {
        cmd = new CommandClearScreen();
    }
    else if (commandName == "time")
    {
        double jd = 2451545;
        paramList->getNumber("jd", jd);
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
                               (float) rate);
    }
    else if (commandName == "setposition")
    {
        Vec3d base, offset;
        paramList->getVector("base", base);
        paramList->getVector("offset", offset);
        cmd = new CommandSetPosition(astro::universalPosition(Point3d(offset.x, offset.y, offset.z),
                                                              Point3f((float) base.x, (float) base.y, (float) base.z)));
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
    istringstream in(s);
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

            ttype = tokenizer.nextToken();
            if (ttype == Tokenizer::TokenBar)
                ttype = tokenizer.nextToken();
        }
        else
        {
            DPRINTF("Error parsing render flags\n");
            return 0;
        }
    }

    return flags;
}


int parseLabelFlags(string s)
{
    istringstream in(s);
    Tokenizer tokenizer(&in);
    int flags = 0;

    Tokenizer::TokenType ttype = tokenizer.nextToken();
    while (ttype != Tokenizer::TokenEnd)
    {
        if (ttype == Tokenizer::TokenName)
        {
            string name = tokenizer.getNameValue();
            if (compareIgnoringCase(name, "planets") == 0)
                flags |= Renderer::MajorPlanetLabels;
            else if (compareIgnoringCase(name, "minorplanets") == 0)
                flags |= Renderer::MinorPlanetLabels;
            else if (compareIgnoringCase(name, "constellations") == 0)
                flags |= Renderer::ConstellationLabels;
            else if (compareIgnoringCase(name, "stars") == 0)
                flags |= Renderer::StarLabels;

            ttype = tokenizer.nextToken();
            if (ttype == Tokenizer::TokenBar)
                ttype = tokenizer.nextToken();
        }
        else
        {
            DPRINTF("Error parsing label flags\n");
            return 0;
        }
    }

    return flags;
}
