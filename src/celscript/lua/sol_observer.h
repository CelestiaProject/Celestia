#include <Eigen/Geometry>
#include <string_view>

#define SOL_ALL_SAFETIES_ON 1
#include <celengine/observer.h>
#include <sol/forward.hpp>

namespace celestia::celx
{

class CelxObserver
{
public:
    CelxObserver(Observer *o) : o(o)
    {
    }

    Observer *get() const
    {
        return o;
    }

    void goto_table(const sol::table &table);

    // First argument is the target object or position; optional second argument
    // is the travel time
    void goto_object(
        const Selection      &sel,
        sol::optional<double> travelTime,
        sol::optional<double> startInter,
        sol::optional<double> endInter,
        sol::optional<double> accelTime);

    void goto_position(const UniversalCoord &p, sol::optional<double> travelTime);

    void gotolonglat(
        const Selection       &sel,
        double                 longitude,
        double                 latitude,
        double                 distance,
        double                 travelTime,
        const Eigen::Vector3d &up);

    void gotolonglat1(
        const Selection      &sel,
        sol::optional<double> longitude,
        sol::optional<double> latitude,
        sol::optional<double> distance,
        sol::optional<double> travelTime);

    // observer:gotolocation(position [, duration [, rotation]])
    //
    // Travels to `position` over `duration` seconds (default 5), optionally
    // also rotating to a final `rotation`. Equivalent to the legacy gotoloc{}
    // command, including its xrot/yrot/zrot Euler form (build the rotation
    // script-side as an intrinsic-XYZ chain of celestia:newrotation factories).
    //
    // NOTE: `position` is interpreted in the observer's *current* reference
    // frame, not in universal coordinates -- this matches the legacy gotoloc
    // command, where Observer::gotoLocation assigns the input straight to
    // journey.to without converting from universal. In practice that means
    // scripts must explicitly put the observer in a known frame before
    // calling -- e.g. obs:setframe(celestia:newframe("universal")) -- when
    // the position is meant as an absolute coordinate. (.cel scripts get
    // this for free because the legacy cancel{} command resets the frame to
    // Universal as part of its work; obs:cancelgoto() does NOT, it only
    // stops motion.) The table form obs:goto{to=...} avoids the gotcha by
    // switching to universal internally before running the journey.
    void gotolocation(
        const UniversalCoord             &p,
        sol::optional<double>             travelTime,
        sol::optional<Eigen::Quaterniond> rot);

    // observer:gotodistance(target, distance [, time [, up [, upframe]]])
    //
    // `upframe` is an optional coordinate-system name ("universal",
    // "observer", "ecliptic", "equatorial", "bodyfixed", "lock", "chase")
    // describing the frame the `up` vector is expressed in. The default
    // remains "universal" for backward compatibility, but ports of the
    // legacy goto{} command should pass "observer" to match its default
    // (parseGotoCommand sets upFrame = ObserverLocal).
    void gotodistance(
        const Selection                &sel,
        double                          distance,
        double                          travelTime,
        const Eigen::Vector3d          &up,
        sol::optional<std::string_view> upFrameName);

    // second function introduced because of Sol cannot handle functions
    // with sol::optional<Eigen::Vector3d>
    void gotodistance1(
        const Selection      &sel,
        sol::optional<double> distance,
        sol::optional<double> travelTime);

    void gotosurface(const Selection &sel, sol::optional<double> travelTime);

    void lookat1(const UniversalCoord &to, const Eigen::Vector3d &upd);

    void lookat2(const UniversalCoord &from, const UniversalCoord &to, const Eigen::Vector3d &upd);

    // observer:setfov(fov [, syncZoom])
    //
    // Sets the observer's field of view (radians). When `syncZoom` is true
    // (the default), also runs CelestiaCore::setZoomFromFOV so each view's
    // zoom field is recomputed from its FOV via the projection mode -- this
    // is the long-standing celx behavior. Pass `false` to skip the sync,
    // matching the legacy `set{name "FOV"}` cel command which only writes
    // Observer::fov and leaves Observer::zoom alone.
    void setfov(double fov, sol::optional<bool> syncZoom, sol::this_state s);

    // observer:changeorbitdistance(d)
    //
    // Exponential dolly toward (d > 0) or away from (d < 0) the current
    // selection.
    void changeorbitdistance(double d, sol::this_state s);

    void center(const Selection &sel, sol::optional<double> travelTime);

    void centerorbit(const Selection &sel, sol::optional<double> travelTime);

    void untrack();

    void track(const Selection &sel);

    void setsurface(sol::optional<std::string_view> s);

    void
    splitview(std::string_view splitType, sol::optional<double> splitPos, sol::this_state s) const;

    void deleteview(sol::this_state s) const;

    void singleview(sol::this_state s) const;

    void makeactiveview(sol::this_state s) const;

    sol::table getlocationflags(sol::this_state s) const;

    void setlocationflags(sol::table table, sol::this_state s);

    bool isvalid() const;

    void cancelgoto();

    void setposition(const UniversalCoord &p);

    void setorientation(const Eigen::Quaterniond &r);

    Eigen::Quaterniond getorientation() const;

    float getspeed() const;

    void setspeed(double speed);

    float getfov() const;

    void rotate(const Eigen::Quaterniond &r);

    void orbit(const Eigen::Quaterniond &r);

    void follow(const Selection &sel);

    void synchronous(const Selection &sel);

    void chase(const Selection &sel);

    void lock(const Selection &sel);

    Selection gettrackedobject() const;

    // Return true if the observer is still moving as a result of a goto, center,
    // or similar command.
    bool travelling() const;

    ObserverFrame getframe() const;

    void setframe(const ObserverFrame &f);

    double gettime() const;

    UniversalCoord getposition() const;

    std::string_view getsurface() const;

    bool eq(const CelxObserver &other) const;

private:
    void lookat(const Eigen::Vector3d &forwardd, const Eigen::Vector3d &upd);

    Observer *o;
};

} // namespace celestia::celx

void CreateObserverMetaTable(sol::state_view lua);
