// stellarclass.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celestia.h"
#include "stellarclass.h"

using namespace std;


Color StellarClass::getApparentColor() const
{
    switch (getSpectralClass())
    {
    case Spectral_O:
        return Color(0.7f, 0.8f, 1.0f);
    case Spectral_B:
        return Color(0.8f, 0.9f, 1.0f);
    case Spectral_A:
        return Color(1.0f, 1.0f, 1.0f);
    case Spectral_F:
        return Color(1.0f, 1.0f, 0.88f);
    case Spectral_G:
        return Color(1.0f, 1.0f, 0.75f);
    case StellarClass::Spectral_K:
        return Color(1.0f, 0.9f, 0.7f);
    case StellarClass::Spectral_M:
        return Color(1.0f, 0.7f, 0.7f);
    case StellarClass::Spectral_R:
    case StellarClass::Spectral_S:
    case StellarClass::Spectral_N:
        return Color(1.0f, 0.6f, 0.6f);
    default:
        // TODO: Figure out reasonable colors for Wolf-Rayet stars,
        // white dwarfs, and other oddities
        return Color(1.0f, 1.0f, 1.0f);
    }
}


ostream& operator<<(ostream& s, const StellarClass& sc)
{
    StellarClass::StarType st = sc.getStarType();

    if (st == StellarClass::WhiteDwarf)
    {
	s << 'D';
    }
    else if (st == StellarClass::NeutronStar)
    {
	s << 'Q';
    }
    else if (st == StellarClass::NormalStar)
    {
	s << "OBAFGKMRSNWW"[(unsigned int) sc.getSpectralClass()];
	s << "0123456789"[sc.getSpectralSubclass()];
        s << ' ';
	switch (sc.getLuminosityClass())
        {
	case StellarClass::Lum_Ia0:
	    s << "I-a0";
	    break;
	case StellarClass::Lum_Ia:
	    s << "I-a";
	    break;
	case StellarClass::Lum_Ib:
	    s << "I-b";
	    break;
	case StellarClass::Lum_II:
	    s << "II";
	    break;
	case StellarClass::Lum_III:
	    s << "III";
	    break;
	case StellarClass::Lum_IV:
	    s << "IV";
	    break;
	case StellarClass::Lum_V:
	    s << "V";
	    break;
	case StellarClass::Lum_VI:
	    s << "VI";
	    break;
	}
    }

    return s;
}
