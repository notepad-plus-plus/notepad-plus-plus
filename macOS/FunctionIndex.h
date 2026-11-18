// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
#import <Foundation/Foundation.h>

// Positions use UTF-8 bytes, matching Scintilla rather than NSString UTF-16 units.
NSArray<NSDictionary *> *NXFunctionIndex(NSData *text, NSString *ruleFile, NSError **error);
NSString *NXFunctionRule(NSString *path, NSString *lexer);
