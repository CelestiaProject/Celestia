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

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string_view>

#include <celengine/name.h>

class StarNameDatabase: private NameDatabase
{
public:
    StarNameDatabase() {};

    using NameDatabase::add;
    using NameDatabase::erase;

    using NameDatabase::getFirstNameIter;
    using NameDatabase::getFinalNameIter;

    using NameDatabase::getCompletion;

    // We don't want users to access the getCatalogMethodByName method on the
    // NameDatabase base class, so use private inheritance to enforce usage of
    // the below method:
    std::uint32_t findCatalogNumberByName(std::string_view, bool i18n) const;

    static std::unique_ptr<StarNameDatabase> readNames(std::istream&);

private:
    std::uint32_t findFlamsteedOrVariable(std::string_view, std::string_view, bool) const;
    std::uint32_t findBayer(std::string_view, std::string_view, bool) const;
    std::uint32_t findBayerNoNumber(std::string_view,
                                    std::string_view,
                                    std::string_view,
                                    bool) const;
    std::uint32_t findBayerWithNumber(std::string_view,
                                      unsigned int,
                                      std::string_view,
                                      std::string_view,
                                      bool) const;
    std::uint32_t findWithComponentSuffix(std::string_view, bool) const;
};
