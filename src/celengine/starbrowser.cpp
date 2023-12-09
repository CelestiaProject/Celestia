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
distanceSquared(const Star* star,
                double jd,
                const Eigen::Vector3f& pos,
                const UniversalCoord& ucPos)
{
    // For the purposes of building the list, we can use the squared distance
    // to avoid evaluating unnecessary square roots.
    float distance = (pos - star->getPosition()).squaredNorm();

    // If the stars are closer than one light year, use
    // a more precise distance estimate.
    if (distance < 1.0f)
        distance = static_cast<float>((ucPos - star->getPosition(jd)).toLy().squaredNorm());

    return distance;
}

class DistanceComparison
{
public:
    DistanceComparison(double jd, const Eigen::Vector3f& pos, const UniversalCoord& ucPos);

    StarBrowserRecord createRecord(const Star*) const;
    void finalizeRecord(StarBrowserRecord&) const;

    bool operator()(const StarBrowserRecord&, const StarBrowserRecord&) const;

private:
    double m_jd;
    Eigen::Vector3f m_pos;
    UniversalCoord m_ucPos;
};

DistanceComparison::DistanceComparison(double jd, const Eigen::Vector3f& pos, const UniversalCoord& ucPos) :
    m_jd(jd), m_pos(pos), m_ucPos(ucPos)
{
}

StarBrowserRecord
DistanceComparison::createRecord(const Star* star) const
{
    StarBrowserRecord result(star);
    result.distance = distanceSquared(star, m_jd, m_pos, m_ucPos);
    return result;
}

void
DistanceComparison::finalizeRecord(StarBrowserRecord& record) const
{
    record.distance = std::sqrt(record.distance);
    record.appMag = record.star->getApparentMagnitude(record.distance);
}

bool
DistanceComparison::operator()(const StarBrowserRecord& lhs, const StarBrowserRecord& rhs) const
{
    return lhs.distance < rhs.distance;
}

class AppMagComparison
{
public:
    AppMagComparison(double jd, const Eigen::Vector3f& pos, const UniversalCoord& ucPos);

    StarBrowserRecord createRecord(const Star*) const;
    void finalizeRecord(const StarBrowserRecord&) const { /* no-op */ }

    bool operator()(const StarBrowserRecord&, const StarBrowserRecord&) const;

private:
    double m_jd;
    Eigen::Vector3f m_pos;
    UniversalCoord m_ucPos;
};

AppMagComparison::AppMagComparison(double jd, const Eigen::Vector3f& pos, const UniversalCoord& ucPos) :
    m_jd(jd), m_pos(pos), m_ucPos(ucPos)
{
}

StarBrowserRecord
AppMagComparison::createRecord(const Star* star) const
{
    StarBrowserRecord result(star);
    result.distance = std::sqrt(distanceSquared(star, m_jd, m_pos, m_ucPos));
    result.appMag = result.star->getApparentMagnitude(result.distance);
    return result;
}

bool
AppMagComparison::operator()(const StarBrowserRecord& lhs, const StarBrowserRecord& rhs) const
{
    return lhs.appMag < rhs.appMag;
}

class AbsMagComparison
{
public:
    AbsMagComparison(double jd, const Eigen::Vector3f& pos, const UniversalCoord& ucPos);

    StarBrowserRecord createRecord(const Star*) const;
    void finalizeRecord(StarBrowserRecord&) const;

    bool operator()(const StarBrowserRecord&, const StarBrowserRecord&) const;

private:
    double m_jd;
    Eigen::Vector3f m_pos;
    UniversalCoord m_ucPos;
};

AbsMagComparison::AbsMagComparison(double jd, const Eigen::Vector3f& pos, const UniversalCoord& ucPos) :
    m_jd(jd), m_pos(pos), m_ucPos(ucPos)
{
}

StarBrowserRecord
AbsMagComparison::createRecord(const Star* star) const
{
    return StarBrowserRecord(star);
}

void
AbsMagComparison::finalizeRecord(StarBrowserRecord& record) const
{
    record.distance = std::sqrt(distanceSquared(record.star, m_jd, m_pos, m_ucPos));
    record.appMag = record.star->getApparentMagnitude(record.distance);
}

