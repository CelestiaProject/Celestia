#pragma once

#include <config.h>

#ifdef HAVE_CHARCONV
#include <charconv>
namespace celestia
{
namespace compat
{
    using std::from_chars_result;
    using std::chars_format;
    using std::from_chars;
}
}
#else
#include "cc.h"
#endif
