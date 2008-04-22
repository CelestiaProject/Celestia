// color.cpp
//
// Copyright (C) 2001-2008, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdio>
#include <cstring>
#include <cctype>
#include "color.h"

using namespace std;


Color::ColorMap Color::x11Colors;

const Color Color::White = Color(1.0f, 1.0f, 1.0f);
const Color Color::Black = Color(0.0f,0.0f, 0.0f);

template<class T> T clamp(T x)
{
    if (x < 0)
        return 0;
    else if (x > 1)
        return 1;
    else
        return x;
}

Color::Color()
{
    c[Red] = c[Green] = c[Blue] = 0;
    c[Alpha] = 0xff;
}


Color::Color(float r, float g, float b)
{
    c[Red] = (unsigned char) (clamp(r) * 255.99f);
    c[Green] = (unsigned char) (clamp(g) * 255.99f);
    c[Blue] = (unsigned char) (clamp(b) * 255.99f);
    c[Alpha] = 0xff;
}


Color::Color(float r, float g, float b, float a)
{
    c[Red]   = (unsigned char) (clamp(r) * 255.99f);
    c[Green] = (unsigned char) (clamp(g) * 255.99f);
    c[Blue]  = (unsigned char) (clamp(b) * 255.99f);
    c[Alpha] = (unsigned char) (clamp(a) * 255.99f);
}


Color::Color(unsigned char r, unsigned char g, unsigned char b)
{
    c[Red] = r;
    c[Green] = g;
    c[Blue] = b;
    c[Alpha] = 0xff;
}


Color::Color(const Color& color, float alpha)
{
    *this = color;
    c[Alpha] = (unsigned char) (clamp(alpha) * 255.99f);
}


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
            if (!isxdigit(s[i]))
                return false;
        }
        
        unsigned int n;
        sscanf(s, "%x", &n);
        if (length == 3)
        {
            c = Color((unsigned char) ((n >> 8) * 17),
                      (unsigned char) (((n & 0x0f0) >> 4) * 17),
                      (unsigned char) ((n & 0x00f) * 17));
            return true;
        }
        else if (length == 6)
        {
            c = Color((unsigned char) (n >> 16),
                      (unsigned char) ((n & 0x00ff00) >> 8),
                      (unsigned char) (n & 0x0000ff));
            return true;
        }
        else
        {
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
       else
       {
           return false;
       }
    }
}


