// qtcompareutil.cpp
//
// Copyright (C) 2026, The Celestia Development Team
//
// Sort order predicate using Qt::SortOrder.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Qt>

namespace celestia::qt
{

class SortOrderComparePredicate
{
public:
    explicit SortOrderComparePredicate(Qt::SortOrder sortOrder) : m_sortOrder(sortOrder) {}

    template<typename T>
    bool operator()(const T& a, const T& b) const
    {
        return m_sortOrder == Qt::SortOrder::AscendingOrder ? a < b : b < a;
    }

private:
    Qt::SortOrder m_sortOrder;
};

}
