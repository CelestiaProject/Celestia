// spiceinterface.cpp
//
// Interface to the SPICE Toolkit
//
// Copyright (C) 2006-2008, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstddef>
#include <string>
#include <type_traits>

#include <celcompat/filesystem.h>

// Various definitions from SpiceCel.h and SpiceZdf.h

using SpiceChar   = char;
using SpiceDouble = double;

using SpiceInt = std::conditional_t<sizeof(long) * 2 == sizeof(SpiceDouble), long, int>;
static_assert(sizeof(SpiceInt) * 2 == sizeof(SpiceDouble));

using SpiceBoolean = int;
constexpr inline SpiceBoolean SPICEFALSE = 0;
constexpr inline SpiceBoolean SPICETRUE = 1;

using ConstSpiceChar = const SpiceChar;

enum _SpiceDataType
{
    SPICE_CHR  = 0,
    SPICE_DP   = 1,
    SPICE_INT  = 2,
    SPICE_TIME = 3,
    SPICE_BOOL = 4
};

using SpiceDataType = _SpiceDataType;
using SpiceCellDataType = _SpiceDataType;

struct _SpiceCell
{
    SpiceCellDataType dtype;
    SpiceInt          length;
    SpiceInt          size;
    SpiceInt          card;
    SpiceBoolean      isSet;
    SpiceBoolean      adjust;
    SpiceBoolean      init;
    void*             base;
    void*             data;
};

using SpiceCell = _SpiceCell;

namespace celestia::ephem
{

struct SpiceInterface
{
    void            (*bodn2c_c)(ConstSpiceChar* name, SpiceInt* code, SpiceBoolean* found);
    SpiceInt        (*card_c  )(SpiceCell* cell);
    void            (*erract_c)(ConstSpiceChar* op, SpiceInt actlen, SpiceChar* action);
    SpiceBoolean    (*failed_c)();
    void            (*furnsh_c)(ConstSpiceChar* file);
    void            (*getmsg_c)(ConstSpiceChar* option, SpiceInt msglen, SpiceChar* msg);
    void            (*kdata_c )(SpiceInt which, ConstSpiceChar* kind,
                                SpiceInt fileln, SpiceInt filtln, SpiceInt srclen,
                                SpiceChar* file, SpiceChar* filtyp, SpiceChar* srcfil,
                                SpiceInt* handle, SpiceBoolean* found);
    void            (*ktotal_c)(ConstSpiceChar* kind, SpiceInt* count);
    void            (*pxform_c)(ConstSpiceChar* from, ConstSpiceChar* to, SpiceDouble et, SpiceDouble rotate[3][3]);
    void            (*reset_c )(void);
    void            (*scard_c )(SpiceInt card, SpiceCell* cell);
    void            (*spkcov_c)(ConstSpiceChar* spkfnm, SpiceInt idcode, SpiceCell* cover);
    void            (*spkgeo_c)(SpiceInt targ, SpiceDouble et, ConstSpiceChar* ref, SpiceInt obs, SpiceDouble state[6], SpiceDouble* lt);
    void            (*spkgps_c)(SpiceInt targ, SpiceDouble et, ConstSpiceChar* ref, SpiceInt obs, SpiceDouble pos[3], SpiceDouble* lt);
    ConstSpiceChar* (*tkvrsn_c)(ConstSpiceChar*);
    void            (*wnfetd_c)(SpiceCell* window, SpiceInt n, SpiceDouble* left, SpiceDouble* right);
    SpiceBoolean    (*wnincd_c)(SpiceDouble left, SpiceDouble right, SpiceCell* window);
};

void SetSpiceInterface(const SpiceInterface*);
const SpiceInterface* GetSpiceInterface();

// SPICE utility functions

bool GetNaifId(const std::string& name, int* id);
bool LoadSpiceKernel(const fs::path& filepath);

}
