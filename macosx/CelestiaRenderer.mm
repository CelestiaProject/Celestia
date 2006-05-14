//
//  CelestiaRenderer.mm
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaRenderer.h"
#import "CelestiaRenderer_PrivateAPI.h"

NSDictionary* _labelFlags;
NSDictionary* _renderFlags;

@implementation CelestiaRenderer(PrivateAPI)
-(CelestiaRenderer *)initWithRenderer:(Renderer *)ren 
{
    self = [super init];
    _data = [[NSValue alloc] initWithBytes:reinterpret_cast<void*>(&ren) objCType:@encode(Renderer*)];
    return self;
}

-(Renderer *)renderer
{
    return reinterpret_cast<Renderer*>([_data pointerValue]);
}
@end

@implementation CelestiaRenderer
+(void)initialize
{
    _labelFlags = [[NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:Renderer::StarLabels],@"Stars",[NSNumber numberWithInt:Renderer::PlanetLabels],@"Planets",[NSNumber numberWithInt:Renderer::MoonLabels],@"Moons",[NSNumber numberWithInt:Renderer::ConstellationLabels],@"Constellations",[NSNumber numberWithInt:Renderer::GalaxyLabels],@"Galaxies",[NSNumber numberWithInt:Renderer::AsteroidLabels],@"Asteroids",[NSNumber numberWithInt:Renderer::SpacecraftLabels],@"Spacecraft",nil,nil] retain];
    _renderFlags = [[NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:Renderer::ShowStars],@"Stars",[NSNumber 
numberWithInt:Renderer::ShowPlanets],@"Planets",[NSNumber numberWithInt:Renderer::ShowGalaxies],@"Galaxies",[NSNumber numberWithInt:Renderer::ShowDiagrams],@"Diagrams",[NSNumber numberWithInt:Renderer::ShowCloudMaps],@"CloudMaps",[NSNumber numberWithInt:Renderer::ShowOrbits],@"Orbits",[NSNumber numberWithInt:Renderer::ShowCelestialSphere],@"CelestialSphere",[NSNumber numberWithInt:Renderer::ShowNightMaps],@"NightMaps",[NSNumber numberWithInt:Renderer::ShowAtmospheres],@"Atmospheres",[NSNumber numberWithInt:Renderer::ShowSmoothLines],@"SmoothLines",[NSNumber numberWithInt:Renderer::ShowEclipseShadows],@"EclipseShadows",[NSNumber numberWithInt:Renderer::ShowStarsAsPoints],@"StarsAsPoints",[NSNumber numberWithInt:Renderer::ShowRingShadows],@"RingShadows",[NSNumber numberWithInt:Renderer::ShowBoundaries],@"Boundaries",[NSNumber numberWithInt:Renderer::ShowAutoMag],@"AutoMag",[NSNumber numberWithInt:Renderer::ShowCometTails],@"CometTails",[NSNumber numberWithInt:Renderer::ShowMarkers],@"Markers",nil,nil] retain];
}

-(void)dealloc
{
    if (_data != nil) {
        [_data release];
        _data = nil;
    }
    [super dealloc];
}

-(NSDictionary*)renderFlags
{
    int iflags = [self renderer]->getRenderFlags();
    NSMutableDictionary* flags = [NSMutableDictionary dictionaryWithCapacity:[_renderFlags count]];
    NSEnumerator* enumerator = [_renderFlags keyEnumerator];
    id obj = nil;
    while ((obj = [enumerator nextObject]) != nil) {
        [flags setObject:[NSNumber numberWithBool:(0!=(iflags & [[_renderFlags objectForKey:obj] intValue]))] forKey:obj];
    }
    return flags;
}
    
-(void)setRenderFlags:(NSDictionary*)flags
{
    int iflags = 0;
    NSEnumerator* enumerator = [flags keyEnumerator];
    id obj = nil;
    while ((obj = [enumerator nextObject]) != nil) {
        if ([[flags objectForKey:obj] boolValue])
            iflags |= [[_renderFlags objectForKey:obj] intValue];
    }
    [self renderer]->setRenderFlags(iflags);
}    

-(void)setRenderFlag:(NSString*)key value:(NSNumber*)value
{
    if ([value boolValue]) {
        [self renderer]->setRenderFlags([self renderer]->getRenderFlags() | [[_renderFlags objectForKey:key] intValue]);
    } else {
        [self renderer]->setRenderFlags([self renderer]->getRenderFlags() &~ [[_renderFlags objectForKey:key] intValue]);
    }
}

-(NSNumber*)renderFlag:(NSString*)key
{
    return [NSNumber numberWithBool:(BOOL)([[_renderFlags objectForKey:key] intValue] & [self renderer]->getRenderFlags())];
}

-(NSDictionary*)labelFlags
{
    int iflags = [self renderer]->getLabelMode();
    NSMutableDictionary* flags = [NSMutableDictionary dictionaryWithCapacity:[_labelFlags count]];
    NSEnumerator* enumerator = [_labelFlags keyEnumerator];
    id obj = nil;
    while ((obj = [enumerator nextObject]) != nil) {
        [flags setObject:[NSNumber numberWithBool:(BOOL)(iflags & [[_labelFlags objectForKey:obj] intValue])] forKey:obj];
    }
    return flags;
}

-(void)setLabelFlags:(NSDictionary*)flags
{
    int iflags = 0;
    NSEnumerator* enumerator = [flags keyEnumerator];
    id obj = nil;
    while ((obj = [enumerator nextObject]) != nil) {
        if ([[flags objectForKey:obj] boolValue])
            iflags |= [[_labelFlags objectForKey:obj] intValue];
    }
    [self renderer]->setLabelMode(iflags);
}

