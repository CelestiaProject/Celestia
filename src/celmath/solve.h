// solve.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <utility>


// Solve a function using the bisection method.  Returns a pair
// with the solution as the first element and the error as the second.
template<class T, class F> std::pair<T, T> solve_bisection(F f,
                                                           T lower, T upper,
                                                           T err,
                                                           int maxIter = 100)
{
    T x = 0.0;

    for (int i = 0; i < maxIter; i++)
    {
        x = (lower + upper) * (T) 0.5;
        if (upper - lower < 2 * err)
            break;

        T y = f(x);
        if (y < 0)
            lower = x;
        else
            upper = x;
    }

    return std::make_pair(x, (upper - lower) / 2);
}
