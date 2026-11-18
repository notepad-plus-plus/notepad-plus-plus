// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Wrapping document tabs, added 2026-11-01.
#import "TabView.h"
@interface NXTabCanvas : NSView
@end
@implementation NXTabCanvas
- (BOOL)isFlipped { return YES; }
@end
@interface NXTabButton : NSButton
@property(weak) NXTabView *owner;
@end
@implementation NXTabButton
- (NSMenu *)menuForEvent:(NSEvent *)event { return self.owner.documentMenu ? self.owner.documentMenu(self.tag) : nil; }
- (void)mouseDown:(NSEvent *)event {
    [self.owner selectTabViewItemAtIndex:self.tag];
    NSPoint initial = event.locationInWindow;
    BOOL dragged = NO;
    while (YES) {
        NSEvent *next = [self.window nextEventMatchingMask:NSEventMaskLeftMouseDragged | NSEventMaskLeftMouseUp];
        if (!next) break;
        if (hypot(next.locationInWindow.x-initial.x,next.locationInWindow.y-initial.y)>5) dragged=YES;
        if (next.type == NSEventTypeLeftMouseUp) {
            if (dragged && self.owner.moveDocument) {
                NSPoint point = [self.superview convertPoint:next.locationInWindow fromView:nil];
                for (NSView *view in self.superview.subviews) if ([view isKindOfClass:NXTabButton.class] && NSPointInRect(point, view.frame)) {
                    self.owner.moveDocument(self.tag, [(NSButton *)view tag]); break;
                }
            }
            break;
        }
    }
}
@end
@implementation NXTabView {
    NSButton *_previous, *_next, *_all;
    NSString *_signature;
}
- (instancetype)initWithFrame:(NSRect)frame {
    if ((self=[super initWithFrame:frame])) {
        self.tabViewType=NSNoTabsNoBorder;
        _tabScroll=[NSScrollView new]; _tabScroll.hasVerticalScroller=YES;_tabScroll.autohidesScrollers=YES;_tabScroll.drawsBackground=NO;
        _tabCanvas=[NXTabCanvas new];_tabScroll.documentView=_tabCanvas;[self addSubview:_tabScroll];
        _previous=[NSButton buttonWithTitle:@"‹" target:self action:@selector(previous:)];_previous.toolTip=@"Previous Tab";[_previous setAccessibilityLabel:@"Previous Tab"];
        _next=[NSButton buttonWithTitle:@"›" target:self action:@selector(next:)];_next.toolTip=@"Next Tab";[_next setAccessibilityLabel:@"Next Tab"];
        _all=[NSButton buttonWithTitle:@"☰" target:self action:@selector(choose:)];_all.toolTip=@"Open Documents";[_all setAccessibilityLabel:@"Open Documents"];
        for(NSButton *b in @[_previous,_next,_all])[self addSubview:b];
        _headerHeight=32;
    }return self;
}
- (NSRect)contentRect { NSRect r=self.bounds;r.size.height=MAX(0,r.size.height-self.headerHeight);if(self.isFlipped)r.origin.y=self.headerHeight;if(self.overview)r.size.width=MAX(0,r.size.width-120);return r; }
- (void)previous:(id)sender {if(!self.numberOfTabViewItems)return;NSInteger i=[self indexOfTabViewItem:self.selectedTabViewItem];for(NSInteger n=0;n<self.numberOfTabViewItems;++n){i=(i+self.numberOfTabViewItems-1)%self.numberOfTabViewItems;NSTabViewItem *item=[self tabViewItemAtIndex:i];if(![self.hiddenDocuments containsObject:item.identifier]){[self selectTabViewItem:item];break;}}}
- (void)next:(id)sender {if(!self.numberOfTabViewItems)return;NSInteger i=[self indexOfTabViewItem:self.selectedTabViewItem];for(NSInteger n=0;n<self.numberOfTabViewItems;++n){i=(i+1)%self.numberOfTabViewItems;NSTabViewItem *item=[self tabViewItemAtIndex:i];if(![self.hiddenDocuments containsObject:item.identifier]){[self selectTabViewItem:item];break;}}}
- (void)choose:(id)sender { if(self.chooseDocument)self.chooseDocument(); }
- (void)select:(NSButton *)sender { [self selectTabViewItemAtIndex:sender.tag]; }
- (void)close:(NSButton *)sender { if(self.closeDocument)self.closeDocument(sender.tag); }
- (void)selectTabViewItem:(NSTabViewItem *)item { [super selectTabViewItem:item];[self refreshTabs]; }
- (void)setFrameSize:(NSSize)size { [super setFrameSize:size];[self refreshTabs]; }
- (void)layout { [super layout];[self refreshTabs];self.selectedTabViewItem.view.frame=self.contentRect;if(self.overview){NSRect r=self.contentRect;r.origin.x=NSMaxX(r);r.size.width=120;self.overview.frame=r;} }
- (void)refreshTabs {
    if(!self.tabScroll)return;
    NSMutableString *signature=[NSMutableString stringWithFormat:@"%.0f/%.0f/%ld",self.bounds.size.width,self.bounds.size.height,(long)[self indexOfTabViewItem:self.selectedTabViewItem]];
    [signature appendFormat:@"%@",self.hiddenDocuments];
    for(NSTabViewItem *item in self.tabViewItems)[signature appendFormat:@"|%@|%@",item.identifier,item.label];
    if([_signature isEqual:signature])return;_signature=[signature copy];
    for(NSView *view in self.tabCanvas.subviews.copy)[view removeFromSuperview];
    CGFloat available=MAX(100,self.bounds.size.width-108),x=0,y=0;
    NSRect selected=NSZeroRect;
    NSInteger i=0;
    for(NSTabViewItem *item in self.tabViewItems) {
        if([self.hiddenDocuments containsObject:item.identifier]){++i;continue;}
        CGFloat width=MIN(220,MAX(128,[item.label sizeWithAttributes:@{NSFontAttributeName:[NSFont systemFontOfSize:12]}].width+36));
        width=MIN(width,available-14);
        if(x>0&&x+width>available-14){x=0;y+=28;}
        NXTabButton *button=[NXTabButton buttonWithTitle:item.label target:self action:@selector(select:)];button.owner=self;button.tag=i;button.frame=NSMakeRect(x,y,width-24,28);button.bezelStyle=NSBezelStyleRecessed;button.buttonType=NSButtonTypePushOnPushOff;button.state=item==self.selectedTabViewItem?NSControlStateValueOn:NSControlStateValueOff;button.lineBreakMode=NSLineBreakByTruncatingMiddle;
        button.toolTip=item.toolTip ?: item.label;[button setAccessibilityLabel:item.label];[self.tabCanvas addSubview:button];
        NSButton *close=[NSButton buttonWithTitle:@"×" target:self action:@selector(close:)];close.tag=i;close.frame=NSMakeRect(x+width-24,y,24,28);close.bordered=NO;[close setAccessibilityLabel:[@"Close " stringByAppendingString:item.label]];[self.tabCanvas addSubview:close];
        if(item==self.selectedTabViewItem)selected=button.frame;
        x+=width+2;++i;
    }
    CGFloat rowBudget=MAX(28,floor(MAX(0,MIN(112,self.bounds.size.height/4-4))/28)*28);
    self.headerHeight=MIN(y+28,rowBudget)+4;
    self.tabScroll.frame=NSMakeRect(0,self.isFlipped?0:self.bounds.size.height-self.headerHeight,available,self.headerHeight);
    self.tabCanvas.frame=NSMakeRect(0,0,available-14,MAX(y+28,self.headerHeight));
    _previous.frame=NSMakeRect(available,self.isFlipped?0:self.bounds.size.height-30,32,28);_next.frame=NSMakeRect(available+34,self.isFlipped?0:self.bounds.size.height-30,32,28);_all.frame=NSMakeRect(available+68,self.isFlipped?0:self.bounds.size.height-30,36,28);
    self.selectedTabViewItem.view.frame=self.contentRect;
    [self.tabCanvas scrollRectToVisible:selected];
}
@end
