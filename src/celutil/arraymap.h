
#pragma once

// blockarray.h
//
// Copyright (C) 2020, the Celestia Development Team
// Original version by Łukasz Buczyński <lukasz.a.buczynski@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstddef>
#include <stdexcept>
#include <array>

/*
 * ArrayMap implements simple array of elements with some additional features.
 * There is mpty/invalid slot concept, hence used (valid) slots number.
 * Array size is determined from key length and is always a power of two.
 */

template<typename K, typename V, size_t ArrayKeyLen>
class ArrayMap
{
public:
    static V invalidValue();
    static constexpr size_t ArraySize = 1 << ArrayKeyLen;
    static size_t arrayKey(K k)
    {
        k = k << (sizeof(k) * 8 - ArrayKeyLen);
        return k >> (sizeof(k) * 8 - ArrayKeyLen);
    }
    size_t size() const { return ArraySize; }
protected:
    std::array<V, ArraySize> m_array;
    size_t m_used { 0 };
public:
    ArrayMap()
    {
        m_array.fill(invalidValue());
    }
    size_t used() const { return m_used; }
    size_t totalUsed() const { return m_used; }
    bool has(K k) const { return m_array[k] != invalidValue(); }
    const V &getRef(K k) const
    {
        auto ak = arrayKey(k);
        if (m_array[ak] == invalidValue())
            throw std::invalid_argument("Invalid element in array!");
        return m_array[ak];
    }
    V &getRef(K k)
    {
        auto ak = arrayKey(k);
        if (m_array[ak] == invalidValue())
            throw std::invalid_argument("Invalid element in array!");
        return m_array[ak];
    }
    V getValue(K k) const
    {
        auto ak = arrayKey(k);
        return m_array[ak];
    }
    V *getPtr(K k)
    {
        auto ak = arrayKey(k);
        if (m_array[ak] == invalidValue())
            return nullptr;
        return &m_array[ak];
    }
    const V *getPtr(K k) const
    {
        auto ak = arrayKey(k);
        if (m_array[ak] == invalidValue())
            return nullptr;
        return &m_array[ak];
    }
    bool insert(K k, const V &v)
    {
        auto ak = arrayKey(k);
        bool ret = m_array[ak] == invalidValue();
        if (ret)
            m_used++;
        m_array[ak] = v;
        return ret;
    }
    bool erase(K k)
    {
        auto ak = arrayKey(k);
        bool ret = m_array[ak] != invalidValue();
        if (ret)
            m_used--;
        m_array[ak] = invalidValue();
        return ret;
    }
};

/*
 * MultilevelArrayMap is similar to ArrayMap but has in its slots another container
 * with API consistent with ArrayMap. It's usefull for memory saving. There could be
 * many levels, as subcontainer may be another MultilevelArrayMap. That's why it have
 * separate ArrayKeyLen and KeyLen parameter. There always should be ArrayKeyLen <= KeyLen,
 * as we may have to use only a subset of key value bits. Used key contains KeyLen less
 * significant bits, while key used for internal search contains ArrayKeyLen of most
 * significant bits from used key. Key is always provided to subcontainer in its original
 * form, because its implementation may demand it.
 */

template<typename K, typename V, typename C, size_t ArrayKeyLen, size_t KeyLen>
class MultilevelArrayMap
{
public:
    static V invalidValue() { return C::invalidValue(); }
    static constexpr size_t ArraySize = 1 << ArrayKeyLen;
    static size_t arrayKey(K k)
    {
        k = k << (sizeof(k) * 8 - KeyLen);
        return k >> (sizeof(k) * 8 - ArrayKeyLen);
    }
    size_t size() const { return ArraySize; }
protected:
    std::array<C *, ArraySize> m_array;
    size_t m_used { 0 };
    size_t m_totUsed { 0 };
public:
    MultilevelArrayMap()
    {
        m_array.fill(nullptr);
    }
    ~MultilevelArrayMap()
    {
        for(auto ptr : m_array)
            delete ptr;
    }
    size_t used() const { return m_used; }
    size_t totalUsed() const { return m_totUsed; }
    C * const *data() const { return m_array.data(); }
    const C *container(K k) const { return m_array[arrayKey(k)]; }
    bool has(K k) const
    {
        size_t ak = arrayKey(k);
        if (m_array[ak] == nullptr)
            return false;
        return m_array[ak]->has(k);
    }
    const V &getRef(K k) const
    {
        size_t ak = arrayKey(k);
        if (m_array[ak] == nullptr)
            throw std::invalid_argument("Invalid element in array!");
        return m_array[ak]->getRef(k);
    }
    V &getRef(K k)
    {
        size_t ak = arrayKey(k);
//         std::cout << "getRef key: " << ak << std::endl;
        if (m_array[ak] == nullptr)
            throw std::invalid_argument("Invalid element in array!");
        return m_array[ak]->getRef(k);
    }
    const V *getPtr(K k) const
    {
        size_t ak = arrayKey(k);
        if (m_array[ak] == nullptr)
            return nullptr;
        return m_array[ak]->getPtr(k);
    }
    V *getPtr(K k)
    {
        size_t ak = arrayKey(k);
        if (m_array[ak] == nullptr)
            return nullptr;
        return m_array[ak]->getPtr(k);
    }
    V getValue(K k) const
    {
        size_t ak = arrayKey(k);
        if (m_array[ak] == nullptr)
            return invalidValue();
        return m_array[ak]->getValue(k);
    }
    bool insert(K k, const V &v)
    {
        size_t ak = arrayKey(k);
        if (m_array[ak] == nullptr)
        {
            m_array[ak] = new C;
            m_used++;
        }
        if (m_array[ak]->insert(k, v))
        {
            m_totUsed++;
            return true;
        }
        return false;
    }
    bool erase(K k)
    {
        size_t ak = arrayKey(k);
        if (m_array[ak] == nullptr)
            return false;
        if (m_array[ak]->erase(k))
        {
            if (m_array[ak]->used() == 0)
            {
                delete m_array[ak];
                m_array[ak] = nullptr;
                m_used--;
            }
            m_totUsed--;
            return true;
        }
        return false;
    }
};
