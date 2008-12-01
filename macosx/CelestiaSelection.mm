//
//  CelestiaSelection.mm
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaSelection.h"
#import "CelestiaSelection_PrivateAPI.h"
#import "CelestiaBody_PrivateAPI.h"
#import "CelestiaGalaxy_PrivateAPI.h"
#import "CelestiaStar_PrivateAPI.h"
#import "CelestiaLocation.h"
#import "NSString_ObjCPlusPlus.h"
#import "CelestiaUniversalCoord_PrivateAPI.h"
#import "CelestiaAppCore.h"
#import "CelestiaUniverse_PrivateAPI.h"


@implementation CelestiaSelection(PrivateAPI)
-(CelestiaSelection *)initWithSelection:(Selection)selection
{
    self=[super init];
    _data = [[NSMutableData alloc] initWithBytes:(void *)&selection length:sizeof(Selection)];
    return self;
}

-(Selection)selection
{
    return *reinterpret_cast<const Selection*>([_data mutableBytes]);
}
@end

@implementation CelestiaSelection

/*
    Selection() : star(NULL), body(NULL), galaxy(NULL) {};
    Selection(Star* _star) : star(_star), body(NULL), galaxy(NULL) {};
    Selection(Body* _body) : star(NULL), body(_body), galaxy(NULL) {};
    Selection(Galaxy* _galaxy) : star(NULL), body(NULL), galaxy(_galaxy) {};
*/

-(void)dealloc
{
    if (_data != nil) {
        [_data release];
        _data = nil;
    }
    [super dealloc];
}

-(CelestiaSelection*)initWithCelestiaStar:(CelestiaStar*)s
{
    return [self initWithSelection:Selection([s star])];
}
-(CelestiaSelection*)initWithCelestiaBody:(CelestiaBody*)b
{
    return [self initWithSelection:Selection([b body])];
}
-(CelestiaSelection*)initWithCelestiaGalaxy:(CelestiaGalaxy*)g
{
    return [self initWithSelection:Selection([g galaxy])];
}
-(CelestiaSelection*)initWithCelestiaLocation:(CelestiaLocation*)g
{
    return [self initWithSelection:Selection([g location])];
}
-(CelestiaBody *)body
{
    if ([self selection].getType() != Selection::Type_Body) return nil;
    return [[[CelestiaBody alloc] initWithBody:[self selection].body()] autorelease];
}
-(BOOL)isEmpty
{
    return (BOOL)([self selection].empty());
}
-(NSNumber*)radius
{
    return [NSNumber numberWithDouble:[self selection].radius()];
}
-(BOOL)isEqualToSelection:(CelestiaSelection*)csel
{
    return (BOOL)([self selection]==[csel selection]);
}

-(CelestiaStar*)star
{
    if ([self selection].getType() != Selection::Type_Star) return nil;
    return [[[CelestiaStar alloc] initWithStar:[self selection].star()] autorelease];
}
-(CelestiaGalaxy*)galaxy
{
    if ([self selection].getType() != Selection::Type_DeepSky) return nil;
    return [[[CelestiaGalaxy alloc] initWithGalaxy:((Galaxy*)[self selection].deepsky())] autorelease];
}
-(CelestiaLocation*)location
{
    if ([self selection].getType() != Selection::Type_Location) return nil;
    return [[[CelestiaLocation alloc] initWithLocation:((Location*)[self selection].location())] autorelease];
}
-(NSString *)name
{
    return [NSString stringWithStdString:[self selection].getName()];
}

-(NSString *) briefName
{
    NSString* name;
    if ([self star] != NULL)
    {
        CelestiaAppCore* appCore = [CelestiaAppCore sharedAppCore];
        Universe* univ = [[[appCore simulation] universe] universe];
        StarDatabase* stardb = univ->getStarCatalog();
        string starname = stardb->getStarName(*[[self star]star]);
        name = [NSString stringWithStdString:starname];
/*
        char buf[20];
        sprintf(buf, "#%d", [[[self star] catalogNumber] intValue]);
//        string tname = string(buf);
        name = [NSString stringWithStdString:string(buf)];
*/
    }
    else if ([ self galaxy] != NULL)
    {
        name = [ [self galaxy] name ];
    }
    else if ([self body] != NULL)
    {
        name = [[self body] name];
    }
    else if ([self location] != NULL)
    {
        name = [[self location] name];
    }
    else
    {
        name = @"";
    }
    return name;
}




-(CelestiaUniversalCoord*)position:(NSNumber*)t
{
    return [[[CelestiaUniversalCoord alloc] initWithUniversalCoord:[self selection].getPosition([t doubleValue])] autorelease];
}

@end
