// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Wrapping document tabs, added 2026-11-01.
#import <Cocoa/Cocoa.h>
@interface NXTabView : NSTabView
@property(copy) void (^closeDocument)(NSInteger);
@property(copy) void (^moveDocument)(NSInteger, NSInteger);
@property(copy) void (^chooseDocument)(void);
@property(copy) NSMenu *(^documentMenu)(NSInteger);
@property NSScrollView *tabScroll;
@property NSView *tabCanvas;
@property CGFloat headerHeight;
@property NSView *overview;
@property NSSet *hiddenDocuments;
- (void)refreshTabs;
- (void)previous:(id)sender;
- (void)next:(id)sender;
@end
