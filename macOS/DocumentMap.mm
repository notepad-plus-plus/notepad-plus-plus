// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native document overview, added 2026-11-01.
#import "DocumentMap.h"
@implementation NXDocumentMap
- (BOOL)isFlipped { return YES; }
- (void)drawRect:(NSRect)dirty {
    [NSColor.controlBackgroundColor setFill];NSRectFill(self.bounds);ScintillaView *view=self.editor;if(!view)return;
    auto lines=MAX(1,[view message:SCI_GETLINECOUNT]);CGFloat height=self.bounds.size.height,scale=height/lines;
    [NSColor.secondaryLabelColor setFill];
    // Sample at most one logical line per physical row for very large documents.
    sptr_t step=MAX(1,(sptr_t)(lines/MAX(1,height)));
    for(sptr_t line=0;line<lines;line+=step){auto start=[view message:SCI_POSITIONFROMLINE wParam:line],end=[view message:SCI_GETLINEENDPOSITION wParam:line],indent=[view message:SCI_GETLINEINDENTATION wParam:line];if(end>start){CGFloat x=MIN(self.bounds.size.width-8,4+indent*0.7),width=MIN(self.bounds.size.width-x-4,(end-start)*0.7);NSRectFill(NSMakeRect(x,line*scale,MAX(1,width),MAX(1,MIN(2,scale))));}}
    auto first=[view message:SCI_DOCLINEFROMVISIBLE wParam:[view message:SCI_GETFIRSTVISIBLELINE]],last=[view message:SCI_DOCLINEFROMVISIBLE wParam:[view message:SCI_GETFIRSTVISIBLELINE]+[view message:SCI_LINESONSCREEN]];
    if(last<first)last=lines;[[NSColor.selectedControlColor colorWithAlphaComponent:0.25] setFill];NSRectFill(NSMakeRect(0,first*scale,self.bounds.size.width,MAX(4,(last-first)*scale)));
}
- (void)mouseDown:(NSEvent *)event {NSPoint point=[self convertPoint:event.locationInWindow fromView:nil];auto lines=[self.editor message:SCI_GETLINECOUNT];auto line=MAX(0,MIN(lines-1,(sptr_t)(point.y/MAX(1,self.bounds.size.height)*lines)));[self.editor message:SCI_GOTOLINE wParam:line];[self.window makeFirstResponder:self.editor];self.needsDisplay=YES;}
- (BOOL)isAccessibilityElement { return YES; }
- (NSString *)accessibilityLabel { return @"Document Map"; }
@end
