// sv.h
//
// Copyright (C) 2021-present, Celestia Development Team.
//
// Read-only view of C/C++ strings.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>
#include <stdexcept>
#include <algorithm>

#if defined(_MSC_VER) && !defined(__clang__)
// M$VC++ 2015 doesn't have required features
#define CEL_CPP_VER (_MSC_VER == 1900 ? 201103L : _MSVC_LANG)
// M$VC++ build without C++ exceptions are not supported yet
#ifndef __cpp_exceptions
#define __cpp_exceptions 1
#endif
#else
#define CEL_CPP_VER __cplusplus
#endif

#if CEL_CPP_VER == 201402L
#define CEL_constexpr constexpr
#else
#define CEL_constexpr /* constexpr */
#endif

#if ! __cpp_exceptions
#include <cstdlib>
#endif

namespace std
{
template<
    typename CharT,
    typename Traits = std::char_traits<CharT>
> class basic_string_view
{
 public:
    using traits_type            = Traits;
    using value_type             = CharT;
    using pointer                = CharT*;
    using const_pointer          = const CharT*;
    using reference              = CharT&;
    using const_reference        = const CharT&;
    using const_iterator         = const CharT*;
    using iterator               = const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator       = const_reverse_iterator;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;

    enum : size_type
    {
        npos = size_type(-1)
    };

    constexpr basic_string_view() noexcept = default;
    constexpr basic_string_view(const basic_string_view& other) noexcept = default;
    constexpr basic_string_view(const CharT* s, size_type count) :
        m_data { s },
        m_size { count }
    {}
    constexpr basic_string_view(const std::basic_string<CharT, Traits> &s) :
        m_data { s.data() },
        m_size { s.size() }
    {}

    constexpr basic_string_view(const CharT* s) :
        m_data { s },
        m_size { Traits::length(s) }
    {}

    CEL_constexpr basic_string_view& operator=(const basic_string_view&) noexcept = default;

    explicit operator basic_string<CharT, Traits>() const
    {
        return { m_data, m_size };
    }

    constexpr const_pointer data() const noexcept
    {
        return m_data;
    }
    constexpr size_type size() const noexcept
    {
        return m_size;
    }
    constexpr size_type length() const noexcept
    {
        return size();
    }
    constexpr bool empty() const noexcept
    {
        return size() == 0;
    }
    constexpr const_reference operator[](size_type pos) const
    {
        return m_data[pos];
    }
    CEL_constexpr const_reference at(size_type pos) const
    {
        if (pos < size())
            return m_data[pos];
#if __cpp_exceptions
        throw std::out_of_range("pos >= size()");
#else
        std::abort();
#endif
    }
    constexpr const_reference front() const
    {
        return *m_data;
    }
    constexpr const_reference back() const
    {
        return *(m_data + m_size - 1);
    }
    constexpr size_type max_size() const noexcept
    {
        return (npos - sizeof(size_type) - sizeof(pointer)) / sizeof(value_type) / 4;
    }
    constexpr const_iterator begin() const noexcept
    {
        return m_data;
    }
    constexpr const_iterator cbegin() const noexcept
    {
        return begin();
    }
    constexpr const_iterator end() const noexcept
    {
        return m_data + m_size;
    }
    constexpr const_iterator cend() const noexcept
    {
        return end();
    }
    constexpr const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }
    constexpr const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }
    constexpr const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }
    constexpr const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(begin());
    }
    CEL_constexpr void remove_prefix(size_type n)
    {
        m_data += n;
	m_size -= n;
    }
    CEL_constexpr void remove_suffix(size_type n)
    {
	m_size -= n;
    }
    CEL_constexpr void swap(basic_string_view& v) noexcept
    {
        auto t = *this;
        this = v;
        v = t;
    }
    size_type copy(CharT* dest, size_type count, size_type pos = 0) const
    {
        if (pos > m_size)
#if __cpp_exceptions
            throw std::out_of_range("pos >= size()");
#else
            std::abort();
#endif

        auto rcount = std::min(count, m_size - pos);
        return Traits::copy(dest, m_data + pos, rcount);
    }
    CEL_constexpr basic_string_view substr(size_type pos = 0, size_type count = npos ) const
    {
        if (pos > m_size)
#if __cpp_exceptions
            throw std::out_of_range("pos >= size()");
#else
            std::abort();
#endif

        auto rcount = std::min(count, m_size - pos);
        return { m_data + pos, rcount };
    }
    CEL_constexpr int compare(basic_string_view v) const noexcept
    {
        auto r = Traits::compare(data(), v.data(), std::min(size(), v.size()));
        return r == 0 ? size() - v.size() : r;
    }
    constexpr int compare(size_type pos1, size_type count1, basic_string_view v) const
    {
        return substr(pos1, count1).compare(v);
    }
    constexpr int compare(size_type pos1, size_type count1, basic_string_view v, size_type pos2, size_type count2) const
    {
        return substr(pos1, count1).compare(v.substr(pos2, count2));
    }
    constexpr int compare(const CharT* s) const
    {
        return compare(basic_string_view(s));
    }
    constexpr int compare(size_type pos1, size_type count1, const CharT* s) const
    {
        return substr(pos1, count1).compare(basic_string_view(s));
    }
    constexpr int compare(size_type pos1, size_type count1, const CharT* s, size_type count2) const
    {
        return substr(pos1, count1).compare(basic_string_view(s, count2));
    }
    constexpr size_type find(basic_string_view v, size_type pos = 0) const noexcept
    {
        return (pos >= m_size ? npos : (basic_string_view(m_data + pos, v.size()).compare(v) == 0 ? pos : find(v, pos + 1)));
    }
    constexpr size_type find(CharT ch, size_type pos = 0) const noexcept
    {
        return find(basic_string_view(std::addressof(ch), 1), pos);
    }
    constexpr size_type find(const CharT* s, size_type pos, size_type count) const
    {
        return find(basic_string_view(s, count), pos);
    }
    constexpr size_type find(const CharT* s, size_type pos = 0) const
    {
        return find(basic_string_view(s), pos);
    }
    CEL_constexpr size_type rfind(basic_string_view v, size_type pos = npos) const noexcept
    {
        if (v.size() > size())
            return npos;
        if (v.size() == 0)
            return std::min(size(), pos);
        auto last = begin() + std::min(pos, size());
        auto r = std::find_end(begin(), last, v.begin(), v.end());
        if (r != last)
            return r - begin();
        return npos;
    }
    constexpr size_type rfind(CharT c, size_type pos = npos) const noexcept
    {
        return rfind(basic_string_view(std::addressof(c), 1), pos);
    }
    constexpr size_type rfind(const CharT* s, size_type pos, size_type count) const
    {
        return rfind(basic_string_view(s, count), pos);
    }
    constexpr size_type rfind(const CharT* s, size_type pos = npos) const
    {
        return rfind(basic_string_view(s), pos);
    }
    CEL_constexpr size_type find_first_of(basic_string_view v, size_type pos = 0) const noexcept
    {
        for (size_type i = pos; i < m_size; i++)
            for (auto c : v)
                if (c == m_data[i])
                    return i;
        return npos;
    }
    CEL_constexpr size_type find_first_of(CharT c, size_type pos = 0) const noexcept
    {
        return find_first_of(basic_string_view(std::addressof(c), 1), pos);
    }
    CEL_constexpr size_type find_first_of(const CharT* s, size_type pos, size_type count) const
    {
        return find_first_of(basic_string_view(s, count), pos);
    }
    CEL_constexpr size_type find_first_of(const CharT* s, size_type pos = 0) const
    {
        return find_first_of(basic_string_view(s), pos);
    }
    CEL_constexpr size_type find_last_of(basic_string_view v, size_type pos = npos) const noexcept
    {
        for (auto iter = cbegin() + std::min(m_size - 1, pos); iter >= cbegin(); iter--)
            for (auto c : v)
                if (c == *iter)
                    return iter - cbegin();
        return npos;
    }
    CEL_constexpr size_type find_last_of(CharT c, size_type pos = npos) const noexcept
    {
        return find_last_of(basic_string_view(std::addressof(c), 1), pos);
    }
    CEL_constexpr size_type find_last_of(const CharT* s, size_type pos, size_type count) const
    {
        return find_last_of(basic_string_view(s, count), pos);
    }
    CEL_constexpr size_type find_last_of(const CharT* s, size_type pos = npos) const
    {
        return find_last_of(basic_string_view(s), pos);
    }
    CEL_constexpr size_type find_first_not_of(basic_string_view v, size_type pos = 0) const noexcept
    {
        for (size_type i = pos; i < m_size; i++)
            if (v.find(m_data[i]) == npos)
                return i;
        return npos;
    }
    CEL_constexpr size_type find_first_not_of(CharT c, size_type pos = 0) const noexcept
    {
        return find_first_not_of(basic_string_view(std::addressof(c), 1), pos);
    }
    CEL_constexpr size_type find_first_not_of(const CharT* s, size_type pos, size_type count) const
    {
        return find_first_not_of(basic_string_view(s, count), pos);
    }
    CEL_constexpr size_type find_first_not_of(const CharT* s, size_type pos = 0) const
    {
        return find_first_not_of(basic_string_view(s), pos);
    }
    CEL_constexpr size_type find_last_not_of(basic_string_view v, size_type pos = npos) const noexcept
    {
        for (auto iter = cbegin() + std::min(m_size - 1, pos); iter >= cbegin(); iter--)
            if (v.find(*iter) == npos)
                return iter - cbegin();
        return npos;
    }
    CEL_constexpr size_type find_last_not_of(CharT c, size_type pos = npos) const noexcept
    {
        return find_last_not_of(basic_string_view(std::addressof(c), 1), pos);
    }
    CEL_constexpr size_type find_last_not_of(const CharT* s, size_type pos, size_type count) const
    {
        return find_last_not_of(basic_string_view(s, count), pos);
    }
    CEL_constexpr size_type find_last_not_of(const CharT* s, size_type pos = npos) const
    {
        return find_last_not_of(basic_string_view(s), pos);
    }

 private:
    const_pointer   m_data { nullptr };
    size_type       m_size { 0 };
};

