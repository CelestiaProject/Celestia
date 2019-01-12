#pragma once

#include <stdint.h>

struct XYZVBinaryHeader
{
    char magic[8];
    uint16_t byteOrder;
    uint16_t digits;
    uint32_t reserved;
    uint64_t count;
};

struct XYZVBinaryData
{
    double tdb;
    double position[3];
    double velocity[3];
};
