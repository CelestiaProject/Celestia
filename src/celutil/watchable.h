#pragma once

#include "watcher.h"
#include <vector>
#include <cassert>
#include <algorithm>

//namespace celutil
//{
template<typename T> class Watchable
{
 public:
    inline void addWatcher(Watcher<T>* watcher);
    inline void removeWatcher(Watcher<T>* watcher);
    void notifyWatchers(int) const;
 private:
    std::vector<Watcher<T>*> watchers;
};

template<typename T> void Watchable<T>::addWatcher(Watcher<T>* watcher)
{
    assert(watcher != nullptr);
    watchers.push_back(watcher);
}

template<typename T> void Watchable<T>::removeWatcher(Watcher<T>* watcher)
{
    std::remove_if(watchers.begin(), watchers.end(),
                   [&watcher](Watcher<T>* w) { return w == watcher; });
}

template<typename T> void Watchable<T>::notifyWatchers(int property) const
{
    for (const auto watcher : watchers)
        watcher->notifyChange(reinterpret_cast<const T*>(this), property);
}
//};
