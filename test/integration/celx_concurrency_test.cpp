// celx_concurrency_test.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celestia/celestiacore.h>
#include <celestia/configfile.h>
#include <celengine/texmanager.h>
#include <celephem/scriptobject.h>
#include <celutil/flag.h>
#include <celutil/fsutils.h>
#include <celutil/logger.h>
#include <doctest.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

class RestoreLogger
{
public:
    ~RestoreLogger()
    {
        // CelestiaCore destroys the logger; restore it after the fixture's members.
        celestia::util::CreateLogger();
    }
};

std::filesystem::path testDirectoryPath()
{
    std::random_device random;
    return std::filesystem::temp_directory_path()
           / ("celestia-concurrent-scripts-" + std::to_string(random()) + "-" + std::to_string(random()));
}

class ScriptFixture : private RestoreLogger, public CelestiaCore::Alerter
{
public:
    using HookFiles = std::vector<std::pair<std::string_view, std::string_view>>;

    explicit ScriptFixture(std::string_view configuration = "Configuration {}",
                           const HookFiles& hooks = {},
                           bool singleHook = false,
                           const HookFiles& additionalHooks = {}) :
        directory(testDirectoryPath())
    {
        REQUIRE(std::filesystem::create_directory(directory));
        core.setAlerter(this);
        std::string source(configuration);
        if (!hooks.empty())
        {
            REQUIRE((!singleHook || hooks.size() == 1));
            std::string hookConfig = singleHook ? " LuaHook " : " LuaHook [ ";
            for (const auto& [name, text] : hooks)
            {
                auto path = std::filesystem::proximate(write(name, text)).generic_u8string();
                hookConfig += '"';
                hookConfig.append(path.begin(), path.end());
                hookConfig += "\" ";
            }
            if (!singleHook)
                hookConfig += "] ";
            auto end = source.rfind('}');
            REQUIRE(end != std::string::npos);
            source.insert(end, hookConfig);
        }
        auto config = write("test.cfg", source);
        std::vector<std::filesystem::path> luaHooks;
        for (const auto& [name, text] : additionalHooks)
            luaHooks.push_back(write(name, text));
        REQUIRE(core.initSimulation(config, {}, luaHooks));
    }

    ~ScriptFixture() override
    {
        core.cancelScript();
        std::error_code ec;
        std::filesystem::remove_all(directory, ec);
    }

    void fatalError(const std::string &error) override
    {
        errors.push_back(error);
    }

    std::filesystem::path write(std::string_view name, std::string_view text)
    {
        auto path = directory / name;
        std::ofstream out(path);
        out << text;
        REQUIRE(out.good());
        return path;
    }

    void tick()
    {
        core.tick(0.1);
    }
    double value() const
    {
        return core.getSimulation()->getTimeScale();
    }

    std::filesystem::path directory;
    std::vector<std::string> errors;
    CelestiaCore core;
};

} // namespace

TEST_SUITE_BEGIN("Concurrent CELX scripts");

