// destination.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <iosfwd>
#include <string>
#include <vector>


class Destination
{
 public:
    Destination() = default;

 public:
    std::string name;
    std::string target;
    double distance{ 0.0 };
    std::string description;
};

using DestinationList = std::vector<Destination*>;

DestinationList* ReadDestinationList(std::istream&);
