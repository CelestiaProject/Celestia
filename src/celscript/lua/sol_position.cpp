#include <cmath> // std::nan
#include <string_view>

#define SOL_ALL_SAFETIES_ON 1
#include <celengine/univcoord.h>
#include <sol/sol.hpp>

using namespace std::string_view_literals;

namespace
{
// Helper functions to be used with sol::overload()
// or too long to fit in lambda

// src:orientationto(target, upd)
Eigen::Quaterniond
orientation_to(const UniversalCoord &src, const UniversalCoord &target, const Eigen::Vector3d &upd)
{
    Eigen::Vector3d src2target = target.offsetFromKm(src).normalized();
    Eigen::Vector3d v          = src2target.cross(upd).normalized();
    Eigen::Vector3d u          = v.cross(src2target);
    Eigen::Matrix3d m;
    m.row(0) = v;
    m.row(1) = u;
    m.row(2) = src2target * (-1);
    return Eigen::Quaterniond(m);
}

// Position + Position -> Position
UniversalCoord
add_position(const UniversalCoord &p1, const UniversalCoord &p2)
{
    // this is not very intuitive, as p1-p2 is a vector
    return p1 + p2;
}

// Position + Vector -> Position
UniversalCoord
add_vector(const UniversalCoord &p, const Eigen::Vector3d &v)
{
    return p.offsetUly(v);
}

// Position - Position -> Vector
Eigen::Vector3d
sub_position(const UniversalCoord &p1, const UniversalCoord &p2)
{
    return p1.offsetFromUly(p2);
}

// Position - Vector -> Position
UniversalCoord
sub_vector(const UniversalCoord &p, const Eigen::Vector3d &v)
{
    return p.offsetUly(v);
}

double
get(const UniversalCoord &p, std::string_view key)
{
    if (key == "x"sv)
        return static_cast<double>(p.x);
    if (key == "y"sv)
        return static_cast<double>(p.y);
    if (key == "z"sv)
        return static_cast<double>(p.z);
    return std::nan(""); // FIXME
}

void
set(UniversalCoord &p, std::string_view key, double value)
{
    if (key == "x"sv)
        p.x = R128(value);
    else if (key == "y"sv)
        p.y = R128(value);
    else if (key == "z"sv)
        p.z = R128(value);
    // TODO: throw error
}

} // namespace

void
CreatePositionMetaTable(sol::state_view lua)
{
    lua.new_usertype<UniversalCoord>(
        "UniversalCoord",
        sol::no_constructor,
        "__tostring",
        []() {
            return "[Position]";
        }, // TODO: print out the coordinate as it would appear in a cel:// URL
        "getx",
        [](const UniversalCoord &p) { return p.x; },
        "gety",
        [](const UniversalCoord &p) { return p.y; },
        "getz",
        [](const UniversalCoord &p) { return p.z; },
        "distanceto",
        [](const UniversalCoord &p1, const UniversalCoord &p2)
        { return p2.offsetFromKm(p1).norm(); },
        "vectorto",
        [](const UniversalCoord &p1, const UniversalCoord &p2) { return p2.offsetFromUly(p1); },
        "orientationto",
        orientation_to,
        "addvector",
        [](const UniversalCoord &p, const Eigen::Vector3d &v) -> UniversalCoord
        { return { p.offsetUly(v) }; },
        "__add",
        sol::overload(add_position, add_vector),
        "__sub",
        sol::overload(sub_position, sub_vector),
        "__index",
        get,
        "__newindex",
        set);
}
