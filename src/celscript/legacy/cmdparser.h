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

#pragma once

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include <celutil/array_view.h>
#include "command.h"

namespace celestia
{
namespace util
{
class Parser;
class Tokenizer;
}

namespace scripts
{

struct ScriptMaps;

class CommandParser
{
public:
    CommandParser(std::istream&, const celestia::scripts::ScriptMaps &sm);
    ~CommandParser();

    CommandSequence parse();
    celestia::util::array_view<std::string> getErrors() const;

private:
    std::unique_ptr<Command> parseCommand();
    void error(std::string&&);

    std::unique_ptr<util::Parser> parser;
    std::unique_ptr<util::Tokenizer> tokenizer;
    std::vector<std::string> errorList;
    const celestia::scripts::ScriptMaps &scriptMaps;
};

} // end namespace celestia::scripts
} // end namespace celestia
