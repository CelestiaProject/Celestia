//
//  CelestiaDestination.mm
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaDestination.h"
#import "CelestiaDestination_PrivateAPI.h"
#import "NSString_ObjCPlusPlus.h"

@implementation CelestiaDestination(PrivateAPI)
-(CelestiaDestination *)initWithDestination:(Destination *)dst 
{
    self = [super init];
    _data = [[NSValue alloc] initWithBytes:reinterpret_cast<void*>(&dst) objCType:@encode(Destination*)];
    return self;
}

-(Destination *)destination
{
    return reinterpret_cast<Destination*>([_data pointerValue]);
}
@end

@implementation CelestiaDestination
-(void)dealloc
{
    if (_data != nil) {
        [_data release];
        _data = nil;
    }
    [super dealloc];
}
-(NSString*)name
{
    return [NSString stringWithStdString:[self destination]->name];
}
-(void)setName:(NSString*)name
{
    [self destination]->name = [name stdString];
}
-(NSString*)target
{
    return [NSString stringWithStdString:[self destination]->target];
}
-(void)setTarget:(NSString*)target
{
    [self destination]->target = [target stdString];
}
-(NSNumber*)distance
{
    return [NSNumber numberWithDouble:[self destination]->distance];
}
-(void)setDistance:(NSNumber*)distance
{
    [self destination]->distance = [distance doubleValue];
}
-(NSString*)description
{
    return [NSString stringWithStdString:[self destination]->description];
}
-(void)setDescription:(NSString*)description
{
    [self destination]->description = [description stdString];
}
@end

@implementation CelestiaDestinations(PrivateAPI)
-(CelestiaDestinations *)initWithDestinations:(const DestinationList *)dsts 
{
    self = [super init];
    _data = [[NSValue alloc] initWithBytes:reinterpret_cast<void*>(&dsts) objCType:@encode(DestinationList*)];
    return self;
}

-(DestinationList *)destinations
{
    return reinterpret_cast<DestinationList*>([_data pointerValue]);
}
@end

@implementation CelestiaDestinations
-(void)setSynchronize:(NSInvocation*)sync
{
    if (_synchronize == nil) {
        [_synchronize release];
        _synchronize = nil;
    }
    _synchronize = [sync retain];
    [_synchronize setArgument:&self atIndex:2];
}
-(void)synchronize
{
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
    if (_synchronize != nil) {
        [_synchronize release];
        _synchronize = nil;
    }
    [super dealloc];
}
-(unsigned)count
{
    return [self destinations]->size();
}
-(id)objectAtIndex:(unsigned)index
{
    return [[[CelestiaDestination alloc] initWithDestination:(*[self destinations])[index]] autorelease];
}
-(void)addObject:(CelestiaDestination*)o
{
    Destination* d=[o destination];
    [self destinations]->push_back(d);
    [self synchronize];
}
-(void)insertObject:(CelestiaDestination*)o atIndex:(unsigned)index
{
    Destination* d=[o destination];
    [self destinations]->insert([self destinations]->begin()+index, d);
    [self synchronize];
}
-(void)removeLastObject
{
    [self destinations]->pop_back();
    [self synchronize];
}
-(void)removeObjectAtIndex:(unsigned)index
{
    [self destinations]->erase([self destinations]->begin()+index);
    [self synchronize];
}
-(void)replaceObjectAtIndex:(unsigned)index withObject:(CelestiaDestination*)o
{
    [self removeObjectAtIndex:index];
    [self insertObject:o atIndex:index];
}
@end