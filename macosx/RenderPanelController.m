#import "RenderPanelController.h"
#import "CelestiaAppCore.h"

NSDictionary *_labelDict, *_renderDict;

@implementation RenderPanelController
-(void)finishSetup
{
    CelestiaRenderer *renderer;
    NSDictionary *flags;
    NSEnumerator *enumerator;
    id obj;
//    NSLog(@"[RenderPanelController(PrivateAPI) finishSetup]");
    renderer = [[CelestiaAppCore sharedAppCore] renderer];
//    NSLog(@"setup _labelDict");

    _labelDict = [[NSDictionary dictionaryWithObjectsAndKeys:@"Asteroids",[NSValue valueWithNonretainedObject:lAsteroids],@"Constellations",[NSValue valueWithNonretainedObject:lConstellations],@"Galaxies",[NSValue valueWithNonretainedObject:lGalaxies],@"Moons",[NSValue valueWithNonretainedObject:lMoons],@"Planets",[NSValue valueWithNonretainedObject:lPlanets],@"Spacecraft",[NSValue valueWithNonretainedObject:lSpacecraft],@"Stars",[NSValue valueWithNonretainedObject:lStars],nil,nil] retain];
//    NSLog(@"setup _renderDict");
    
    _renderDict = [[NSDictionary dictionaryWithObjectsAndKeys:@"Atmospheres",[NSValue valueWithNonretainedObject:rAtmospheres],@"AutoMag",[NSValue valueWithNonretainedObject:rAutoMag],@"Boundaries",[NSValue valueWithNonretainedObject:rBoundaries],@"CelestialSphere",[NSValue valueWithNonretainedObject:rCelestialSphere],@"CloudMaps",[NSValue valueWithNonretainedObject:rCloudMaps],@"CometTails",[NSValue valueWithNonretainedObject:rCometTails],@"Diagrams",[NSValue valueWithNonretainedObject:rDiagrams],@"EclipseShadows",[NSValue valueWithNonretainedObject:rEclipseShadows],@"Galaxies",[NSValue valueWithNonretainedObject:rGalaxies],@"Markers",[NSValue valueWithNonretainedObject:rMarkers],@"NightMaps",[NSValue valueWithNonretainedObject:rNightMaps],@"Orbits",[NSValue 
valueWithNonretainedObject:rOrbits],@"Planets",[NSValue valueWithNonretainedObject:rPlanets],@"RingShadows",[NSValue valueWithNonretainedObject:rRingShadows],@"SmoothLines",[NSValue valueWithNonretainedObject:rSmoothLines],@"Stars",[NSValue valueWithNonretainedObject:rStars],@"StarsAsPoints",[NSValue valueWithNonretainedObject:rStarsAsPoints],nil,nil] retain];

    [renderer unarchive];

//    NSLog(@"enumerate renderFlags");
    flags = [renderer renderFlags];
    enumerator = [_renderDict keyEnumerator];
    while ((obj = [enumerator nextObject]) != nil)
        [[obj nonretainedObjectValue] setState:([[flags objectForKey:[_renderDict objectForKey:obj]] boolValue])!=NO?NSOnState:NSOffState];

//    NSLog(@"enumerate labelFlags");
    flags = [renderer labelFlags];
    enumerator = [_labelDict keyEnumerator];
    while ((obj = [enumerator nextObject]) != nil)
        [[obj nonretainedObjectValue] setState:([[flags objectForKey:[_labelDict objectForKey:obj]] boolValue])!=NO?NSOnState:NSOffState];

//    NSLog(@"setup vertexShader");
    [rVertexShader setState:([renderer isVertexShaderEnabled]?NSOnState:NSOffState)];
    [rVertexShader setEnabled:([renderer isVertexShaderSupported]?YES:NO)];

//    NSLog(@"setup fragmentShader");
    [rFragmentShader setState:([renderer isFragmentShaderEnabled]?NSOnState:NSOffState)];
    [rFragmentShader setEnabled:([renderer isFragmentShaderSupported]?YES:NO)];

    [fBrightnessBias setFloatValue:[[renderer brightnessBias] floatValue]];
    [fSaturation setFloatValue:[[renderer saturationMagnitude] floatValue]];
    [fResolution selectItemAtIndex:[[renderer resolution] intValue]];

    [self applyChanges:nil];
}

