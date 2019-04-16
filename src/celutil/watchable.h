#pragma once

#include "watcher.h"
#include <vector>
#include <cassert>

//namespace celutil
//{
template<typename T> class Watchable
{
 public:
    void addWatcher(Watcher<T>* watcher);
    void removeWatcher(Watcher<T>* watcher);
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
    auto iter = find(watchers.begin(), watchers.end(), watcher);
    if (iter != watchers.end())
        watchers.erase(iter);
}

template<typename T> void Watchable<T>::notifyWatchers(int property) const
{
    for (const auto watcher : watchers)
        watcher->notifyChange(reinterpret_cast<const T*>(this), property);
}
//};
