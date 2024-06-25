// windowmetrics.cpp
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

#include "windowmetrics.h"

namespace celestia
{

int
WindowMetrics::getSafeAreaWidth() const
{
    return width - insetLeft - insetRight;
}

int
WindowMetrics::getSafeAreaHeight() const
{
    return height - insetTop - insetBottom;
}

int
WindowMetrics::getSafeAreaStart(int offset) const
{
    return layoutDirection == LayoutDirection::LeftToRight
        ? insetLeft + offset
        : width - insetRight - offset;
}

int
WindowMetrics::getSafeAreaEnd(int offset) const
{
    return layoutDirection == LayoutDirection::LeftToRight
        ? width - insetRight - offset
        : insetLeft + offset;
}

int
WindowMetrics::getSafeAreaTop(int offset) const
{
    return height - insetTop - offset;
}

int
WindowMetrics::getSafeAreaBottom(int offset) const
{
    return insetBottom + offset;
}

} // end namespace celestia
