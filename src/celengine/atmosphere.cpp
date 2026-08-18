// atmosphere.cpp
//
// Copyright (C) 2001-2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "atmosphere.h"

#include <algorithm>
#include <cmath>

bool
Atmosphere::hasValidScaleHeights() const noexcept
{
    return mieScaleHeight > 0.0f && std::isfinite(mieScaleHeight) &&
           rayleighScaleHeight > 0.0f && std::isfinite(rayleighScaleHeight);
}

float
Atmosphere::getLegacyScaleHeight() const noexcept
{
    return hasValidScaleHeights() ? std::max(mieScaleHeight, rayleighScaleHeight) : mieScaleHeight;
}

float
Atmosphere::getLegacyMieCoeff() const noexcept
{
    float legacyScaleHeight = getLegacyScaleHeight();
    return hasValidScaleHeights() ? mieCoeff * mieScaleHeight / legacyScaleHeight : mieCoeff;
}
