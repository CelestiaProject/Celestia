#pragma once

#include <config.h>

#ifdef HAVE_STRING_VIEW
#include <string_view>
#elif defined(HAVE_EXPERIMENTAL_STRING_VIEW)
#include <experimental/string_view>
namespace std
{
    using string_view = std::experimental::string_view;
}
#else
#include "sv.h"
#endif
