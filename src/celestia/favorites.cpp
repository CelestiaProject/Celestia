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
#include <iostream>
#include <iomanip>
#include <celengine/parser.h>
#include <celengine/tokenizer.h>
#include <celutil/debug.h>
#include <celutil/util.h>
#include "favorites.h"

using namespace Eigen;
using namespace std;


FavoritesList* ReadFavoritesList(istream& in)
{
    auto* favorites = new FavoritesList();
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        if (tokenizer.getTokenType() != Tokenizer::TokenString)
        {
            DPRINTF(LOG_LEVEL_ERROR, "Error parsing favorites file.\n");
            for_each(favorites->begin(), favorites->end(), deleteFunc<FavoritesEntry*>());
            delete favorites;
            return nullptr;
        }

        FavoritesEntry* fav = new FavoritesEntry(); // FIXME: check
        fav->name = tokenizer.getStringValue();

        Value* favParamsValue = parser.readValue();
        if (favParamsValue == nullptr || favParamsValue->getType() != Value::HashType)
        {
            DPRINTF(LOG_LEVEL_ERROR, "Error parsing favorites entry %s\n", fav->name.c_str());
            for_each(favorites->begin(), favorites->end(), deleteFunc<FavoritesEntry*>());
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
        Vector3d base(Vector3d::Zero());
        Vector3d offset(Vector3d::Zero());
        favParams->getVector("base", base);
        favParams->getVector("offset", offset);
        fav->position = UniversalCoord::CreateLy(base) + UniversalCoord::CreateLy(offset * 1.0e-6);

        // Get orientation
        Vector3d axis = Vector3d::UnitX();
        double angle = 0.0;
        favParams->getVector("axis", axis);
        favParams->getNumber("angle", angle);
        fav->orientation = Quaternionf(AngleAxisf((float) angle, axis.cast<float>()));

        // Get time
        fav->jd = 0.0;
        favParams->getNumber("time", fav->jd);

        // Get the selected object
        favParams->getString("selection", fav->selectionName);

        string coordSysName;
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


void WriteFavoritesList(FavoritesList& favorites, ostream& out)
{
    for (const auto fav : favorites)
    {
        AngleAxisf aa(fav->orientation);
        Vector3f axis = aa.axis();
        float angle = aa.angle();

        // Ugly conversion from a universal coordinate to base+offset, with base
        // in light years and offset in microlight years. This is a bad way to
        // store position, but we need to maintain compatibility with older version
        // of Celestia (for now...)
        Vector3d baseUly((double) fav->position.x, (double) fav->position.y, (double) fav->position.z);
        Vector3d offset((double) (fav->position.x - (BigFix) baseUly.x()),
                        (double) (fav->position.y - (BigFix) baseUly.y()),
                        (double) (fav->position.z - (BigFix) baseUly.z()));
        Vector3d base = baseUly * 1e-6; // Base is in micro-light years

        out << '"' << fav->name << "\" {\n";
        if(fav->isFolder)
            out << "\tisFolder " << "true\n";
        else
        {
            out << "\tisFolder " << "false\n";
            out << "\tparentFolder \"" << fav->parentFolder << "\"\n";
            out << setprecision(16);
            out << "\tbase   [ " << base.x()   << ' ' << base.y()   << ' ' << base.z()   << " ]\n";
            out << "\toffset [ " << offset.x() << ' ' << offset.y() << ' ' << offset.z() << " ]\n";
            out << setprecision(6);
            out << "\taxis   [ " << axis.x() << ' ' << axis.y() << ' ' << axis.z() << " ]\n";
            out << "\tangle  " << angle << '\n';
            out << setprecision(16);
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
