
// blockarray.h
//
// Copyright (C) 2001-2020, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include <boost/container/static_vector.hpp>

/*! BlockArray is a container class that is similar to an STL vector
 *  except for two very important differences:
 *  - The elements of a BlockArray are not necessarily in one
 *    contiguous block of memory.
 *  - The address of a BlockArray element is guaranteed not to
 *    change over the lifetime of the BlockArray (or until the
 *    BlockArray is cleared.)
 */
template<typename T, std::size_t BLOCKSIZE = 1024>
class BlockArray
{
public:
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using difference_type = std::ptrdiff_t;
    using size_type = std::size_t;

    static_assert(BLOCKSIZE > 0, "BLOCKSIZE must be greater than zero");

    class const_iterator;

    class iterator
    {
    public:
        using difference_type = BlockArray::difference_type;
        using value_type = BlockArray::value_type;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::random_access_iterator_tag;

        iterator() = default;

        reference operator*() const { return (*m_array)[m_position]; }
        pointer operator->() const { return &(*m_array)[m_position]; }
        reference operator[](difference_type n) { return (*m_array)[m_position + n]; }

        iterator& operator++() { ++m_position; return *this; }
        iterator operator++(int) { iterator result = *this; ++m_position; return result; }
        iterator& operator--() { --m_position; return *this; }
        iterator operator--(int) { iterator result = *this; --m_position; return result; }

        friend bool operator==(const iterator& lhs, const iterator& rhs) { return lhs.m_array == rhs.m_array && lhs.m_position == rhs.m_position; }
        friend bool operator!=(const iterator& lhs, const iterator& rhs) { return !(lhs == rhs); }
        friend bool operator<(const iterator& lhs, const iterator& rhs) { return lhs.m_position < rhs.m_position; }
        friend bool operator>(const iterator& lhs, const iterator& rhs) { return lhs.m_position > rhs.m_position; }
        friend bool operator<=(const iterator& lhs, const iterator& rhs) { return lhs.m_position <= rhs.m_position; }
        friend bool operator>=(const iterator& lhs, const iterator& rhs) { return lhs.m_position >= rhs.m_position; }

        iterator& operator+=(difference_type n) { m_position += n; return *this; }
        iterator& operator-=(difference_type n) { m_position -= n; return *this; }
        friend iterator operator+(const iterator& lhs, difference_type rhs) { iterator result = lhs; result += rhs; return result; }
        friend iterator operator+(difference_type lhs, const iterator& rhs) { iterator result = rhs; result += lhs; return result; }
        friend iterator operator-(const iterator& lhs, difference_type rhs) { iterator result = lhs; result -= rhs; return result; }
        friend difference_type operator-(const iterator& lhs, const iterator& rhs) { return lhs.m_position - rhs.m_position; }

    private:
        iterator(BlockArray* array, std::size_t position) :
            m_array(array), m_position(position)
        {
        }

        BlockArray* m_array{ nullptr };
        difference_type m_position{ 0 };

        friend class BlockArray;
        friend class const_iterator;
    };

    class const_iterator
    {
    public:
        using difference_type = BlockArray::difference_type;
        using value_type = const BlockArray::value_type;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::random_access_iterator_tag;

        const_iterator() = default;
        const_iterator(const iterator& other) : //NOSONAR
            m_array(other.m_array), m_position(other.m_position)
        {
        }

        reference operator*() const { return (*m_array)[m_position]; }
        pointer operator->() const { return &(*m_array)[m_position]; }
        reference operator[](difference_type n) { return (*m_array)[m_position + n]; }

        const_iterator& operator++() { ++m_position; return *this; }
        const_iterator operator++(int) { const_iterator result = *this; ++m_position; return result; }
        const_iterator& operator--() { --m_position; return *this; }
        const_iterator operator--(int) { const_iterator result = *this; --m_position; return result; }

        friend bool operator==(const const_iterator& lhs, const const_iterator& rhs) { return lhs.m_array == rhs.m_array && lhs.m_position == rhs.m_position; }
        friend bool operator!=(const const_iterator& lhs, const const_iterator& rhs) { return !(lhs == rhs); }
        friend bool operator<(const const_iterator& lhs, const const_iterator& rhs) { return lhs.m_position < rhs.m_position; }
        friend bool operator>(const const_iterator& lhs, const const_iterator& rhs) { return lhs.m_position > rhs.m_position; }
        friend bool operator<=(const const_iterator& lhs, const const_iterator& rhs) { return lhs.m_position <= rhs.m_position; }
        friend bool operator>=(const const_iterator& lhs, const const_iterator& rhs) { return lhs.m_position >= rhs.m_position; }

        const_iterator& operator+=(difference_type n) { m_position += n; return *this; }
        const_iterator& operator-=(difference_type n) { m_position -= n; return *this; }
        friend const_iterator operator+(const const_iterator& lhs, difference_type rhs) { const_iterator result = lhs; result += rhs; return result; }
        friend const_iterator operator+(difference_type lhs, const const_iterator& rhs) { const_iterator result = rhs; result += lhs; return result; }
        friend const_iterator operator-(const const_iterator& lhs, difference_type rhs) { const_iterator result = lhs; result -= rhs; return result; }
        friend difference_type operator-(const const_iterator& lhs, const const_iterator& rhs) { return lhs.m_position - rhs.m_position; }

