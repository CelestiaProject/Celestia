//
//  CelestiaFavorite.h
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "CelestiaUniversalCoord.h"

@interface CelestiaFavorite : NSObject {
    NSValue* _data;
}
//-(void)activate:(id)sender;
-(void)setName:(NSString*)name;
-(NSString*)name;
-(NSString*)selectionName;
-(CelestiaUniversalCoord*)position;
-(CelestiaVector*)orientation;
-(NSNumber*)jd;
-(BOOL)isFolder;
-(void)setParentFolder:(NSString*)parentFolder;
-(NSString*)parentFolder;
-(BOOL)isEqualToFavorite:(CelestiaFavorite*)fav;
-(BOOL)isEqualToString:(NSString*)str;
-(BOOL)isEqual:(id)obj;
-(NSDictionary*)dictionaryRepresentation;
-(NSString*)description;
-(void)setName:(NSString*)name;
-(NSString*)selectionName;
-(NSString*)coordinateSystem;
@end


@interface CelestiaFavorites : NSMutableArray {
    NSValue* _data;
    NSMutableArray* _retain;
}
-(void)synchronize;
-(void)setSynchronize:(NSInvocation*)sync;
-(unsigned)count;
-(id)objectAtIndex:(unsigned)index;
-(void)addObject:(CelestiaFavorite*)o;
-(void)insertObject:(CelestiaFavorite*)o atIndex:(unsigned)index;
-(void)removeLastObject;
-(void)removeObjectAtIndex:(unsigned)index;
-(void)replaceObjectAtIndex:(unsigned)index withObject:(CelestiaFavorite*)o;
-(unsigned)depthAtIndex:(unsigned)idx;
-(unsigned)firstIndexWithParentFolder:(CelestiaFavorite*)folder;
-(unsigned)lastIndexWithParentFolder:(CelestiaFavorite*)folder;
-(void)addNewFavorite:(NSString*)name withParentFolder:(CelestiaFavorite*)folder atIndex:(unsigned)idx;
-(void)addNewFavorite:(NSString*)name withParentFolder:(CelestiaFavorite*)folder;
-(void)addNewFolder:(NSString*)name withParentFolder:(CelestiaFavorite*)parentFolder atIndex:(unsigned)idx;
-(void)addNewFolder:(NSString*)name withParentFolder:(CelestiaFavorite*)parentFolder;
-(id)objectAtIndex:(unsigned)index parent:(CelestiaFavorite*)parent;
-(unsigned)numberOfChildrenOfItem:(CelestiaFavorite*)folder;

@end