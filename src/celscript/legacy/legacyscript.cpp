// legacyscript.cpp
//
// Copyright (C) 2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <fstream>
#include <string>
#include <celcompat/filesystem.h>
#include <celcompat/memory.h>
#include <celestia/celestiacore.h>
#include <celutil/util.h>
#include "legacyscript.h"
#include "cmdparser.h"
#include "execution.h"

using namespace std;

namespace celestia
{
namespace scripts
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

    Simulation* getSimulation() const
    {
        return core.getSimulation();
    }

    Renderer* getRenderer() const
    {
        return core.getRenderer();
    }

    CelestiaCore* getCelestiaCore() const
    {
        return &core;
    }

    void showText(string s, int horig, int vorig, int hoff, int voff,
                  double duration)
    {
        core.showText(s, horig, vorig, hoff, voff, duration);
    }
};

LegacyScript::LegacyScript(CelestiaCore *core) :
    m_appCore(core),
    m_execEnv(make_unique<CoreExecutionEnvironment>(*core))
{
}

bool LegacyScript::load(ifstream &scriptfile, const fs::path &path, string &errorMsg)
{
    CommandParser parser(scriptfile, m_appCore->scriptMaps());
    CommandSequence* script = parser.parse();
    if (script == nullptr)
    {
        const vector<string>* errors = parser.getErrors();
        if (errors->size() > 0)
            errorMsg = (*errors)[0];
        return false;
    }
    m_runningScript = make_unique<Execution>(*script, *m_execEnv);
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

unique_ptr<IScript> LegacyScriptPlugin::loadScript(const fs::path &path)
{
    ifstream scriptfile(path.string());
    if (!scriptfile.good())
    {
        appCore()->fatalError(_("Error opening script file."));
        return nullptr;
    }

    auto script = make_unique<LegacyScript>(appCore());
    string errorMsg;
    if (!script->load(scriptfile, path, errorMsg))
    {
        if (errorMsg.empty())
            errorMsg = _("Unknown error loading script");
        appCore()->fatalError(errorMsg);
        return nullptr;
    }
    return script;
}

}
}
