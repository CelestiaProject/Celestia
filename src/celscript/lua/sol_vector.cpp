#include <Eigen/Geometry>
#include <cmath> // std::nan
#include <fmt/format.h>
#include <string_view>
#include <variant>
#define SOL_ALL_SAFETIES_ON 1
#include <celengine/univcoord.h>
#include <sol/sol.hpp>

using namespace std::string_view_literals;

namespace
{
// Helper functions to be used with sol::overload()
// or too long to fit in lambda

// Vector + Vector
Eigen::Vector3d
add_vector(const Eigen::Vector3d &v1, const Eigen::Vector3d &v2)
{
    return v1 + v2;
}

// Vector + Position
UniversalCoord
add_position(const Eigen::Vector3d &v, const UniversalCoord &p)
{
    return p.offsetUly(v);
}

#if 0
std::variant<Eigen::Vector3d, UniversalCoord> add(const Eigen::Vector3d &v1, const std::variant<Eigen::Vector3d, UniversalCoord> &v)
{
    if (std::holds_alternative<Eigen::Vector3d>(v))
        return v1 + std::get<Eigen::Vector3d>(v);
    else
        return std::get<UniversalCoord>(v).offsetUly(v1);
}
#endif

// Vector * Vector
double
dot(const Eigen::Vector3d &v1, const Eigen::Vector3d &v2)
{
    return v1.dot(v2);
}

// Vector * double
Eigen::Vector3d
vector_x_scalar(const Eigen::Vector3d &v, double value)
{
    return v * value;
}

// Vector * Rotation
Eigen::Quaterniond
vector_x_rotation(const Eigen::Vector3d &v, const Eigen::Quaterniond &r)
{
    return Eigen::Quaterniond(0, v.x(), v.y(), v.z()) * r;
}

// double * Vector
Eigen::Vector3d
scalar_x_vector(double value, const Eigen::Vector3d &vec)
{
    return vec * value;
}

// Vector[Key]
double
get(const Eigen::Vector3d &v, std::string_view key)
{
    if (key == "x"sv)
        return v.x();
    if (key == "y"sv)
        return v.y();
    if (key == "z"sv)
        return v.z();
    return std::nan(""); // FIXME
}

// Vector[Key] = scalar
void
set(Eigen::Vector3d &v, std::string_view key, double value)
{
    if (key == "x"sv)
        v.x() = value;
    else if (key == "y"sv)
        v.y() = value;
    else if (key == "z"sv)
        v.z() = value;
    // TODO: throw error
}

} // namespace

void
CreateVectorMetaTable(sol::state_view lua)
{
    lua.new_usertype<Eigen::Vector3d>(
        "Vector3d",
        // sol::no_constructor,
        "__tostring",
        [](const Eigen::Vector3d &v)
        {
            // return "[Vector]";
            return fmt::format("[{}, {}, {}]", v.x(), v.y(), v.z());
        },
        "getx",
        [](const Eigen::Vector3d &v) { return v.x(); },
        "gety",
        [](const Eigen::Vector3d &v) { return v.y(); },
        "getz",
        [](const Eigen::Vector3d &v) { return v.z(); },
        "normalize",
        [](const Eigen::Vector3d &v) { return v.normalized(); },
        "length",
        [](const Eigen::Vector3d &v) { return v.norm(); },
        "__add",
        sol::overload(add_vector, add_position),
        //"__add", add,
        "__sub",
        [](const Eigen::Vector3d &v, const Eigen::Vector3d &other) { return v - other; },
        "__pow",
        [](const Eigen::Vector3d &v, const Eigen::Vector3d &other) { return v.cross(other); },
        "__mul",
        sol::overload(dot, vector_x_rotation, vector_x_scalar, scalar_x_vector),
        "__index",
        get,
        "__newindex",
        set);
}
