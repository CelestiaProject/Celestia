// array_view.h
//
// Copyright (C) 2021-present, Celestia Development Team.
//
// Read-only view of array-like containers.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <vector>
#include <type_traits> // std::remove_cv
#include <cstddef>     // std::size_t

namespace celestia::util
{

/**
 * Read-only view of array-like containers similar to C++20's std::span.
 */
template<typename T, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<T>, void>>>
class array_view
{
public:
    using element_type = std::remove_cv_t<T>;
    using size_type = std::size_t;

    /**
     * Create an empty array view.
     */
    constexpr array_view() noexcept :
        m_ptr(nullptr),
        m_size(0)
    {}

    /**
     * Wrap a pointer and length
     */
    constexpr array_view(const T* ptr, size_type size) noexcept :
        m_ptr(ptr),
        m_size(size)
    {}

    /**
     * Wrap a C-style array.
     */
    template<std::size_t N> constexpr array_view(const T (&ary)[N]) noexcept : //NOSONAR
        m_ptr(ary),
        m_size(N)
    {}

    /**
     * Wrap a std::array or std::vector or other classes which have the same
     * memory layout and interface.
     */
    template<std::size_t N> constexpr array_view(const std::array<element_type, N> &ary) noexcept :
        m_ptr(ary.data()),
        m_size(N)
    {}

    /** @copydoc array_view::array_view((const std::array<T, N> &ary) */
    constexpr array_view(const std::vector<element_type> &vec) noexcept :
        m_ptr(vec.data()),
        m_size(vec.size())
    {}

    /**
     * Direct access to the underlying array.
     */
    constexpr const element_type* data() const noexcept
    {
        return m_ptr;
    }

    /**
     * Return the number of elements.
     */
    constexpr size_type size() const noexcept
    {
        return m_size;
    }

    /**
     * Check whether the view is empty.
     */
    constexpr bool empty() const noexcept
    {
        return m_size == 0;
    }

    /**
     * Return the first element.
     *
     * Calling front on an empty container is undefined.
     */
    constexpr const T& front() const noexcept
    {
        return *m_ptr;
    }

    /**
     * Return the last element.
     *
     * Calling back on an empty container is undefined.
     */
    constexpr const T& back() const noexcept
    {
        return m_ptr[m_size - 1];
    }

    /**
     * Return array element at specified position. No bounds checking
     * performed.
     *
     * Accessing a nonexistent element through this operator is
     * undefined behavior.
     *
     * @param pos - position of the element to return
     * @return the requested element
     */
    constexpr const T& operator[](std::size_t pos) const noexcept
    {
        return m_ptr[pos];
    }

    /**
     * Return an iterator to the beginning.
     */
    constexpr const element_type* begin() const noexcept
    {
        return m_ptr;
    }

    /**
     * Return an iterator to the beginning.
     */
    constexpr const element_type* cbegin() const noexcept
    {
        return begin();
    }

    /**
     * Return an iterator to the end.
     */
    constexpr const element_type* end() const noexcept
    {
        return m_ptr + m_size;
    }

    /**
     * Return an iterator to the end.
     */
    constexpr const element_type* cend() const noexcept
    {
        return end();
    }

private:
    const element_type* m_ptr;
    size_type m_size;
};

template<typename T>
inline array_view<std::byte> byte_view(const array_view<T>& view) noexcept
{
    return array_view<std::byte>(reinterpret_cast<const std::byte*>(view.data()), //NOSONAR
                                 view.size() * sizeof(T));
}

template<typename T>
inline array_view<std::byte> byte_view(const T* ptr, std::size_t size) noexcept
{
    return array_view<std::byte>(reinterpret_cast<const std::byte*>(ptr), //NOSONAR
                                 size * sizeof(T));
}

template<typename T>
inline array_view<std::byte> byte_view(const std::vector<T>& vec) noexcept
{
    return array_view<std::byte>(reinterpret_cast<const std::byte*>(vec.data()), //NOSONAR
                                 vec.size() * sizeof(T));
}

template<typename T, std::size_t N>
inline array_view<std::byte> byte_view(const T (&ary)[N]) noexcept //NOSONAR
{
    return array_view<std::byte>(reinterpret_cast<const std::byte*>(ary), //NOSONAR
                                 N * sizeof(T));
}

template<typename T, std::size_t N>
inline array_view<std::byte> byte_view(const std::array<T, N>& arr) noexcept
{
    return array_view<std::byte>(reinterpret_cast<const std::byte*>(arr.data()), //NOSONAR
                                 arr.size() * sizeof(T));
}

} // end namespace celestia::util
