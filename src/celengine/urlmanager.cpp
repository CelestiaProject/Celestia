// urlmanager.cpp
//
// Copyright (C) 2026, the Celestia Development Team
// Original version by Andrew Tribick
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "urlmanager.h"

#include <utility>

namespace celestia::engine
{

std::string_view
UrlManager::getURL(const Selection& selection) const
{
    if (auto it = m_urls.find(selection); it != m_urls.end())
        return it->second;

    return {};
}

void
UrlManager::setURL(const Selection& selection, std::string&& infoUrl)
{
    if (infoUrl.empty())
        m_urls.erase(selection);
    else
        m_urls.insert_or_assign(selection, std::move(infoUrl));
}

}
