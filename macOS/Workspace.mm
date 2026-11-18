// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Three native project panels, folder browsing, and Function List, created 2026-11-01.
#import "Workspace.h"
#import "Storage.h"
#import "FunctionIndex.h"

NSMutableDictionary *NXProjectNode(NSString *kind, NSString *name, NSString *path) {
    return [@{@"id":NSUUID.UUID.UUIDString, @"kind":kind, @"name":name, @"path":path ?: @"", @"children":[NSMutableArray new]} mutableCopy];
}
BOOL NXValidateProject(id node, NSUInteger *count, unsigned depth) {
    if (depth > 64 || ++*count > 20000 || ![node isKindOfClass:NSDictionary.class]) return NO;
    for (NSString *key in @[@"id", @"kind", @"name", @"path"]) if (![node[key] isKindOfClass:NSString.class]) return NO;
    if (![@[@"workspace", @"project", @"folder", @"file"] containsObject:node[@"kind"]] || ![node[@"children"] isKindOfClass:NSArray.class]) return NO;
    if ([node[@"kind"] isEqual:@"file"] && [node[@"children"] count]) return NO;
    for (id child in node[@"children"]) if (!NXValidateProject(child, count, depth + 1)) return NO;
    return YES;
}
static NSMutableDictionary *MutableNode(NSDictionary *node) {
    NSMutableDictionary *copy = [node mutableCopy];
    NSMutableArray *children = [NSMutableArray new];
    for (NSDictionary *child in node[@"children"]) [children addObject:MutableNode(child)];
    copy[@"children"] = children; return copy;
}
static NSMutableDictionary *ReadNode(NSXMLElement *element, NSString *base, unsigned depth, NSUInteger *count, NSError **error) {
    NSDictionary *types = @{@"Project":@"project", @"Folder":@"folder", @"File":@"file"};
    NSString *kind = types[element.name], *name = [element attributeForName:@"name"].stringValue;
    if (!kind || !name.length || depth > 64 || ++*count > 20000) { if (error) *error = NXError(@"The workspace contains an invalid or excessively large project tree."); return nil; }
    NSString *path = @"";
    if ([kind isEqual:@"file"]) {
        path = [name stringByReplacingOccurrencesOfString:@"\\" withString:@"/"];
        // Windows drive paths are retained so the user can explicitly relink them.
        if (!path.isAbsolutePath && !(path.length > 1 && [path characterAtIndex:1] == ':')) path = [base stringByAppendingPathComponent:path].stringByStandardizingPath;
        name = path.lastPathComponent;
    }
    NSMutableDictionary *node = NXProjectNode(kind, name, path);
    for (NSXMLNode *child in element.children) if (child.kind == NSXMLElementKind) {
        if ([kind isEqual:@"file"] || [child.name isEqual:@"Project"]) { if (error) *error = NXError(@"Invalid project hierarchy."); return nil; }
        NSMutableDictionary *nested = ReadNode((NSXMLElement *)child, base, depth + 1, count, error);
        if (!nested) return nil;
        [node[@"children"] addObject:nested];
    }
    return node;
}
NSMutableDictionary *NXReadWorkspace(NSString *path, NSError **error) {
    NSData *data = [NSData dataWithContentsOfFile:path options:0 error:error];
    if (!data) return nil;
    if (data.length > 8 * 1024 * 1024) { if (error) *error = NXError(@"The workspace XML is too large."); return nil; }
    NSXMLDocument *xml = [[NSXMLDocument alloc] initWithData:data options:NSXMLNodeLoadExternalEntitiesNever error:error];
    if (!xml || xml.DTD || ![xml.rootElement.name isEqual:@"NotepadPlus"]) { if (error && !*error) *error = NXError(@"This is not a supported Notepad++ workspace XML file."); return nil; }
    NSMutableDictionary *root = NXProjectNode(@"workspace", path.lastPathComponent, @""); NSUInteger count = 0;
    for (NSXMLNode *child in xml.rootElement.children) if (child.kind == NSXMLElementKind) {
        if (![child.name isEqual:@"Project"]) { if (error) *error = NXError(@"A workspace must contain Project elements."); return nil; }
        NSMutableDictionary *project = ReadNode((NSXMLElement *)child, path.stringByDeletingLastPathComponent, 0, &count, error);
        if (!project) return nil;
        [root[@"children"] addObject:project];
    }
    return root;
}
static NSString *RelativePath(NSString *path, NSString *base) {
    if (!path.isAbsolutePath) return path;
    NSArray *a = path.stringByStandardizingPath.pathComponents, *b = base.stringByStandardizingPath.pathComponents;
    NSUInteger same = 0; while (same < a.count && same < b.count && [a[same] isEqual:b[same]]) ++same;
    NSMutableArray *parts = [NSMutableArray new];
    for (NSUInteger i = same; i < b.count; ++i) [parts addObject:@".."];
    for (NSUInteger i = same; i < a.count; ++i) [parts addObject:a[i]];
    return [NSString pathWithComponents:parts];
}
static NSXMLElement *WriteNode(NSDictionary *node, NSString *base) {
    NSString *kind = node[@"kind"];
    NSString *tag = [kind isEqual:@"file"] ? @"File" : [kind isEqual:@"project"] ? @"Project" : @"Folder";
    NSXMLElement *element = [[NSXMLElement alloc] initWithName:tag];
    [element addAttribute:[NSXMLNode attributeWithName:@"name" stringValue:[kind isEqual:@"file"] ? RelativePath(node[@"path"], base) : node[@"name"]]];
    for (NSDictionary *child in node[@"children"]) [element addChild:WriteNode(child, base)];
    return element;
}
BOOL NXWriteWorkspace(NSDictionary *root, NSString *path, NSError **error) {
    NSUInteger count = 0;
    if (!NXValidateProject(root, &count, 0)) { if (error) *error = NXError(@"The project tree is invalid."); return NO; }
    NSXMLElement *element = [[NSXMLElement alloc] initWithName:@"NotepadPlus"];
    for (NSDictionary *child in root[@"children"]) [element addChild:WriteNode(child, path.stringByDeletingLastPathComponent)];
    NSXMLDocument *document = [[NSXMLDocument alloc] initWithRootElement:element]; document.version = @"1.0"; document.characterEncoding = @"UTF-8";
    return NXWrite([document XMLDataWithOptions:NSXMLNodePrettyPrint], path, error);
}

