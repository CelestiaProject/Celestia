// dsobrowser.cpp
//
// Copyright (C) 2026, The Celestia Development Team
//
// Based on starbrowser.cpp
// Original version:
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
// Incorporates elements from qtcelestialbrowser.cpp
// Copyright (C) 2007-2008, Celestia Development Team
//
// Star browser tool for Celestia.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "dsobrowser.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <numbers>

#include <celastro/astro.h>
#include "dsodb.h"
#include "dsooctree.h"
#include "universe.h"

namespace celestia::engine
{

namespace
{

using UniqueDSO = std::unique_ptr<DeepSkyObject>;

class DSOFilter
{
public:
    explicit DSOFilter(DeepSkyObjectType);
    bool operator()(const DeepSkyObject&) const;

    void setTypeFilter(const std::function<bool(const char*)>&);

private:
    DeepSkyObjectType m_dsoType;
    std::function<bool(const char*)> m_typeFilter;
};

DSOFilter::DSOFilter(DeepSkyObjectType dsoType) :
    m_dsoType(dsoType)
{
}

void
DSOFilter::setTypeFilter(const std::function<bool(const char*)>& filter)
{
    m_typeFilter = filter;
}

bool
DSOFilter::operator()(const DeepSkyObject& dso) const
{
    return dso.getObjType() == m_dsoType &&
           (!m_typeFilter || m_typeFilter(dso.getType()));
}

class DistanceProcessor
{
public:
    DistanceProcessor(std::vector<DSOBrowserRecord>&,
                      std::uint32_t,
                      const DSOFilter&,
                      const Eigen::Vector3d&);

    ~DistanceProcessor() = default;

    DistanceProcessor(const DistanceProcessor&) = delete;
    DistanceProcessor& operator=(const DistanceProcessor&) = delete;
    DistanceProcessor(DistanceProcessor&&) noexcept = default;
    DistanceProcessor& operator=(DistanceProcessor&&) noexcept = default;

    bool checkNode(const Eigen::Vector3d&, double, float) const;
    void process(const UniqueDSO&);
    void finalize();

private:
    static bool compare(const DSOBrowserRecord&, const DSOBrowserRecord&);

    const DSOFilter* m_filter;
    std::vector<DSOBrowserRecord>* m_records;
    Eigen::Vector3d m_pos;
    std::uint32_t m_size;
    double m_maxDistance{ std::numeric_limits<double>::max() };
};

DistanceProcessor::DistanceProcessor(std::vector<DSOBrowserRecord>& records,
                                     std::uint32_t size,
                                     const DSOFilter& filter,
                                     const Eigen::Vector3d& pos) :
    m_filter(&filter),
    m_records(&records),
    m_pos(pos),
    m_size(size)
{
    assert(m_size > 0);
}

bool
DistanceProcessor::compare(const DSOBrowserRecord& lhs, const DSOBrowserRecord& rhs)
{
    return lhs.distance < rhs.distance;
}

bool
DistanceProcessor::checkNode(const Eigen::Vector3d& center,
                             double size,
                             float /* brightestMag */) const
{
    if (m_records->size() < m_size)
        return true;

    // Compute the distance to node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * SQRT3.
    double nodeDistance = (m_pos - center).norm() - size * std::numbers::sqrt3;
    return nodeDistance <= m_maxDistance;
}

void
DistanceProcessor::process(const UniqueDSO& dso)
{
    if (!(*m_filter)(*dso))
        return;

    const DeepSkyObject* oldWorst = m_records->empty() ? nullptr : m_records->front().dso;
    auto distance2 = (dso->getPosition() - m_pos).squaredNorm();
    if (m_records->size() < m_size)
    {
        auto& record = m_records->emplace_back(dso.get());
        record.distance = distance2;
        std::ranges::push_heap(*m_records, &compare);
    }
    else if (distance2 < m_records->front().distance)
    {
        std::ranges::pop_heap(*m_records, &compare);
        auto& record = m_records->back();
        record.dso = dso.get();
        record.distance = distance2;
        std::ranges::push_heap(*m_records, &compare);
    }
    else
    {
        return;
    }

    if (m_records->front().dso != oldWorst)
        m_maxDistance = std::sqrt(m_records->front().distance);
}

void
DistanceProcessor::finalize()
{
    std::ranges::sort_heap(*m_records, &compare);
    for (DSOBrowserRecord& record : *m_records)
    {
        record.distance = std::sqrt(record.distance);
        record.appMag = astro::absToAppMag(record.dso->getAbsoluteMagnitude(), record.distance);
    }
}

class AppMagProcessor
{
public:
    AppMagProcessor(std::vector<DSOBrowserRecord>&,
                    std::uint32_t,
                    const DSOFilter&,
                    const Eigen::Vector3d&);