TEST_CASE("Renderer notifications during script execution")
{
    ScriptFixture fixture;
    struct Watcher : RendererWatcher
    {
        void notifyRenderSettingsChanged(const Renderer*) override
        {
            if (!std::exchange(called, true))
                callback();
        }
        std::function<void()> callback;
        bool called{ false };
    } watcher;
    watcher.callback = [&] { fixture.core.cancelScript(); };
    auto* renderer = fixture.core.getRenderer();
    SUBCASE("An unhandled key during a legacy tick reaches object-name entry")
    {
        fixture.core.runScript(fixture.write("flags.cel", R"({ renderflags { clear "stars" } })"));
        fixture.tick();
        watcher.callback = [&] { fixture.core.charEntered('\r'); };
        renderer->addWatcher(&watcher);
        fixture.tick();
        renderer->removeWatcher(&watcher);
        CHECK(fixture.core.getTextEnterMode() == celestia::Hud::TextEnterMode::AutoComplete);
    }
    SUBCASE("A legacy script stops before its next command")
    {
        fixture.core.runScript(fixture.write("commands.cel", R"({
            renderflags { clear "stars" }
            timerate { rate 999 }
        })"));
        fixture.tick();
        auto original = fixture.value();
        renderer->addWatcher(&watcher);
        fixture.tick();
        renderer->removeWatcher(&watcher);
        CHECK(fixture.value() == original);
    }
    SUBCASE("A captured key during a CELX tick reaches its keyboard callback")
    {
        fixture.core.runScript(fixture.write("capture.celx", R"(
            function celestia_keyboard_callback(key)
                celestia:settimescale(42)
                return true
            end
            celestia:requestkeyboard(true)
            celestia:hide("stars")
            wait(1000)
        )"));
        watcher.callback = [&] { fixture.core.charEntered('\r'); };
        renderer->addWatcher(&watcher);
        fixture.tick();
        renderer->removeWatcher(&watcher);
        CHECK(fixture.value() == 42);
        CHECK(fixture.core.getTextEnterMode() == celestia::Hud::TextEnterMode::PassToScript);
    }
}

TEST_CASE("Loading a Lua hook distinguishes a failed read from an empty script")
{
    LuaState state;
    std::istringstream empty;
    CHECK(state.loadScript(empty, "empty.lua") == 0);
    std::istringstream failed;
    failed.setstate(std::ios::badbit);
    CHECK(state.loadScript(failed, "failed.lua") != 0);
}

TEST_CASE("Scripted-object contexts outlive hook finalizers and close before simulation teardown")
{
    std::string policy;
    SUBCASE("Shared context") { policy = "allow"; }
    SUBCASE("Private context") { policy = "ask"; }
    double finalScale = 0;
    celestia::ephem::ScriptedObjectContext handle;
    {
        ScriptFixture fixture("Configuration { ScriptSystemAccessPolicy \"" + policy + "\" }", {
            { "first.lua", "" },
            { "second.lua", R"(
                local mt = { __gc = function() celestia:settimescale(42) end }
                if newproxy then
                    marker = newproxy(true)
                    getmetatable(marker).__gc = mt.__gc
                else
                    marker = setmetatable({}, mt)
                end
            )" }
        });
        auto* context = celestia::ephem::GetScriptedObjectContext();
        REQUIRE(context != nullptr);
        handle = celestia::ephem::GetScriptedObjectContextHandle();
        REQUIRE(handle != nullptr);
        REQUIRE(handle->state == context);
        lua_newuserdata(context, 1);
        lua_newtable(context);
        lua_pushlightuserdata(context, &fixture.core);
        lua_pushlightuserdata(context, &finalScale);
        lua_pushlightuserdata(context, &handle);
        lua_pushcclosure(context, [](lua_State* l)
        {
            auto* core = static_cast<CelestiaCore*>(lua_touserdata(l, lua_upvalueindex(1)));
            auto* result = static_cast<double*>(lua_touserdata(l, lua_upvalueindex(2)));
            auto* saved = static_cast<celestia::ephem::ScriptedObjectContext*>(lua_touserdata(l, lua_upvalueindex(3)));
            CHECK((*saved)->state == nullptr);
            if (auto* sim = core->getSimulation())
                *result = sim->getTimeScale();
            return 0;
        }, 3);
        lua_setfield(context, -2, "__gc");
        lua_setmetatable(context, -2);
        lua_setglobal(context, "finalizer");
    }
    CHECK(finalScale == 42);
    CHECK(handle->state == nullptr);
}

TEST_CASE("Configured Lua hooks accept a single file or isolated files and broadcast ticks")
{
    for (bool singleHook : { true, false })
    {
        for (std::string_view policy : { "allow", "ask", "deny" })
        {
            CAPTURE(singleHook);
            CAPTURE(policy);
            std::vector<std::pair<std::string_view, std::string_view>> hooks = {
                { "first.lua", R"(
                    marker = 1
                    celestia:setluahook({
                        tick = function(self, dt)
                            assert(dt >= 0)
                            celestia:settimescale(celestia:gettimescale() * 10 + 1)
                            return true
                        end
                    })
                )" }
            };
            if (!singleHook)
            {
                hooks.emplace_back("second.lua", R"(
                    assert(marker == nil, "Hooks share Lua globals")
                    marker = 2
                    celestia:setluahook({
                        tick = function(self, dt)
                            assert(dt >= 0)
                            celestia:settimescale(celestia:gettimescale() * 10 + 2)
                        end
                    })
                )");
            }
            ScriptFixture fixture("Configuration { ScriptSystemAccessPolicy \"" + std::string(policy) + "\" }",
                                  hooks, singleHook);
            fixture.core.getSimulation()->setTimeScale(1);
            fixture.tick();
            CHECK(fixture.value() == (singleHook ? 11 : 112));
            auto* context = celestia::ephem::GetScriptedObjectContext();
            REQUIRE(context != nullptr);
            lua_getglobal(context, "marker");
            if (policy == "allow")
                CHECK(lua_tonumber(context, -1) == 1);
            else
                CHECK(lua_isnil(context, -1));
            lua_pop(context, 1);
            CHECK(fixture.errors.empty());
        }
    }
}

TEST_CASE("Additional Lua hooks run after configured hooks without loading duplicates")
{
    constexpr std::string_view first =
        "celestia:setluahook({tick = function() celestia:settimescale(celestia:gettimescale() + 4) end})";
    constexpr std::string_view next =
        "celestia:setluahook({tick = function() celestia:settimescale(celestia:gettimescale() * 2) end})";
    ScriptFixture fixture("Configuration {}", { { "first.lua", first } }, false,
                          { { "next.lua", next }, { "first.lua", first }, { "next.lua", next } });
    fixture.core.getSimulation()->setTimeScale(0);
    fixture.tick();
    CHECK(fixture.value() == 8);
    CHECK(fixture.errors.empty());
}

TEST_CASE("A single-string LuaHook setting still replaces the previous configuration value")
{
    ScriptFixture fixture;
    CelestiaConfig config;
    celestia::engine::TexturePaths textures;
    auto first = fixture.write("first.cfg", "Configuration { LuaHook \"first.lua\" }");
    auto second = fixture.write("second.cfg", "Configuration { LuaHook \"second.lua\" }");
    REQUIRE(ReadCelestiaConfig(first, config, textures));
    REQUIRE(ReadCelestiaConfig(second, config, textures));
    REQUIRE(config.paths.luaHooks.size() == 1);
    CHECK(config.paths.luaHooks.front() == "second.lua");
    auto empty = fixture.write("empty.cfg", R"(Configuration { LuaHook "" })");
    REQUIRE(ReadCelestiaConfig(empty, config, textures));
    CHECK(config.paths.luaHooks.empty());
}

TEST_CASE("A hook must be a regular file to supply the scripted-object context")
{
    ScriptFixture fixture(R"(Configuration { LuaHook "." ScriptSystemAccessPolicy "allow" })", {}, false,
                          { { "valid.lua", "marker = 42" } });
    REQUIRE(fixture.errors.size() == 1);
    auto* context = celestia::ephem::GetScriptedObjectContext();
    REQUIRE(context != nullptr);
    lua_getglobal(context, "marker");
    CHECK(lua_tonumber(context, -1) == 42);
    lua_pop(context, 1);
}

TEST_CASE("An empty configured hook leaves the shared context to the additional hook")
{
    ScriptFixture fixture(R"(Configuration { LuaHook "" ScriptSystemAccessPolicy "allow" })", {}, false,
                          { { "additional.lua", "marker = 42" } });
    auto* context = celestia::ephem::GetScriptedObjectContext();
    REQUIRE(context != nullptr);
    lua_getglobal(context, "marker");
    CHECK(lua_tonumber(context, -1) == 42);
    lua_pop(context, 1);
    CHECK(fixture.errors.empty());
}

TEST_CASE("Configured Lua hooks dispatch input in order until consumed and isolate callback errors")
{
    ScriptFixture fixture("Configuration {}", {
        { "first.lua", R"(
            celestia:setluahook({
                charentered = function(self, key)
                    if key == "e" then error({reason = "expected hook callback failure"}) end
                    celestia:settimescale(celestia:gettimescale() * 10 + 1)
                    return key == "y"
                end
            })
        )" },
        { "second.lua", R"(
            celestia:setluahook({
                charentered = function(self, key)
                    celestia:settimescale(celestia:gettimescale() * 10 + 2)
                    return true
                end
            })
        )" },
        { "third.lua", R"(
            celestia:setluahook({
                charentered = function(self, key)
                    celestia:settimescale(999)
                    return true
                end
            })
        )" }
    });
    for (char key : { 'x', 'y', 'e' })
    {
        fixture.core.getSimulation()->setTimeScale(0);
        fixture.core.charEntered(key);
        CHECK(fixture.value() == (key == 'x' ? 12 : key == 'y' ? 1 : 2));
    }
    CHECK(fixture.errors.empty());
}

TEST_CASE("A failed configured Lua hook load does not prevent later hooks from running")
{
    ScriptFixture fixture("Configuration {}", {
        { "broken.lua", "this is not valid Lua" },
        { "partial.lua", R"(
            function celestia_keyboard_callback() return true end
            local mt = { __gc = function() celestia:requestkeyboard(true) end }
            if newproxy then
                finalizer = newproxy(true)
                getmetatable(finalizer).__gc = mt.__gc
            else
                finalizer = setmetatable({}, mt)
            end
            celestia:requestkeyboard(true)
            celestia:setluahook({charentered = function() return true end})
            wait(0)
            error("hook initialization failed")
        )" },
        { "working.lua", R"(
            celestia:setluahook({
                charentered = function() celestia:settimescale(42); return true end
            })
        )" }
    });
    REQUIRE(fixture.errors.size() == 2);
    CHECK_FALSE(celestia::util::is_set(fixture.core.getTextEnterMode(), celestia::Hud::TextEnterMode::PassToScript));
    fixture.core.charEntered('x');
    CHECK(fixture.value() == 42);
}

TEST_CASE("Scripted objects keep a private context when all configured hooks fail")
{
    ScriptFixture fixture(R"(Configuration { ScriptSystemAccessPolicy "allow" })", {
        { "broken.lua", "marker = 42; error('hook initialization failed')" }
    });
    REQUIRE(fixture.errors.size() == 1);
    auto* context = celestia::ephem::GetScriptedObjectContext();
    REQUIRE(context != nullptr);
    lua_getglobal(context, "marker");
    CHECK(lua_isnil(context, -1));
    lua_getglobal(context, "require");
    CHECK(lua_isfunction(context, -1));
    lua_pop(context, 2);
    REQUIRE(luaL_dostring(context, "celestia:requestsystemaccess(); return type(io) == 'table'") == 0);
    CHECK(lua_toboolean(context, -1));
    lua_pop(context, 1);
}

TEST_CASE("System access requested by a child coroutine is available to the parent script")
{
    ScriptFixture fixture(R"(Configuration { ScriptSystemAccessPolicy "allow" })");
    auto script = fixture.write("access.celx", R"(
        local app = celestia
        local child = coroutine.create(function()
            if setfenv then setfenv(0, {}) end
            app:requestsystemaccess()
        end)
        assert(coroutine.resume(child))
        app:requestsystemaccess()
        assert(io and os)
        app:settimescale(42)
    )");
    fixture.core.runScript(script);
    fixture.tick();
    CHECK(fixture.value() == 42);
    CHECK(fixture.errors.empty());
}

TEST_CASE("Private catalog system access requires a global allow policy")
{
    using Policy = CelestiaCore::ScriptSystemAccessPolicy;
    ScriptFixture fixture(R"(Configuration { ScriptSystemAccessPolicy "ask" })");
    bool prompted = false;
    fixture.core.setScriptSystemAccessHandler([&prompted](const std::string&)
    {
        prompted = true;
        return Policy::Allow;
    });
    auto* context = celestia::ephem::GetScriptedObjectContext();
    REQUIRE(context != nullptr);
    auto modulePath = fixture.write("aftergrant.lua", "return 42").parent_path() / "?.lua";
    auto path = modulePath.generic_u8string();
    lua_pushlstring(context, reinterpret_cast<const char*>(path.data()), path.size());
    lua_setglobal(context, "modulePath");
    REQUIRE(luaL_dostring(context, "package.path = modulePath") == 0);
    REQUIRE(luaL_dostring(context, "celestia:requestsystemaccess(); assert(not io and not os)") == 0);
    CHECK_FALSE(prompted);
    fixture.core.setScriptSystemAccessPolicy(Policy::Allow);
    REQUIRE(luaL_dostring(context, "celestia:requestsystemaccess(); assert(io and os and require('aftergrant') == 42)") == 0);
}

TEST_CASE("Ask resolves synchronously and only an explicit approval grants system access")
{
    using Policy = CelestiaCore::ScriptSystemAccessPolicy;
    ScriptFixture fixture(R"(Configuration { ScriptSystemAccessPolicy "ask" })");
    auto checkAccess = [&](int expectedScale)
    {
        fixture.core.runScript(fixture.write("access.celx", R"(
            celestia:requestsystemaccess()
            celestia:settimescale(io and os and 42 or 2)
        )"));
        fixture.tick();
        CHECK(fixture.value() == expectedScale);
        CHECK(fixture.errors.empty());
    };
    SUBCASE("No frontend handler") { checkAccess(2); }
    SUBCASE("The frontend declines")
    {
        fixture.core.setScriptSystemAccessHandler([](const std::string&) { return Policy::Deny; });
        checkAccess(2);
    }
    SUBCASE("The frontend returns no decision")
    {
        fixture.core.setScriptSystemAccessHandler([](const std::string&) { return Policy::Ask; });
        checkAccess(2);
    }
    SUBCASE("The frontend approves")
    {
        fixture.core.setScriptSystemAccessHandler([&fixture](const std::string& scriptPath)
        {
            CHECK(scriptPath == celestia::util::PathToString(fixture.directory / "access.celx"));
            return Policy::Allow;
        });
        checkAccess(42);
    }
}

TEST_CASE("A waiting script does not stall another script or share its Lua globals")
{
    ScriptFixture fixture;
    fixture.write("child.celx", R"(
        assert(marker == nil)
        celestia:settimescale(10)
        wait(0)
        celestia:settimescale(20)
    )");
    auto parent = fixture.write("parent.celx", R"(
        marker = 17
        celestia_cleanup_callback = function() celestia:settimescale(77) end
        celestia:settimescale(2)
        celestia:runscript("child.celx", "concurrent")
        wait(1000)
        error("long wait resumed too soon")
    )");
    fixture.core.runScript(parent);
    fixture.tick();
    CHECK(fixture.value() == 2);
    fixture.tick();
    CHECK(fixture.value() == 10);
    fixture.tick();
    CHECK(fixture.value() == 20);
    fixture.tick();
    CHECK(fixture.value() == 20);
    fixture.core.charEntered('\033');
    CHECK(fixture.value() == 77);
    CHECK(fixture.errors.empty());
}

TEST_CASE("CELX script launches follow the configured mode unless explicitly overridden")
{
    for (std::string_view setting : { "replace", "concurrent" })
    {
        for (std::string_view argument : { "", ", \"default\"", ", \"replace\"", ", \"concurrent\"" })
        {
            CAPTURE(setting);
            CAPTURE(argument);
            ScriptFixture fixture("Configuration { ScriptRunMode \"" + std::string(setting) + "\" }");
            fixture.write("child.celx", R"(
                celestia:settimescale(celestia:gettimescale() + 10)
                wait(1000)
            )");
            std::string source = R"(
                celestia:settimescale(2)
                celestia_cleanup_callback = function()
                    celestia:settimescale(celestia:gettimescale() + 100)
                end
            )";
            source += "celestia:runscript(\"child.celx\"" + std::string(argument) + R"()
                while true do
                    wait(0)
                    celestia:settimescale(celestia:gettimescale() + 1)
                end
            )";
            auto parent = fixture.write("parent.celx", source);
            fixture.core.runScript(parent);
            fixture.tick();
            bool concurrent = argument == ", \"concurrent\""
                              || (argument != ", \"replace\"" && setting == "concurrent");
            CHECK(fixture.value() == (concurrent ? 2 : 102));
            fixture.tick();
            CHECK(fixture.value() == (concurrent ? 13 : 112));
            fixture.tick();
            CHECK(fixture.value() == (concurrent ? 14 : 112));
            CHECK(fixture.errors.empty());
        }
    }
}

