
#pragma once

#include <cstddef>
#include <algorithm>

template<typename K, typename V>
class HashMap
{
protected:
    K *m_keys;
    V *m_vals;
    size_t m_size { 0 };
    size_t m_used { 0 };
    size_t m_shift { 0 };
    size_t m_thres { 0 };
    float m_loadF { 0.8 };
    size_t m_maxSize { 1024 };
    size_t m_minSize { 16 };
    V m_zero;
    bool m_hasZero { false };
public:
    static V invalidValue();
    size_t size() const { return m_size + 1; }
    size_t used() const { return m_hasZero ? m_used + 1 : m_used; }
    const K *keyData() const { return m_keys; }
    const V *valData() const { return m_vals; }
    size_t hashedKey(K k) const
    {
        return (k ^ k >> (sizeof(k) * 4) * 0x9E3779B97F4A7C15L >> m_shift);
    }
    size_t rounded(size_t k) const { return k % m_size; }
    size_t roundedHash(size_t k) const { return rounded(hashedKey(k)); }
    size_t roundedInc(size_t k) const { return rounded((k + 1)); }
    static size_t leadZerosNum(size_t k)
    {
        size_t ss = sizeof(k) * 8;
        size_t ret = 0;
        size_t mask = 1 << ss - 1;
        while(k & mask)
        {
            mask >>= 1;
            ret ++;
        }
        return ret;
    }
    HashMap(size_t size, size_t minSize = 16, size_t maxSize = 1024, float load = 0.8) :
        m_size(size), m_minSize(minSize), m_maxSize(maxSize), m_loadF(load)
    {
        if (maxSize < minSize)
            throw "Maximum size lesser than minimum size!";
        if (m_size < m_minSize)
            m_size = m_minSize;
        m_keys = new K[m_size];
        m_vals = new V[m_size];
        std::fill(m_keys, m_keys + m_size - 1, 0);
        m_thres = m_size * m_loadF;
        m_shift = leadZerosNum(m_size);
    }

    int locateKey(size_t k) const
    {
        for (size_t i = roundedHash(k);; i = roundedInc(i))
        {
            if (m_keys[i] == 0)
                return -(i + 1);
            if (m_keys[i] == k)
                return i;
        }
    }
protected:
    bool insertToTable(K k, V v)
    {
        int i = locateKey(k);
        if (i >= 0)
        {
            m_keys[i] = k;
            m_vals[i] = v;
            return false;
        }
        i = -(i + 1);
        m_keys[i] = k;
        m_vals[i] = v;
        return true;
    }
public:
    void resize(size_t S)
    {
        if (S < m_used)
            throw "Requested size to small!";
        size_t oldsize = m_size;
        m_size = S;
        m_thres = m_size * m_loadF;
        K *oldkeys = m_keys;
        V *oldvals = m_vals;
        m_keys = new K[S];
        m_vals = new V[S];
        std::fill(m_keys, m_keys + S, 0);
        for (size_t i = 0; i < oldsize; i++)
            insertToTable(oldkeys[i], oldvals[i]);
        delete oldkeys;
        delete oldvals;
    }
    size_t newSize()
    {
        if (m_used >= m_thres)
        {
            if (m_size < m_maxSize)
                return m_size * 2;
            else
                return m_size * 1.1;
        }
        float div = m_used < m_maxSize ? 0.9 : 0.5;
        if (m_used < m_size * div * m_loadF)
        {
            size_t nsize = m_size * div;
            if (nsize < m_minSize)
                return m_minSize;
            return nsize;
        }
        return m_size;
    }
    bool checkSize()
    {
        size_t nsize = newSize();
        if (nsize == m_size)
            return false;
        resize(nsize);
        return true;
    }
    bool insert(K k, V v)
    {
        if (k == 0)
        {
            bool ret = !m_hasZero;
            m_zero = v;
            m_hasZero = true;
            return ret;
        }
        if (insertToTable(k, v))
        {
            m_used++;
            checkSize();
            return true;
        }
        return false;
    }
    bool has(K k) const
    {
        if (k == 0)
            return m_hasZero;
        return locateKey(k) >= 0 ? true : false;
    }
    const V *getPtr(K k) const
    {
        if (k == 0)
        {
            if (m_hasZero)
                return &m_zero;
            return nullptr;
        }
        int i = locateKey(k);
        if (i >= 0)
            return &m_vals[i];
        return nullptr;
    }
    V *getPtr(K k)
    {
        return const_cast<V*>(static_cast<const HashMap &>(*this).getPtr(k));
    }
    V getValue(K k) const
    {
        const V* ptr = getPtr(k);
        if (ptr == nullptr)
            return invalidValue();
        return *ptr;
    }
    const V& getRef(K k) const
    {
        const V *ptr = getPtr(k);
        if (ptr == nullptr)
            throw "Invalid argument";
        return *ptr;
    }
    V& getRef(K k)
    {
        return const_cast<V&>(static_cast<const HashMap&>(*this).getRef(k));
    }
    bool erase(K k)
    {
        if (k == 0)
        {
            bool ret = m_hasZero;
            m_hasZero = false;
            return ret;
        }
        int i = locateKey(k);
        if (i < 0)
            return false;
        int next = roundedInc(i);
        while(m_keys[next] != 0)
        {
            size_t key = m_keys[next];
            size_t pl = roundedHash(key);
            if ((next - pl) > (i - pl))
            {
                m_keys[i] = key;
                m_vals[i] = m_vals[next];
                i = next;
            }
            next = rounded(next + 1);
        }
        m_keys[i] = 0;
        m_used--;
        checkSize();
        return true;
    }
    void clear()
    {
        std::fill(m_keys, m_keys + m_size, 0);
        m_used = 0;
        m_hasZero = false;
        checkSize();
    }
};
