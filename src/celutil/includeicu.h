// icu.h
//
// Copyright (C) 2018-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#ifdef USE_ICU
#ifdef HAVE_WIN_ICU
#include <icu.h>
#else
#include <unicode/ubidi.h>
#include <unicode/udat.h>
#include <unicode/decimfmt.h>
#include <unicode/uloc.h>
#include <unicode/ulocdata.h>
#include <unicode/umachine.h>
#include <unicode/ushape.h>
#include <unicode/ustring.h>
#endif
#endif
