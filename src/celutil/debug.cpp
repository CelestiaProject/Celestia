// debug.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

int debugVerbosity = 0;

void SetDebugVerbosity(int dv)
{
    if(dv<0)
        dv=0;
    debugVerbosity = dv;
}


int GetDebugVerbosity()
{
    return debugVerbosity;
}
