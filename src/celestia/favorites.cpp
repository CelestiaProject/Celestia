// favorites.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <iomanip>
#include <limits>
#include <ostream>
#include <utility>

#include <celutil/associativearray.h>
#include <celutil/logger.h>
#include <celutil/parser.h>
#include <celutil/tokenizer.h>
#include "favorites.h"

namespace util = celestia::util;

using celestia::util::GetLogger;

std::unique_ptr<FavoritesList>
ReadFavoritesList(std::istream& in)
{
    auto favorites = std::make_unique<FavoritesList>();
    util::Tokenizer tokenizer(&in);
    util::Parser parser(&tokenizer);

    while (tokenizer.nextToken() != util::Tokenizer::TokenEnd)
    {
        if (tokenizer.getTokenType() != util::Tokenizer::TokenString)
        {
            GetLogger()->error("Error parsing favorites file.\n");
            return nullptr;
        }

        auto fav = std::make_unique<FavoritesEntry>(); // FIXME: check
        fav->name = *tokenizer.getStringValue();

        const util::Value favParamsValue = parser.readValue();
        const util::AssociativeArray* favParams = favParamsValue.getHash();
        if (favParams == nullptr)
        {
            GetLogger()->error("Error parsing favorites entry {}\n", fav->name);
            return nullptr;
        }

        //If this is a folder, don't get any other params.
        fav->isFolder = favParams->getBoolean("isFolder").value_or(false);
        if (fav->isFolder)
        {
            favorites->push_back(std::move(fav));
            continue;
        }

        // Get parentFolder
        if (const std::string* parentFolder = favParams->getString("parentFolder"); parentFolder != nullptr)
            fav->parentFolder = *parentFolder;

        // Get position. It is stored as a base in light years with an offset
        // in microlight years. Bad idea, but we need to stay compatible.
        auto base = favParams->getVector3<double>("base").value_or(Eigen::Vector3d::Zero());
        auto offset = favParams->getVector3<double>("offset").value_or(Eigen::Vector3d::Zero());
        fav->position = UniversalCoord::CreateLy(base) + UniversalCoord::CreateLy(offset * 1.0e-6);

        // Get orientation
        auto axis = favParams->getVector3<double>("axis").value_or(Eigen::Vector3d::UnitX());
        auto angle = favParams->getNumber<double>("angle").value_or(0.0);
        fav->orientation = Eigen::Quaternionf(Eigen::AngleAxisf((float) angle, axis.cast<float>()));

        // Get time
        fav->jd = favParams->getNumber<double>("time").value_or(0.0);

        // Get the selected object
        if (const std::string* selectionName = favParams->getString("selection"); selectionName != nullptr)
            fav->selectionName = *selectionName;

        if (const std::string* coordSysName = favParams->getString("coordsys"); coordSysName != nullptr)
        {
            if (*coordSysName == "ecliptical")
                fav->coordSys = ObserverFrame::CoordinateSystem::Ecliptical;
            else if (*coordSysName == "equatorial")
                fav->coordSys = ObserverFrame::CoordinateSystem::Equatorial;
            else if (*coordSysName == "geographic")
                fav->coordSys = ObserverFrame::CoordinateSystem::BodyFixed;
            else
                fav->coordSys = ObserverFrame::CoordinateSystem::Universal;
        }

        favorites->push_back(std::move(fav));
    }

    return favorites;
}


void WriteFavoritesList(FavoritesList& favorites, std::ostream& out)
{
    for (const auto& fav : favorites)
    {
        Eigen::AngleAxisf aa(fav->orientation);
        Eigen::Vector3f axis = aa.axis();
        float angle = aa.angle();

        // Ugly conversion from a universal coordinate to base+offset, with base
        // in light years and offset in microlight years. This is a bad way to
        // store position, but we need to maintain compatibility with older version
        // of Celestia (for now...)
        Eigen::Vector3d baseUly((double) fav->position.x, (double) fav->position.y, (double) fav->position.z);
        Eigen::Vector3d offset((double) (fav->position.x - R128(baseUly.x())),
                               (double) (fav->position.y - R128(baseUly.y())),
                               (double) (fav->position.z - R128(baseUly.z())));
        Eigen::Vector3d base = baseUly * 1e-6; // Base is in micro-light years

        out << '"' << fav->name << "\" {\n";
        if(fav->isFolder)
            out << "\tisFolder " << "true\n";
        else
        {
            out << "\tisFolder " << "false\n";
            out << "\tparentFolder \"" << fav->parentFolder << "\"\n";
            out << std::setprecision(std::numeric_limits<double>::max_digits10);
            out << "\tbase   [ " << base.x()   << ' ' << base.y()   << ' ' << base.z()   << " ]\n";
            out << "\toffset [ " << offset.x() << ' ' << offset.y() << ' ' << offset.z() << " ]\n";
            out << std::setprecision(std::numeric_limits<float>::max_digits10);
            out << "\taxis   [ " << axis.x() << ' ' << axis.y() << ' ' << axis.z() << " ]\n";
            out << "\tangle  " << angle << '\n';
            out << std::setprecision(std::numeric_limits<double>::max_digits10);
            out << "\ttime   " << fav->jd << '\n';
            out << "\tselection \"" << fav->selectionName << "\"\n";
            out << "\tcoordsys \"";
            switch (fav->coordSys)
            {
            case ObserverFrame::CoordinateSystem::Universal:
                out << "universal"; break;
            case ObserverFrame::CoordinateSystem::Ecliptical:
                out << "ecliptical"; break;
            case ObserverFrame::CoordinateSystem::Equatorial:
                out << "equatorial"; break;
            case ObserverFrame::CoordinateSystem::BodyFixed:
                out << "geographic"; break;
            case ObserverFrame::CoordinateSystem::ObserverLocal:
                out << "local"; break;
            case ObserverFrame::CoordinateSystem::PhaseLock:
                out << "phaselock"; break;
            case ObserverFrame::CoordinateSystem::Chase:
                out << "chase"; break;
            default:
                out << "universal"; break;
            }
            out << "\"\n";
        }

        out << "}\n\n";
    }
}
