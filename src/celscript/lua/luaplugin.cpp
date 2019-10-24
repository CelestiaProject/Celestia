// luaplugin.cpp
//
// Copyright (C) 2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <string>
#include <celplugin/plugin-common.h>
#include "luascript.h"

class CelestiaCore;
class CelestiaConfig;
class ProgressNotifier;
class Hash;
class CachingOrbit;
class RotationModel;

using namespace celestia::plugin;
using namespace celestia::scripts;

CELESTIA_PLUGIN_ENTRYPOINT()
{
    static celestia::plugin::PluginInfo pinf(celestia::plugin::Scripting, "LUA");
    return &pinf;
}

CELESTIA_PLUGIN_EXPORTABLE bool CreateScriptEnvironment(CelestiaCore *appCore, const CelestiaConfig *config, ProgressNotifier *progressNotifier)
{
    return celestia::scripts::CreateLuaEnvironment(appCore, config, progressNotifier);
}

CELESTIA_PLUGIN_EXPORTABLE LuaScript* CreateScript(CelestiaCore *appCore)
{
    return nullptr;
}

RotationModel* CreateScriptedRotation(const std::string& moduleName, const std::string& funcName, Hash* parameters)
{
    return nullptr;
}

CachingOrbit* CreateScriptedOrbit(const std::string& moduleName, const std::string& funcName, Hash* parameters)
{
    return nullptr;
}
