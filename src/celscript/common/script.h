// script.h
//
// Copyright (C) 2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <celcompat/filesystem.h>

class CelestiaCore;

namespace celestia
{
namespace scripts
{

class IScript
{
 public:
    virtual ~IScript() = default;

    virtual bool handleMouseButtonEvent(float x, float y, int button, bool down);
    virtual bool charEntered(const char*);
    virtual bool handleKeyEvent(const char* key);
    virtual bool handleTickEvent(double dt);
    virtual bool tick(double) = 0;
};

class IScriptPlugin
{
 public:
    IScriptPlugin() = delete;
    IScriptPlugin(CelestiaCore *appcore) : m_appCore(appcore) {};
    IScriptPlugin(const IScriptPlugin&) = delete;
#ifdef ENABLE_PLUGINS
    IScriptPlugin(IScriptPlugin&&) = default;
#else
    IScriptPlugin(IScriptPlugin&&) = delete;
#endif
    IScriptPlugin& operator=(const IScriptPlugin&) = delete;
    IScriptPlugin& operator=(IScriptPlugin&&) = delete;
    virtual ~IScriptPlugin() = default;

    virtual bool isOurFile(const fs::path&) const = 0;
    virtual std::unique_ptr<IScript> loadScript(const fs::path&) = 0;

    CelestiaCore *appCore() const { return m_appCore; }

 private:
    CelestiaCore *m_appCore;
};

class IScriptHook
{
 public:
    IScriptHook() = delete;
    IScriptHook(CelestiaCore *appcore) : m_appCore(appcore) {};
    IScriptHook(const IScriptHook&) = default;
    IScriptHook(IScriptHook&&) = default;
    IScriptHook& operator=(const IScriptHook&) = default;
    IScriptHook& operator=(IScriptHook&&) = default;
    virtual ~IScriptHook() = default;

    virtual bool call(const char *method) const = 0;
    virtual bool call(const char *method, const char *keyName) const = 0;
    virtual bool call(const char *method, float x, float y) const = 0;
    virtual bool call(const char *method, float x, float y, int b) const = 0;
    virtual bool call(const char *method, double dt) const = 0;

    CelestiaCore *appCore() const { return m_appCore; }

 private:
    CelestiaCore *m_appCore;
};

}
}
