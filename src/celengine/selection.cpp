// selection.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "astro.h"
#include "selection.h"


double Selection::radius() const
{
    if (star != NULL)
        return star->getRadius();
    else if (body != NULL)
        return body->getRadius();
    else if (galaxy != NULL)
        return astro::lightYearsToKilometers(galaxy->getRadius());
    else
        return 0.0;
}
