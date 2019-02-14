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
    return std::string([self UTF8String]);
}
-(NSString *)initWithStdString:(std::string)str
{
    return [self initWithUTF8String:str.c_str()];
}
+(NSString *)stringWithStdString:(std::string)str
{
    return [NSString stringWithUTF8String:str.c_str()];
}
@end