using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;

template<class CharT, class Traits>
constexpr bool operator==(basic_string_view <CharT, Traits> lhs,
                          basic_string_view <CharT, Traits> rhs) noexcept
{
    return lhs.compare(rhs) == 0;
}

/*!
 * Helpers missing in C++17 for compatibility with C++11/C++14
 */
template<class CharT, class Traits>
constexpr bool operator==(basic_string_view <CharT, Traits> lhs,
                          basic_string <CharT, Traits> rhs) noexcept
{
    return lhs == basic_string_view<CharT, Traits>(rhs);
}
template<class CharT, class Traits>
constexpr bool operator==(basic_string <CharT, Traits> lhs,
                          basic_string_view <CharT, Traits> rhs) noexcept
{
    return rhs == lhs;
}
template<class CharT, class Traits, size_t N>
constexpr bool operator==(basic_string_view <CharT, Traits> lhs,
                          const CharT (&rhs)[N]) noexcept
{
    return lhs == basic_string_view<CharT, Traits>(rhs, N-1);
}
template<class CharT, class Traits, size_t N>
constexpr bool operator==(const CharT (&lhs)[N],
                          basic_string_view <CharT, Traits> rhs) noexcept
{
    return rhs == lhs;
}
template<class CharT, class Traits, size_t N>
constexpr bool operator==(basic_string_view <CharT, Traits> lhs,
                          const CharT* rhs) noexcept
{
    return lhs.compare(rhs) == 0;
}
template<class CharT, class Traits, size_t N>
constexpr bool operator==(const CharT* lhs,
                          basic_string_view <CharT, Traits> rhs) noexcept
{
    return rhs == lhs;
}

