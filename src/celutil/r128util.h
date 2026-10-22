// r128util.h
//
// Copyright (C) 2007-present, Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// 128-bit fixed point (64.64) numbers for high-precision celestial
// coordinates. When you need millimeter accurate navigation across a scale
// of thousands of light years, double precision floating point numbers
// are inadequate.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <optional>
#include <string_view>

#include <fmt/format.h>

#include "r128.h"

namespace celestia::util
{

std::optional<R128> DecodeFromBase64(std::string_view);

// Checks whether the coordinate exceeds a magnitude of 2^62 microlightyears,
// which represents the bounds of the simulated volume.
bool isOutOfBounds(const R128 &);

} // end namespace celestia::util

template<>
struct fmt::formatter<R128> : formatter<std::string_view>
{
    auto format(const R128& value, format_context& ctx) const -> decltype(ctx.out());
};
