#include <Eigen/Geometry>
#include <cmath> // std::nan
#include <string_view>
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

using namespace std::string_view_literals;

namespace
{
// Helper functions to be used with sol::overload()
// or too long to fit in lambda

// Rotation * Rotation
Eigen::Quaterniond
rotation_x_rotation(const Eigen::Quaterniond &r1, const Eigen::Quaterniond &r2)
{
    return r1 * r2;
}

// Rotation * scalar
Eigen::Quaterniond
rotation_x_scalar(const Eigen::Quaterniond &r, double s)
{
    return Eigen::Quaterniond(r.w() * s, r.x() * s, r.y() * s, r.x() * s);
}

// scalar * Rotation
Eigen::Quaterniond
scalar_x_rotation(double s, const Eigen::Quaterniond &r)
{
    return rotation_x_scalar(r, s);
}

// Rotation + Rotation
Eigen::Quaterniond
add(const Eigen::Quaterniond &r1, const Eigen::Quaterniond &r2)
{
    Eigen::Quaterniond q(r1.w() + r2.w(), r1.x() + r2.x(), r1.y() + r2.y(), r1.z() + r2.z());
    return { q };
}

double
get(const Eigen::Quaterniond &r, std::string_view key)
{
    if (key == "x"sv)
        return r.x();
    if (key == "y"sv)
        return r.y();
    if (key == "z"sv)
        return r.z();
    if (key == "w"sv)
        return r.w();
    return std::nan(""); // FIXME
}

void
set(Eigen::Quaterniond &r, std::string_view key, double value)
{
    if (key == "x"sv)
        r.x() = value;
    else if (key == "y"sv)
        r.y() = value;
    else if (key == "z"sv)
        r.z() = value;
    else if (key == "w"sv)
        r.w() = value;
    // TODO: throw error
}
} // namespace

void
CreateRotationMetaTable(sol::state_view lua)
{
    lua.new_usertype<Eigen::Quaterniond>(
        "Rotation",
        sol::no_constructor,
        "__tostring",
        []() { return "[Rotation]"; },
        "real",
        [](const Eigen::Quaterniond &r) { return r.w(); },
        "imag",
        [](const Eigen::Quaterniond &r) { return r.vec(); },
        "transform",
        [](const Eigen::Quaterniond &r, const Eigen::Vector3d &v)
        {
            return r.toRotationMatrix().adjoint() * v; // XXX or transpose() instead of .adjoint()?
        },
        "setaxisangle",
        [](Eigen::Quaterniond &r, const Eigen::Vector3d &v, double angle) -> void
        { r = Eigen::Quaterniond(Eigen::AngleAxisd(angle, v.normalized())); },
        "slerp",
        [](const Eigen::Quaterniond &r1, const Eigen::Quaterniond &r2, double t)
        { return r1.slerp(t, r2); },
        "__add",
        add,
        "__mul",
        sol::overload(rotation_x_rotation, rotation_x_scalar, scalar_x_rotation),
        "__index",
        get,
        "__newindex",
        set);
}
