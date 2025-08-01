// legacyscript.h
//
// Copyright (C) 2019, the Celestia Development Team
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

#include <celscript/common/script.h>


class CelestiaCore;

namespace celestia::scripts
{

class Execution;
class ExecutionEnvironment;

class LegacyScript : public IScript
{
 public:
    LegacyScript(CelestiaCore*);
    ~LegacyScript() override = default;

    bool load(std::istream&, const std::filesystem::path&, std::string&);

    bool tick(double) override;

 private:
    CelestiaCore *m_appCore;
    std::unique_ptr<Execution> m_runningScript;
    std::unique_ptr<ExecutionEnvironment> m_execEnv;

    friend class LegacyScriptPlugin;
};

class LegacyScriptPlugin : public IScriptPlugin
{
 public:
    LegacyScriptPlugin() = delete;
    LegacyScriptPlugin(CelestiaCore *appCore) : IScriptPlugin(appCore) {};
    ~LegacyScriptPlugin() override = default;
    LegacyScriptPlugin(const LegacyScriptPlugin&) = delete;
    LegacyScriptPlugin(LegacyScriptPlugin&&) = delete;
    LegacyScriptPlugin& operator=(const LegacyScriptPlugin&) = delete;
    LegacyScriptPlugin& operator=(LegacyScriptPlugin&&) = delete;

    bool isOurFile(const std::filesystem::path&) const override;
    std::unique_ptr<IScript> loadScript(const std::filesystem::path&) override;
};

} // end namespace celestia::scripts
