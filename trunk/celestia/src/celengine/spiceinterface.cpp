// spiceinterface.cpp
//
// Interface to the SPICE Toolkit
//
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "SpiceUsr.h"
#include "spiceinterface.h"


bool
InitializeSpice()
{
    // Set the error behavior to the RETURN action, so that
    // Celestia do its own handling of SPICE errors.
    erract_c("SET", 0, (SpiceChar*)"RETURN");

    return true;
}
