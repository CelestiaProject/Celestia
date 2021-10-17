#pragma once

#include <config.h>

#ifdef HAVE_STRING_VIEW
#include <string_view>
namespace celestia
{
namespace compat
{
    using std::basic_string_view;
    using std::string_view;
}
}
#elif defined(HAVE_EXPERIMENTAL_STRING_VIEW)
#include <experimental/string_view>
namespace celestia
{
namespace compat
{
    using std::experimental::basic_string_view;
    using std::experimental::string_view;
}
}
#else
#include <fmt/format.h>
#include "sv.h"

template<>
struct fmt::formatter<celestia::compat::string_view> : formatter<fmt::string_view>
{
    template<typename FormatContext>
    auto format(celestia::compat::string_view v, FormatContext& ctx)
    {
        fmt::string_view f{v.data(), v.size()};
        return formatter<fmt::string_view>::format(f, ctx);
    }
};
#endif
