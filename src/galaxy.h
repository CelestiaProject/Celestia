// galaxy.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _GALAXY_H_
#define _GALAXY_H_

#include <vector>
#include <string>
#include "vecmath.h"
#include "quaternion.h"


class Galaxy
{
 public:
    Galaxy();

    std::string getName() const;
    void setName(const std::string&);
    Point3d getPosition() const;
    void setPosition(Point3d);
    Quatf getOrientation() const;
    void setOrientation(Quatf);
    float getRadius() const;
    void setRadius(float);
    
 private:
    std::string name;
    Point3d position;
    Quatf orientation;
    float radius;
};


typedef std::vector<Galaxy*> GalaxyList;

#endif // _GALAXY_H_
