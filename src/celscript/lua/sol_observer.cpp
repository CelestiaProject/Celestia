#include <string_view>

#define SOL_ALL_SAFETIES_ON 1
#include "celx_internal.h" // TODO
#include "sol_observer.h"

#include <celestia/celestiacore.h>
#include <celutil/logger.h>
#include <sol/sol.hpp>

using namespace std::string_view_literals;
using celestia::util::GetLogger;

ObserverFrame::CoordinateSystem parseCoordSys(std::string_view); // celx_celestia.cpp

namespace
{
// TODO: move to own file, rewrite without CelxLua
CelestiaCore *
getAppCore(sol::this_state s)
{
    return CelxLua(s).appCore(AllErrors);
}

template<typename T>
T
in_limits_or(const sol::optional<T> &val, const T &lo, const T &hi, const T &def)
{
    T v = val.value_or(def);
    return (v < lo || v > hi) ? def : v;
}

} // namespace

namespace celestia::celx
{

void
CelxObserver::goto_table(const sol::table &table)
{
    Observer::JourneyParams jparams;
    jparams.duration           = Observer::JourneyDuration;
    jparams.from               = o->getPosition();
    jparams.to                 = o->getPosition();
    jparams.initialOrientation = o->getOrientation();
    jparams.finalOrientation   = o->getOrientation();
    jparams.startInterpolation = Observer::StartInterpolation;
    jparams.endInterpolation   = Observer::EndInterpolation;
    jparams.accelTime          = Observer::AccelerationTime;
    jparams.traj               = Observer::TrajectoryType::Linear;

    sol::optional<double> t = table["duration"];
    jparams.duration        = std::max(0.0, t.value_or(Observer::JourneyDuration));

    if (sol::optional<UniversalCoord> from = table["from"]; from.has_value())
        jparams.from = from.value();

    if (sol::optional<UniversalCoord> to = table["to"]; to.has_value())
        jparams.to = to.value();

    if (sol::optional<Eigen::Quaterniond> orientation = table["initialOrientation"];
        orientation.has_value())
        jparams.initialOrientation = orientation.value();

    if (sol::optional<Eigen::Quaterniond> orientation = table["finalOrientation"];
        orientation.has_value())
        jparams.finalOrientation = orientation.value();

    t                          = table["startInterpolation"];
    jparams.startInterpolation = std::clamp(t.value_or(Observer::StartInterpolation), 0.0, 1.0);

    t                        = table["endInterpolation"];
    jparams.endInterpolation = std::clamp(t.value_or(Observer::EndInterpolation), 0.0, 1.0);

    t                 = table["accelTime"];
    jparams.accelTime = std::clamp(t.value_or(Observer::AccelerationTime), 0.1, 1.0);

    // args are in universal coords, let setFrame handle conversion:
    auto frame = o->getFrame();
    o->setFrame(ObserverFrame::CoordinateSystem::Universal, Selection());
    o->gotoJourney(jparams);
    o->setFrame(frame);
}

// First argument is the target object or position; optional second argument
// is the travel time
void
CelxObserver::goto_object(
    const Selection      &sel,
    sol::optional<double> travelTime,
    sol::optional<double> startInter,
    sol::optional<double> endInter,
    sol::optional<double> accelTime)
{
    o->gotoSelection(
        sel,
        travelTime.value_or(Observer::JourneyDuration),
        in_limits_or(startInter, 0.0, 1.0, Observer::StartInterpolation),
        in_limits_or(endInter, 0.0, 1.0, Observer::EndInterpolation),
        accelTime.value_or(Observer::AccelerationTime),
        Eigen::Vector3f::UnitY(),
        ObserverFrame::CoordinateSystem::ObserverLocal);
}

void
CelxObserver::goto_position(const UniversalCoord &p, sol::optional<double> travelTime)
{
    o->gotoLocation(p, o->getOrientation(), travelTime.value_or(Observer::JourneyDuration));
}

void
CelxObserver::gotolonglat(
    const Selection       &sel,
    double                 longitude,
    double                 latitude,
    double                 distance,
    double                 travelTime,
    const Eigen::Vector3d &up)
{
    o->gotoSelectionLongLat(
        sel,
        travelTime,
        distance,
        static_cast<float>(longitude),
        static_cast<float>(latitude),
        up.cast<float>());
}

void
CelxObserver::gotolonglat1(
    const Selection      &sel,
    sol::optional<double> longitude,
    sol::optional<double> latitude,
    sol::optional<double> distance,
    sol::optional<double> travelTime)
{
    o->gotoSelectionLongLat(
        sel,
        travelTime.value_or(5.0),
        distance.value_or(sel.radius() * 5.0),
        static_cast<float>(longitude.value_or(0.0)),
        static_cast<float>(latitude.value_or(0.0)),
        Eigen::Vector3f::UnitY());
}

void
CelxObserver::gotolocation(
    const UniversalCoord             &p,
    sol::optional<double>             travelTime,
    sol::optional<Eigen::Quaterniond> rot)
{
    Eigen::Quaterniond orientation = rot.has_value() ? rot.value() : o->getOrientation();

    double t = std::max(0.0, travelTime.value_or(5.0));
    o->gotoLocation(p, orientation, t);
}

void
CelxObserver::gotodistance(
    const Selection                &sel,
    double                          distance,
    double                          travelTime,
    const Eigen::Vector3d          &up,
    sol::optional<std::string_view> upFrameName)
{
    auto upFrame = ObserverFrame::CoordinateSystem::Universal;
    if (upFrameName.has_value())
        upFrame = parseCoordSys(upFrameName.value());

    o->gotoSelection(sel, travelTime, distance, up.cast<float>(), upFrame);
}

void
CelxObserver::gotodistance1(
    const Selection      &sel,
    sol::optional<double> distance,
    sol::optional<double> travelTime)
{
    gotodistance(
        sel,
        distance.value_or(20000.0),
        travelTime.value_or(5.0),
        Eigen::Vector3d::UnitY(),
        {});
}

void
CelxObserver::gotosurface(const Selection &sel, sol::optional<double> travelTime)
{
    // This is needed because gotoSurface expects frame to be geosync:
    o->geosynchronousFollow(sel);
    o->gotoSurface(sel, travelTime.value_or(5.0));
}

void
CelxObserver::lookat1(const UniversalCoord &to, const Eigen::Vector3d &upd)
{
    lookat(to.offsetFromKm(o->getPosition()), upd);
}

void
CelxObserver::lookat2(
    const UniversalCoord  &from,
    const UniversalCoord  &to,
    const Eigen::Vector3d &upd)
{
    lookat(to.offsetFromKm(from), upd);
}

void
CelxObserver::setfov(double fov, sol::optional<bool> syncZoom, sol::this_state s)
{
    if ((fov >= 0.001_deg) && (fov <= 120.0_deg))
    {
        o->setFOV(static_cast<float>(fov));
        if (syncZoom.value_or(true))
            getAppCore(s)->setZoomFromFOV();
    }
}

void
CelxObserver::changeorbitdistance(double d, sol::this_state s)
{
    o->changeOrbitDistance(getAppCore(s)->getSimulation()->getSelection(), static_cast<float>(d));
}

void
CelxObserver::center(const Selection &sel, sol::optional<double> travelTime)
{
    o->centerSelection(sel, travelTime.value_or(5.0));
}

void
CelxObserver::centerorbit(const Selection &sel, sol::optional<double> travelTime)
{
    o->centerSelectionCO(sel, travelTime.value_or(5.0));
}

void
CelxObserver::untrack()
{
    o->setTrackedObject(Selection());
}

void
CelxObserver::track(const Selection &sel)
{
    o->setTrackedObject(sel);
}

void
CelxObserver::setsurface(sol::optional<std::string_view> s)
{
    o->setDisplayedSurface(s.value_or(std::string_view()));
}

void
CelxObserver::splitview(
    std::string_view      splitType,
    sol::optional<double> splitPos,
    sol::this_state       s) const
{
    auto                *appCore = getAppCore(s);
    celestia::View::Type type    = (compareIgnoringCase(splitType, "h"sv) == 0) ?
                                       celestia::View::HorizontalSplit :
                                       celestia::View::VerticalSplit;
    double               split   = std::clamp(splitPos.value_or(0.5), 0.1, 0.9);
    auto                *view    = getViewByObserver(appCore, o);
    appCore->splitView(type, view, static_cast<float>(split));
}

void
CelxObserver::deleteview(sol::this_state s) const
{
    auto *appCore = getAppCore(s);
    auto *view    = getViewByObserver(appCore, o);
    appCore->deleteView(view);
}

void
CelxObserver::singleview(sol::this_state s) const
{
    auto *appCore = getAppCore(s);
    auto *view    = getViewByObserver(appCore, o);
    appCore->singleView(view);
}

void
CelxObserver::makeactiveview(sol::this_state s) const
{
    auto *appCore = getAppCore(s);
    auto *view    = getViewByObserver(appCore, o);
    appCore->setActiveView(view);
}

sol::table
CelxObserver::getlocationflags(sol::this_state s) const
{
    auto  locationFlags   = o->getLocationFilter();
    auto &LocationFlagMap = getAppCore(s)->scriptMaps().LocationFlagMap;
    auto  table           = sol::state_view(s).create_table();
    for (const auto &it : LocationFlagMap) table[it.first] = (it.second & locationFlags) != 0;
    return table;
}

void
CelxObserver::setlocationflags(sol::table table, sol::this_state s)
{
    auto  locationFlags   = o->getLocationFilter();
    auto &LocationFlagMap = getAppCore(s)->scriptMaps().LocationFlagMap;

    for (const auto &pair : table)
    {
        if (pair.first.get_type() != sol::type::string)
        {
            // celx.doError("Keys in table-argument to observer:setlocationflags() must be
            // strings"); // FIXME
            return;
        }
        if (pair.second.get_type() != sol::type::boolean)
        {
            // celx.doError("Values in table-argument to observer:setlocationflags() must be
            // boolean"); // FIXME
            return;
        }
        std::string_view key   = pair.first.as<std::string_view>();
        bool             value = pair.second.as<bool>();
        if (LocationFlagMap.count(key) != 0)
        {
            auto flag = LocationFlagMap[key];
            if (value)
                locationFlags |= flag;
            else
                locationFlags &= ~flag;
        }
        else
        {
            GetLogger()->warn("Unknown key: {}\n", key);
        }
    }
    o->setLocationFilter(locationFlags);
}

bool
CelxObserver::isvalid() const
{
    return o != nullptr;
}

void
CelxObserver::cancelgoto()
{
    o->cancelMotion();
}

void
CelxObserver::setposition(const UniversalCoord &p)
{
    o->setPosition(p);
}

void
CelxObserver::setorientation(const Eigen::Quaterniond &r)
{
    o->setOrientation(r);
}

Eigen::Quaterniond
CelxObserver::getorientation() const
{
    return o->getOrientation();
}

float
CelxObserver::getspeed() const
{
    return astro::kilometersToMicroLightYears(o->getTargetSpeed());
}

void
CelxObserver::setspeed(double speed)
{
    o->setTargetSpeed(static_cast<float>(astro::microLightYearsToKilometers(speed)));
}

float
CelxObserver::getfov() const
{
    return o->getFOV();
}

void
CelxObserver::rotate(const Eigen::Quaterniond &r)
{
    o->rotate(r.cast<float>());
}

void
CelxObserver::orbit(const Eigen::Quaterniond &r)
{
    o->orbit(Selection(), r.cast<float>());
}

void
CelxObserver::follow(const Selection &sel)
{
    o->follow(sel);
}

void
CelxObserver::synchronous(const Selection &sel)
{
    o->geosynchronousFollow(sel);
}

void
CelxObserver::chase(const Selection &sel)
{
    o->chase(sel);
}

void
CelxObserver::lock(const Selection &sel)
{
    o->phaseLock(sel);
}

Selection
CelxObserver::gettrackedobject() const
{
    return o->getTrackedObject();
}

// Return true if the observer is still moving as a result of a goto, center,
// or similar command.
bool
CelxObserver::travelling() const
{
    return o->getMode() == Observer::ObserverMode::Travelling;
}

ObserverFrame
CelxObserver::getframe() const
{
    return *o->getFrame();
}

void
CelxObserver::setframe(const ObserverFrame &f)
{
    // o->setFrame(&f.self); // FIXME: std::make_shared<ObserverFrame>(
}

double
CelxObserver::gettime() const
{
    return o->getTime();
}

UniversalCoord
CelxObserver::getposition() const
{
    return o->getPosition();
}

std::string_view
CelxObserver::getsurface() const
{
    return o->getDisplayedSurface();
}

bool
CelxObserver::eq(const CelxObserver &other) const
{
    return o == other.o;
}

void
CelxObserver::lookat(const Eigen::Vector3d &forwardd, const Eigen::Vector3d &upd)
{
    Eigen::Vector3f up      = upd.cast<float>();
    Eigen::Vector3f forward = forwardd.cast<float>().normalized();
    Eigen::Vector3f side    = forward.cross(up).normalized();
    // Recompute up as: up = side x forward
    up = side.cross(forward);
    Eigen::Matrix3f m;
    m.row(0) = side;
    m.row(1) = up;
    m.row(2) = -forward;

    o->setOrientation(Eigen::Quaternionf(m));
}

} // namespace celestia::celx

