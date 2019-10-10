/*
 * POConverter.m
 * 
 * Created by Da Woon Jung on Wed March 09 2005.
 * Copyright (c) 2005 dwj. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#import <Foundation/Foundation.h>


#define POCONV_DEFAULT_STRINGS_FILENAME    @"po.strings"
#define POCONV_DEBUG_LEVEL                 1

BOOL gIgnoreFuzzy;  /*! -n cmdline arg, YES = ignore fuzzy entries */
int gDebugLevel;    /*! -v cmdline arg, 0   = no console messages */
NSCharacterSet *gNewLineSet;
NSCharacterSet *gDelimitSet;


/*! Returns NO if line is a comment. range and result are modified. */
static BOOL readLine(NSString *string, NSRange *range, NSString **outString)
{
    // Using -lineRangeForRange: because it reliably detects all sorts of line endings
    NSRange lineRange = [string lineRangeForRange: *range];
    NSString *lineString = (NSNotFound != lineRange.location && lineRange.length > 0) ? [string substringWithRange: lineRange] : nil;
    range->location = NSMaxRange(lineRange);
    range->length = 0;
    *outString = lineString;

    // Detect comment lines
    return lineString ? ![lineString hasPrefix: @"#"] : YES;
}


/*! Scans data (from a po file) in ascii mode and gets the charset specification */
static NSStringEncoding getCharset(NSData *data)
{
    NSString *string = [[NSString alloc] initWithData:data encoding:NSASCIIStringEncoding];
    if (nil == string)
        return 0;

    CFStringEncoding enc = kCFStringEncodingUTF8;
    NSString *line = nil;
    NSString *encName = nil;
    NSRange encRange;
    NSRange range = NSMakeRange(0, 0);
    BOOL notComment = YES;

    while (YES)
    {
        notComment = readLine(string, &range, &line);
        if (nil == line)
            break;

        if (notComment)
        {
            encRange = [line rangeOfString: @"Content-Type:"];
            if (NSNotFound != encRange.location && encRange.length > 0)
            {
                encRange = [line rangeOfString: @"charset="];
                if (NSNotFound != encRange.location && encRange.length > 0)
                {
                    encName = [line substringWithRange: NSMakeRange(NSMaxRange(encRange), [line length] - NSMaxRange(encRange))];
                    if (encName)
                    {
                        encRange = [encName rangeOfString:@"\\n\"" options:NSBackwardsSearch];
                        if (NSNotFound != encRange.location && encRange.location > 0 && encRange.length > 0)
                            encName = [encName substringToIndex: encRange.location];

                        // Some special cases
                        if (NSOrderedSame == [encName caseInsensitiveCompare:@"gbk"])
                            enc = kCFStringEncodingGBK_95;
                        else if (NSOrderedSame == [encName caseInsensitiveCompare:@"ISO_8859-15"])
                            enc = kCFStringEncodingISOLatin9;
                        else
                            enc = CFStringConvertIANACharSetNameToEncoding((CFStringRef)encName);
                    }

                    if (gDebugLevel)
                        CFShow((__bridge CFTypeRef)(line));
                    break;
                }
            }
        }
    }

    return (kCFStringEncodingInvalidId == enc) ? NSUTF8StringEncoding : CFStringConvertEncodingToNSStringEncoding(enc);
}


static NSString *extractStringFromLine(NSString *line)
{
    NSMutableString *result = [NSMutableString stringWithCapacity: [line length]];
    NSString *aString;
    NSScanner *scanner = [NSScanner scannerWithString: line];
    NSRange escRange;
    NSMutableString *escString = [NSMutableString string];

    while (![scanner isAtEnd])
    {
        aString = nil;

        [scanner scanUpToString:@"\"" intoString: nil];
        if ([scanner scanString:@"\"" intoString: nil])
        {
            while (YES)
            {
                // Make sure we don't skip leading spaces (but skip newlines)
                NSCharacterSet *origSkip = [scanner charactersToBeSkipped];
                [scanner setCharactersToBeSkipped: gNewLineSet];
                [scanner scanUpToCharactersFromSet: gDelimitSet intoString: &aString];
                // Remove shorcut key ampersands
                // TODO: Recognize longer "(&x)" format used in
                //       non-roman localizations
                while ([scanner scanString: @"&" intoString: nil])
                {
                    if (aString)
                        [escString appendString: aString];
                    [scanner scanUpToCharactersFromSet: gDelimitSet intoString: &aString];
                }
//                [scanner scanUpToString:@"\"" intoString: &aString];
                [scanner setCharactersToBeSkipped: origSkip];

                if (aString)
                {
                    [escString appendString: aString];

                    escRange = [aString rangeOfString:@"\\" options:(NSBackwardsSearch|NSAnchoredSearch)];

                    if (NSNotFound != escRange.location && escRange.length > 0)
                    {
                        [scanner scanString:@"\"" intoString: &aString];
                        [escString appendString: aString];
                        aString = nil;
                        continue;
                    }
                }

                aString = nil;
                break;
            }

            if ([scanner scanString:@"\"" intoString: nil])
            {
                if (escString)
                    [result appendString: escString];
            }
        }
        else
            break;
    }

    return result;
}