    ~AppMagProcessor() = default;

    AppMagProcessor(const AppMagProcessor&) = delete;
    AppMagProcessor& operator=(const AppMagProcessor&) = delete;
    AppMagProcessor(AppMagProcessor&&) noexcept = default;
    AppMagProcessor& operator=(AppMagProcessor&&) noexcept = default;

    bool checkNode(const Eigen::Vector3d&, double, float) const;
    void process(const UniqueDSO&);
    void finalize();

private:
    static bool compare(const DSOBrowserRecord&, const DSOBrowserRecord&);

    const DSOFilter* m_filter;
    std::vector<DSOBrowserRecord>* m_records;
    Eigen::Vector3d m_pos;
    std::uint32_t m_size;
};

AppMagProcessor::AppMagProcessor(std::vector<DSOBrowserRecord>& records,
                                 std::uint32_t size,
                                 const DSOFilter& filter,
                                 const Eigen::Vector3d& pos) :
    m_filter(&filter),
    m_records(&records),
    m_pos(pos),
    m_size(size)
{
    assert(m_size > 0);
}

bool
AppMagProcessor::compare(const DSOBrowserRecord& lhs, const DSOBrowserRecord& rhs)
{
    return lhs.appMag < rhs.appMag;
}

bool
AppMagProcessor::checkNode(const Eigen::Vector3d& center,
                           double size,
                           float brightestMag) const
{
    if (m_records->size() < m_size)
        return true;

    // Compute the distance to node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * SQRT3.
    double nodeDistance = (m_pos - center).norm() - size * std::numbers::sqrt3;
    if (nodeDistance < 1e-3)
        return true;

    double nodeAppMag = astro::absToAppMag(brightestMag, nodeDistance);
    return nodeAppMag < m_records->front().appMag;
}

void
AppMagProcessor::process(const UniqueDSO& dso)
{
    if (!(*m_filter)(*dso))
        return;

    auto distance = (dso->getPosition() - m_pos).norm();
    auto appMag = astro::absToAppMag(dso->getAbsoluteMagnitude(), distance);
    if (m_records->size() < m_size)
    {
        auto& record = m_records->emplace_back(dso.get());
        record.distance = distance;
        record.appMag = appMag;
        std::ranges::push_heap(*m_records, &compare);
    }
    else if (appMag < m_records->front().appMag)
    {
        std::ranges::pop_heap(*m_records, &compare);
        auto& record = m_records->back();
        record.dso = dso.get();
        record.distance = distance;
        record.appMag = appMag;
        std::ranges::push_heap(*m_records, &compare);
    }
}

void
AppMagProcessor::finalize()
{
    std::ranges::sort_heap(*m_records, &compare);
}

class AbsMagProcessor
{
public:
    AbsMagProcessor(std::vector<DSOBrowserRecord>&,
                    std::uint32_t,
                    const DSOFilter&,
                    const Eigen::Vector3d&);

    ~AbsMagProcessor() = default;

    AbsMagProcessor(const AbsMagProcessor&) = delete;
    AbsMagProcessor& operator=(const AbsMagProcessor&) = delete;
    AbsMagProcessor(AbsMagProcessor&&) noexcept = default;
    AbsMagProcessor& operator=(AbsMagProcessor&&) noexcept = default;

    bool checkNode(const Eigen::Vector3d&, double, float) const;
    void process(const UniqueDSO&);
    void finalize();

private:
    static bool compare(const DSOBrowserRecord&, const DSOBrowserRecord&);

