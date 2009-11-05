// wingotodlg.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Goto object dialog for Windows.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _WINGOTODLG_H_
#define _WINGOTODLG_H_

#include "celestia/celestiacore.h"


class GotoObjectDialog
{
 public:
    GotoObjectDialog(HINSTANCE, HWND, CelestiaCore*);

 public:
    CelestiaCore* appCore;
    HWND parent;
    HWND hwnd;
};


#endif // _WINGOTODLG_H_
