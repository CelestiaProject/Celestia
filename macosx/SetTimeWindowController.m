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
#import "CelestiaBody.h"
#import "Astro.h"
#import "CelestiaVector.h"
#include <math.h>

#define UNITS_KM 0
#define UNITS_RADII 1
#define UNITS_AU 2

@implementation SetTimeWindowController

- (IBAction)showWindow:(id)sender
{
/*
    CelestiaSimulation *sim;
    CelestiaSelection *csel;
    CelestiaBody *body;
    NSArray *distLongLat;
    double distance;
    //NSLog(@"-[GotoWindowController showWindow:%@]",sender);
    sim = [[CelestiaAppCore sharedAppCore] simulation];
    distLongLat = [sim getSelectionLongLat];
    //NSLog(@"distLongLat = %@",distLongLat);
    csel = [sim selection];
    body = [csel body];
    if (body != nil) {
        //NSLog(@"Body is NOT nil");
        distance = [[distLongLat objectAtIndex:0] doubleValue] - [[body radius] doubleValue];
        [distanceField setDoubleValue:distance];
        [longitudeField setDoubleValue:[[distLongLat objectAtIndex:1] doubleValue]];
        [latitudeField setDoubleValue:[[distLongLat objectAtIndex:2] doubleValue]];
        [objectField setStringValue:[body name]];
        [unitsButton selectItemAtIndex:UNITS_KM];
    }
*/
    [super showWindow:sender];

}

- (IBAction)setTime:(id)sender
{
    CelestiaSimulation *sim;
    NSString* dateString = [dateField stringValue];
    NSString* timeString = [timeField stringValue];
    NSString* inputString = [ [  dateString stringByAppendingString: @" " ]  
                             stringByAppendingString: timeString ];  
    NSString* fmtString = @"%m/%d/%Y %H:%M:%S";
    NSCalendarDate* cdate = [NSCalendarDate dateWithString: inputString calendarFormat: fmtString ];
    NSNumber* jd = [ Astro julianDate: cdate];

    sim = [[CelestiaAppCore sharedAppCore] simulation];
    if ( [dateString isEqualToString: @""] && [timeString isEqualToString: @""])
    {
       NSRunAlertPanel(@"No Date or Time Entered",@"Please enter a date and/or time.",nil,nil,nil);
       return;
    }
    if ( [timeString isEqualToString: @""] )
    {
        NSLog(@"emptyTime");
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
        NSLog(timeString);
        
    }
    if ( cdate == NULL )
       {
//       NSLog(@"bad date");
       NSRunAlertPanel(@"Improper Date or Time Format",@"Please enter the date as \"mm/dd/yyyy\" and the time as \"hh:mm:ss\".",nil,nil,nil);
       return;
       }
    NSLog( [cdate description] );
//    float fjd = [jd intValue];
//    NSLog ( @"jdate = %", fjd);
    [sim setDate: jd ];
}

@end
