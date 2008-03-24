// memorypool.h
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

#ifndef _CELUTIL_MEMORYPOOL_H_
#define _CELUTIL_MEMORYPOOL_H_

#include <list>

class MemoryPool
{
public:
    MemoryPool(unsigned int alignment, unsigned int blockSize);
    ~MemoryPool();
    
    void* allocate(unsigned int size);
    void freeAll();
    
    unsigned int blockSize() const;
    unsigned int alignment() const;
    
private:
    unsigned int m_alignment;
    unsigned int m_blockSize;

    struct Block
    {
        char* m_memory;
    };
    
    std::list<Block> m_blockList;
    std::list<Block>::iterator m_currentBlock;
    unsigned int m_blockOffset;
};

#endif // _CELUTIL_MEMORYPOOL_H_

