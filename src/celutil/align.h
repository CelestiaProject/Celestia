#pragma once

#include <cstddef>
#include <cstdint>

namespace celestia::util
{

/*! Returns aligned position for type T inside memory region.
 *  Designed for usage with functions which allocate unaligned
 *  memory regions, as such void* is the appropriate type.
 */
template<typename T> T* aligned_addr(void* addr) //NOSONAR
{
    // alignof(T) is guaranteed to be a power of two
    constexpr auto align_one = static_cast<std::uintptr_t>(alignof(T) - 1);
    return reinterpret_cast<T*>((reinterpret_cast<std::uintptr_t>(addr) + align_one) & (~align_one));
}

/*! Returns size large enough so an object of type T can be placed
 *  inside allocated unaligned memory region with its address aligned.
 */
template<typename T> constexpr std::size_t aligned_sizeof()
{
    return sizeof(T) + alignof(T) - 1;
}

}
