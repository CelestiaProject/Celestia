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

using namespace std;


CommandParser::CommandParser(istream& in)
{
    tokenizer = new Tokenizer(&in);
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

    while (tokenizer->nextToken() != Tokenizer::TokenEnd)
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

    cout << "parsing: " << commandName << '\n';
    if (commandName == "wait")
    {
        double duration = 1.0;
        paramList->getNumber("duration", duration);
        return new CommandWait(duration);
    }
    else if (commandName == "select")
    {
        string object;
        paramList->getString("object", object);
        return new CommandSelect(object);
    }
    else if (commandName == "goto")
    {
        double t = 1.0;
        paramList->getNumber("time", t);
        return new CommandGoto(t);
    }
    else if (commandName == "center")
    {
        double t = 1.0;
        paramList->getNumber("time", t);
        return new CommandCenter(t);
    }
    else if (commandName == "follow")
    {
        return new CommandFollow();
    }
    else if (commandName == "cancel")
    {
        return new CommandCancel();
    }
    else if (commandName == "print")
    {
        string text;
        paramList->getString("text", text);
        return new CommandPrint(text);
    }
    else if (commandName == "cls")
    {
        return new CommandClearScreen();
    }
    else if (commandName == "time")
    {
        double jd = 2451545;
        paramList->getNumber("jd", jd);
        return new CommandSetTime(jd);
    }
    else if (commandName == "timerate")
    {
        double rate = 1.0;
        paramList->getNumber("rate", rate);
        return new CommandSetTimeRate(rate);
    }
    else if (commandName == "changedistance")
    {
        double rate = 0.0;
        double duration = 1.0;
        paramList->getNumber("rate", rate);
        paramList->getNumber("duration", duration);
        return new CommandChangeDistance(duration, rate);
    }
    else
    {
        error("Unknown command name '" + commandName + "'");
        return NULL;
    }
}
