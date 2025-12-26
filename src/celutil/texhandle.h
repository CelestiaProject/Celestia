#pragma once

#include <cstdint>

namespace celestia::util
{
enum class TextureHandle : std::uint32_t
{
    Invalid = ~UINT32_C(0),
};
}
