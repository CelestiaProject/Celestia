// starbrowser.h
//
// Copyright (C) 2023, The Celestia Development Team
//
// Original version:
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
// Incorporates elements from qtcelestialbrowser.cpp
// Copyright (C) 2007-2008, Celestia Development Team
//
// Star browser tool for celestia.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

#include <Eigen/Core>

#include <celastro/date.h>
#include <celutil/array_view.h>
#include <celutil/flag.h>
#include "solarsys.h"
#include "univcoord.h"

class Star;
class Universe;

namespace celestia::engine
{

struct StarBrowserRecord
{
    explicit StarBrowserRecord(const Star* _star) : star(_star) {}
    const Star* star;
    float distance{ std::numeric_limits<float>::max() };
    float appMag{ std::numeric_limits<float>::max() };
};

class StarBrowser
{
public:
    static constexpr std::uint32_t MinListStars = 10;
    static constexpr std::uint32_t DefaultListStars = 100;
    static constexpr std::uint32_t MaxListStars = 1000;

    static_assert(MinListStars <= DefaultListStars);
    static_assert(DefaultListStars <= MaxListStars);

    enum class Comparison : int
    {
        Nearest,
        ApparentMagnitude,
        AbsoluteMagnitude,
    };

    enum class Filter : unsigned int
    {
        All = 0,
        Visible = 1,
        Multiple = 2,
        WithPlanets = 4,
        SpectralType = 8,
    };

    StarBrowser(const Universe*,
                std::uint32_t = DefaultListStars,
                Comparison = Comparison::Nearest,
                Filter = Filter::Visible);

    inline const Universe* universe() const { return m_universe; }

    inline Comparison comparison() const { return m_comparison; }
    inline void setComparison(Comparison newComparison) { m_comparison = newComparison; }

    inline Filter filter() const { return m_filter; }
    inline void setFilter(Filter newFilter) { m_filter = newFilter; }

    inline std::uint32_t size() const { return m_size; }
    bool setSize(std::uint32_t newSize);

    // Currently this is only used by the Qt front-end, whose implementation
    // relies on Qt's regular expression classes. For now, we allow this to be
    // supplied as a function, in future we may want to implement a more
    // specialized version to enable queries like "B5-F5"
    void setSpectralTypeFilter(const std::function<bool(const char*)>& filter) { m_spectralTypeFilter = filter; }

    const UniversalCoord& position() const { return m_ucPos; }
    void setPosition(const UniversalCoord&);
    double time() const { return m_jd; }
    inline void setTime(double jd) { m_jd = jd; }

    void populate(std::vector<StarBrowserRecord>&) const;

private:
    const Universe* m_universe;

    // The star browser data is valid for a particular point
    // in space/time, and for performance issues is not continuously
    // updated.
    UniversalCoord m_ucPos;
    double m_jd{ celestia::astro::J2000 };

    std::uint32_t m_size;

    Comparison m_comparison;
    Filter m_filter;

    std::function<bool(const char*)> m_spectralTypeFilter;
};

ENUM_CLASS_BITWISE_OPS(StarBrowser::Filter);

} // end namespace celestia::engine
