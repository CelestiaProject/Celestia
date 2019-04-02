
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
    virtual IndexNumber nameToCatalogNumber(const std::string&) = 0;
    virtual std::string catalogNumberToName(IndexNumber) = 0;
    virtual const std::string& getName() const = 0;
};

class SimpleAstroCatalog : public AstroCatalog
{
 protected:
    std::string m_prefix;
 public:
    SimpleAstroCatalog(const std::string& pref) { m_prefix = pref; }
    virtual IndexNumber nameToCatalogNumber(const std::string&) override;
    virtual std::string catalogNumberToName(IndexNumber) override;
    virtual const std::string& getName() const override;
    const std::string& getPrefix() const { return m_prefix; };
};

class TychoAstroCatalog : public SimpleAstroCatalog
{
 public:
    TychoAstroCatalog() : SimpleAstroCatalog("TYC") {}
    virtual IndexNumber nameToCatalogNumber(const std::string&);
    virtual std::string catalogNumberToName(IndexNumber);
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
    static constexpr int MaxCatalogNumber = 2000000; // Approximately
    HipparcosAstroCatalog() : SimpleAstroCatalog("HIP") {}
};

