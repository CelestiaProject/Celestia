// Copyright (C) 2023 The Celestia Development Team
// Original version by Andrew Tribick
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

namespace celestia::util
{

//**
//* \class IntrusivePtr intrusiveptr.h celutil/intrusiveptr.h
//*
//* @brief A basic intrusive pointer class.
//*
//* @details An implementation of a shared pointer where the reference count
//* is stored in the object. The implementation relies on `intrusiveAddRef()`
//* and `intrusiveRemoveRef()` methods on the object to manipulate the
//* reference count. The `intrusiveRemoveRef()` should return zero if and only
//* if the reference count is zero.
template<typename T>
class IntrusivePtr
{
public:
    using element_type = std::remove_extent_t<T>;

    IntrusivePtr() noexcept = default;
    IntrusivePtr(std::nullptr_t) noexcept {}

    explicit IntrusivePtr(T* ptr) : m_ptr(ptr)
    {
        addRef();
    }

    template<typename Y, std::enable_if_t<std::is_convertible_v<Y*, T*>, int> = 0>
    explicit IntrusivePtr(Y* ptr) : m_ptr(ptr)
    {
        addRef();
    }

    ~IntrusivePtr()
    {
        removeRef();
    }

    IntrusivePtr(const IntrusivePtr& other) : m_ptr(other.m_ptr)
    {
        addRef();
    }

    IntrusivePtr& operator=(const IntrusivePtr& other)
    {
        if (m_ptr != other.m_ptr)
        {
            removeRef();
            m_ptr = other.m_ptr;
            addRef();
        }

        return *this;
    }

    template<typename Y, std::enable_if_t<std::is_convertible_v<Y*, T*>, int> = 0>
    // Similarly to the equivalent unique_ptr/shared_ptr constructor, this is
    // intentionally left non-explicit, so disable the Sonar lint.
    IntrusivePtr(const IntrusivePtr<Y>& other) : m_ptr(other.m_ptr) //NOSONAR
    {
        addRef();
    }

    template<typename Y, std::enable_if_t<std::is_convertible_v<Y*, T*>, int> = 0>
    IntrusivePtr& operator=(const IntrusivePtr<Y>& other)
    {
        if (m_ptr != other.m_ptr)
        {
            removeRef();
            m_ptr = other.m_ptr;
            addRef();
        }

        return *this;
    }

    IntrusivePtr(IntrusivePtr&& other) noexcept : m_ptr(other.m_ptr)
    {
        other.m_ptr = nullptr;
    }

    IntrusivePtr& operator=(IntrusivePtr&& other) noexcept
    {
        removeRef();
        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;
        return *this;
    }

    template<typename Y, std::enable_if_t<std::is_convertible_v<Y*, T*>, int> = 0>
    // Similarly to the equivalent unique_ptr/shared_ptr constructor, this is
    // intentionally left non-explicit, so disable the Sonar lint.
    IntrusivePtr(IntrusivePtr<Y>&& other) noexcept : m_ptr(other.m_ptr) //NOSONAR
    {
        other.m_ptr = nullptr;
    }

    template<typename Y, std::enable_if_t<std::is_convertible_v<Y*, T*>, int> = 0>
    // Move is implemented by manipulating pointer members directly, disable
    // the Sonar lint requiring explicit use of std::move.
    IntrusivePtr& operator=(IntrusivePtr<Y>&& other) noexcept //NOSONAR
    {
        removeRef();
        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;
        return *this;
    }

    void reset() noexcept
    {
        removeRef();
        m_ptr = nullptr;
    }

    void swap(IntrusivePtr& other) noexcept
    {
        using std::swap;
        swap(m_ptr, other.m_ptr);
    }

    inline element_type* get() const noexcept { return m_ptr; }
    inline T& operator*()      const noexcept { return *m_ptr; }
    inline T* operator->()     const noexcept { return m_ptr; }

    explicit operator bool() const noexcept { return m_ptr != nullptr; }

private:
    inline void addRef()
    {
        if (m_ptr != nullptr)
            m_ptr->intrusiveAddRef();
    }

    inline void removeRef()
    {
        if (m_ptr != nullptr && m_ptr->intrusiveRemoveRef() == 0)
            delete m_ptr;
    }

    T* m_ptr{ nullptr };

    template<typename Y>
    friend class IntrusivePtr;
};

template<typename T>
inline void swap(IntrusivePtr<T>& lhs, IntrusivePtr<T>& rhs)
{
    lhs.swap(rhs);
}

template<typename T, typename U>
inline bool operator==(const IntrusivePtr<T>& lhs, const IntrusivePtr<U>& rhs) noexcept { return lhs.get() == rhs.get(); }

template<typename T, typename U>
inline bool operator!=(const IntrusivePtr<T>& lhs, const IntrusivePtr<U>& rhs) noexcept { return !(lhs == rhs); }

template<typename T, typename U>
inline bool operator<(const IntrusivePtr<T>& lhs, const IntrusivePtr<U>& rhs) noexcept { return std::less<std::common_type_t<T*, U*>>()(lhs.get(), rhs.get()); }

template<typename T, typename U>
inline bool operator>(const IntrusivePtr<T>& lhs, const IntrusivePtr<U>& rhs) noexcept { return rhs < lhs; }

template<typename T, typename U>
inline bool operator<=(const IntrusivePtr<T>& lhs, const IntrusivePtr<U>& rhs) noexcept { return !(rhs < lhs); }

template<typename T, typename U>
inline bool operator>=(const IntrusivePtr<T>& lhs, const IntrusivePtr<U>& rhs) noexcept { return !(lhs < rhs); }

template<typename T>
inline bool operator==(const IntrusivePtr<T>& lhs, std::nullptr_t) noexcept { return lhs.get() == nullptr; }

template<typename T>
inline bool operator==(std::nullptr_t, const IntrusivePtr<T>& rhs) noexcept { return nullptr == rhs.get(); }

template<typename T>
inline bool operator!=(const IntrusivePtr<T>& lhs, std::nullptr_t) noexcept { return !(lhs == nullptr); }

template<typename T>
inline bool operator!=(std::nullptr_t, const IntrusivePtr<T>& rhs) noexcept { return !(nullptr == rhs); }

template<typename T>
inline bool operator<(const IntrusivePtr<T>& lhs, std::nullptr_t) noexcept { return std::less<T*>()(lhs.get(), nullptr); }

template<typename T>
inline bool operator<(std::nullptr_t, const IntrusivePtr<T>& rhs) noexcept { return std::less<T*>()(nullptr, rhs.get()); }

template<typename T>
inline bool operator>(const IntrusivePtr<T>& lhs, std::nullptr_t) noexcept { return nullptr < lhs; }

template<typename T>
inline bool operator>(std::nullptr_t, const IntrusivePtr<T>& rhs) noexcept { return rhs < nullptr; }

template<typename T>
inline bool operator<=(const IntrusivePtr<T>& lhs, std::nullptr_t) noexcept { return !(nullptr < lhs); }

template<typename T>
inline bool operator<=(std::nullptr_t, const IntrusivePtr<T>& rhs) noexcept { return !(rhs < nullptr); }

template<typename T>
inline bool operator>=(const IntrusivePtr<T>& lhs, std::nullptr_t) noexcept { return !(lhs < nullptr); }

template<typename T>
inline bool operator>=(std::nullptr_t, const IntrusivePtr<T>& rhs) noexcept { return !(nullptr < rhs); }

} // end namespace celestia::util
