// unixtimer.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.


#include <sys/time.h>
#include <unistd.h>
#include "timer.h"

class UnixTimer : public Timer
{
public:
    UnixTimer();
    ~UnixTimer() override = default;
    double getTime() const override;
    void reset() override;

private:
    double start;
};


UnixTimer::UnixTimer()
{
    reset();
}

double UnixTimer::getTime() const
{
    struct timeval t;
    gettimeofday(&t, nullptr);
    return (double) t.tv_sec + (double) t.tv_usec / 1000000.0 - start;
}

void UnixTimer::reset()
{
    struct timeval t;
    gettimeofday(&t, nullptr);
    start = (double) t.tv_sec + (double) t.tv_usec / 1000000.0;
}

Timer* CreateTimer()
{
    return new UnixTimer();
}
