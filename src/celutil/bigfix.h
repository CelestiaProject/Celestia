// bigfix.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// 128-bit fixed point (64.64) numbers for high-precision celestial
// coordinates.  When you need millimeter accurate navigation across a scale
// of thousands of light years, doubles just don't do it.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.


#ifndef _BIGFIX_H_
#define _BIGFIX_H_

#define N_WORDS 8
#include <string>

class BigFix
{
public:
	BigFix();
	BigFix(int);
	BigFix(double);
	BigFix(const std::string&);

	operator double() const;
	operator float() const;

	BigFix operator-() const;

	friend BigFix operator+(BigFix, BigFix);
	friend BigFix operator-(BigFix, BigFix);
	friend BigFix operator*(BigFix, BigFix);
	friend bool operator==(BigFix, BigFix);
	friend bool operator!=(BigFix, BigFix);

	int sign();

	// for debugging
	void dump();
	std::string toString();

private:
	unsigned short n[N_WORDS];  // high 16-bits is n[N_WORDS - 1]
};

#endif // _BIGFIX_H_
