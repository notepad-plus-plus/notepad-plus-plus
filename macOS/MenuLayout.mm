// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native menu visibility and order editor, created 2026-11-01.
#import "MenuLayout.h"
#import "Storage.h"

@implementation NXMenuLayout {
    NSMenu *_canonical;
    NSDictionary *_settings;
    NSMutableDictionary *_draft;
    NSArray *_nodes;
    NSOutlineView *_tree;
    NSWindow *_dialog;
    NSMutableDictionary *_shortcutDraft;
    NSPopUpButton *_shortcutChoice;
    NSTextField *_shortcutKey;
    NSArray *_shortcutModifiers;
    NSArray *_commands;
    NSTextField *_shortcutStatus;
}
static BOOL ValidLayout(NSDictionary *value) {
    if (![value isKindOfClass:NSDictionary.class] || ![value[@"order"] isKindOfClass:NSDictionary.class] ||
        ![value[@"hidden"] isKindOfClass:NSArray.class] || [value[@"hidden"] count] > 5000) return NO;
    for (id key in value[@"order"]) {
        id order = value[@"order"][key];
        if (![key isKindOfClass:NSString.class] || ![order isKindOfClass:NSArray.class] || [order count] > 1000) return NO;
        for (id item in order) if (![item isKindOfClass:NSString.class]) return NO;
    }
    for (id item in value[@"hidden"]) if (![item isKindOfClass:NSString.class]) return NO;
    if(value[@"shortcuts"]){
        if(![value[@"shortcuts"] isKindOfClass:NSDictionary.class]||[value[@"shortcuts"] count]>5000)return NO;
        for(id key in value[@"shortcuts"]){id shortcut=value[@"shortcuts"][key];if(![key isKindOfClass:NSString.class]||![shortcut isKindOfClass:NSDictionary.class]||![shortcut[@"key"] isKindOfClass:NSString.class]||[shortcut[@"key"] length]>1||![shortcut[@"modifiers"] isKindOfClass:NSNumber.class])return NO;}
    }
    return YES;
}
- (NSArray *)nodesForMenu:(NSMenu *)menu parent:(NSString *)parent {
    NSMutableArray *result = [NSMutableArray new];
    for (NSMenuItem *item in menu.itemArray) {
        if (item.separatorItem) continue;
        // AppKit injects system-only commands (for example Dictation) at runtime.
        // They cannot be recreated as ordinary application menu items.
        if (item.keyEquivalentModifierMask & NSEventModifierFlagFunction) continue;
        // Keys do not depend on translated display labels for executable commands.
        NSString *local = item.submenu ? [@"menu:" stringByAppendingString:item.title] :
            [NSString stringWithFormat:@"%@:%ld", NSStringFromSelector(item.action) ?: item.title, (long)item.tag];
        if(item.action==NSSelectorFromString(@"chooseBuiltinLanguage:")&&[item.representedObject isKindOfClass:NSString.class])local=[@"language:" stringByAppendingString:item.representedObject];
        NSString *key = [parent stringByAppendingFormat:@"/%@", local];
        BOOL locked = ([parent isEqual:@""] && result.count == 0) ||
            item.action == @selector(terminate:) || item.action == NSSelectorFromString(@"customizeMenus:") ||
            item.action == NSSelectorFromString(@"resetMenus:");
        [result addObject:@{@"key":key, @"parent":parent, @"title":item.title, @"locked":@(locked),
            @"item":item, @"children":item.submenu ? [self nodesForMenu:item.submenu parent:key] : @[]}];
    }
    return result;
}
- (instancetype)initWithMenu:(NSMenu *)menu settings:(NSDictionary *)settings {
    self = [super init];
    if (self) {
        _canonical = menu;
        _nodes = [self nodesForMenu:menu parent:@""];
        _settings = ValidLayout(settings) ? [settings copy] : @{@"order":@{}, @"hidden":@[]};
    }
    return self;
}
- (NSDictionary *)settings { return _settings; }
- (NSArray *)ordered:(NSArray *)nodes parent:(NSString *)parent layout:(NSDictionary *)layout {
    NSMutableArray *ordered = [NSMutableArray new];
    if (!parent.length && nodes.count) [ordered addObject:nodes[0]]; // macOS application menu stays first.
    for (NSString *key in layout[@"order"][parent]) for (NSDictionary *node in nodes)
        if ([node[@"key"] isEqual:key] && ![ordered containsObject:node]) [ordered addObject:node];
    for (NSDictionary *node in nodes) if (![ordered containsObject:node]) [ordered addObject:node];
    return ordered;
}
- (NSMenu *)render:(NSArray *)nodes parent:(NSString *)parent {
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@""];
    for (NSDictionary *node in [self ordered:nodes parent:parent layout:_settings]) {
        if (![node[@"locked"] boolValue] && [_settings[@"hidden"] containsObject:node[@"key"]]) continue;
        NSMenuItem *source = node[@"item"];
        NSMenuItem *copy = [[NSMenuItem alloc] initWithTitle:source.title action:source.action keyEquivalent:source.keyEquivalent];
        copy.target = source.target; copy.tag = source.tag; copy.keyEquivalentModifierMask = source.keyEquivalentModifierMask;
        copy.state = source.state; copy.enabled = source.enabled; copy.representedObject = source.representedObject;
        copy.identifier = node[@"key"];
        NSDictionary *shortcut=_settings[@"shortcuts"][node[@"key"]];
        if(shortcut){copy.keyEquivalent=shortcut[@"key"];copy.keyEquivalentModifierMask=[shortcut[@"modifiers"] unsignedLongLongValue];}
        if (source.submenu) {
            copy.submenu = [self render:node[@"children"] parent:node[@"key"]];
            if (!copy.submenu.numberOfItems) continue;
        }
        [menu addItem:copy];
    }
    return menu;
}
- (void)apply { NSApp.mainMenu = [self render:_nodes parent:@""]; }
- (BOOL)setLayout:(NSDictionary *)settings error:(NSError **)error {
    if (!ValidLayout(settings)) { if (error) *error = NXError(@"The menu layout is invalid."); return NO; }
    NSDictionary *previous = _settings;
    _settings = [settings copy];
    if (self.commit && !self.commit(error)) { _settings = previous; return NO; }
    [self apply]; return YES;
}
- (void)collectCommands:(NSArray *)nodes into:(NSMutableArray *)result {
    for(NSDictionary *node in nodes){NSMenuItem *item=node[@"item"];
        if(item.submenu)[self collectCommands:node[@"children"] into:result];
        else if(item.action){NSString *key=item.keyEquivalent;NSUInteger modifiers=item.keyEquivalentModifierMask;
            if(key.length&&![key isEqual:key.lowercaseString]){key=key.lowercaseString;modifiers|=NSEventModifierFlagShift;}
            [result addObject:@{@"id":node[@"key"],@"name":[NSString stringWithFormat:@"%@ — %@",[[node[@"parent"] stringByReplacingOccurrencesOfString:@"/menu:" withString:@" / "] stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@" / "]],item.title],@"key":key,@"modifiers":@(modifiers),@"locked":@(item.action==@selector(terminate:))}];
        }
    }
}
- (NSArray *)commands {NSMutableArray *commands=[NSMutableArray new];[self collectCommands:_nodes into:commands];return commands;}
- (BOOL)setShortcuts:(NSDictionary *)shortcuts error:(NSError **)error {
    NSMutableDictionary *layout=[_settings mutableCopy];layout[@"shortcuts"]=shortcuts;
    if(!ValidLayout(layout)){if(error)*error=NXError(@"Invalid shortcut settings.");return NO;}
    NSMutableDictionary *used=[NSMutableDictionary new];NSSet *ids=[NSSet setWithArray:[self.commands valueForKey:@"id"]];
    for(NSString *key in shortcuts)if(![ids containsObject:key]){if(error)*error=NXError(@"A shortcut refers to an unknown command.");return NO;}
    for(NSDictionary *command in self.commands){NSDictionary *shortcut=shortcuts[command[@"id"]] ?: command;NSString *key=shortcut[@"key"];NSUInteger modifiers=[shortcut[@"modifiers"] unsignedLongLongValue];
        NSUInteger allowed=NSEventModifierFlagCommand|NSEventModifierFlagControl|NSEventModifierFlagOption|NSEventModifierFlagShift;
        if((modifiers&~allowed)||(key.length&&[key characterAtIndex:0]<0xf700&&!(modifiers&(NSEventModifierFlagCommand|NSEventModifierFlagControl)))){if(error)*error=NXError(@"Text shortcuts require Command or Control. Use a function key for an unmodified shortcut.");return NO;}
        if([command[@"locked"] boolValue]&&(![key isEqual:command[@"key"]]||modifiers!=[command[@"modifiers"] unsignedLongLongValue])){if(error)*error=NXError(@"Command-Q is reserved for Quit.");return NO;}
        if(!key.length)continue;NSString *identity=[NSString stringWithFormat:@"%@:%lu",key.lowercaseString,(unsigned long)(modifiers|(![key isEqual:key.lowercaseString]?NSEventModifierFlagShift:0))];
        if(used[identity]){if(error)*error=NXError([NSString stringWithFormat:@"Shortcut conflict: %@ and %@. Clear one assignment before applying.",used[identity],command[@"name"]]);return NO;}used[identity]=command[@"name"];
    }
    return [self setLayout:layout error:error];
}
- (void)selectShortcut:(id)sender {
    NSDictionary *command=_commands[_shortcutChoice.indexOfSelectedItem],*shortcut=_shortcutDraft[command[@"id"]] ?: command;NSString *key=shortcut[@"key"];
    if(key.length&&[key characterAtIndex:0]>=NSF1FunctionKey&&[key characterAtIndex:0]<=NSF20FunctionKey)key=[NSString stringWithFormat:@"F%d",[key characterAtIndex:0]-NSF1FunctionKey+1];
    _shortcutKey.stringValue=key;NSUInteger masks[]={NSEventModifierFlagCommand,NSEventModifierFlagControl,NSEventModifierFlagOption,NSEventModifierFlagShift};
    for(NSUInteger i=0;i<4;++i)[_shortcutModifiers[i] setState:([shortcut[@"modifiers"] unsignedLongLongValue]&masks[i])?NSControlStateValueOn:NSControlStateValueOff];
}
- (void)assignShortcut:(id)sender {
    NSString *key=_shortcutKey.stringValue;NSUInteger modifiers=0,masks[]={NSEventModifierFlagCommand,NSEventModifierFlagControl,NSEventModifierFlagOption,NSEventModifierFlagShift};for(NSUInteger i=0;i<4;++i)if([_shortcutModifiers[i] state]==NSControlStateValueOn)modifiers|=masks[i];
    if(key.length>1&&[key.uppercaseString hasPrefix:@"F"]){NSInteger number=[[key substringFromIndex:1] integerValue];if(number>=1&&number<=20)key=[NSString stringWithFormat:@"%C",(unichar)(NSF1FunctionKey+number-1)];}
    if(key.length>1){_shortcutStatus.stringValue=@"Enter one character or F1–F20. Leave the key empty to clear it.";return;}
    if(![key isEqual:key.lowercaseString])modifiers|=NSEventModifierFlagShift;
    _shortcutDraft[_commands[_shortcutChoice.indexOfSelectedItem][@"id"]]=@{@"key":key.lowercaseString,@"modifiers":@(modifiers)};
    _shortcutStatus.stringValue=@"Assignment added to the draft. Apply checks conflicts and saves all assignments.";
}
- (void)resetShortcuts:(id)sender {[_shortcutDraft removeAllObjects];[self selectShortcut:nil];_shortcutStatus.stringValue=@"Default shortcuts restored in this draft.";}
- (void)acceptShortcuts:(id)sender {NSError *error=nil;if(![self setShortcuts:_shortcutDraft error:&error]){_shortcutStatus.stringValue=error.localizedDescription;return;}[NSApp stopModal];[_dialog orderOut:nil];}
- (void)editShortcutsForWindow:(NSWindow *)window {
    _commands=self.commands;_shortcutDraft=[(_settings[@"shortcuts"] ?: @{}) mutableCopy];
    _dialog=[[NSWindow alloc]initWithContentRect:NSMakeRect(0,0,720,300) styleMask:NSWindowStyleMaskTitled backing:NSBackingStoreBuffered defer:NO];_dialog.title=@"Shortcut Editor";_dialog.appearance=window.appearance;
    _shortcutChoice=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(16,245,688,30) pullsDown:NO];[_shortcutChoice addItemsWithTitles:[_commands valueForKey:@"name"]];_shortcutChoice.target=self;_shortcutChoice.action=@selector(selectShortcut:);[_dialog.contentView addSubview:_shortcutChoice];
    _shortcutKey=[[NSTextField alloc]initWithFrame:NSMakeRect(16,196,110,26)];_shortcutKey.placeholderString=@"Key or F1–F20";[_dialog.contentView addSubview:_shortcutKey];NSMutableArray *modifiers=[NSMutableArray new];NSUInteger i=0;
    for(NSString *name in @[@"Command",@"Control",@"Option",@"Shift"]){NSButton *b=[NSButton checkboxWithTitle:name target:nil action:nil];b.frame=NSMakeRect(140+i++*120,196,116,26);[modifiers addObject:b];[_dialog.contentView addSubview:b];}_shortcutModifiers=modifiers;
    _shortcutStatus=[NSTextField wrappingLabelWithString:@"Choose a command and set its key and modifiers. Assign updates the draft; Apply saves it. Clear a conflicting command before assigning its shortcut to another command."];_shortcutStatus.frame=NSMakeRect(16,74,688,70);[_dialog.contentView addSubview:_shortcutStatus];
    NSArray *labels=@[@"Assign",@"Reset All",@"Cancel",@"Apply"];SEL actions[]={@selector(assignShortcut:),@selector(resetShortcuts:),@selector(cancel:),@selector(acceptShortcuts:)};
    for(NSUInteger n=0;n<labels.count;++n){NSButton *b=[NSButton buttonWithTitle:labels[n] target:self action:actions[n]];b.frame=NSMakeRect(16+n*174,20,164,32);[_dialog.contentView addSubview:b];}
    [self selectShortcut:nil];[_dialog center];[NSApp runModalForWindow:_dialog];_dialog=nil;
}
- (void)reset {
    NSError *error = nil;
    NSMutableDictionary *layout=[_settings mutableCopy];layout[@"order"]=@{};layout[@"hidden"]=@[];
    if (![self setLayout:layout error:&error]) [NSApp presentError:error];
}
- (NSArray *)children:(NSDictionary *)node {
    return [self ordered:node ? node[@"children"] : _nodes parent:node ? node[@"key"] : @"" layout:_draft];
}
- (NSInteger)outlineView:(NSOutlineView *)outline numberOfChildrenOfItem:(id)item { return [self children:item].count; }
- (id)outlineView:(NSOutlineView *)outline child:(NSInteger)index ofItem:(id)item { return [self children:item][index]; }
- (BOOL)outlineView:(NSOutlineView *)outline isItemExpandable:(id)item { return [item[@"children"] count] > 0; }
- (NSView *)outlineView:(NSOutlineView *)outline viewForTableColumn:(NSTableColumn *)column item:(id)item {
    if ([column.identifier isEqual:@"show"]) {
        NSButton *button = [NSButton checkboxWithTitle:@"" target:self action:@selector(toggle:)];
        button.identifier = item[@"key"];
        button.state = [_draft[@"hidden"] containsObject:item[@"key"]] && ![item[@"locked"] boolValue] ? NSControlStateValueOff : NSControlStateValueOn;
        button.enabled = ![item[@"locked"] boolValue];
        return button;
    }
    return [NSTextField labelWithString:item[@"title"]];
}
- (void)toggle:(NSButton *)button {
    NSMutableArray *hidden = [_draft[@"hidden"] mutableCopy];
    [hidden removeObject:button.identifier];
    if (button.state == NSControlStateValueOff) [hidden addObject:button.identifier];
    _draft[@"hidden"] = hidden;
}
- (NSDictionary *)find:(NSString *)key nodes:(NSArray *)nodes {
    for (NSDictionary *node in nodes) {
        if ([node[@"key"] isEqual:key]) return node;
        NSDictionary *found = [self find:key nodes:node[@"children"]];
        if (found) return found;
    }
    return nil;
}
- (void)move:(NSButton *)sender {
    NSDictionary *node = [_tree itemAtRow:_tree.selectedRow];
    if (!node || [node[@"locked"] boolValue]) return;
    NSString *parent = node[@"parent"];
    NSDictionary *parentNode = parent.length ? [self find:parent nodes:_nodes] : nil;
    NSMutableArray *siblings = [[self children:parentNode] mutableCopy];
    NSInteger position = [siblings indexOfObject:node], next = position + sender.tag;
    if (next < 0 || next >= (NSInteger)siblings.count || [siblings[next][@"locked"] boolValue]) return;
    [siblings exchangeObjectAtIndex:position withObjectAtIndex:next];
    NSMutableDictionary *order = [_draft[@"order"] mutableCopy];
    order[parent] = [siblings valueForKey:@"key"]; _draft[@"order"] = order;
    [_tree reloadData]; [_tree expandItem:nil expandChildren:YES];
    [_tree selectRowIndexes:[NSIndexSet indexSetWithIndex:[_tree rowForItem:node]] byExtendingSelection:NO];
}
- (void)resetDraft:(id)sender { _draft[@"order"]=@{};_draft[@"hidden"]=@[]; [_tree reloadData]; }
- (void)accept:(id)sender {
    NSError *error = nil;
    if (![self setLayout:_draft error:&error]) { [NSApp presentError:error]; return; }
    [NSApp stopModal]; [_dialog orderOut:nil];
}
- (void)cancel:(id)sender { [NSApp abortModal]; [_dialog orderOut:nil]; }
- (void)customizeForWindow:(NSWindow *)window {
    _draft = [_settings mutableCopy];
    _dialog = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 600, 520) styleMask:NSWindowStyleMaskTitled backing:NSBackingStoreBuffered defer:NO];
    _dialog.title = @"Customize Menus"; _dialog.appearance = window.appearance;
    NSTextField *help = [NSTextField wrappingLabelWithString:@"Choose visible commands and reorder menus or commands with Move Up / Move Down. The NoteX menu, Quit, and layout recovery commands remain available."];
    help.frame = NSMakeRect(16, 456, 568, 48); [_dialog.contentView addSubview:help];
    NSScrollView *scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(16, 68, 568, 380)]; scroll.hasVerticalScroller = YES;
    _tree = [[NSOutlineView alloc] initWithFrame:scroll.bounds];
    NSTableColumn *title = [[NSTableColumn alloc] initWithIdentifier:@"title"]; title.title = @"Menu or Command"; title.width = 460;
    NSTableColumn *show = [[NSTableColumn alloc] initWithIdentifier:@"show"]; show.title = @"Show"; show.width = 60;
    [_tree addTableColumn:title]; [_tree addTableColumn:show]; _tree.outlineTableColumn = title; _tree.delegate = self; _tree.dataSource = self;
    scroll.documentView = _tree; [_dialog.contentView addSubview:scroll];
    NSArray *names = @[@"Move Up", @"Move Down", @"Reset", @"Cancel", @"Apply"];
    SEL actions[] = {@selector(move:), @selector(move:), @selector(resetDraft:), @selector(cancel:), @selector(accept:)};
    for (NSUInteger i = 0; i < names.count; ++i) {
        NSButton *button = [NSButton buttonWithTitle:names[i] target:self action:actions[i]];
        button.frame = NSMakeRect(16 + i * 114, 20, 108, 32); button.tag = i == 0 ? -1 : 1;
        if (i == 4) button.keyEquivalent = @"\r";
        if (i == 3) button.keyEquivalent = @"\e";
        [_dialog.contentView addSubview:button];
    }
    [_tree reloadData]; [_tree expandItem:nil expandChildren:YES];
    [_dialog center]; [NSApp runModalForWindow:_dialog];
    _dialog = nil; _tree = nil; _draft = nil;
}
@end
