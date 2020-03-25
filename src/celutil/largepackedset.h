
#pragma once

#include <map>

#include <celutil/packedrangeset.h>

template<typename K, typename V>
class LargePackedSet
{
public:
    typedef PackedRangeSet<K, V> RangeContainer;
    typedef std::map<K, RangeContainer> Container;
protected:
    Container m_map;
    size_t m_size { 0 };
    struct lowerThanPred
    {
        bool operator()(const std::pair<const K, const RangeContainer> &pair, const K &k) const
        {
            return pair.first < k;
        }
        bool operator()(const K &k, const std::pair<const K, RangeContainer> &pair) const
        {
            return k < pair.first;
        }
    };
    size_t m_splitThreshold { 0 };
    size_t m_mergeThreshold { 0 };
    size_t m_rangePreserved { 0 };
public:

    class iterator
    {
    protected:
        typename Container::iterator m_rangeIt;
        size_t m_vIndex { 0 };
    public:
        iterator() = default;
        iterator(typename Container::iterator rit, size_t i) :
            m_rangeIt(rit), m_vIndex(i) {}
        bool operator==(const iterator &other) const
        {
            return m_rangeIt == other.m_rangeIt && m_vIndex == other.m_vIndex;
        }
        bool operator!=(const iterator &other) const
        {
            return m_rangeIt != other.m_rangeIt || m_vIndex != other.m_vIndex;
        }
        iterator &operator++()
        {
            if (m_rangeIt->second.getSize() < m_vIndex)
                ++m_vIndex;
            else
            {
                ++m_rangeIt;
                m_vIndex = 0;
            }
            return *this;
        }
        iterator &operator--()
        {
            if (0 < m_vIndex)
                --m_vIndex;
            else
            {
                --m_rangeIt;
                m_vIndex = m_rangeIt->second.getSize() > 0 ? m_rangeIt->second.getSize() - 1 : 0;
            }
            return *this;
        }
        V *operator->() { return &m_rangeIt->second[m_vIndex]; }
        const V *operator->() const { return &m_rangeIt->second[m_vIndex]; }
    };

    LargePackedSet() = default;
    LargePackedSet(size_t splitT, size_t mergeT, size_t pres = 0) :
        m_splitThreshold(splitT),
        m_mergeThreshold(mergeT),
        m_rangePreserved(pres) {}
    size_t getSplitThreshold() const { return m_splitThreshold; }
    size_t getMergeThreshold() const { return m_mergeThreshold; }
    size_t getRangePreservedSpace() const { return m_rangePreserved; }
    size_t getSize() const { return m_size; }
    size_t getRangesNumber() const { return m_map.size(); }
    const Container &getContainer() const { return m_map; }
    iterator begin() { return iterator(m_map.begin(), 0); }
    iterator end() { return iterator(m_map.end(), 0); }
    typename Container::const_iterator rangesBegin() const { return m_map.begin(); }
    typename Container::const_iterator rangesEnd() const { return m_map.end(); }
    typename Container::iterator findRangeIterator(K k)
    {
        if (m_map.size() == 0)
            return m_map.end();
        typename Container::iterator it = std::lower_bound(m_map.begin(), m_map.end(), k, lowerThanPred {});
        if (it != m_map.end())
        {
            if (it->second.isWithinRange(k))
                return it;
        }
        if (it != m_map.begin())
        {
            --it;
            if (it->second.isWithinRange(k))
                return it;
            else
                return m_map.end();
        }
        return m_map.end();
    }
    iterator findValueIterator(K k)
    {
        auto rit = findRangeIterator(k);
        if (rit == rangesEnd())
            return end();
        size_t i = rit->second.findIndex(k);
        if (i == -1)
            return end();
        return iterator(rit, i);
    }
protected:
    bool insertRange(const RangeContainer &rc)
    {
        m_map.insert(std::make_pair(rc.getMinKey(), rc));
        m_size += rc.getSize();
        return true;
    }
public:
    bool insert(const RangeContainer &rc)
    {
        if (rc.getSize() == 0)
            return false;
        if (getSize() == 0)
            return insertRange(rc);
        auto rit = std::upper_bound(m_map.begin(), m_map.end(), rc.getMinKey(), lowerThanPred {});
        if (rit == rangesEnd())
        {
            --rit;
            if (rit->second.getMaxKey() >= rc.getMinKey())
                return false;
            return insertRange(rc);
        }
        if (rit->second.getMinKey() <= rc.getMaxKey())
            return false;
        if (rit != rangesBegin())
        {
            --rit;
            if (rit->second.getMaxKey() >= rc.getMinKey())
                return false;
        }
        return insertRange(rc);
    }
    bool splitCheck(typename Container::iterator it)
    {
        if (getSplitThreshold() > 0 && it->second.getSize() > getSplitThreshold())
        {
            RangeContainer rc2 = it->second.split();
            if (rc2.getSize() == 0)
                return false;
            if (!it->second.isWithinRange(it->first))
            {
                RangeContainer rc1 = it->second;
                m_map.erase(it);
                m_map.insert(std::make_pair(rc1.getMinKey(), rc1));
            }
            m_map.insert(std::make_pair(rc2.getMinKey(), rc2));
            return true;
        }
        return false;
    }
protected:
    bool insert(typename Container::iterator it, V v)
    {
        bool ret = it->second.insert(v);
        splitCheck(it);
        if (ret)
            m_size++;
        return ret;
    }
public:
    bool insert(V v)
    {
        if (getSize() == 0)
        {
            RangeContainer rc(getRangePreservedSpace());
            rc.insert(v);
            return insertRange(rc);
        }
        K k = RangeContainer::getKey(v);
        typename Container::iterator it = std::lower_bound(m_map.begin(),
                                                           m_map.end(),
                                                           k,
                                                           lowerThanPred {});
        if (it == rangesEnd())
        {
            --it;
            return insert(it, v);
        }
        if (it == rangesBegin() || it->second.isWithinRange(k))
            return insert(it, v);
        typename Container::iterator it2 = it;
        --it2;
        if (it2->second.isWithinRange(k))
            return insert(it2, v);
        if (it2->second.getSize() <= it->second.getSize())
            return insert(it2, v);
        else
            return insert(it, v);
    }
protected:
    bool tryMerge(typename Container::iterator it1, typename Container::iterator it2)
    {
        if (it1->second.getSize() + it2->second.getSize() <= getSplitThreshold())
        {
            it1->second.merge(it2->second);
            m_map.erase(it2);
            return true;
        }
        return false;
    }
public:
    bool erase(K k)
    {
        auto it = findRangeIterator(k);
        if (it == rangesEnd() || !it->second.isWithinRange(k))
            return false;
        if (it->second.erase(k))
        {
            m_size--;
            if (it->second.getSize() == 0)
                m_map.erase(it);
            if (it->second.getSize() < getMergeThreshold())
            {
                if (it != rangesBegin())
                {
                    auto it2 = it;
                    --it2;
                    if (tryMerge(it2, it))
                        return true;
                }
                auto it2 = it;
                ++it2;
                tryMerge(it, it2);
            }
            return true;
        }
        return false;
    }
};
