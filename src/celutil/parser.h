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

#include <memory>

#include "associativearray.h"

namespace celestia::util
{

class Tokenizer;

class Parser
{
public:
    explicit Parser(Tokenizer*);
    ~Parser() = default;

    Value readValue();

private:
    Tokenizer* tokenizer;

    std::unique_ptr<ValueArray> readArray();
    std::unique_ptr<AssociativeArray> readHash();
};

} // end namespace celestia::util
