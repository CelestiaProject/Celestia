//
//  BrowserItem.h
//  celestia
//
//  Created by Da Woon Jung on 2007-11-26
//  Copyright (C) 2007, Celestia Development Team
//

#import <Cocoa/Cocoa.h>

@class CelestiaDSO;
@class CelestiaStar;
@class CelestiaBody;
@class CelestiaLocation;


@interface BrowserItem : NSObject
{
    id data;
    NSMutableDictionary *children;
    NSArray *childNames;
    BOOL childrenChanged;
}
- (id)initWithCelestiaDSO:      (CelestiaDSO *)aDSO;
- (id)initWithCelestiaStar:     (CelestiaStar *)aStar;
- (id)initWithCelestiaBody:     (CelestiaBody *)aBody;
- (id)initWithCelestiaLocation: (CelestiaLocation *)aLocation;
- (id)initWithName:             (NSString *)aName;
- (id)initWithName:             (NSString *)aName
          children:             (NSDictionary *)aChildren;
+ (void)addChildrenToStar: (BrowserItem *) aStar;
+ (void)addChildrenToBody: (BrowserItem *) aBody;
- (void)dealloc;

- (NSString *)name;
- (id)body;
- (void)addChild: (BrowserItem *)aChild;
- (id)childNamed: (NSString *)aName;
- (NSArray *)allChildNames;
- (unsigned)childCount;
@end
