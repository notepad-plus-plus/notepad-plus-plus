// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native input customization, added 2026-11-01.
#import "NativeEditor.h"
@interface NXContent : SCIContentView
@end
@implementation NXContent
- (NSMenu *)menuForEvent:(NSEvent *)event {
    NSView *view=self;while(view&&![view isKindOfClass:NXEditor.class])view=view.superview;
    [self.window makeFirstResponder:self];NXEditor *editor=(NXEditor *)view;return editor.editorContextMenu?editor.editorContextMenu():[super menuForEvent:event];
}
- (void)keyDown:(NSEvent *)event {
    if (![self hasMarkedText] && (event.keyCode == 36 || event.keyCode == 76) &&
        !(event.modifierFlags & (NSEventModifierFlagCommand | NSEventModifierFlagControl | NSEventModifierFlagOption | NSEventModifierFlagShift))) {
        NSView *view = self;
        while (view && ![view isKindOfClass:NXEditor.class]) view = view.superview;
        NXEditor *editor = (NXEditor *)view;
        if (editor.newline && editor.newline(editor)) return;
    }
    if(![self hasMarkedText]&&!(event.modifierFlags&(NSEventModifierFlagCommand|NSEventModifierFlagControl|NSEventModifierFlagOption))){NSView *view=self;while(view&&![view isKindOfClass:NXEditor.class])view=view.superview;NXEditor *editor=(NXEditor *)view;if(editor.typedKey&&editor.typedKey(editor,event.characters))return;}
    [super keyDown:event];
}
@end
@implementation NXEditor
+ (Class)contentViewClass { return NXContent.class; }
@end
