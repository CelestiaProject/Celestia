// This file is based on strnatcmp.c by Martin Pool
// Copyright (C) 2000, 2004 by Martin Pool <mbp sourcefrog net>
// Adaptations
// Copyright (C) 2020, the Celestia Development Team
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.


#include "strnatcmp.h"

#include <cwctype>
#include <optional>

#include <celutil/utf8.h>


namespace
{

using MaybeChar = std::optional<wchar_t>;


bool isDigit(MaybeChar c) { return c.has_value() && std::iswdigit(*c); }
bool isSpace(MaybeChar c) { return c.has_value() && std::iswspace(*c); }


class CharMapper
{
public:
    explicit CharMapper(std::string_view _sv) : sv(_sv) {}

    MaybeChar next()
    {
        if (offset >= sv.size())
            return std::nullopt;

        wchar_t ch;
        if (!UTF8Decode(sv, offset, ch))
        {
            offset = sv.size();
            return std::nullopt;
        }

        offset += UTF8EncodedSize(ch);
        return ch;
    }

private:
    std::string_view sv;
    std::string_view::size_type offset{ 0 };
};


int
right(CharMapper& a, MaybeChar& ca, CharMapper& b, MaybeChar& cb)
{
    int bias = 0;

    // The longest run of digits wins.  That aside, the greatest value
    // wins, but we can't know that it will until we've scanned both
    // numbers to know that they have the same magnitude, so we remember
    // it in BIAS.
    for (;;)
    {
        if (!isDigit(ca))
            return isDigit(cb) ? -1 : bias;
        if (!isDigit(cb))
            return +1;

        if (bias == 0)
        {
            if (*ca < *cb)
                bias = -1;
            else if (*ca > *cb)
                bias = 1;
        }

        ca = a.next();
        cb = b.next();
    }
}


int
left(CharMapper& a, MaybeChar& ca, CharMapper& b, MaybeChar& cb)
{
    // Compare two left-aligned numbers: the first to have a
    // different value wins.
    for (;;)
    {
        if (!isDigit(ca))
           return isDigit(cb) ? -1 : 0;
        if (!isDigit(cb))
            return 1;
        if (*ca < *cb)
            return -1;
        if (*ca > *cb)
            return 1;

        ca = a.next();
        cb = b.next();
    }
}

int
strnatcmp0(CharMapper& a, CharMapper& b)
{
    // skip over leading space
    MaybeChar ca = a.next();
    while (isSpace(ca))
        ca = a.next();

    MaybeChar cb = b.next();
    while (isSpace(cb))
        cb = b.next();

    for (;;)
    {
        if (!ca.has_value())
            return cb.has_value() ? -1 : 0;
        if (!cb.has_value())
            return 1;

        // process run of digits
        if (std::iswdigit(*ca) && std::iswdigit(*cb))
        {
            int r = (*ca == L'0' || *cb == L'0')
                ? left(a, ca, b, cb)
                : right(a, ca, b, cb);
            if (r != 0)
                return r;
            continue;
        }

        if (*ca < *cb)
            return -1;

        if (*ca > *cb)
            return 1;

        ca = a.next();
        cb = b.next();
    }
}

} // end unnamed namespace


int
strnatcmp(std::string_view a, std::string_view b)
{
    CharMapper aMapper{a};
    CharMapper bMapper{b};
    return strnatcmp0(aMapper, bMapper);
}
