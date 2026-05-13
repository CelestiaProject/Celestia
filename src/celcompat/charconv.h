#pragma once

#include <config.h>

#ifdef HAVE_CHARCONV
#include <charconv>
namespace celestia::compat
{
    using ::std::chars_format;
    using ::std::from_chars_result;
    using ::std::from_chars;
}
#else
#include <fast_float/fast_float.h>
namespace celestia::compat
{
    using ::fast_float::chars_format;
    using ::fast_float::from_chars_result;
    using ::fast_float::from_chars;
}
#endif
