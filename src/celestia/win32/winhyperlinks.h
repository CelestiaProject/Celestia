// winhyperlinks.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// Code to convert a static control to a hyperlink.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <windows.h>

namespace celestia::win32
{

BOOL MakeHyperlinkFromStaticCtrl(HWND hDlg, UINT ctrlID);

} // end namespace celestia::win32
