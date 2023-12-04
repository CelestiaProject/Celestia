// windroptarget.h
//
// Copyright (C) 2023, Celestia Development Team
//
// Extracted from winmain.cpp:
// Copyright (C) 2001-2007, Chris Laurel <claurel@shatters.net>
//
// A very minimal IDropTarget interface implementation.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <windows.h>
#include <oleidl.h>

class CelestiaCore;

namespace celestia::win32
{

class CelestiaDropTarget : public IDropTarget
{
public:
    explicit CelestiaDropTarget(CelestiaCore*);
    ~CelestiaDropTarget() = default;

    STDMETHOD  (QueryInterface)(REFIID idd, void** ppvObject);
    STDMETHOD_ (ULONG, AddRef) (void);
    STDMETHOD_ (ULONG, Release) (void);

    // IDropTarget methods
    STDMETHOD (DragEnter)(LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
    STDMETHOD (DragOver) (DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
    STDMETHOD (DragLeave)(void);
    STDMETHOD (Drop)     (LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);

private:
    CelestiaCore* appCore;
    ULONG refCount{ 0 };
};

} // end namespace celestia::win32
