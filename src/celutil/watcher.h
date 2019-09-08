// watcher.h
//
// Copyright (C) 2019, Celestia Development Team
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once
#include <memory>
#include <iostream>

namespace celestia
{
namespace utility
{

template<typename T> class Watcher
{
 private:
    std::shared_ptr<T> m_watched;

 protected:
    const std::shared_ptr<T>& watched()
    {
        return m_watched;
    }

 public:
    Watcher() = default;
    Watcher(const std::shared_ptr<T>& watched) :
        m_watched(watched)
    {
        m_watched->addWatcher(this);
    };

    virtual ~Watcher()
    {
        if (m_watched != nullptr)
            m_watched->removeWatcher(this);
    }

    virtual void notifyChange(int = 0) /*= 0*/{std::cout << "Watcher::notifyChange()\n";};
};

}
}
