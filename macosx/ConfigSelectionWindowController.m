//
//  ConfigSelectionWindowController.m
//  Celestia
//
//  Created by 李林峰 on 2019/9/8.
//

#import "ConfigSelectionWindowController.h"
#import "CelestiaController.h"

NSString *const configFilePathPrefKey = @"configFilePath";
NSString *const dataDirPathPrefKey = @"dataDirPath";

@implementation ConfigSelectionWindowController

- (void)awakeFromNib {
    [super awakeFromNib];

    [configFilePathControl setURL:configFilePath];
    [dataDirPathControl setURL:dataDirPath];
}

- (void)dealloc {
    [configFilePath release];
    [dataDirPath release];
    [super dealloc];
}

- (IBAction)confirmSelection:(id)sender {
    // save the selection, since we are running in a sandboxed environment, save bookmark
    NSError *error = nil;
    NSData *configFilePathData = [[configFilePathControl URL] bookmarkDataWithOptions:NSURLBookmarkCreationSecurityScopeAllowOnlyReadAccess includingResourceValuesForKeys:nil relativeToURL:nil error:&error];
    NSData *dataDirPathData = [[dataDirPathControl URL] bookmarkDataWithOptions:NSURLBookmarkCreationSecurityScopeAllowOnlyReadAccess includingResourceValuesForKeys:nil relativeToURL:nil error:&error];
    if (error != nil) {
        // failed
        [[NSAlert alertWithError:error] runModal];
        return;
    }

    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];

    [prefs setValue:configFilePathData forKey:configFilePathPrefKey];
    [prefs setValue:dataDirPathData forKey:dataDirPathPrefKey];
    [prefs synchronize];
    [[CelestiaController shared] setNeedsRelaunch:YES];
    [NSApp terminate:nil];
}

@end
