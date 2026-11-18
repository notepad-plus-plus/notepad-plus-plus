// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Searchable native selection dialog, added 2026-11-01.
#import "Chooser.h"
@interface NXChooser : NSObject <NSTableViewDataSource,NSTableViewDelegate,NSSearchFieldDelegate>
@property NSArray *items;
@property NSMutableArray<NSNumber *> *indices;
@property NSTableView *table;
@property NSSearchField *search;
@property NSWindow *window;
@property NSInteger result;
@end
@implementation NXChooser
- (NSInteger)numberOfRowsInTableView:(NSTableView *)table { return self.indices.count; }
- (NSView *)tableView:(NSTableView *)table viewForTableColumn:(NSTableColumn *)column row:(NSInteger)row {
    NSTextField *label=[NSTextField labelWithString:self.items[self.indices[row].integerValue]];label.lineBreakMode=NSLineBreakByTruncatingMiddle;label.toolTip=label.stringValue;return label;
}
- (void)controlTextDidChange:(NSNotification *)note {self.indices=[NSMutableArray new];for(NSUInteger i=0;i<self.items.count;++i)if(!self.search.stringValue.length||[self.items[i] localizedCaseInsensitiveContainsString:self.search.stringValue])[self.indices addObject:@(i)];[self.table reloadData];if(self.indices.count)[self.table selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];}
- (void)accept:(id)sender {if(self.table.selectedRow<0)return;self.result=self.indices[self.table.selectedRow].integerValue;[NSApp stopModal];[self.window orderOut:nil];}
- (void)cancel:(id)sender {[NSApp abortModal];[self.window orderOut:nil];}
@end
NSInteger NXChoose(NSString *title, NSArray<NSString *> *items, NSWindow *parent) {
    NXChooser *owner=[NXChooser new];owner.items=items;owner.result=-1;
    owner.window=[[NSWindow alloc]initWithContentRect:NSMakeRect(0,0,620,420) styleMask:NSWindowStyleMaskTitled backing:NSBackingStoreBuffered defer:NO];owner.window.title=title;owner.window.appearance=parent.appearance;
    owner.search=[[NSSearchField alloc]initWithFrame:NSMakeRect(16,372,588,28)];owner.search.placeholderString=@"Filter";owner.search.delegate=owner;[owner.window.contentView addSubview:owner.search];
    owner.table=[NSTableView new];NSTableColumn *column=[[NSTableColumn alloc]initWithIdentifier:@"document"];column.width=560;[owner.table addTableColumn:column];owner.table.headerView=nil;owner.table.delegate=owner;owner.table.dataSource=owner;owner.table.target=owner;owner.table.doubleAction=@selector(accept:);
    NSScrollView *scroll=[[NSScrollView alloc]initWithFrame:NSMakeRect(16,64,588,296)];scroll.documentView=owner.table;scroll.hasVerticalScroller=YES;[owner.window.contentView addSubview:scroll];
    NSButton *cancel=[NSButton buttonWithTitle:@"Cancel" target:owner action:@selector(cancel:)];cancel.frame=NSMakeRect(400,16,96,32);cancel.keyEquivalent=@"\e";[owner.window.contentView addSubview:cancel];
    NSButton *open=[NSButton buttonWithTitle:@"Choose" target:owner action:@selector(accept:)];open.frame=NSMakeRect(508,16,96,32);open.keyEquivalent=@"\r";[owner.window.contentView addSubview:open];
    [owner controlTextDidChange:[NSNotification notificationWithName:NSControlTextDidChangeNotification object:owner.search]];[owner.window center];[owner.window makeFirstResponder:owner.search];[NSApp runModalForWindow:owner.window];return owner.result;
}
