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
Material::operator==(const Material& other) const
{
    return std::tie(opacity, blend, diffuse, emissive, specular, specularPower, maps)
        == std::tie(other.opacity, other.blend, other.diffuse, other.emissive, other.specular, other.specularPower, other.maps);
}


bool
Material::operator!=(const Material& other) const
{
    return !(*this == other);
}


bool
Material::operator<(const Material& other) const
{
    // Checking opacity first and doing it backwards is deliberate. It means
    // that after sorting, translucent materials will end up with higher
    // material indices than opaque ones. Ultimately, after sorting
    // mesh primitive groups by material, translucent groups will end up
    // rendered after opaque ones.

    // Reverse sense of comparison for blending--additive blending is 1, normal
    // blending is 0, and we'd prefer to render additively blended submeshes
    // last.

    return std::tie(opacity, other.blend, diffuse, emissive, specular, specularPower, maps)
        < std::tie(other.opacity, blend, other.diffuse, other.emissive, other.specular, other.specularPower, other.maps);
}

}
