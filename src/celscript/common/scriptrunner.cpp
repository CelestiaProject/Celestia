// scriptrunner.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "scriptrunner.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace celestia::scripts
{

namespace
{

class FlagGuard
{
public:
    explicit FlagGuard(bool &flag) : m_flag(flag), m_previous(std::exchange(flag, true))
    {
    }
    FlagGuard(const FlagGuard &) = delete;
    FlagGuard &operator=(const FlagGuard &) = delete;

    ~FlagGuard()
    {
        m_flag = m_previous;
    }

private:
    bool &m_flag;
    bool m_previous;
};

} // namespace

ScriptRunner::Entry::Entry(ScriptRunner &runner, std::unique_ptr<IScript> value, State initial) :
    owner(runner), script(std::move(value)), state(initial), position(runner.m_scripts.end())
{
}

ScriptRunner::Entry::~Entry()
{
    if (canceled)
    {
        FlagGuard guard(owner.m_canceling);
        script.reset();
    }
}

ScriptRunner::~ScriptRunner()
{
    clear();
}

void
ScriptRunner::add(std::unique_ptr<IScript> script, State state)
{
    if (script == nullptr)
        return;
    auto entry = std::make_shared<Entry>(*this, std::move(script), state);
    entry->position = m_scripts.insert(m_scripts.end(), entry);
    if (m_dispatching)
        checkExclusive(entry);
}

void
ScriptRunner::clear()
{
    auto scripts = std::exchange(m_scripts, {});
    m_exclusiveScript.reset();
    // Snapshot references may defer cleanup until the active callback returns.
    for (const auto &entry : scripts)
        entry->canceled = true;
}

void
ScriptRunner::checkExclusive(const std::shared_ptr<Entry> &entry)
{
    if (!entry->canceled && m_exclusiveScript.expired() && entry->script->wantsExclusiveInput())
        m_exclusiveScript = entry;
}

void
ScriptRunner::setPaused(bool paused) const
{
    for (const auto &entry : m_scripts)
        entry->state = paused ? State::Paused : State::Running;
}

bool
ScriptRunner::allPaused() const
{
    return !m_scripts.empty()
        && std::ranges::all_of(m_scripts, [](const auto &entry) { return entry->state == State::Paused; });
}

bool
ScriptRunner::wantsKeyboard() const
{
    return std::ranges::any_of(m_scripts, [](const auto &entry) { return entry->script->wantsKeyboard(); });
}

bool
ScriptRunner::wantsExclusiveInput() const
{
    return std::ranges::any_of(m_scripts, [](const auto &entry) { return entry->script->wantsExclusiveInput(); });
}

std::shared_ptr<ScriptRunner::Entry>
ScriptRunner::exclusiveScript()
{
    auto entry = m_exclusiveScript.lock();
    if (entry != nullptr && !entry->canceled && entry->script->wantsExclusiveInput())
        return entry;

    auto it = std::ranges::find_if(m_scripts, [](const auto &candidate) { return candidate->script->wantsExclusiveInput(); });
    entry = it != m_scripts.end() ? *it : nullptr;
    m_exclusiveScript = entry;
    return entry;
}

void
ScriptRunner::tickEntry(const std::shared_ptr<Entry> &entry, double dt)
{
    if (entry->canceled)
        return;

    entry->script->handleTickEvent(entry->state == State::Paused ? 0.0 : dt);
    checkExclusive(entry);
    if (!m_exclusiveScript.expired() || entry->canceled || entry->state != State::Running)
        return;

    if (entry->script->tick(dt))
    {
        if (!entry->canceled)
            m_scripts.erase(entry->position);
    }
    else
    {
        checkExclusive(entry);
    }
}

void
ScriptRunner::tick(double dt)
{
    if (m_dispatching)
        return;

    FlagGuard guard(m_dispatching);
    // Keep callbacks alive if they cancel/replace scripts, and defer new scripts
    // until the next frame.
    auto exclusive = exclusiveScript();
    if (!exclusive)
    {
        std::vector scripts(m_scripts.begin(), m_scripts.end());
        for (auto &entry : scripts)
        {
            tickEntry(entry, dt);
            entry.reset();
            if (!m_exclusiveScript.expired())
                break;
        }
        exclusive = m_exclusiveScript.lock();
    }

    // Advance only the active permission prompt, without resuming script execution.
    if (exclusive != nullptr && exclusive->script->tick(0.0) && !exclusive->canceled)
    {
        m_scripts.erase(exclusive->position);
    }
}

template<typename F>
bool
ScriptRunner::dispatch(F &&callback, bool exclusiveCharacters)
{
    if (m_dispatching)
        return true;

    FlagGuard guard(m_dispatching);
    if (auto exclusive = exclusiveScript(); exclusive != nullptr)
    {
        if (exclusiveCharacters)
            callback(*exclusive->script);
        return true;
    }

    std::vector scripts(m_scripts.begin(), m_scripts.end());
    for (auto it = scripts.rbegin(); it != scripts.rend(); ++it)
    {
        if (!(*it)->canceled && callback(*(*it)->script))
            return true;
        checkExclusive(*it);
        if (!m_exclusiveScript.expired())
            return true;
    }
    return false;
}

bool
ScriptRunner::charEntered(const char *text)
{
    return dispatch([text](IScript &script) { return script.wantsKeyboard() && script.charEntered(text); }, true);
}

bool
ScriptRunner::handleKeyEvent(const char *key)
{
    return dispatch([key](IScript &script) { return script.handleKeyEvent(key); });
}

bool
ScriptRunner::handleMouseButtonEvent(float x, float y, int button, bool down)
{
    return dispatch([x, y, button, down](IScript &script) { return script.handleMouseButtonEvent(x, y, button, down); });
}

} // namespace celestia::scripts
