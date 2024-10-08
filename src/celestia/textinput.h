// textinput.h
//
// Copyright (C) 2023, the Celestia Development Team
//
// Split out from celestiacore.h/celestiacore.cpp
// Copyright (C) 2001-2009, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <celengine/completion.h>
#include <celutil/array_view.h>

class Color;
class Overlay;
class Simulation;

namespace celestia
{

class HudFonts;
struct WindowMetrics;

enum class CharEnteredResult
{
    Normal,
    Finished,
    Cancelled,
};

class TextInput
{
public:
    std::string_view getTypedText() const;
    util::array_view<engine::Completion> getCompletion() const;
    std::optional<Selection> getSelectedCompletion();

    CharEnteredResult charEntered(const Simulation*, std::string_view, bool withLocations);
    void appendText(const Simulation*, std::string_view, bool withLocations);
    void reset();

    void render(Overlay*, const HudFonts&, const WindowMetrics&) const;

private:
    void doBackspace(const Simulation*, bool);
    void doTab();
    void doBackTab();

    void renderCompletion(Overlay*, const WindowMetrics&, int) const;

    std::string m_text;
    std::vector<engine::Completion> m_completion;
    int m_completionIdx{ -1 };
};

}
