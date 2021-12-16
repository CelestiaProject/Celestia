// luascript.cpp
//
// Copyright (C) 2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <set>
#include <string>
#include <fstream>
#include <fmt/ostream.h>
#include <celcompat/filesystem.h>
#include <celephem/scriptobject.h>
#include <celestia/configfile.h>
#include <celestia/celestiacore.h>
#include <celutil/gettext.h>
#include "celx_internal.h"
#include "luascript.h"

using namespace std;

namespace celestia
{
namespace scripts
{

LuaScript::LuaScript(CelestiaCore *appcore) :
    m_appCore(appcore),
    m_celxScript(new LuaState)
{
    m_celxScript->init(m_appCore);
}

LuaScript::~LuaScript()
{
    m_celxScript->cleanup();
}

bool LuaScript::load(ifstream &scriptfile, const fs::path &path, string &errorMsg)
{
    if (m_celxScript->loadScript(scriptfile, path) != 0)
    {
        errorMsg = m_celxScript->getErrorMessage();
        return false;
    }
    return true;
}

bool LuaScript::handleMouseButtonEvent(float x, float y, int button, bool down)
{
    return m_celxScript->handleMouseButtonEvent(x, y, button, down);
}

bool LuaScript::charEntered(const char* c)
{
    return m_celxScript->charEntered(c);
}

bool LuaScript::handleKeyEvent(const char* key)
{
    return m_celxScript->handleKeyEvent(key);
}

bool LuaScript::handleTickEvent(double dt)
{
    return m_celxScript->handleTickEvent(dt);
}

bool LuaScript::tick(double dt)
{
    return m_celxScript->tick(dt);
}

bool LuaScriptPlugin::isOurFile(const fs::path &p) const
{
    auto ext = p.extension();
    return ext == ".celx" || ext == ".clx";
}

unique_ptr<IScript> LuaScriptPlugin::loadScript(const fs::path &path)
{
    ifstream scriptfile(path);
    if (!scriptfile.good())
    {
        appCore()->fatalError(fmt::sprintf(_("Error opening script '%s'"), path));
        return nullptr;
    }

    auto script = unique_ptr<LuaScript>(new LuaScript(appCore()));
    string errMsg;
    if (!script->load(scriptfile, path, errMsg))
    {
        if (errMsg.empty())
            errMsg = _("Unknown error loading script");
        appCore()->fatalError(errMsg);
        return nullptr;
    }
    // Coroutine execution; control may be transferred between the
    // script and Celestia's event loop
    if (!script->m_celxScript->createThread())
    {
        appCore()->fatalError(_("Script coroutine initialization failed"));
        return nullptr;
    }

    return script;
}

bool LuaHook::call(const char *method) const
{
    return m_state->callLuaHook(appCore(), method);
}

bool LuaHook::call(const char *method, const char *keyName) const
{
    return m_state->callLuaHook(appCore(), method, keyName);
}

bool LuaHook::call(const char *method, float x, float y) const
{
    return m_state->callLuaHook(appCore(), method, x, y);
}

bool LuaHook::call(const char *method, float x, float y, int b) const
{
    return m_state->callLuaHook(appCore(), method, x, y, b);
}

bool LuaHook::call(const char *method, double dt) const
{
    return m_state->callLuaHook(appCore(), method, dt);
}

class LuaPathFinder
{
    set<fs::path> dirs;

 public:
    const string getLuaPath() const
    {
        string out;
        for (const auto& dir : dirs)
            out += (dir / "?.lua;").string();
        return out;
    }

    void process(const fs::path& p)
    {
        auto dir = p.parent_path();
        if (p.extension() == ".lua" && dirs.count(dir) == 0)
            dirs.insert(dir);
    }
};

static string lua_path(const CelestiaConfig *config)
{
    string LuaPath = "?.lua;celxx/?.lua;";

    // Find the path for lua files in the extras directories
    for (const auto& dir : config->extrasDirs)
    {
        if (dir.empty())
            continue;

        std::error_code ec;
        if (!fs::is_directory(dir, ec))
        {
            cerr << fmt::sprintf(_("Path %s doesn't exist or isn't a directory\n"), dir);
            continue;
        }

        LuaPathFinder loader;
        auto iter = fs::recursive_directory_iterator(dir, ec);
        for (; iter != end(iter); iter.increment(ec))
        {
            if (ec)
                continue;
            loader.process(*iter);
        }
        LuaPath += loader.getLuaPath();
    }
    return LuaPath;
}

// Initialize the Lua hook table as well as the Lua state for scripted
// objects. The Lua hook operates in a different Lua state than user loaded
// scripts. It always has file system access via the IO package. If the script
// system access policy is "allow", then scripted objects will run in the same
// Lua context as the Lua hook. Sharing state between scripted objects and the
// hook can be very useful, but it gives system access to scripted objects,
// and therefore must be restricted based on the system access policy.
bool CreateLuaEnvironment(CelestiaCore *appCore, const CelestiaConfig *config, ProgressNotifier *progressNotifier)
{
    auto LuaPath = lua_path(config);

    LuaState *luaHook = new LuaState();
    luaHook->init(appCore);

    // Always grant access for the Lua hook
    luaHook->allowSystemAccess();
    luaHook->setLuaPath(LuaPath);

    int status = 0;
    // Execute the Lua hook initialization script
    if (!config->luaHook.empty())
    {
        ifstream scriptfile(config->luaHook);
        if (!scriptfile.good())
            appCore->fatalError(fmt::sprintf(_("Error opening LuaHook '%s'"), config->luaHook));

        if (progressNotifier != nullptr)
            progressNotifier->update(config->luaHook.string());

        status = luaHook->loadScript(scriptfile, config->luaHook);
    }
    else
    {
        status = luaHook->loadScript("");
    }

    if (status != 0)
    {
        cerr << "lua hook load failed\n";
        string errMsg = luaHook->getErrorMessage();
        if (errMsg.empty())
            errMsg = _("Unknown error loading hook script");
        appCore->fatalError(errMsg);
        luaHook = nullptr;
    }
    else
    {
        // Coroutine execution; control may be transferred between the
        // script and Celestia's event loop
        if (!luaHook->createThread())
        {
            cerr << "hook thread failed\n";
            appCore->fatalError(_("Script coroutine initialization failed"));
            luaHook = nullptr;
        }

        if (luaHook != nullptr)
        {
            auto lh = unique_ptr<LuaHook>(new LuaHook(appCore));
            lh->m_state = unique_ptr<LuaState>(luaHook);
            appCore->setScriptHook(std::move(lh));

            while (!luaHook->tick(0.1)) ;
        }
    }

    // Set up the script context; if the system access policy is allow,
    // it will share the same context as the Lua hook. Otherwise, we
    // create a private context.
    if (config->scriptSystemAccessPolicy == "allow")
    {
        if (luaHook != nullptr)
            SetScriptedObjectContext(luaHook->getState());
    }
    else
    {
        auto luaSandbox = new LuaState();
        luaSandbox->init(appCore);

        // Allow access to functions in package because we need 'require'
        // But, loadlib is prohibited.
        luaSandbox->allowLuaPackageAccess();
        luaSandbox->setLuaPath(LuaPath);

        status = luaSandbox->loadScript("");
        if (status != 0)
        {
            luaSandbox = nullptr;
            return false;
        }

        SetScriptedObjectContext(luaSandbox->getState());
    }

    return true;
}

}
}