@implementation NXWorkspace {
    NSMutableArray *_projects;
    NSMutableArray *_workspacePaths;
    NSMutableArray *_workspaceHashes;
    NSMutableArray *_dirty;
    NSMutableArray *_folders;
    NSArray *_symbols;
    NSArray *_rows;
    NSArray *_folderRows;
    NSPopUpButton *_panel;
    NSSearchField *_filter;
    NSOutlineView *_tree;
    NSTextField *_status;
    NSMutableArray<NSButton *> *_buttons;
    NSOperationQueue *_queue;
    NSData *_lastText;
    NSString *_lastPath, *_lastLexer, *_ruleOverride;
    NSUInteger _generation;
    BOOL _sorted;
    NSInteger _mode;
}
- (instancetype)initWithState:(NSDictionary *)state {
    self = [super init]; if (!self) return nil;
    _projects = [NSMutableArray new]; _workspacePaths = [NSMutableArray new]; _workspaceHashes = [NSMutableArray new]; _dirty = [NSMutableArray new]; _folders = [NSMutableArray new];
    NSArray *saved = [state[@"projects"] isKindOfClass:NSArray.class] ? state[@"projects"] : @[];
    for (NSUInteger i = 0; i < 3; ++i) {
        NSUInteger count = 0;
        NSDictionary *record = i < saved.count && [saved[i] isKindOfClass:NSDictionary.class] ? saved[i] : @{};
        [_projects addObject:NXValidateProject(record[@"root"], &count, 0) ? MutableNode(record[@"root"]) : NXProjectNode(@"workspace", [NSString stringWithFormat:@"Workspace %lu", (unsigned long)i + 1], @"")];
        [_workspacePaths addObject:[record[@"path"] isKindOfClass:NSString.class] ? record[@"path"] : @""];
        [_workspaceHashes addObject:[record[@"hash"] isKindOfClass:NSData.class] ? record[@"hash"] : NSData.data];
        [_dirty addObject:@([record[@"dirty"] respondsToSelector:@selector(boolValue)] && [record[@"dirty"] boolValue])];
    }
    if ([state[@"folders"] isKindOfClass:NSArray.class]) for (id path in state[@"folders"]) if ([path isKindOfClass:NSString.class]) [_folders addObject:path];
    _ruleOverride = [state[@"ruleOverride"] isKindOfClass:NSString.class] ? state[@"ruleOverride"] : @"";
    _sorted = [state[@"sorted"] respondsToSelector:@selector(boolValue)] && [state[@"sorted"] boolValue];
    _mode = [state[@"panel"] respondsToSelector:@selector(integerValue)] ? std::clamp<NSInteger>([state[@"panel"] integerValue], 0, 4) : 0;
    _symbols = @[]; _rows = @[]; _queue = [NSOperationQueue new]; _queue.maxConcurrentOperationCount = 1;
    return self;
}
- (NSArray *)projects { return _projects; }
- (NSArray *)symbols { return _symbols; }
- (NSDictionary *)record {
    NSMutableArray *projects = [NSMutableArray new];
    for (NSUInteger i = 0; i < 3; ++i) [projects addObject:@{@"root":_projects[i], @"path":_workspacePaths[i], @"hash":_workspaceHashes[i], @"dirty":_dirty[i]}];
    // Deep immutable copy: subsequent tree edits must not mutate the last saved snapshot.
    NSDictionary *value = @{@"projects":projects, @"folders":_folders, @"panel":@(_mode), @"sorted":@(_sorted), @"ruleOverride":_ruleOverride};
    NSData *data = [NSPropertyListSerialization dataWithPropertyList:value format:NSPropertyListBinaryFormat_v1_0 options:0 error:nil];
    return [NSPropertyListSerialization propertyListWithData:data options:NSPropertyListImmutable format:nil error:nil];
}
- (NSButton *)button:(NSString *)title selector:(SEL)action tag:(NSInteger)tag {
    NSButton *button = [NSButton buttonWithTitle:title target:self action:action]; button.tag = tag; return button;
}
- (void)loadView {
    self.view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 290, 600)];
    _panel = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
    [_panel addItemsWithTitles:@[@"Function List", @"Project Panel 1", @"Project Panel 2", @"Project Panel 3", @"Folder as Workspace"]];
    [_panel selectItemAtIndex:_mode]; _panel.target = self; _panel.action = @selector(panelChanged:);
    _filter = [[NSSearchField alloc] initWithFrame:NSZeroRect]; _filter.placeholderString = @"Filter"; _filter.delegate = self;
    _tree = [[NSOutlineView alloc] initWithFrame:NSZeroRect];
    NSTableColumn *column = [[NSTableColumn alloc] initWithIdentifier:@"name"]; column.width = 270;
    [_tree addTableColumn:column]; _tree.outlineTableColumn = column; _tree.headerView = nil; _tree.delegate = self; _tree.dataSource = self;
    _tree.target = self; _tree.doubleAction = @selector(activate:); _tree.rowHeight = 23;
    NSScrollView *scroll = [[NSScrollView alloc] initWithFrame:NSZeroRect]; scroll.documentView = _tree; scroll.hasVerticalScroller = YES; scroll.hasHorizontalScroller = YES; scroll.autohidesScrollers = YES;
    _status = [NSTextField wrappingLabelWithString:@""];
    NSArray *names = @[@"Add", @"Folder", @"Files", @"Remove", @"Rename", @"↑", @"↓", @"Open XML", @"Save XML", @"Refresh", @"Rules…", @"A–Z", @"Relink…"];
    SEL actions[] = {@selector(add:), @selector(addFolder:), @selector(addFiles:), @selector(remove:), @selector(rename:), @selector(move:), @selector(move:), @selector(openWorkspace:), @selector(exportWorkspace:), @selector(refresh:), @selector(rules:), @selector(sort:), @selector(relink:)};
    _buttons = [NSMutableArray new];
    for (NSUInteger i = 0; i < names.count; ++i) [_buttons addObject:[self button:names[i] selector:actions[i] tag:i == 5 ? -1 : 1]];
    NSStackView *row1 = [NSStackView stackViewWithViews:[_buttons subarrayWithRange:NSMakeRange(0, 4)]];
    NSStackView *row2 = [NSStackView stackViewWithViews:[_buttons subarrayWithRange:NSMakeRange(4, 3)]];
    NSStackView *row3 = [NSStackView stackViewWithViews:[_buttons subarrayWithRange:NSMakeRange(7, 3)]];
    NSStackView *row4 = [NSStackView stackViewWithViews:[_buttons subarrayWithRange:NSMakeRange(10, 3)]];
    NSStackView *stack = [NSStackView stackViewWithViews:@[_panel, _filter, scroll, row1, row2, row3, row4, _status]];
    stack.orientation = NSUserInterfaceLayoutOrientationVertical; stack.alignment = NSLayoutAttributeLeading; stack.spacing = 6; stack.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:stack];
    [NSLayoutConstraint activateConstraints:@[[stack.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor constant:6], [stack.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor constant:-6],
        [stack.topAnchor constraintEqualToAnchor:self.view.topAnchor constant:6], [stack.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor constant:-6],
        [scroll.widthAnchor constraintEqualToAnchor:stack.widthAnchor], [scroll.heightAnchor constraintGreaterThanOrEqualToConstant:120],
        [_filter.widthAnchor constraintEqualToAnchor:stack.widthAnchor], [_panel.widthAnchor constraintEqualToAnchor:stack.widthAnchor], [_status.widthAnchor constraintEqualToAnchor:stack.widthAnchor]]];
    [scroll setContentHuggingPriority:1 forOrientation:NSLayoutConstraintOrientationVertical];
    [self refresh:nil];
}
- (void)notifyChange { if (self.changed) self.changed(); }
- (void)markChanged {
    if (_mode >= 1 && _mode <= 3) _dirty[_mode - 1] = @YES;
    [self refreshRows]; [self notifyChange];
}
- (NSInteger)activePanel { return _mode; }
- (void)addFolderRoot:(NSString *)path {if(![_folders containsObject:path])[_folders addObject:path];[self showPanel:4];[self refresh:nil];[self notifyChange];}
- (void)showPanel:(NSInteger)panel { (void)self.view; [_panel selectItemAtIndex:std::clamp<NSInteger>(panel, 0, 4)]; [self panelChanged:nil]; }
- (void)panelChanged:(id)sender { _mode = _panel.indexOfSelectedItem; [self refresh:nil]; [self notifyChange]; }
- (void)controlTextDidChange:(NSNotification *)notification { [self refreshRows]; [_tree expandItem:nil expandChildren:YES]; }
- (NSMutableDictionary *)selected {
    NSDictionary *selected = [_tree itemAtRow:_tree.selectedRow];
    if (!selected || _mode < 1 || _mode > 3) return nil;
    return [self find:selected[@"id"] root:_projects[_mode - 1]];
}
- (NSMutableDictionary *)find:(NSString *)identifier root:(NSMutableDictionary *)root {
    if ([root[@"id"] isEqual:identifier]) return root;
    for (NSMutableDictionary *node in root[@"children"]) { NSMutableDictionary *result = [self find:identifier root:node]; if (result) return result; }
    return nil;
}
- (NSMutableDictionary *)parentOf:(NSString *)identifier root:(NSMutableDictionary *)root {
    for (NSMutableDictionary *node in root[@"children"]) {
        if ([node[@"id"] isEqual:identifier]) return root;
        NSMutableDictionary *parent = [self parentOf:identifier root:node]; if (parent) return parent;
    }
    return nil;
}
- (NSMutableDictionary *)container {
    if (_mode < 1 || _mode > 3) return nil;
    NSMutableDictionary *node = [self selected] ?: _projects[_mode - 1];
    if ([node[@"kind"] isEqual:@"file"]) node = [self parentOf:node[@"id"] root:_projects[_mode - 1]];
    if ([node[@"kind"] isEqual:@"workspace"]) {
        if (![node[@"children"] count]) [node[@"children"] addObject:NXProjectNode(@"project", @"Project", @"")];
        node = node[@"children"][0];
    }
    return node;
}
- (NSArray *)filterNodes:(NSArray *)nodes {
    NSString *filter = _filter.stringValue;
    if (!filter.length) return nodes;
    NSMutableArray *result = [NSMutableArray new];
    for (NSDictionary *node in nodes) {
        if ([node[@"name"] localizedCaseInsensitiveContainsString:filter]) [result addObject:node];
        else {
            NSArray *children = [self filterNodes:node[@"children"] ?: @[]];
            if (children.count) { NSMutableDictionary *copy = [node mutableCopy]; copy[@"children"] = children; [result addObject:copy]; }
        }
    }
    return result;
}
- (void)refreshRows {
    if (!self.isViewLoaded) return;
    NSArray *base = @[];
    if (_mode == 0) {
        NSMutableArray *roots = [NSMutableArray new]; NSMutableDictionary *groups = [NSMutableDictionary new];
        for (NSDictionary *symbol in _symbols) {
            NSMutableDictionary *node = [symbol mutableCopy]; node[@"children"] = [NSMutableArray new];
            NSString *scope = symbol[@"scope"];
            if ([symbol[@"kind"] isEqual:@"class"]) groups[scope.length ? [scope stringByAppendingFormat:@"::%@", symbol[@"name"]] : symbol[@"name"]] = node;
            if (scope.length) {
                NSMutableDictionary *parent = groups[scope];
                if (!parent) { parent = [@{@"name":scope, @"kind":@"class", @"position":symbol[@"position"], @"children":[NSMutableArray new]} mutableCopy]; groups[scope] = parent; [roots addObject:parent]; }
                [parent[@"children"] addObject:node];
            } else [roots addObject:node];
        }
        if (_sorted) {
            NSSortDescriptor *sort = [NSSortDescriptor sortDescriptorWithKey:@"name" ascending:YES selector:@selector(localizedCaseInsensitiveCompare:)];
            [roots sortUsingDescriptors:@[sort]];
            for (NSMutableDictionary *group in groups.allValues) [group[@"children"] sortUsingDescriptors:@[sort]];
        }
        base = roots;
    } else if (_mode <= 3) base = @[_projects[_mode - 1]];
    else base = _folderRows ?: @[];
    _rows = [self filterNodes:base];
    [_tree reloadData];
    for (NSUInteger i = 0; i < _buttons.count; ++i) _buttons[i].hidden = _mode == 0 ? ![@[@9, @10, @11] containsObject:@(i)] : _mode == 4 ? ![@[@0, @3, @9] containsObject:@(i)] : [@[@10, @11] containsObject:@(i)];
    if (_mode >= 1 && _mode <= 3) _status.stringValue = [_dirty[_mode - 1] boolValue] ? @"Workspace changes are recovered automatically. Save XML to export them." : @"Double-click a file to open it. Remove only removes the reference.";
}
- (void)updateFunctions:(NSData *)text path:(NSString *)path lexer:(NSString *)lexer {
    if (_mode != 0) return;
    if ([text isEqual:_lastText] && [path isEqual:_lastPath] && [lexer isEqual:_lastLexer]) return;
    _lastText = [text copy]; _lastPath = [path copy]; _lastLexer = [lexer copy];
    NSString *rule = _ruleOverride.length ? _ruleOverride : NXFunctionRule(path, lexer);
    NSUInteger generation = ++_generation;
    [_queue cancelAllOperations];
    if (!rule) { _symbols = @[]; _status.stringValue = @"No rule for this language. Choose Rules… to load a Function List XML parser."; [self refreshRows]; return; }
    _status.stringValue = @"Indexing functions…";
    __weak NXWorkspace *weakSelf = self;
    [_queue addOperationWithBlock:^{
        NSError *error = nil; NSArray *result = NXFunctionIndex(text, rule, &error);
        [NSOperationQueue.mainQueue addOperationWithBlock:^{
            NXWorkspace *owner = weakSelf;
            if (!owner || generation != owner->_generation) return;
            owner->_symbols = result ?: @[];
            owner->_status.stringValue = error ? error.localizedDescription : [NSString stringWithFormat:@"%lu symbols · %@", (unsigned long)result.count, rule.lastPathComponent];
            [owner refreshRows];
        }];
    }];
}
- (NSInteger)outlineView:(NSOutlineView *)view numberOfChildrenOfItem:(id)item { return item ? [item[@"children"] count] : _rows.count; }
- (id)outlineView:(NSOutlineView *)view child:(NSInteger)index ofItem:(id)item { return item ? item[@"children"][index] : _rows[index]; }
- (BOOL)outlineView:(NSOutlineView *)view isItemExpandable:(id)item { return [item[@"children"] count] > 0; }
- (NSView *)outlineView:(NSOutlineView *)view viewForTableColumn:(NSTableColumn *)column item:(id)item {
    NSString *kind = item[@"kind"], *prefix = [kind isEqual:@"file"] ? @"▧ " : [kind isEqual:@"function"] ? @"ƒ " : [kind isEqual:@"class"] ? @"C " : @"▸ ";
    NSTextField *label = [NSTextField labelWithString:[prefix stringByAppendingString:item[@"name"]]];
    label.toolTip = item[@"path"];
    if ([kind isEqual:@"file"] && ![NSFileManager.defaultManager fileExistsAtPath:item[@"path"]]) { label.textColor = NSColor.secondaryLabelColor; label.toolTip = [@"Missing file: " stringByAppendingString:item[@"path"]]; }
    return label;
}
- (void)activate:(id)sender {
    NSDictionary *node = [_tree itemAtRow:_tree.clickedRow >= 0 ? _tree.clickedRow : _tree.selectedRow];
    if (!node) return;
    if (_mode == 0) { if (self.goToPosition) self.goToPosition([node[@"position"] integerValue]); }
    else if ([node[@"kind"] isEqual:@"file"]) { if (self.openFile) self.openFile(node[@"path"]); }
    else if ([_tree isItemExpanded:node]) [_tree collapseItem:node]; else [_tree expandItem:node];
}
- (NSString *)askName:(NSString *)title value:(NSString *)value {
    NSAlert *alert = [NSAlert new]; alert.messageText = title;
    NSTextField *field = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 280, 26)]; field.stringValue = value; alert.accessoryView = field;
    [alert addButtonWithTitle:@"OK"]; [alert addButtonWithTitle:@"Cancel"];
    return [alert runModal] == NSAlertFirstButtonReturn ? [field.stringValue stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet] : nil;
}
- (void)addProjectNamed:(NSString *)name {
    if (_mode < 1 || _mode > 3 || !name.length) return;
    [_projects[_mode - 1][@"children"] addObject:NXProjectNode(@"project", name, @"")]; [self markChanged];
}
- (void)add:(id)sender {
    if (_mode == 4) {
        NSOpenPanel *panel = [NSOpenPanel openPanel]; panel.canChooseFiles = NO; panel.canChooseDirectories = YES;
        if ([panel runModal] == NSModalResponseOK && ![_folders containsObject:panel.URL.path]) { [_folders addObject:panel.URL.path]; [self refresh:nil]; [self notifyChange]; }
    } else { NSString *name = [self askName:@"New Project" value:@"Project"]; if (name.length) [self addProjectNamed:name]; }
}
- (void)addFolder:(id)sender {
    NSString *name = [self askName:@"New Virtual Folder" value:@"Folder"];
    if (name.length) { [[self container][@"children"] addObject:NXProjectNode(@"folder", name, @"")]; [self markChanged]; [_tree expandItem:nil expandChildren:YES]; }
}
- (NSMutableDictionary *)nodeFromPath:(NSString *)path depth:(unsigned)depth count:(NSUInteger *)count error:(NSError **)error {
    if (depth > 32 || ++*count > 20000) { if (error) *error = NXError(@"The directory tree exceeds the 20,000-entry or 32-level limit. Choose a smaller folder."); return nil; }
    NSDictionary *attributes = [NSFileManager.defaultManager attributesOfItemAtPath:path error:error];
    if (!attributes) return nil;
    BOOL directory = [attributes[NSFileType] isEqual:NSFileTypeDirectory];
    NSMutableDictionary *node = NXProjectNode(directory ? @"folder" : @"file", path.lastPathComponent, path);
    if (directory) {
        NSArray *urls = [NSFileManager.defaultManager contentsOfDirectoryAtURL:[NSURL fileURLWithPath:path] includingPropertiesForKeys:nil options:NSDirectoryEnumerationSkipsHiddenFiles error:error];
        if (!urls) return nil;
        urls = [urls sortedArrayUsingComparator:^NSComparisonResult(NSURL *a, NSURL *b) { return [a.lastPathComponent localizedStandardCompare:b.lastPathComponent]; }];
        for (NSURL *url in urls) {
            // Do not follow directory symlinks; they can escape the chosen tree or form cycles.
            NSDictionary *attr = [NSFileManager.defaultManager attributesOfItemAtPath:url.path error:nil];
            if ([attr[NSFileType] isEqual:NSFileTypeSymbolicLink]) continue;
            NSMutableDictionary *child = [self nodeFromPath:url.path depth:depth + 1 count:count error:error];
            if (!child) return nil;
            [node[@"children"] addObject:child];
        }
    }
    return node;
}
- (BOOL)addPaths:(NSArray<NSString *> *)paths toNode:(NSMutableDictionary *)parent error:(NSError **)error {
    if (!parent || [parent[@"kind"] isEqual:@"file"]) { if (error) *error = NXError(@"Select a project or folder first."); return NO; }
    NSMutableArray *pending = [NSMutableArray new]; NSUInteger count = 0;
    if (_mode >= 1 && _mode <= 3) NXValidateProject(_projects[_mode - 1], &count, 0);
    for (NSString *path in paths) {
        if ([[parent[@"children"] valueForKey:@"path"] containsObject:path] || [[pending valueForKey:@"path"] containsObject:path]) continue;
        NSMutableDictionary *node = [self nodeFromPath:path depth:0 count:&count error:error];
        if (!node) return NO;
        [pending addObject:node];
    }
    [parent[@"children"] addObjectsFromArray:pending]; [self markChanged]; return YES;
}
- (void)addFiles:(id)sender {
    NSOpenPanel *panel = [NSOpenPanel openPanel]; panel.canChooseDirectories = YES; panel.allowsMultipleSelection = YES;
    if ([panel runModal] != NSModalResponseOK) return;
    NSError *error = nil;
    if (![self addPaths:[panel.URLs valueForKey:@"path"] toNode:[self container] error:&error]) [NSApp presentError:error];
    [_tree expandItem:nil expandChildren:YES];
}
- (void)remove:(id)sender {
    NSDictionary *displayed = [_tree itemAtRow:_tree.selectedRow]; if (!displayed) return;
    if (_mode == 4) { [_folders removeObject:displayed[@"path"]]; [self refresh:nil]; [self notifyChange]; return; }
    NSMutableDictionary *node = [self selected];
    NSMutableDictionary *parent = node ? [self parentOf:node[@"id"] root:_projects[_mode - 1]] : nil;
    if (!parent) return;
    NSAlert *alert = [NSAlert new]; alert.messageText = [@"Remove " stringByAppendingString:node[@"name"]];
    alert.informativeText = @"This removes the project reference and its children. Files on disk will not be deleted.";
    [alert addButtonWithTitle:@"Cancel"]; [alert addButtonWithTitle:@"Remove"];
    if ([alert runModal] == NSAlertSecondButtonReturn) { [parent[@"children"] removeObject:node]; [self markChanged]; }
}
- (void)rename:(id)sender {
    NSMutableDictionary *node = [self selected]; if (!node) return;
    NSString *name = [self askName:@"Rename Project Item" value:node[@"name"]];
    if (name.length) { node[@"name"] = name; [self markChanged]; }
}
- (void)relink:(id)sender {
    NSMutableDictionary *node = [self selected]; if (![node[@"kind"] isEqual:@"file"]) return;
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    if ([panel runModal] == NSModalResponseOK) { node[@"path"] = panel.URL.path; node[@"name"] = panel.URL.lastPathComponent; [self markChanged]; }
}
- (void)move:(NSButton *)sender {
    NSMutableDictionary *node = [self selected], *parent = node ? [self parentOf:node[@"id"] root:_projects[_mode - 1]] : nil;
    NSMutableArray *children = parent[@"children"]; NSInteger index = [children indexOfObject:node], next = index + sender.tag;
    if (!parent || next < 0 || next >= (NSInteger)children.count) return;
    [children exchangeObjectAtIndex:index withObjectAtIndex:next]; [self markChanged]; [_tree expandItem:nil expandChildren:YES];
}
- (BOOL)loadWorkspace:(NSString *)path panel:(NSInteger)panel error:(NSError **)error {
    if (panel < 0 || panel >= 3) { if (error) *error = NXError(@"Invalid project panel."); return NO; }
    NSMutableDictionary *root = NXReadWorkspace(path, error); if (!root) return NO;
    _projects[panel] = root; _workspacePaths[panel] = path; _workspaceHashes[panel] = NXDigest([NSData dataWithContentsOfFile:path]); _dirty[panel] = @NO;
    [self refreshRows]; [self notifyChange]; return YES;
}
- (BOOL)saveWorkspace:(NSString *)path panel:(NSInteger)panel error:(NSError **)error {
    if (panel < 0 || panel >= 3) return NO;
    if ([_workspacePaths[panel] isEqual:path] && ![NXDigest([NSData dataWithContentsOfFile:path]) isEqual:_workspaceHashes[panel]]) {
        if (error) *error = NXError(@"The workspace changed outside NoteX. Choose a new filename to avoid overwriting it."); return NO;
    }
    if (!NXWriteWorkspace(_projects[panel], path, error)) return NO;
    _workspacePaths[panel] = path; _workspaceHashes[panel] = NXDigest([NSData dataWithContentsOfFile:path]); _dirty[panel] = @NO;
    [self refreshRows]; [self notifyChange]; return YES;
}
- (void)openWorkspace:(id)sender {
    if (_mode < 1 || _mode > 3) return;
    if ([_dirty[_mode - 1] boolValue]) {
        NSAlert *alert = [NSAlert new]; alert.messageText = @"Replace this panel's workspace?"; alert.informativeText = @"Export the current workspace first if you want to keep its unsaved project changes.";
        [alert addButtonWithTitle:@"Cancel"]; [alert addButtonWithTitle:@"Replace"];
        if ([alert runModal] != NSAlertSecondButtonReturn) return;
    }
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    if ([panel runModal] == NSModalResponseOK) { NSError *error = nil; if (![self loadWorkspace:panel.URL.path panel:_mode - 1 error:&error]) [NSApp presentError:error]; [_tree expandItem:nil expandChildren:YES]; }
}
- (void)exportWorkspace:(id)sender {
    if (_mode < 1 || _mode > 3) return;
    NSSavePanel *panel = [NSSavePanel savePanel]; panel.nameFieldStringValue = [_workspacePaths[_mode - 1] length] ? [_workspacePaths[_mode - 1] lastPathComponent] : @"workspace.xml";
    if ([panel runModal] == NSModalResponseOK) { NSError *error = nil; if (![self saveWorkspace:panel.URL.path panel:_mode - 1 error:&error]) [NSApp presentError:error]; }
}
- (void)refresh:(id)sender {
    if (_mode == 0) { _lastText = nil; if (self.refreshRequested) self.refreshRequested(); }
    if (_mode == 4) {
        NSMutableArray *roots = [NSMutableArray new]; NSUInteger count = 0; NSError *error = nil;
        for (NSString *folder in _folders) { NSMutableDictionary *node = [self nodeFromPath:folder depth:0 count:&count error:&error]; if (node) [roots addObject:node]; }
        _folderRows = roots; _status.stringValue = error ? error.localizedDescription : @"Folder contents refresh on request. Double-click a file to open it.";
    }
    [self refreshRows];
}
- (void)rules:(id)sender {
    NSAlert *choice = [NSAlert new]; choice.messageText = @"Function List Rules";
    choice.informativeText = @"Use bundled rules selected by filename, or choose a custom Notepad++ Function List XML parser.";
    [choice addButtonWithTitle:@"Automatic"]; [choice addButtonWithTitle:@"Choose XML…"]; [choice addButtonWithTitle:@"Cancel"];
    auto result = [choice runModal];
    if (result == NSAlertFirstButtonReturn) _ruleOverride = @"";
    else if (result == NSAlertSecondButtonReturn) {
        NSOpenPanel *panel = [NSOpenPanel openPanel]; panel.directoryURL = [NSBundle.mainBundle.resourceURL URLByAppendingPathComponent:@"functionList"];
        if ([panel runModal] != NSModalResponseOK) return;
        NSError *error = nil; if (!NXFunctionIndex(NSData.data, panel.URL.path, &error)) { [NSApp presentError:error]; return; }
        _ruleOverride = panel.URL.path;
    } else return;
    [self refresh:nil]; [self notifyChange];
}
- (void)sort:(id)sender { _sorted = !_sorted; [self refreshRows]; [self notifyChange]; }
@end
