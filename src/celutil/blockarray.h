
#pragma once

// blockarray.h
//
// Copyright (C) 2001-2020, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <vector>

// TODO: consider making it a full STL
// style container with iterator support.

/*! BlockArray is a container class that is similar to an STL vector
 *  except for two very important differences:
 *  - The elements of a BlockArray are not necessarily in one
 *    contiguous block of memory.
 *  - The address of a BlockArray element is guaranteed not to
 *    change over the lifetime of the BlockArray (or until the
 *    BlockArray is cleared.)
 */
template<class T> class BlockArray
{
public:
    BlockArray() :
        m_blockSize(1000),
        m_elementCount(0)
    {
    }

    ~BlockArray()
    {
        clear();
    }

    unsigned int size() const
    {
        return m_elementCount;
    }

    /*! Append an item to the BlockArray. */
    void add(T& element)
    {
        unsigned int blockIndex = m_elementCount / m_blockSize;
        if (blockIndex == m_blocks.size())
        {
            T* newBlock = new T[m_blockSize];
            m_blocks.push_back(newBlock);
        }

        unsigned int elementIndex = m_elementCount % m_blockSize;
        new (&m_blocks.back()[elementIndex]) T(std::move(element));

        ++m_elementCount;
    }

    void clear()
    {
        for (typename std::vector<T*>::const_iterator iter = m_blocks.begin(); iter != m_blocks.end(); ++iter)
        {
            delete[] *iter;
        }
        m_elementCount = 0;
        m_blocks.clear();
    }

    T& operator[](int index)
    {
        unsigned int blockNumber = index / m_blockSize;
        unsigned int elementNumber = index % m_blockSize;
        return m_blocks[blockNumber][elementNumber];
    }

    const T& operator[](int index) const
    {
        unsigned int blockNumber = index / m_blockSize;
        unsigned int elementNumber = index % m_blockSize;
        return m_blocks[blockNumber][elementNumber];
    }

private:
    unsigned int m_blockSize;
    unsigned int m_elementCount;
    std::vector<T*> m_blocks;
};