TEST_CASE("Changing the script run mode setting affects subsequent default launches")
{
    ScriptFixture fixture;
    auto parent = fixture.write("parent.celx", R"(
        celestia:settimescale(2)
        celestia_cleanup_callback = function()
            celestia:settimescale(celestia:gettimescale() + 100)
        end
        while true do
            wait(0)
            celestia:settimescale(celestia:gettimescale() + 1)
        end
    )");
    auto child = fixture.write("child.celx", R"(
        celestia:settimescale(celestia:gettimescale() + 10)
        wait(1000)
    )");
    fixture.core.runScript(parent);
    fixture.tick();
    REQUIRE(fixture.value() == 2);
    fixture.core.setScriptRunMode(CelestiaCore::ScriptRunMode::Concurrent);
    fixture.core.runScript(child);
    fixture.tick();
    CHECK(fixture.value() == 13);
    fixture.core.setScriptRunMode(CelestiaCore::ScriptRunMode::Replace);
    fixture.core.runScript(child);
    fixture.tick();
    CHECK(fixture.value() == 123);
    fixture.tick();
    CHECK(fixture.value() == 123);
    CHECK(fixture.errors.empty());
}

TEST_CASE("A canceled callback cannot replace a script launched by the frontend")
{
    ScriptFixture fixture;
    auto replacement = fixture.write("replacement.celx", "celestia:settimescale(42)");
    fixture.write("stale.celx", "celestia:settimescale(99)");
    int requests = 0;
    fixture.core.setScriptSystemAccessHandler([&](const std::string&)
    {
        ++requests;
        fixture.core.runScript(replacement);
        return CelestiaCore::ScriptSystemAccessPolicy::Allow;
    });
    fixture.core.runScript(fixture.write("original.celx", R"(
        celestia:requestsystemaccess()
        assert(io == nil)
        celestia:requestsystemaccess()
        celestia:runscript("stale.celx")
    )"));
    fixture.tick();
    fixture.tick();
    CHECK(fixture.value() == 42);
    CHECK(requests == 1);
    CHECK(fixture.errors.empty());
}

TEST_CASE("Canceled cleanup callbacks cannot launch scripts alongside or instead of a replacement")
{
    for (std::string_view trigger : { "external", "tick", "keyboard" })
    {
        for (std::string_view mode : { "replace", "concurrent" })
        {
            CAPTURE(trigger);
            CAPTURE(mode);
            ScriptFixture fixture;
            fixture.write("cleanup-child.celx", "celestia:settimescale(celestia:gettimescale() + 100)");
            auto replacement = fixture.write("replacement.celx", R"(
                celestia:settimescale(celestia:gettimescale() + 1)
                wait(1000)
            )");
            std::string source = R"(
                celestia_cleanup_callback = function()
                    celestia:runscript("cleanup-child.celx", ")";
            source += mode;
            source += "\")\nend\n";
            if (trigger == "tick")
            {
                source += "celestia:runscript(\"replacement.celx\")\n";
            }
            else if (trigger == "keyboard")
            {
                source += R"(
                    celestia_keyboard_callback = function(key)
                        celestia:runscript("replacement.celx")
                        return true
                    end
                    celestia:requestkeyboard(true)
                )";
            }
            source += "wait(1000)";
            auto old = fixture.write("old.celx", source);
            fixture.core.runScript(old);
            fixture.tick();
            if (trigger == "external")
            {
                fixture.core.runScript(replacement);
            }
            else if (trigger == "keyboard")
            {
                fixture.core.charEntered('a');
            }
            fixture.tick();
            fixture.tick();
            CHECK(fixture.value() == 2);
            CHECK(fixture.errors.empty());
        }
    }
}

TEST_CASE("Concurrent children start paused without pausing their parent")
{
    for (std::string_view child : { "child.celx", "child.cel" })
    {
        CAPTURE(child);
        ScriptFixture fixture;
        fixture.write("child.celx", "celestia:settimescale(999)\nwait(1000)");
        fixture.write("child.cel", R"({
            timerate { rate 999 }
            wait { duration 1000 }
        })");
        std::string source = "celestia:pause(true)\ncelestia:runscript(\"";
        source += child;
        source += R"(", "concurrent")
            wait(0)
            celestia:settimescale(77)
            wait(0)
            celestia:settimescale(78)
            celestia:pause(false)
            wait(1000)
        )";
        auto parent = fixture.write("parent.celx", source);
        fixture.core.runScript(parent);
        fixture.tick();
        REQUIRE(fixture.core.getSimulation()->getPauseState());
        fixture.tick();
        CHECK(fixture.value() == 77);
        fixture.tick();
        CHECK(fixture.value() == 78);
        CHECK_FALSE(fixture.core.getSimulation()->getPauseState());
        fixture.tick();
        CHECK(fixture.value() == 78);
        fixture.core.charEntered(' ');
        fixture.tick();
        CHECK(fixture.value() == 78);
        fixture.core.charEntered(' ');
        fixture.tick();
        fixture.tick();
        CHECK(fixture.value() == 999);
        CHECK(fixture.errors.empty());
    }
}

