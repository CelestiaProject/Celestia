#pragma once

#if __cpp_lib_math_constants >= 201907L
#include <numbers>
namespace celestia::numbers
{
using std::numbers::e_v;
using std::numbers::log2e_v;
using std::numbers::log10e_v;
using std::numbers::pi_v;
using std::numbers::inv_pi_v;
using std::numbers::inv_sqrtpi_v;
using std::numbers::ln2_v;
using std::numbers::ln10_v;
using std::numbers::sqrt2_v;
using std::numbers::sqrt3_v;
using std::numbers::inv_sqrt3_v;
using std::numbers::egamma_v;
using std::numbers::phi_v;

using std::numbers::e;
using std::numbers::log2e;
using std::numbers::log10e;
using std::numbers::pi;
using std::numbers::inv_pi;
using std::numbers::inv_sqrtpi;
using std::numbers::ln2;
using std::numbers::ln10;
using std::numbers::sqrt2;
using std::numbers::sqrt3;
using std::numbers::inv_sqrt3;
using std::numbers::egamma;
using std::numbers::phi;
}
#else
#include "numbers_impl.h"
#endif