-(void)awakeFromNib
{
    _labelDict = nil;
    _renderDict = nil;
}

- (IBAction)showWindow:(id)sender
{
//    NSLog(@"[RenderPanelController showWindow:%@]",sender);
    if (_labelDict == nil || _renderDict == nil)
        [self finishSetup];
    [super showWindow:sender];
}

- (IBAction)applyChanges:(id)sender
{
    CelestiaRenderer *renderer;
//    NSLog(@"[RenderPanelController applyChanges:%@]",sender);
    renderer = [[CelestiaAppCore sharedAppCore] renderer];
    if ((sender == bButton)||(sender == nil)) {
        NSMutableDictionary *flags;
        NSEnumerator *enumerator;
        id obj;
        flags = [[renderer labelFlags] mutableCopy];
        enumerator = [_labelDict keyEnumerator];
        while ((obj = [enumerator nextObject]) != nil)
            [flags setObject:[NSNumber numberWithBool:([[obj nonretainedObjectValue] state]==NSOnState)] forKey:[_labelDict objectForKey:obj]];
        [renderer setLabelFlags:flags];
//        NSLog(@"labelFlags = %@",flags);

        flags = [[renderer renderFlags] mutableCopy];
        enumerator = [_renderDict keyEnumerator];
        while ((obj = [enumerator nextObject]) != nil)
            [flags setObject:[NSNumber numberWithBool:([[obj nonretainedObjectValue] state]==NSOnState)] forKey:[_renderDict objectForKey:obj]];
        [renderer setRenderFlags:flags];
//        NSLog(@"renderFlags = %@",flags);

        [renderer setVertexShaderEnabled:[NSNumber numberWithBool:([rVertexShader state]==NSOnState)]];
        [renderer setFragmentShaderEnabled:[NSNumber numberWithBool:([rFragmentShader state]==NSOnState)]];

//        [renderer setBrightnessBias:[NSNumber numberWithFloat:[fBrightnessBias floatValue]]];
//        [renderer setSaturationMagnitude:[NSNumber numberWithFloat:[fSaturation floatValue]]];
        [renderer setResolution:[NSNumber numberWithInt:[fResolution indexOfSelectedItem]]];

        return;
    }

    if (sender == rVertexShader) {
        [renderer setVertexShaderEnabled:[NSNumber numberWithBool:([rVertexShader state]==NSOnState)]];
        return;
    }
    
    if (sender == rFragmentShader) {
        [renderer setFragmentShaderEnabled:[NSNumber numberWithBool:([rFragmentShader state]==NSOnState)]];
        return;
    }

    if (sender == fBrightnessBias) {
//        [renderer setBrightnessBias:[NSNumber numberWithFloat:[fBrightnessBias floatValue]]];
        return;
    }

    if (sender == fSaturation) {
//        [renderer setSaturationMagnitude:[NSNumber numberWithFloat:[fSaturation floatValue]]];
        return;
    }

    if (sender == fResolution) {
        [renderer setResolution:[NSNumber numberWithInt:[fResolution indexOfSelectedItem]]];
        return;
    }

    if ([_renderDict objectForKey:[NSValue valueWithNonretainedObject:sender]]!=nil) {
        [renderer setRenderFlag:[_renderDict objectForKey:[NSValue valueWithNonretainedObject:sender]] value:[NSNumber numberWithBool:([sender state]==NSOnState)]];
        return;
    }

    if ([_labelDict objectForKey:[NSValue valueWithNonretainedObject:sender]]!=nil) {
        [renderer setLabelFlag:[_labelDict objectForKey:[NSValue valueWithNonretainedObject:sender]] value:[NSNumber numberWithBool:([sender state]==NSOnState)]];
        return;
    }
    
}
@end
