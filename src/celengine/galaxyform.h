// galaxyform.h
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel, Fridger Schrempp, and Toti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <type_traits> // std::is_standard_layout_v<>
#include <vector>

#include <Eigen/Core>

#include <celcompat/filesystem.h>

namespace celestia::engine
{

class GalacticForm
{
public:
    struct Blob
    {
        Eigen::Vector3f position;
        std::uint8_t    colorIndex; // color index [0; 255]
        std::uint8_t    brightness; // blob brightness [0.0; 1.0] packed as normalized byte
    };
    static_assert(std::is_standard_layout_v<Blob>);

    using BlobVector = std::vector<Blob>;

    BlobVector      blobs;
    Eigen::Vector3f scale;
};

class GalacticFormManager
{
public:
    GalacticFormManager();

    static GalacticFormManager* get();

    const GalacticForm* getForm(int) const;
    int getCustomForm(const fs::path& path);

    int getCount() const;

private:
    void initializeStandardForms();

    static constexpr std::size_t GalacticFormsReserve = 32;

    std::vector<std::optional<GalacticForm>> galacticForms{ };
    std::map<fs::path, int> customForms{ };
};

} // namespace celestia::engine
