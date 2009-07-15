// cmdparser.h
//
// Parse Celestia command files and turn them into CommandSequences.
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CMDPARSER_H_
#define _CMDPARSER_H_

#include <celengine/command.h>
#include <celengine/parser.h>
#include <iostream>


class CommandParser
{
 public:
    CommandParser(std::istream&);
    CommandParser(Tokenizer&);
    ~CommandParser();

    CommandSequence* parse();
    const std::vector<std::string>* getErrors() const;

 private:
    Command* parseCommand();
    void error(const string);

    Parser* parser;
    Tokenizer* tokenizer;
    std::vector<std::string> errorList;
};

#endif // _CMDPARSER_H_

