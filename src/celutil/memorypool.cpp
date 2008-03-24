// memorypool.cpp
//
// A simple, sequential allocator with zero overhead for allocating
// and freeing objects.
//
// Copyright (C) 2008, the Celestia Development Team
// Initial version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cassert>
#include "memorypool.h"

using namespace std;


static bool isPowerOfTwo(unsigned int x)
{
    return x != 0 && (x & (x - 1)) == 0;
}

/*! Create a new memory pool with the specified alignment and block size.
 *  The alignment must be a power of two and adequate for the largest
 *  structure that is to be allocated from the pool. The block size should
 *  be chosen considerably larger than frequently allocated structures, and
 *  large enough to accommodate the largest requested allocation. Choosing
 *  it too large will result in a lot of wasted space; too small, and
 *  there could be a lot of overhead from C runtime memory allocation for
 *  new blocks.
 */
MemoryPool::MemoryPool(unsigned int alignment, unsigned int blockSize) :
    m_alignment(alignment),
    m_blockSize(blockSize),
    m_blockOffset(0)
{
    assert(isPowerOfTwo(alignment));

    m_currentBlock = m_blockList.begin();
}


MemoryPool::~MemoryPool()
{
    for (list<Block>::iterator iter = m_blockList.begin(); iter != m_blockList.end(); iter++)
        delete iter->m_memory;
}


/*! Allocate size bytes from the memory pool and return a pointer to
 *  the newly allocated memory. The pointer is valid until the next time
 *  freeAll() is called for the pool. Returns NULL if the requested size
 *  is larger than the block size of the pool, or if a new block is
 *  required but cannot be allocated (out of memory.)
 */
void*
MemoryPool::allocate(unsigned int size)
{
    if (size > m_blockSize)
        return NULL;
    
    // See if the current block has enough room
    if (m_blockOffset + size > m_blockSize)
    {
        m_currentBlock++;
    }
    
    // See if we need to allocate a new block
    if (m_currentBlock == m_blockList.end())
    {
        Block block;
        block.m_memory = new char[m_blockSize];
        if (block.m_memory == NULL)
            return NULL;
        m_currentBlock = m_blockList.insert(m_currentBlock, block);
        m_blockOffset = 0;
    }
    
    void* memory = m_currentBlock->m_memory + m_blockOffset;
    
    // Advance
    unsigned int padSize = (size + m_alignment - 1) & ~(m_alignment - 1);
    m_blockOffset += padSize;
    
    return memory;
}


/*! Free all memory allocated by the pool. All pointers allocated are
 *  invalid after this call.
 */
void
MemoryPool::freeAll()
{
#ifdef DEBUG
    // Fill blocks with garbage after freeing to help catch invalid
    // memory errors in debug.
    for (list<Block>::iterator iter = m_blockList.begin(); iter != m_blockList.end(); iter++)
    {
        unsigned int* p = reinterpret_cast<unsigned int*>(iter->m_memory);
        for (unsigned int i = 0; i < m_blockSize / sizeof(unsigned int); i++)
            p[i] = 0xdeaddead;
    }
#endif
    m_currentBlock = m_blockList.begin();
    m_blockOffset = 0;
}


/*! Return the block size for this memory pool.
 */
unsigned int
MemoryPool::blockSize() const
{
    return m_blockSize;
}


/*! Return the alignment for this pool.
 */
unsigned int
MemoryPool::alignment() const
{
    return m_alignment;
}
