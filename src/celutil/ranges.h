// ranges.h
//
// Copyright (C) 2023-present, Celestia Development Team.
//
// Utilities for returning transformed ranges.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.#pragma once

#include <iterator>
#include <type_traits>
#include <utility>

namespace celestia::util
{

template<typename T, typename F>
class TransformIterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = typename std::iterator_traits<T>::difference_type;
    using value_type = std::invoke_result_t<F, decltype(*std::declval<T>())>;
    using pointer = void;
    using reference = void;

    TransformIterator(const T& iter, const F& func) : m_data{iter, func} {}

    value_type operator*() const { return m_data.func()(*m_data.iter()); }

    bool operator==(const TransformIterator& rhs) { return m_data.iter() == rhs.m_data.iter(); }
    bool operator!=(const TransformIterator& rhs) { return m_data.iter() != rhs.m_data.iter(); }

    TransformIterator& operator++() { ++m_data.iter(); return *this; }
    TransformIterator operator++(int) { auto old = *this; ++m_data.iter(); return old; }

private:
    class CompressedPair : private F
    {
    public:
        CompressedPair(const T& iter, const F& func) : F(func), m_iter{iter} {}
        const F& func() const { return *this; }
        T& iter() { return m_iter; }
        const T& iter() const { return m_iter; }
    private:
        T m_iter;
    };

    CompressedPair m_data;
};


template<typename T, typename F>
class TransformView
{
public:
    using value_type = std::invoke_result_t<F, typename T::value_type>;
    using size_type = typename T::size_type;
    using difference_type = typename T::difference_type;
    using iterator = TransformIterator<typename T::const_iterator, F>;
    using const_iterator = iterator;

    TransformView(const T& source, const F& func) : m_data(source, func) {}

    bool empty() const { return std::empty(m_data.source()); }
    size_type size() const { return std::size(m_data.source()); }

    value_type operator[](size_type pos) const { return m_data.func()(m_data.source()[pos]); }
    value_type front() const { return m_data.func()(m_data.source().front()); }
    value_type back() const { return m_data.func()(m_data.source().back()); }

    iterator begin() const { return iterator(std::begin(m_data.source()), m_data.func()); }
    const_iterator cbegin() const { return const_iterator(std::cbegin(m_data.source()), m_data.func()); }
    iterator end() const { return iterator(std::end(m_data.source()), m_data.func()); }
    const_iterator cend() const { return const_iterator(std::cend(m_data.source()), m_data.func()); }

private:
    class CompressedPair : private F
    {
    public:
        CompressedPair(const T& source, const F& func) : F(func), m_source(&source) {}
        const F& func() const { return *this; }
        const T& source() const { return *m_source; }
    private:
        const T* m_source;
    };

    CompressedPair m_data;
};


template<typename T>
auto pointerView(const T& source)
{
    return TransformView(source, [](const auto& ptr) { return ptr.get(); });
}


template<typename T>
auto constPointerView(const T& source)
{
    using ptr_type = const typename T::value_type::element_type*;
    return TransformView(source, [](const auto& ptr) -> ptr_type { return ptr.get(); });
}


template<typename T>
auto keysView(const T& source)
{
    return TransformView(source, [](const auto& pair) -> const auto& { return pair.first; });
}

} // end namespace celestia::util
