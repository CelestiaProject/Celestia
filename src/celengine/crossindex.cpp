
#include <iostream>
#include <fmt/printf.h>
#include "crossindex.h"

using namespace std;

static inline AstroCatalog::IndexNumber firstNr(const CrossIndex::CrossIndexMap::const_iterator &it)
{
    return it->first;
}

static inline AstroCatalog::IndexNumber lastNr(const CrossIndex::CrossIndexMap::const_iterator &it)
{
    return it->first + it->second.length - 1;
}

bool CrossIndex::set(AstroCatalog::IndexNumber nr, int shift, size_t length, bool overwrite)
{
    AstroCatalog::IndexNumber lastnr = nr + length - 1;
    CrossIndexMap::iterator it = m_map.lower_bound(nr);
    if (it != m_map.begin() || (m_map.size() > 0 && firstNr(it) > nr))
        it--;
    CrossIndexRange last{0, 0};

    while(it != m_map.end() && firstNr(it) <= lastnr)
    {
        if (lastNr(it) > lastnr)
        {
            if (!overwrite)
            {
                fmt::fprintf(cerr, "CrossIndex: Found overlapping rear range [%i, %i]!\n", firstNr(it), lastNr(it));
                return false;
            }
            last.shift = it->second.shift;
            last.length = lastNr(it) - lastnr;
        }
        if (lastNr(it) < nr)
            it++;
        else
        {
            if (!overwrite)
            {
                fmt::fprintf(cerr, "CrossIndex: Found overlapping front or middle range [%i, %i]!\n", firstNr(it), lastNr(it));
                return false;
            }
            if (firstNr(it) < nr)
            {
                it->second.length = nr - firstNr(it);
                it++;
            }
            else
                it = m_map.erase(it);
        }
    }
    m_map.insert(make_pair(nr, CrossIndexRange{ shift, length }));
    if (last.length > 0)
        m_map.insert(make_pair(lastnr + 1, last));
    return true;
}

AstroCatalog::IndexNumber CrossIndex::get(AstroCatalog::IndexNumber nr) const
{
    if (m_map.size() == 0)
        return AstroCatalog::InvalidIndex;
    CrossIndexMap::const_iterator it = m_map.lower_bound(nr);
    if (it != m_map.begin() && (it == m_map.end() || it->first > nr))
        it--;
    if (it == m_map.end())
        return AstroCatalog::InvalidIndex;
//     fmt::fprintf(cout, "%i => [ %i, %i ]\n", it->first, it->second.shift, it->second.length);
    if (firstNr(it) <= nr && lastNr(it) >= nr)
        return nr + it->second.shift;
    return AstroCatalog::InvalidIndex;
}