/*! Get all parts of a quoted string (possibly broken across multiple lines) and returns the concatenated result. */
static NSString *getQuotedString(NSString *fileString, NSRange *range, NSString *line)
{
    NSMutableString *result = [NSMutableString stringWithString:extractStringFromLine(line)];
    NSString *aString;
    NSRange origRange;

    // Loop until no more string parts found
    BOOL notComment = YES;
    while (YES)
    {
        origRange = *range;
        notComment = readLine(fileString, range, &line);
        if (nil == line || ([line length] == 0))
            break;

        if (!notComment || [line hasPrefix: @"msgid"] || [line hasPrefix: @"msgstr"])
        {
            // Back up the cursor
            *range = origRange;
            break;
        }

        aString = extractStringFromLine(line);
        [result appendString: aString];
    }

    return ([result length] > 0) ? result : nil;
}


/*! Takes any comment line. If it is a flag line (starts with #, ) and contains the "fuzzy" flag, returns YES */
static BOOL isFuzzy(NSString *line)
{
    NSRange flagRange = [line rangeOfString:@"#," options:NSAnchoredSearch];
    if (NSNotFound != flagRange.location && flagRange.length > 0)
    {
        // Parse comma-delimited list of flags
        NSString *aString = [line substringWithRange: NSMakeRange(NSMaxRange(flagRange), [line length] - NSMaxRange(flagRange))];
        if (aString)
        {
            NSArray *flags;
            NSEnumerator *flagEnum;
            NSScanner *scanner;

            flags = [aString componentsSeparatedByString: @","];
            flagEnum = [flags objectEnumerator];
            while ((aString = [flagEnum nextObject]))
            {
                scanner = [NSScanner scannerWithString: aString];
                if ([scanner scanString:@"fuzzy" intoString: nil])
                {
                    // Check if fuzzy is the entire word
                    if ([scanner isAtEnd])
                        return YES;
                }
            }
        }
    }

    return NO;
}

#ifdef TARGET_CELESTIA
/*! Celestia: Returns YES if the line contains source file entries */
static BOOL isSource(NSString *line, BOOL *containsNonKde)
{
    BOOL isSource = NO;

    NSRange range = [line rangeOfString:@"#:" options:NSAnchoredSearch];
    if (NSNotFound != range.location && range.length > 0)
    {
        BOOL nonKdeFound = NO;
        BOOL kdeFound = NO;
        isSource = YES;

        NSString *aString = [line substringWithRange: NSMakeRange(NSMaxRange(range), [line length] - NSMaxRange(range))];

        if (aString)
        {
            NSArray *sources;
            NSEnumerator *sourceEnum;

            sources = [aString componentsSeparatedByString: @" "];
            sourceEnum = [sources objectEnumerator];

            while ((aString = [sourceEnum nextObject]))
            {
                if (0 == [aString length])
                    continue;
                range = [aString rangeOfString: @"kde"];
                if (NSNotFound == range.location)
                {
                    nonKdeFound = YES;
                    break;
                }
                else if (range.length > 0)
                    kdeFound = YES;
            }
        }

        if (nonKdeFound || !kdeFound)
            *containsNonKde = YES;
    }

    return isSource;
}
#endif /* TARGET_CELESTIA */

