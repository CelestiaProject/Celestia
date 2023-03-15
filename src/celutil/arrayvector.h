// Copyright (C) 2023 The Celestia Development Team
// Original version by Andrew Tribick
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace celestia::util
{

template<typename T, std::size_t N,
         std::enable_if_t<std::is_default_constructible_v<T>, int> = 0>
class ArrayVector
{
public:
    using value_type             = T;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using pointer                = T*;
    using const_pointer          = const T*;
    using reference              = T&;
    using const_reference        = const T&;
    using iterator               = typename std::array<T, N>::iterator;
    using const_iterator         = typename std::array<T, N>::const_iterator;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    ArrayVector() = default;

    iterator               begin()         noexcept { return m_data.begin(); }
    const_iterator         begin()   const noexcept { return m_data.begin(); }
    const_iterator         cbegin()  const noexcept { return m_data.cbegin(); }
    iterator               end()           noexcept { return m_data.begin() + m_size; }
    const_iterator         end()     const noexcept { return m_data.begin() + m_size; }
    const_iterator         cend()    const noexcept { return m_data.cbegin() + m_size; }
    reverse_iterator       rbegin()        noexcept { return std::reverse_iterator(end()); }
    const_reverse_iterator rbegin()  const noexcept { return std::reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return std::reverse_iterator(cend()); }
    reverse_iterator       rend()          noexcept { return std::reverse_iterator(begin()); }
    const_reverse_iterator rend()    const noexcept { return std::reverse_iterator(begin()); }
    const_reverse_iterator crend()   const noexcept { return std::reverse_iterator(cbegin()); }

    reference       operator[](size_type idx)       { return m_data[idx]; }
    const_reference operator[](size_type idx) const { return m_data[idx]; }

    reference       front()       { return m_data[0]; }
    const_reference front() const { return m_data[0]; }
    reference       back()        { return m_data[m_size - 1]; }
    const_reference back()  const { return m_data[m_size - 1]; }

    pointer       data()       noexcept { return m_data.data(); }
    const_pointer data() const noexcept { return m_data.data(); }

    bool      empty()    const noexcept { return m_size == 0; }
    size_type size()     const noexcept { return m_size; }

    constexpr size_type max_size() const noexcept { return N; }
    constexpr size_type capacity() const noexcept { return N; }

    void clear()
    {
        std::fill(begin(), end(), T());
        m_size = 0;
    }

    bool try_push_back(const T& value)
    {
        if (m_size == N)
            return false;

        m_data[m_size] = value;
        ++m_size;
        return true;
    }

    bool try_push_back(T&& value)
    {
        if (m_size == N)
            return false;

        m_data[m_size] = std::move(value);
        ++m_size;
        return true;
    }

    void pop_back()
    {
        --m_size;
        m_data[m_size] = T();
    }

    void resize(size_type count)
    {
        if (count < m_size)
            std::fill(begin() + count, end(), T());
        else if (count > m_size)
            std::fill(end(), begin() + count, T());
        m_size = count;
    }

    iterator erase(const_iterator pos)
    {
        auto idx = static_cast<size_type>(pos - cbegin());
        if (idx == m_size - 1)
        {
            pop_back();
            return end();
        }

        auto dest = begin() + idx;
        std::move(begin() + idx + 1, end(), dest);
        --m_size;
        return dest;
    }

    iterator erase(const_iterator first, const_iterator last)
    {
        auto firstIdx = static_cast<size_type>(first - cbegin());
        auto lastIdx = static_cast<size_type>(last - cbegin());
        if (firstIdx == lastIdx)
            return begin() + lastIdx;

        if (lastIdx == m_size)
        {
            resize(firstIdx);
            return end();
        }

        auto dest = begin() + firstIdx;
        std::move(begin() + lastIdx, end(), begin() + firstIdx);
        resize(m_size - (last - first));
        return dest;
    }

    // insert not implemented so far

    void swap(ArrayVector& other) noexcept
    {
        using std::swap;
        swap(m_size, other.m_size);
        swap(m_data, other.m_data);
    }

private:
    size_type        m_size{ 0 };
    std::array<T, N> m_data{};
};


template<typename T, std::size_t N1, std::size_t N2>
inline bool operator==(const ArrayVector<T, N1>& lhs, const ArrayVector<T, N2>& rhs)
{
    return lhs.size() == rhs.size() &&
           std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}


template<typename T, std::size_t N1, std::size_t N2>
inline bool operator!=(const ArrayVector<T, N1>& lhs, const ArrayVector<T, N2>& rhs)
{
    return !(lhs == rhs);
}


template<typename T, std::size_t N1, std::size_t N2>
inline bool operator<(const ArrayVector<T, N1>& lhs, const ArrayVector<T, N2>& rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}


template<typename T, std::size_t N1, std::size_t N2>
inline bool operator>(const ArrayVector<T, N1>& lhs, const ArrayVector<T, N2>& rhs)
{
    return rhs < lhs;
}


template<typename T, std::size_t N1, std::size_t N2>
inline bool operator<=(const ArrayVector<T, N1>& lhs, const ArrayVector<T, N2>& rhs)
{
    return !(rhs < lhs);
}


template<typename T, std::size_t N1, std::size_t N2>
inline bool operator>=(const ArrayVector<T, N1>& lhs, const ArrayVector<T, N2>& rhs)
{
    return !(lhs < rhs);
}


template<typename T, std::size_t N>
inline void swap(ArrayVector<T, N>& lhs, ArrayVector<T, N>& rhs) noexcept
{
    lhs.swap(rhs);
}

} // end namespace celestia::util
