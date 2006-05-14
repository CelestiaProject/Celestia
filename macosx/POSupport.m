/*
 * POSupport.m
 * Celestia
 *
 * Created by Da Woon Jung on 3/14/05.
 * Copyright 2005 dwj. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#import "POSupport.h"


const char *localizedUTF8String(const char *key)
{
    return localizedUTF8StringWithDomain("po", key);
}

const char *localizedUTF8StringWithDomain(const char *domain, const char *key)
{
    static char zero = 0;
    NSString *str = NSLocalizedStringFromTable([NSString stringWithUTF8String:key], [NSString stringWithUTF8String:domain], nil);

    if (str)
    {
        NSMutableData *data = [NSMutableData dataWithData:
            [str dataUsingEncoding: NSUTF8StringEncoding]];
        if ([data length] > 0)
        {
            [data appendBytes:&zero length:sizeof(char)];
            return [data bytes];
        }
        
        return key;
    }

    return key;
}