    const DSOFilter* m_filter;
    std::vector<DSOBrowserRecord>* m_records;
    UniversalCoord m_ucPos;
    Eigen::Vector3d m_pos;
    std::uint32_t m_size;
};

AbsMagProcessor::AbsMagProcessor(std::vector<DSOBrowserRecord>& records,
                                 std::uint32_t size,
                                 const DSOFilter& filter,
                                 const Eigen::Vector3d& pos) :
    m_filter(&filter),
    m_records(&records),
    m_pos(pos),
    m_size(size)
{
    assert(m_size > 0);
}

bool
AbsMagProcessor::compare(const DSOBrowserRecord& lhs, const DSOBrowserRecord& rhs)
{
    return lhs.dso->getAbsoluteMagnitude() < rhs.dso->getAbsoluteMagnitude();
}

bool
AbsMagProcessor::checkNode(const Eigen::Vector3d& /* center */,
                           double /* size */,
                           float brightestMag) const
{
    if (m_records->size() < m_size)
        return true;

    return brightestMag < m_records->front().dso->getAbsoluteMagnitude();
}

void
AbsMagProcessor::process(const UniqueDSO& dso)
{
    if (!(*m_filter)(*dso))
        return;

    if (m_records->size() < m_size)
    {
        m_records->emplace_back(dso.get());
        std::ranges::push_heap(*m_records, &compare);
    }
    else if (dso->getAbsoluteMagnitude() < m_records->front().dso->getAbsoluteMagnitude())
    {
        std::ranges::pop_heap(*m_records, &compare);
        m_records->back().dso = dso.get();
        std::ranges::push_heap(*m_records, &compare);
    }
}

void
AbsMagProcessor::finalize()
{
    std::ranges::sort_heap(*m_records, &compare);
    for (DSOBrowserRecord& record : *m_records)
    {
        record.distance = (record.dso->getPosition() - m_pos).norm();
        record.appMag = astro::absToAppMag(record.dso->getAbsoluteMagnitude(), record.distance);
    }
}

template<typename PROCESSOR>
void processOctree(const DSOOctree& octree,
                   std::vector<DSOBrowserRecord>& records,
                   std::uint32_t size,
                   const DSOFilter& filter,
                   const Eigen::Vector3d& pos)
{
    records.clear();
    records.reserve(size);

    PROCESSOR processor(records, size, filter, pos);
    octree.processBreadthFirst(processor);
    processor.finalize();
}

} // end unnamed namespace

DSOBrowser::DSOBrowser(const Universe* universe,
                       std::uint32_t size,
                       Comparison comparison,
                       DeepSkyObjectType dsoType) :
    m_universe(universe),
    m_size(size),
    m_comparison(comparison),
    m_dsoType(dsoType)
{
    if (m_size < MinListDSOs)
        m_size = MinListDSOs;
    else if (m_size > MaxListDSOs)
        m_size = MaxListDSOs;
}

bool
DSOBrowser::size(std::uint32_t size)
{
    m_size = size;
    if (m_size < MinListDSOs)
    {
        m_size = MinListDSOs;
        return false;
    }
    if (m_size > MaxListDSOs)
    {
        m_size = MaxListDSOs;
        return false;
    }
    return true;
}

void
DSOBrowser::position(const UniversalCoord& ucPos)
{
    m_pos = ucPos.offsetFromKm(UniversalCoord::Zero()) * astro::kilometersToLightYears(1.0);
}

void
DSOBrowser::populate(std::vector<DSOBrowserRecord>& records) const
{
    records.clear();
    DSOFilter filter(m_dsoType);
    if (m_typeFilter)
        filter.setTypeFilter(m_typeFilter);

    const DSODatabase* dsoDB = m_universe->getDSOCatalog();
    if (!dsoDB)
        return;

    const DSOOctree* octree = dsoDB->getOctree();
    if (octree == nullptr)
        return;

    switch (m_comparison)
    {
    case Comparison::Nearest:
        processOctree<DistanceProcessor>(*octree, records, m_size, filter, m_pos);
        break;

    case Comparison::ApparentMagnitude:
        processOctree<AppMagProcessor>(*octree, records, m_size, filter, m_pos);
        break;

    case Comparison::AbsoluteMagnitude:
        processOctree<AbsMagProcessor>(*octree, records, m_size, filter, m_pos);
        break;

    default:
        assert(0);
        return;
    }
}

} // end namespace celestia::engine
