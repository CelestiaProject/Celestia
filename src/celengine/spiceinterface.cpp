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
#include <cstdio>
#include <iostream>
#include <set>

using namespace std;


// Track loaded SPICE kernels in order to avoid loading the same kernel
// multiple times. This is a global variable because SPICE uses a global
// kernel pool.
static set<string> ResidentSpiceKernels;


/*! Perform one-time initialization of SPICE.
 */
bool
InitializeSpice()
{
    // Set the error behavior to the RETURN action, so that
    // Celestia do its own handling of SPICE errors.
    erract_c("SET", 0, (SpiceChar*)"RETURN");

    return true;
}


/*! Convert an object name to a NAIF integer ID. Return true if the name
 *  refers to a known object, false if not. Both names and numeric IDs are
 *  accepted in the string.
 */
bool GetNaifId(const string& name, int* id)
{
    SpiceInt spiceID = 0;
    SpiceBoolean found = SPICEFALSE;

    // Don't call bodn2c on an empty string because SPICE generates
    // an error if we do.
    if (!name.empty())
    {
        bodn2c_c(name.c_str(), &spiceID, &found);
        if (found)
        {
            *id = (int) spiceID;
        }
        else   // Is it a numeric ID?
        {
            // SpiceInt maps to an int on those architectures where sizeof(long) != sizeof(double)/2.
            // Otherwise it maps to a long. This avoids GCC complaints about type mismatches:
            long spiceIDlng;
            if (sscanf(name.c_str(), " %ld", &spiceIDlng) == 1)
            {
                *id = (int) spiceIDlng;
                found = SPICETRUE;
            }
        }
    }

    return found == SPICETRUE;
}


/*! Return true if a SPICE kernel has already been loaded, false if not.
 */
bool IsSpiceKernelLoaded(const string& filepath)
{
	return ResidentSpiceKernels.find(filepath) != ResidentSpiceKernels.end();
}


/*! Load a SPICE kernel file of any type into the kernel pool. If the kernel
 *  is already resident, it will not be reloaded.
 */
bool LoadSpiceKernel(const string& filepath)
{
	// Only load the kernel if it is not already resident. Note that this detection
    // of duplicate kernels will not work if a file was originally loaded through
    // a metakernel.
	if (IsSpiceKernelLoaded(filepath))
		return true;

	ResidentSpiceKernels.insert(filepath);
	furnsh_c(filepath.c_str());

	// If there was an error loading the kernel, dump the error message.
	if (failed_c())
	{
		char errMsg[1024];
		getmsg_c("long", sizeof(errMsg), errMsg);
		clog << errMsg << "\n";

		// Reset the SPICE error state so that future calls to
		// SPICE can still succeed.
		reset_c();

		return false;
	}
	else
	{
		clog << "Loaded SPK file " << filepath << "\n";
		return true;
	}
}
