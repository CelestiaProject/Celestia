//
//  Astro.h
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaUniversalCoord.h"
#import "CelestiaVector.h"

@interface NSDate(AstroAPI)
+(NSDate*)dateWithJulian:(NSNumber*)jd;
@end

@interface Astro : NSObject
+(NSString*)stringWithCoordinateSystem:(NSNumber*)n;
/*
+(CelestiaVector*)equatorialToCelestialCart:(NSNumber*)ra declination:(NSNumber*)dec distance:(NSNumber*)distance;
+(CelestiaUniversalCoord*)universalPosition:(CelestiaVector*)heliocentric starPosition:(CelestiaVector*)starPosition;
+(CelestiaVector*)heliocentricPosition:(CelestiaUniversalCoord*)universal starPosition:(CelestiaVector*)starPosition;
*/
+(NSNumber*)sphereIlluminationFraction:(CelestiaVector*)spherePos viewerPosition:(CelestiaVector*)viewerPos;
+(NSNumber*)coordinateSystem:(NSString*)coord;
+(NSNumber*)julianDate:(NSDate*)date;
+(NSNumber*)speedOfLight;
+(NSNumber*)J2000;
+(NSNumber*)G;
+(NSNumber*)solarMass;
+(NSNumber*)earthMass;
+(NSNumber*)lumToAbsMag:(NSNumber*)lum;
+(NSNumber*)lumToAppMag:(NSNumber*)lum lightYears:(NSNumber*)lyrs;
+(NSNumber*)absMagToLum:(NSNumber*)mag;
+(NSNumber*)absToAppMag:(NSNumber*)mag lightYears:(NSNumber*)lyrs;
+(NSNumber*)appToAbsMag:(NSNumber*)mag lightYears:(NSNumber*)lyrs;
+(NSNumber*)lightYearsToParsecs:(NSNumber*)ly;
+(NSNumber*)parsecsToLightYears:(NSNumber*)pc;
+(NSNumber*)lightYearsToKilometers:(NSNumber*)ly;
+(NSNumber*)kilometersToLightYears:(NSNumber*)km;
+(NSNumber*)lightYearsToAU:(NSNumber*)ly;
+(NSNumber*)AUtoLightYears:(NSNumber*)au;
+(NSNumber*)kilometersToAU:(NSNumber*)km;
+(NSNumber*)AUtoKilometers:(NSNumber*)au;
+(NSNumber*)microLightYearsToKilometers:(NSNumber*)mly;
+(NSNumber*)kilometersToMicroLightYears:(NSNumber*)km;
+(NSNumber*)microLightYearsToAU:(NSNumber*)mly;
+(NSNumber*)AUtoMicroLightYears:(NSNumber*)au;
+(NSNumber*)secondsToJulianDate:(NSNumber*)sec;
+(NSNumber*)julianDateToSeconds:(NSNumber*)jd;
+(NSArray*)decimalToDegMinSec:(NSNumber*)angle;
+(NSNumber*)degMinSecToDecimal:(NSArray*)dms;
+(NSArray*)anomaly:(NSNumber*)meanAnamaly eccentricity:(NSNumber*)eccentricity;
+(NSNumber*)meanEclipticObliquity:(NSNumber*)jd;
@end
