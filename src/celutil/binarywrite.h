#pragma once

#include <array>
#include <cstddef>
#include <cstring>
#include <ostream>
#include <type_traits>
#include <utility>

#include <celcompat/bit.h>

namespace celestia::util
{

/*! Write a value to an output stream in machine-native byte order.
 */
template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
inline bool
writeNative(std::ostream& out, T value)
{
    return out.write(reinterpret_cast<const char*>(&value), sizeof(T)).good(); //NOSONAR
}

/*! Write a value to an output stream with the opposite of machine-native byte order.
 */
template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
inline bool
writeReversed(std::ostream& out, T value)
{
    if constexpr (sizeof(T) == 1)
        return writeNative(out, value);

    std::array<char, sizeof(T)> temp;
    std::memcpy(temp.data(), &value, sizeof(T));

    constexpr auto halfSize = sizeof(T) / 2;
    for (std::size_t i = 0; i < halfSize; ++i)
    {
        std::swap(temp[i], temp[sizeof(T) - i - 1]);
    }

    return out.write(temp.data(), sizeof(T)).good();
}

/*! Write a value to an output stream in little-endian byte order
 */
template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
inline bool
writeLE(std::ostream& out, T value)
{
    using celestia::compat::endian;
    if constexpr (endian::native == endian::little)
        return writeNative(out, value);
    else
        return writeReversed(out, value);
}

/*! Write a value to an output stream in big-endian byte order
 */
template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
inline bool
writeBE(std::ostream& out, T value)
{
    using celestia::compat::endian;
    if constexpr (endian::native == endian::little)
        return writeReversed(out, value);
    else
        return writeNative(out, value);
}

} // end namespace celestia::util