TEST_CASE("Concurrent launches cannot resume an existing paused script")
{
    ScriptFixture fixture;
    fixture.write("child.celx", "wait(0)\ncelestia:settimescale(77)");
    auto parent = fixture.write("parent.celx", R"(
        local launched = false
        celestia:registereventhandler("tick", function(event)
            if celestia:ispaused() and not launched then
                launched = true
                celestia:pause(false)
                celestia:runscript("child.celx", "concurrent")
            end
        end)
        wait(0)
        celestia:settimescale(999)
    )");
    fixture.core.runScript(parent);
    fixture.tick();
    fixture.core.charEntered(' ');
    fixture.tick();
    CHECK_FALSE(fixture.core.getSimulation()->getPauseState());
    fixture.tick();
    CHECK(fixture.value() == 1);
    fixture.tick();
    fixture.tick();
    CHECK(fixture.value() == 77);
    fixture.core.charEntered(' ');
    fixture.core.charEntered(' ');
    fixture.tick();
    CHECK(fixture.value() == 999);
    CHECK(fixture.errors.empty());
}

TEST_CASE("A completed script cannot release another script's keyboard capture")
{
    ScriptFixture fixture;
    auto parent = fixture.write("keyboard.celx", R"(
        celestia_keyboard_callback = function(key)
            celestia:settimescale(celestia:gettimescale() + 1)
            return true
        end
        celestia:requestkeyboard(true)
        wait(1000)
    )");
    auto child = fixture.write("child.celx", "celestia:requestkeyboard(false)");
    fixture.core.runScript(parent);
    fixture.tick();
    fixture.core.runScript(child, CelestiaCore::ScriptRunMode::Concurrent);
    fixture.tick();
    CHECK(
        celestia::util::is_set(
            fixture.core.getTextEnterMode(),
            celestia::Hud::TextEnterMode::PassToScript));
    fixture.core.charEntered('a');
    CHECK(fixture.value() == 2);
    fixture.core.charEntered('\033');
    CHECK_FALSE(
        celestia::util::is_set(
            fixture.core.getTextEnterMode(),
            celestia::Hud::TextEnterMode::PassToScript));
    CHECK(fixture.errors.empty());
}

