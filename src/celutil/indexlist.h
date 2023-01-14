// indexlist.h
//
// Copyright (C) 2023-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <limits>
#include <vector>

namespace celestia::util
{
template <
    typename T,
    std::enable_if_t<std::is_integral_v<T>, bool> = true
>
constexpr T
IndexListCapacity(T nSlices, T nPoints)
{
    return (nSlices * 2 + 4) * (nPoints - 1) - 2;
}

template <
    typename T,
    std::enable_if_t<std::is_integral_v<T>, bool> = true
>
void
BuildIndexList(T I, T J, std::vector<T> &out)
{
    assert(std::numeric_limits<T>::max() / I >= J);

    out.reserve(IndexListCapacity(I, J)); // noop if current capacity >= new one

    for (auto i = static_cast<T>(0); i < I; i++)
    {
        T baseVertex = i * J;

        if (i > static_cast<T>(0))
            out.push_back(baseVertex); // degenerate triangle

        for (auto j = static_cast<T>(0); j < J; j++)
        {
            T v = baseVertex + j;
            out.push_back(v);
            out.push_back(v + J);
        }
        out.push_back(baseVertex);
        out.push_back(baseVertex + J);

        if (i < I - static_cast<T>(1))
            out.push_back(baseVertex + J); // degenerate triangle
    }
}

template <
    typename T,
    std::enable_if_t<std::is_integral_v<T>, bool> = true
>
void
BuildIndexList(T I, T J, T* out)
{
    assert(std::numeric_limits<T>::max() / I >= J);

    for (auto i = static_cast<T>(0); i < I; i++)
    {
        T baseVertex = i * J;

        if (i > static_cast<T>(0))
            *out++ = baseVertex; // degenerate triangle

        for (auto j = static_cast<T>(0); j < J; j++)
        {
            T v = baseVertex + j;
            *out++ = v;
            *out++ = v + J;
        }
        *out++ = baseVertex;
        *out++ = baseVertex + J;

        if (i < I - static_cast<T>(1))
            *out++ = baseVertex + J; // degenerate triangle
    }
}

}
