// legacyscript.cpp
//
// Copyright (C) 2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "legacyscript.h"

#include <fstream>
#include <istream>

#include <celestia/celestiacore.h>
#include <celutil/gettext.h>
#include "cmdparser.h"
#include "command.h"
#include "execenv.h"
#include "execution.h"


namespace celestia::scripts
{

namespace
{

// Extremely basic implementation of an ExecutionEnvironment for
// running scripts.
class CoreExecutionEnvironment : public ExecutionEnvironment
{
private:
    CelestiaCore& core;

public:
    CoreExecutionEnvironment(CelestiaCore& _core) : core(_core)
    {
    }

    Simulation* getSimulation() const override
    {
        return core.getSimulation();
    }

    Renderer* getRenderer() const override
    {
        return core.getRenderer();
    }

    CelestiaCore* getCelestiaCore() const override
    {
        return &core;
    }

    void showText(std::string_view s, int horig, int vorig, int hoff, int voff,
                  double duration) override
    {
        core.showText(s, horig, vorig, hoff, voff, duration);
    }
};

} // end unnamed namespace

LegacyScript::LegacyScript(CelestiaCore *core) :
    m_appCore(core),
    m_execEnv(std::make_unique<CoreExecutionEnvironment>(*core))
{
}

bool LegacyScript::load(std::istream &scriptfile, const fs::path &/*path*/, std::string &errorMsg)
{
    CommandParser parser(scriptfile, m_appCore->scriptMaps());
    CommandSequence script = parser.parse();
    if (script.empty())
    {
        auto errors = parser.getErrors();
        if (!errors.empty())
            errorMsg = errors[0];
        return false;
    }
    m_runningScript = std::make_unique<Execution>(std::move(script), *m_execEnv);
    return true;
}

bool LegacyScript::tick(double dt)
{
    return m_runningScript->tick(dt);
}

bool LegacyScriptPlugin::isOurFile(const fs::path &p) const
{
    return p.extension() == ".cel";
}

std::unique_ptr<IScript> LegacyScriptPlugin::loadScript(const fs::path &path)
{
    std::ifstream scriptfile(path);
    if (!scriptfile.good())
    {
        appCore()->fatalError(_("Error opening script file."));
        return nullptr;
    }

    auto script = std::make_unique<LegacyScript>(appCore());
    std::string errorMsg;
    if (!script->load(scriptfile, path, errorMsg))
    {
        if (errorMsg.empty())
            errorMsg = _("Unknown error loading script");
        appCore()->fatalError(errorMsg);
        return nullptr;
    }
    return script;
}

} // end namespace celestia::scripts
