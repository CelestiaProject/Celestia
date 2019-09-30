
#pragma once

#include <map>
#include "astrocat.h"

class CrossIndex
{
 public:
    struct CrossIndexRange
    {
        int shift;
        size_t length;
    };
    typedef std::map<AstroCatalog::IndexNumber, CrossIndexRange> CrossIndexMap;
 protected:
    CrossIndexMap m_map;
 public:
    bool set(AstroCatalog::IndexNumber, int, size_t, bool);
    AstroCatalog::IndexNumber get(AstroCatalog::IndexNumber) const;
    const CrossIndexMap& getRecords() const { return m_map; }
};
