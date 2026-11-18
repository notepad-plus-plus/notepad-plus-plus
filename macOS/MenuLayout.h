// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
#import <Cocoa/Cocoa.h>
@interface NXMenuLayout : NSObject <NSOutlineViewDataSource, NSOutlineViewDelegate>
@property(nonatomic, readonly) NSDictionary *settings;
@property(copy) BOOL (^commit)(NSError **error);
- (instancetype)initWithMenu:(NSMenu *)menu settings:(NSDictionary *)settings;
- (void)apply;
- (BOOL)setLayout:(NSDictionary *)settings error:(NSError **)error;
- (void)customizeForWindow:(NSWindow *)window;
- (void)reset;
- (NSArray *)commands;
- (void)editShortcutsForWindow:(NSWindow *)window;
- (BOOL)setShortcuts:(NSDictionary *)shortcuts error:(NSError **)error;
@end