/*! Does all the work of reading, converting, and writing. Returns the number of lines successfully converted. */
static long convertData(NSData *data, NSStringEncoding encoding, NSFileHandle *ofh)
{
    long numConverted = 0;
    NSString *fileString = [[NSString alloc] initWithData:data encoding:encoding];
    if (nil == fileString)
        return 0;

    BOOL notComment = YES;
    NSString *line;
    __strong NSString **aString;
    NSRange range = NSMakeRange(0, 0);
    long fileStringLength = [fileString length];

    BOOL acceptEntry = YES;
#ifdef TARGET_CELESTIA
    BOOL parsingSources = NO;
    BOOL containsNonKde = NO;
#endif
    NSString *msgid = nil;
    NSString *msgstr = nil;

    while (range.location < fileStringLength)
    {
        notComment = readLine(fileString, &range, &line);
        if (nil == line)
            break;

        if (notComment)
        {
            aString = nil;

            if ([line hasPrefix: @"msgid"])
                aString = &msgid;
            else if ([line hasPrefix: @"msgstr"])
            {
                if (nil == msgid)
                {
                    if (gDebugLevel>2)
                        CFShow(@"Ignoring msgstr without msgid (probably the po header)");
                    continue;
                }
                aString = &msgstr;
            }
            else
            {
                // Blank line cancels fuzzy flag
                acceptEntry = YES;
            }

#ifdef TARGET_CELESTIA
//            if (parsingSources)
//                acceptEntry = containsNonKde;
            acceptEntry    = YES;
            parsingSources = NO;
            containsNonKde = NO;
#endif
            if (aString)
            {
                *aString = getQuotedString(fileString, &range, line);

                if (*aString == msgstr && msgid)
                {
                    if (nil == *aString)
                    {
                        if (gDebugLevel>1)
                            CFShow((CFStringRef)[NSString stringWithFormat:@"Ignoring msgid \"%@\" without msgstr", msgid]);
                        continue;
                    }

                    if (acceptEntry)
                    {
                        NSString *ofhString = [NSString stringWithFormat:@"\"%@\" = \"%@\";\n", msgid, msgstr];
                        if (ofhString)
                        {
                            NSData *ofhData;
                            if (numConverted > 0)
                            {
                                size_t ofhBufSize = [ofhString length]*sizeof(unichar);
                                unichar *ofhBuf = malloc(ofhBufSize);
                                [ofhString getCharacters: ofhBuf];  // gets utf-16 chars
                                ofhData = [NSData dataWithBytesNoCopy:ofhBuf length:ofhBufSize];
                            }
                            else
                            {
                                // Before overwriting an existing file, have to zero it
                                if (ofh)
                                    [ofh truncateFileAtOffset: 0];

                                // Output bom for first entry
                                ofhData = [ofhString dataUsingEncoding:NSUnicodeStringEncoding];
                            }
                            
                            [ofh writeData:ofhData];
                        }
                        else if (gDebugLevel)
                            CFShow(@"***Couldn't write data***");

                        ++numConverted;

                        if (gDebugLevel>2 && *aString)
                            CFShow((__bridge CFTypeRef)(*aString));
                    }
                    else if (gDebugLevel)
                        CFShow((CFStringRef)[NSString stringWithFormat:@"Ignoring entry \"%@\"", *aString]);

                    msgid = nil;
                    msgstr = nil;
                    acceptEntry = YES;
                }
            }
        }
        else
        {
#ifdef TARGET_CELESTIA
            // Celestia: filter out kde entries
            // This looks complicated, but is needed because
            // source entries (#:) may span multiple lines
            if (!parsingSources)
            {
                containsNonKde = NO;
                parsingSources = isSource(line, &containsNonKde);
            }
            else
            {
                BOOL tempKdeCheck = NO;
                if (isSource(line, &tempKdeCheck))
                    containsNonKde = containsNonKde || tempKdeCheck;
            }
#endif
            // Filter out fuzzy entries if specified by the caller
            if (gIgnoreFuzzy && isFuzzy(line))
                acceptEntry = NO;
        }
    }

    return numConverted;
}


static void usage()
{
    printf("Poconverter converts a GNU gettext po file to Mac OS-native\n"
           "UTF-16 Localized.strings format.\n"
           "\n"
           "Usage: Poconverter [-nv] file.po [destination filename]\n"
           "\n"
           "  -n        Ignore entries tagged with fuzzy flag\n"
           "  -s        Silent mode (existing file overwritten without warning)\n"
           "  -v        Verbose output on stderr\n"
           "\n");
}

static void ctrl_c_handler(int sig)
{
    fprintf(stderr, "\nGot Ctrl-C, cleaning up...\n");
    exit(1);
}

