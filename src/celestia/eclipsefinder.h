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
    Eclipse() = default;
    Eclipse(int Y, int M, int D);
    Eclipse(double JD);
    ~Eclipse();

    // values must be 2^n
    enum Type {
        Solar = 0x01,
        Moon  = 0x02,
        Lunar = 0x02
    };

 public:
    Body* body{ nullptr };

    Body* occulter{ nullptr };
    Body* receiver{ nullptr };

    std::string planete;
    std::string sattelite;
    astro::Date* date{ nullptr };
    double startTime{ 0.0 };
    double endTime{ 0.0 };
};

class EclipseFinderWatcher
{
 public:
    enum Status
    {
        ContinueOperation = 0,
        AbortOperation = 1,
    };

    virtual Status eclipseFinderProgressUpdate(double t) = 0;
};

class EclipseFinder
{
 public:
    EclipseFinder(Body*, EclipseFinderWatcher* _watcher);
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

    void findEclipses(double startDate,
                      double endDate,
                      int eclipseTypeMask,
                      vector<Eclipse>& eclipses);

    const std::vector<Eclipse>& getEclipses() { if (toProcess) CalculateEclipses(); return Eclipses_; };

 private:
    CelestiaCore* appCore;
    std::vector<Eclipse> Eclipses_;

    std::string strPlaneteToFindOn;
    Eclipse::Type type;
    double JDfrom, JDto;

    bool toProcess;

    double findEclipseStart(const Body& recever, const Body& occulter, double now, double startStep, double minStep) const;
    double findEclipseEnd(const Body& recever, const Body& occulter, double now, double startStep, double minStep) const;

    int CalculateEclipses();
    bool testEclipse(const Body& receiver, const Body& caster, double now) const;
    double findEclipseSpan(const Body& receiver, const Body& caster, double now, double dt) const;

    Body* body;
    EclipseFinderWatcher* watcher;
};
#endif // _ECLIPSEFINDER_H_

