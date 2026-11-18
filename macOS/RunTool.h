// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Explicit user-command runner, added 2026-11-01.
#import <Cocoa/Cocoa.h>
@interface NXCommandTask : NSObject
@property BOOL running;
@property int terminationStatus;
@property int processIdentifier;
- (void)terminate;
@end
@interface NXRunTool : NSWindowController
@property NSArray *commands;
@property(copy) BOOL (^saveCommands)(NSArray *);
@property(copy) NSDictionary *(^documentEnvironment)(void);
@property NXCommandTask *task;
@property NSMutableData *captured;
@property NSString *output;
- (void)showForWindow:(NSWindow *)parent;
- (BOOL)launchCommand:(NSString *)command environment:(NSDictionary *)environment error:(NSError **)error;
- (void)stop:(id)sender;
@end
