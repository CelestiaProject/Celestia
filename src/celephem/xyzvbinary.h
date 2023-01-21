#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>

namespace celestia::ephem
{

#pragma pack(push, 1)
struct XYZVBinaryHeader
{
    XYZVBinaryHeader() = delete;

    char magic[8];
    std::uint16_t byteOrder;
    std::uint16_t digits;
    std::uint32_t reserved;
    std::uint64_t count;
};

struct XYZVBinaryData
{
    XYZVBinaryData() = delete;

    double tdb;
    double position[3];
    double velocity[3];
};

#pragma pack(pop)

static_assert(std::is_standard_layout_v<XYZVBinaryHeader>);
static_assert(std::is_standard_layout_v<XYZVBinaryData>);

constexpr inline std::string_view XYZV_MAGIC{ "CELXYZV\0", 8 };
static_assert(XYZV_MAGIC.size() == sizeof(XYZVBinaryHeader::magic));

}
