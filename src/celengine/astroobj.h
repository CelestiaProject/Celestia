#pragma once

#include <cstdint>

#include <celengine/selection.h>

class UserCategory;

namespace AstroCatalog
{
    using IndexNumber = std::uint32_t;
    constexpr inline IndexNumber InvalidIndex = UINT32_MAX;
};

class AstroObject
{
    AstroCatalog::IndexNumber m_mainIndexNumber { AstroCatalog::InvalidIndex };
public:
    virtual ~AstroObject() = default;

    virtual Selection toSelection() = 0;
    AstroCatalog::IndexNumber getIndex() const { return m_mainIndexNumber; }
    void setIndex(AstroCatalog::IndexNumber);
};
