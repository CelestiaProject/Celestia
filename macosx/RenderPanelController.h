/* RenderPanelController */

#import <Cocoa/Cocoa.h>

@interface RenderPanelController : NSWindowController
{
    IBOutlet NSButton *bButton;
    IBOutlet NSTextField *fBrightnessBias;
    IBOutlet NSPopUpButton *fResolution;
    IBOutlet NSTextField *fSaturation;
    IBOutlet NSButton *lAsteroids;
    IBOutlet NSButton *lConstellations;
    IBOutlet NSButton *lGalaxies;
    IBOutlet NSButton *lMoons;
    IBOutlet NSButton *lPlanets;
    IBOutlet NSButton *lSpacecraft;
    IBOutlet NSButton *lStars;
    IBOutlet NSButton *rAtmospheres;
    IBOutlet NSButton *rAutoMag;
    IBOutlet NSButton *rBoundaries;
    IBOutlet NSButton *rCelestialSphere;
    IBOutlet NSButton *rCloudMaps;
    IBOutlet NSButton *rDiagrams;
    IBOutlet NSButton *rEclipseShadows;
    IBOutlet NSButton *rFragmentShader;
    IBOutlet NSButton *rGalaxies;
    IBOutlet NSButton *rNightMaps;
    IBOutlet NSButton *rOrbits;
    IBOutlet NSButton *rPlanets;
    IBOutlet NSButton *rRingShadows;
    IBOutlet NSButton *rSmoothLines;
    IBOutlet NSButton *rStars;
    IBOutlet NSButton *rStarsAsPoints;
    IBOutlet NSButton *rVertexShader;
}
- (void)finishSetup;
- (IBAction)applyChanges:(id)sender;
@end
