// celx.h
// 
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// Lua script extensions for Celestia
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELESTIA_CELX_H_
#define _CELESTIA_CELX_H_

#include <iostream>
#include <string>
extern "C" {
#include "lua.h"
}
#include <celutil/timer.h>
#include <celengine/observer.h>

class CelestiaCore;
class View;

class LuaState
{
public:
    LuaState();
    ~LuaState();

    lua_State* getState() const;

    int loadScript(std::istream&, const std::string& streamname);
    int loadScript(const std::string&);
    bool init(CelestiaCore*);

    std::string getErrorMessage();

    bool createThread();
    int resume();
    bool tick(double);
    void cleanup();
    bool isAlive() const;
    bool timesliceExpired();
    void requestIO();

    bool charEntered(const char*);
    double getTime() const;
    int screenshotCount;
    double timeout;
    
    bool handleKeyEvent(const char* key);
    
    enum IOMode {
        NoIO = 1,
        Asking = 2,
        IOAllowed = 4,
        IODenied = 8
    };

private:
    lua_State* state;
    lua_State* costate; // coroutine stack
    bool alive;
    Timer* timer;
    double scriptAwakenTime;
    IOMode ioMode;
};

View* getViewByObserver(CelestiaCore*, Observer*);
void getObservers(CelestiaCore*, std::vector<Observer*>&);

#endif // _CELESTIA_CELX_H_