template<class CharT, class Traits>
constexpr bool operator!=(basic_string_view <CharT, Traits> lhs,
                          basic_string_view <CharT, Traits> rhs) noexcept
{
    return lhs.compare(rhs) != 0;
}

/*!
 * Helpers missing in C++17 for compatibility with C++11/C++14
 */
template<class CharT, class Traits>
constexpr bool operator!=(basic_string_view <CharT, Traits> lhs,
                          basic_string <CharT, Traits> rhs) noexcept
{
    return !(lhs == rhs);
}
template<class CharT, class Traits>
constexpr bool operator!=(basic_string <CharT, Traits> lhs,
                          basic_string_view <CharT, Traits> rhs) noexcept
{
    return !(lhs == rhs);
}
template<class CharT, class Traits, size_t N>
constexpr bool operator!=(basic_string_view <CharT, Traits> lhs,
                          const CharT* rhs) noexcept
{
    return !(lhs == rhs);
}
template<class CharT, class Traits, size_t N>
constexpr bool operator!=(const CharT* lhs,
                          basic_string_view <CharT, Traits> rhs) noexcept
{
    return !(lhs == rhs);
}

template<class CharT, class Traits>
constexpr bool operator<(basic_string_view <CharT, Traits> lhs,
                         basic_string_view <CharT, Traits> rhs) noexcept
{
    return lhs.compare(rhs) < 0;
}
template<class CharT, class Traits>
constexpr bool operator<=(basic_string_view <CharT, Traits> lhs,
                          basic_string_view <CharT, Traits> rhs) noexcept
{
    return lhs.compare(rhs) <= 0;
}
template<class CharT, class Traits>
constexpr bool operator>(basic_string_view <CharT, Traits> lhs,
                         basic_string_view <CharT, Traits> rhs) noexcept
{
    return lhs.compare(rhs) > 0;
}
template<class CharT, class Traits>
constexpr bool operator>=(basic_string_view <CharT, Traits> lhs,
                          basic_string_view <CharT, Traits> rhs) noexcept
{
    return lhs.compare(rhs) >= 0;
}

