#pragma once

#include <cstdint>

#pragma pack(push, 1)
struct XYZVBinaryHeader
{
    char magic[8];
    std::uint16_t byteOrder;
    std::uint16_t digits;
    std::uint32_t reserved;
    std::uint64_t count;
};

struct XYZVBinaryData
{
    double tdb;
    double position[3];
    double velocity[3];
};
#pragma pack(pop)
