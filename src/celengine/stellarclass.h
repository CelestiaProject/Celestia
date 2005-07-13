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
        Spectral_DA    = 16, // white dwarf A (Balmer lines, no He I or metals)
        Spectral_DB    = 17, // white dwarf B (He I lines, no H or metals)
        Spectral_DC    = 18, // white dwarf C, continuous spectrum
        Spectral_DO    = 19, // white dwarf O, He II strong, He I or H
        Spectral_DQ    = 20, // white dwarf Q, carbon features
        Spectral_DZ    = 21, // white dwarf Z, metal lines only, no H or He
        Spectral_D     = 22, // generic white dwarf, no additional data
        Spectral_DX    = 23,
        Spectral_Count = 24,
    };

    enum
    {
        FirstWDClass = 16,
        WDClassCount = 8,
        SubclassCount = 11,
        NormalClassCount = 16,
    };

    enum LuminosityClass
    {
	Lum_Ia0     = 0,
	Lum_Ia      = 1,
	Lum_Ib      = 2,
	Lum_II      = 3,
	Lum_III     = 4,
	Lum_IV      = 5,
	Lum_V       = 6,
	Lum_VI      = 7,
        Lum_Unknown = 8,
        Lum_Count   = 9,
    };

    enum
    {
        Subclass_Unknown = 10
    };

    inline StellarClass();
    inline StellarClass(StarType,
			SpectralClass,
			unsigned int,
			LuminosityClass);

    inline StarType getStarType() const;
    inline SpectralClass getSpectralClass() const;
    inline unsigned int getSubclass() const;
    inline LuminosityClass getLuminosityClass() const;

    Color getApparentColor() const;
    Color getApparentColor(StellarClass::SpectralClass sc) const;

    char* str(char* buf, unsigned int buflen) const;
    std::string str() const;

    static StellarClass parse(const std::string&);

    friend bool operator<(const StellarClass& sc0, const StellarClass& sc1);

    uint16 pack() const;
    bool unpack(uint16);

private:
    StarType starType;
    SpectralClass specClass;
    LuminosityClass lumClass;
    unsigned int subclass;
};


std::ostream& operator<<(std::ostream& s, const StellarClass& sc);

// A rough ordering of stellar classes, from 'early' to 'late' . . .
// Useful for organizing a list of stars by spectral class.
bool operator<(const StellarClass& sc0, const StellarClass& sc1);

StellarClass::StellarClass(StarType t,
			   SpectralClass sc,
			   unsigned int ssub,
			   LuminosityClass lum) :
    starType(t),
    specClass(sc),
    lumClass(lum),
    subclass(ssub)
{
}

StellarClass::StellarClass() :
    starType(NormalStar),
    specClass(Spectral_Unknown),
    lumClass(Lum_Unknown),
    subclass(Subclass_Unknown)
{
    
}

StellarClass::StarType StellarClass::getStarType() const
{
    return starType;
}

StellarClass::SpectralClass StellarClass::getSpectralClass() const
{
    return specClass;
}

unsigned int StellarClass::getSubclass() const
{
    return subclass;
}

StellarClass::LuminosityClass StellarClass::getLuminosityClass() const
{
    return lumClass;
}

#endif // _STELLARCLASS_H_
