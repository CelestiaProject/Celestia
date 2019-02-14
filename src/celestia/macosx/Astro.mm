//
//  Astro.mm
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (C) 2001-9, the Celestia Development Team
//

#import "Astro.h"
#import "Observer.h"
#import "CelestiaUniversalCoord_PrivateAPI.h"
#import "CelestiaVector_PrivateAPI.h"

NSDictionary* coordinateDict;

@implementation NSDate(AstroAPI)
+(NSDate*)dateWithJulian:(NSNumber*)jd
{
    NSDate *date = nil;
    astro::Date astroDate([jd doubleValue]);
    int year = astroDate.year;
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
    NSCalendar *currentCalendar = [NSCalendar currentCalendar];
    [currentCalendar setTimeZone: [NSTimeZone timeZoneWithAbbreviation:@"GMT"]];
    NSDateComponents *comps = [[[NSDateComponents alloc] init] autorelease];
    int era = 1;
    if (year < 1)
    {
        era  = 0;
        year = 1 - year;
    }
    [comps setEra:    era];
    [comps setYear:   year];
    [comps setMonth:  astroDate.month];
    [comps setDay:    astroDate.day];
    [comps setHour:   astroDate.hour];
    [comps setMinute: astroDate.minute];
    [comps setSecond: (int)astroDate.seconds];
    date = [currentCalendar dateFromComponents: comps];
#else
    date = [NSCalendarDate dateWithYear: year
                                  month: astroDate.month
                                    day: astroDate.day
                                   hour: astroDate.hour
                                 minute: astroDate.minute
                                 second: (int)astroDate.seconds
                               timeZone: [NSTimeZone timeZoneWithAbbreviation:@"GMT"]];
#endif

    return date;
}
@end


@implementation Astro
+(NSString*)stringWithCoordinateSystem:(NSNumber*)n
{
    NSArray* keys = [coordinateDict allKeys];
    unsigned int i;
    for (i=0;i<[keys count];i++) {
        if ([[coordinateDict objectForKey:[keys objectAtIndex:i]] isEqualToNumber:n])
            return [keys objectAtIndex:i];
    }
    return nil;
}
+(void)initialize
{
    // compiler macro would be prettier I guess
    coordinateDict = [[NSDictionary alloc] initWithObjectsAndKeys:
                       [NSNumber numberWithInt:ObserverFrame::Universal],  @"Universal",
                       [NSNumber numberWithInt:ObserverFrame::Ecliptical], @"Ecliptical",
                       [NSNumber numberWithInt:ObserverFrame::Equatorial], @"Equatorial",
                       [NSNumber numberWithInt:ObserverFrame::BodyFixed],  @"Geographic",
                       [NSNumber numberWithInt:ObserverFrame::ObserverLocal], @"ObserverLocal",
                       [NSNumber numberWithInt:ObserverFrame::PhaseLock],  @"PhaseLock",
                       [NSNumber numberWithInt:ObserverFrame::Chase],      @"Chase",
                       nil];
}
+(NSNumber*)sphereIlluminationFraction:(CelestiaVector*)spherePos viewerPosition:(CelestiaVector*)viewerPos
{
    return [NSNumber numberWithFloat:astro::sphereIlluminationFraction(
        [spherePos point3d],
        [viewerPos point3d])];
}

/*
+(CelestiaVector*)heliocentricPosition:(CelestiaUniversalCoord*)universal starPosition:(CelestiaVector*)starPosition
{
    return [CelestiaVector vectorWithPoint3d:astro::heliocentricPosition([universal universalCoord], [starPosition point3f])];
}
+(CelestiaUniversalCoord*)universalPosition:(CelestiaVector*)heliocentric starPosition:(CelestiaVector*)starPosition
{
    return [[[CelestiaUniversalCoord alloc] initWithUniversalCoord:astro::universalPosition([heliocentric point3d],[starPosition point3f])] autorelease];
}
+(CelestiaVector*)equatorialToCelestialCart:(NSNumber*)ra declination:(NSNumber*)dec distance:(NSNumber*)distance
{
    return [CelestiaVector vectorWithPoint3d:astro::equatorialToCelestialCart([ra doubleValue], [dec doubleValue], [distance doubleValue])];
}
 */

