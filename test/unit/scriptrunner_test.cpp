// scriptrunner_test.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celscript/common/scriptrunner.h>
#include <doctest.h>
#include <functional>
#include <memory>
#include <string_view>

namespace
{

struct ScriptState
{
    int ticks{ 0 };
    int events{ 0 };
    int keys{ 0 };
    bool finished{ false };
    bool wantsKeyboard{ false };
    bool destroyed{ false };
    double eventDt{ 0.0 };
    std::function<void()> onTick;
    std::function<void()> onEvent;
    std::function<bool(std::string_view)> onKey;
    std::function<void()> onDestroy;
};

class TestScript : public celestia::scripts::IScript
{
public:
    explicit TestScript(ScriptState &state) : m_state(state)
    {
    }
    ~TestScript() override
    {
        m_state.destroyed = true;
        if (m_state.onDestroy)
            m_state.onDestroy();
    }

    bool tick(double) override
    {
        ++m_state.ticks;
        if (m_state.onTick)
            m_state.onTick();
        return m_state.finished;
    }

    bool handleTickEvent(double dt) override
    {
        ++m_state.events;
        m_state.eventDt = dt;
        if (m_state.onEvent)
            m_state.onEvent();
        return false;
    }

    bool handleKeyEvent(const char *key) override
    {
        ++m_state.keys;
        return m_state.onKey && m_state.onKey(key);
    }

    bool charEntered(const char *key) override
    {
        return handleKeyEvent(key);
    }
    bool wantsKeyboard() const override
    {
        return m_state.wantsKeyboard;
    }

private:
    ScriptState &m_state;
};

} // namespace

TEST_SUITE_BEGIN("Script runner");

TEST_CASE("Replacement inside a tick keeps the callback alive and defers the new script")
{
    ScriptState first, canceled, replacement;
    celestia::scripts::ScriptRunner runner;
    first.onTick = [&]
    {
        runner.clear();
        CHECK_FALSE(first.destroyed);
        runner.add(std::make_unique<TestScript>(replacement));
        runner.tick(0.1);
    };
    runner.add(std::make_unique<TestScript>(first));
    runner.add(std::make_unique<TestScript>(canceled));
    runner.tick(0.1);
    CHECK(first.destroyed);
    CHECK(canceled.destroyed);
    CHECK(canceled.events == 0);
    CHECK(replacement.ticks == 0);
    runner.tick(0.1);
    CHECK(replacement.ticks == 1);
}

TEST_CASE("Cancellation context follows deferred cleanup and survives nested cleanup")
{
    ScriptState first, child;
    celestia::scripts::ScriptRunner runner;
    first.onTick = [&]
    {
        runner.clear();
        CHECK_FALSE(first.destroyed);
        runner.add(std::make_unique<TestScript>(child));
    };
    first.onDestroy = [&]
    {
        CHECK(runner.isCanceling());
        runner.clear();
        CHECK(child.destroyed);
        CHECK(runner.isCanceling());
    };
    child.onDestroy = [&] { CHECK(runner.isCanceling()); };
    runner.add(std::make_unique<TestScript>(first));
    runner.tick(0.1);
    CHECK(first.destroyed);
    CHECK(runner.empty());
    CHECK_FALSE(runner.isCanceling());
}

TEST_CASE("Normal completion cleanup may still launch another script")
{
    ScriptState first, child;
    celestia::scripts::ScriptRunner runner;
    first.finished = true;
    first.onDestroy = [&]
    {
        CHECK_FALSE(runner.isCanceling());
        runner.add(std::make_unique<TestScript>(child));
    };
    runner.add(std::make_unique<TestScript>(first));
    runner.tick(0.1);
    CHECK(first.destroyed);
    CHECK(child.ticks == 0);
    runner.tick(0.1);
    CHECK(child.ticks == 1);
}

TEST_CASE("Pause changes inside tick events apply to the active snapshot")
{
    ScriptState first, second;
    celestia::scripts::ScriptRunner runner;
    first.onEvent = [&] { runner.setPaused(true); };
    runner.add(std::make_unique<TestScript>(first));
    runner.add(std::make_unique<TestScript>(second));
    runner.tick(0.1);
    CHECK(first.ticks == 0);
    CHECK(second.ticks == 0);
    CHECK(second.eventDt == 0.0);
    CHECK(runner.allPaused());
    first.onEvent = {};
    runner.setPaused(false);
    runner.tick(0.1);
    CHECK(first.ticks == 1);
    CHECK(second.ticks == 1);
}

TEST_CASE("Input is offered newest first and consumed once")
{
    ScriptState first, second;
    second.onKey = [](std::string_view key) { return key == "a"; };
    celestia::scripts::ScriptRunner runner;
    runner.add(std::make_unique<TestScript>(first));
    runner.add(std::make_unique<TestScript>(second));
    CHECK(runner.handleKeyEvent("a"));
    CHECK(second.keys == 1);
    CHECK(first.keys == 0);
    CHECK_FALSE(runner.handleKeyEvent("b"));
    CHECK(second.keys == 2);
    CHECK(first.keys == 1);
}

TEST_CASE("Input replacement cannot destroy the active callback")
{
    ScriptState first, second, replacement;
    celestia::scripts::ScriptRunner runner;
    second.onKey = [&](std::string_view)
    {
        runner.clear();
        CHECK_FALSE(second.destroyed);
        runner.add(std::make_unique<TestScript>(replacement));
        return false;
    };
    runner.add(std::make_unique<TestScript>(first));
    runner.add(std::make_unique<TestScript>(second));
    CHECK_FALSE(runner.handleKeyEvent("a"));
    CHECK(first.keys == 0);
    CHECK(replacement.keys == 0);
    CHECK(second.destroyed);
}

TEST_CASE("Keyboard capture routes characters without pausing other scripts")
{
    ScriptState capture, running;
    celestia::scripts::ScriptRunner runner;
    capture.wantsKeyboard = true;
    capture.onKey = [](std::string_view key) { return key == "a"; };
    runner.add(std::make_unique<TestScript>(capture));
    runner.add(std::make_unique<TestScript>(running));
    runner.tick(0.1);
    CHECK(capture.ticks == 1);
    CHECK(running.ticks == 1);
    CHECK(runner.wantsKeyboard());
    CHECK(runner.charEntered("a"));
    CHECK(capture.keys == 1);
    CHECK(running.keys == 0);
    capture.wantsKeyboard = false;
    CHECK_FALSE(runner.wantsKeyboard());
    CHECK_FALSE(runner.charEntered("a"));
    CHECK(capture.keys == 1);
}

TEST_SUITE_END();
