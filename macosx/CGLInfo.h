//
//  CGLInfo.h
//  celestia
//
//  Created by Bob Ippolito on Wed Jun 26 2002.
//  Copyright (c) 2002 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <OpenGL/OpenGL.h>

@interface CGLInfo : NSObject {

}
+(NSArray*)displayDescriptions;
+(NSString*)rendererFromID:(unsigned)rc;
@end
