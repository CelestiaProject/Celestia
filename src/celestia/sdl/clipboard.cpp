// clipboard.cpp
//
// Copyright (C) 2020-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "clipboard.h"

#include <string>

#include <SDL_clipboard.h>
#include <SDL_stdinc.h>

#include <celestia/celestiacore.h>
#include <celestia/celestiastate.h>
#include <celestia/hud.h>
#include <celestia/url.h>
#include <celutil/gettext.h>

namespace celestia::sdl
{

void
doCopy(CelestiaCore& appCore)
{
    CelestiaState appState(&appCore);
    appState.captureState();

    if (SDL_SetClipboardText(Url(appState).getAsString().c_str()) == 0)
        appCore.flash(_("Copied URL"));
}

void
doPaste(CelestiaCore& appCore)
{
    if (SDL_HasClipboardText() != SDL_TRUE)
        return;

    char* str = SDL_GetClipboardText();
    if (str == nullptr)
        return;

    if (appCore.getTextEnterMode() == Hud::TextEnterMode::Normal)
    {
        if (appCore.goToUrl(str))
            appCore.flash(_("Pasting URL"));
    }
    else
    {
        appCore.setTypedText(str);
    }

    SDL_free(str);
}

} // end namespace celestia::sdl
