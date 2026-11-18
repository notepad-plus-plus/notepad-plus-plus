// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native find-in-files and replacement previews, created 2026-11-01.
#import <Cocoa/Cocoa.h>
NSArray *NXBoostMatches(NSString *text, NSDictionary *options, NSError **error);
NSString *NXDecodeText(NSData *data);
NSArray *NXSearchMatches(NSString *text, NSDictionary *options, NSError **error);
NSArray *NXSearchFiles(NSArray *inputs, NSDictionary *options, BOOL (^cancelled)(void), NSError **error);
@interface NXFileSearch : NSWindowController <NSTableViewDataSource, NSTableViewDelegate>
@property(copy) NSArray *(^scopeFiles)(NSInteger scope);
@property(copy) void (^navigate)(NSDictionary *result);
@property(copy) NSString *(^replaceResults)(NSArray *results);
@property(copy) void (^showBackups)(void);
- (void)showForWindow:(NSWindow *)parent;
- (void)showMatches:(NSArray *)matches document:(NSDictionary *)document parent:(NSWindow *)parent;
- (void)navigateBy:(NSInteger)direction;
@end