+(NSNumber*)coordinateSystem:(NSString*)coord
{
    return [coordinateDict objectForKey:coord];
}

+(NSNumber*)julianDate:(NSDate *)date
{
    NSTimeZone *prevTimeZone = [NSTimeZone defaultTimeZone];
    // UTCtoTDB() expects GMT
    [NSTimeZone setDefaultTimeZone: [NSTimeZone timeZoneWithAbbreviation: @"GMT"]];
    NSDate *roundedDate = nil;

#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
    NSCalendar *currentCalendar = [[NSCalendar alloc] initWithCalendarIdentifier: NSGregorianCalendar];
    NSDateComponents *comps = [currentCalendar components:
        NSEraCalendarUnit  |
        NSYearCalendarUnit | NSMonthCalendarUnit  | NSDayCalendarUnit | 
        NSHourCalendarUnit | NSMinuteCalendarUnit | NSSecondCalendarUnit
                                                 fromDate: date];
    int era  = [comps era];
    int year = [comps year];
    if (era < 1) year = 1 - year;
    astro::Date astroDate(year, [comps month], [comps day]);
    astroDate.hour    = [comps hour];
    astroDate.minute  = [comps minute];
    astroDate.seconds = [comps second];
    // -[NSDateComponents second] is rounded to an integer,
    // so have to calculate and add decimal part
    roundedDate = [currentCalendar dateFromComponents: comps];
    [currentCalendar release];
#else
    NSCalendarDate *cd =  [date dateWithCalendarFormat: nil timeZone: nil];
    astro::Date astroDate([cd yearOfCommonEra],
                          [cd monthOfYear],
                          [cd dayOfMonth]);
    astroDate.hour    = [cd hourOfDay];    // takes DST in to account
    astroDate.minute  = [cd minuteOfHour];
    astroDate.seconds = [cd secondOfMinute];
    roundedDate = cd;
#endif

    NSTimeInterval extraSeconds = [date timeIntervalSinceDate: roundedDate];
    astroDate.seconds += extraSeconds;

    [NSTimeZone setDefaultTimeZone: prevTimeZone];
    double jd = astro::UTCtoTDB(astroDate);
    return [NSNumber numberWithDouble: jd];
}

