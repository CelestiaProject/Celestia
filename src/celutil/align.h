#pragma once

#include <type_traits>

/*! Returns aligned position for type T inside memory region.
 *  Designed for usage with functions which allocate unalligned
 *  memory regions.
 */
template<typename T> T* aligned_addr(T* addr)
{
    size_t align = std::alignment_of<T>::value;
    return (T*) (((size_t(addr) - 1) / align + 1) * align);
};

template<typename T> T* aligned_addr(T* addr, size_t align)
{
    return (T*) (((size_t(addr) - 1) / align + 1) * align);
};

/*! Returns size large enough so an object of type T can be placed
 *  inside allocated unalligned memory region with its address aligned.
 */
template<typename T> size_t aligned_sizeof()
{
    return sizeof(T) + std::alignment_of<T>::value - 1;
};