-(void)setLabelFlag:(NSString*)key value:(NSNumber*)value
{
    if ([value boolValue]) {
        [self renderer]->setLabelMode([self renderer]->getLabelMode() | [[_labelFlags objectForKey:key] intValue]);
    } else {
        [self renderer]->setLabelMode([self renderer]->getLabelMode() &~ [[_labelFlags objectForKey:key] intValue]);
    }
}

-(NSNumber*)labelFlag:(NSString*)key
{
    return [NSNumber numberWithBool:(BOOL)([[_labelFlags objectForKey:key] intValue] & [self renderer]->getLabelMode())];
}

-(NSNumber*)ambientLightLevel
{
    return [NSNumber numberWithFloat:[self renderer]->getAmbientLightLevel()];
}
-(void)setAmbientLightLevel:(NSNumber*)level
{
    [self renderer]->setAmbientLightLevel([level floatValue]);
}

-(void)setMinimumOrbitSize:(NSNumber*)pixels
{
    [self renderer]->setMinimumOrbitSize([pixels intValue]);
}

-(BOOL)isFragmentShaderEnabled
{
    return (BOOL)[self renderer]->getFragmentShaderEnabled();
}

-(NSNumber*)fragmentShaderEnabled
{
    return [NSNumber numberWithBool:[self isFragmentShaderEnabled]];
}

-(void)setFragmentShaderEnabled:(NSNumber*)enable
{
    [self renderer]->setFragmentShaderEnabled((bool)[enable boolValue]);
}

-(BOOL)isFragmentShaderSupported
{
    return (BOOL)[self renderer]->fragmentShaderSupported();
}

-(BOOL)isVertexShaderEnabled
{
    return (BOOL)[self renderer]->getVertexShaderEnabled();
}

-(NSNumber*)vertexShaderEnabled;
{
    return [NSNumber numberWithBool:[self isVertexShaderEnabled]];
}

-(void)setVertexShaderEnabled:(NSNumber*)enable
{
    [self renderer]->setVertexShaderEnabled((bool)[enable boolValue]);
}

-(BOOL)isVertexShaderSupported
{
    return (BOOL)[self renderer]->vertexShaderSupported();
}
/*
-(NSNumber*)saturationMagnitude
{
    return [NSNumber numberWithFloat:[self renderer]->getSaturationMagnitude()];
}

-(void)setSaturationMagnitude:(NSNumber*)mag
{
    [self renderer]->setSaturationMagnitude([mag floatValue]);
}

-(NSNumber*)brightnessBias
{
    return [NSNumber numberWithFloat:[self renderer]->getBrightnessBias()];
}

-(void)setBrightnessBias:(NSNumber*)bias
{
    [self renderer]->setBrightnessBias([bias floatValue]);
}
*/
-(NSNumber*)resolution
{
    return [NSNumber numberWithUnsignedInt:[self renderer]->getResolution()];
}

-(void)setResolution:(NSNumber*)res
{
    [self renderer]->setResolution([res unsignedIntValue]);
}

- (int) getOrbitmask
{
    return [self renderer]->getOrbitMask();
}

- (void) setOrbitMask: (int) mask
{
    [self renderer]->setOrbitMask(mask);
}

-(void)unarchive
{
    if ([[NSUserDefaults standardUserDefaults] objectForKey:@"renderPreferences"]!=nil) {
        id obj;
        NSDictionary *prefs;
        Class dictClass = [NSDictionary class];
        SEL intValueSel = @selector(intValue);
        SEL unsignedIntValueSel = @selector(unsignedIntValue);
//        NSLog(@"deserializing render preferences from user defaults");
        obj = [[NSUserDefaults standardUserDefaults] objectForKey:@"renderPreferences"];
        if (obj && [obj isKindOfClass:dictClass]) {
            prefs = (NSDictionary *)obj;
            if ((obj = [prefs objectForKey:@"labelFlags"]) && [obj isKindOfClass:dictClass])
                [self setLabelFlags:obj];
            if ((obj = [prefs objectForKey:@"renderFlags"]) && [obj isKindOfClass:dictClass])
                [self setRenderFlags:obj];
//        [self setBrightnessBias:[prefs objectForKey:@"brightnessBias"]];
//        [self setSaturationMagnitude:[prefs objectForKey:@"saturationMagnitude"]];
//        [self setVertexShaderEnabled:[prefs objectForKey:@"vertexShaderEnabled"]];
//        [self setFragmentShaderEnabled:[prefs objectForKey:@"fragmentShaderEnabled"]];
            if ((obj = [prefs objectForKey:@"resolution"]) && [obj respondsToSelector:unsignedIntValueSel])
                [self setResolution:obj];
            if ((obj = [prefs objectForKey:@"orbitMask"]) && [obj respondsToSelector:intValueSel])
                [self setOrbitMask:[obj intValue]];
        }
    }
}

-(void)archive
{
    [[NSUserDefaults standardUserDefaults] setObject:[NSDictionary dictionaryWithObjectsAndKeys:[self labelFlags],@"labelFlags",[self renderFlags],@"renderFlags",[self resolution],@"resolution",[self vertexShaderEnabled],@"vertexShaderEnabled",[self fragmentShaderEnabled],@"fragmentShaderEnabled",
    [NSNumber numberWithInt: [self getOrbitmask]], @"orbitMask",
    
    nil,nil] forKey:@"renderPreferences"];
}

@end
