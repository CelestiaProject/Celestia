// timeline.cpp
//
// Object timelines.
//
// Copyright (C) 2008, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "timeline.h"

#include <utility>

#include "frametree.h"

bool
Timeline::appendPhase(std::unique_ptr<TimelinePhase>&& phase)
{
    // Validate start and end times. If there are existing phases in the timeline,
    // startTime must be equal to endTime of the previous phases so that there are
    // no gaps and no overlaps.
    if (!phases.empty())
    {
        if (phase->startTime() != phases.back()->endTime())
            return false;
    }

    phases.emplace_back(std::move(phase));
    return true;
}

const TimelinePhase&
Timeline::findPhase(double t) const
{
    // Find the phase containing time t. The overwhelming common case is
    // nPhases = 1, so we special case that. Otherwise, we do a simple linear search,
    // as the number of phases in a timeline should always be quite small.
    if (phases.size() == 1)
    {
        return *phases[0];
    }
    else
    {
        for (const auto& phase : phases)
        {
            if (t < phase->endTime())
                return *phase;
        }

        // Time is greater than the end time of the final phase. Just return the final phase.
        return *phases.back();
    }
}

/*! Get the phase at the specified index.
 */
const TimelinePhase&
Timeline::getPhase(unsigned int n) const
{
    assert(n < phases.size());
    return *phases[n];
}

/*! Get the number of phases in this timeline.
 */
unsigned int
Timeline::phaseCount() const
{
    return phases.size();
}

double
Timeline::startTime() const
{
    return phases.front()->startTime();
}

double
Timeline::endTime() const
{
    return phases.back()->endTime();
}

/*! Check whether the timeline covers the specified time t. True if
 *  startTime <= t <= endTime. Note that this is deliberately different
 *  than the TimelinePhase::includes function, which is only true if
 *  t is strictly less than the end time.
 */
bool
Timeline::includes(double t) const
{
    return phases.front()->startTime() <= t && t <= phases.back()->endTime();
}

// We don't want this to be a non-const function as it is actually modifying stuff
void
Timeline::markChanged() //NOSONAR
{
    for (const auto& phase : phases)
    {
        if (auto frameTree = phase->getFrameTree(); frameTree)
            frameTree->markChanged();
    }
}