+(NSNumber*)speedOfLight
{
    return [NSNumber numberWithDouble:astro::speedOfLight];
}
+(NSNumber*)J2000
{
    return [NSNumber numberWithDouble:astro::J2000];
}
+(NSNumber*)G
{
    return [NSNumber numberWithDouble:astro::G];
}
+(NSNumber*)solarMass
{
    return [NSNumber numberWithDouble:astro::SolarMass];
}
+(NSNumber*)earthMass
{
    return [NSNumber numberWithDouble:astro::EarthMass];
}
+(NSNumber*)lumToAbsMag:(NSNumber*)lum
{
    return [NSNumber numberWithFloat:astro::lumToAbsMag([lum floatValue])];
}
+(NSNumber*)lumToAppMag:(NSNumber*)lum lightYears:(NSNumber*)lyrs
{
    return [NSNumber numberWithFloat:astro::lumToAppMag([lum floatValue], [lyrs floatValue])];
}
+(NSNumber*)absMagToLum:(NSNumber*)mag
{
    return [NSNumber numberWithFloat:astro::absMagToLum([mag floatValue])];
}
+(NSNumber*)absToAppMag:(NSNumber*)mag lightYears:(NSNumber*)lyrs
{
    return [NSNumber numberWithFloat:astro::absToAppMag([mag floatValue], [lyrs floatValue])];
}
+(NSNumber*)appToAbsMag:(NSNumber*)mag lightYears:(NSNumber*)lyrs
{
    return [NSNumber numberWithFloat:astro::appToAbsMag([mag floatValue], [lyrs floatValue])];
}
+(NSNumber*)lightYearsToParsecs:(NSNumber*)ly
{
    return [NSNumber numberWithFloat:astro::lightYearsToParsecs([ly floatValue])];
}
+(NSNumber*)parsecsToLightYears:(NSNumber*)pc
{
    return [NSNumber numberWithFloat:astro::parsecsToLightYears([pc floatValue])];
}
+(NSNumber*)lightYearsToKilometers:(NSNumber*)ly
{
    return [NSNumber numberWithDouble:astro::lightYearsToKilometers([ly doubleValue])];
}
+(NSNumber*)kilometersToLightYears:(NSNumber*)km
{
    return [NSNumber numberWithDouble:astro::kilometersToLightYears([km doubleValue])];
}
+(NSNumber*)lightYearsToAU:(NSNumber*)ly
{
    return [NSNumber numberWithDouble:astro::lightYearsToAU([ly doubleValue])];
}
+(NSNumber*)AUtoLightYears:(NSNumber*)au
{
    return [NSNumber numberWithFloat:astro::AUtoLightYears([au floatValue])];
}
+(NSNumber*)kilometersToAU:(NSNumber*)km
{
    return [NSNumber numberWithDouble:astro::kilometersToAU([km doubleValue])];
}
+(NSNumber*)AUtoKilometers:(NSNumber*)au
{
    return [NSNumber numberWithFloat:astro::AUtoKilometers([au floatValue])];
}
+(NSNumber*)microLightYearsToKilometers:(NSNumber*)mly
{
    return [NSNumber numberWithDouble:astro::microLightYearsToKilometers([mly doubleValue])];
}
+(NSNumber*)kilometersToMicroLightYears:(NSNumber*)km
{
    return [NSNumber numberWithDouble:astro::kilometersToMicroLightYears([km doubleValue])];
}
+(NSNumber*)microLightYearsToAU:(NSNumber*)mly
{
    return [NSNumber numberWithDouble:astro::microLightYearsToAU([mly doubleValue])];
}
+(NSNumber*)AUtoMicroLightYears:(NSNumber*)au
{
    return [NSNumber numberWithDouble:astro::AUtoMicroLightYears([au doubleValue])];
}
+(NSNumber*)secondsToJulianDate:(NSNumber*)sec
{
    return [NSNumber numberWithDouble:astro::secondsToJulianDate([sec doubleValue])];
}
+(NSNumber*)julianDateToSeconds:(NSNumber*)jd
{
    return [NSNumber numberWithDouble:astro::julianDateToSeconds([jd doubleValue])];
}
+(NSArray*)decimalToDegMinSec:(NSNumber*)angle
{
    int hours, minutes;
    double seconds;
    astro::decimalToDegMinSec([angle doubleValue], hours, minutes, seconds);
    return [NSArray arrayWithObjects:[NSNumber numberWithInt:hours],[NSNumber numberWithInt:minutes],[NSNumber numberWithDouble:seconds],nil];
}
+(NSNumber*)degMinSecToDecimal:(NSArray*)dms
{
    return [NSNumber numberWithDouble:astro::degMinSecToDecimal([[dms objectAtIndex:0] intValue], [[dms objectAtIndex:1] intValue], [[dms objectAtIndex:2] intValue])];
}
+(NSArray*)anomaly:(NSNumber*)meanAnamaly eccentricity:(NSNumber*)eccentricity
{
    double trueAnomaly,eccentricAnomaly;
    astro::anomaly([meanAnamaly doubleValue], [eccentricity doubleValue], trueAnomaly, eccentricAnomaly);
    return [NSArray arrayWithObjects:[NSNumber numberWithDouble:trueAnomaly], [NSNumber numberWithDouble:eccentricAnomaly],nil];
}
+(NSNumber*)meanEclipticObliquity:(NSNumber*)jd
{
    return [NSNumber numberWithDouble:astro::meanEclipticObliquity([jd doubleValue])];
}
@end
