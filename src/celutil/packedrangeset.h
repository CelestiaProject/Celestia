
#pragma once

#include <vector>
#include <algorithm>

template<typename K, typename V>
class PackedRangeSet
{
public:
    typedef std::vector<V> Container;
    static K defaultKey();
    static V defaultValue();
    static K getKey(const V &);
    struct lessThanPred
    {
        bool operator()(const V &v1, const V &v2)
        {
            return getKey(v1) < getKey(v2);
        }
    };
protected:
    Container m_vector;
    K m_minKey;
    K m_maxKey;
    bool m_sorted { false };
    void setSorted(bool v) { m_sorted = v; }

public:
    typename Container::const_iterator end() const { return m_vector.end(); }
    typename Container::const_iterator begin() const { return m_vector.begin(); }
    const V &operator[](size_t i) const { return m_vector[i]; }
    PackedRangeSet(size_t n) { m_vector.reserve(n); }
    size_t getSize() const { return m_vector.size(); }
    K getMinKey() const { return m_minKey; }
    K getMaxKey() const { return m_maxKey; }
    bool isSorted() const { return m_sorted; }
    void sort()
    {
        std::sort(m_vector.begin(), m_vector.end(), lessThanPred{});
        setSorted(true);
    }
    bool isWithinRange(K k) const
    {
        if (getSize() > 0)
            return k >= getMinKey() && k <= getMaxKey();
        return false;
    }
    typename Container::iterator findIterator(K k)
    {
        if (!isWithinRange(k))
            return m_vector.end();
        for (auto it = m_vector.begin(); it != m_vector.end(); it++)
        {
            if (k == getKey(*it))
                return it;
            if (isSorted() && k < getKey(*it))
                return m_vector.end();
        }
        return m_vector.end();
    }
    V &find(K k)
    {
        return *findIterator(k);
    }
    size_t findIndex(K k) const
    {
        if (!isWithinRange(k))
            return -1;
        for (size_t i = 0; i < getSize(); i++)
        {
            if (k == getKey(m_vector[i]))
                return i;
            if (isSorted() && k < getKey(m_vector[i]))
                return -1;
        }
        return -1;
    }
    bool insert(V v)
    {
        if (getSize() > 0)
        {
            for (typename Container::iterator it = m_vector.begin(); it != m_vector.end(); it++)
            {
                if (getKey(*it) == getKey(v))
                {
                    *it = v;
                    return true;
                }
                if (isSorted() && getKey(v) > getKey(*it))
                    break;
            }
        }
        m_vector.push_back(v);
        if (getSize() == 1)
        {
            m_minKey = getKey(v);
            m_maxKey = getKey(v);
            return true;
        }
        if (getMaxKey() < getKey(v))
            m_maxKey = getKey(v);
        else if (getMaxKey() != getKey(v))
            setSorted(false);
        if (getMinKey() > getKey(v))
            m_minKey = getKey(v);
        return true;
    }
    void updateMinKey()
    {
        if (getSize() == 0)
            return;
        if (isSorted())
            m_minKey = getKey(m_vector[0]);
        else
        {
            bool first = true;
            for (const auto &v : m_vector)
            {
                if (first)
                {
                    m_minKey = getKey(v);
                    first = false;
                    continue;
                }
                if (m_minKey > getKey(v))
                    m_minKey = getKey(v);
            }
        }
    }
    void updateMaxKey()
    {
        if (getSize() == 0)
            return;
        if (isSorted())
            m_maxKey = getKey(m_vector[getSize() - 1]);
        else
        {
            bool first = true;
            for (const auto &v : m_vector)
            {
                if (first)
                {
                    m_maxKey = getKey(v);
                    first = false;
                    continue;
                }
                if (m_maxKey < getKey(v))
                    m_maxKey = getKey(v);
            }
        }
    }
    bool eraseIndex(size_t i)
    {
        if (i >= getSize())
            return false;
        K k = getKey(m_vector[i]);
        m_vector.erase(m_vector.begin() + i);
        if (k == getMinKey())
            updateMinKey();
        if (k == getMaxKey())
            updateMaxKey();
        return true;
    }
    bool erase(K k)
    {
        if (getSize() == 0)
            return false;
        if (isSorted() && getMaxKey() == k)
        {
            m_vector.erase(m_vector.end() - 1);
            return true;
        }
        for (auto it = m_vector.begin(); it != m_vector.end(); it++)
        {
            if (k == getKey(*it))
            {
                m_vector.erase(it);
                if (k == getMinKey())
                    updateMinKey();
                if (k == getMaxKey())
                    updateMaxKey();
                return true;
            }
        }
        return false;
    }
    PackedRangeSet<K, V> split()
    {
        size_t newsize = getSize() / 2;
        size_t oldsize = getSize() - newsize;
        PackedRangeSet set(newsize);
        if (newsize < 1)
            return set;
        if (!isSorted())
            sort();
        set.m_vector.assign(m_vector.begin() + oldsize, m_vector.end());
        m_vector.erase(m_vector.begin() + oldsize, m_vector.end());
        updateMaxKey();
        set.m_sorted = true;
        set.updateMinKey();
        set.updateMaxKey();
        return set;
    }
    void merge(const PackedRangeSet<K, V> &set)
    {
        if (set.getSize() == 0)
            return;
        setSorted(isSorted() && set.isSorted() && (getMaxKey() < set.getMinKey()));
        if (set.getMinKey() < getMinKey())
            m_minKey = set.getMinKey();
        if (set.getMaxKey() > getMaxKey())
            m_maxKey = set.getMaxKey();
        m_vector.insert(m_vector.end(), set.m_vector.begin(), set.m_vector.end());
    }
};
