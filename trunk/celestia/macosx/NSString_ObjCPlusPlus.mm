//
//  NSString_ObjCPlusPlus.mm
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "NSString_ObjCPlusPlus.h"

@implementation NSString(ObjCPlusPlus)
-(std::string)stdString
{
    return std::string([self cString]);
}
-(NSString *)initWithStdString:(std::string)str
{
    return [self initWithCString:str.c_str()];
}
+(NSString *)stringWithStdString:(std::string)str
{
    return [NSString stringWithCString:str.c_str()];
}
@end
