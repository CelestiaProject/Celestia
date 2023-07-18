// watcher.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

template<class T> class Watcher
{
 private:
    T& watched;

 public:
    Watcher(T& _watched) : watched(_watched)
    {
        watched.addWatcher(this);
    };

    virtual ~Watcher()
    {
        watched.removeWatcher(this);
    }

    virtual void notifyChange(T*, int) = 0;
};
