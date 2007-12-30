//
//  Menu_Extensions.h
//  celestia
//
//  Created by Da Woon Jung on 12/9/07.
//  Copyright (C) 2007, Celestia Development Team
//

#import <Cocoa/Cocoa.h>

@class CelestiaSelection;
@class BrowserItem;

@interface NSMenu (CelestiaMenu)
- (int) indexOfItemWithLocalizableTitle: (NSString *) aTitle;
#if REFMARKS
- (BOOL) insertRefMarkItemsForSelection: (CelestiaSelection *) aSelection
                                atIndex: (int) aIndex;
#endif
- (BOOL) insertPlanetarySystemItemForSelection: (CelestiaSelection *) aSelection
                                        target: (id) aTarget
                                       atIndex: (int) aIndex;
- (BOOL) insertAltSurfaceItemForSelection: (CelestiaSelection *) aSelection
                                   target: (id) aTarget
                                  atIndex: (int) aIndex;
#if REFMARKS
- (void) removeRefMarkItems;
#endif
- (void) removePlanetarySystemItem;
- (void) removeAltSurfaceItem;
@end

@interface NSMenuItem (CelestiaMenu)
- (void) addPlanetarySystemMenuForItem: (BrowserItem *) browseItem
                                target: (id)  target
                                action: (SEL) action;
- (void) addAltSurfaceMenuWithNames: (NSArray *) surfaces
                             target: (id)  target
                             action: (SEL) action;
@end
