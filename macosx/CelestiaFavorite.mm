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

NSInvocation *_synchronize;

@implementation CelestiaFavorite(PrivateAPI)

-(CelestiaFavorite *)initWithFavorite:(FavoritesEntry *)fav 
{
    self = [super init];
    _data = [[NSValue alloc] initWithBytes:reinterpret_cast<void*>(&fav) objCType:@encode(FavoritesEntry*)];
    return self;
}
-(FavoritesEntry *)favorite
{
    return reinterpret_cast<FavoritesEntry*>([_data pointerValue]);
}
@end

@implementation CelestiaFavorites(PrivateAPI)
-(CelestiaFavorites *)initWithFavorites:(const FavoritesList *)dsts 
{
    self = [super init];
    _data = [[NSValue alloc] initWithBytes:reinterpret_cast<void*>(&dsts) objCType:@encode(FavoritesList*)];
    _retain = [[NSMutableArray arrayWithCapacity:[self count]] retain];
    return self;
}
-(FavoritesList *)favorites
{
    return reinterpret_cast<FavoritesList*>([_data pointerValue]);
}
@end

@implementation CelestiaFavorite
-(unsigned)hash
{
    return (unsigned)[_data pointerValue];
}
-(void)dealloc
{
    if (_data != nil) {
        [_data release];
        _data = nil;
    }
    [super dealloc];
}
-(BOOL)isEqualToFavorite:(CelestiaFavorite*)fav
{
    /*
     return (
        [[self name] isEqualToString:[fav name]] &&
        [[self parentFolder] isEqualToString:[fav parentFolder]] &&
        [[self selectionName] isEqualToString:[fav selectionName]] &&
        ([self isFolder] == [fav isFolder]) &&
        [[self jd] isEqualToNumber:[fav jd]]
    );
    */
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

@implementation CelestiaFavorites
+(void)initialize
{
    _synchronize = nil;
}

-(unsigned)depthAtIndex:(unsigned)idx
{
    unsigned i,d;
    NSString *lastName=@"";
    d=0;
    for(i=0;i<=idx;++i) {
        CelestiaFavorite* fav = [self objectAtIndex:i];
        if ([[fav parentFolder] isEqualToString:@""]) {
            d=0;
        } else if ([[fav parentFolder] isEqualToString:lastName] && [lastName length]>0) {
            ++d;
        } else {
            --d;
        }
        lastName = [fav name];
    }
    return d;
}
-(id)objectAtIndex:(unsigned)index parent:(CelestiaFavorite*)parent
{
    /* FIXME, needs to step over children folders */
    unsigned idx = 0;
    if (parent != nil) {
        idx = [self firstIndexWithParentFolder:parent];
        if (idx == NSNotFound) return nil;
    }
    return [self objectAtIndex:(index+idx)];
}
-(unsigned)firstIndexWithParentFolder:(CelestiaFavorite*)folder
{
    unsigned begin;
    NSString *folderName;
    if (folder == nil) return 0;
    folderName = [folder name];
    begin = [self indexOfObjectIdenticalTo:folder];
    if (begin == NSNotFound)
        return NSNotFound;
    return ++begin;
}
-(unsigned)lastIndexWithParentFolder:(CelestiaFavorite*)folder
{
    /* FIXME, needs to step over children folders */
    unsigned begin,end,i;
    NSString *folderName;
    if (folder == nil) 
        return [self count];
    folderName = [folder name];
    begin = [self indexOfObjectIdenticalTo:folder];
    if (begin == NSNotFound)
        return NSNotFound;
    end = [self count];
    for (i=++begin;i<end;++i) {
        if (![folderName isEqualToString:[[self objectAtIndex:i] parentFolder]]) {
            break;
        }
    }
    return i;
}
-(unsigned)numberOfChildrenOfItem:(CelestiaFavorite*)folder
{
    /* FIXME, needs to step over children folders */
    unsigned first,last;
    if (folder == nil) 
        return [self count];
    if (![folder isFolder]) 
        return 0;
    first = [self firstIndexWithParentFolder:folder];
    last = [self lastIndexWithParentFolder:folder];
    if ((last == NSNotFound) || (first == NSNotFound)) 
        return 0;
    return last-first;
}
-(void)addNewFavorite:(NSString*)name withParentFolder:(CelestiaFavorite*)folder atIndex:(unsigned)idx
{
    FavoritesEntry* fav = new FavoritesEntry();
    CelestiaFavorite* cfav = [[[CelestiaFavorite alloc] initWithFavorite:fav] autorelease];
    Simulation* sim = [[[CelestiaAppCore sharedAppCore] simulation] simulation];
    NSString *parentFolder = (folder == nil) ? @"" : [folder name];
    fav->jd = sim->getTime();
    fav->position = sim->getObserver().getPosition();
    fav->orientation = sim->getObserver().getOrientation();
    fav->name = [name stdString];
    fav->isFolder = false;
    fav->parentFolder = [parentFolder stdString];
    fav->selectionName = sim->getSelection().getName();
    fav->coordSys = sim->getFrame().coordSys;
    [self insertObject:cfav atIndex:idx];
}

-(void)addNewFavorite:(NSString*)name withParentFolder:(CelestiaFavorite*)folder
{
    unsigned idx;
    FavoritesEntry* fav = new FavoritesEntry();
    CelestiaFavorite* cfav = [[[CelestiaFavorite alloc] initWithFavorite:fav] autorelease];
    Simulation* sim = [[[CelestiaAppCore sharedAppCore] simulation] simulation];
    NSString *parentFolder = (folder == nil) ? @"" : [folder name];
    if (name == nil)
        name = [[[[CelestiaAppCore sharedAppCore] simulation] selection] name];
    if (name == nil)
        name = [[[[CelestiaAppCore sharedAppCore] simulation] julianDate] description];
    NSLog(@"[CelestiaFavorites addNewFavorite:%@ inFolder:%@]",name,folder);
    fav->jd = sim->getTime();
    fav->position = sim->getObserver().getPosition();
    fav->orientation = sim->getObserver().getOrientation();
    fav->name = [name stdString];
    fav->isFolder = false;
    fav->parentFolder = [parentFolder stdString];
    fav->selectionName = sim->getSelection().getName();
    fav->coordSys = sim->getFrame().coordSys;
    idx = [self lastIndexWithParentFolder:folder];
    [self insertObject:cfav atIndex:((idx==NSNotFound)?[self count]:idx)];
}
-(void)addNewFolder:(NSString*)name withParentFolder:(CelestiaFavorite*)folder atIndex:(unsigned)idx
{
    FavoritesEntry* fav = new FavoritesEntry();
    CelestiaFavorite* cfav = [[[CelestiaFavorite alloc] initWithFavorite:fav] autorelease];
    NSString *parentFolder = ((folder == nil) ? @"" : [folder name]);
    fav->parentFolder = [parentFolder stdString];
    fav->isFolder = true;
    fav->name = [name stdString];
    [self insertObject:cfav atIndex:idx];
}
-(void)addNewFolder:(NSString*)name withParentFolder:(CelestiaFavorite*)folder
{
    unsigned idx;
    FavoritesEntry* fav = new FavoritesEntry();
    CelestiaFavorite* cfav = [[[CelestiaFavorite alloc] initWithFavorite:fav] autorelease];
    NSString *parentFolder = (folder == nil) ? @"" : [folder name];
    fav->parentFolder = [parentFolder stdString];
    fav->isFolder = true;
    fav->name = [name stdString];
    idx = [self firstIndexWithParentFolder:folder];
    [self insertObject:cfav atIndex:idx];
}
-(unsigned)count
{
    if (![self favorites]) 
        return 0;
    return [self favorites]->size();
}
-(id)objectAtIndex:(unsigned)index
{
    CelestiaFavorite* orig = [[[CelestiaFavorite alloc] initWithFavorite:(*[self favorites])[index]] autorelease];
    unsigned idx = [_retain indexOfObject:orig];
    if (idx == NSNotFound) {
        [_retain addObject:orig];
        return orig;
    }
    return [_retain objectAtIndex:idx];
}
-(void)addObject:(CelestiaFavorite*)o
{
    [_retain addObject:o];
    [self favorites]->push_back([o favorite]);
    [self synchronize];
}
-(void)insertObject:(CelestiaFavorite*)o atIndex:(unsigned)index
{
    [_retain addObject:o];
    [self favorites]->insert([self favorites]->begin()+index, [o favorite]);
    [self synchronize];
}
-(void)removeLastObject
{
    [_retain removeObject:[self lastObject]];
    [self favorites]->pop_back();
    [self synchronize];
}
-(void)removeObjectAtIndex:(unsigned)index
{
    [_retain removeObject:[self objectAtIndex:index]];
    [self favorites]->erase([self favorites]->begin()+index);
    [self synchronize];
}
-(void)replaceObjectAtIndex:(unsigned)index withObject:(CelestiaFavorite*)o
{
    [self removeObjectAtIndex:index];
    [self insertObject:o atIndex:index];
}
-(void)setSynchronize:(NSInvocation*)sync
{
    if (_synchronize != nil) {
        [_synchronize release];
        _synchronize = nil;
    }
    _synchronize = [sync retain];
    [_synchronize setArgument:&self atIndex:2];
}
-(void)synchronize
{
    NSLog(@"[CelestiaFavorites synchronize]");
    //NSLog(@"%@",self);
    if (_synchronize == nil)
        return;
    [_synchronize invoke];
}
-(void)dealloc
{
    if (_data != nil) {
        [_data release];
        _data = nil;
    }
    if (_retain != nil) {
        [_retain release];
        _retain = nil;
    }
    [super dealloc];
}
@end