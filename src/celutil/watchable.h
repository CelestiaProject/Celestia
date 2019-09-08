// watchable.h
//
// Copyright (C) 2019, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once
#include <cassert>
#include <algorithm>
#include <vector>
#include <celutil/watcher.h>

namespace celestia
{
namespace utility
{

template<typename T> class Watchable
{
    std::vector<Watcher<T>*> m_watchers;

 protected:
    inline const std::vector<Watcher<T>*>& watchers()
    {
        return m_watchers;
    }

 public:
    virtual void notifyWatchers(int = 0);
    virtual void addWatcher(Watcher<T>*);
    virtual void removeWatcher(Watcher<T>*);
};

template<typename T> void Watchable<T>::addWatcher(Watcher<T>* watcher)
{
    assert(watcher != nullptr);
    auto w = std::find(m_watchers.cbegin(), m_watchers.cend(), watcher);
    if (w == m_watchers.cend())
    {
        m_watchers.push_back(watcher);
        watcher->notifyChange();
    }
}

template<typename T> void Watchable<T>::removeWatcher(Watcher<T>* watcher)
{
    assert(watcher != nullptr);
    std::remove_if(m_watchers.begin(), m_watchers.end(),
                   [&watcher](Watcher<T>* w) { return w == watcher; });
}

template<typename T> void Watchable<T>::notifyWatchers(int property)
{
    for (auto *watcher : m_watchers)
        watcher->notifyChange(property);
}

}
}
