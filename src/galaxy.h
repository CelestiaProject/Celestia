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
#include <iostream>
#include "vecmath.h"
#include "quaternion.h"


struct GalacticForm
{
    std::vector<Point3f>* points;
    Vec3f scale;
};

class Galaxy
{
 public:
    enum GalaxyType {
        S0   =  0,
        Sa   =  1,
        Sb   =  2,
        Sc   =  3,
        SBa  =  4,
        SBb  =  5,
        SBc  =  6,
        E0   =  7,
        E1   =  8,
        E2   =  9,
        E3   = 10,
        E4   = 11, 
        E5   = 12,
        E6   = 13,
        E7   = 14,
        Irr  = 15,
    };
        
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
    GalaxyType getType() const;
    void setType(GalaxyType);

    GalacticForm* getForm() const;
    
 private:
    std::string name;
    Point3d position;
    Quatf orientation;
    float radius;
    GalaxyType type;
    GalacticForm* form;
};


typedef std::vector<Galaxy*> GalaxyList;

GalaxyList* ReadGalaxyList(std::istream& in);

#endif // _GALAXY_H_
