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
#include <celmath/vecmath.h>
#include <celmath/quaternion.h>
#include <celengine/deepskyobj.h>


struct GalacticForm
{
    std::vector<Point3f>* points;
    Vec3f scale;
};

class Galaxy : public DeepSkyObject
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

    GalaxyType getType() const;
    void setType(GalaxyType);
    float getDetail() const;
    void setDetail(float);
    //    float getBrightness() const;
    //    void setBrightness();

    virtual bool load(AssociativeArray*, const std::string&);
    virtual void render(const Vec3f& offset,
                        const Quatf& viewerOrientation,
                        float brightness,
                        float pixelSize);

    GalacticForm* getForm() const;
    
 private:
    float detail;
    //    float brightness;
    GalaxyType type;
    GalacticForm* form;
};

std::ostream& operator<<(std::ostream& s, const Galaxy::GalaxyType& sc);

#endif // _GALAXY_H_
