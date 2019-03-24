
#include <cstdio>

#include "astrocat.h" 

AstroCatalog::IndexNumber SimpleAstroCatalog::getIndexNumberByName(const std::string &name)
{
    if (compareIgnoringCase(name, m_prefix, m_prefix.length()) == 0)
    {
        IndexNumber num;
        // Use scanf to see if we have a valid catalog number; it must be
        // of the form: <prefix> <non-negative integer>  No additional
        // characters other than whitespace are allowed after the number.
        char extra[4];
        if (std::sscanf(name.c_str() + m_prefix.length(), " %u %c", &num, extra) == 1)
        {
            return num;
        }
    }
    return InvalidIndex;
}

std::string SimpleAstroCatalog::getNameByIndexNumber(IndexNumber index)
{
    return fmt::sprintf("%s %d", m_prefix, index);
}

const std::string& SimpleAstroCatalog::getPrefix() const
{
    return m_prefix;
}

AstroCatalog::IndexNumber TychoAstroCatalog::getIndexNumberByName(const std::string& name)
{
    int len = m_prefix.size();
    if (compareIgnoringCase(name, m_prefix, len) == 0)
    {
        unsigned int tyc1 = 0, tyc2 = 0, tyc3 = 0;
        if (std::sscanf(std::string(name, len, std::string::npos).c_str(),
                   " %u-%u-%u", &tyc1, &tyc2, &tyc3) == 3)
        {
            return (tyc3 * 1000000000 + tyc2 * 10000 + tyc1);
        }
    }
    return InvalidIndex;
}

std::string TychoAstroCatalog::getNameByIndexNumber(IndexNumber index)
{
    uint32_t tyc3 = index / 1000000000;
    index -= tyc3 * 1000000000;
    uint32_t tyc2 = index / 10000;
    index -= tyc2 * 10000;
    uint32_t tyc1 = index;
    return fmt::sprintf("%s %d-%d-%d", m_prefix, tyc1, tyc2, tyc3);
}

AstroCatalog::IndexNumber CelestiaAstroCatalog::getIndexNumberByName(const std::string &name)
{
    char extra[4];
    IndexNumber index = SimpleAstroCatalog::getIndexNumberByName(name);
    if (index != InvalidIndex)
        return index;
    if (std::sscanf(name.c_str(), "#%u %c", &index, extra) == 1)
        return index;
    return InvalidIndex;
}
