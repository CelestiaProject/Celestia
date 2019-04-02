
#include <cstdio>

#include "astrocat.h" 

AstroCatalog::IndexNumber SimpleAstroCatalog::nameToCatalogNumber(const std::string &name)
{
    if (compareIgnoringCase(name, m_prefix, m_prefix.length()) == 0)
    {
        unsigned int num;
        // Use scanf to see if we have a valid catalog number; it must be
        // of the form: <prefix> <non-negative integer>  No additional
        // characters other than whitespace are allowed after the number.
        char extra[4];
        if (std::sscanf(name.c_str() + m_prefix.length(), " %u %c", &num, extra) == 1)
        {
            return static_cast<IndexNumber>(num);
        }
    }
    return InvalidIndex;
}

const std::string& SimpleAstroCatalog::getName() const
{
    return getPrefix();
}

std::string SimpleAstroCatalog::catalogNumberToName(IndexNumber index)
{
    return fmt::sprintf("%s %u", m_prefix, static_cast<unsigned int>(index));
}

AstroCatalog::IndexNumber TychoAstroCatalog::nameToCatalogNumber(const std::string& name)
{
    int len = m_prefix.length();
    if (compareIgnoringCase(name, m_prefix, len) == 0)
    {
        unsigned int tyc1 = 0, tyc2 = 0, tyc3 = 0;
        if (std::sscanf(name.c_str() + len,
                   " %u-%u-%u", &tyc1, &tyc2, &tyc3) == 3)
        {
            return (tyc3 * 1000000000 + tyc2 * 10000 + tyc1);
        }
    }
    return InvalidIndex;
}

std::string TychoAstroCatalog::catalogNumberToName(IndexNumber index)
{
    uint32_t tyc3 = index / 1000000000;
    index -= tyc3 * 1000000000;
    uint32_t tyc2 = index / 10000;
    index -= tyc2 * 10000;
    uint32_t tyc1 = index;
    return fmt::sprintf("%s %u-%u-%u", m_prefix, tyc1, tyc2, tyc3);
}

constexpr int HipparcosAstroCatalog::MaxCatalogNumber;
