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

- (void)awakeFromNib
{
    [super awakeFromNib];

    [configFilePathControl setURL:configFilePath];
    [dataDirPathControl setURL:dataDirPath];
    [cancelButton setTitle:mandatory ? NSLocalizedString(@"Quit", @"") : NSLocalizedString(@"Cancel", @"")];
    [[[self window] standardWindowButton:NSWindowCloseButton] setEnabled:!mandatory];
}

- (void)dealloc
{
    [configFilePath release];
    [dataDirPath release];
    [super dealloc];
}

- (IBAction)reset:(id)sender
{
    [configFilePathControl setURL:[ConfigSelectionWindowController applicationConfig]];
    [dataDirPathControl setURL:[ConfigSelectionWindowController applicationDataDirectory]];
}

- (IBAction)confirmSelection:(id)sender
{
    // save the selection, since we are running in a sandboxed environment, save bookmark
    NSError *error = nil;
    NSData *configFilePathData = nil;
    NSData *dataDirPathData = nil; [[dataDirPathControl URL] bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope includingResourceValuesForKeys:nil relativeToURL:nil error:&error];

    // if reset to default, don't change and save nil
    if (![[configFilePathControl URL] isEqual:[ConfigSelectionWindowController applicationConfig]])
    {
        configFilePathData = [[configFilePathControl URL] bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope includingResourceValuesForKeys:nil relativeToURL:nil error:&error];
    }

    if (![[dataDirPathControl URL] isEqual:[ConfigSelectionWindowController applicationDataDirectory]])
    {
        dataDirPathData = [[dataDirPathControl URL] bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope includingResourceValuesForKeys:nil relativeToURL:nil error:&error];
    }

    if (error != nil)
    {
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

- (void)setMandatory:(BOOL)isMandatory {
    mandatory = isMandatory;
    if ([self window] != nil)
    {
        [cancelButton setTitle:mandatory ? NSLocalizedString(@"Quit", @"") : NSLocalizedString(@"Cancel", @"")];
        [[[self window] standardWindowButton:NSWindowCloseButton] setEnabled:!mandatory];
    }
}

- (IBAction)cancelButtonClicked:(id)sender
{
    if (mandatory)
        [NSApp terminate:nil];
    else
        [[self window] performClose:nil];
}

+ (NSURL *)applicationConfig
{
    return [[NSBundle mainBundle] URLForResource:[NSString stringWithFormat:@"%@/celestia.cfg", CELESTIA_RESOURCES_FOLDER] withExtension:nil];
}

+ (NSURL *)applicationDataDirectory
{
    return [[NSBundle mainBundle] URLForResource:[NSString stringWithFormat:@"%@", CELESTIA_RESOURCES_FOLDER] withExtension:nil];
}

@end
