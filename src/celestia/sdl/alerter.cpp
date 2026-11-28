// alerter.cpp
//
// Copyright (C) 2020-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "alerter.h"

#include <array>

#include <SDL_error.h>
#include <SDL_keyboard.h>
#include <SDL_log.h>
#include <SDL_messagebox.h>

#include <celutil/gettext.h>

namespace celestia::sdl
{

void
Alerter::fatalError(const std::string& msg)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", msg.c_str(), m_window);
}

CelestiaCore::ScriptSystemAccessPolicy
Alerter::requestScriptSystemAccess(const std::string& scriptPath)
{
    using Policy = CelestiaCore::ScriptSystemAccessPolicy;
#ifdef __EMSCRIPTEN__
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "SDL script system access prompt unavailable in browser; denying access to %s.", scriptPath.c_str());
    return Policy::Deny;
#else
    constexpr int allow = 1;
    const std::array buttons{
        SDL_MessageBoxButtonData{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, _("Deny") },
        SDL_MessageBoxButtonData{ 0, allow, _("Allow") },
    };
    const auto message = scriptPath + "\n\n" +
        _("This script requests permission to read/write files "
          "and execute external programs. Allowing this can be "
          "dangerous.\n"
          "Do you trust the script and want to allow this?");
    const SDL_MessageBoxData prompt{
        SDL_MESSAGEBOX_WARNING, SDL_GetKeyboardFocus(), _("Script System Access"), message.c_str(),
        static_cast<int>(buttons.size()), buttons.data(), nullptr,
    };
    int selected = -1;
    if (SDL_ShowMessageBox(&prompt, &selected) != 0)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Could not display script system access prompt; denying access: %s", SDL_GetError());
        return Policy::Deny;
    }
    return selected == allow ? Policy::Allow : Policy::Deny;
#endif
}

}
