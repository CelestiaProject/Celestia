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
#include <ostream>

#include <celengine/hash.h>
#include <celengine/parser.h>
#include <celengine/value.h>
#include <celutil/logger.h>
#include <celutil/tokenizer.h>
#include "favorites.h"

using celestia::util::GetLogger;

FavoritesList* ReadFavoritesList(std::istream& in)
{
    auto* favorites = new FavoritesList();
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        if (tokenizer.getTokenType() != Tokenizer::TokenString)
        {
            GetLogger()->error("Error parsing favorites file.\n");
            std::for_each(favorites->begin(), favorites->end(), [](FavoritesEntry* fav) { delete fav; });
            delete favorites;
            return nullptr;
        }

        FavoritesEntry* fav = new FavoritesEntry(); // FIXME: check
        fav->name = tokenizer.getStringValue();

        Value* favParamsValue = parser.readValue();
        if (favParamsValue == nullptr || favParamsValue->getType() != Value::HashType)
        {
            GetLogger()->error("Error parsing favorites entry {}\n", fav->name);
            std::for_each(favorites->begin(), favorites->end(), [](FavoritesEntry* fav) { delete fav; });
            delete favorites;
            if (favParamsValue != nullptr)
                delete favParamsValue;
            delete fav;
            return nullptr;
        }

        Hash* favParams = favParamsValue->getHash();

        //If this is a folder, don't get any other params.
        if(!favParams->getBoolean("isFolder", fav->isFolder))
            fav->isFolder = false;
        if(fav->isFolder)
        {
            favorites->push_back(fav);
            continue;
        }

        // Get parentFolder
        favParams->getString("parentFolder", fav->parentFolder);

        // Get position. It is stored as a base in light years with an offset
        // in microlight years. Bad idea, but we need to stay compatible.
        Eigen::Vector3d base(Eigen::Vector3d::Zero());
        Eigen::Vector3d offset(Eigen::Vector3d::Zero());
        favParams->getVector("base", base);
        favParams->getVector("offset", offset);
        fav->position = UniversalCoord::CreateLy(base) + UniversalCoord::CreateLy(offset * 1.0e-6);

        // Get orientation
        Eigen::Vector3d axis = Eigen::Vector3d::UnitX();
        double angle = 0.0;
        favParams->getVector("axis", axis);
        favParams->getNumber("angle", angle);
        fav->orientation = Eigen::Quaternionf(Eigen::AngleAxisf((float) angle, axis.cast<float>()));

        // Get time
        fav->jd = 0.0;
        favParams->getNumber("time", fav->jd);

        // Get the selected object
        favParams->getString("selection", fav->selectionName);

        std::string coordSysName;
        favParams->getString("coordsys", coordSysName);
        if (coordSysName == "ecliptical")
            fav->coordSys = ObserverFrame::Ecliptical;
        else if (coordSysName == "equatorial")
            fav->coordSys = ObserverFrame::Equatorial;
        else if (coordSysName == "geographic")
            fav->coordSys = ObserverFrame::BodyFixed;
        else
            fav->coordSys = ObserverFrame::Universal;

        favorites->push_back(fav);
    }

    return favorites;
}


void WriteFavoritesList(FavoritesList& favorites, std::ostream& out)
{
    for (const auto fav : favorites)
    {
        Eigen::AngleAxisf aa(fav->orientation);
        Eigen::Vector3f axis = aa.axis();
        float angle = aa.angle();

        // Ugly conversion from a universal coordinate to base+offset, with base
        // in light years and offset in microlight years. This is a bad way to
        // store position, but we need to maintain compatibility with older version
        // of Celestia (for now...)
        Eigen::Vector3d baseUly((double) fav->position.x, (double) fav->position.y, (double) fav->position.z);
        Eigen::Vector3d offset((double) (fav->position.x - (BigFix) baseUly.x()),
                               (double) (fav->position.y - (BigFix) baseUly.y()),
                               (double) (fav->position.z - (BigFix) baseUly.z()));
        Eigen::Vector3d base = baseUly * 1e-6; // Base is in micro-light years

        out << '"' << fav->name << "\" {\n";
        if(fav->isFolder)
            out << "\tisFolder " << "true\n";
        else
        {
            out << "\tisFolder " << "false\n";
            out << "\tparentFolder \"" << fav->parentFolder << "\"\n";
            out << std::setprecision(16);
            out << "\tbase   [ " << base.x()   << ' ' << base.y()   << ' ' << base.z()   << " ]\n";
            out << "\toffset [ " << offset.x() << ' ' << offset.y() << ' ' << offset.z() << " ]\n";
            out << std::setprecision(6);
            out << "\taxis   [ " << axis.x() << ' ' << axis.y() << ' ' << axis.z() << " ]\n";
            out << "\tangle  " << angle << '\n';
            out << std::setprecision(16);
            out << "\ttime   " << fav->jd << '\n';
            out << "\tselection \"" << fav->selectionName << "\"\n";
            out << "\tcoordsys \"";
            switch (fav->coordSys)
            {
            case ObserverFrame::Universal:
                out << "universal"; break;
            case ObserverFrame::Ecliptical:
                out << "ecliptical"; break;
            case ObserverFrame::Equatorial:
                out << "equatorial"; break;
            case ObserverFrame::BodyFixed:
                out << "geographic"; break;
            case ObserverFrame::ObserverLocal:
                out << "local"; break;
            case ObserverFrame::PhaseLock:
                out << "phaselock"; break;
            case ObserverFrame::Chase:
                out << "chase"; break;
            default:
                out << "universal"; break;
            }
            out << "\"\n";
        }

        out << "}\n\n";
    }
}
