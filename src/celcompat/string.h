#pragma once

#include <string>

namespace compat
{
#if __has_cpp_attribute(__cpp_char8_t)
    using char8_t = std::char8_t;
    using u8string = std::u8string;
#else
    using char8_t = unsigned char;
    using u8string = std::basic_string<char8_t>;
#endif
}
