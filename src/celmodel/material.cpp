// material.cpp
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <tuple>

#include "material.h"

namespace cmod
{
Material::Material()
{
    maps.fill(InvalidResource);
}


bool
operator<(const Material::Color& c0, const Material::Color& c1)
{
    return std::tie(c0.m_red, c0.m_green, c0.m_blue)
        < std::tie(c1.m_red, c1.m_green, c1.m_blue);
}


bool
operator<(const Material& m0, const Material& m1)
{
    // Checking opacity first and doing it backwards is deliberate. It means
    // that after sorting, translucent materials will end up with higher
    // material indices than opaque ones. Ultimately, after sorting
    // mesh primitive groups by material, translucent groups will end up
    // rendered after opaque ones.

    // Reverse sense of comparison for blending--additive blending is 1, normal
    // blending is 0, and we'd prefer to render additively blended submeshes
    // last.

    return std::tie(m0.opacity, m1.blend, m0.diffuse, m0.emissive, m0.specular, m0.specularPower, m0.maps)
        < std::tie(m1.opacity, m0.blend, m1.diffuse, m1.emissive, m1.specular, m1.specularPower, m1.maps);
}

}
