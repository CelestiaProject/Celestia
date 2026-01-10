// material.h
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include <tuple>

#include <Eigen/Core>

#include <celutil/color.h>
#include <celutil/texhandle.h>

namespace cmod
{

class Color
{
public:
    constexpr Color() = default;

    constexpr Color(float r, float g, float b) :
        m_red(r),
        m_green(g),
        m_blue(b)
    {
    }

    Color(const ::Color& color) :
        m_red(color.red()),
        m_green(color.green()),
        m_blue(color.blue())
    {
    }

    constexpr float red() const noexcept
    {
        return m_red;
    }

    constexpr float green() const noexcept
    {
        return m_green;
    }

    constexpr float blue() const noexcept
    {
        return m_blue;
    }

    Eigen::Vector3f toVector3() const
    {
        return Eigen::Vector3f(m_red, m_green, m_blue);
    }

    constexpr bool operator==(const Color& other) const
    {
        return std::tie(m_red, m_green, m_blue) == std::tie(other.m_red, other.m_green, other.m_blue);
    }

    constexpr bool operator!=(const Color& other) const
    {
        return !(*this == other);
    }

    constexpr bool operator<(const Color& other) const
    {
        return std::tie(m_red, m_green, m_blue) < std::tie(other.m_red, other.m_green, other.m_blue);
    }

private:
    float m_red{ 0.0f };
    float m_green{ 0.0f };
    float m_blue{ 0.0f };
};

enum class BlendMode : std::int16_t
{
    NormalBlend             = 0,
    AdditiveBlend           = 1,
    PremultipliedAlphaBlend = 2,
    BlendMax                = 3,
    InvalidBlend            = -1,
};

enum class TextureSemantic : std::int16_t
{
    DiffuseMap             =  0,
    NormalMap              =  1,
    SpecularMap            =  2,
    EmissiveMap            =  3,
    TextureSemanticMax     =  4,
    InvalidTextureSemantic = -1,
};

struct Material
{
    Material() { maps.fill(celestia::util::TextureHandle::Invalid); }

    celestia::util::TextureHandle getMap(TextureSemantic semantic) const
    {
        return maps[static_cast<std::size_t>(semantic)];
    }

    void setMap(TextureSemantic semantic, celestia::util::TextureHandle handle)
    {
        maps[static_cast<std::size_t>(semantic)] = handle;
    }

    Color diffuse;
    Color emissive;
    Color specular;
    float specularPower{ 1.0f };
    float opacity{ 1.0f };
    BlendMode blend{ BlendMode::NormalBlend };
    std::array<celestia::util::TextureHandle,
               static_cast<std::size_t>(TextureSemantic::TextureSemanticMax)> maps;
};

inline bool operator==(const Material& lhs, const Material& rhs)
{
    return std::tie(lhs.opacity, lhs.blend, lhs.diffuse, lhs.emissive, lhs.specular, lhs.specularPower, lhs.maps)
        == std::tie(rhs.opacity, rhs.blend, rhs.diffuse, rhs.emissive, rhs.specular, rhs.specularPower, rhs.maps);
}

inline bool operator!=(const Material& lhs, const Material& rhs)
{
    return !(lhs == rhs);
}

// Define an ordering for materials; required for elimination of duplicate
// materials.
inline bool operator<(const Material& lhs, const Material& rhs)
{
    // Checking opacity first and doing it backwards is deliberate. It means
    // that after sorting, translucent materials will end up with higher
    // material indices than opaque ones. Ultimately, after sorting
    // mesh primitive groups by material, translucent groups will end up
    // rendered after opaque ones.

    // Reverse sense of comparison for blending--additive blending is 1, normal
    // blending is 0, and we'd prefer to render additively blended submeshes
    // last.

    return std::tie(lhs.opacity, rhs.blend, lhs.diffuse, lhs.emissive, lhs.specular, lhs.specularPower, lhs.maps)
         < std::tie(rhs.opacity, lhs.blend, rhs.diffuse, rhs.emissive, rhs.specular, rhs.specularPower, rhs.maps);
}

inline bool operator>(const Material& lhs, const Material& rhs)
{
    return rhs < lhs;
}

inline bool operator<=(const Material& lhs, const Material& rhs)
{
    return !(rhs < lhs);
}

inline bool operator>=(const Material& lhs, const Material& rhs)
{
    return !(lhs < rhs);
}

} // namespace cmod
