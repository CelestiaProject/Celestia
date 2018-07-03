// timer.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
// Copyright (C) 2018, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once
#include <chrono>

class Timer
{
 public:
    void reset() { start = std::chrono::high_resolution_clock::now(); }

    Timer() { reset(); };
    double getTime() const;

 private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};
