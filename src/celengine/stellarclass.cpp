// stellarclass.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstring>
#include <cstdio>
#include <cassert>
#include "celestia.h"
#include "stellarclass.h"

using namespace std;


Color StellarClass::getApparentColor() const
{
    return getApparentColor(getSpectralClass());
}


Color StellarClass::getApparentColor(StellarClass::SpectralClass sc) const
{
    switch (sc)
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
    case StellarClass::Spectral_C:
        return Color(1.0f, 0.4f, 0.4f);
    case StellarClass::Spectral_L:
    case StellarClass::Spectral_T:
        return Color(0.75f, 0.2f, 0.2f);
    default:
        // TODO: Figure out reasonable colors for Wolf-Rayet stars,
        // white dwarfs, and other oddities
        return Color(1.0f, 1.0f, 1.0f);
    }
}


// The << method of converting the stellar class to a string is
// preferred, but it's not always practical, especially when you've
// got a completely broken implemtation of stringstreams to
// deal with (*cough* gcc *cough*).
//
// Return the buffer if successful or NULL if not (the buffer wasn't
// large enough.)
char* StellarClass::str(char* buf, unsigned int buflen) const
{
    StellarClass::StarType st = getStarType();
    char s0[3];
    char s1[2];
    const char* s2 = "";
    s0[0] = '\0';
    s1[0] = '\0';

    if (st == StellarClass::WhiteDwarf)
    {
        strcpy(s0, "WD");
    }
    else if (st == StellarClass::NeutronStar)
    {
        strcpy(s0, "Q");
    }
    else if (st == StellarClass::BlackHole)
    {
        strcpy(s0, "X");
    }
    else if (st == StellarClass::NormalStar)
    {
	s0[0] = "OBAFGKMRSNWW?LTC"[(unsigned int) getSpectralClass()];
        s0[1] = '\0';
	s1[0] = "0123456789"[getSpectralSubclass()];
        s1[1] = '\0';
	switch (getLuminosityClass())
        {
	case StellarClass::Lum_Ia0:
	    s2 = " I-a0";
	    break;
	case StellarClass::Lum_Ia:
	    s2 = " I-a";
	    break;
	case StellarClass::Lum_Ib:
	    s2 = " I-b";
	    break;
	case StellarClass::Lum_II:
	    s2 = " II";
	    break;
	case StellarClass::Lum_III:
	    s2 = " III";
	    break;
	case StellarClass::Lum_IV:
	    s2 = " IV";
	    break;
	case StellarClass::Lum_V:
	    s2 = " V";
	    break;
	case StellarClass::Lum_VI:
	    s2 = " VI";
	    break;
	}
    }
    else
    {
        strcpy(s0, "?");
    }

    if (strlen(s0) + strlen(s1) + strlen(s2) >= buflen)
    {
        return NULL;
    }
    else
    {
        sprintf(buf, "%s%s%s", s0, s1, s2);
        return buf;
    }
}


string StellarClass::str() const
{
    char buf[20];
    str(buf, sizeof buf);
    return string(buf);
}


ostream& operator<<(ostream& os, const StellarClass& sc)
{
    char buf[20];
    char *scString = sc.str(buf, sizeof buf);
    assert(scString != NULL);

    os << scString;

    return os;
}


bool operator<(const StellarClass& sc0, const StellarClass& sc1)
{
    return sc0.data < sc1.data;
}


StellarClass StellarClass::parse(const std::string& s)
{
    const char* starType = s.c_str();
    StellarClass::StarType type = StellarClass::NormalStar;
    StellarClass::SpectralClass specClass = StellarClass::Spectral_A;
    StellarClass::LuminosityClass lum = StellarClass::Lum_V;
    unsigned short number = 5;
    int i = 0;

    // Subdwarves (luminosity class VI) are prefixed with sd
    if (starType[i] == 's' && starType[i + 1] == 'd')
    {
        lum = StellarClass::Lum_VI;
        i += 2;
    }

    switch (starType[i])
    {
    case 'O':
        specClass = StellarClass::Spectral_O;
        break;
    case 'B':
        specClass = StellarClass::Spectral_B;
        break;
    case 'A':
        specClass = StellarClass::Spectral_A;
        break;
    case 'F':
        specClass = StellarClass::Spectral_F;
        break;
    case 'G':
        specClass = StellarClass::Spectral_G;
        break;
    case 'K':
        specClass = StellarClass::Spectral_K;
        break;
    case 'M':      
        specClass = StellarClass::Spectral_M;
        break;
    case 'R':
        specClass = StellarClass::Spectral_R;
        break;
    case 'N':
        specClass = StellarClass::Spectral_S;
        break;
    case 'S':
        specClass = StellarClass::Spectral_N;
        break;
    case 'C':
        specClass = StellarClass::Spectral_C;
        break;
    case 'W':
        i++;
        if (starType[i] == 'C')
             specClass = StellarClass::Spectral_WC;
        else if (starType[i] == 'N')
            specClass = StellarClass::Spectral_WN;
        else
            i--;
        break;
    case 'L':
        specClass = StellarClass::Spectral_L;
        break;
    case 'T':
        specClass = StellarClass::Spectral_T;
        break;
    case 'D':
        type = StellarClass::WhiteDwarf;
        return StellarClass(type, specClass, 0, lum);

    case 'Q':
        type = StellarClass::NeutronStar;
        return StellarClass(type, specClass, 0, lum);

    case 'X':
        type = StellarClass::BlackHole;
        return StellarClass(type, specClass, 0, lum);

    default:
        specClass = StellarClass::Spectral_Unknown;
        break;
    }

    i++;
    if (starType[i] >= '0' && starType[i] <= '9')
    {
        number = starType[i] - '0';
    }
    else
    {
        // No number given for spectral class; assume it's a 5 unless
        // the star is type O, as O5 stars are exceedingly rare.
        if (specClass == StellarClass::Spectral_O)
            number = 9;
        else
            number = 5;
    }

    if (lum != StellarClass::Lum_VI)
    {
        i++;
        lum = StellarClass::Lum_V;
        while (i < 13 && starType[i] != '\0') {
            if (starType[i] == 'I') {
                if (starType[i + 1] == 'I') {
                    if (starType[i + 2] == 'I') {
                        lum = StellarClass::Lum_III;
                    } else {
                        lum = StellarClass::Lum_II;
                    }
                } else if (starType[i + 1] == 'V') {
                    lum = StellarClass::Lum_IV;
                } else if (starType[i + 1] == 'a') {
                    if (starType[i + 2] == '0')
                        lum = StellarClass::Lum_Ia0;
                    else
                        lum = StellarClass::Lum_Ia;
                } else if (starType[i + 1] == 'b') {
                    lum = StellarClass::Lum_Ib;
                } else {
                    lum = StellarClass::Lum_Ib;
                }
                break;
            } else if (starType[i] == 'V') {
                if (starType[i + 1] == 'I')
                    lum = StellarClass::Lum_VI;
                else
                    lum = StellarClass::Lum_V;
                break;
            }
            i++;
        }
    }

    return StellarClass(type, specClass, number, lum);    
}
