// asterism.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _ASTERISM_H_
#define _ASTERISM_H_

#include <string>
#include <vector>
#include <iostream>
#include "vecmath.h"
#include "stardb.h"

class Asterism
{
 public:
    Asterism(std::string);
    ~Asterism();

    typedef std::vector<Point3f> Chain;

    std::string getName() const;
    int getChainCount() const;
    const Chain& getChain(int) const;

    void addChain(Chain&);
 private:
    std::string name;
    std::vector<Chain*> chains;
};

typedef std::vector<Asterism*> AsterismList;

AsterismList* ReadAsterismList(std::istream&, const StarDatabase&);

#endif // _ASTERISM_H_



