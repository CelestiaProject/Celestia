//
//  SetTimeWindowController.m
//  celestia
//
//  Created by Bob Ippolito on Tue May 28 2002.
//  Copyright (C) 2007, Celestia Development Team
//
#import "SetTimeWindowController.h"
#import "CelestiaAppCore.h"
#import "CelestiaSimulation.h"
#import "Astro.h"
#import "CelestiaSettings.h"

@interface SetTimeWindowController(Private)
- (NSNumber *) julianDateFromDateAndTime;
@end


@implementation SetTimeWindowController

- (void) awakeFromNib
{
    dateTimeFormat = [[NSDateFormatter alloc] init];
    [ dateTimeFormat setFormatterBehavior: NSDateFormatterBehavior10_4 ];
    [ dateTimeFormat setDateFormat: @"MM/dd/uuuu HH:mm:ss" ];
    // uuuu handles -ve (BCE) years
    // 1 BCE = 0 (not -1)
    // Reference: Unicode Technical Standard #35
    // http://unicode.org/reports/tr35/tr35-4.html
}

- (void) dealloc
{
    [dateTimeFormat release];
    [super dealloc];
}

- (IBAction)setTime:(id)sender
{
    NSString* dateString = [dateField stringValue];
    NSString* timeString = [timeField stringValue];
    if ( [dateString isEqualToString: @""] && [timeString isEqualToString: @""])
    {
        NSRunAlertPanel(NSLocalizedString(@"No Date or Time Entered",@""),
                        NSLocalizedString(@"Please enter a date and/or time.",@""),
                        nil,nil,nil);
        return;
    }

    CelestiaSimulation *sim = [[CelestiaAppCore sharedAppCore] simulation];
    NSNumber *jd = [self julianDateFromDateAndTime];
    if (jd)
        [sim setDate: jd ];
}

- (void)windowDidBecomeKey:(NSNotification *)aNotification
{
    if (!setupDone && [self window]==[aNotification object])
    {
        setupDone = YES;
        [[CelestiaSettings shared] scanForKeys: [self window]];
        [[CelestiaSettings shared] validateItems];
    }
}

- (NSNumber *) julianDateFromDateAndTime
{
    NSNumber *jd = nil;
    NSDate *date = nil;
    BOOL dateValid = NO;
    NSString *inputString = nil;
    NSString *errorString = nil;
    NSString* dateString = [dateField stringValue];
    NSString* timeString = [timeField stringValue];
    BOOL useUTC = (0 != [[CelestiaSettings shared] timeZone]);

    if ( [timeString isEqualToString: @""] )
    {
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
    }
    inputString = [ [  dateString stringByAppendingString: @" " ]  
                             stringByAppendingString: timeString ];  

    if (useUTC)
    {
        [dateTimeFormat setTimeZone: [NSTimeZone timeZoneWithAbbreviation: @"GMT"]];
    }
    else
    {
        [NSTimeZone resetSystemTimeZone];
        [dateTimeFormat setTimeZone: [NSTimeZone systemTimeZone]];
    }
    dateValid = [ dateTimeFormat getObjectValue: &date
                                      forString: inputString
                               errorDescription: &errorString ];

    if (dateValid)
    {
        jd = [ Astro julianDate: date];
    }
    return jd;
}

- (void) controlTextDidEndEditing: (NSNotification *) aNotification
{
    id sender = [aNotification object];

    if (dateField == sender || timeField == sender)
    {
        if ([[dateField stringValue] isEqualToString: @""])
            return;

        NSNumber *jd = [self julianDateFromDateAndTime];
        if (jd)
        {
            [ jdField setDoubleValue: [jd doubleValue] ];
        }
        else
        {
            NSRunAlertPanel(NSLocalizedString(@"Improper Date or Time Format",@""),
                            NSLocalizedString(@"Please enter the date as \"mm/dd/yyyy\" and the time as \"hh:mm:ss\".",@""),
                            nil,nil,nil);
        }
    }
    else if (jdField == sender)
    {
        if ([[jdField stringValue] isEqualToString: @""])
            return;

        NSDate *date = [ NSDate dateWithJulian: [NSNumber numberWithDouble: [jdField doubleValue] ] ];
        NSString *dateTimeString = nil;
        dateTimeString = [dateTimeFormat stringFromDate: date];
        if (dateTimeString && [dateTimeString length] > 0)
        {
            NSArray *components = [dateTimeString componentsSeparatedByString: @" "];
            if (components && [components count] > 1)
            {
                [dateField setStringValue: [components objectAtIndex: 0]];
                [timeField setStringValue: [components objectAtIndex: 1]];
            }
        }
    }
}
@end
