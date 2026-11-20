#include "celx_internal.h" // TODO

#include <Eigen/Geometry>
#include <celengine/observer.h>
#include <celestia/celestiacore.h>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

using namespace std::string_view_literals;

namespace
{

UniversalCoord
to_position(
    const ObserverFrame  &f,
    const UniversalCoord &p,
    sol::optional<double> jd,
    sol::this_state       s)
{
    auto  *appCore = CelxLua(s).appCore(AllErrors);
    double t       = jd.value_or(appCore->getSimulation()->getTime());
    return f.convertFromUniversal(p, t);
}

Eigen::Quaterniond
to_rotation(
    const ObserverFrame      &f,
    const Eigen::Quaterniond &r,
    sol::optional<double>     jd,
    sol::this_state           s)
{
    auto  *appCore = CelxLua(s).appCore(AllErrors);
    double t       = jd.value_or(appCore->getSimulation()->getTime());
    return f.convertFromUniversal(r, t);
}

UniversalCoord
from_position(
    const ObserverFrame  &f,
    const UniversalCoord &p,
    sol::optional<double> jd,
    sol::this_state       s)
{
    auto  *appCore = CelxLua(s).appCore(AllErrors);
    double t       = jd.value_or(appCore->getSimulation()->getTime());
    return f.convertToUniversal(p, t);
}

Eigen::Quaterniond
from_rotation(
    const ObserverFrame      &f,
    const Eigen::Quaterniond &r,
    sol::optional<double>     jd,
    sol::this_state           s)
{
    auto  *appCore = CelxLua(s).appCore(AllErrors);
    double t       = jd.value_or(appCore->getSimulation()->getTime());
    return f.convertToUniversal(r, t);
}

const char *
getcoordinatesystem(const ObserverFrame &f)
{
    switch (f.getCoordinateSystem())
    {
    case ObserverFrame::CoordinateSystem::Universal:
        return "universal";
    case ObserverFrame::CoordinateSystem::Ecliptical:
        return "ecliptic";
    case ObserverFrame::CoordinateSystem::Equatorial:
        return "equatorial";
    case ObserverFrame::CoordinateSystem::BodyFixed:
        return "bodyfixed";
    case ObserverFrame::CoordinateSystem::ObserverLocal:
        return "observer";
    case ObserverFrame::CoordinateSystem::PhaseLock:
        return "lock";
    case ObserverFrame::CoordinateSystem::Chase:
        return "chase";
    default:
        return "invalid";
    }
}

sol::optional<Selection>
getrefobject(const ObserverFrame &f)
{
    if (auto sel = f.getRefObject(); sel.getType() != SelectionType::None)
        return sel;

    return {};
}

sol::optional<Selection>
gettargetobject(const ObserverFrame &f)
{
    if (auto sel = f.getTargetObject(); sel.getType() != SelectionType::None)
        return sel;
    return {};
}

} // namespace

void
CreateFrameMetaTable(sol::state_view lua)
{
    lua.new_usertype<ObserverFrame>(
        "ObserverFrame",
        sol::no_constructor,
        // TODO: print out the actual information about the frame
        "__tostring",
        []() { return "[Frame]"; },
        // Explicitly call the destructor since the object was created with placement new
        "__gc",
        [](ObserverFrame &f) { std::destroy_at(&f); },
        // Convert from universal to frame coordinates.
        "to",
        sol::overload(to_position, to_rotation),
        // Convert from frame coordinates to universal.
        "from",
        sol::overload(from_position, from_rotation),
        "getcoordinatesystem",
        getcoordinatesystem,
        "getrefobject",
        getrefobject,
        "gettargetobject",
        gettargetobject);
}