bool
AbsMagComparison::operator()(const StarBrowserRecord& lhs, const StarBrowserRecord& rhs) const
{
    return lhs.star->getAbsoluteMagnitude() < rhs.star->getAbsoluteMagnitude();
}

class StarFilter
{
public:
    StarFilter(StarBrowser::Filter, const SolarSystemCatalog*);
    bool operator()(const Star*) const;

    void setFilter(StarBrowser::Filter filter);
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
StarFilter::setFilter(StarBrowser::Filter filter)
{
    m_filter = filter;
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
StarFilter::operator()(const Star* star) const
{
    // If ordering is done by brightness, filter out barycenters by default
    bool visibleOnly = util::is_set(m_filter, StarBrowser::Filter::Visible);
    if (visibleOnly && !star->getVisibility())
        return false;

    if (util::is_set(m_filter, StarBrowser::Filter::Multiple))
    {
        auto barycenter = star->getOrbitBarycenter();

        // Check the number of stars orbiting the barycenter to handle cases
        // like the Sun orbiting the Solar System Barycenter
        if (barycenter == nullptr || barycenter->getOrbitingStars()->size() < 2)
            return false;
    }

    if (util::is_set(m_filter, StarBrowser::Filter::WithPlanets) &&
        m_solarSystems->find(star->getIndex()) == m_solarSystems->end() &&
        !(visibleOnly && parentHasPlanets(m_solarSystems, star)))
    {
        return false;
    }

    return (util::is_set(m_filter, StarBrowser::Filter::SpectralType) && m_spectralTypeFilter)
        ? m_spectralTypeFilter(star->getSpectralType())
        : true;
}

template<typename C>
void populateRecords(std::vector<StarBrowserRecord>& records,
                     const C& comparison,
                     const StarFilter& filter,
                     std::uint32_t size,
                     const StarDatabase& stardb)
{
    records.clear();
    if (size == 0)
        return;

    records.reserve(size);

    std::uint32_t totalStars = stardb.size();
    if (totalStars == 0)
        return;

    records.reserve(size);
    std::uint32_t index = 0;

    // Add all stars that match the filter until we fill up the list
    for (;;)
    {
        if (index == totalStars)
        {
            std::sort(records.begin(), records.end(), comparison);
            for (StarBrowserRecord& record : records)
            {
                comparison.finalizeRecord(record);
            }
            return;
        }

        if (const Star* star = stardb.getStar(index); filter(star))
        {
            if (records.size() == size)
                break;

            records.push_back(comparison.createRecord(star));
        }

        ++index;
    }

    // We have filled up the number of requested stars, so only add stars that
    // are better than the worst star in the list according to the predicate.
    std::make_heap(records.begin(), records.end(), comparison);

    for (; index < totalStars; ++index)
    {
        const Star* star = stardb.getStar(index);
        if (!filter(star))
            continue;

        StarBrowserRecord record = comparison.createRecord(star);
        if (comparison(record, records.front()))
        {
            std::pop_heap(records.begin(), records.end(), comparison);
            records.back() = record;
            std::push_heap(records.begin(), records.end(), comparison);
        }
    }

    std::sort_heap(records.begin(), records.end(), comparison);
    for (StarBrowserRecord& record : records)
    {
        comparison.finalizeRecord(record);
    }
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
    m_pos = m_ucPos.toLy().cast<float>();
}

void
StarBrowser::populate(std::vector<StarBrowserRecord>& records) const
{
    StarFilter filter(m_filter, m_universe->getSolarSystemCatalog());
    const auto stardb = m_universe->getStarCatalog();
    if (stardb == nullptr)
        return;

    switch (m_comparison)
    {
    case Comparison::Nearest:
        populateRecords(records, DistanceComparison(m_jd, m_pos, m_ucPos), filter, m_size, *stardb);
        return;
    case Comparison::ApparentMagnitude:
        populateRecords(records, AppMagComparison(m_jd, m_pos, m_ucPos), filter, m_size, *stardb);
        return;
    case Comparison::AbsoluteMagnitude:
        populateRecords(records, AbsMagComparison(m_jd, m_pos, m_ucPos), filter, m_size, *stardb);
        return;
    default:
        assert(0);
        return;
    }
}

} // end namespace celestia::engine
