// solarsysxml.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>

#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/SAX.h>

#include <celutil/debug.h>
#include <celmath/mathlib.h>
#include "astro.h"
#include "customorbit.h"
#include "texmanager.h"
#include "meshmanager.h"
#include "solarsysxml.h"

using namespace std;


struct UnitDefinition
{
    char* name;
    double conversion;
};

UnitDefinition distanceUnits[] =
{
    { "km", 1.0 },
    { "m",  0.001 },
    { "au", 149597870.7 },
    { "ly", 9466411842000.0 },
};

UnitDefinition timeUnits[] =
{
    { "s", 1.0 },
    { "m", 60.0 },
    { "h", 3600.0 },
    { "d", 86400.0 },
    { "y", 86400.0 * 365.25 },
};


static xmlSAXHandler emptySAXHandler =
{
    NULL, /* internalSubset */
    NULL, /* isStandalone */
    NULL, /* hasInternalSubset */
    NULL, /* hasExternalSubset */
    NULL, /* resolveEntity */
    NULL, /* getEntity */
    NULL, /* entityDecl */
    NULL, /* notationDecl */
    NULL, /* attributeDecl */
    NULL, /* elementDecl */
    NULL, /* unparsedEntityDecl */
    NULL, /* setDocumentLocator */
    NULL, /* startDocument */
    NULL, /* endDocument */
    NULL, /* startElement */
    NULL, /* endElement */
    NULL, /* reference */
    NULL, /* characters */
    NULL, /* ignorableWhitespace */
    NULL, /* processingInstruction */
    NULL, /* comment */
    NULL, /* xmlParserWarning */
    NULL, /* xmlParserError */
    NULL, /* xmlParserError */
    NULL, /* getParameterEntity */
    NULL, /* cdataBlock; */
    NULL,  /* externalSubset; */
    1
};

static xmlSAXHandler saxHandler;

enum ParserState {
    StartState,
    EndState,
    BodyState,
    SurfaceState,
    AtmosphereState,
    RingsState,
    BodyLeafState,
    SurfaceLeafState,
    AtmosphereLeafState,
    RingsLeafState,
    ErrorState,
};

struct ParserContext
{
    ParserState state;
    Body* body;
    Universe* universe;
};


static bool matchName(const xmlChar* s, const char* name)
{
    while ((char) *s == *name)
    {
        if (*s == '\0')
            return true;
        s++;
        name++;
    }

    return false;
}


static bool parseBoolean(const xmlChar* s, bool& b)
{
    if (matchName(s, "true") || matchName(s, "1") || matchName(s, "on"))
    {
        b = true;
        return true;
    }
    else if (matchName(s, "false") || matchName(s, "0") || matchName(s, "off"))
    {
        b = false;
        return true;
    }
    else
    {
        return false;
    }
}


static bool parseNumber(const xmlChar* s, double& d)
{
    return sscanf((char*) s, "%lf", &d) == 1;
}


static bool parseNumber(const xmlChar* s, float& f)
{
    double d;
    if (parseNumber(s, d))
    {
        f = (float) d;
        return true;
    }
    else
    {
        return false;
    }
}


