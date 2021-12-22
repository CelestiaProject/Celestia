#pragma once

#include <cstring>
#include <ostream>
#include <type_traits>
#include <utility>

#include <config.h>


namespace celestia::util
{

/*! Write a value to an output stream in machine-native byte order.
 */
template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
inline bool writeNative(std::ostream& out, T value)
{
    char data[sizeof(T)];
    std::memcpy(data, &value, sizeof(T));
    return out.write(data, sizeof(T)).good();
}

template<>
inline bool writeNative<char>(std::ostream& out, char value)
{
    return out.put(value).good();
}

template<>
inline bool writeNative<signed char>(std::ostream& out, signed char value)
{
    return out.put(static_cast<char>(value)).good();
}

template<>
inline bool writeNative<unsigned char>(std::ostream& out, unsigned char value)
{
    return out.put(static_cast<unsigned char>(value)).good();
}


/*! Write a value to an output stream with the opposite of machine-native byte order.
 */
template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
inline bool writeReversed(std::ostream& out, T value)
{
    char data[sizeof(T)];
    std::memcpy(data, &value, sizeof(T));
    for (std::size_t i = 0; i < sizeof(T) / 2; ++i)
    {
        std::swap(data[i], data[sizeof(T) - i - 1]);
    }
    return out.write(data, sizeof(T)).good();
}

template<>
inline bool writeReversed<char>(std::ostream& out, char value)
{
    return writeNative(out, value);
}

template<>
inline bool writeReversed<signed char>(std::ostream& out, signed char value)
{
    return writeNative(out, value);
}

template<>
inline bool writeReversed<unsigned char>(std::ostream& out, unsigned char value)
{
    return writeNative(out, value);
}

#ifdef WORDS_BIGENDIAN

/*! Write a value to an output stream in little-endian byte order
 */
template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
inline bool writeLE(std::ostream& out, T value)
{
    return writeReversed(out, value);
}

/*! Write a value to an output stream in big-endian byte order
 */
template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
inline bool writeBE(std::ostream& out, T value)
{
    return writeNative(out, value);
}

#else

/*! Write a value to an output stream in little-endian byte order
 */
template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
inline bool writeLE(std::ostream& out, T value)
{
    return writeNative(out, value);
}

/*! Write a value to an output stream in big-endian byte order
 */
template<typename T, typename = std::enable_if_t<std::is_trivially_copyable<T>::value>>
inline bool writeBE(std::ostream& out, T value)
{
    return writeReversed(out, value);
}

#endif

} // end namespace celestia::util
