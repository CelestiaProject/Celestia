
#pragma once

#include <climits>
#include <string>
#include <celutil/util.h>
#include <fmt/printf.h>

class AstroCatalog
{
public:
    typedef unsigned int IndexNumber;
    static const IndexNumber InvalidIndex = UINT_MAX;
    virtual IndexNumber getIndexNumberByName(const std::string&) = 0;
    virtual std::string getNameByIndexNumber(IndexNumber) = 0;
    virtual const std::string& getPrefix() const = 0;
};

class SimpleAstroCatalog : public AstroCatalog
{
protected:
    std::string m_prefix;
public:
    SimpleAstroCatalog(const std::string& pref) { m_prefix = pref; }
    virtual IndexNumber getIndexNumberByName(const std::string&);
    virtual std::string getNameByIndexNumber(IndexNumber);
    virtual const std::string& getPrefix() const;
};

class TychoAstroCatalog : public SimpleAstroCatalog
{
public:
    TychoAstroCatalog() : SimpleAstroCatalog("TYC") {}
    virtual IndexNumber getIndexNumberByName(const std::string&);
    virtual std::string getNameByIndexNumber(IndexNumber);
};

class CelestiaAstroCatalog : public SimpleAstroCatalog
{
public:
    CelestiaAstroCatalog() : SimpleAstroCatalog("CEL") {}
    virtual IndexNumber getIndexNumberByName(const std::string&);
};

class HenryDrapperCatalog : public SimpleAstroCatalog
{
public:
    HenryDrapperCatalog() : SimpleAstroCatalog("HD") {}
};

class GlieseAstroCatalog : public SimpleAstroCatalog
{
public:
    GlieseAstroCatalog() : SimpleAstroCatalog("Gliese") {}
};

class SAOAstroCatalog : public SimpleAstroCatalog
{
public:
    SAOAstroCatalog() : SimpleAstroCatalog("SAO") {}
};

class HipparcosAstroCatalog : public SimpleAstroCatalog
{
public:
    HipparcosAstroCatalog() : SimpleAstroCatalog("HIP") {}
};

