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

#ifndef _CELESTIA_WINHYPERLINKS_H_
#define _CELESTIA_WINHYPERLINKS_H_

#include <windows.h>

// The following preprocessor command is included to avoid compiling for
// WINVER >= 0x0500. We would like to be able to use the hand icon for
// clicking hyperlinks but the resource was not available until
// WINVER >= 0x0500. We will try to use this resource anyway. If it cannot
// be loaded, we will default to the standard arrow cursor.
#define IDC_HAND            MAKEINTRESOURCE(32649)

BOOL MakeHyperlinkFromStaticCtrl(HWND hDlg, UINT ctrlID);

#endif
