// winviewoptsdlg.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// View Options dialog for Windows.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _WINVIEWOPTSDLG_H_
#define _WINVIEWOPTSDLG_H_

#include "celestiacore.h"

class ViewOptionsDialog : public CelestiaWatcher
{
 public:
    ViewOptionsDialog(HINSTANCE, HWND, CelestiaCore*);

    void SetControls(HWND);
    void RestoreSettings(HWND);

    virtual void notifyChange(CelestiaCore*, int);

 public:
    CelestiaCore* appCore;
    HWND parent;
    HWND hwnd;
    int initialRenderFlags;
    int initialLabelMode;
    int initialHudDetail;
};


#endif // _WINVIEWOPTSDLG_H_
