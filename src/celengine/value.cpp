// value.cpp
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "value.h"

namespace celestia
{
namespace engine
{

/****** Value method implementations *******/

Value::~Value()
{
    switch (type)
    {
    case StringType:
        delete data.s;
        break;
    case ArrayType:
        if (data.a != nullptr)
        {
            for (auto *p : *data.a)
                delete p;
            delete data.a;
        }
        break;
    case HashType:
        if (data.h != nullptr)
        {
#if 0 // TODO: fixme
            for (const auto &t : *data.h)
            {
                delete t.second;
            }
#endif
            delete data.h;
        }
        break;
    default:
        break;
    }
}

}
}
