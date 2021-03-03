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

struct Eclipse
{
    // values must be 2^n
    enum Type {
        Solar = 0x01,
        Lunar = 0x02
    };

    Body* occulter{ nullptr };
    Body* receiver{ nullptr };

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
    virtual ~EclipseFinderWatcher() = default;
};

class EclipseFinder
{
 public:
    EclipseFinder(Body*, EclipseFinderWatcher* = nullptr);

    void findEclipses(double startDate,
                      double endDate,
                      int eclipseTypeMask,
                      std::vector<Eclipse>& eclipses);
 private:
    Body* body;
    EclipseFinderWatcher* watcher;
};
#endif // _ECLIPSEFINDER_H_

