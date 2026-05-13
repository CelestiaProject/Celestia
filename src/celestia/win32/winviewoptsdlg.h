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

#pragma once

#include <cstdint>

#include <windows.h>

#include <celengine/render.h>
#include <celestia/celestiacore.h>

namespace celestia::win32
{

class ViewOptionsDialog : public RendererWatcher
{
public:
    ViewOptionsDialog(HINSTANCE, HWND, CelestiaCore*);
    ~ViewOptionsDialog();

    void SetControls(HWND);
    void RestoreSettings(HWND);

    void notifyRenderSettingsChanged(const Renderer*) override;

public:
    CelestiaCore* appCore;
    HWND parent;
    HWND hwnd;
    RenderFlags initialRenderFlags;
    RenderLabels initialLabelMode;
    int initialHudDetail;
};

} // end namespace celestia::win32
