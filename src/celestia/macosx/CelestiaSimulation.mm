//
//  CelestiaSimulation.mm
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaSimulation.h"
#import "CelestiaSimulation_PrivateAPI.h"
#import "CelestiaUniversalCoord_PrivateAPI.h"
#import "CelestiaSelection_PrivateAPI.h"
#import "CelestiaUniverse_PrivateAPI.h"
#import "CelestiaObserver_PrivateAPI.h"
#import "CelestiaVector_PrivateAPI.h"
#import "NSString_ObjCPlusPlus.h"
#import "Astro.h"

/*
void gotoLocation(const RigidTransform& transform, double duration);
void setFrame(const FrameOfReference&);
FrameOfReference getFrame() const;
*/

@implementation CelestiaSimulation(PrivateAPI)
-(CelestiaSimulation *)initWithSimulation:(Simulation *)sim 
{
    self = [super init];
    _data = [[NSValue alloc] initWithBytes:reinterpret_cast<void*>(&sim) objCType:@encode(Simulation*)];
    return self;
}
-(Simulation *)simulation
{
    return reinterpret_cast<Simulation*>([_data pointerValue]);
}
@end

@implementation CelestiaSimulation
-(void)dealloc
{
    if (_data != nil)
    {
        [_data release];
        _data = nil;
    }
    [super dealloc];
}
-(NSNumber*)julianDate
{
    return [NSNumber numberWithDouble:[self simulation]->getTime()];
}
-(void)setDate:(NSNumber*)t
{
    [self simulation]->setTime([t doubleValue]);
}

-(NSNumber*)getTimeScale
{
    return [NSNumber numberWithDouble:[self simulation]->getTimeScale()];
}

-(NSNumber*)realTime
{
    return [NSNumber numberWithDouble:[self simulation]->getRealTime()];
}
-(NSNumber*)arrivalTime
{
    return [NSNumber numberWithDouble:[self simulation]->getArrivalTime()];
}
-(void)update:(NSNumber*)dt
{
    [self simulation]->update([dt doubleValue]);
}
/*
-(CelestiaSelection*)pickObject:(CelestiaVector*)pickRay tolerance:(NSNumber*)tolerance
{
    return [[[CelestiaSelection alloc] initWithSelection:[self simulation]->pickObject([pickRay vec3f],[tolerance floatValue])] autorelease];
}
 */
-(CelestiaSelection*)pickObject:(CelestiaVector*)pickRay
{
    return [self pickObject:pickRay tolerance:[NSNumber numberWithDouble:0.0]];
}
-(CelestiaUniverse*)universe
{
    Universe* uni = [self simulation]->getUniverse();
    if (uni == NULL)
        return nil;
    return [[[CelestiaUniverse alloc] initWithUniverse:uni] autorelease];
}

/*
-(void)orbit:(CelestiaVector*)q
{
    [self simulation]->orbit([q quatf]);
}
-(void)rotate:(CelestiaVector*)q
{
    [self simulation]->rotate([q quatf]);
}
 */
-(void)changeOrbitDistance:(NSNumber*)d
{
    [self simulation]->changeOrbitDistance([d floatValue]);
}
-(void)setTargetSpeed:(NSNumber*)s
{
    [self simulation]->setTargetSpeed([s floatValue]);
}
-(NSNumber*)targetSpeed
{
    return [NSNumber numberWithDouble:[self simulation]->getTargetSpeed()];
}
-(CelestiaSelection*)selection
{
    return [[[CelestiaSelection alloc] initWithSelection:[self simulation]->getSelection()] autorelease];
}
-(void)setSelection:(CelestiaSelection*)sel
{
    [self simulation]->setSelection([sel selection]);
}
-(CelestiaSelection*)trackedObject
{
    return [[[CelestiaSelection alloc] initWithSelection:[self simulation]->getTrackedObject()] autorelease];
}
-(void)setTrackedObject:(CelestiaSelection*)sel
{
    [self simulation]->setTrackedObject([sel selection]);
}    

-(void)selectPlanet:(NSNumber*)planetNo
{
    [self simulation]->selectPlanet([planetNo intValue]);
}

-(CelestiaSelection*)findObject:(NSString*)s
{
    return [[[CelestiaSelection alloc] initWithSelection:[self simulation]->findObject([s stdString])] autorelease];
}

-(CelestiaSelection*)findObjectFromPath:(NSString*)s
{
    return [[[CelestiaSelection alloc] initWithSelection:[self simulation]->findObjectFromPath([s stdString], true)] autorelease];
}

-(void)gotoSelection:(NSNumber*)gotoTime up:(CelestiaVector*)up coordinateSystem:(NSString*)csysName
{
    [self simulation]->gotoSelection(
        [gotoTime doubleValue],
        [up vector3f],
        (ObserverFrame::CoordinateSystem)[[Astro coordinateSystem:csysName] intValue]);
}


-(void)gotoSelection:(NSNumber*)gotoTime distance:(NSNumber*)distance up:(CelestiaVector*)up coordinateSystem:(NSString*)csysName
{
    [self simulation]->gotoSelection(
        [gotoTime doubleValue],
        [distance doubleValue],
        [up vector3f], 
        (ObserverFrame::CoordinateSystem)[[Astro coordinateSystem:csysName] intValue]);
}

-(void)gotoSelection:(NSNumber*)gotoTime distance:(NSNumber*)distance longitude:(NSNumber*)longitude latitude:(NSNumber*)latitude up:(CelestiaVector*)up
{
    [self simulation]->gotoSelectionLongLat(
        [gotoTime doubleValue],
        [distance doubleValue],
        [longitude floatValue],
        [latitude floatValue],
        [up vector3f]);
}

-(NSArray*)getSelectionLongLat
{
    double distance, longitude, latitude;
    [self simulation]->getSelectionLongLat(distance, longitude, latitude);
    return [NSArray arrayWithObjects:[NSNumber numberWithDouble:distance],[NSNumber numberWithDouble:longitude],[NSNumber numberWithDouble:latitude],nil];
}

-(void)centerSelection
{
    [self simulation]->centerSelection();
}
-(void)centerSelection:(NSNumber*)centerTime
{
    [self simulation]->centerSelection([centerTime doubleValue]);
}
-(void)follow
{
    [self simulation]->follow();
}
-(void)geosynchronousFollow
{
    [self simulation]->geosynchronousFollow();
}
-(void)phaseLock
{
    [self simulation]->phaseLock();
}
-(void)chase
{
    [self simulation]->chase();
}
-(void)cancelMotion
{
    [self simulation]->cancelMotion();
}
-(CelestiaObserver*)observer
{
    return [[[CelestiaObserver alloc] initWithObserver:[self simulation]->getObserver()] autorelease];
}
-(void)setObserverPosition:(CelestiaUniversalCoord*)uc
{
    [self simulation]->setObserverPosition([uc universalCoord]);
}
-(void)setObserverOrientation:(CelestiaVector*)q
{
    [self simulation]->setObserverOrientation([q quaternionf]);
}
-(void)setObserverMode:(NSString*)m
{
    [self simulation]->setObserverMode([m isEqualToString:@"Free"] ? Observer::Free : Observer::Travelling);
}
-(NSString*)observerMode
{
    return ([self simulation]->getObserverMode() == Observer::Free) ? @"Free" : @"Travelling";
}

-(void)setFrame:(NSString*)cs selection:(CelestiaSelection*)sel
{
    [self simulation]->setFrame((ObserverFrame::CoordinateSystem)[[Astro coordinateSystem:cs] intValue], [sel selection]);
}

@end