// parser.h
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>

#include "hash.h"
#include "value.h"

class Tokenizer;
class Value;

class Parser
{
 public:
    Parser(Tokenizer*);

    Value* readValue();

 private:
    Tokenizer* tokenizer;

    bool readUnits(const std::string&, Hash*);
    ValueArray* readArray();
    Hash* readHash();
};
