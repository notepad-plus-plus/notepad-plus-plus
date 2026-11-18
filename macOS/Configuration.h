// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native configuration editors and user-defined syntax, created 2026-11-01.
#import <Cocoa/Cocoa.h>
#import "ScintillaView.h"
NSDictionary *NXDefaultConfiguration(void);
NSDictionary *NXDefaultPreferences(void);
NSDictionary *NXDefaultLanguage(void);
NSDictionary *NXDefaultTheme(void);
NSDictionary *NXDefaultStyle(void);
BOOL NXValidateStyles(id styles);
void NXApplyStyleOverrides(ScintillaView *view, NSArray *styles, NSString *lexer);
BOOL NXValidateConfiguration(id value);
BOOL NXValidatePreferences(id value);
void NXStyleLanguage(ScintillaView *view, NSDictionary *language);
void NXApplyTheme(ScintillaView *view, NSDictionary *theme, NSString *lexer, BOOL custom);
NSDictionary *NXLanguageForPath(NSDictionary *configuration, NSString *path);

// The dialog edits an isolated draft; the caller commits only after Apply.
@interface NXRecordEditor : NSObject <NSTableViewDataSource, NSTableViewDelegate>
- (instancetype)initWithKind:(NSString *)kind records:(NSArray *)records;
- (NSArray *)runForWindow:(NSWindow *)parent;
@end
