#pragma once

#include <array>
#include <cstddef>
#include <cstring>
#include <istream>
#include <type_traits>
#include <utility>

#include <celcompat/bit.h>

namespace celestia::util
{

/*! Read a value stored in machine-native byte order from an input stream.
 */
template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
inline bool
readNative(std::istream& in, T& value)
{
    return in.read(reinterpret_cast<char*>(&value), sizeof(T)).good(); /* Flawfinder: ignore */ //NOSONAR
}

/*! Read a value stored in machine-native byte order from memory.
 */
template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
inline T
fromMemoryNative(const void* src) //NOSONAR
{
    T value;
    std::memcpy(&value, src, sizeof(T));
    return value;
}

/*! Read a value stored opposite to machine-native byte order from an input stream.
 */
template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
inline bool
readReversed(std::istream& in, T& value)
{
    if constexpr (sizeof(T) == 1)
        return readNative(in, value);

    std::array<char, sizeof(T)> temp;
    if (!in.read(temp.data(), sizeof(T)).good()) /* Flawfinder: ignore */
        return false;

    constexpr auto halfSize = sizeof(T) / 2;
    for (std::size_t i = 0; i < halfSize; ++i)
        std::swap(temp[i], temp[sizeof(T) - i - 1]);

    std::memcpy(&value, temp.data(), sizeof(T));
    return true;
}

/*! Read a value stored opposite to machine-native byte order from memory.
 */
template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
inline T
fromMemoryReversed(const void* src) //NOSONAR
{
    if constexpr (sizeof(T) == 1)
        return fromMemoryNative<T>(src);

    std::array<std::byte, sizeof(T)> temp;
    std::memcpy(temp.data(), src, sizeof(T));
    constexpr auto halfSize = sizeof(T) / 2;
    for (std::size_t i = 0; i < halfSize; ++i)
        std::swap(temp[i], temp[sizeof(T) - i - 1]);

    T value;
    std::memcpy(&value, temp.data(), sizeof(T));
    return value;
}

/*! Read a value stored in little-endian byte order from an input stream.
 */
template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
inline bool
readLE(std::istream& in, T& value)
{
    using celestia::compat::endian;
    if constexpr (endian::native == endian::little)
        return readNative(in, value);
    else
        return readReversed(in, value);
}

/*! Read a value stored in little-endian byte order from memory.
 */
template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
inline T
fromMemoryLE(const void* src) //NOSONAR
{
    using celestia::compat::endian;
    if constexpr (endian::native == endian::little)
        return fromMemoryNative<T>(src);
    else
        return fromMemoryReversed<T>(src);
}

/*! Read a value stored in big-endian byte order from an input stream.
 */
template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
inline bool
readBE(std::istream& in, T& value)
{
    using celestia::compat::endian;
    if constexpr (endian::native == endian::little)
        return readReversed(in, value);
    else
        return readNative(in, value);
}

/*! Read a value stored in big-endian byte order from memory.
 */
template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
inline T
fromMemoryBE(const void* src) //NOSONAR
{
    using celestia::compat::endian;
    if constexpr (endian::native == endian::little)
        return fromMemoryReversed<T>(src);
    else
        return fromMemoryNative<T>(src);
}

} // end namespace celestia::util
