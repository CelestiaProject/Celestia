// color.cpp
//
// Copyright (C) 2001-2008, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "color.h"

using namespace std;

Color::ColorMap Color::x11Colors;

const Color Color::White = Color(1.0f, 1.0f, 1.0f);
const Color Color::Black = Color(0.0f,0.0f, 0.0f);

/*! Parse a color string and return true if it was a valid color, otherwise
 *  false.  Accetable inputs are HTML/X11 style #xxxxxx colors (where x is
 *  hexadecimal digit) or one of a list of named colors.
 */
bool Color::parse(const char* s, Color& c)
{
    // Initialize the color dictionary if this is the first
    // time Color::parse has been called.
    if (x11Colors.empty())
    {
        buildX11ColorMap();
    }

    if (s[0] == '#')
    {
        s++;

        int length = strlen(s);

        // Verify that the string contains only hex digits
        for (int i = 0; i < length; i++)
        {
            if (isxdigit(s[i]) == 0)
                return false;
        }

        unsigned int n;
        sscanf(s, "%x", &n);
        switch(length)
        {
        case 3:
            c = Color((uint8_t) ((n >> 8) * 17),
                      (uint8_t) (((n & 0x0f0) >> 4) * 17),
                      (uint8_t) ((n & 0x00f) * 17));
            return true;
        case 6:
            c = Color((uint8_t) (n >> 16),
                      (uint8_t) ((n & 0x00ff00) >> 8),
                      (uint8_t) (n & 0x0000ff));
            return true;
        default:
            return false;
        }
    }
    else
    {
        if (x11Colors.find(s) != x11Colors.end())
        {
            c = x11Colors.find(s)->second;
            return true;
        }
        return false;
    }
}


