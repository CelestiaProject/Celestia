// luascript.h
//
// Copyright (C) 2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <celscript/common/script.h>
#include <iosfwd>

class LuaState;
class CelestiaConfig;
class CelestiaCore;
class ProgressNotifier;

namespace celestia::scripts
{

class LuaScript : public IScript
{
 public:
    LuaScript(CelestiaCore*);
    ~LuaScript() override;

    bool load(std::ifstream&, const fs::path&, std::string&);

    bool handleMouseButtonEvent(float x, float y, int button, bool down) override;
    bool charEntered(const char*) override;
    bool handleKeyEvent(const char* key) override;
    bool handleTickEvent(double dt) override;
    bool tick(double) override;

 private:
    CelestiaCore *m_appCore;
    std::unique_ptr<LuaState> m_celxScript;

    friend class LuaScriptPlugin;
};

class LuaScriptPlugin : public IScriptPlugin
{
 public:
    LuaScriptPlugin() = delete;
    LuaScriptPlugin(CelestiaCore *appCore) : IScriptPlugin(appCore) {};
    ~LuaScriptPlugin() override = default;
    LuaScriptPlugin(const LuaScriptPlugin&) = delete;
    LuaScriptPlugin(LuaScriptPlugin&&) = delete;
    LuaScriptPlugin& operator=(const LuaScriptPlugin&) = delete;
    LuaScriptPlugin& operator=(LuaScriptPlugin&&) = delete;

    bool isOurFile(const fs::path&) const override;
    std::unique_ptr<IScript> loadScript(const fs::path&) override;
};

class LuaHook : public IScriptHook
{
 public:
    LuaHook() = delete;
    LuaHook(CelestiaCore *appCore) : IScriptHook(appCore) {};
    ~LuaHook() override = default;
    LuaHook(const LuaHook&) = delete;
    LuaHook(LuaHook&&) = default;
    LuaHook& operator=(const LuaHook&) = delete;
    LuaHook& operator=(LuaHook&&) = default;

    bool call(const char *method) const override;
    bool call(const char *method, const char *keyName) const override;
    bool call(const char *method, float x, float y) const override;
    bool call(const char *method, float x, float y, int b) const override;
    bool call(const char *method, double dt) const override;

 private:
    std::unique_ptr<LuaState> m_state;

    friend bool CreateLuaEnvironment(CelestiaCore*, const CelestiaConfig*, ProgressNotifier*);
};

bool CreateLuaEnvironment(CelestiaCore *appCore, const CelestiaConfig *config, ProgressNotifier *progressNotifier = nullptr);

} // end namespace celestia::scripts
