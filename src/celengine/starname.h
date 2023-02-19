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
#include <string_view>

#include <celengine/name.h>


class StarNameDatabase: public NameDatabase
{
 public:
    StarNameDatabase() {};

    std::uint32_t findCatalogNumberByName(std::string_view, bool i18n) const;

    static StarNameDatabase* readNames(std::istream&);

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
