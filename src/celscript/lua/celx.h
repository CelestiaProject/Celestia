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

#pragma once

#include <filesystem>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include <lua.hpp>

#include <celephem/scriptobject.h>
#include <celutil/timer.h>
#include <celengine/observer.h>

#ifndef LUA_VERSION_NUM
#define LUA_VERSION_NUM 501
#endif

#if LUA_VERSION_NUM < 503
int lua_isinteger(lua_State *L, int index);
#endif

class CelestiaCore;

namespace celestia
{
class View;
}

class LuaState // NOSONAR S1448: the typed hook callback overloads all dispatch into this VM.
{
public:
    LuaState();
    ~LuaState();

    lua_State* getState() const;
    celestia::ephem::ScriptedObjectContext catalogContext();

    int loadScript(std::istream&, const std::filesystem::path&);
    int loadScript(const std::string&);
    bool init(CelestiaCore*);

    std::string getErrorMessage();

    bool createThread();
    int resume();
    bool tick(double);
    void cleanup();
    bool isAlive() const;
    bool timesliceExpired();
    void chargeIOTime(double start, double cost = 0.1);
    void requestIO();

    bool charEntered(const char*);
    void requestKeyboard(bool requested, CelestiaCore&);
    void markCanceled();
    bool isCanceled() const { return m_canceled; }
    bool hasSystemAccess() const { return ioMode == IOMode::Allowed; }
    bool wantsKeyboard() const { return !m_keyboardCapture.expired(); }
    double getTime() const;
    int screenshotCount;
    double timeout;

    // Celx script event handlers
    bool handleKeyEvent(const char* key);
    bool handleMouseButtonEvent(float x, float y, int button, bool down);
    bool handleTickEvent(double dt);

    // Lua hook handling
    bool runHookInitialization();
    void setLuaPath(const std::string& s);
    void allowSystemAccess();
    void allowLuaPackageAccess();
    void setLuaHookEventHandlerEnabled(bool);
    bool callLuaHook(void* obj, const char* method);
    bool callLuaHook(void* obj, const char* method, const char* keyName);
    bool callLuaHook(void* obj, const char* method, float x, float y);
    bool callLuaHook(void* obj, const char* method, float x, float y, int b);
    bool callLuaHook(void* obj, const char* method, double dt);

    enum class IOMode
    {
        NotDetermined,
        Allowed,
        Denied
    };

private:
    class Timeslice;
    int callWithTimeslice(int nargs, int nresults);
    lua_State* state;
    std::string m_scriptPath;
    std::shared_ptr<celestia::ephem::ScriptedObjectVM> m_catalogContext;
    lua_State* costate{ nullptr }; // coroutine stack
    bool alive{ false };
    bool m_timesliceActive{ false };
    double m_ioTimeAdjustment{ 0.0 };
    Timer timer;
    double scriptAwakenTime{ 0.0 };
    IOMode ioMode{ IOMode::NotDetermined };
    bool eventHandlerEnabled{ false };
    std::weak_ptr<const bool> m_keyboardCapture;
    bool m_canceled{ false };
};

celestia::View* getViewByObserver(const CelestiaCore*, const Observer*);
void getObservers(const CelestiaCore*, std::vector<Observer*>&);
