// luascript.cpp
//
// Copyright (C) 2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <set>
#include <string>
#include <filesystem>
#include <fstream>

#include <fmt/ostream.h>

#include <celephem/scriptobject.h>
#include <celestia/configfile.h>
#include <celestia/celestiacore.h>
#include <celestia/progressnotifier.h>
#include <celutil/fsutils.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include "celx_internal.h"
#include "luascript.h"

using namespace std;

namespace celestia
{
using util::GetLogger;

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

bool LuaScript::load(ifstream &scriptfile, const std::filesystem::path &path, string &errorMsg)
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

bool LuaScript::wantsKeyboard() const
{
    return m_celxScript->wantsKeyboard();
}

void LuaScript::markCanceled()
{
    m_celxScript->markCanceled();
}

bool LuaScriptPlugin::isOurFile(const std::filesystem::path &p) const
{
    auto ext = p.extension();
    return ext == ".celx" || ext == ".clx";
}

unique_ptr<IScript> LuaScriptPlugin::loadScript(const std::filesystem::path &path)
{
    ifstream scriptfile(path);
    if (!scriptfile.good())
    {
        appCore()->fatalError(fmt::format(fmt::runtime(_("Error opening script {}")), path));
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

LuaHook::~LuaHook()
{
    const auto* context = celestia::ephem::GetScriptedObjectContext();
    // Hook finalizers may still evaluate orbits in the private context or first hook.
    while (m_states.size() > 1)
        m_states.pop_back();
    if (!m_privateContexts.empty())
        m_states.clear();
    if ((!m_privateContexts.empty() && m_privateContexts.back()->getState() == context) ||
        (!m_states.empty() && m_states.front()->getState() == context))
        celestia::ephem::SetScriptedObjectContext(nullptr);
}

bool LuaHook::wantsKeyboard() const
{
    return std::ranges::any_of(m_states, [](const auto& state) { return state->wantsKeyboard(); });
}

template<typename... Args>
bool LuaHook::dispatch(const char* method, bool stopOnHandled, Args... args) const
{
    bool handled = false;
    for (const auto& state : m_states)
    {
        if (state->callLuaHook(appCore(), method, args...))
        {
            handled = true;
            if (stopOnHandled)
                break;
        }
    }
    return handled;
}

bool LuaHook::call(const char *method) const
{
    return dispatch(method, false);
}

bool LuaHook::call(const char *method, const char *keyName) const
{
    return dispatch(method, true, keyName);
}

bool LuaHook::call(const char *method, float x, float y) const
{
    return dispatch(method, true, x, y);
}

bool LuaHook::call(const char *method, float x, float y, int b) const
{
    return dispatch(method, true, x, y, b);
}

bool LuaHook::call(const char *method, double dt) const
{
    return dispatch(method, false, dt);
}

class LuaPathFinder
{
    set<std::filesystem::path> dirs;

 public:
    const string getLuaPath() const
    {
        string out;
        for (const auto& dir : dirs)
            out += util::PathToString(dir / "?.lua;");
        return out;
    }

    void process(const std::filesystem::path& p)
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
    for (const auto& dir : config->paths.extrasDirs)
    {
        if (dir.empty())
            continue;

        std::error_code ec;
        if (!std::filesystem::is_directory(dir, ec))
        {
            GetLogger()->warn(_("Path {} doesn't exist or isn't a directory\n"), dir);
            continue;
        }

        LuaPathFinder loader;
        auto iter = std::filesystem::recursive_directory_iterator(dir, ec);
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

static bool loadHook(LuaState& state, CelestiaCore& appCore, const std::filesystem::path& path,
                     ProgressNotifier* progressNotifier)
{
    int status;
    if (path.empty())
    {
        status = state.loadScript("");
    }
    else
    {
        if (std::error_code ec; !std::filesystem::is_regular_file(path, ec))
        {
            appCore.fatalError(fmt::format(fmt::runtime(_("Error opening LuaHook {}")), path));
            return false;
        }
        ifstream scriptfile(path);
        if (!scriptfile.good())
        {
            appCore.fatalError(fmt::format(fmt::runtime(_("Error opening LuaHook {}")), path));
            return false;
        }
        if (progressNotifier != nullptr)
            progressNotifier->update(util::PathToString(path));
        status = state.loadScript(scriptfile, path);
    }

    if (status != 0)
    {
        GetLogger()->debug("lua hook load failed\n");
        string errMsg = state.getErrorMessage();
        appCore.fatalError(errMsg.empty() ? _("Unknown error loading hook script") : errMsg);
        return false;
    }
    if (!state.createThread())
    {
        GetLogger()->debug("hook thread failed\n");
        appCore.fatalError(_("Script coroutine initialization failed"));
        return false;
    }
    return true;
}

bool LuaHook::createPrivateContext(const std::string& luaPath)
{
    auto context = make_unique<LuaState>();
    context->init(appCore());
    context->allowLuaPackageAccess(); // Allow require, but not loadlib.
    context->setLuaPath(luaPath);
    if (context->loadScript("") != 0)
    {
        GetLogger()->error("Cannot initialize private catalog context: {}\n", context->getErrorMessage());
        celestia::ephem::SetScriptedObjectContext(nullptr);
        return false;
    }
    m_privateContexts.push_back(std::move(context));
    celestia::ephem::SetScriptedObjectContext(m_privateContexts.back()->catalogContext());
    return true;
}

void LuaHook::restrictCatalogContext()
{
    const auto* active = celestia::ephem::GetScriptedObjectContext();
    if ((!m_states.empty() && active == m_states.front()->getState()) ||
        (!m_privateContexts.empty() && active == m_privateContexts.back()->getState() &&
         m_privateContexts.back()->hasSystemAccess()))
        createPrivateContext(lua_path(appCore()->getConfig()));
}

// Initialize the Lua hooks as well as the Lua state for scripted
// objects. Each Lua hook operates in a different Lua state than user loaded
// scripts. It always has file system access via the IO package. If the script
// system access policy is "allow", then scripted objects will run in the same
// Lua context as the first loaded hook. Sharing state between scripted objects
// and the hook can be very useful, but it gives system access to scripted objects,
// and therefore must be restricted based on the system access policy.
bool CreateLuaEnvironment(CelestiaCore *appCore, const CelestiaConfig *config, ProgressNotifier *progressNotifier)
{
    auto LuaPath = lua_path(config);

    auto hooks = make_unique<LuaHook>(appCore);
    auto* hookSet = hooks.get();
    appCore->setScriptHook(std::move(hooks));
    auto hookPaths = config->paths.luaHooks;
    std::erase(hookPaths, std::filesystem::path{});
    if (hookPaths.empty())
        hookPaths.emplace_back();

    bool success = true;
    for (const auto& path : hookPaths)
    {
        auto state = make_unique<LuaState>();
        state->init(appCore);
        state->allowSystemAccess();
        state->setLuaPath(LuaPath);

        if (!loadHook(*state, *appCore, path, progressNotifier) || !state->runHookInitialization())
        {
            success = false;
            state.reset(); // Run finalizers before reconciling keyboard capture.
            appCore->updateScriptKeyboardInput();
            continue;
        }

        hookSet->m_states.push_back(std::move(state));
    }

    // Set up the script context; if the system access policy is allow,
    // it will share the same context as the first loaded hook. Otherwise, we
    // create a private context.
    if (!hookSet->m_states.empty() && appCore->getScriptSystemAccessPolicy() == CelestiaCore::ScriptSystemAccessPolicy::Allow)
    {
        celestia::ephem::SetScriptedObjectContext(hookSet->m_states.front()->catalogContext());
    }
    else if (!hookSet->createPrivateContext(LuaPath))
    {
        return false;
    }

    return success;
}

}
}
