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


#include <cwctype>
#include <celutil/utf8.h>
#include "strnatcmp.h"

static int right(const wchar_t *a, const wchar_t *b)
{
    int bias = 0;
    wchar_t ca, cb;

    // The longest run of digits wins.  That aside, the greatest value
    // wins, but we can't know that it will until we've scanned both
    // numbers to know that they have the same magnitude, so we remember
    // it in BIAS.
    for (;; a++, b++)
    {
        ca = *a;
        cb = *b;

        if (!iswdigit(ca))
            return iswdigit(cb) ? -1 : bias;
        if (!iswdigit(cb))
            return +1;

        if (ca < cb)
        {
            if (bias == 0)
                bias = -1;
        }
        else if (ca > cb)
        {
            if (bias == 0)
                bias = +1;
        }
        else if (ca == 0 && cb == 0)
        {
            return bias;
        }
    }
    return 0;
}


static int left(const wchar_t *a, const wchar_t *b)
{
    wchar_t ca, cb;

    // Compare two left-aligned numbers: the first to have a
    // different value wins.
    for (;; a++, b++)
    {
        ca = *a;
        cb = *b;

        if (!iswdigit(ca))
           return iswdigit(cb) ? -1 : 0;
        if (!iswdigit(cb))
            return +1;
        if (ca < cb)
            return -1;
        if (ca > cb)
            return +1;
    }

    return 0;
}

static int strnatcmp0(const std::wstring &a, const std::wstring &b, bool fold_case)
{
    size_t ai = 0, bi = 0;
    wchar_t ca, cb;
    for (;;)
    {
        // skip over leading spaces
        ca = a[ai];
        while(iswspace(ca))
            ca = a[++ai];

        cb = b[bi];
        while(iswspace(cb))
            cb = b[++bi];

        // process run of digits
        if (iswdigit(ca) && iswdigit(cb))
        {
            bool frac = (ca == L'0' || cb == L'0');
            int r = frac ? left(&a[ai], &b[bi]) : right(&a[ai], &b[bi]);
            if (r != 0)
                return r;
        }

        if (ca == 0 && cb == 0)
        {
            // The strings compare the same. Perhaps the caller
            // will want to call strcmp to break the tie.
            return 0;
        }

        if (fold_case)
        {
            ca = towupper(ca);
            cb = towupper(cb);
        }

        if (ca < cb)
            return -1;

        if (ca > cb)
            return +1;

        ai++;
        bi++;
    }
    return 0;
}

static std::wstring utf8_to_wide(const std::string &s)
{
    std::wstring w;
    wchar_t ch;
    for (size_t i = 0, e = s.length(); i < e; )
    {
        if (!UTF8Decode(s, i, ch))
            break;
        i += UTF8EncodedSize(ch);
        w.push_back(ch);
    }
    w.push_back(L'\0');
    return w;
}

int strnatcmp(const std::string &a, const std::string &b)
{
    return strnatcmp0(utf8_to_wide(a), utf8_to_wide(b), false);
}

int strnatcmp(const std::wstring &wa, const std::wstring &wb)
{
    return strnatcmp0(wa, wb, false);
}

// Compare, recognizing numeric std::string and ignoring case.
int strnatcasecmp(const std::string &a, const std::string &b)
{
    return strnatcmp0(utf8_to_wide(a), utf8_to_wide(b), true);
}

int strnatcasecmp(const std::wstring &wa, const std::wstring &wb)
{
    return strnatcmp0(wa, wb, true);
}
