// aboutdialog.h
//
// Copyright (C) 2025-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <vector>

namespace celestia::sdl
{

class AboutDialog //NOSONAR
{
public:
    AboutDialog();
    ~AboutDialog();

    AboutDialog(const AboutDialog&) = delete;
    AboutDialog& operator=(const AboutDialog&) = delete;
    AboutDialog(AboutDialog&&) noexcept = default;
    AboutDialog& operator=(AboutDialog&&) noexcept = default;

    void show(bool* isOpen) const;

private:
    struct LibraryInfo;
    std::vector<LibraryInfo> m_libraries;
};

}
