// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Document ownership between native editor panes, added 2026-11-01.
@implementation NXApp (ViewTransfer)
- (void)activateDocument:(NXDocument *)doc {
    if(doc.secondaryOnly){if(!self.secondaryVisible)[self cloneView:nil];[self bindSecondary:doc];self.secondaryFocused=YES;doc.activeEditor=self.secondaryEditor;[self.window makeFirstResponder:self.secondaryEditor];}
    else [self.tabs selectTabViewItem:doc.tab];[self updateControls];
}
- (void)moveOtherView:(id)sender {
    NXDocument *doc=self.current;if(self.secondaryFocused){doc.secondaryOnly=NO;NXDocument *remaining=nil;for(NXDocument *candidate in self.documents)if(candidate.secondaryOnly){remaining=candidate;break;}if(remaining)[self bindSecondary:remaining];else[self closeSecondary:nil];[self.tabs selectTabViewItem:doc.tab];}
    else {doc.secondaryOnly=YES;[self cloneView:nil];NXDocument *primary=nil;for(NXDocument *candidate in self.documents)if(!candidate.secondaryOnly){primary=candidate;break;}if(!primary)primary=[self newDocument:nil];[self.tabs selectTabViewItem:primary.tab];self.secondaryFocused=YES;doc.activeEditor=self.secondaryEditor;[self.window makeFirstResponder:self.secondaryEditor];}
    [self tick:nil];
}
- (void)synchronizeAxis:(id)sender {
    if([sender tag]==1)self.syncVertical=!self.syncVertical;else self.syncHorizontal=!self.syncHorizontal;self.synchronizedScroll=self.syncVertical||self.syncHorizontal;[self tick:nil];
}
- (void)buildTransferMenus {
    NSMenu *view=[NSApp.mainMenu itemWithTitle:@"View"].submenu;[self item:view title:@"Move to Other View" action:@selector(moveOtherView:) key:@"" tag:0];[self item:view title:@"Synchronize Vertical Scrolling" action:@selector(synchronizeAxis:) key:@"" tag:1];[self item:view title:@"Synchronize Horizontal Scrolling" action:@selector(synchronizeAxis:) key:@"" tag:2];
}
@end
