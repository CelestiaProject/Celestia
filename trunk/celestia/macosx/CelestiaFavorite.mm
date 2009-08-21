//
//  CelestiaFavorite.mm
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaFavorite.h"
#import "CelestiaFavorite_PrivateAPI.h"
#import "CelestiaVector_PrivateAPI.h"
#import "CelestiaUniversalCoord_PrivateAPI.h"
#import "NSString_ObjCPlusPlus.h"
#import "Astro.h"
#import "Observer.h"
#import "CelestiaAppCore.h"
#import "CelestiaSimulation.h"
#import "CelestiaSimulation_PrivateAPI.h"

#include <celengine/eigenport.h>

#ifdef URL_FAVORITES
#include "url.h"
#endif

@implementation CelestiaFavorite(PrivateAPI)
-(CelestiaFavorite *)initWithFavorite:(FavoritesEntry *)fav 
{
    self = [super init];
    _data = [[NSValue alloc] initWithBytes:reinterpret_cast<void*>(&fav) objCType:@encode(FavoritesEntry*)];
    _freeWhenDone = NO;
    return self;
}
-(FavoritesEntry *)favorite
{
    return reinterpret_cast<FavoritesEntry*>([_data pointerValue]);
}
-(void)setSelectionName:(NSString*)name
{
    if (name == nil)
        name = @"";
    [self favorite]->selectionName = [name stdString];
}
-(void)setPosition:(CelestiaUniversalCoord*)position
{
    [self favorite]->position = [position universalCoord];
}
-(void)setOrientation:(CelestiaVector*)orientation
{
    [self favorite]->orientation = [orientation quaternionf];
}
-(void)setJd:(NSNumber*)jd
{
    [self favorite]->jd = [jd doubleValue];
}
-(void)setCoordinateSystem:(NSString*)coordSys
{
    [self favorite]->coordSys = (ObserverFrame::CoordinateSystem)[[Astro coordinateSystem:coordSys] intValue];
}
@end

