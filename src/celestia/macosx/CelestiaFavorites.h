//
//  CelestiaFavorites.h
//  celestia
//
//  Created by Bob Ippolito on Thu Jun 20 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaFavorite.h"
#import "myTree.h"


@interface CelestiaFavorites : MyTree
-(void)archive;
-(void)setSynchronize:(NSInvocation*)synchronize;
-(void)synchronize;
+(CelestiaFavorites*)sharedFavorites;
-(MyTree*)addNewFavorite:(NSString*)name;
-(MyTree*)addNewFolder:(NSString*)name;
@end
