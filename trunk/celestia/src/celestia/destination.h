// destination.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _DESTINATION_H_
#define _DESTINATION_H_

#include <string>
#include <vector>
#include <iostream>


class Destination
{
 public:
    Destination();

 public:
    std::string name;
    std::string target;
    double distance;
    std::string description;
};

typedef std::vector<Destination*> DestinationList;

DestinationList* ReadDestinationList(std::istream&);

#endif // _DESTINATION_H_
