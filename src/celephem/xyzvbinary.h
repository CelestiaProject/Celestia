#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include <celutil/bytes.h>

namespace celestia::ephem
{

struct XYZVBinaryHeader
{
    std::array<char, 8> magic;
    util::NE<std::uint16_t> byteOrder;
    util::NE<std::uint16_t> digits;
    util::NE<std::uint32_t> reserved;
    util::NE<std::uint64_t> count;
};

struct XYZVBinaryData
{
    util::NE<double> tdb;
    std::array<util::NE<double>, 3> position;
    std::array<util::NE<double>, 3> velocity;
};

constexpr inline std::string_view XYZV_MAGIC{ "CELXYZV\0", 8 };
static_assert(XYZV_MAGIC.size() == sizeof(XYZVBinaryHeader::magic));

}
