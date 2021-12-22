#pragma once

#include <config.h>

#ifdef HAVE_FLOAT_CHARCONV
#include <charconv>
namespace celestia::compat
{
    using std::chars_format;
    using std::from_chars_result;
    using std::from_chars;
}
#else
#include "cc.h"
#endif
