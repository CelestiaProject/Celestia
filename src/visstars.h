// visstars.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Keep track of the subset of stars within a database that are visible
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _VISSTARS_H_
#define _VISSTARS_H_

#include <vector>
#include "stardb.h"
#include "observer.h"

using namespace std;


class IntArray
{
};


class VisibleStarSet
{
public:
    VisibleStarSet(StarDatabase*);
    ~VisibleStarSet();

    void update(const Observer&, float fraction = 1);
    void updateAll(const Observer&);
    vector<uint32>* getVisibleSet() const;
    vector<uint32>* getCloseSet() const;
    void setLimitingMagnitude(float);
    void setCloseDistance(float);

private:
    VisibleStarSet(VisibleStarSet&) {};

    StarDatabase* starDB;
    float faintest;
    float closeDistance;
    vector<uint32>* current;
    vector<uint32>* last;
    vector<uint32>* currentNear;
    vector<uint32>* lastNear;
    uint32 firstIndex;
};

#endif // _VISSTARS_H_
