//
//  Astro.mm
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "Astro.h"
#import "Astro_PrivateAPI.h"
#import "CelestiaUniversalCoord_PrivateAPI.h"
#import "CelestiaVector_PrivateAPI.h"

NSDictionary* coordinateDict;

@implementation NSDate(AstroAPI)
+(NSDate*)dateWithJulian:(NSNumber*)jd
{
    return [NSDate dateWithTimeIntervalSince1970:[[Astro julianDateToSeconds:jd] doubleValue]-[[Astro julianDateToSeconds:[NSNumber numberWithDouble:astro::Date(1970,1,1)]] doubleValue]];
}
@end

@implementation Astro(PrivateAPI)
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
    coordinateDict = [[NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:astro::Universal],@"Universal",[NSNumber numberWithInt:astro::Ecliptical],@"Ecliptical",[NSNumber numberWithInt:astro::Equatorial],@"Equatorial",[NSNumber numberWithInt:astro::Geographic],@"Geographic",[NSNumber numberWithInt:astro::ObserverLocal],@"ObserverLocal",[NSNumber numberWithInt:astro::PhaseLock],@"PhaseLock",[NSNumber numberWithInt:astro::Chase],@"Chase",nil,nil] retain];
}
+(NSNumber*)sphereIlluminationFraction:(CelestiaVector*)spherePos viewerPosition:(CelestiaVector*)viewerPos
{
    return [NSNumber numberWithFloat:astro::sphereIlluminationFraction(
        [spherePos point3d],
        [viewerPos point3d])];
}
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
+(NSNumber*)coordinateSystem:(NSString*)coord
{
    return [coordinateDict objectForKey:coord];
}

+(NSNumber*)julianDate:(NSDate *)date
{
    return [NSNumber numberWithDouble:([[Astro secondsToJulianDate:[NSNumber numberWithDouble:(double)[date timeIntervalSince1970]]] doubleValue]+(double)astro::Date(1970,1,1))];
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
    astro::Anomaly([meanAnamaly doubleValue], [eccentricity doubleValue], trueAnomaly, eccentricAnomaly);
    return [NSArray arrayWithObjects:[NSNumber numberWithDouble:trueAnomaly], [NSNumber numberWithDouble:eccentricAnomaly],nil];
}
+(NSNumber*)meanEclipticObliquity:(NSNumber*)jd
{
    return [NSNumber numberWithDouble:astro::meanEclipticObliquity([jd doubleValue])];
}
@end