// X11 Colors STL map
void Color::buildX11ColorMap()
{
    x11Colors["aliceblue"]            = Color((unsigned char)240, (unsigned char)248, (unsigned char)255);
    x11Colors["antiquewhite"]         = Color((unsigned char)250, (unsigned char)235, (unsigned char)215);
    x11Colors["aqua"]                 = Color((unsigned char)0,   (unsigned char)255, (unsigned char)255);  
    x11Colors["aquamarine"]           = Color((unsigned char)127, (unsigned char)255, (unsigned char)212);  
    x11Colors["azure"]                = Color((unsigned char)240, (unsigned char)255, (unsigned char)255);
    x11Colors["beige"]                = Color((unsigned char)245, (unsigned char)245, (unsigned char)220);
    x11Colors["bisque"]               = Color((unsigned char)255, (unsigned char)228, (unsigned char)196);
    x11Colors["black"]                = Color((unsigned char)0,   (unsigned char)0,   (unsigned char)0);
    x11Colors["blanchedalmond"]       = Color((unsigned char)255, (unsigned char)235, (unsigned char)205);  
    x11Colors["blue"]                 = Color((unsigned char)0,   (unsigned char)0,   (unsigned char)255);
    x11Colors["blueviolet"]           = Color((unsigned char)138, (unsigned char)43,  (unsigned char)226);
    x11Colors["brown"]                = Color((unsigned char)165, (unsigned char)42,  (unsigned char)42);
    x11Colors["burlywood"]            = Color((unsigned char)222, (unsigned char)184, (unsigned char)135);
    x11Colors["cadetblue"]            = Color((unsigned char)95,  (unsigned char)158, (unsigned char)160);
    x11Colors["chartreuse"]           = Color((unsigned char)127, (unsigned char)255, (unsigned char)0);
    x11Colors["chocolate"]            = Color((unsigned char)210, (unsigned char)105, (unsigned char)30);
    x11Colors["coral"]                = Color((unsigned char)255, (unsigned char)127, (unsigned char)80);
    x11Colors["cornflowerblue"]       = Color((unsigned char)100, (unsigned char)149, (unsigned char)237);  
    x11Colors["cornsilk"]             = Color((unsigned char)255, (unsigned char)248, (unsigned char)220);
    x11Colors["crimson"]              = Color((unsigned char)220, (unsigned char)20,  (unsigned char)60);
    x11Colors["cyan"]                 = Color((unsigned char)0,   (unsigned char)255, (unsigned char)255);
    x11Colors["darkblue"]             = Color((unsigned char)0,   (unsigned char)0,   (unsigned char)139);
    x11Colors["darkcyan"]             = Color((unsigned char)0,   (unsigned char)139, (unsigned char)139);
    x11Colors["darkgoldenrod"]        = Color((unsigned char)184, (unsigned char)134, (unsigned char)11);
    x11Colors["darkgray"]             = Color((unsigned char)169, (unsigned char)169, (unsigned char)169);
    x11Colors["darkgreen"]            = Color((unsigned char)0,   (unsigned char)100, (unsigned char)0);
    x11Colors["darkkhaki"]            = Color((unsigned char)189, (unsigned char)183, (unsigned char)107);
    x11Colors["darkmagenta"]          = Color((unsigned char)139, (unsigned char)0,   (unsigned char)139);
    x11Colors["darkolivegreen"]       = Color((unsigned char)85,  (unsigned char)107, (unsigned char)47);
    x11Colors["darkorange"]           = Color((unsigned char)255, (unsigned char)140, (unsigned char)0);
    x11Colors["darkorchid"]           = Color((unsigned char)153, (unsigned char)50,  (unsigned char)204);
    x11Colors["darkred"]              = Color((unsigned char)139, (unsigned char)0,   (unsigned char)0);
    x11Colors["darksalmon"]           = Color((unsigned char)233, (unsigned char)150, (unsigned char)122);
    x11Colors["darkseagreen"]         = Color((unsigned char)143, (unsigned char)188, (unsigned char)143);
    x11Colors["darkslateblue"]        = Color((unsigned char)72,  (unsigned char)61,  (unsigned char)139);
    x11Colors["darkslategray"]        = Color((unsigned char)47,  (unsigned char)79,  (unsigned char)79); 
    x11Colors["darkturquoise"]        = Color((unsigned char)0,   (unsigned char)206, (unsigned char)209);  
    x11Colors["darkviolet"]           = Color((unsigned char)148, (unsigned char)0,   (unsigned char)211);
    x11Colors["deeppink"]             = Color((unsigned char)255, (unsigned char)20,  (unsigned char)147);
    x11Colors["deepskyblue"]          = Color((unsigned char)0,   (unsigned char)191, (unsigned char)255);
    x11Colors["dimgray"]              = Color((unsigned char)105, (unsigned char)105, (unsigned char)105);
    x11Colors["dodgerblue"]           = Color((unsigned char)30,  (unsigned char)144, (unsigned char)255);
    x11Colors["firebrick"]            = Color((unsigned char)178, (unsigned char)34,  (unsigned char)34);
    x11Colors["floralwhite"]          = Color((unsigned char)255, (unsigned char)250, (unsigned char)240);  
    x11Colors["forestgreen"]          = Color((unsigned char)34,  (unsigned char)139, (unsigned char)34);
    x11Colors["fuchsia"]              = Color((unsigned char)255, (unsigned char)0,   (unsigned char)255);
    x11Colors["gainsboro"]            = Color((unsigned char)220, (unsigned char)220, (unsigned char)220);
    x11Colors["ghostwhite"]           = Color((unsigned char)248, (unsigned char)248, (unsigned char)255); 
    x11Colors["gold"]                 = Color((unsigned char)255, (unsigned char)215, (unsigned char)0);
    x11Colors["goldenrod"]            = Color((unsigned char)218, (unsigned char)165, (unsigned char)32);
    x11Colors["gray"]                 = Color((unsigned char)128, (unsigned char)128, (unsigned char)128);
    x11Colors["green"]                = Color((unsigned char)0,   (unsigned char)128, (unsigned char)0);
    x11Colors["greenyellow"]          = Color((unsigned char)173, (unsigned char)255, (unsigned char)47); 
    x11Colors["honeydew"]             = Color((unsigned char)240, (unsigned char)255, (unsigned char)240);
    x11Colors["hotpink"]              = Color((unsigned char)255, (unsigned char)105, (unsigned char)180);
    x11Colors["indianred"]            = Color((unsigned char)205, (unsigned char)92,  (unsigned char)92);
    x11Colors["indigo"]               = Color((unsigned char)75,  (unsigned char)0,   (unsigned char)130);
    x11Colors["ivory"]                = Color((unsigned char)255, (unsigned char)255, (unsigned char)240);
    x11Colors["khaki"]                = Color((unsigned char)240, (unsigned char)230, (unsigned char)140);
    x11Colors["lavender"]             = Color((unsigned char)230, (unsigned char)230, (unsigned char)250);
    x11Colors["lavenderblush"]        = Color((unsigned char)255, (unsigned char)240, (unsigned char)245 ); 
    x11Colors["lawngreen"]            = Color((unsigned char)124, (unsigned char)252, (unsigned char)0);
    x11Colors["lemonchiffon"]         = Color((unsigned char)255, (unsigned char)250, (unsigned char)205);  
    x11Colors["lightblue"]            = Color((unsigned char)173, (unsigned char)216, (unsigned char)230);
    x11Colors["lightcoral"]           = Color((unsigned char)240, (unsigned char)128, (unsigned char)128);
    x11Colors["lightcyan"]            = Color((unsigned char)224, (unsigned char)255, (unsigned char)255);
    x11Colors["lightgoldenrodyellow"] = Color((unsigned char)250, (unsigned char)250, (unsigned char)210);  
    x11Colors["lightgreen"]           = Color((unsigned char)144, (unsigned char)238, (unsigned char)144);
    x11Colors["lightgrey"]            = Color((unsigned char)211, (unsigned char)211, (unsigned char)211);
    x11Colors["lightpink"]            = Color((unsigned char)255, (unsigned char)182, (unsigned char)193);
    x11Colors["lightsalmon"]          = Color((unsigned char)255, (unsigned char)160, (unsigned char)122);
    x11Colors["lightseagreen"]        = Color((unsigned char)32,  (unsigned char)178, (unsigned char)170);
    x11Colors["lightskyblue"]         = Color((unsigned char)135, (unsigned char)206, (unsigned char)250);
    x11Colors["lightslategray"]       = Color((unsigned char)119, (unsigned char)136, (unsigned char)153);
    x11Colors["lightsteelblue"]       = Color((unsigned char)176, (unsigned char)196, (unsigned char)222);
    x11Colors["lightyellow"]          = Color((unsigned char)255, (unsigned char)255, (unsigned char)224);
    x11Colors["lime"]                 = Color((unsigned char)0,   (unsigned char)255, (unsigned char)0);
    x11Colors["limegreen"]            = Color((unsigned char)50,  (unsigned char)205, (unsigned char)50);
    x11Colors["linen"]                = Color((unsigned char)250, (unsigned char)240, (unsigned char)230);
    x11Colors["magenta"]              = Color((unsigned char)255, (unsigned char)0,   (unsigned char)255);
    x11Colors["maroon"]               = Color((unsigned char)128, (unsigned char)0,   (unsigned char)0);
    x11Colors["mediumaquamarine"]     = Color((unsigned char)102, (unsigned char)205, (unsigned char)170);
    x11Colors["mediumblue"]           = Color((unsigned char)0,   (unsigned char)0,   (unsigned char)205);
    x11Colors["mediumorchid"]         = Color((unsigned char)186, (unsigned char)85,  (unsigned char)211);
    x11Colors["mediumpurple"]         = Color((unsigned char)147, (unsigned char)112, (unsigned char)219);
    x11Colors["mediumseagreen"]       = Color((unsigned char)60,  (unsigned char)179, (unsigned char)113);
    x11Colors["mediumslateblue"]      = Color((unsigned char)123, (unsigned char)104, (unsigned char)238);
    x11Colors["mediumspringgreen"]    = Color((unsigned char)0,   (unsigned char)250, (unsigned char)154);
    x11Colors["mediumturquoise"]      = Color((unsigned char)72,  (unsigned char)209, (unsigned char)204);
    x11Colors["mediumvioletred"]      = Color((unsigned char)199, (unsigned char)21,  (unsigned char)133);
    x11Colors["midnightblue"]         = Color((unsigned char)25,  (unsigned char)25,  (unsigned char)112);
    x11Colors["mintcream"]            = Color((unsigned char)245, (unsigned char)255, (unsigned char)250);
    x11Colors["mistyrose"]            = Color((unsigned char)255, (unsigned char)228, (unsigned char)225);
    x11Colors["moccasin"]             = Color((unsigned char)255, (unsigned char)228, (unsigned char)181);
    x11Colors["navajowhite"]          = Color((unsigned char)255, (unsigned char)222, (unsigned char)173);
    x11Colors["navy"]                 = Color((unsigned char)0,   (unsigned char)0,   (unsigned char)128);
    x11Colors["oldlace"]              = Color((unsigned char)253, (unsigned char)245, (unsigned char)230);  
    x11Colors["olive"]                = Color((unsigned char)128, (unsigned char)128, (unsigned char)0);
    x11Colors["olivedrab"]            = Color((unsigned char)107, (unsigned char)142, (unsigned char)35);  
    x11Colors["orange"]               = Color((unsigned char)255, (unsigned char)165, (unsigned char)0);
    x11Colors["orangered"]            = Color((unsigned char)255, (unsigned char)69,  (unsigned char)0);
    x11Colors["orchid"]               = Color((unsigned char)218, (unsigned char)112, (unsigned char)214);
    x11Colors["palegoldenrod"]        = Color((unsigned char)238, (unsigned char)232, (unsigned char)170); 
    x11Colors["palegreen"]            = Color((unsigned char)152, (unsigned char)251, (unsigned char)152);
    x11Colors["paleturquoise"]        = Color((unsigned char)175, (unsigned char)238, (unsigned char)238); 
    x11Colors["palevioletred"]        = Color((unsigned char)219, (unsigned char)112, (unsigned char)147); 
    x11Colors["papayawhip"]           = Color((unsigned char)255, (unsigned char)239, (unsigned char)213);
    x11Colors["peachpuff"]            = Color((unsigned char)255, (unsigned char)218, (unsigned char)185);
    x11Colors["peru"]                 = Color((unsigned char)205, (unsigned char)133, (unsigned char)63);
    x11Colors["pink"]                 = Color((unsigned char)255, (unsigned char)192, (unsigned char)203);
    x11Colors["plum"]                 = Color((unsigned char)221, (unsigned char)160, (unsigned char)221);
    x11Colors["powderblue"]           = Color((unsigned char)176, (unsigned char)224, (unsigned char)230);
    x11Colors["purple"]               = Color((unsigned char)128, (unsigned char)0,   (unsigned char)128);
    x11Colors["red"]                  = Color((unsigned char)255, (unsigned char)0,   (unsigned char)0);
    x11Colors["rosybrown"]            = Color((unsigned char)188, (unsigned char)143, (unsigned char)143);
    x11Colors["royalblue"]            = Color((unsigned char)65,  (unsigned char)105, (unsigned char)225);
    x11Colors["saddlebrown"]          = Color((unsigned char)139, (unsigned char)69,  (unsigned char)19);
    x11Colors["salmon"]               = Color((unsigned char)250, (unsigned char)128, (unsigned char)114);
    x11Colors["sandybrown"]           = Color((unsigned char)244, (unsigned char)164, (unsigned char)96);
    x11Colors["seagreen"]             = Color((unsigned char)46,  (unsigned char)139, (unsigned char)87);
    x11Colors["seashell"]             = Color((unsigned char)255, (unsigned char)245, (unsigned char)238);
    x11Colors["sienna"]               = Color((unsigned char)160, (unsigned char)82,  (unsigned char)45);
    x11Colors["silver"]               = Color((unsigned char)192, (unsigned char)192, (unsigned char)192);
    x11Colors["skyblue"]              = Color((unsigned char)135, (unsigned char)206, (unsigned char)235);
    x11Colors["slateblue"]            = Color((unsigned char)106, (unsigned char)90,  (unsigned char)205);
    x11Colors["slategray"]            = Color((unsigned char)112, (unsigned char)128, (unsigned char)144);
    x11Colors["snow"]                 = Color((unsigned char)255, (unsigned char)250, (unsigned char)250);
    x11Colors["springgreen"]          = Color((unsigned char)0,   (unsigned char)255, (unsigned char)127);
    x11Colors["steelblue"]            = Color((unsigned char)70,  (unsigned char)130, (unsigned char)180);
    x11Colors["tan"]                  = Color((unsigned char)210, (unsigned char)180, (unsigned char)140);
    x11Colors["teal"]                 = Color((unsigned char)0,   (unsigned char)128, (unsigned char)128);
    x11Colors["thistle"]              = Color((unsigned char)216, (unsigned char)191, (unsigned char)216);
    x11Colors["tomato"]               = Color((unsigned char)255, (unsigned char)99,  (unsigned char)71);
    x11Colors["turquoise"]            = Color((unsigned char)64,  (unsigned char)224, (unsigned char)208);
    x11Colors["violet"]               = Color((unsigned char)238, (unsigned char)130, (unsigned char)238);
    x11Colors["wheat"]                = Color((unsigned char)245, (unsigned char)222, (unsigned char)179);
    x11Colors["white"]                = Color((unsigned char)255, (unsigned char)255, (unsigned char)255);
    x11Colors["whitesmoke"]           = Color((unsigned char)245, (unsigned char)245, (unsigned char)245);
    x11Colors["yellow"]               = Color((unsigned char)255, (unsigned char)255, (unsigned char)0);
    x11Colors["yellowgreen"]          = Color((unsigned char)154, (unsigned char)205, (unsigned char)50);
}