using namespace celestia::celx;

void
CreateObserverMetaTable(sol::state_view lua)
{
    lua.new_usertype<CelxObserver>(
        "Observer",
        sol::no_constructor,
        "__tostring",
        []() { return "[Observer]"; },
        "isvalid",
        &CelxObserver::isvalid,
#if LUA_VERSION_NUM < 502
        "goto",
        sol::overload(
            &CelxObserver::goto_table,
            &CelxObserver::goto_object,
            &CelxObserver::goto_position),
#endif
        "gotoobject",
        sol::overload(
            &CelxObserver::goto_table,
            &CelxObserver::goto_object,
            &CelxObserver::goto_position),
        "gotolonglat",
        sol::overload(&CelxObserver::gotolonglat, &CelxObserver::gotolonglat1),
        "gotolocation",
        &CelxObserver::gotolocation,
        "gotodistance",
        sol::overload(&CelxObserver::gotodistance, &CelxObserver::gotodistance1),
        "gotosurface",
        &CelxObserver::gotosurface,
        "cancelgoto",
        &CelxObserver::cancelgoto,
        "setposition",
        &CelxObserver::setposition,
        "lookat",
        sol::overload(&CelxObserver::lookat1, &CelxObserver::lookat2),
        "setorientation",
        &CelxObserver::setorientation,
        "getorientation",
        &CelxObserver::getorientation,
        "getspeed",
        &CelxObserver::getspeed,
        "setspeed",
        &CelxObserver::setspeed,
        "getfov",
        &CelxObserver::getfov,
        "setfov",
        &CelxObserver::setfov,
        "rotate",
        &CelxObserver::rotate,
        "orbit",
        &CelxObserver::orbit,
        "changeorbitdistance",
        &CelxObserver::changeorbitdistance,
        "center",
        &CelxObserver::center,
        "centerorbit",
        &CelxObserver::centerorbit,
        "follow",
        &CelxObserver::follow,
        "synchronous",
        &CelxObserver::synchronous,
        "chase",
        &CelxObserver::chase,
        "lock",
        &CelxObserver::lock,
        "track",
        sol::overload(&CelxObserver::track, &CelxObserver::untrack),
        "gettrackedobject",
        &CelxObserver::gettrackedobject,
        "travelling",
        &CelxObserver::travelling,
        "getframe",
        &CelxObserver::getframe,
        "setframe",
        &CelxObserver::setframe,
        "gettime",
        &CelxObserver::gettime,
        "getposition",
        &CelxObserver::getposition,
        "getsurface",
        &CelxObserver::getsurface,
        "setsurface",
        &CelxObserver::setsurface,
        "splitview",
        &CelxObserver::splitview,
        "deleteview",
        &CelxObserver::deleteview,
        "singleview",
        &CelxObserver::singleview,
        "makeactiveview",
        &CelxObserver::makeactiveview,
        "getlocationflags",
        &CelxObserver::getlocationflags,
        "setlocationflags",
        &CelxObserver::setlocationflags,
        "__eq",
        &CelxObserver::eq);
}
