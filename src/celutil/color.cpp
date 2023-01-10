// color.cpp
//
// Copyright (C) 2001-2008, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <map>
#include <system_error>

#include <celcompat/charconv.h>
#include "color.h"

using namespace std::string_view_literals;

namespace
{

using ColorMap = std::map<std::string_view, Color>;

const ColorMap* buildX11ColorMap()
{
    return new ColorMap
    {
        { "aliceblue"sv,            Color((std::uint8_t)240, (std::uint8_t)248, (std::uint8_t)255) },
        { "antiquewhite"sv,         Color((std::uint8_t)250, (std::uint8_t)235, (std::uint8_t)215) },
        { "aqua"sv,                 Color((std::uint8_t)0,   (std::uint8_t)255, (std::uint8_t)255) },
        { "aquamarine"sv,           Color((std::uint8_t)127, (std::uint8_t)255, (std::uint8_t)212) },
        { "azure"sv,                Color((std::uint8_t)240, (std::uint8_t)255, (std::uint8_t)255) },
        { "beige"sv,                Color((std::uint8_t)245, (std::uint8_t)245, (std::uint8_t)220) },
        { "bisque"sv,               Color((std::uint8_t)255, (std::uint8_t)228, (std::uint8_t)196) },
        { "black"sv,                Color((std::uint8_t)0,   (std::uint8_t)0,   (std::uint8_t)0)   },
        { "blanchedalmond"sv,       Color((std::uint8_t)255, (std::uint8_t)235, (std::uint8_t)205) },
        { "blue"sv,                 Color((std::uint8_t)0,   (std::uint8_t)0,   (std::uint8_t)255) },
        { "blueviolet"sv,           Color((std::uint8_t)138, (std::uint8_t)43,  (std::uint8_t)226) },
        { "brown"sv,                Color((std::uint8_t)165, (std::uint8_t)42,  (std::uint8_t)42)  },
        { "burlywood"sv,            Color((std::uint8_t)222, (std::uint8_t)184, (std::uint8_t)135) },
        { "cadetblue"sv,            Color((std::uint8_t)95,  (std::uint8_t)158, (std::uint8_t)160) },
        { "chartreuse"sv,           Color((std::uint8_t)127, (std::uint8_t)255, (std::uint8_t)0)   },
        { "chocolate"sv,            Color((std::uint8_t)210, (std::uint8_t)105, (std::uint8_t)30)  },
        { "coral"sv,                Color((std::uint8_t)255, (std::uint8_t)127, (std::uint8_t)80)  },
        { "cornflowerblue"sv,       Color((std::uint8_t)100, (std::uint8_t)149, (std::uint8_t)237) },
        { "cornsilk"sv,             Color((std::uint8_t)255, (std::uint8_t)248, (std::uint8_t)220) },
        { "crimson"sv,              Color((std::uint8_t)220, (std::uint8_t)20,  (std::uint8_t)60)  },
        { "cyan"sv,                 Color((std::uint8_t)0,   (std::uint8_t)255, (std::uint8_t)255) },
        { "darkblue"sv,             Color((std::uint8_t)0,   (std::uint8_t)0,   (std::uint8_t)139) },
        { "darkcyan"sv,             Color((std::uint8_t)0,   (std::uint8_t)139, (std::uint8_t)139) },
        { "darkgoldenrod"sv,        Color((std::uint8_t)184, (std::uint8_t)134, (std::uint8_t)11)  },
        { "darkgray"sv,             Color((std::uint8_t)169, (std::uint8_t)169, (std::uint8_t)169) },
        { "darkgreen"sv,            Color((std::uint8_t)0,   (std::uint8_t)100, (std::uint8_t)0)   },
        { "darkkhaki"sv,            Color((std::uint8_t)189, (std::uint8_t)183, (std::uint8_t)107) },
        { "darkmagenta"sv,          Color((std::uint8_t)139, (std::uint8_t)0,   (std::uint8_t)139) },
        { "darkolivegreen"sv,       Color((std::uint8_t)85,  (std::uint8_t)107, (std::uint8_t)47)  },
        { "darkorange"sv,           Color((std::uint8_t)255, (std::uint8_t)140, (std::uint8_t)0)   },
        { "darkorchid"sv,           Color((std::uint8_t)153, (std::uint8_t)50,  (std::uint8_t)204) },
        { "darkred"sv,              Color((std::uint8_t)139, (std::uint8_t)0,   (std::uint8_t)0)   },
        { "darksalmon"sv,           Color((std::uint8_t)233, (std::uint8_t)150, (std::uint8_t)122) },
        { "darkseagreen"sv,         Color((std::uint8_t)143, (std::uint8_t)188, (std::uint8_t)143) },
        { "darkslateblue"sv,        Color((std::uint8_t)72,  (std::uint8_t)61,  (std::uint8_t)139) },
        { "darkslategray"sv,        Color((std::uint8_t)47,  (std::uint8_t)79,  (std::uint8_t)79)  },
        { "darkturquoise"sv,        Color((std::uint8_t)0,   (std::uint8_t)206, (std::uint8_t)209) },
        { "darkviolet"sv,           Color((std::uint8_t)148, (std::uint8_t)0,   (std::uint8_t)211) },
        { "deeppink"sv,             Color((std::uint8_t)255, (std::uint8_t)20,  (std::uint8_t)147) },
        { "deepskyblue"sv,          Color((std::uint8_t)0,   (std::uint8_t)191, (std::uint8_t)255) },
        { "dimgray"sv,              Color((std::uint8_t)105, (std::uint8_t)105, (std::uint8_t)105) },
        { "dodgerblue"sv,           Color((std::uint8_t)30,  (std::uint8_t)144, (std::uint8_t)255) },
        { "firebrick"sv,            Color((std::uint8_t)178, (std::uint8_t)34,  (std::uint8_t)34)  },
        { "floralwhite"sv,          Color((std::uint8_t)255, (std::uint8_t)250, (std::uint8_t)240) },
        { "forestgreen"sv,          Color((std::uint8_t)34,  (std::uint8_t)139, (std::uint8_t)34)  },
        { "fuchsia"sv,              Color((std::uint8_t)255, (std::uint8_t)0,   (std::uint8_t)255) },
        { "gainsboro"sv,            Color((std::uint8_t)220, (std::uint8_t)220, (std::uint8_t)220) },
        { "ghostwhite"sv,           Color((std::uint8_t)248, (std::uint8_t)248, (std::uint8_t)255) },
        { "gold"sv,                 Color((std::uint8_t)255, (std::uint8_t)215, (std::uint8_t)0)   },
        { "goldenrod"sv,            Color((std::uint8_t)218, (std::uint8_t)165, (std::uint8_t)32)  },
        { "gray"sv,                 Color((std::uint8_t)128, (std::uint8_t)128, (std::uint8_t)128) },
        { "green"sv,                Color((std::uint8_t)0,   (std::uint8_t)128, (std::uint8_t)0)   },
        { "greenyellow"sv,          Color((std::uint8_t)173, (std::uint8_t)255, (std::uint8_t)47)  },
        { "honeydew"sv,             Color((std::uint8_t)240, (std::uint8_t)255, (std::uint8_t)240) },
        { "hotpink"sv,              Color((std::uint8_t)255, (std::uint8_t)105, (std::uint8_t)180) },
        { "indianred"sv,            Color((std::uint8_t)205, (std::uint8_t)92,  (std::uint8_t)92)  },
        { "indigo"sv,               Color((std::uint8_t)75,  (std::uint8_t)0,   (std::uint8_t)130) },
        { "ivory"sv,                Color((std::uint8_t)255, (std::uint8_t)255, (std::uint8_t)240) },
        { "khaki"sv,                Color((std::uint8_t)240, (std::uint8_t)230, (std::uint8_t)140) },
        { "lavender"sv,             Color((std::uint8_t)230, (std::uint8_t)230, (std::uint8_t)250) },
        { "lavenderblush"sv,        Color((std::uint8_t)255, (std::uint8_t)240, (std::uint8_t)245) },
        { "lawngreen"sv,            Color((std::uint8_t)124, (std::uint8_t)252, (std::uint8_t)0)   },
        { "lemonchiffon"sv,         Color((std::uint8_t)255, (std::uint8_t)250, (std::uint8_t)205) },
        { "lightblue"sv,            Color((std::uint8_t)173, (std::uint8_t)216, (std::uint8_t)230) },
        { "lightcoral"sv,           Color((std::uint8_t)240, (std::uint8_t)128, (std::uint8_t)128) },
        { "lightcyan"sv,            Color((std::uint8_t)224, (std::uint8_t)255, (std::uint8_t)255) },
        { "lightgoldenrodyellow"sv, Color((std::uint8_t)250, (std::uint8_t)250, (std::uint8_t)210) },
        { "lightgreen"sv,           Color((std::uint8_t)144, (std::uint8_t)238, (std::uint8_t)144) },
        { "lightgrey"sv,            Color((std::uint8_t)211, (std::uint8_t)211, (std::uint8_t)211) },
        { "lightpink"sv,            Color((std::uint8_t)255, (std::uint8_t)182, (std::uint8_t)193) },
        { "lightsalmon"sv,          Color((std::uint8_t)255, (std::uint8_t)160, (std::uint8_t)122) },
        { "lightseagreen"sv,        Color((std::uint8_t)32,  (std::uint8_t)178, (std::uint8_t)170) },
        { "lightskyblue"sv,         Color((std::uint8_t)135, (std::uint8_t)206, (std::uint8_t)250) },
        { "lightslategray"sv,       Color((std::uint8_t)119, (std::uint8_t)136, (std::uint8_t)153) },
        { "lightsteelblue"sv,       Color((std::uint8_t)176, (std::uint8_t)196, (std::uint8_t)222) },
        { "lightyellow"sv,          Color((std::uint8_t)255, (std::uint8_t)255, (std::uint8_t)224) },
        { "lime"sv,                 Color((std::uint8_t)0,   (std::uint8_t)255, (std::uint8_t)0)   },
        { "limegreen"sv,            Color((std::uint8_t)50,  (std::uint8_t)205, (std::uint8_t)50)  },
        { "linen"sv,                Color((std::uint8_t)250, (std::uint8_t)240, (std::uint8_t)230) },
        { "magenta"sv,              Color((std::uint8_t)255, (std::uint8_t)0,   (std::uint8_t)255) },
        { "maroon"sv,               Color((std::uint8_t)128, (std::uint8_t)0,   (std::uint8_t)0)   },
        { "mediumaquamarine"sv,     Color((std::uint8_t)102, (std::uint8_t)205, (std::uint8_t)170) },
        { "mediumblue"sv,           Color((std::uint8_t)0,   (std::uint8_t)0,   (std::uint8_t)205) },
        { "mediumorchid"sv,         Color((std::uint8_t)186, (std::uint8_t)85,  (std::uint8_t)211) },
        { "mediumpurple"sv,         Color((std::uint8_t)147, (std::uint8_t)112, (std::uint8_t)219) },
        { "mediumseagreen"sv,       Color((std::uint8_t)60,  (std::uint8_t)179, (std::uint8_t)113) },
        { "mediumslateblue"sv,      Color((std::uint8_t)123, (std::uint8_t)104, (std::uint8_t)238) },
        { "mediumspringgreen"sv,    Color((std::uint8_t)0,   (std::uint8_t)250, (std::uint8_t)154) },
        { "mediumturquoise"sv,      Color((std::uint8_t)72,  (std::uint8_t)209, (std::uint8_t)204) },
        { "mediumvioletred"sv,      Color((std::uint8_t)199, (std::uint8_t)21,  (std::uint8_t)133) },
        { "midnightblue"sv,         Color((std::uint8_t)25,  (std::uint8_t)25,  (std::uint8_t)112) },
        { "mintcream"sv,            Color((std::uint8_t)245, (std::uint8_t)255, (std::uint8_t)250) },
        { "mistyrose"sv,            Color((std::uint8_t)255, (std::uint8_t)228, (std::uint8_t)225) },
        { "moccasin"sv,             Color((std::uint8_t)255, (std::uint8_t)228, (std::uint8_t)181) },
        { "navajowhite"sv,          Color((std::uint8_t)255, (std::uint8_t)222, (std::uint8_t)173) },
        { "navy"sv,                 Color((std::uint8_t)0,   (std::uint8_t)0,   (std::uint8_t)128) },
        { "oldlace"sv,              Color((std::uint8_t)253, (std::uint8_t)245, (std::uint8_t)230) },
        { "olive"sv,                Color((std::uint8_t)128, (std::uint8_t)128, (std::uint8_t)0)   },
        { "olivedrab"sv,            Color((std::uint8_t)107, (std::uint8_t)142, (std::uint8_t)35)  },
        { "orange"sv,               Color((std::uint8_t)255, (std::uint8_t)165, (std::uint8_t)0)   },
        { "orangered"sv,            Color((std::uint8_t)255, (std::uint8_t)69,  (std::uint8_t)0)   },
        { "orchid"sv,               Color((std::uint8_t)218, (std::uint8_t)112, (std::uint8_t)214) },
        { "palegoldenrod"sv,        Color((std::uint8_t)238, (std::uint8_t)232, (std::uint8_t)170) },
        { "palegreen"sv,            Color((std::uint8_t)152, (std::uint8_t)251, (std::uint8_t)152) },
        { "paleturquoise"sv,        Color((std::uint8_t)175, (std::uint8_t)238, (std::uint8_t)238) },
        { "palevioletred"sv,        Color((std::uint8_t)219, (std::uint8_t)112, (std::uint8_t)147) },
        { "papayawhip"sv,           Color((std::uint8_t)255, (std::uint8_t)239, (std::uint8_t)213) },
        { "peachpuff"sv,            Color((std::uint8_t)255, (std::uint8_t)218, (std::uint8_t)185) },
        { "peru"sv,                 Color((std::uint8_t)205, (std::uint8_t)133, (std::uint8_t)63)  },
        { "pink"sv,                 Color((std::uint8_t)255, (std::uint8_t)192, (std::uint8_t)203) },
        { "plum"sv,                 Color((std::uint8_t)221, (std::uint8_t)160, (std::uint8_t)221) },
        { "powderblue"sv,           Color((std::uint8_t)176, (std::uint8_t)224, (std::uint8_t)230) },
        { "purple"sv,               Color((std::uint8_t)128, (std::uint8_t)0,   (std::uint8_t)128) },
        { "red"sv,                  Color((std::uint8_t)255, (std::uint8_t)0,   (std::uint8_t)0)   },
        { "rosybrown"sv,            Color((std::uint8_t)188, (std::uint8_t)143, (std::uint8_t)143) },
        { "royalblue"sv,            Color((std::uint8_t)65,  (std::uint8_t)105, (std::uint8_t)225) },
        { "saddlebrown"sv,          Color((std::uint8_t)139, (std::uint8_t)69,  (std::uint8_t)19)  },
        { "salmon"sv,               Color((std::uint8_t)250, (std::uint8_t)128, (std::uint8_t)114) },
        { "sandybrown"sv,           Color((std::uint8_t)244, (std::uint8_t)164, (std::uint8_t)96)  },
        { "seagreen"sv,             Color((std::uint8_t)46,  (std::uint8_t)139, (std::uint8_t)87)  },
        { "seashell"sv,             Color((std::uint8_t)255, (std::uint8_t)245, (std::uint8_t)238) },
        { "sienna"sv,               Color((std::uint8_t)160, (std::uint8_t)82,  (std::uint8_t)45)  },
        { "silver"sv,               Color((std::uint8_t)192, (std::uint8_t)192, (std::uint8_t)192) },
        { "skyblue"sv,              Color((std::uint8_t)135, (std::uint8_t)206, (std::uint8_t)235) },
        { "slateblue"sv,            Color((std::uint8_t)106, (std::uint8_t)90,  (std::uint8_t)205) },
        { "slategray"sv,            Color((std::uint8_t)112, (std::uint8_t)128, (std::uint8_t)144) },
        { "snow"sv,                 Color((std::uint8_t)255, (std::uint8_t)250, (std::uint8_t)250) },
        { "springgreen"sv,          Color((std::uint8_t)0,   (std::uint8_t)255, (std::uint8_t)127) },
        { "steelblue"sv,            Color((std::uint8_t)70,  (std::uint8_t)130, (std::uint8_t)180) },
        { "tan"sv,                  Color((std::uint8_t)210, (std::uint8_t)180, (std::uint8_t)140) },
        { "teal"sv,                 Color((std::uint8_t)0,   (std::uint8_t)128, (std::uint8_t)128) },
        { "thistle"sv,              Color((std::uint8_t)216, (std::uint8_t)191, (std::uint8_t)216) },
        { "tomato"sv,               Color((std::uint8_t)255, (std::uint8_t)99,  (std::uint8_t)71)  },
        { "turquoise"sv,            Color((std::uint8_t)64,  (std::uint8_t)224, (std::uint8_t)208) },
        { "violet"sv,               Color((std::uint8_t)238, (std::uint8_t)130, (std::uint8_t)238) },
        { "wheat"sv,                Color((std::uint8_t)245, (std::uint8_t)222, (std::uint8_t)179) },
        { "white"sv,                Color((std::uint8_t)255, (std::uint8_t)255, (std::uint8_t)255) },
        { "whitesmoke"sv,           Color((std::uint8_t)245, (std::uint8_t)245, (std::uint8_t)245) },
        { "yellow"sv,               Color((std::uint8_t)255, (std::uint8_t)255, (std::uint8_t)0)   },
        { "yellowgreen"sv,          Color((std::uint8_t)154, (std::uint8_t)205, (std::uint8_t)50)  },
    };
}

bool parseHexColor(std::string_view s, Color& c)
{
    using celestia::compat::from_chars;

    std::uint32_t value;
    switch (s.size())
    {
    case 3: // rgb
        if (auto [ptr, ec] = from_chars(s.data(), s.data() + s.size(), value, 16);
            ec == std::errc{} && ptr == s.data() + s.size())
        {
            c = Color(static_cast<std::uint8_t>((value >> 8) * UINT32_C(0x11)),
                    static_cast<std::uint8_t>(((value & UINT32_C(0x0f0)) >> 4) * UINT32_C(0x11)),
                    static_cast<std::uint8_t>((value & UINT32_C(0x00f)) * UINT32_C(0x11)));
            return true;
        }
        break;

    case 6: // rrggbb
        if (auto [ptr, ec] = from_chars(s.data(), s.data() + s.size(), value, 16);
            ec == std::errc{} && ptr == s.data() + s.size())
        {
            c = Color(static_cast<std::uint8_t>(value >> 16),
                    static_cast<std::uint8_t>((value & UINT32_C(0x00ff00)) >> 8),
                    static_cast<std::uint8_t>(value & UINT32_C(0x0000ff)));
            return true;
        }
        break;

    case 8: // rrggbbaa
        if (auto [ptr, ec] = from_chars(s.data(), s.data() + s.size(), value, 16);
            ec == std::errc{} && ptr == s.data() + s.size())
        {
            c = Color(static_cast<std::uint8_t>(value >> 24),
                    static_cast<std::uint8_t>((value & UINT32_C(0x00ff0000)) >> 16),
                    static_cast<std::uint8_t>((value & UINT32_C(0x0000ff00)) >> 8),
                    static_cast<std::uint8_t>(value & UINT32_C(0x000000ff)));
            return true;
        }
        break;

    default:
        break;
    }
    return false;
}

} // end unnamed namespace


const Color Color::White = Color(1.0f, 1.0f, 1.0f);
const Color Color::Black = Color(0.0f,0.0f, 0.0f);

/*! Parse a color string and return true if it was a valid color, otherwise
 *  false.  Accetable inputs are HTML/X11 style #xxxxxx colors (where x is
 *  hexadecimal digit) or one of a list of named colors.
 */
bool Color::parse(std::string_view s, Color& c)
{
    using celestia::compat::from_chars;

    if (s.empty()) { return false; }

    if (s[0] == '#')
    {
        return parseHexColor(s.substr(1), c);
    }
    else
    {
        static const ColorMap* x11Colors = nullptr;

        // Initialize the color dictionary if this is the first
        // time Color::parse has been called for a non-hex value.
        if (x11Colors == nullptr)
        {
            x11Colors = buildX11ColorMap();
        }

        auto it = x11Colors->find(s);
        if (it != x11Colors->end())
        {
            c = it->second;
            return true;
        }
    }

    return false;
}
