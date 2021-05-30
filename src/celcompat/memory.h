#pragma once

#include <config.h>
#include <memory>

#ifndef HAVE_MAKE_UNIQUE
namespace std
{
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
}
#endif
