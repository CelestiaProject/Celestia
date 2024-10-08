// starbrowser.cpp
//
// Copyright (C) 2023, The Celestia Development Team
//
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

#include "starbrowser.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "star.h"
#include "stardb.h"
#include "univcoord.h"
#include "universe.h"

namespace celestia::engine
{

namespace
{

inline float
distanceSquared(const Star& star,
                double jd,
                const Eigen::Vector3f& pos,
                const UniversalCoord& ucPos)
{
    // For the purposes of building the list, we can use the squared distance
    // to avoid evaluating unnecessary square roots.
    float distance = (pos - star.getPosition()).squaredNorm();

    // If the stars are closer than one light year, use
    // a more precise distance estimate.
    if (distance < 1.0f)
        distance = static_cast<float>((ucPos - star.getPosition(jd)).toLy().squaredNorm());

    return distance;
}

class StarFilter
{
public:
    StarFilter(StarBrowser::Filter, const SolarSystemCatalog*);
    bool operator()(const Star&) const;

    void setSpectralTypeFilter(const std::function<bool(const char*)>&);

private:
    StarBrowser::Filter m_filter;
    const SolarSystemCatalog* m_solarSystems;
    std::function<bool(const char*)> m_spectralTypeFilter{ nullptr };
};

StarFilter::StarFilter(StarBrowser::Filter filter, const SolarSystemCatalog* solarSystems) :
    m_filter(filter), m_solarSystems(solarSystems)
{
}

void
StarFilter::setSpectralTypeFilter(const std::function<bool(const char*)>& filter)
{
    m_spectralTypeFilter = filter;
}

bool
parentHasPlanets(const SolarSystemCatalog* solarSystems, const Star* star)
{
    // When searching for visible stars only, also take planets orbiting the
    // parent barycenters into account
    const auto end = solarSystems->end();
    for (;;)
    {
        star = star->getOrbitBarycenter();
        if (star == nullptr || star->getVisibility())
            return false;

        if (solarSystems->find(star->getIndex()) != end)
            return true;
    }
}

bool
StarFilter::operator()(const Star& star) const
{
    // If ordering is done by brightness, filter out barycenters by default
    bool visibleOnly = util::is_set(m_filter, StarBrowser::Filter::Visible);
    if (visibleOnly && !star.getVisibility())
        return false;

    if (util::is_set(m_filter, StarBrowser::Filter::Multiple))
    {
        auto barycenter = star.getOrbitBarycenter();

        // Check the number of stars orbiting the barycenter to handle cases
        // like the Sun orbiting the Solar System Barycenter
        if (barycenter == nullptr || barycenter->getOrbitingStars().size() < 2)
            return false;
    }

    if (util::is_set(m_filter, StarBrowser::Filter::WithPlanets) &&
        m_solarSystems->find(star.getIndex()) == m_solarSystems->end() &&
        !(visibleOnly && parentHasPlanets(m_solarSystems, &star)))
    {
        return false;
    }

    return (util::is_set(m_filter, StarBrowser::Filter::SpectralType) && m_spectralTypeFilter)
        ? m_spectralTypeFilter(star.getSpectralType())
        : true;
}

class DistanceProcessor
{
public:
    DistanceProcessor(std::vector<StarBrowserRecord>&,
                      std::uint32_t,
                      const StarFilter&,
                      const UniversalCoord&,
                      double);

    ~DistanceProcessor() = default;

    DistanceProcessor(const DistanceProcessor&) = delete;
    DistanceProcessor& operator=(const DistanceProcessor&) = delete;
    DistanceProcessor(DistanceProcessor&&) noexcept = default;
    DistanceProcessor& operator=(DistanceProcessor&&) noexcept = default;

    bool checkNode(const Eigen::Vector3f&, float, float) const;
    void process(const Star&);
    void finalize();

private:
    static bool compare(const StarBrowserRecord&, const StarBrowserRecord&);

