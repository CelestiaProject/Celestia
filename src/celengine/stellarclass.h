// stellarclass.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _STELLARCLASS_H_
#define _STELLARCLASS_H_

#include <iostream>
#include <string>
#include <celutil/basictypes.h>
#include <celutil/color.h>


class StellarClass
{
public:
    enum StarType
    {
	NormalStar     = 0,
	WhiteDwarf     = 1,
	NeutronStar    = 2,
        BlackHole      = 3,
    };

    enum SpectralClass
    {
	Spectral_O     = 0,
	Spectral_B     = 1,
	Spectral_A     = 2,
	Spectral_F     = 3,
	Spectral_G     = 4,
	Spectral_K     = 5,
	Spectral_M     = 6,
	Spectral_R     = 7, // superceded by class C
	Spectral_S     = 8,
	Spectral_N     = 9, // superceded by class C
	Spectral_WC    = 10,
	Spectral_WN    = 11,
        Spectral_Unknown = 12,
        Spectral_L     = 13,
        Spectral_T     = 14,
        Spectral_C     = 15,
    };

    enum LuminosityClass
    {
	Lum_Ia0  = 0,
	Lum_Ia   = 1,
	Lum_Ib   = 2,
	Lum_II   = 3,
	Lum_III  = 4,
	Lum_IV   = 5,
	Lum_V    = 6,
	Lum_VI   = 7,
    };

    inline StellarClass();
    inline StellarClass(StarType,
			SpectralClass,
			unsigned int,
			LuminosityClass);

    inline StarType getStarType() const;
    inline SpectralClass getSpectralClass() const;
    inline unsigned int getSpectralSubclass() const;
    inline LuminosityClass getLuminosityClass() const;

    Color getApparentColor() const;
    Color getApparentColor(StellarClass::SpectralClass sc) const;

    char* str(char* buf, unsigned int buflen) const;
    std::string str() const;

    static StellarClass parse(const std::string&);

    friend bool operator<(const StellarClass& sc0, const StellarClass& sc1);

private:
    // It'd be nice to use bitfields, but gcc can't seem to pack
    // them into under 4 bytes.
    uint16 data;
};


std::ostream& operator<<(std::ostream& s, const StellarClass& sc);

// A rough ordering of stellar classes, from 'early' to 'late' . . .
// Useful for organizing a list of stars by spectral class.
bool operator<(const StellarClass& sc0, const StellarClass& sc1);

StellarClass::StellarClass(StarType t,
			   SpectralClass sc,
			   unsigned int ssub,
			   LuminosityClass lum)
{
    data = (((unsigned short) t << 12) |
	    ((unsigned short) sc << 8) |
	    ((unsigned short) ssub << 4) |
	    ((unsigned short) lum));
}

StellarClass::StellarClass()
{
    data = 0;
}

StellarClass::StarType StellarClass::getStarType() const
{
    return (StarType) (data >> 12);
}

StellarClass::SpectralClass StellarClass::getSpectralClass() const
{
    return (SpectralClass) (data >> 8 & 0xf);
}

unsigned int StellarClass::getSpectralSubclass() const
{
    return data >> 4 & 0xf;
}

StellarClass::LuminosityClass StellarClass::getLuminosityClass() const
{
    return (LuminosityClass) (data & 0xf);
}

#endif // _STELLARCLASS_H_
