//
//  SetTimeWindowController.m
//  celestia
//
//  Created by Bob Ippolito on Tue May 28 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//
#import "SetTimeWindowController.h"
#import "CelestiaAppCore.h"
#import "CelestiaSimulation.h"
#import "Astro.h"


@implementation SetTimeWindowController

- (IBAction)setTime:(id)sender
{
    CelestiaSimulation *sim;
    NSString* dateString = [dateField stringValue];
    NSString* timeString = [timeField stringValue];
    NSString* inputString;  
    NSString* fmtString = @"%m/%d/%Y %H:%M:%S";
    NSCalendarDate* cdate;
    NSNumber* jd;

    sim = [[CelestiaAppCore sharedAppCore] simulation];
    if ( [dateString isEqualToString: @""] && [timeString isEqualToString: @""])
    {
       NSRunAlertPanel(@"No Date or Time Entered",@"Please enter a date and/or time.",nil,nil,nil);
       return;
    }
    
    if ( [timeString isEqualToString: @""] )
    {
//        NSLog(@"emptyTime");
        timeString = @"00:00:00";
    }
    else
    {
        NSArray* pieces = [ timeString componentsSeparatedByString: @":" ];
        if ( [[pieces objectAtIndex: [pieces count]-1 ] isEqualToString: @"" ])
           timeString = [timeString stringByAppendingString: @"00" ];
        if ([ pieces count] == 1) 
           timeString = [ timeString stringByAppendingString: @":00:00" ];
        else if ([ pieces count] == 2) 
           timeString = [ timeString stringByAppendingString: @":00" ];
//        NSLog(timeString);
    }
    inputString = [ [  dateString stringByAppendingString: @" " ]  
                             stringByAppendingString: timeString ];  
    cdate = [NSCalendarDate dateWithString: inputString calendarFormat: fmtString ];

    if ( cdate == NULL )
       {
//       NSLog(@"bad date");
       NSRunAlertPanel(@"Improper Date or Time Format",@"Please enter the date as \"mm/dd/yyyy\" and the time as \"hh:mm:ss\".",nil,nil,nil);
       return;
       }
//    NSLog( [cdate description] );
//    float fjd = [jd intValue];
//    NSLog ( @"jdate = %", fjd);
    jd = [ Astro julianDate: cdate];
    [sim setDate: jd ];
}

@end