    const StarFilter* m_filter;
    std::vector<StarBrowserRecord>* m_records;
    double m_jd;
    UniversalCoord m_ucPos;
    Eigen::Vector3f m_pos;
    std::uint32_t m_size;
    float m_maxDistance{ std::numeric_limits<float>::max() };
};

DistanceProcessor::DistanceProcessor(std::vector<StarBrowserRecord>& records,
                                     std::uint32_t size,
                                     const StarFilter& filter,
                                     const UniversalCoord& ucPos,
                                     double jd) :
    m_filter(&filter),
    m_records(&records),
    m_jd(jd),
    m_ucPos(ucPos),
    m_pos(ucPos.toLy().cast<float>()),
    m_size(size)
{
    assert(m_size > 0);
}

bool
DistanceProcessor::compare(const StarBrowserRecord& lhs, const StarBrowserRecord& rhs)
{
    return lhs.distance < rhs.distance;
}

bool
DistanceProcessor::checkNode(const Eigen::Vector3f& center,
                             float size,
                             float /* brightestMag */) const
{
    if (m_records->size() < m_size)
        return true;

    // Compute the distance to node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * SQRT3.
    float nodeDistance = (m_pos - center).norm() - size * numbers::sqrt3_v<float>;
    return nodeDistance <= m_maxDistance;
}

void
DistanceProcessor::process(const Star& star)
{
    if (!(*m_filter)(star))
        return;

    const Star* oldWorst = m_records->empty() ? nullptr : m_records->front().star;
    auto distance2 = distanceSquared(star, m_jd, m_pos, m_ucPos);
    if (m_records->size() < m_size)
    {
        auto& record = m_records->emplace_back(&star);
        record.distance = distance2;
        std::push_heap(m_records->begin(), m_records->end(), &compare);
    }
    else if (distance2 < m_records->front().distance)
    {
        std::pop_heap(m_records->begin(), m_records->end(), &compare);
        auto& record = m_records->back();
        record.star = &star;
        record.distance = distance2;
        std::push_heap(m_records->begin(), m_records->end(), &compare);
    }
    else
    {
        return;
    }

    if (m_records->front().star != oldWorst)
        m_maxDistance = std::sqrt(distance2);
}

void
DistanceProcessor::finalize()
{
    std::sort_heap(m_records->begin(), m_records->end(), &compare);
    for (StarBrowserRecord& record : *m_records)
    {
        record.distance = std::sqrt(record.distance);
        record.appMag = record.star->getApparentMagnitude(record.distance);
    }
}

class AppMagProcessor
{
public:
    AppMagProcessor(std::vector<StarBrowserRecord>&,
                      std::uint32_t,
                      const StarFilter&,
                      const UniversalCoord&,
                      double);

    ~AppMagProcessor() = default;

    AppMagProcessor(const AppMagProcessor&) = delete;
    AppMagProcessor& operator=(const AppMagProcessor&) = delete;
    AppMagProcessor(AppMagProcessor&&) noexcept = default;
    AppMagProcessor& operator=(AppMagProcessor&&) noexcept = default;

    bool checkNode(const Eigen::Vector3f&, float, float) const;
    void process(const Star&);
    void finalize();

private:
    static bool compare(const StarBrowserRecord&, const StarBrowserRecord&);

    const StarFilter* m_filter;
    std::vector<StarBrowserRecord>* m_records;
    double m_jd;
    UniversalCoord m_ucPos;
    Eigen::Vector3f m_pos;
    std::uint32_t m_size;
};

AppMagProcessor::AppMagProcessor(std::vector<StarBrowserRecord>& records,
                                 std::uint32_t size,
                                 const StarFilter& filter,
                                 const UniversalCoord& ucPos,
                                 double jd) :
    m_filter(&filter),
    m_records(&records),
    m_jd(jd),
    m_ucPos(ucPos),
    m_pos(ucPos.toLy().cast<float>()),
    m_size(size)
{
    assert(m_size > 0);
}

bool
AppMagProcessor::compare(const StarBrowserRecord& lhs, const StarBrowserRecord& rhs)
{
    return lhs.appMag < rhs.appMag;
}

bool
AppMagProcessor::checkNode(const Eigen::Vector3f& center,
                           float size,
                           float brightestMag) const
{
    if (m_records->size() < m_size)
        return true;

    // Compute the distance to node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * SQRT3.
    float nodeDistance = (m_pos - center).norm() - size * numbers::sqrt3_v<float>;
    if (nodeDistance < 1e-3)
        return true;

    float nodeAppMag = astro::absToAppMag(brightestMag, nodeDistance);
    return nodeAppMag < m_records->front().appMag;
}

void
AppMagProcessor::process(const Star& star)
{
    if (!(*m_filter)(star))
        return;

    auto distance = std::sqrt(distanceSquared(star, m_jd, m_pos, m_ucPos));
    auto appMag = star.getApparentMagnitude(distance);
    if (m_records->size() < m_size)
    {
        auto& record = m_records->emplace_back(&star);
        record.distance = distance;
        record.appMag = appMag;
        std::push_heap(m_records->begin(), m_records->end(), &compare);
    }
    else if (appMag < m_records->front().appMag)
    {
        std::pop_heap(m_records->begin(), m_records->end(), &compare);
        auto& record = m_records->back();
        record.star = &star;
        record.distance = distance;
        record.appMag = appMag;
        std::push_heap(m_records->begin(), m_records->end(), &compare);
    }
}

void
AppMagProcessor::finalize()
{
    std::sort_heap(m_records->begin(), m_records->end(), &compare);
}

class AbsMagProcessor
{
public:
    AbsMagProcessor(std::vector<StarBrowserRecord>&,
                    std::uint32_t,
                    const StarFilter&,
                    const UniversalCoord&,
                    double);

    ~AbsMagProcessor() = default;

