#pragma once

#include <cstring>
#include <istream>
#include <type_traits>
#include <utility>

#include <config.h>


namespace celestia
{
namespace util
{
/*! Read a value stored in machine-native byte order from an input stream.
 */
template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
inline bool readNative(std::istream& in, T& value)
{
    char data[sizeof(T)];
    if (!in.read(data, sizeof(T)).good()) { return false; }
    std::memcpy(&value, data, sizeof(T));
    return true;
}

template<>
inline bool readNative<char>(std::istream& in, char& value)
{
    return in.get(value).good();
}

template<>
inline bool readNative<signed char>(std::istream& in, signed char& value)
{
    char c;
    if (!in.get(c).good()) { return false; }
    value = static_cast<signed char>(c);
    return true;
}

template<>
inline bool readNative<unsigned char>(std::istream& in, unsigned char& value)
{
    char c;
    if (!in.get(c).good()) { return false; }
    value = static_cast<unsigned char>(c);
    return true;
}


/*! Read a value stored opposite to machine-native byte order from an input stream
 */
template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
inline bool readReversed(std::istream& in, T& value)
{
    char data[sizeof(T)];
    if (!in.read(data, sizeof(T)).good()) { return false; }
    for (std::size_t i = 0; i < sizeof(T) / 2; ++i)
    {
        std::swap(data[i], data[sizeof(T) - i - 1]);
    }

    std::memcpy(&value, data, sizeof(T));
    return true;
}

template<>
inline bool readReversed<char>(std::istream& in, char& value)
{
    return readNative(in, value);
}

template<>
inline bool readReversed<signed char>(std::istream& in, signed char& value)
{
    return readNative(in, value);
}

template<>
inline bool readReversed<unsigned char>(std::istream& in, unsigned char& value)
{
    return readNative(in, value);
}

#ifdef WORDS_BIGENDIAN

/*! Read a value stored in little-endian byte order from an input stream.
 */
template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
inline bool readLE(std::istream& in, T& value)
{
    return readReversed(in, value);
}

/*! Read a value stored in big-endian byte order from an input stream.
 */
template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
inline bool readBE(std::istream& in, T& value)
{
    return readNative(in, value);
}

#else

/*! Read a value stored in little-endian byte order from an input stream.
 */
template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
inline bool readLE(std::istream& in, T& value)
{
    return readNative(in, value);
}

/*! Read a value stored in big-endian byte order from an input stream.
 */
template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
inline bool readBE(std::istream& in, T& value)
{
    return readReversed(in, value);
}

#endif

}
}
