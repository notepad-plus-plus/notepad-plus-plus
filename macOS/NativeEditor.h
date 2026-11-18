// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native input customization, added 2026-11-01.
#import "ScintillaView.h"
@interface NXEditor : ScintillaView
@property(copy) NSMenu *(^editorContextMenu)(void);
@property(copy) BOOL (^typedKey)(ScintillaView *, NSString *);
@property(copy) BOOL (^newline)(ScintillaView *);
@end
