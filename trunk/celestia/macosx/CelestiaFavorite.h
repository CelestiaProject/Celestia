//
//  CelestiaFavorite.h
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "CelestiaUniversalCoord.h"

// parentFolder is totally vestigal crap

@interface CelestiaFavorite : NSObject <NSCoding> {
    NSValue* _data;
    BOOL _freeWhenDone;
}
-(void)encodeWithCoder:(NSCoder*)coder;
-(id)initWithCoder:(NSCoder*)coder;
-(id)initWithDictionary:(NSDictionary*)dictionary;
-(NSDictionary*)dictionary;
-(void)activate;
-(void)setName:(NSString*)name;
-(id)initWithName:(NSString*)name;
-(id)initWithName:(NSString*)name parentFolder:(CelestiaFavorite*)folder;
-(id)initWithFolderName:(NSString*)name;
-(id)initWithFolderName:(NSString*)name parentFolder:(CelestiaFavorite*)folder;
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
//-(NSDictionary*)dictionaryRepresentation;
-(NSString*)description;
-(void)setName:(NSString*)name;
-(NSString*)selectionName;
-(NSString*)coordinateSystem;
@end