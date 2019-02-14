//
//  CelestiaFavorites.m
//  celestia
//
//  Created by Bob Ippolito on Thu Jun 20 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//  

#import "CelestiaFavorites.h"
#import "CelestiaFavorite.h"
@implementation CelestiaFavorites
static NSInvocation* _synchronize;
static CelestiaFavorites* _celestiaFavorites;
-(void)archive
{
    NSMutableArray* children = [NSMutableArray arrayWithCapacity:[self numberOfChildren]];
    NSEnumerator* enumerator = [[self children] objectEnumerator];
    id obj = nil;
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    while ((obj = [enumerator nextObject]) != nil)
        [children addObject:[(MyTree*)obj recursiveDictionary]];
    [defaults setObject:children forKey:@"favorites"];
    // NSLog(@"\"favorites\" = %@",[defaults objectForKey:@"favorites"]);
}
-(void)setSynchronize:(NSInvocation*)synchronize
{
    if (_synchronize != nil)
        [_synchronize autorelease];
    _synchronize = [synchronize retain];
}
-(void)synchronize
{
    if (_synchronize != nil)
        [_synchronize invoke];
}
-(NSString*)description
{
    return [NSString stringWithFormat:@"<CelestiaFavorites numberOfChildren=%d>",[self numberOfChildren]];
}
-(MyTree*)addNewFavorite:(NSString*)name
{
    MyTree* obj = [[[MyTree alloc] initWithNode:[[[CelestiaFavorite alloc] initWithName:name] autorelease] parent:self] autorelease];
    [[self children] addObject:obj];
    return obj;
}
-(MyTree*)addNewFolder:(NSString*)name
{
    MyTree* obj = [[[MyTree alloc] initWithNode:[[[CelestiaFavorite alloc] initWithFolderName:name] autorelease] parent:self children:[NSArray array]] autorelease];
    [[self children] addObject:obj];
    return obj;
}
+(void)initialize
{
    _celestiaFavorites = nil;
    _synchronize = nil;
}
+(CelestiaFavorites*)sharedFavorites
{
    if (_celestiaFavorites != nil) {
        return _celestiaFavorites;
    } else {
        NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
        _celestiaFavorites = [[CelestiaFavorites alloc] initWithNode:nil parent:nil children:[NSArray array]];
        if ([defaults objectForKey:@"favorites"] != nil) {
            NSEnumerator *enumerator = [[defaults objectForKey:@"favorites"] objectEnumerator];
            MyVector* children = [_celestiaFavorites children];
            id obj = nil;
            while ((obj = [enumerator nextObject]) != nil)
                [children addObject:[[[MyTree alloc] initWithDictionary:obj parent:_celestiaFavorites] autorelease]];            
        }
        return _celestiaFavorites;
    }
}
@end