TEST_CASE("A hook retains keyboard capture until it releases its request")
{
    ScriptFixture fixture("Configuration {}", { { "keyboard.lua", R"(
        function celestia_keyboard_callback() return false end
        celestia:setluahook({
            mousebuttondown = function() celestia:requestkeyboard(true); return true end,
            charentered = function(self, key)
                if key == "[" then celestia:settimescale(celestia:gettimescale() + 1) end
                if key == "r" then celestia:requestkeyboard(false) end
                return true
            end
        })
    )" } });
    auto captured = [&] {
        return celestia::util::is_set(fixture.core.getTextEnterMode(), celestia::Hud::TextEnterMode::PassToScript);
    };
    fixture.core.mouseButtonDown(1, 1, CelestiaCore::LeftButton);
    REQUIRE(captured());
    fixture.core.charEntered('[');
    REQUIRE(captured());
    fixture.core.runScript(fixture.write("waiting.celx", "wait(1000)"));
    fixture.tick();
    fixture.core.cancelScript();
    REQUIRE(captured());
    fixture.core.charEntered('[');
    CHECK(fixture.value() == 3);
    fixture.core.charEntered('r');
    CHECK_FALSE(captured());
}

TEST_CASE("A frontend can release a script's keyboard capture to enter an object name")
{
    ScriptFixture fixture;
    fixture.core.runScript(fixture.write("keyboard.celx", R"(
        celestia_keyboard_callback = function() celestia:settimescale(99); return true end
        celestia:requestkeyboard(true)
        wait(1000)
    )"));
    fixture.tick();
    fixture.core.setTextEnterMode(celestia::Hud::TextEnterMode::Normal);
    fixture.core.charEntered('\r');
    fixture.tick();
    fixture.core.charEntered('a');
    CHECK(fixture.core.getTypedText() == "a");
    CHECK(fixture.value() != 99);
}

TEST_CASE("Temporary script capture does not wake a frontend-released keyboard callback")
{
    ScriptFixture fixture;
    fixture.core.runScript(fixture.write("first.celx", R"(
        function celestia_keyboard_callback() celestia:settimescale(99); return true end
        celestia:requestkeyboard(true)
        wait(1000)
    )"));
    fixture.tick();
    fixture.core.setTextEnterMode(celestia::Hud::TextEnterMode::Normal);
    fixture.core.charEntered('\r');
    fixture.core.runScript(fixture.write("second.celx", R"(
        function celestia_keyboard_callback() return false end
        celestia:requestkeyboard(true)
        wait(0)
    )"), CelestiaCore::ScriptRunMode::Concurrent);
    fixture.tick();
    fixture.core.charEntered('a');
    CHECK(fixture.value() != 99);
    fixture.tick();
    CHECK_FALSE(celestia::util::is_set(fixture.core.getTextEnterMode(), celestia::Hud::TextEnterMode::PassToScript));
}

TEST_CASE("A failed script or failed concurrent load leaves other scripts running")
{
    ScriptFixture fixture;
    auto running = fixture.write("running.celx", R"(
        while true do
            celestia:settimescale(celestia:gettimescale() + 1)
            wait(0)
        end
    )");
    auto failed = fixture.write("failed.celx", R"(error("expected failure"))");
    fixture.core.runScript(running);
    fixture.core.runScript(failed, CelestiaCore::ScriptRunMode::Concurrent);
    fixture.tick();
    CHECK(fixture.value() == 2);
    REQUIRE(fixture.errors.size() == 1);
    fixture.core.runScript(fixture.directory / "missing.celx", CelestiaCore::ScriptRunMode::Concurrent);
    fixture.tick();
    CHECK(fixture.value() == 3);
    CHECK(fixture.errors.size() == 2);
}

TEST_SUITE_END();
