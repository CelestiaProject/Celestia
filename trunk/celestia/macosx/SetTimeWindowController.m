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
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
    dateTimeFormat = [[NSDateFormatter alloc] init];
    [ dateTimeFormat setFormatterBehavior: NSDateFormatterBehavior10_4 ];
    [ dateTimeFormat setDateFormat: @"MM/dd/uuuu HH:mm:ss" ];
    // uuuu handles -ve (BCE) years
    // 1 BCE = 0 (not -1)
    // Reference: Unicode Technical Standard #35
    // http://unicode.org/reports/tr35/tr35-4.html
#else
    dateTimeFormat = [[NSDateFormatter alloc] initWithDateFormat:
                                                   @"%m/%d/%Y %H:%M:%S"
                                            allowNaturalLanguage: NO ];
    bcFormat       = [[NSDateFormatter alloc] initWithDateFormat:
                                                   @"%m/%d/-%Y %H:%M:%S"
                                            allowNaturalLanguage: NO ];
    zeroFormat     = [[NSDateFormatter alloc] initWithDateFormat:
                                                  @"%m/%d/%y %H:%M:%S"
                                            allowNaturalLanguage: NO ];
    // Due to a "reverse Y2K" bug, the 4-digit %Y format rejects
    // the year 0 (1 BCE), even if 0000 is entered.
    // But the 2-digit %y format ends up working!
#endif
}

- (void) dealloc
{
#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_4
    [zeroFormat release];
    [bcFormat release];
#endif
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
#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_4
    {
        // 1 BCE - only works if year is 2 digits (00)
        // so transform 0000 etc to 2 digits
        NSArray* pieces = [dateString componentsSeparatedByString: @"/"];
        if ([pieces count] == 3)
        {
            NSString *yearString = [pieces objectAtIndex: 2];
            NSScanner *zeroScan  = [NSScanner scannerWithString: yearString];
            int yearVal = 0;
            if ([zeroScan scanInt: &yearVal] && 0 == yearVal)
            {
                dateString = [NSString stringWithFormat: @"%@/%@/00", [pieces objectAtIndex: 0], [pieces objectAtIndex: 1]];
            }
        }
    }
#endif
    inputString = [ [  dateString stringByAppendingString: @" " ]  
                             stringByAppendingString: timeString ];  

#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
    if (useUTC)
    {
        [dateTimeFormat setTimeZone: [NSTimeZone timeZoneWithAbbreviation: @"GMT"]];
    }
    else
    {
        [NSTimeZone resetSystemTimeZone];
        [dateTimeFormat setTimeZone: [NSTimeZone systemTimeZone]];
    }
#endif
    dateValid = [ dateTimeFormat getObjectValue: &date
                                      forString: inputString
                               errorDescription: &errorString ];
#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_4
    if (!dateValid)
    {
        // BCE dates (except 1 BCE)
        dateValid = [ bcFormat getObjectValue: &date
                                    forString: inputString
                             errorDescription: &errorString ];
    }
    if (!dateValid)
    {
        // 1 BCE - only works if year is 2 digits (00)
        dateValid = [ zeroFormat getObjectValue: &date
                                      forString: inputString
                               errorDescription: &errorString ];
    }
#endif

    if (dateValid)
    {
#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_4
        // +[Astro julianDate:] also converts NSDate->NSCalendarDate
        // but it doesn't work so well with -ve (BCE) dates
        // so help it by passing a ready-made NSCalendarDate
        NSArray *dtParts = [dateString componentsSeparatedByString: @"/"];
        NSArray *tmParts = [timeString componentsSeparatedByString: @":"];
        if (dtParts && tmParts &&
            [dtParts count] > 0 && [tmParts count] > 0)
        {
            NSTimeZone *tz = nil;
            if (useUTC)
            {
                tz = [NSTimeZone timeZoneWithAbbreviation: @"GMT"];
            }
            else
            {
                [NSTimeZone resetSystemTimeZone];
                tz = [NSTimeZone systemTimeZone];
            }
            date = [NSCalendarDate dateWithYear: [[dtParts objectAtIndex: 2] intValue]
                                          month: [[dtParts objectAtIndex: 0] intValue]
                                            day: [[dtParts objectAtIndex: 1] intValue]
                                           hour: [[tmParts objectAtIndex: 0] intValue]
                                         minute: [[tmParts objectAtIndex: 1] intValue]
                                         second: [[tmParts objectAtIndex: 2] intValue]
                                       timeZone: tz];
        }
#endif
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
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
        dateTimeString = [dateTimeFormat stringFromDate: date];
#else
        dateTimeString = [dateTimeFormat stringForObjectValue: date];
#endif
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
