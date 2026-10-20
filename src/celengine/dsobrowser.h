// dsobrowser.h
//
// Copyright (C) 2026, The Celestia Development Team
//
// Based on starbrowser.h:
// Original version:
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
// Incorporates elements from qtcelestialbrowser.cpp
// Copyright (C) 2007-2008, Celestia Development Team
//
// DSO browser tool for celestia.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <any>
#include <cstdint>
#include <functional>
#include <limits>
#include <vector>

#include <Eigen/Core>

#include "deepskyobj.h"
#include "univcoord.h"

class Universe;

namespace celestia::engine
{

struct DSOBrowserRecord
{
    explicit DSOBrowserRecord(const DeepSkyObject* _dso) : dso(_dso) {}
    const DeepSkyObject* dso;
    double distance{ std::numeric_limits<double>::max() };
    float appMag{ std::numeric_limits<float>::max() };
    std::any userData;
};

class DSOBrowser
{
public:
    static constexpr std::uint32_t MinListDSOs = 10;
    static constexpr std::uint32_t DefaultListDSOs = 100;
    static constexpr std::uint32_t MaxListDSOs = 1000;

    static_assert(MinListDSOs <= DefaultListDSOs);
    static_assert(DefaultListDSOs <= MaxListDSOs);

    enum class Comparison : int
    {
        Nearest,
        ApparentMagnitude,
        AbsoluteMagnitude,
    };

    explicit DSOBrowser(const Universe*,
                        std::uint32_t = DefaultListDSOs,
                        Comparison = Comparison::Nearest,
                        DeepSkyObjectType = DeepSkyObjectType::Galaxy);

    inline Comparison comparison() const noexcept { return m_comparison; }
    inline void comparison(Comparison newComparison) { m_comparison = newComparison; }

    inline DeepSkyObjectType dsoType() const noexcept { return m_dsoType; }
    inline void dsoType(DeepSkyObjectType newDsoType) { m_dsoType = newDsoType; }

    inline std::uint32_t size() const noexcept { return m_size; }
    bool size(std::uint32_t newSize);

    // Currently this is only used by the Qt front-end, whose implementation
    // relies on Qt's regular expression classes. For now, we allow this to be
    // supplied as a function.
    void setTypeFilter(const std::function<bool(const char*)>& filter) { m_typeFilter = filter; }

    const UniversalCoord& position() const noexcept { return m_ucPos; }
    void position(const UniversalCoord&);

    void populate(std::vector<DSOBrowserRecord>&) const;

private:
    const Universe* m_universe;

    // The star browser data is valid for a particular point
    // in space/time, and for performance issues is not continuously
    // updated.
    UniversalCoord m_ucPos;

    std::uint32_t m_size;

    Comparison m_comparison;
    DeepSkyObjectType m_dsoType;

    Eigen::Vector3d m_pos;

    std::function<bool(const char*)> m_typeFilter;
};

} // end namespace celestia::engine