static bool parseNumberUnits(const xmlChar* s,
                             double& d,
                             UnitDefinition* unitTable,
                             int unitTableLength,
                             char* defaultUnitName)
{
    char unitName[4];
    double value;
    int nMatched = sscanf(reinterpret_cast<const char*>(s),
                          "%lf%3s", &value, unitName);
    cout << "parseNumberUnits(" << reinterpret_cast<const char*>(s) << ")\n";
    if (nMatched == 1)
    {
        cout << "matched = 1: " << reinterpret_cast<const char*>(s) << '\n';
        d = value;
        return true;
    }
    else if (nMatched == 2)
    {
        cout << "matched = 2: " << reinterpret_cast<const char*>(s) << '\n';
        // Found a value and units; multiply the value by a conversion
        // factor to produce default units
        UnitDefinition* defaultUnit = NULL;
        UnitDefinition* unit = NULL;
        int i;

        // Get the default unit
        for (i = 0; i < unitTableLength; i++)
        {
            if (strcmp(defaultUnitName, unitTable[i].name) == 0)
            {
                defaultUnit = &unitTable[i];
                break;
            }
        }
        assert(defaultUnit != NULL);

        // Try and the match the unit name specified in the xml attribute
        for (i = 0; i < unitTableLength; i++)
        {
            if (strcmp(unitName, unitTable[i].name) == 0)
            {
                unit = &unitTable[i];
                break;
            }
        }

        if (unit != NULL)
        {
            // Match found; perform the conversion
            d = value * unit->conversion  / defaultUnit->conversion;
            cout << "converting: " << value << unit->name << " = " << d << defaultUnit->name << "\n";
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        // Error . . . we matched nothing.
        return false;
    }
}


static bool parseDistance(const xmlChar* s, double& d, char* defaultUnitName)
{
    return parseNumberUnits(s, d,
                            distanceUnits,
                            sizeof distanceUnits / sizeof distanceUnits[0],
                            defaultUnitName);
}


static bool parseDistance(const xmlChar* s, float& f, char* defaultUnitName)
{
    double d;
    if (parseDistance(s, d, defaultUnitName))
    {
        f = (float) d;
        return true;
    }
    else
    {
        return false;
    }
}


static bool parseAngle(const xmlChar* s, double& d)
{
    return parseNumber(s, d);
}


static bool parseAngle(const xmlChar* s, float& f)
{
    double d;
    if (parseAngle(s, d))
    {
        f = (float) d;
        return true;
    }
    else
    {
        return false;
    }
}


static bool parseTime(const xmlChar* s, double& d, char* defaultUnitName)
{
    return parseNumberUnits(s, d,
                            timeUnits,
                            sizeof timeUnits / sizeof timeUnits[0],
                            defaultUnitName);
}


static bool parseTime(const xmlChar* s, float& f, char* defaultUnitName)
{
    double d;
    if (parseTime(s, d, defaultUnitName))
    {
        f = (float) d;
        return true;
    }
    else
    {
        return false;
    }
}


static bool parseEpoch(const xmlChar* s, double& d)
{
    if (matchName(s, "J2000"))
    {
        d = astro::J2000;
        return true;
    }
    else
    {
        return parseNumber(s, d);
    }
}


static int hexDigit(char c)
{
    // Assumes an ASCII character set . . .
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    else if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    else
        return 0; // bad digit
}


ostream& operator<<(ostream& out, const Color& c)
{
    cout << '[' << c.red() << ',' << c.green() << ',' << c.blue() << ']';
    return cout;
}


static bool parseColor(const xmlChar* s, Color& c)
{
    const char* colorName = reinterpret_cast<const char*>(s);
    char hexColor[7];
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;

    cout << "parsing color: " << colorName << '\n';

    if (sscanf(colorName, " #%6[0-9a-fA-F] ", hexColor))
    {
        if (strlen(hexColor) == 6)
        {
            c = Color((hexDigit(hexColor[0]) * 16 +
                       hexDigit(hexColor[1])) / 255.0f,
                      (hexDigit(hexColor[2]) * 16 +
                       hexDigit(hexColor[3])) / 255.0f,
                      (hexDigit(hexColor[4]) * 16 +
                       hexDigit(hexColor[5])) / 255.0f);
            cout << "1: " << c << '\n';
            return true;
        }
        else if (strlen(hexColor) == 3)
        {
            c = Color((hexDigit(hexColor[0]) * 17) / 255.0f,
                      (hexDigit(hexColor[1]) * 17) / 255.0f,
                      (hexDigit(hexColor[2]) * 17) / 255.0f);
            cout << "2: " << c << '\n';
            return true;
        }
        else
        {
            return false;
        }
    }
    else if (sscanf(colorName, " rgb( %f , %f , %f ) ", &r, &g, &b) == 3)
    {
        c = Color(r / 255.0f, g / 255.0f, b / 255.0f);
        cout << "3: " << c << '\n';
        return true;
    }
    else if (sscanf(colorName, " rgb( %f%% , %f%% , %f%% ) ", &r, &g, &b) == 3)
    {
        c = Color(r / 100.0f, g / 100.0f, b / 100.0f);
        cout << "4: " << c << '\n';
        return true;
    }
    else
    {
        return false;
    }
}


static bool createBody(ParserContext* ctx, const xmlChar** att)
{
    const xmlChar* name = NULL;
    const xmlChar* parentName = NULL;

    // Get the name and parent attributes
    if (att != NULL)
    {
        for (int i = 0; att[i] != NULL; i += 2)
        {
            if (matchName(att[i], "name"))
                name = att[i + 1];
            else if (matchName(att[i], "parent"))
                parentName = att[i + 1];
        }
    }

    // Require that both are present
    if (name == NULL)
    {
        return false;
    }
    else if (parentName == NULL)
    {
        return false;
    }

    bool orbitsPlanet = false;
    Selection parent = ctx->universe->findPath(reinterpret_cast<const char*>(parentName), NULL, 0);
    PlanetarySystem* parentSystem = NULL;
    if (parent.star != NULL)
    {
        SolarSystem* solarSystem = ctx->universe->getSolarSystem(parent.star);
        if (solarSystem == NULL)
        {
            // No solar system defined for this star yet, so we need
            // to create it.
            solarSystem = ctx->universe->createSolarSystem(parent.star);
        }
        parentSystem = solarSystem->getPlanets();
    }
    else if (parent.body != NULL)
    {
        // Parent is a planet or moon
        parentSystem = parent.body->getSatellites();
        if (parentSystem == NULL)
        {
            // If the planet doesn't already have any satellites, we
            // have to create a new planetary system for it.
            parentSystem = new PlanetarySystem(parent.body);
            parent.body->setSatellites(parentSystem);
        }
        orbitsPlanet = true;
    }
    else
    {
        cout << "Parent body '" << parentName << "' of '" << name << "' not found.\n";
        return false;
    }

    if (parentSystem != NULL)
    {
        ctx->body = new Body(parentSystem);
        ctx->body->setName(reinterpret_cast<const char*>(name));
        parentSystem->addBody(ctx->body);
        return true;
    }

    return false;
}


static ResourceHandle createTexture(ParserContext* ctx, const xmlChar** att)
{
    const xmlChar* type = reinterpret_cast<const xmlChar*>("base");
    const xmlChar* image = NULL;
    bool compress = false;

    // Get the type, image, and compress attributes
    if (att != NULL)
    {
        for (int i = 0; att[i] != NULL; i += 2)
        {
            if (matchName(att[i], "type"))
                type = att[i + 1];
            else if (matchName(att[i], "image"))
                image = att[i + 1];
            else if (matchName(att[i], "compress"))
                parseBoolean(att[i + 1], compress);
        }
    }

    if (image == NULL)
    {
        cout << "Texture has no image source.\n";
        return false;
    }

    ResourceHandle texHandle = GetTextureManager()->getHandle(TextureInfo(reinterpret_cast<const char*>(image), compress));
    if (ctx->state == SurfaceState)
    {
        assert(ctx->body != NULL);

        if (matchName(type, "base"))
            ctx->body->getSurface().baseTexture = texHandle;
        else if (matchName(type, "night"))
            ctx->body->getSurface().nightTexture = texHandle;
    }
    else if (ctx->state == AtmosphereState)
    {
        assert(ctx->body != NULL);
        Atmosphere* atmosphere = ctx->body->getAtmosphere();
        assert(atmosphere != NULL);

        if (matchName(type, "base"))
            atmosphere->cloudTexture = texHandle;
    }
    else if (ctx->state == RingsState)
    {
        assert(ctx->body != NULL);
        assert(ctx->body->getRings() != NULL);

        if (matchName(type, "base"))
            ctx->body->getRings()->texture = texHandle;
    }

    return true;
}


static ResourceHandle createBumpMap(ParserContext* ctx, const xmlChar** att)
{
    const xmlChar* heightmap = NULL;
    float bumpHeight = 2.5f;

    // Get the type, image, and compress attributes
    if (att != NULL)
    {
        for (int i = 0; att[i] != NULL; i += 2)
        {
            if (matchName(att[i], "heightmap"))
                heightmap = att[i + 1];
            else if (matchName(att[i], "bump-height"))
                parseNumber(att[i + 1], bumpHeight);
        }
    }

    if (heightmap == NULL)
    {
        cout << "Bump map has no height map source.\n";
        return false;
    }

    ResourceHandle texHandle = GetTextureManager()->getHandle(TextureInfo(reinterpret_cast<const char*>(heightmap), bumpHeight));
    if (ctx->state == SurfaceState)
    {
        assert(ctx->body != NULL);
        if (texHandle != InvalidResource)
        {
            ctx->body->getSurface().bumpTexture = texHandle;
            ctx->body->getSurface().appearanceFlags |= Surface::ApplyBumpMap;
        }
        return true;
    }
    else
    {
        return false;
    }
}


static bool createAtmosphere(ParserContext* ctx, const xmlChar** att)
{
    Atmosphere* atmosphere = new Atmosphere();

    if (att != NULL)
    {
        for (int i = 0; att[i] != NULL; i += 2)
        {
            if (matchName(att[i], "height"))
                parseDistance(att[i + 1], atmosphere->height, "km");
            else if (matchName(att[i], "lower-color"))
                parseColor(att[i + 1], atmosphere->lowerColor);
            else if (matchName(att[i], "upper-color"))
                parseColor(att[i + 1], atmosphere->upperColor);
            else if (matchName(att[i], "sky-color"))
                parseColor(att[i + 1], atmosphere->skyColor);
            else if (matchName(att[i], "cloud-height"))
                parseDistance(att[i + 1], atmosphere->cloudHeight, "km");
            else if (matchName(att[i], "cloud-speed"))
                parseAngle(att[i + 1], atmosphere->cloudSpeed);
        }
    }

    assert(ctx->body != NULL);
    ctx->body->setAtmosphere(*atmosphere);
    delete atmosphere;
    
    return true;
}


static bool createHaze(ParserContext* ctx, const xmlChar** att)
{
    Color hazeColor;
    float hazeDensity = 0.0f;

    if (att != NULL)
    {
        for (int i = 0; att[i] != NULL; i += 2)
        {
            if (matchName(att[i], "density"))
                parseNumber(att[i + 1], hazeDensity);
            else if (matchName(att[i], "color"))
                parseColor(att[i + 1], hazeColor);
        }
    }

    assert(ctx->body != NULL);
    ctx->body->getSurface().hazeColor = Color(hazeColor.red(),
                                              hazeColor.green(),
                                              hazeColor.blue(),
                                              hazeDensity);
    
    return true;
}


static bool createSurface(ParserContext* ctx, const xmlChar** att)
{
    Color color(1.0f, 1.0f, 1.0f);
    Color specularColor(0.0f, 0.0f, 0.0f);
    float specularPower = 0.0f;
    float albedo = 0.5f;
    bool blendTexture = false;
    bool emissive = false;

    if (att != NULL)
    {
        for (int i = 0; att[i] != NULL; i += 2)
        {
            if (matchName(att[i], "color"))
                parseColor(att[i + 1], color);
            if (matchName(att[i], "specular-color"))
                parseColor(att[i + 1], specularColor);
            if (matchName(att[i], "specular-power"))
                parseNumber(att[i + 1], specularPower);
            if (matchName(att[i], "blend-texture"))
                parseBoolean(att[i + 1], blendTexture);
            if (matchName(att[i], "emissive"))
                parseBoolean(att[i + 1], emissive);
            if (matchName(att[i], "albedo"))
                parseNumber(att[i + 1], albedo);
        }
    }

    assert(ctx->body != NULL);
    ctx->body->setAlbedo(albedo);
    ctx->body->getSurface().color = color;
    ctx->body->getSurface().specularColor = specularColor;
    ctx->body->getSurface().specularPower = specularPower;
    if (blendTexture)
        ctx->body->getSurface().appearanceFlags |= Surface::BlendTexture;
    if (emissive)
        ctx->body->getSurface().appearanceFlags |= Surface::Emissive;

    return true;
}


static bool createEllipticalOrbit(ParserContext* ctx, const xmlChar** att)
{
    // SemiMajorAxis and Period are absolutely required; everything
    // else has a reasonable default.
    double pericenterDistance = 0.0;
    double semiMajorAxis = 0.0;
    double period = 0.0;
    double eccentricity = 0.0;
    double inclination = 0.0;
    double ascendingNode = 0.0;
    double argOfPericenter = 0.0;
    double anomalyAtEpoch = 0.0;
    double epoch = astro::J2000;
    bool foundPeriod = false;
    bool foundSMA = false;
    bool foundPD = false;

    // On the first pass through the attribute list, extract the
    // period, epoch, ascending node, semi-major axis, eccentricity,
    // and inclination.
    if (att != NULL)
    {
        int i;
        for (i = 0; att[i] != NULL; i += 2)
        {
            if (matchName(att[i], "period"))
            {
                foundPeriod = true;
                parseTime(att[i + 1], period, "d");
            }
            else if (matchName(att[i], "semi-major-axis"))
            {
                foundSMA = true;
                parseDistance(att[i + 1], semiMajorAxis, "km");
                cout << "SMA: " << semiMajorAxis << '\n';
            }
            else if (matchName(att[i], "pericenter-distance"))
            {
                foundPD = true;
                parseDistance(att[i + 1], pericenterDistance, "km");
            }
            else if (matchName(att[i], "epoch"))
                parseEpoch(att[i + 1], epoch);
            else if (matchName(att[i], "eccentricity"))
                parseNumber(att[i + 1], eccentricity);
            else if (matchName(att[i], "inclination"))
                parseAngle(att[i + 1], inclination);
            else if (matchName(att[i], "ascending-node"))
                parseAngle(att[i + 1], ascendingNode);
        }

        // On the next pass, get the argument or longitude of pericenter; it's
        // important that we get the longitude of pericenter after we know the
        // ascending node, because this value is required to convert to
        // argument of pericenter
        for (i = 0; att[i] != NULL; i += 2)
        {
            if (matchName(att[i], "arg-of-pericenter"))
            {
                parseAngle(att[i + 1], argOfPericenter);
            }
            else if (matchName(att[i + 1], "long-of-pericenter"))
            {
                double longOfPericenter;
                parseAngle(att[i + 1], longOfPericenter);
                argOfPericenter = longOfPericenter - ascendingNode;
            }
        }

        // On the third pass, get the anomaly or mean longitude; converting
        // from mean longitude to anomaly requires the arg of pericenter from the
        // second pass.
        for (i = 0; att[i] != NULL; i += 2)
        {
            if (matchName(att[i], "mean-anomaly"))
            {
                parseAngle(att[i + 1], anomalyAtEpoch);
            }
            else if (matchName(att[i + 1], "mean-longitude"))
            {
                double longAtEpoch;
                parseAngle(att[i + 1], longAtEpoch);
                anomalyAtEpoch = longAtEpoch - (argOfPericenter + ascendingNode);
            }
        }
    }

    if (!foundPeriod)
    {
        return false;
    }
    else if (!foundSMA && !foundPD)
    {
        return false;
    }

    // If we read the semi-major axis, use it to compute the pericenter
    // distance.
    if (foundSMA)
        pericenterDistance = semiMajorAxis * (1.0 - eccentricity);

    EllipticalOrbit* orbit = new EllipticalOrbit(pericenterDistance,
                                                 eccentricity,
                                                 degToRad(inclination),
                                                 degToRad(ascendingNode),
                                                 degToRad(argOfPericenter),
                                                 degToRad(anomalyAtEpoch),
                                                 period,
                                                 epoch);
    assert(ctx->body != NULL);

    // Custom orbits have precedence over elliptical orbits, so don't set
    // the orbit if the object already has one assigned.
    if (ctx->body->getOrbit() == NULL)
        ctx->body->setOrbit(orbit);
    else
        delete orbit;

    return true;
}


static bool createCustomOrbit(ParserContext* ctx, const xmlChar** att)
{
    const xmlChar* name = NULL;

    // Get the type, image, and compress attributes
    if (att != NULL)
    {
        for (int i = 0; att[i] != NULL; i += 2)
        {
            if (matchName(att[i], "name"))
                name = att[i + 1];
        }
    }

    if (name == NULL)
        return false;

    Orbit* orbit = GetCustomOrbit(reinterpret_cast<const char*>(name));
    if (orbit == NULL)
    {
        DPRINTF(0, "Could not find custom orbit named '%s'\n",
                reinterpret_cast<const char*>(name));
    }
    else
    {
        assert(ctx->body != NULL);
        ctx->body->setOrbit(orbit);
    }

    return true;
}


static bool createRotation(ParserContext* ctx, const xmlChar** att)
{
    double period = 0.0;
    double obliquity = 0.0;
    double axisLongitude = 0.0;
    double offset = 0.0;
    double epoch = astro::J2000;

    if (att != NULL)
    {
        for (int i = 0; att[i] != NULL; i += 2)
        {
            if (matchName(att[i], "period"))
            {
                if (matchName(att[i + 1], "sync"))
                    period = 0.0;
                else
                    parseTime(att[i + 1], period, "h");
            }
            else if (matchName(att[i], "obliquity"))
                parseAngle(att[i + 1], obliquity);
            else if (matchName(att[i], "axis-longitude"))
                parseAngle(att[i + 1], axisLongitude);
            else if (matchName(att[i], "offset"))
                parseAngle(att[i + 1], offset);
            else if (matchName(att[i], "epoch"))
                parseEpoch(att[i + 1], epoch);
        }
    }

    assert(ctx->body != NULL);

    RotationElements re;
    // A period of 0 means that the object is in synchronous rotation, so
    // we'll set its rotation period equal to its orbital period.  The catch
    // is that we require that the orbit was specified before the rotation
    // elements within the XML file.
    Orbit* orbit = ctx->body->getOrbit();
    if (orbit == NULL)
        return false;

    if (period == 0.0)
        re.period = (float) orbit->getPeriod();
    else
        re.period = (float) period / 24.0f;
    re.obliquity = (float) degToRad(obliquity);
    re.axisLongitude = (float) degToRad(axisLongitude);
    re.offset = (float) degToRad(offset);
    re.epoch = epoch;
    ctx->body->setRotationElements(re);
    
    return true;
}


static bool createGeometry(ParserContext* ctx, const xmlChar** att)
{
    double radius = 1.0;
    double oblateness = 0.0;
    const xmlChar* meshName = NULL;

    // Get the radius and mesh attributes
    if (att != NULL)
    {
        for (int i = 0; att[i] != NULL; i += 2)
        {
            if (matchName(att[i], "radius"))
                parseDistance(att[i + 1], radius, "km");
            else if (matchName(att[i], "mesh"))
                meshName = att[i + 1];
            else if (matchName(att[i], "oblateness"))
                parseNumber(att[i + 1], oblateness);
        }
    }

    assert(ctx->body != NULL);

    ResourceHandle meshHandle = InvalidResource;
    if (meshName != NULL)
        meshHandle = GetMeshManager()->getHandle(MeshInfo(reinterpret_cast<const char*>(meshName)));
    ctx->body->setMesh(meshHandle);
    ctx->body->setRadius((float) radius);
    ctx->body->setOblateness((float) oblateness);

    return false;
}


static bool createRings(ParserContext* ctx, const xmlChar** att)
{
    double innerRadius = 0.0;
    double outerRadius = 0.0;
    Color color(1.0f, 1.0f, 1.0f);

    // Get the radius and color attributes
    if (att != NULL)
    {
        for (int i = 0; att[i] != NULL; i += 2)
        {
            if (matchName(att[i], "inner-radius"))
                parseDistance(att[i + 1], innerRadius, "km");
            else if (matchName(att[i], "outer-radius"))
                parseDistance(att[i + 1], outerRadius, "km");
            else if (matchName(att[i], "color"))
                parseColor(att[i + 1], color);
        }
    }

    assert(ctx->body != NULL);
    ctx->body->setRings(RingSystem(innerRadius, outerRadius, color));

    return true;
}


// SAX startDocument callback
static void solarSysStartDocument(void* data)
{
    ParserContext* ctx = reinterpret_cast<ParserContext*>(data);
    ctx->state = StartState;
    ctx->body = NULL;
}


// SAX endDocument callback
static void solarSysEndDocument(void* data)
{
    ParserContext* ctx = reinterpret_cast<ParserContext*>(data);
    ctx->state = EndState;
    ctx->body = NULL;
}


// SAX startElement callback
static void solarSysStartElement(void* data,
                                 const xmlChar* name,
                                 const xmlChar** att)
{
    ParserContext* ctx = reinterpret_cast<ParserContext*>(data);

    switch (ctx->state)
    {
    case ErrorState:
        return;

    case StartState:
        if (matchName(name, "body"))
        {
            createBody(ctx, att);
            ctx->state = BodyState;
        }
        else if (!matchName(name, "catalog"))
        {
            ctx->state = ErrorState;
        }
        break;

    case BodyState:
        if (matchName(name, "surface"))
        {
            createSurface(ctx, att);
            ctx->state = SurfaceState;
        }
        else if (matchName(name, "geometry"))
        {
            createGeometry(ctx, att);
            ctx->state = BodyLeafState;
        }
        else if (matchName(name, "elliptical"))
        {
            createEllipticalOrbit(ctx, att);
            ctx->state = BodyLeafState;
        }
        else if (matchName(name, "customorbit"))
        {
            createCustomOrbit(ctx, att);
            ctx->state = BodyLeafState;
        }
        else if (matchName(name, "rotation"))
        {
            createRotation(ctx, att);
            ctx->state = BodyLeafState;
        }
        else if (matchName(name, "atmosphere"))
        {
            createAtmosphere(ctx, att);
            ctx->state = AtmosphereState;
        }
        else if (matchName(name, "rings"))
        {
            createRings(ctx, att);
            ctx->state = RingsState;
        }
        else
        {
            ctx->state = ErrorState;
        }
        break;

    case SurfaceState:
        if (matchName(name, "texture"))
        {
            createTexture(ctx, att);
            ctx->state = SurfaceLeafState;
        }
        else if (matchName(name, "bumpmap"))
        {
            createBumpMap(ctx, att);
            ctx->state = SurfaceLeafState;
        }
        else if (matchName(name, "haze"))
        {
            createHaze(ctx, att);
            ctx->state = SurfaceLeafState;
        }
        else
        {
            ctx->state = ErrorState;
        }
        break;

    case RingsState:
        if (matchName(name, "texture"))
        {
            createTexture(ctx, att);
            ctx->state = RingsLeafState;
        }
        break;

    case AtmosphereState:
        if (matchName(name, "texture"))
        {
            createTexture(ctx, att);
            ctx->state = AtmosphereLeafState;
        }
        break;

    case BodyLeafState:
    case SurfaceLeafState:
    case AtmosphereLeafState:
    case RingsLeafState:
        ctx->state = ErrorState;
        break;

    default:
        break;
    }

    if (ctx->state == ErrorState)
    {
        cout << "Error!  " << name << " element not expected.\n";
    }
}


// SAX endElement callback
static void solarSysEndElement(void* data, const xmlChar* name)
{
    ParserContext* ctx = reinterpret_cast<ParserContext*>(data);
    switch (ctx->state)
    {
    case ErrorState:
        return;

    case BodyState:
        if (matchName(name, "body"))
        {
            assert(ctx->body != NULL);
            if (ctx->body->getOrbit() == NULL)
            {
                DPRINTF(0, "Object %s has no orbit!  Removing . . .\n",
                        ctx->body->getName().c_str());
                
            }
            ctx->body = NULL;
            ctx->state = StartState;
        }
        else
        {
            ctx->state = ErrorState;
        }
        break;

    case SurfaceState:
        if (matchName(name, "surface"))
            ctx->state = BodyState;
        else
            ctx->state = ErrorState;
        break;

    case AtmosphereState:
        if (matchName(name, "atmosphere"))
            ctx->state = BodyState;
        else
            ctx->state = ErrorState;
        break;

    case RingsState:
        if (matchName(name, "rings"))
            ctx->state = BodyState;
        else
            ctx->state = ErrorState;
        break;

    case BodyLeafState:
        if (matchName(name, "geometry") ||
            matchName(name, "elliptical") ||
            matchName(name, "customorbit") ||
            matchName(name, "rotation"))
        {
            ctx->state = BodyState;
        }
        break;

    case SurfaceLeafState:
        if (matchName(name, "texture") ||
            matchName(name, "haze") ||
            matchName(name, "bumpmap"))
        {
            ctx->state = SurfaceState;
        }
        break;

    case AtmosphereLeafState:
        if (matchName(name, "texture"))
            ctx->state = AtmosphereState;
        break;

    case RingsLeafState:
        if (matchName(name, "texture"))
            ctx->state = RingsState;
        break;

    default:
        break;
    }

    if (ctx->state == ErrorState)
    {
        cout << "Error!  End of " << name << " element not expected.\n";
    }
}


static void initSAXHandler(xmlSAXHandler& sax)
{
    sax = emptySAXHandler;
    sax.startDocument = solarSysStartDocument;
    sax.endDocument = solarSysEndDocument;
    sax.startElement = solarSysStartElement;
    sax.endElement = solarSysEndElement;
}


static bool parseSolarSystemXML(xmlSAXHandlerPtr sax,
                                void* userData,
                                const char* filename)
{
    xmlParserCtxtPtr ctxt = xmlCreateFileParserCtxt(filename);
    if (ctxt == NULL)
        return false;

    ctxt->sax = sax;
    ctxt->userData = userData;

    xmlParseDocument(ctxt);

    int wellFormed = ctxt->wellFormed;
    if (sax != NULL)
        ctxt->sax = NULL;
    xmlFreeParserCtxt(ctxt);

    cout << "Well formed: " << wellFormed << '\n';

    return wellFormed != 0;
}


bool LoadSolarSystemObjectsXML(const string& source, Universe& universe)
{
    initSAXHandler(saxHandler);

    ParserContext ctx;
    ctx.universe = &universe;

    if (!parseSolarSystemXML(&saxHandler, &ctx, source.c_str()))
    {
        cout << "Error parsing " << source << '\n';
        return false;
    }

    return true;
}