    AbsMagProcessor(const AbsMagProcessor&) = delete;
    AbsMagProcessor& operator=(const AbsMagProcessor&) = delete;
    AbsMagProcessor(AbsMagProcessor&&) noexcept = default;
    AbsMagProcessor& operator=(AbsMagProcessor&&) noexcept = default;

    bool checkNode(const Eigen::Vector3f&, float, float) const;
    void process(const Star&);
    void finalize();

private:
    static bool compare(const StarBrowserRecord&, const StarBrowserRecord&);

    const StarFilter* m_filter;
    std::vector<StarBrowserRecord>* m_records;
    double m_jd;
    UniversalCoord m_ucPos;
    Eigen::Vector3f m_pos;
    std::uint32_t m_size;
};

AbsMagProcessor::AbsMagProcessor(std::vector<StarBrowserRecord>& records,
                                 std::uint32_t size,
                                 const StarFilter& filter,
                                 const UniversalCoord& ucPos,
                                 double jd) :
    m_filter(&filter),
    m_records(&records),
    m_jd(jd),
    m_ucPos(ucPos),
    m_pos(ucPos.toLy().cast<float>()),
    m_size(size)
{
    assert(m_size > 0);
}

bool
AbsMagProcessor::compare(const StarBrowserRecord& lhs, const StarBrowserRecord& rhs)
{
    return lhs.star->getAbsoluteMagnitude() < rhs.star->getAbsoluteMagnitude();
}

bool
AbsMagProcessor::checkNode(const Eigen::Vector3f& /* center */,
                           float /* size */,
                           float brightestMag) const
{
    if (m_records->size() < m_size)
        return true;

    return brightestMag < m_records->front().star->getAbsoluteMagnitude();
}

void
AbsMagProcessor::process(const Star& star)
{
    if (!(*m_filter)(star))
        return;

    if (m_records->size() < m_size)
    {
        m_records->emplace_back(&star);
        std::push_heap(m_records->begin(), m_records->end(), &compare);
    }
    else if (star.getAbsoluteMagnitude() < m_records->front().star->getAbsoluteMagnitude())
    {
        std::pop_heap(m_records->begin(), m_records->end(), &compare);
        m_records->back().star = &star;
        std::push_heap(m_records->begin(), m_records->end(), &compare);
    }
}

void
AbsMagProcessor::finalize()
{
    std::sort_heap(m_records->begin(), m_records->end(), &compare);
    for (StarBrowserRecord& record : *m_records)
    {
        record.distance = std::sqrt(distanceSquared(*record.star, m_jd, m_pos, m_ucPos));
        record.appMag = record.star->getApparentMagnitude(record.distance);
    }
}

template<typename PROCESSOR>
void processOctree(const StarOctree& octree,
                   std::vector<StarBrowserRecord>& records,
                   std::uint32_t size,
                   const StarFilter& filter,
                   const UniversalCoord& ucPos,
                   double jd)
{
    records.clear();
    records.reserve(size);

    PROCESSOR processor(records, size, filter, ucPos, jd);
    octree.processBreadthFirst(processor);
    processor.finalize();
}

} // end unnamed namespace

StarBrowser::StarBrowser(const Universe* universe,
                         std::uint32_t size,
                         Comparison comparison,
                         Filter filter) :
    m_universe(universe),
    m_size(size),
    m_comparison(comparison),
    m_filter(filter)
{
    if (m_size < MinListStars)
        m_size = MinListStars;
    else if (m_size > MaxListStars)
        m_size = MaxListStars;
}

bool
StarBrowser::setSize(std::uint32_t size)
{
    m_size = size;
    if (m_size < MinListStars)
    {
        m_size = MinListStars;
        return false;
    }
    if (m_size > MaxListStars)
    {
        m_size = MaxListStars;
        return false;
    }
    return true;
}

void
StarBrowser::setPosition(const UniversalCoord& ucPos)
{
    m_ucPos = ucPos;
}

void
StarBrowser::populate(std::vector<StarBrowserRecord>& records) const
{
    StarFilter filter(m_filter, m_universe->getSolarSystemCatalog());
    if (m_spectralTypeFilter)
        filter.setSpectralTypeFilter(m_spectralTypeFilter);
    const StarDatabase* stardb = m_universe->getStarCatalog();
    if (stardb == nullptr)
        return;

    const StarOctree* octree = stardb->getOctree();
    if (octree == nullptr)
        return;

    switch (m_comparison)
    {
    case Comparison::Nearest:
        processOctree<DistanceProcessor>(*octree, records, m_size, filter, m_ucPos, m_jd);
        return;

    case Comparison::ApparentMagnitude:
        processOctree<AppMagProcessor>(*octree, records, m_size, filter, m_ucPos, m_jd);
        return;

    case Comparison::AbsoluteMagnitude:
        processOctree<AbsMagProcessor>(*octree, records, m_size, filter, m_ucPos, m_jd);
        return;

    default:
        assert(0);
        return;
    }
}

} // end namespace celestia::engine
