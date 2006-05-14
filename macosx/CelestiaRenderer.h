//
//  CelestiaRenderer.h
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//


@interface CelestiaRenderer : NSObject {
    NSValue* _data;
}

-(NSDictionary*)renderFlags;
-(void)setRenderFlags:(NSDictionary*)flags;
-(void)setRenderFlag:(NSString*)key value:(NSNumber*)value;
-(NSNumber*)renderFlag:(NSString*)key;

-(NSDictionary*)labelFlags;
-(void)setLabelFlags:(NSDictionary*)flags;
-(void)setLabelFlag:(NSString*)key value:(NSNumber*)value;
-(NSNumber*)labelFlag:(NSString*)key;

-(NSNumber*)ambientLightLevel;
-(void)setAmbientLightLevel:(NSNumber*)level;

-(void)setMinimumOrbitSize:(NSNumber*)pixels;

-(BOOL)isFragmentShaderEnabled;
-(NSNumber*)fragmentShaderEnabled;
-(void)setFragmentShaderEnabled:(NSNumber*)enable;
-(BOOL)isFragmentShaderSupported;

-(BOOL)isVertexShaderEnabled;
-(NSNumber*)vertexShaderEnabled;
-(void)setVertexShaderEnabled:(NSNumber*)enable;
-(BOOL)isVertexShaderSupported;
/*
-(NSNumber*)saturationMagnitude;
-(void)setSaturationMagnitude:(NSNumber*)mag;
-(NSNumber*)brightnessBias;
-(void)setBrightnessBias:(NSNumber*)bias;
*/
-(NSNumber*)resolution;
-(void)setResolution:(NSNumber*)res;
- (int) getOrbitmask;
- (void) setOrbitMask: (int) mask;

-(void)archive;
-(void)unarchive;
@end