int main(int argc, const char **argv)
{
    @autoreleasepool {
        // Catch ctrl-c so we can release the autorelease pool
        signal(SIGINT, ctrl_c_handler);

        gIgnoreFuzzy = NO;
        gDebugLevel = 0;
        BOOL isSilent = NO;

        BOOL isErr = NO;
        NSProcessInfo *theProcess = [NSProcessInfo processInfo];
        NSArray *args = [theProcess arguments];
        NSString *arg;
        NSEnumerator *argEnum = [args objectEnumerator];
        [argEnum nextObject];   // advance past args[0] (the program full path)

        NSString *poFilename = nil;
        NSString *resultFilename = nil;
        NSFileHandle *ofh;

        do
        {
            if (argc < 2 || argc > 5)
            {
                usage(); break;
            }

            // Parse options
            long j;

            while ((arg = [argEnum nextObject]) && [arg hasPrefix: @"-"])
            {
                if ([arg length] > 1)
                {
                    for (j = 1; j < [arg length]; ++j)
                    {
                        switch ([arg characterAtIndex: j])
                        {
                            case 'n':
                                gIgnoreFuzzy = YES; break;
                            case 's':
                                isSilent = YES; break;
                            case 'v':
                                gDebugLevel = POCONV_DEBUG_LEVEL; break;
                            default:
                                isErr = YES;
                        }

                        if (isErr)
                            break;
                    }
                }
                else
                {
                    isErr = YES;
                    break;
                }
            }

            if (isErr)
            {
                usage(); break;
            }

            // Can't just have options, need filename(s) too
            if (nil == arg)//i == argc)
            {
                usage(); break;
            }

            poFilename = arg;

            // Check if po file exists and isn't a directory
            BOOL isDirectory;
            BOOL fileExists;
            NSFileManager *fm = [NSFileManager defaultManager];

            if (![fm fileExistsAtPath:poFilename isDirectory:&isDirectory] || isDirectory)
            {
                usage(); break;
            }

            // Destination path - can be either a filename or a directory
            // If unspecified, defaults to stdout
            resultFilename = [argEnum nextObject];

            if (resultFilename)
            {
                if ([argEnum nextObject])
                {
                    // Too many filenames
                    usage(); break;
                }

                // If directory, append default filename
                fileExists = [fm fileExistsAtPath:resultFilename isDirectory:&isDirectory];
                if (fileExists && isDirectory)
                {
                    resultFilename = [resultFilename stringByAppendingPathComponent:POCONV_DEFAULT_STRINGS_FILENAME];
                    fileExists = [fm fileExistsAtPath: resultFilename];
                }

                if (!fileExists)
                {
                    [fm createFileAtPath:resultFilename contents:nil attributes:nil];
                }

                if(!isSilent && fileExists)
                {
                    printf("\nDestination file exists, overwrite (y/n)? [y] ");
                    int confirm = getchar();
                    fflush(stdout);

                    switch (confirm)
                    {
                        case 'y':
                        case 'Y':
                        case '\n':
                            break;
                        default:
                            printf("Destination file not overwritten.\n\n");
                            exit(0);
                    }
                }

                ofh = [NSFileHandle fileHandleForWritingAtPath: resultFilename];
                // Delay truncating file and wiping it out until we actually
                // get something to write to
            }
            else
            {
                // Output filename not specified, use stdout
                ofh = [NSFileHandle fileHandleWithStandardOutput];
            }

            if (nil == ofh)
            {
                CFShow(@"***Could not write output. Please check if directory exists, and you have permissions.***");
                exit(1);
            }

            if (gDebugLevel)
                CFShow((CFStringRef)[NSString stringWithFormat:@"poFile='%@', result='%@', ignoreFuzzy=%d, debugLevel=%d, silent=%d", poFilename, (resultFilename ? resultFilename : @"stdout"), gIgnoreFuzzy, gDebugLevel, isSilent]);

            NSData *data = [NSData dataWithContentsOfMappedFile: poFilename];
            NSMutableCharacterSet *newLineSet = [[NSCharacterSet whitespaceAndNewlineCharacterSet] mutableCopy];
            [newLineSet formIntersectionWithCharacterSet:
             [[NSCharacterSet whitespaceCharacterSet] invertedSet]];
            gNewLineSet = newLineSet;
            gDelimitSet = [NSCharacterSet characterSetWithCharactersInString: @"&\""];

            // First detect the file encoding
            NSStringEncoding encoding = getCharset(data);
            if (gDebugLevel)
                CFShow((CFStringRef)[NSString stringWithFormat:@"NSStringEncoding = %#lx", encoding]);

            // Reread the file again, in the correct encoding
            long numConverted = 0;
            if (encoding)
            {
                numConverted = convertData(data, encoding, ofh);
            }

            if (gDebugLevel)
                CFShow((CFStringRef)[NSString stringWithFormat:@"%ld entries converted.", numConverted]);
        } while (NO);
    }

    return 0;
}
