// scriptparse.h
//
// Copyright (C) 2002 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELSCRIPT_SCRIPTPARSER_H_
#define _CELSCRIPT_SCRIPTPARSER_H_

#include <celscript/scanner.h>
#include <celscript/expression.h>

namespace celx
{

class Parser
{
public:
    Parser(Scanner&);
    ~Parser();

    Expression* parseExpression();

private:
    Scanner& scanner;
};

} // namespace celx

#endif // _CELSCRIPT_SCRIPTPARSER_H_
