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

#include <iosfwd>
#include <string_view>
#include <celscript/common/scriptmaps.h>
#include <celutil/array_view.h>
#include "command.h" // CommandSequence

class Command;
class Parser;
class Tokenizer;


class CommandParser
{
 public:
    CommandParser(std::istream&, const std::shared_ptr<celestia::scripts::ScriptMaps> &sm);
    CommandParser(Tokenizer&, const std::shared_ptr<celestia::scripts::ScriptMaps> &sm);
    ~CommandParser();

    CommandSequence* parse();
    celestia::util::array_view<std::string> getErrors() const;

 private:
    Command* parseCommand();
    void error(std::string);

    Parser* parser;
    Tokenizer* tokenizer;
    std::vector<std::string> errorList;
    std::shared_ptr<celestia::scripts::ScriptMaps> scriptMaps;
};

#endif // _CMDPARSER_H_

