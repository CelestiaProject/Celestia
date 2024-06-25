// datehelpers.h
//
// Copyright (C) 2023, Celestia Development Team
//
// Utilities for date handling in the Windows UI
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <celutil/array_view.h>
#include "tstring.h"

namespace celestia::win32
{

util::array_view<tstring> GetLocalizedMonthNames();

} // end namespace celestia::win32
