// urlmanager.h
//
// Copyright (C) 2026, the Celestia Development Team
// Original version by Andrew Tribick
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include "selection.h"

namespace celestia::engine
{

class UrlManager
{
public:
    std::string_view getURL(const Selection&) const;
    void setURL(const Selection&, std::string&&);

private:
    std::unordered_map<Selection, std::string> m_urls;
};

} // end namespace celestia::engine
