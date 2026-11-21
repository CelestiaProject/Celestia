// celx_concurrency_test.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celestia/celestiacore.h>
#include <celutil/flag.h>
#include <celutil/logger.h>
#include <chrono>
#include <doctest.h>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <string_view>
#include <thread>
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

class ScriptFixture : private RestoreLogger, public CelestiaCore::Alerter
{
public:
    explicit ScriptFixture(std::string_view configuration = "Configuration {}")
    {
        std::random_device random;
        directory = std::filesystem::temp_directory_path()
                    / ("celestia-concurrent-scripts-" + std::to_string(random()) + "-"
                       + std::to_string(random()));
        REQUIRE(std::filesystem::create_directory(directory));
        core.setAlerter(this);
        auto config = write("test.cfg", configuration);
        REQUIRE(core.initSimulation(config, {}));
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

TEST_CASE("Invalid script arguments are rejected without launching a script or replacing the caller")
{
    ScriptFixture fixture;
    fixture.write("child.celx", "celestia:settimescale(999)");
    auto parent = fixture.write("parent.celx", R"(
        celestia:settimescale(7)
        local ok, message = pcall(function()
            celestia:runscript("child.celx", "unknown")
        end)
        assert(not ok)
        assert(string.find(message, "mode"))
        wait(1000)
    )");
    fixture.core.runScript(parent);
    fixture.tick();
    fixture.tick();
    CHECK(fixture.value() == 7);
    CHECK(fixture.errors.empty());
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

TEST_CASE("Permission requests pause all scripts until every prompt is answered")
{
    ScriptFixture fixture;
    fixture.core.setScriptSystemAccessPolicy(CelestiaCore::ScriptSystemAccessPolicy::Ask);
    auto prompt = fixture.write("prompt.celx", R"(
        celestia:requestsystemaccess()
        wait(0)
        assert(io == nil)
        celestia:settimescale(celestia:gettimescale() + 100)
    )");
    auto running = fixture.write("running.celx", R"(
        celestia:registereventhandler("tick", function(event)
            celestia:settimescale(celestia:gettimescale() + 10)
        end)
        while true do
            celestia:settimescale(celestia:gettimescale() + 1)
            wait(0)
        end
    )");
    auto flagChanger = fixture.write("flags.celx", R"(celestia:hide("stars"))");
    const auto flags = fixture.core.getRenderer()->getRenderFlags();
    fixture.core.runScript(running);
    fixture.tick();
    fixture.core.runScript(prompt, CelestiaCore::ScriptRunMode::Concurrent);
    fixture.core.runScript(prompt, CelestiaCore::ScriptRunMode::Concurrent);
    fixture.tick();
    CHECK(fixture.value() == 13);
    fixture.core.runScript(flagChanger, CelestiaCore::ScriptRunMode::Concurrent);
    fixture.tick();
    CHECK(fixture.value() == 13);
    CHECK(fixture.core.getRenderer()->getRenderFlags() == RenderFlags::ShowNothing);
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    fixture.core.charEntered('n');
    CHECK(fixture.core.getRenderer()->getRenderFlags() == flags);
    fixture.tick();
    CHECK(fixture.value() == 124);
    CHECK(fixture.core.getRenderer()->getRenderFlags() == RenderFlags::ShowNothing);
    fixture.core.charEntered('y');
    fixture.tick();
    CHECK(fixture.value() == 124);
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    fixture.core.charEntered('n');
    CHECK(fixture.core.getRenderer()->getRenderFlags() == flags);
    fixture.tick();
    CHECK(fixture.value() == 235);
    CHECK(fixture.core.getRenderer()->getRenderFlags() == (flags & ~RenderFlags::ShowStars));
    fixture.core.cancelScript();
    CHECK(fixture.core.getRenderer()->getRenderFlags() == (flags & ~RenderFlags::ShowStars));
    CHECK(fixture.errors.empty());
}

TEST_CASE("Permission requests from paused tick handlers can be answered without resuming scripts")
{
    ScriptFixture fixture;
    fixture.core.setScriptSystemAccessPolicy(CelestiaCore::ScriptSystemAccessPolicy::Ask);
    auto script = fixture.write("paused.celx", R"(
        local requested = false
        celestia:registereventhandler("tick", function(event)
            if celestia:ispaused() and not requested then
                requested = true
                celestia:requestsystemaccess()
            end
        end)
        wait(0)
        celestia:settimescale(77)
    )");
    const auto flags = fixture.core.getRenderer()->getRenderFlags();
    fixture.core.runScript(script);
    fixture.tick();
    fixture.core.charEntered(' ');
    REQUIRE(fixture.core.getSimulation()->getPauseState());
    fixture.tick();
    fixture.tick();
    CHECK(fixture.core.getRenderer()->getRenderFlags() == RenderFlags::ShowNothing);
    CHECK(fixture.value() == 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    fixture.core.charEntered('n');
    CHECK(fixture.core.getRenderer()->getRenderFlags() == flags);
    fixture.tick();
    CHECK(fixture.core.getSimulation()->getPauseState());
    CHECK(fixture.value() == 1);
    fixture.core.charEntered(' ');
    fixture.tick();
    CHECK_FALSE(fixture.core.getSimulation()->getPauseState());
    CHECK(fixture.value() == 77);
    CHECK(fixture.errors.empty());
}

TEST_SUITE_END();
