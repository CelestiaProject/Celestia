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
#import "NSString_ObjCPlusPlus.h"
#import "CelestiaUniversalCoord_PrivateAPI.h"

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
-(CelestiaBody *)body
{
    if ([self selection].body == NULL) return nil;
    return [[[CelestiaBody alloc] initWithBody:[self selection].body] autorelease];
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
-(void)select:(id)inst
{
        if ([inst isKindOfClass:[CelestiaBody class]]) {
            [self selection].select((Body*)[(CelestiaBody*)inst body]);
        }
        else if ([inst isKindOfClass:[CelestiaStar class]]) {
            [self selection].select((Star *)[(CelestiaStar*)inst star]);
	}
        else if ([inst isKindOfClass:[CelestiaGalaxy class]]) {
            [self selection].select((Galaxy*)[(CelestiaGalaxy*)inst galaxy]);
	}
}
-(CelestiaStar*)star
{
    if ([self selection].star==NULL) return nil;
    return [[[CelestiaStar alloc] initWithStar:[self selection].star] autorelease];
}
-(CelestiaGalaxy*)galaxy
{
    if ([self selection].galaxy==NULL) return nil;
    return [[[CelestiaGalaxy alloc] initWithGalaxy:[self selection].galaxy] autorelease];
}
-(NSString *)name
{
    return [NSString stringWithStdString:[self selection].getName()];
}
-(CelestiaUniversalCoord*)position:(NSNumber*)t
{
    return [[[CelestiaUniversalCoord alloc] initWithUniversalCoord:[self selection].getPosition([t doubleValue])] autorelease];
}
@end
