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
#import "CelestiaAppCore.h"
#import "CelestiaSimulation.h"
#import "CelestiaSimulation_PrivateAPI.h"

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
@end

@implementation CelestiaFavorite
-(void)activate
{
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
        name = @"untitled folder";
    fav->name = [name stdString];
    fav->isFolder = true;
    fav->parentFolder = [parentFolder stdString];
    self = [self initWithFavorite:fav];
    _freeWhenDone = YES;
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
    fav->orientation = sim->getObserver().getOrientation();
    fav->name = [name stdString];
    fav->isFolder = false;
    fav->parentFolder = [parentFolder stdString];
    fav->selectionName = sim->getSelection().getName();
    fav->coordSys = sim->getFrame().coordSys;
    self = [self initWithFavorite:fav];
    _freeWhenDone = YES;
    return self;
}
-(unsigned)hash
{
    return (unsigned)[_data pointerValue];
}
-(void)dealloc
{
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
-(NSDictionary*)dictionaryRepresentation
{
    if ([self isFolder])
        return [NSDictionary dictionaryWithObjectsAndKeys:[self name],@"name",@"YES",@"isFolder",nil,nil];
    return [NSDictionary dictionaryWithObjectsAndKeys:[self name],@"name",[self parentFolder],@"parentFolder",[self selectionName],@"selectionName",[self position],@"position",[self orientation],@"orientation",[self jd],@"jd",@"NO",@"isFolder",[self coordinateSystem],@"coordinateSystem",nil,nil];
}
-(NSString*)description
{
    return [[self dictionaryRepresentation] description];
}
-(void)setName:(NSString*)name
{
    [self favorite]->name = [name stdString];
}
-(void)setParentFolder:(NSString*)parentFolder
{
    [self favorite]->parentFolder = [parentFolder stdString];
}
-(NSString*)name
{
    return [NSString stringWithStdString:[self favorite]->name];
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
    return [CelestiaVector vectorWithQuatf:[self favorite]->orientation];
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
@end