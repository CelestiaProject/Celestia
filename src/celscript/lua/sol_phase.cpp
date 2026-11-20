#include "sol_phase.h"

#include <algorithm> // std::clamp
#include <celengine/frametree.h>
#include <celengine/timelinephase.h>
#include <celephem/orbit.h>
#include <celephem/rotation.h>
#include <sol/sol.hpp>

using namespace std::string_view_literals;

namespace celestia::celx
{

std::tuple<double, double>
CelxPhase::timespan() const
{
    return { p->startTime(), p->endTime() };
}

ObserverFrame
CelxPhase::orbitframe() const
{
    return ObserverFrame(p->getFrameTree()->getOwner(), p->orbitFrame());
}

ObserverFrame
CelxPhase::bodyframe() const
{
    return ObserverFrame(p->body(), p->bodyFrame());
}

UniversalCoord
CelxPhase::getposition(double tdb) const
{
    tdb = std::clamp(tdb, p->startTime(), p->endTime());
    return UniversalCoord(
        p->orbit()->positionAtTime(tdb) * astro::kilometersToMicroLightYears(1.0));
}

Eigen::Quaterniond
CelxPhase::getorientation(double tdb) const
{
    tdb = std::clamp(tdb, p->startTime(), p->endTime());
    return p->rotationModel()->orientationAtTime(tdb);
}

} // namespace celestia::celx

using namespace celestia::celx;

void
CreatePhaseMetaTable(sol::state_view lua)
{
    lua.new_usertype<CelxPhase>(
        "TimelinePhase",
        sol::no_constructor,
        "__tostring",
        []() { return "[Phase]"; },
        "timespan",
        &CelxPhase::timespan,
        "orbitframe",
        &CelxPhase::orbitframe,
        "bodyframe",
        &CelxPhase::bodyframe,
        "getposition",
        &CelxPhase::getposition,
        "getorientation",
        &CelxPhase::getorientation);
}