@implementation CelestiaFavorite
-(void)encodeWithCoder:(NSCoder*)coder
{
    //[super encodeWithCoder:coder];
    [coder encodeObject:[NSNumber numberWithBool:[self isFolder]]];
    [coder encodeObject:[self name]];
    [coder encodeObject:[self parentFolder]];
    if ([self isFolder])
        return;
    [coder encodeObject:[self selectionName]];
    [coder encodeObject:[self position]];
    [coder encodeObject:[self orientation]];
    [coder encodeObject:[self jd]];
    [coder encodeObject:[self coordinateSystem]];
}
-(id)initWithCoder:(NSCoder*)coder
{
    FavoritesEntry* fav = new FavoritesEntry();
    //self = [super initWithCoder:coder];
    self = [self init];
    _data = [[NSValue alloc] initWithBytes:reinterpret_cast<void*>(&fav) objCType:@encode(FavoritesEntry*)];
    _freeWhenDone = YES;
    fav->isFolder = (bool)[[coder decodeObject] boolValue];
    [self setName:[coder decodeObject]];
    [self setParentFolder:[coder decodeObject]];
    if ([self isFolder])
        return self;
    [self setSelectionName:[coder decodeObject]];
    [self setPosition:[coder decodeObject]];
    [self setOrientation:[coder decodeObject]];
    [self setJd:[coder decodeObject]];
    [self setCoordinateSystem:[coder decodeObject]];
    return self;
}
-(void)activate
{
#ifdef URL_FAVORITES
    if (url)
        [[CelestiaAppCore sharedAppCore] goToUrl: url];
    else
#endif
        [[CelestiaAppCore sharedAppCore] activateFavorite:self];
}
-(id)initWithName:(NSString*)name
{
    return [self initWithName:name parentFolder:nil];
}
-(id)initWithFolderName:(NSString*)name
{
    return [self initWithFolderName:name parentFolder:nil];
}
-(id)initWithFolderName:(NSString*)name parentFolder:(CelestiaFavorite*)folder
{
    FavoritesEntry* fav = new FavoritesEntry();
    NSString *parentFolder = (folder == nil) ? @"" : [folder name];
    if (name == nil || [name isEqualToString:@""])
        name = NSLocalizedString(@"untitled folder",@"");
    fav->name = [name stdString];
    fav->isFolder = true;
    fav->parentFolder = [parentFolder stdString];
    self = [self initWithFavorite:fav];
    _freeWhenDone = YES;
#ifdef URL_FAVORITES
    [self setName: name];
#endif
    return self;
}
-(id)initWithName:(NSString*)name parentFolder:(CelestiaFavorite*)folder
{
    FavoritesEntry* fav = new FavoritesEntry();
    Simulation* sim = [[[CelestiaAppCore sharedAppCore] simulation] simulation];
    NSString *parentFolder = (folder == nil) ? @"" : [folder name];
    if (name == nil)
        name = [[[[CelestiaAppCore sharedAppCore] simulation] selection] name];
    if (name == nil)
        name = [[[[CelestiaAppCore sharedAppCore] simulation] julianDate] description];
    fav->jd = sim->getTime();
    fav->position = sim->getObserver().getPosition();
    fav->orientation = sim->getObserver().getOrientationf();
    fav->name = [name stdString];
    fav->isFolder = false;
    fav->parentFolder = [parentFolder stdString];

    Selection sel = sim->getSelection();
    if (sel.deepsky() != NULL)
        fav->selectionName = sim->getUniverse()->getDSOCatalog()->getDSOName(sel.deepsky());
    else
        fav->selectionName = sel.getName();

    fav->coordSys = sim->getFrame()->getCoordinateSystem();
    self = [self initWithFavorite:fav];
    _freeWhenDone = YES;
#ifdef URL_FAVORITES
    Url currentUrl = Url([[CelestiaAppCore sharedAppCore] appCore], Url::Absolute);
    [self setUrl:  [NSString stringWithStdString: currentUrl.getAsString()]];
    [self setName: [NSString stringWithStdString: currentUrl.getName()]];
    if (_name == nil)
        [self setName: [[[[CelestiaAppCore sharedAppCore] simulation] selection] name]];
    if (_name == nil)
        [self setName: [[[[CelestiaAppCore sharedAppCore] simulation] julianDate] description]];
#endif
    return self;
}
-(unsigned)hash
{
#ifdef URL_FAVORITES
    if (url)
        return [url hash];
#endif
    return (unsigned)[_data pointerValue];
}
-(void)dealloc
{
#ifdef URL_FAVORITES
    [url release];   url = nil;
    [_name release]; _name = nil;
#endif
    if (_data != nil) {
        if (_freeWhenDone) {
            FavoritesEntry* fav = [self favorite];
            delete fav;
        }
        [_data release];
        _data = nil;
    }
    [super dealloc];
}
-(BOOL)isEqualToFavorite:(CelestiaFavorite*)fav
{
    // pointer equality is enough in this case
    return ([self favorite] == [fav favorite]);
}
-(BOOL)isEqualToString:(NSString*)str
{
    return ([[self name] isEqualToString:str]);
}
-(BOOL)isEqual:(id)obj
{
    if ([obj isKindOfClass:[NSString class]]) {
        return [self isEqualToString:(NSString*)obj];
    } else if ([obj isKindOfClass:[self class]]) {
        return [self isEqualToFavorite:(CelestiaFavorite*)obj];
    }
    return NO;
}
-(NSDictionary*)dictionary
{
    if ([self isFolder])
        return [NSDictionary dictionaryWithObjectsAndKeys:[self name],@"name",[NSNumber numberWithBool:YES],@"isFolder",nil,nil];
    return [NSDictionary dictionaryWithObjectsAndKeys:
        [self name], @"name",
        [self parentFolder], @"parentFolder",
        [self selectionName], @"selectionName",
        [NSArray arrayWithArray:[self orientation]], @"orientation",
        [[self position] data], @"position",
        [self jd], @"jd",
        [NSNumber numberWithBool:NO], @"isFolder",
        [self coordinateSystem], @"coordinateSystem",
#ifdef URL_FAVORITES
        url, @"url",
#endif
        nil];
}
-(id)initWithDictionary:(NSDictionary*)dict
{
    FavoritesEntry* fav;
    if ([[dict objectForKey:@"isFolder"] boolValue]) {
        self = [self initWithFolderName:[dict objectForKey:@"name"]];
        [self setParentFolder:[dict objectForKey:@"parentFolder"]];
        return self;
    }
    fav = new FavoritesEntry();
    self = [self init];
    _data = [[NSValue alloc] initWithBytes:reinterpret_cast<void*>(&fav) objCType:@encode(FavoritesEntry*)];
    _freeWhenDone = YES;
    fav->isFolder = false;
    [self setName:[dict objectForKey:@"name"]];
    [self setParentFolder:[dict objectForKey:@"parentFolder"]];
    [self setSelectionName:[dict objectForKey:@"selectionName"]];
    [self setOrientation:[CelestiaVector vectorWithArray:[dict objectForKey:@"orientation"]]];
    [self setPosition:[[[CelestiaUniversalCoord alloc] initWithData:[dict objectForKey:@"position"]] autorelease]];
    [self setJd:[dict objectForKey:@"jd"]];
    [self setCoordinateSystem:[dict objectForKey:@"coordinateSystem"]];
#ifdef URL_FAVORITES
    [self setUrl: [dict objectForKey: @"url"]];
#endif
    return self;
}
-(NSString*)description
{
    return [[self dictionary] description];
}
-(void)setName:(NSString*)name
{
    if (name == nil)
        name = @"";
#ifdef URL_FAVORITES
    [_name release];
    _name = [name retain];
#else
    [self favorite]->name = [name stdString];
#endif
}
-(void)setParentFolder:(NSString*)parentFolder
{
    if (parentFolder == nil)
        parentFolder = @"";
    [self favorite]->parentFolder = [parentFolder stdString];
}
-(NSString*)name
{
#ifdef URL_FAVORITES
    return [[_name retain] autorelease];
#else
    return [NSString stringWithStdString:[self favorite]->name];
#endif
}
-(NSString*)selectionName
{
    return [NSString stringWithStdString:[self favorite]->selectionName];
}
-(CelestiaUniversalCoord*)position
{
    return [[[CelestiaUniversalCoord alloc] initWithUniversalCoord:[self favorite]->position] autorelease];
}
-(CelestiaVector*)orientation
{
    return [CelestiaVector vectorWithQuaternionf:[self favorite]->orientation];
}
-(NSNumber*)jd
{
    return [NSNumber numberWithDouble:[self favorite]->jd];
}
-(BOOL)isFolder
{
    return [self favorite]->isFolder;
}
-(NSString*)parentFolder
{
    return [NSString stringWithStdString:[self favorite]->parentFolder];
}
-(NSString*)coordinateSystem
{
    return [Astro stringWithCoordinateSystem:[NSNumber numberWithInt:(int)[self favorite]->coordSys]];
}
#ifdef URL_FAVORITES
-(NSString*)url
{
    return url;
}
-(void)setUrl:(NSString *)aUrl
{
    url = [aUrl retain];
}
#endif
@end