// X11 Colors STL map
void Color::buildX11ColorMap()
{
    x11Colors["aliceblue"]            = Color((uint8_t)240, (uint8_t)248, (uint8_t)255);
    x11Colors["antiquewhite"]         = Color((uint8_t)250, (uint8_t)235, (uint8_t)215);
    x11Colors["aqua"]                 = Color((uint8_t)0,   (uint8_t)255, (uint8_t)255);
    x11Colors["aquamarine"]           = Color((uint8_t)127, (uint8_t)255, (uint8_t)212);
    x11Colors["azure"]                = Color((uint8_t)240, (uint8_t)255, (uint8_t)255);
    x11Colors["beige"]                = Color((uint8_t)245, (uint8_t)245, (uint8_t)220);
    x11Colors["bisque"]               = Color((uint8_t)255, (uint8_t)228, (uint8_t)196);
    x11Colors["black"]                = Color((uint8_t)0,   (uint8_t)0,   (uint8_t)0);
    x11Colors["blanchedalmond"]       = Color((uint8_t)255, (uint8_t)235, (uint8_t)205);
    x11Colors["blue"]                 = Color((uint8_t)0,   (uint8_t)0,   (uint8_t)255);
    x11Colors["blueviolet"]           = Color((uint8_t)138, (uint8_t)43,  (uint8_t)226);
    x11Colors["brown"]                = Color((uint8_t)165, (uint8_t)42,  (uint8_t)42);
    x11Colors["burlywood"]            = Color((uint8_t)222, (uint8_t)184, (uint8_t)135);
    x11Colors["cadetblue"]            = Color((uint8_t)95,  (uint8_t)158, (uint8_t)160);
    x11Colors["chartreuse"]           = Color((uint8_t)127, (uint8_t)255, (uint8_t)0);
    x11Colors["chocolate"]            = Color((uint8_t)210, (uint8_t)105, (uint8_t)30);
    x11Colors["coral"]                = Color((uint8_t)255, (uint8_t)127, (uint8_t)80);
    x11Colors["cornflowerblue"]       = Color((uint8_t)100, (uint8_t)149, (uint8_t)237);
    x11Colors["cornsilk"]             = Color((uint8_t)255, (uint8_t)248, (uint8_t)220);
    x11Colors["crimson"]              = Color((uint8_t)220, (uint8_t)20,  (uint8_t)60);
    x11Colors["cyan"]                 = Color((uint8_t)0,   (uint8_t)255, (uint8_t)255);
    x11Colors["darkblue"]             = Color((uint8_t)0,   (uint8_t)0,   (uint8_t)139);
    x11Colors["darkcyan"]             = Color((uint8_t)0,   (uint8_t)139, (uint8_t)139);
    x11Colors["darkgoldenrod"]        = Color((uint8_t)184, (uint8_t)134, (uint8_t)11);
    x11Colors["darkgray"]             = Color((uint8_t)169, (uint8_t)169, (uint8_t)169);
    x11Colors["darkgreen"]            = Color((uint8_t)0,   (uint8_t)100, (uint8_t)0);
    x11Colors["darkkhaki"]            = Color((uint8_t)189, (uint8_t)183, (uint8_t)107);
    x11Colors["darkmagenta"]          = Color((uint8_t)139, (uint8_t)0,   (uint8_t)139);
    x11Colors["darkolivegreen"]       = Color((uint8_t)85,  (uint8_t)107, (uint8_t)47);
    x11Colors["darkorange"]           = Color((uint8_t)255, (uint8_t)140, (uint8_t)0);
    x11Colors["darkorchid"]           = Color((uint8_t)153, (uint8_t)50,  (uint8_t)204);
    x11Colors["darkred"]              = Color((uint8_t)139, (uint8_t)0,   (uint8_t)0);
    x11Colors["darksalmon"]           = Color((uint8_t)233, (uint8_t)150, (uint8_t)122);
    x11Colors["darkseagreen"]         = Color((uint8_t)143, (uint8_t)188, (uint8_t)143);
    x11Colors["darkslateblue"]        = Color((uint8_t)72,  (uint8_t)61,  (uint8_t)139);
    x11Colors["darkslategray"]        = Color((uint8_t)47,  (uint8_t)79,  (uint8_t)79);
    x11Colors["darkturquoise"]        = Color((uint8_t)0,   (uint8_t)206, (uint8_t)209);
    x11Colors["darkviolet"]           = Color((uint8_t)148, (uint8_t)0,   (uint8_t)211);
    x11Colors["deeppink"]             = Color((uint8_t)255, (uint8_t)20,  (uint8_t)147);
    x11Colors["deepskyblue"]          = Color((uint8_t)0,   (uint8_t)191, (uint8_t)255);
    x11Colors["dimgray"]              = Color((uint8_t)105, (uint8_t)105, (uint8_t)105);
    x11Colors["dodgerblue"]           = Color((uint8_t)30,  (uint8_t)144, (uint8_t)255);
    x11Colors["firebrick"]            = Color((uint8_t)178, (uint8_t)34,  (uint8_t)34);
    x11Colors["floralwhite"]          = Color((uint8_t)255, (uint8_t)250, (uint8_t)240);
    x11Colors["forestgreen"]          = Color((uint8_t)34,  (uint8_t)139, (uint8_t)34);
    x11Colors["fuchsia"]              = Color((uint8_t)255, (uint8_t)0,   (uint8_t)255);
    x11Colors["gainsboro"]            = Color((uint8_t)220, (uint8_t)220, (uint8_t)220);
    x11Colors["ghostwhite"]           = Color((uint8_t)248, (uint8_t)248, (uint8_t)255);
    x11Colors["gold"]                 = Color((uint8_t)255, (uint8_t)215, (uint8_t)0);
    x11Colors["goldenrod"]            = Color((uint8_t)218, (uint8_t)165, (uint8_t)32);
    x11Colors["gray"]                 = Color((uint8_t)128, (uint8_t)128, (uint8_t)128);
    x11Colors["green"]                = Color((uint8_t)0,   (uint8_t)128, (uint8_t)0);
    x11Colors["greenyellow"]          = Color((uint8_t)173, (uint8_t)255, (uint8_t)47);
    x11Colors["honeydew"]             = Color((uint8_t)240, (uint8_t)255, (uint8_t)240);
    x11Colors["hotpink"]              = Color((uint8_t)255, (uint8_t)105, (uint8_t)180);
    x11Colors["indianred"]            = Color((uint8_t)205, (uint8_t)92,  (uint8_t)92);
    x11Colors["indigo"]               = Color((uint8_t)75,  (uint8_t)0,   (uint8_t)130);
    x11Colors["ivory"]                = Color((uint8_t)255, (uint8_t)255, (uint8_t)240);
    x11Colors["khaki"]                = Color((uint8_t)240, (uint8_t)230, (uint8_t)140);
    x11Colors["lavender"]             = Color((uint8_t)230, (uint8_t)230, (uint8_t)250);
    x11Colors["lavenderblush"]        = Color((uint8_t)255, (uint8_t)240, (uint8_t)245 );
    x11Colors["lawngreen"]            = Color((uint8_t)124, (uint8_t)252, (uint8_t)0);
    x11Colors["lemonchiffon"]         = Color((uint8_t)255, (uint8_t)250, (uint8_t)205);
    x11Colors["lightblue"]            = Color((uint8_t)173, (uint8_t)216, (uint8_t)230);
    x11Colors["lightcoral"]           = Color((uint8_t)240, (uint8_t)128, (uint8_t)128);
    x11Colors["lightcyan"]            = Color((uint8_t)224, (uint8_t)255, (uint8_t)255);
    x11Colors["lightgoldenrodyellow"] = Color((uint8_t)250, (uint8_t)250, (uint8_t)210);
    x11Colors["lightgreen"]           = Color((uint8_t)144, (uint8_t)238, (uint8_t)144);
    x11Colors["lightgrey"]            = Color((uint8_t)211, (uint8_t)211, (uint8_t)211);
    x11Colors["lightpink"]            = Color((uint8_t)255, (uint8_t)182, (uint8_t)193);
    x11Colors["lightsalmon"]          = Color((uint8_t)255, (uint8_t)160, (uint8_t)122);
    x11Colors["lightseagreen"]        = Color((uint8_t)32,  (uint8_t)178, (uint8_t)170);
    x11Colors["lightskyblue"]         = Color((uint8_t)135, (uint8_t)206, (uint8_t)250);
    x11Colors["lightslategray"]       = Color((uint8_t)119, (uint8_t)136, (uint8_t)153);
    x11Colors["lightsteelblue"]       = Color((uint8_t)176, (uint8_t)196, (uint8_t)222);
    x11Colors["lightyellow"]          = Color((uint8_t)255, (uint8_t)255, (uint8_t)224);
    x11Colors["lime"]                 = Color((uint8_t)0,   (uint8_t)255, (uint8_t)0);
    x11Colors["limegreen"]            = Color((uint8_t)50,  (uint8_t)205, (uint8_t)50);
    x11Colors["linen"]                = Color((uint8_t)250, (uint8_t)240, (uint8_t)230);
    x11Colors["magenta"]              = Color((uint8_t)255, (uint8_t)0,   (uint8_t)255);
    x11Colors["maroon"]               = Color((uint8_t)128, (uint8_t)0,   (uint8_t)0);
    x11Colors["mediumaquamarine"]     = Color((uint8_t)102, (uint8_t)205, (uint8_t)170);
    x11Colors["mediumblue"]           = Color((uint8_t)0,   (uint8_t)0,   (uint8_t)205);
    x11Colors["mediumorchid"]         = Color((uint8_t)186, (uint8_t)85,  (uint8_t)211);
    x11Colors["mediumpurple"]         = Color((uint8_t)147, (uint8_t)112, (uint8_t)219);
    x11Colors["mediumseagreen"]       = Color((uint8_t)60,  (uint8_t)179, (uint8_t)113);
    x11Colors["mediumslateblue"]      = Color((uint8_t)123, (uint8_t)104, (uint8_t)238);
    x11Colors["mediumspringgreen"]    = Color((uint8_t)0,   (uint8_t)250, (uint8_t)154);
    x11Colors["mediumturquoise"]      = Color((uint8_t)72,  (uint8_t)209, (uint8_t)204);
    x11Colors["mediumvioletred"]      = Color((uint8_t)199, (uint8_t)21,  (uint8_t)133);
    x11Colors["midnightblue"]         = Color((uint8_t)25,  (uint8_t)25,  (uint8_t)112);
    x11Colors["mintcream"]            = Color((uint8_t)245, (uint8_t)255, (uint8_t)250);
    x11Colors["mistyrose"]            = Color((uint8_t)255, (uint8_t)228, (uint8_t)225);
    x11Colors["moccasin"]             = Color((uint8_t)255, (uint8_t)228, (uint8_t)181);
    x11Colors["navajowhite"]          = Color((uint8_t)255, (uint8_t)222, (uint8_t)173);
    x11Colors["navy"]                 = Color((uint8_t)0,   (uint8_t)0,   (uint8_t)128);
    x11Colors["oldlace"]              = Color((uint8_t)253, (uint8_t)245, (uint8_t)230);
    x11Colors["olive"]                = Color((uint8_t)128, (uint8_t)128, (uint8_t)0);
    x11Colors["olivedrab"]            = Color((uint8_t)107, (uint8_t)142, (uint8_t)35);
    x11Colors["orange"]               = Color((uint8_t)255, (uint8_t)165, (uint8_t)0);
    x11Colors["orangered"]            = Color((uint8_t)255, (uint8_t)69,  (uint8_t)0);
    x11Colors["orchid"]               = Color((uint8_t)218, (uint8_t)112, (uint8_t)214);
    x11Colors["palegoldenrod"]        = Color((uint8_t)238, (uint8_t)232, (uint8_t)170);
    x11Colors["palegreen"]            = Color((uint8_t)152, (uint8_t)251, (uint8_t)152);
    x11Colors["paleturquoise"]        = Color((uint8_t)175, (uint8_t)238, (uint8_t)238);
    x11Colors["palevioletred"]        = Color((uint8_t)219, (uint8_t)112, (uint8_t)147);
    x11Colors["papayawhip"]           = Color((uint8_t)255, (uint8_t)239, (uint8_t)213);
    x11Colors["peachpuff"]            = Color((uint8_t)255, (uint8_t)218, (uint8_t)185);
    x11Colors["peru"]                 = Color((uint8_t)205, (uint8_t)133, (uint8_t)63);
    x11Colors["pink"]                 = Color((uint8_t)255, (uint8_t)192, (uint8_t)203);
    x11Colors["plum"]                 = Color((uint8_t)221, (uint8_t)160, (uint8_t)221);
    x11Colors["powderblue"]           = Color((uint8_t)176, (uint8_t)224, (uint8_t)230);
    x11Colors["purple"]               = Color((uint8_t)128, (uint8_t)0,   (uint8_t)128);
    x11Colors["red"]                  = Color((uint8_t)255, (uint8_t)0,   (uint8_t)0);
    x11Colors["rosybrown"]            = Color((uint8_t)188, (uint8_t)143, (uint8_t)143);
    x11Colors["royalblue"]            = Color((uint8_t)65,  (uint8_t)105, (uint8_t)225);
    x11Colors["saddlebrown"]          = Color((uint8_t)139, (uint8_t)69,  (uint8_t)19);
    x11Colors["salmon"]               = Color((uint8_t)250, (uint8_t)128, (uint8_t)114);
    x11Colors["sandybrown"]           = Color((uint8_t)244, (uint8_t)164, (uint8_t)96);
    x11Colors["seagreen"]             = Color((uint8_t)46,  (uint8_t)139, (uint8_t)87);
    x11Colors["seashell"]             = Color((uint8_t)255, (uint8_t)245, (uint8_t)238);
    x11Colors["sienna"]               = Color((uint8_t)160, (uint8_t)82,  (uint8_t)45);
    x11Colors["silver"]               = Color((uint8_t)192, (uint8_t)192, (uint8_t)192);
    x11Colors["skyblue"]              = Color((uint8_t)135, (uint8_t)206, (uint8_t)235);
    x11Colors["slateblue"]            = Color((uint8_t)106, (uint8_t)90,  (uint8_t)205);
    x11Colors["slategray"]            = Color((uint8_t)112, (uint8_t)128, (uint8_t)144);
    x11Colors["snow"]                 = Color((uint8_t)255, (uint8_t)250, (uint8_t)250);
    x11Colors["springgreen"]          = Color((uint8_t)0,   (uint8_t)255, (uint8_t)127);
    x11Colors["steelblue"]            = Color((uint8_t)70,  (uint8_t)130, (uint8_t)180);
    x11Colors["tan"]                  = Color((uint8_t)210, (uint8_t)180, (uint8_t)140);
    x11Colors["teal"]                 = Color((uint8_t)0,   (uint8_t)128, (uint8_t)128);
    x11Colors["thistle"]              = Color((uint8_t)216, (uint8_t)191, (uint8_t)216);
    x11Colors["tomato"]               = Color((uint8_t)255, (uint8_t)99,  (uint8_t)71);
    x11Colors["turquoise"]            = Color((uint8_t)64,  (uint8_t)224, (uint8_t)208);
    x11Colors["violet"]               = Color((uint8_t)238, (uint8_t)130, (uint8_t)238);
    x11Colors["wheat"]                = Color((uint8_t)245, (uint8_t)222, (uint8_t)179);
    x11Colors["white"]                = Color((uint8_t)255, (uint8_t)255, (uint8_t)255);
    x11Colors["whitesmoke"]           = Color((uint8_t)245, (uint8_t)245, (uint8_t)245);
    x11Colors["yellow"]               = Color((uint8_t)255, (uint8_t)255, (uint8_t)0);
    x11Colors["yellowgreen"]          = Color((uint8_t)154, (uint8_t)205, (uint8_t)50);
}
