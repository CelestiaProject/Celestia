#pragma once

#include <type_traits>

namespace celestia::numbers
{
namespace detail
{
template<typename T>
using enable_if_fp = std::enable_if_t<std::is_floating_point_v<T>, T>;
}

// constants are given to 128 bits of precision, computed with sollya
template<typename T> constexpr inline T e_v =          detail::enable_if_fp<T>{0x1.5bf0a8b1457695355fb8ac404e7a79e4p1};
template<typename T> constexpr inline T log2e_v =      detail::enable_if_fp<T>{0x1.71547652b82fe1777d0ffda0d23a7d12p0};
template<typename T> constexpr inline T log10e_v =     detail::enable_if_fp<T>{0x1.bcb7b1526e50e32a6ab7555f5a67b864p-2};
template<typename T> constexpr inline T pi_v =         detail::enable_if_fp<T>{0x1.921fb54442d18469898cc51701b839a2p1};
template<typename T> constexpr inline T inv_pi_v =     detail::enable_if_fp<T>{0x1.45f306dc9c882a53f84eafa3ea69bb82p-2};
template<typename T> constexpr inline T inv_sqrtpi_v = detail::enable_if_fp<T>{0x1.20dd750429b6d11ae3a914fed7fd8688p-1};
template<typename T> constexpr inline T ln2_v =        detail::enable_if_fp<T>{0x1.62e42fefa39ef35793c7673007e5ed5ep-1};
template<typename T> constexpr inline T ln10_v =       detail::enable_if_fp<T>{0x1.26bb1bbb5551582dd4adac5705a61452p1};
template<typename T> constexpr inline T sqrt2_v =      detail::enable_if_fp<T>{0x1.6a09e667f3bcc908b2fb1366ea957d3ep0};
template<typename T> constexpr inline T sqrt3_v =      detail::enable_if_fp<T>{0x1.bb67ae8584caa73b25742d7078b83b8ap0};
template<typename T> constexpr inline T inv_sqrt3_v =  detail::enable_if_fp<T>{0x1.279a74590331c4d218f81e4afb257d06p-1};
template<typename T> constexpr inline T egamma_v =     detail::enable_if_fp<T>{0x1.2788cfc6fb618f49a37c7f0202a596aep-1};
template<typename T> constexpr inline T phi_v =        detail::enable_if_fp<T>{0x1.9e3779b97f4a7c15f39cc0605cedc834p0};

constexpr inline double e          = e_v<double>;
constexpr inline double log2e      = log2e_v<double>;
constexpr inline double log10e     = log10e_v<double>;
constexpr inline double pi         = pi_v<double>;
constexpr inline double inv_pi     = inv_pi_v<double>;
constexpr inline double inv_sqrtpi = inv_sqrtpi_v<double>;
constexpr inline double ln2        = ln2_v<double>;
constexpr inline double ln10       = ln10_v<double>;
constexpr inline double sqrt2      = sqrt2_v<double>;
constexpr inline double sqrt3      = sqrt3_v<double>;
constexpr inline double inv_sqrt3  = inv_sqrt3_v<double>;
constexpr inline double egamma     = egamma_v<double>;
constexpr inline double phi        = phi_v<double>;
}
