// utf8.h
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELUTIL_UTF8_
#define _CELUTIL_UTF8_

#include <string>
#include <vector>

#define UTF8_DEGREE_SIGN         "\302\260"
#define UTF8_MULTIPLICATION_SIGN "\303\227"
#define UTF8_SUPERSCRIPT_0       "\342\201\260"
#define UTF8_SUPERSCRIPT_1       "\302\271"
#define UTF8_SUPERSCRIPT_2       "\302\262"
#define UTF8_SUPERSCRIPT_3       "\302\263"
#define UTF8_SUPERSCRIPT_4       "\342\201\264"
#define UTF8_SUPERSCRIPT_5       "\342\201\265"
#define UTF8_SUPERSCRIPT_6       "\342\201\266"
#define UTF8_SUPERSCRIPT_7       "\342\201\267"
#define UTF8_SUPERSCRIPT_8       "\342\201\270"
#define UTF8_SUPERSCRIPT_9       "\342\201\271"


bool UTF8Decode(const std::string& str, int pos, wchar_t& ch);
bool UTF8Decode(const char* str, int pos, int length, wchar_t& ch);
int UTF8Encode(wchar_t ch, char* s);
int UTF8StringCompare(const std::string& s0, const std::string& s1);
int UTF8StringCompare(const std::string& s0, const std::string& s1, size_t n, bool ignoreCase = false);

class UTF8StringOrderingPredicate
{
 public:
    bool operator()(const std::string& s0, const std::string& s1) const
    {
        return UTF8StringCompare(s0, s1) == -1;
    }
};


int UTF8Length(const std::string& s);

inline int UTF8EncodedSize(wchar_t ch)
{
    if (ch < 0x80)
        return 1;
    if (ch < 0x800)
        return 2;
#if WCHAR_MAX > 0xFFFFu
    if (ch < 0x10000)
#endif
        return 3;
#if WCHAR_MAX > 0xFFFFu
    if (ch < 0x200000)
        return 4;
    if (ch < 0x4000000)
        return 5;
    else
        return 6;
#endif
}

inline int UTF8EncodedSizeFromFirstByte(unsigned int ch)
{
    if (ch < 0x80)
        return 1;
    if ((ch & 0xe0) == 0xc0)
        return 2;
    if ((ch & 0xf0) == 0xe0)
        return 3;
    if ((ch & 0xf8) == 0xf0)
        return 4;
    if ((ch & 0xfc) == 0xf8)
        return 5;
    if ((ch & 0xfe) == 0xfc)
        return 6;
    else
        return 1;
}

std::string ReplaceGreekLetterAbbr(const std::string&);
#if 0
unsigned int ReplaceGreekLetterAbbr(char* dst, unsigned int dstSize, const char* src, unsigned int srcLength);
#endif

class Greek
{
 private:
    Greek();
    ~Greek();

 public:
    enum Letter
    {
        Alpha     =  1,
        Beta      =  2,
        Gamma     =  3,
        Delta     =  4,
        Epsilon   =  5,
        Zeta      =  6,
        Eta       =  7,
        Theta     =  8,
        Iota      =  9,
        Kappa     = 10,
        Lambda    = 11,
        Mu        = 12,
        Nu        = 13,
        Xi        = 14,
        Omicron   = 15,
        Pi        = 16,
        Rho       = 17,
        Sigma     = 18,
        Tau       = 19,
        Upsilon   = 20,
        Phi       = 21,
        Chi       = 22,
        Psi       = 23,
        Omega     = 24,
    };

    static const std::string& canonicalAbbreviation(const std::string&);
 private:
    static Greek* m_instance;
 public:
    static Greek* getInstance();
    int nLetters;
    std::string* names;
    std::string* abbrevs;
};

std::vector<std::string> getGreekCompletion(const std::string &);

#endif // _CELUTIL_UTF8_
