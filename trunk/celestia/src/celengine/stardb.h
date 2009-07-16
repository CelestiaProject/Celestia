// stardb.h
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_STARDB_H_
#define _CELENGINE_STARDB_H_

#include <iostream>
#include <vector>
#include <map>
#include <celengine/constellation.h>
#include <celengine/starname.h>
#include <celengine/star.h>
#include <celengine/staroctree.h>
#include <celengine/parser.h>


static const unsigned int MAX_STAR_NAMES = 10;

// TODO: Move BlockArray to celutil; consider making it a full STL
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
        m_blocks.back()[elementIndex] = element;
        
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


class StarDatabase
{        
 public:
    StarDatabase();
    ~StarDatabase();


    inline Star*  getStar(const uint32) const;
    inline uint32 size() const;

    Star* find(uint32 catalogNumber) const;
    Star* find(const std::string&) const;
    uint32 findCatalogNumberByName(const std::string&) const;

    std::vector<std::string> getCompletion(const std::string&) const;

    void findVisibleStars(StarHandler& starHandler,
                          const Eigen::Vector3f& obsPosition,
                          const Eigen::Quaternionf&   obsOrientation,
                          float fovY,
                          float aspectRatio,
                          float limitingMag) const;

    void findCloseStars(StarHandler& starHandler,
                        const Eigen::Vector3f& obsPosition,
                        float radius) const;

    std::string getStarName    (const Star&, bool i18n = false) const;
    void getStarName(const Star& star, char* nameBuffer, unsigned int bufferSize, bool i18n = false) const;
    std::string getStarNameList(const Star&, const unsigned int maxNames = MAX_STAR_NAMES) const;

    StarNameDatabase* getNameDatabase() const;
    void setNameDatabase(StarNameDatabase*);
    
    bool load(std::istream&, const std::string& resourcePath);
    bool loadBinary(std::istream&);

    enum Catalog
    {
        HenryDraper = 0,
        Gliese      = 1,
        SAO         = 2,
        MaxCatalog  = 3,
    };

    enum StcDisposition
    {
        AddStar,
        ReplaceStar,
        ModifyStar,
    };
    
	// Not exact, but any star with a catalog number greater than this is assumed to not be
	// a HIPPARCOS stars.
	static const uint32 MAX_HIPPARCOS_NUMBER = 999999;

    struct CrossIndexEntry
    {
        uint32 catalogNumber;
        uint32 celCatalogNumber;

        bool operator<(const CrossIndexEntry&) const;
    };

    typedef std::vector<CrossIndexEntry> CrossIndex;

    bool   loadCrossIndex  (const Catalog, std::istream&);
    uint32 searchCrossIndexForCatalogNumber(const Catalog, const uint32 number) const;
    Star*  searchCrossIndex(const Catalog, const uint32 number) const;
    uint32 crossIndex      (const Catalog, const uint32 number) const;

    void finish();

    static StarDatabase* read(std::istream&);

    static const char* FILE_HEADER;
    static const char* CROSSINDEX_FILE_HEADER;

private:
    bool createStar(Star* star,
                    StcDisposition disposition,
                    uint32 catalogNumber,
                    Hash* starData,
                    const std::string& path,
                    const bool isBarycenter);

    void buildOctree();
    void buildIndexes();
    Star* findWhileLoading(uint32 catalogNumber) const;

    int nStars;
        
    Star*             stars;
    StarNameDatabase* namesDB;
    Star**            catalogNumberIndex;
    StarOctree*       octreeRoot;
    uint32            nextAutoCatalogNumber;

    std::vector<CrossIndex*> crossIndexes;

    // These values are used by the star database loader; they are
    // not used after loading is complete.
    BlockArray<Star> unsortedStars;
    // List of stars loaded from binary file, sorted by catalog number
    Star** binFileCatalogNumberIndex;
    unsigned int binFileStarCount;
    // Catalog number -> star mapping for stars loaded from stc files
    std::map<uint32, Star*> stcFileCatalogNumberIndex;    

    struct BarycenterUsage
    {
        uint32 catNo;
        uint32 barycenterCatNo;
    };
    std::vector<BarycenterUsage> barycenters;
};


Star* StarDatabase::getStar(const uint32 n) const
{
    return stars + n;
}

uint32 StarDatabase::size() const
{
    return nStars;
}

#endif // _CELENGINE_STARDB_H_
