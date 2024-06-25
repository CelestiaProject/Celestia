//
// C++ Interface: starname
//
// Description:
//
//
// Author: Toti <root@totibox>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string_view>

#include <celengine/name.h>

enum class StarCatalog : unsigned int
{
    HenryDraper,
    SAO,
    _CatalogCount,
};

class StarNameDatabase : private NameDatabase
{
public:
    static constexpr AstroCatalog::IndexNumber TYC3_MULTIPLIER = 1000000000u;
    static constexpr AstroCatalog::IndexNumber TYC2_MULTIPLIER = 10000u;

    StarNameDatabase() = default;

    using NameDatabase::add;
    using NameDatabase::erase;

    using NameDatabase::getFirstNameIter;
    using NameDatabase::getFinalNameIter;

    using NameDatabase::getCompletion;

    // We don't want users to access the getCatalogMethodByName method on the
    // NameDatabase base class, so use private inheritance to enforce usage of
    // the below method:
    AstroCatalog::IndexNumber findCatalogNumberByName(std::string_view, bool i18n) const;

    AstroCatalog::IndexNumber searchCrossIndexForCatalogNumber(StarCatalog, AstroCatalog::IndexNumber number) const;
    AstroCatalog::IndexNumber crossIndex(StarCatalog, AstroCatalog::IndexNumber number) const;

    bool loadCrossIndex(StarCatalog, std::istream&);
    static std::unique_ptr<StarNameDatabase> readNames(std::istream&);

private:
    static constexpr auto NumCatalogs = static_cast<std::size_t>(StarCatalog::_CatalogCount);

    struct CrossIndexEntry
    {
        AstroCatalog::IndexNumber catalogNumber;
        AstroCatalog::IndexNumber celCatalogNumber;
    };

    using CrossIndex = std::vector<CrossIndexEntry>;

    AstroCatalog::IndexNumber findByName(std::string_view, bool) const;
    AstroCatalog::IndexNumber findFlamsteedOrVariable(std::string_view, std::string_view, bool) const;
    AstroCatalog::IndexNumber findBayer(std::string_view, std::string_view, bool) const;
    AstroCatalog::IndexNumber findBayerNoNumber(std::string_view,
                                                std::string_view,
                                                std::string_view,
                                                bool) const;
    AstroCatalog::IndexNumber findBayerWithNumber(std::string_view,
                                                  unsigned int,
                                                  std::string_view,
                                                  std::string_view,
                                                  bool) const;
    AstroCatalog::IndexNumber findWithComponentSuffix(std::string_view, bool) const;

    std::array<CrossIndex, NumCatalogs> crossIndices;
};
