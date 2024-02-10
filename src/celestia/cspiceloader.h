// cspiceloader.h
//
// Copyright (C) 2024, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>

#include <celephem/spiceinterface.h>

namespace celestia
{

class SpiceLibraryWrapper
{
public:
    ~SpiceLibraryWrapper();

    SpiceLibraryWrapper(const SpiceLibraryWrapper&) = delete;
    SpiceLibraryWrapper& operator=(const SpiceLibraryWrapper&) = delete;
    SpiceLibraryWrapper(SpiceLibraryWrapper&&) noexcept = delete;
    SpiceLibraryWrapper& operator=(SpiceLibraryWrapper&&) noexcept = delete;

private:
    explicit SpiceLibraryWrapper(void* _handle) : handle(_handle) {}

    void* handle;

    friend std::unique_ptr<SpiceLibraryWrapper> LoadSpiceLibrary();
};

std::unique_ptr<SpiceLibraryWrapper> LoadSpiceLibrary();

}
