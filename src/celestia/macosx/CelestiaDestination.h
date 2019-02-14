//
//  CelestiaDestination.h
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//


@interface CelestiaDestination : NSObject {
    NSValue* _data;
}
-(NSString*)name;
-(void)setName:(NSString*)name;
-(NSString*)target;
-(void)setTarget:(NSString*)target;
-(NSNumber*)distance;
-(void)setDistance:(NSNumber*)distance;
-(NSString*)description;
-(void)setDescription:(NSString*)description;
@end

@interface CelestiaDestinations : NSMutableArray {
    NSValue* _data;
    NSInvocation* _synchronize;
}
-(void)synchronize;
-(void)setSynchronize:(NSInvocation*)sync;
-(unsigned)count;
-(id)objectAtIndex:(unsigned)index;
-(void)addObject:(CelestiaDestination*)o;
-(void)insertObject:(CelestiaDestination*)o atIndex:(unsigned)index;
-(void)removeLastObject;
-(void)removeObjectAtIndex:(unsigned)index;
-(void)replaceObjectAtIndex:(unsigned)index withObject:(CelestiaDestination*)o;
@end
