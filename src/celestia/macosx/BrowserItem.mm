//
//  BrowserItem.mm
//  celestia
//
//  Created by Da Woon Jung on 2007-11-26
//  Copyright (C) 2007, Celestia Development Team
//

#import "BrowserItem.h"
#import "CelestiaStar.h"
#import "CelestiaStar_PrivateAPI.h"
#import "CelestiaBody.h"
#import "CelestiaBody_PrivateAPI.h"
#import "CelestiaDSO.h"
#import "CelestiaLocation.h"
#import "CelestiaAppCore.h"
#import "CelestiaUniverse_PrivateAPI.h"
#include "solarsys.h"


@implementation BrowserItem
- (id)initWithCelestiaDSO: (CelestiaDSO *)aDSO
{
    self = [super init];
    if (self) data = [aDSO retain];
    return self;
}
- (id)initWithCelestiaStar: (CelestiaStar *)aStar
{
    self = [super init];
    if (self) data = [aStar retain];
    return self;
}
- (id)initWithCelestiaBody: (CelestiaBody *)aBody
{
    self = [super init];
    if (self) data = [aBody retain];
    return self;
}
- (id)initWithCelestiaLocation: (CelestiaLocation *)aLocation
{
    self = [super init];
    if (self) data = [aLocation retain];
    return self;
}
- (id)initWithName: (NSString *)aName
{
    self = [super init];
    if (self) data = [aName retain];
    return self;
}
- (id)initWithName: (NSString *)aName
          children: (NSDictionary *)aChildren
{
    self = [super init];
    if (self)
    {
        data = [aName retain];
        if (nil == children)
        {
            children = [[NSMutableDictionary alloc] initWithDictionary: aChildren];
            childrenChanged = YES;
        }
    }
    return self;
}

+ (void)addChildrenToStar: (BrowserItem *) aStar
{
    SolarSystem *ss = [[[[CelestiaAppCore sharedAppCore] simulation] universe] universe]->getSolarSystem([((CelestiaStar *)[aStar body]) star]);
    PlanetarySystem* sys = NULL;
    if (ss) sys = ss->getPlanets();
    
    if (sys)
    {
        int sysSize = sys->getSystemSize();
        BrowserItem *subItem = nil;
        BrowserItem *planets = nil;
        BrowserItem *dwarfPlanets = nil;
        BrowserItem *minorMoons = nil;
        BrowserItem *asteroids = nil;
        BrowserItem *comets = nil;
        BrowserItem *spacecrafts = nil;
        int i;
        for ( i=0; i<sysSize; i++)
        {
            Body* body = sys->getBody(i);
            if (body->getName().empty())
                continue;
            BrowserItem *item = [[[BrowserItem alloc] initWithCelestiaBody:
                [[[CelestiaBody alloc] initWithBody: body] autorelease]] autorelease];
            int bodyClass  = body->getClassification();
            switch (bodyClass)
            {
                case Body::Invisible:
                    continue;
                case Body::Planet:
                    if (!planets)
                        planets = [[[BrowserItem alloc] initWithName: NSLocalizedStringFromTable(@"Planets",@"po",@"")] autorelease];
                    subItem = planets;
                    break;
                case Body::DwarfPlanet:
                    if (!dwarfPlanets)
                        dwarfPlanets = [[[BrowserItem alloc] initWithName: NSLocalizedStringFromTable(@"Dwarf Planets",@"po",@"")] autorelease];
                    subItem = dwarfPlanets;
                    break;
                case Body::Moon:
                case Body::MinorMoon:
                    if (body->getRadius() < 100.0f || Body::MinorMoon == bodyClass)
                    {
                        if (!minorMoons)
                            minorMoons = [[[BrowserItem alloc] initWithName: NSLocalizedString(@"Minor Moons",@"")] autorelease];
                        subItem = minorMoons;
                    }
                    else
                    {
                        subItem = aStar;
                    }
                    break;
                case Body::Asteroid:
                    if (!asteroids)
                        asteroids = [[[BrowserItem alloc] initWithName: NSLocalizedStringFromTable(@"Asteroids",@"po",@"")] autorelease];
                    subItem = asteroids;
                    break;
                case Body::Comet:
                    if (!comets)
                        comets = [[[BrowserItem alloc] initWithName: NSLocalizedStringFromTable(@"Comets",@"po",@"")] autorelease];
                    subItem = comets;
                    break;
                case Body::Spacecraft:
                    if (!spacecrafts)
                        spacecrafts = [[[BrowserItem alloc] initWithName: NSLocalizedString(@"Spacecrafts",@"")] autorelease];
                    subItem = spacecrafts;
                    break;
                default:
                    subItem = aStar;
                    break;
            }
            [subItem addChild: item];
        }
        
        if (planets)      [aStar addChild: planets];
        if (dwarfPlanets) [aStar addChild: dwarfPlanets];
        if (minorMoons)   [aStar addChild: minorMoons];
        if (asteroids)    [aStar addChild: asteroids];
        if (comets)       [aStar addChild: comets];
        if (spacecrafts)  [aStar addChild: spacecrafts];
    }
}

