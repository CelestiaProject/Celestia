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
#include <cstddef> // std::size_t
#include <vector>
#include <type_traits>

namespace celestia::util
{

/**
 * Read-only view of array-like containers similar to C++20's std::span.
 */
template<typename T>
class array_view
{
public:
    using element_type = std::remove_cv_t<T>;
    using size_type = std::size_t;

    /**
     * Create an empty array view.
     */
    constexpr array_view() noexcept = default;

    /**
     * Wrap a pointer and length
     */
    template<typename U,
             std::enable_if_t<std::is_convertible_v<U(*)[], const element_type(*)[]>, int> = 0>
    constexpr array_view(const U* ptr, size_type size) noexcept :
        m_ptr(ptr),
        m_size(size)
    {}

    /**
     * Wrap a C-style array.
     */
    template<typename U, std::size_t N,
             std::enable_if_t<std::is_convertible_v<U(*)[], const element_type(*)[]>, int> = 0>
    constexpr array_view(const U (&ary)[N]) noexcept : //NOSONAR
        m_ptr(ary),
        m_size(N)
    {}

    /**
     * Wrap a std::array or std::vector or other classes which have the same
     * memory layout and interface.
     */
    template<typename U, std::size_t N,
             std::enable_if_t<std::is_convertible_v<U(*)[], const element_type(*)[]>, int> = 0>
    constexpr array_view(const std::array<U, N> &ary) noexcept : //NOSONAR
        m_ptr(ary.data()),
        m_size(N)
    {}

    /** @copydoc array_view::array_view((const std::array<T, N> &ary) */
    template<typename U,
             std::enable_if_t<std::is_convertible_v<U(*)[], const element_type(*)[]>, int> = 0>
    constexpr array_view(const std::vector<U> &vec) noexcept : //NOSONAR
        m_ptr(vec.data()),
        m_size(vec.size())
    {}

    /**
     * Copy another view.
     */
    constexpr array_view(const array_view&) = default;

    /**
     * Copy another view of compatible type
     */
    template<typename U,
             std::enable_if_t<!std::is_same_v<std::remove_cv_t<U>, void> &&
                              !std::is_same_v<std::remove_cv_t<U>, element_type> &&
                              std::is_convertible_v<std::remove_cv_t<U>(*)[], const element_type(*)[]>, int> = 0>
    constexpr array_view(const array_view<U>& other) noexcept : //NOSONAR
        m_ptr(other.m_ptr),
        m_size(other.m_size)
    {
    }

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
    const element_type* m_ptr{ nullptr };
    size_type m_size{ 0 };
};

/**
 * Specialization for API accepting opaque blobs of data.
 */
template <>
class array_view<void>
{
public:
    using element_type = void;
    using size_type = std::size_t;

    /**
     * Create an empty array view.
     */
    constexpr array_view() noexcept = default;

    /**
     * Wrap a pointer and length
     */
    constexpr array_view(const void* ptr, size_type size) noexcept :
        m_ptr(ptr),
        m_size(size)
    {}

    /**
     * Wrap a C-style array.
     */
    template<typename T, std::size_t N>
    constexpr array_view(const T (&ary)[N]) noexcept : //NOSONAR
        m_ptr(ary),
        m_size(N * sizeof(T))
    {}

    /**
     * Wrap a std::array or std::vector or other classes which have the same
     * memory layout and interface.
     */
    template<typename T, std::size_t N>
    constexpr array_view(const std::array<T, N> &ary) noexcept : //NOSONAR
        m_ptr(ary.data()),
        m_size(N * sizeof(T))
    {}

    /** @copydoc array_view::array_view((const std::array<T, N> &ary) */
    template<typename T>
    constexpr array_view(const std::vector<T> &vec) noexcept : //NOSONAR
        m_ptr(vec.data()),
        m_size(vec.size() * sizeof(T))
    {}

    /**
     * Copy another view.
     */
    constexpr array_view(const array_view&) = default;

    /**
     * Copy another view of concrete type.
     */
    template<typename T,
             std::enable_if_t<!std::is_same_v<std::remove_cv_t<T>, void>, int> = 0>
    constexpr array_view(const array_view<T> &ary) : //NOSONAR
        m_ptr(ary.data()),
        m_size(ary.size() * sizeof(T))
    {}

    /**
     * Assign another view.
     */
    constexpr array_view& operator=(const array_view&) = default;

    /**
     * Assign from another view of concrete type.
     */
    template<typename T,
             std::enable_if_t<!std::is_same_v<std::remove_cv_t<T>, void>, int> = 0>
    constexpr array_view& operator=(const array_view<T> &ary)
    {
        m_ptr = ary.data();
        m_size = ary.size() * sizeof(T);
        return *this;
    }

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

private:
    const element_type* m_ptr{ nullptr };
    size_type m_size{ 0 };
};

template<typename T>
array_view(const T*, std::size_t) -> array_view<std::remove_cv_t<T>>;

template<typename T, std::size_t N>
array_view(const T (&)[N]) -> array_view<std::remove_cv_t<T>>;

template<typename T, std::size_t N>
array_view(const std::array<T, N> &) -> array_view<std::remove_cv_t<T>>;

template<typename T>
array_view(const std::vector<T> &) -> array_view<std::remove_cv_t<T>>;

} // end namespace celestia::util
