// completion.h
//
// Copyright (C) 2024-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <functional>
#include <string>
#include <variant>
#include <celengine/selection.h>

namespace celestia::engine
{

class Completion
{
public:
    Completion(const std::string& name, const std::variant<Selection, std::function<Selection()>>& selection) :
        name(name),
        selection(selection)
    {}

    std::string getName() const;
    Selection getSelection() const;

private:
    std::string name;
    std::variant<Selection, std::function<Selection()>> selection;
};

}