    private:
        const_iterator(const BlockArray* array, std::size_t position) :
            m_array(array), m_position(position)
        {
        }

        const BlockArray* m_array{ nullptr };
        difference_type m_position{ 0 };

        friend class BlockArray;
    };

    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    size_type size() const noexcept
    {
        if (m_blocks.empty())
            return 0;
        return ((m_blocks.size() - 1) * BLOCKSIZE) + m_blocks.back()->size();
    }

    size_type max_size() const noexcept { return static_cast<size_type>(std::numeric_limits<difference_type>::max()); }
    bool empty() const noexcept { return m_blocks.empty(); }

    iterator begin() noexcept { return iterator(this, 0); }
    const_iterator begin() const noexcept { return const_iterator(this, 0); }
    const_iterator cbegin() const noexcept { return const_iterator(this, 0); }
    iterator end() noexcept { return iterator(this, size()); }
    const_iterator end() const noexcept { return const_iterator(this, size()); }
    const_iterator cend() const noexcept { return const_iterator(this, size()); }

    reverse_iterator rbegin() noexcept { return std::reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept { return std::reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return std::reverse_iterator(cend()); }
    reverse_iterator rend() noexcept { return std::reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept { return std::reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept { return std::reverse_iterator(cbegin()); }

    reference operator[](std::size_t pos) { return (*m_blocks[pos / BLOCKSIZE])[pos % BLOCKSIZE]; }
    const_reference operator[](std::size_t pos) const { return (*m_blocks[pos / BLOCKSIZE])[pos % BLOCKSIZE]; }

    reference front() { return m_blocks.front()->front(); }
    const_reference front() const { return m_blocks.front()->front(); }
    reference back() { return m_blocks.back()->back(); }
    const_reference back() const { return m_blocks.back()->back(); }

    void push_back(const T& value)
    {
        if (m_blocks.empty() || m_blocks.back()->size() == BLOCKSIZE)
            m_blocks.push_back(std::make_unique<block_type>());
        m_blocks.back()->push_back(value);
    }

    void push_back(T&& value)
    {
        if (m_blocks.empty() || m_blocks.back()->size() == BLOCKSIZE)
            m_blocks.push_back(std::make_unique<block_type>());
        m_blocks.back()->push_back(std::move(value));
    }

    template<typename... Args>
    reference emplace_back(Args&&... args)
    {
        if (m_blocks.empty() || m_blocks.back()->size() == BLOCKSIZE)
            m_blocks.push_back(std::make_unique<block_type>());
        return m_blocks.back()->emplace_back(std::forward<Args>(args)...);
    }

    void pop_back()
    {
        block_type* lastBlock = m_blocks.back();
        lastBlock->pop_back();
        if (lastBlock->empty())
            m_blocks.pop_back();
    }

    void clear() { m_blocks.clear(); }

    void swap(BlockArray& other) noexcept
    {
        using std::swap;
        swap(m_blocks, other.m_blocks);
    }

private:
    using block_type = boost::container::static_vector<T, BLOCKSIZE>;

    std::vector<std::unique_ptr<block_type>> m_blocks;
};

template<typename T, std::size_t BLOCKSIZE1, std::size_t BLOCKSIZE2>
bool operator==(const BlockArray<T, BLOCKSIZE1>& lhs, const BlockArray<T, BLOCKSIZE2>& rhs)
{
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template<typename T, std::size_t BLOCKSIZE1, std::size_t BLOCKSIZE2>
bool operator!=(const BlockArray<T, BLOCKSIZE1>& lhs, const BlockArray<T, BLOCKSIZE2>& rhs)
{
    return !(lhs == rhs);
}

template<typename T, std::size_t BLOCKSIZE1, std::size_t BLOCKSIZE2>
bool operator<(const BlockArray<T, BLOCKSIZE1>& lhs, const BlockArray<T, BLOCKSIZE2>& rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template<typename T, std::size_t BLOCKSIZE1, std::size_t BLOCKSIZE2>
bool operator>(const BlockArray<T, BLOCKSIZE1>& lhs, const BlockArray<T, BLOCKSIZE2>& rhs)
{
    return rhs < lhs;
}

template<typename T, std::size_t BLOCKSIZE1, std::size_t BLOCKSIZE2>
bool operator<=(const BlockArray<T, BLOCKSIZE1>& lhs, const BlockArray<T, BLOCKSIZE2>& rhs)
{
    return !(rhs < lhs);
}

template<typename T, std::size_t BLOCKSIZE1, std::size_t BLOCKSIZE2>
bool operator>=(const BlockArray<T, BLOCKSIZE1>& lhs, const BlockArray<T, BLOCKSIZE2>& rhs)
{
    return !(lhs < rhs);
}

template<typename T, std::size_t BLOCKSIZE>
void swap(BlockArray<T, BLOCKSIZE>& lhs, BlockArray<T, BLOCKSIZE>& rhs) noexcept
{
    lhs.swap(rhs);
}
