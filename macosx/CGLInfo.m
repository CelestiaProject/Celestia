//
//  CGLInfo.m
//  celestia
//
//  Created by Bob Ippolito on Wed Jun 26 2002.
//  Copyright (c) 2002 __MyCompanyName__. All rights reserved.
//

#import "CGLInfo.h"
#import <OpenGL/OpenGL.h>
#import <OpenGL/CGLRenderers.h>
#define NSUNSIGNED(x) [NSNumber numberWithUnsignedInt:x]
#define MAX_DISPLAYS 16
FOUNDATION_STATIC_INLINE NSRect NSRectFromCGRect(CGRect r) {
    return NSMakeRect(r.origin.x,r.origin.y,r.size.width,r.size.height);
}
NSDictionary* _CGLRenderers;
@implementation CGLInfo
+(void)initialize
{
    _CGLRenderers = [[NSDictionary dictionaryWithObjectsAndKeys:@"Generic",NSUNSIGNED(kCGLRendererGenericID),@"Software",NSUNSIGNED(kCGLRendererAppleSWID),@"ATI Rage 128",NSUNSIGNED(kCGLRendererATIRage128ID),@"ATI Rage Pro",NSUNSIGNED(kCGLRendererATIRageProID),@"ATI Radeon",NSUNSIGNED(kCGLRendererATIRadeonID),@"nVidia GeForce2MX",NSUNSIGNED(kCGLRendererGeForce2MXID),@"nVidia GeForce3",NSUNSIGNED(kCGLRendererGeForce3ID),@"3DFX (Mesa)",NSUNSIGNED(kCGLRendererMesa3DFXID),nil,nil] retain];
}
+(NSString*)rendererFromID:(unsigned)rc
{
    NSString* obj = [_CGLRenderers objectForKey:NSUNSIGNED(rc)];
    return ((id)obj == nil) ? (NSString*)@"Unknown" : (NSString*)obj;
}
+(NSArray*)displayDescriptions
{
    NSMutableArray* allDisplays = [NSMutableArray arrayWithCapacity:1];
    CGDirectDisplayID* displayList;
    CGDisplayCount displayIndex, displayCount;
    CGDisplayErr err;

    err = CGGetActiveDisplayList(0, NULL, &displayCount);
    if (err != CGDisplayNoErr)
        return nil;
    displayList = (CGDirectDisplayID*)malloc(displayCount*sizeof(CGDirectDisplayID));
    err = CGGetActiveDisplayList(displayCount, displayList, &displayCount);
    for (displayIndex = 0; displayIndex<displayCount; ++displayIndex) {
        CGDirectDisplayID selectedDisplay;
        CGOpenGLDisplayMask glMask;
        NSMutableDictionary* displayInfo = [NSMutableDictionary dictionaryWithCapacity:10];
        selectedDisplay = displayList[displayIndex];
        if ((glMask = CGDisplayIDToOpenGLDisplayMask(selectedDisplay)) != 0) {
            CGLRendererInfoObj rend;
            NSMutableArray* renderers;
            long i,nrend;
            err = CGLQueryRendererInfo(glMask, &rend, &nrend);
            renderers = [NSMutableArray arrayWithCapacity:nrend];
            for (i = 0; i < nrend; ++i) {
                long value;
                NSMutableDictionary *glInfo = [NSMutableDictionary dictionaryWithCapacity:10];
                CGLDescribeRenderer(rend, i, kCGLRPAccelerated, &value);
                [glInfo setObject:[NSNumber numberWithLong:value] forKey:@"Accelerated"];
                CGLDescribeRenderer(rend, i, kCGLRPMultiScreen, &value);
                [glInfo setObject:[NSNumber numberWithLong:value] forKey:@"MultiScreen"];
                CGLDescribeRenderer(rend, i, kCGLRPRendererID, &value);
                [glInfo setObject:[NSNumber numberWithLong:value] forKey:@"RendererID"];
                [glInfo setObject:[CGLInfo rendererFromID:value] forKey:@"Renderer"];
                CGLDescribeRenderer(rend, i, kCGLRPOffScreen, &value);
                [glInfo setObject:[NSNumber numberWithLong:value] forKey:@"OffScreen"];
                CGLDescribeRenderer(rend, i, kCGLRPVideoMemory, &value);
                [glInfo setObject:[NSNumber numberWithLong:value] forKey:@"VRAM"];
                CGLDescribeRenderer(rend, i, kCGLRPTextureMemory, &value);
                [glInfo setObject:[NSNumber numberWithLong:value] forKey:@"TextureVRAM"];
                [renderers addObject:glInfo];
            }
            CGLDestroyRendererInfo(rend);
            [displayInfo setObject:renderers forKey:@"Renderers"];
        }
        [displayInfo setObject:[NSNumber numberWithUnsignedInt:glMask] forKey:@"OpenGLDisplayMask"];
        [displayInfo setObject:NSStringFromRect(NSRectFromCGRect(CGDisplayBounds(selectedDisplay))) forKey:@"displayBounds"];
        [displayInfo setObject:[(NSDictionary*)CGDisplayCurrentMode(selectedDisplay) autorelease] forKey:@"currentMode"];
        [allDisplays addObject:displayInfo];
    }
    free(displayList);
    return allDisplays;
}
@end
