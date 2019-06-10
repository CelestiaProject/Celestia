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

#ifndef LUA_VER
#define LUA_VER 0x050100
#endif

#include <iostream>
#include <string>
#include <vector>
#include "lua.hpp"
#include <celutil/timer.h>
#include <celengine/observer.h>

#if LUA_VER < 0x050300
int lua_isinteger(lua_State *L, int index);
#endif

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

    // Celx script event handlers
    bool handleKeyEvent(const char* key);
    bool handleMouseButtonEvent(float x, float y, int button, bool down);
    bool handleTickEvent(double dt);

    // Lua hook handling
    void setLuaPath(const string& s);
    void allowSystemAccess();
    void allowLuaPackageAccess();
    void setLuaHookEventHandlerEnabled(bool);
    bool callLuaHook(void* obj, const char* method);
    bool callLuaHook(void* obj, const char* method, const char* keyName);
    bool callLuaHook(void* obj, const char* method, float x, float y);
    bool callLuaHook(void* obj, const char* method, float x, float y, int b);
    bool callLuaHook(void* obj, const char* method, double dt);

    enum IOMode {
        NoIO = 1,
        Asking = 2,
        IOAllowed = 4,
        IODenied = 8
    };

private:
    lua_State* state;
    lua_State* costate{ nullptr }; // coroutine stack
    bool alive{ false };
    Timer* timer;
    double scriptAwakenTime{ 0.0 };
    IOMode ioMode{ NoIO };
    bool eventHandlerEnabled{ false };
};

View* getViewByObserver(CelestiaCore*, Observer*);
void getObservers(CelestiaCore*, std::vector<Observer*>&);

#endif // _CELESTIA_CELX_H_
