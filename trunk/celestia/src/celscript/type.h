// type.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef CELSCRIPT_TYPE_H_
#define CELSCRIPT_TYPE_H_

#include <celscript/celx.h>


namespace celx
{

enum Type
{
    NilType     = 0,
    NumberType  = 1,
    StringType  = 2,
    VectorType  = 3,
    BooleanType = 4,
    InvalidType = -1,
};

}

#endif // CELSCRIPT_TYPE_H_