+ (void)addChildrenToBody: (BrowserItem *) aBody
{
    PlanetarySystem* sys = [(CelestiaBody *)[aBody body] body]->getSatellites();
    
    if (sys)
    { 
        int sysSize = sys->getSystemSize();
        BrowserItem *subItem = nil;
        BrowserItem *minorMoons = nil;
        BrowserItem *comets = nil;
        BrowserItem *spacecrafts = nil;
        int i;
        for ( i=0; i<sysSize; i++)
        {
            Body* body = sys->getBody(i);
            if (body->getName().empty())
                continue;
            BrowserItem *item = [[[BrowserItem alloc] initWithCelestiaBody:
                [[[CelestiaBody alloc] initWithBody: body] autorelease]] autorelease];
            int bodyClass  = body->getClassification();
            if (bodyClass==Body::Asteroid) bodyClass = Body::Moon;
            
            switch (bodyClass)
            {
                case Body::Invisible:
                    continue;
                case Body::Moon:
                case Body::MinorMoon:
                    if (body->getRadius() < 100.0f || Body::MinorMoon == bodyClass)
                    {
                        if (!minorMoons)
                            minorMoons = [[[BrowserItem alloc] initWithName: NSLocalizedString(@"Minor Moons",@"")] autorelease];
                        subItem = minorMoons;
                    }
                    else
                    {
                        subItem = aBody;
                    }
                    break;
                case Body::Comet:
                    if (!comets)
                        comets = [[[BrowserItem alloc] initWithName: NSLocalizedStringFromTable(@"Comets",@"po",@"")] autorelease];
                    subItem = comets;
                    break;
                case Body::Spacecraft:
                    if (!spacecrafts)
                        spacecrafts = [[[BrowserItem alloc] initWithName: NSLocalizedString(@"Spacecrafts",@"")] autorelease];
                    subItem = spacecrafts;
                    break;
                default:
                    subItem = aBody;
                    break;
            }
            [subItem addChild: item];
        }
        
        if (minorMoons)  [aBody addChild: minorMoons];
        if (comets)      [aBody addChild: comets];
        if (spacecrafts) [aBody addChild: spacecrafts];
    }
    
    std::vector<Location*>* locations = [(CelestiaBody *)[aBody body] body]->getLocations();
    if (locations != NULL)
    {
        BrowserItem *locationItems = [[[BrowserItem alloc] initWithName: NSLocalizedStringFromTable(@"Locations",@"po",@"")] autorelease];
        for (vector<Location*>::const_iterator iter = locations->begin();
             iter != locations->end(); iter++)
        {
            [locationItems addChild: [[[BrowserItem alloc] initWithCelestiaLocation: [[[CelestiaLocation alloc] initWithLocation: *iter] autorelease]] autorelease]];
        }
        [aBody addChild: locationItems];
    }
}

- (void)dealloc
{
    [childNames release];
    [children release];
    [data release];
    [super dealloc];
}

- (NSString *)name
{
    return ([data respondsToSelector:@selector(name)]) ? [data name] : data;
}

- (id)body
{
    return ([data isKindOfClass: [NSString class]]) ? nil : data;
}

- (void)addChild: (BrowserItem *)aChild
{
    if (nil == children)
        children = [[NSMutableDictionary alloc] init];
    
    [children setObject: aChild forKey: [aChild name]];
    childrenChanged = YES;
}

- (id)childNamed: (NSString *)aName
{
    return [children objectForKey: aName];
}

- (NSArray *)allChildNames
{
    if (childrenChanged)
    {
        [childNames release]; childNames = nil;
    }
    
    if (nil == childNames)
    {
        childNames = [[[children allKeys] sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)] retain];
        childrenChanged = NO;
    }
    
    return childNames;
}

- (unsigned)childCount
{
    return [children count];
}
@end
