// eclipsefinder.h by Christophe Teyssier <chris@teyssier.org>
// adapted form wineclipses.h by Kendrix <kendrix@wanadoo.fr>
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Compute Solar Eclipses for our Solar System planets
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _ECLIPSEFINDER_H_
#define _ECLIPSEFINDER_H_

#include <vector>

#include "celestiacore.h"

class Eclipse
{
public:
    Eclipse(int Y, int M, int D);
    Eclipse(double JD);
    
    enum Type {
        Solar = 0,
        Moon  = 1
    };

public:
    Body* body;
    std::string planete;
    std::string sattelite;
    astro::Date* date;
    double startTime;
    double endTime;
};

class EclipseFinder
{
 public:
    EclipseFinder(CelestiaCore* core, 
                  const std::string& strPlaneteToFindOn_, 
                  Eclipse::Type type_,
                  double from, 
                  double to )
                   :appCore(core),
                    strPlaneteToFindOn(strPlaneteToFindOn_),
                    type(type_),
                    JDfrom(from),
                    JDto(to),
                    toProcess(true) {};
       
    const std::vector<Eclipse>& getEclipses() { if (toProcess) CalculateEclipses(); return Eclipses_; };
    
 private:
    CelestiaCore* appCore;
    std::vector<Eclipse> Eclipses_;

    std::string strPlaneteToFindOn;   
    Eclipse::Type type;
    double JDfrom, JDto;  
    
    bool toProcess;
    
    int CalculateEclipses();
    bool testEclipse(const Body& receiver, const Body& caster, double now) const;
    double findEclipseSpan(const Body& receiver, const Body& caster, double now, double dt) const;

};
#endif // _ECLIPSEFINDER_H_