template <class CharT, class Traits>
std::basic_ostream <CharT, Traits>& operator<<(std::basic_ostream <CharT, Traits>& os,
                                              std::basic_string_view <CharT, Traits> v)
{
    os.write(v.data(), v.size());
    return os;
}

namespace
{
template<size_t size = sizeof(size_t)>
struct fnv;

template<>
struct fnv<4>
{
    enum : uint32_t
    {
        offset = 2166136261ul,
        prime  = 16777619ul
    };
};

template<>
struct fnv<8>
{
    enum : uint64_t
    {
        offset = 14695981039346656037ull,
        prime  = 1099511628211ull
    };
};

constexpr size_t fnv1a_loop_helper(const char *begin, const char *end, size_t hash = fnv<>::offset)
{
    return begin < end ? fnv1a_loop_helper(begin + 1, end, (hash ^ size_t(*begin)) * fnv<>::prime) : hash;
}
}

template<typename T>
struct hash;

template<>
struct hash<string_view>
{
    size_t operator()(const string_view& sv) const noexcept
    {
#if 0
        size_t _hash = fnv<>::offset;
        for (char c : sv)
        {
            _hash ^= size_t(c);
            _hash *= fnv<>::prime;
        }
        return _hash;
#else
        return fnv1a_loop_helper(sv.data(), sv.data() + sv.length());
#endif
    }
};
}

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wreserved-user-defined-literal"
//#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuser-defined-literals"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wliteral-suffix"
#endif
constexpr std::string_view operator "" sv(const char *str, size_t len)
{
    return { str, len };
}

constexpr std::wstring_view operator "" sv(const wchar_t *str, size_t len)
{
    return { str, len };
}

constexpr std::string_view operator "" _sv(const char *str, size_t len)
{
    return { str, len };
}

constexpr std::wstring_view operator "" _sv(const wchar_t *str, size_t len)
{
    return { str, len };
}
#if defined(__clang__)
//#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif


#undef CEL_constexpr
