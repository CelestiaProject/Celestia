//
//  NSString_ObjCPlusPlus.h
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#include <string>

@interface NSString(ObjCPlusPlus)
-(std::string)stdString;
+(NSString *)stringWithStdString:(std::string)str;
@end
