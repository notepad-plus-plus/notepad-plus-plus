// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
static void Check(bool condition, const char *name);
static void Bold24(ScintillaView *view);
static NSArray *NormalizedSymbolNames(NSArray *names) {
    NSMutableArray *result = [NSMutableArray new];
    for (NSString *name in names) [result addObject:[name stringByReplacingOccurrencesOfString:@"\r\n" withString:@"\n"]];
    return [result sortedArrayUsingSelector:@selector(compare:)];
}
static NSButton *TestCommand(NSInteger tag) { NSButton *button = [NSButton new]; button.tag = tag; return button; }
static NSMenuItem *FindMenuCommand(NSMenu *menu, SEL action, NSInteger tag) {
    for (NSMenuItem *item in menu.itemArray) {
        if (item.action == action && item.tag == tag) return item;
        if (item.submenu) { NSMenuItem *found = FindMenuCommand(item.submenu, action, tag); if (found) return found; }
    }
    return nil;
}
@implementation NXApp (FeatureTests)
- (void)featureTests {
    NSError *error = nil;
    NSDictionary *layout = @{@"order":@{@"":@[@"/menu:View", @"/menu:File"]}, @"hidden":@[@"/menu:Format/toggleFormat::1"]};
    Check([self.menuLayout setLayout:layout error:&error], "persist customized menu layout");
    Check([[NSApp.mainMenu itemAtIndex:0].title isEqual:@"NoteX"] && [[NSApp.mainMenu itemAtIndex:1].title isEqual:@"View"], "keep application menu first while reordering menus");
    Check(!FindMenuCommand(NSApp.mainMenu, @selector(toggleFormat:), NoteX::Bold), "hide selected formatting command");
    Check(FindMenuCommand(NSApp.mainMenu, @selector(customizeMenus:), 0) && FindMenuCommand(NSApp.mainMenu, @selector(terminate:), 0), "layout recovery and Quit remain accessible");
    NSDictionary *savedLayout = self.menuLayout.settings;
    NSTimer *cancelDialog = [NSTimer timerWithTimeInterval:0.1 repeats:NO block:^(NSTimer *timer) {
        for (NSView *view in NSApp.modalWindow.contentView.subviews) if ([view isKindOfClass:NSButton.class] && [((NSButton *)view).title isEqual:@"Cancel"]) { [(NSButton *)view performClick:nil]; break; }
    }];
    [NSRunLoop.mainRunLoop addTimer:cancelDialog forMode:NSModalPanelRunLoopMode];
    [self.menuLayout customizeForWindow:self.window];
    Check([self.menuLayout.settings isEqual:savedLayout], "cancel native menu dialog leaves layout unchanged");
    [self.menuLayout reset];
    Check(FindMenuCommand(NSApp.mainMenu, @selector(toggleFormat:), NoteX::Bold), "restore default menus");
    error = nil;
    Check(![self.menuLayout setLayout:@{@"order":@[], @"hidden":@[@1]} error:&error], "reject malformed menu layout");

    [self.workspace showPanel:1];
    [self.workspace addProjectNamed:@"Café Project & Tools"];
    NSMutableDictionary *root = self.workspace.projects[0], *project = root[@"children"][0];
    NSString *directory = [self.storage.directory stringByAppendingPathComponent:@"project fixtures"];
    [NSFileManager.defaultManager createDirectoryAtPath:directory withIntermediateDirectories:YES attributes:nil error:nil];
    NSString *file = [directory stringByAppendingPathComponent:@"café & tools.cpp"];
    NSData *fileBytes = [@"int answer() { return 42; }\n" dataUsingEncoding:NSUTF8StringEncoding];
    Check(NXWrite(fileBytes, file, &error), "create synthetic project file");
    Check([self.workspace addPaths:@[file] toNode:project error:&error], "add file reference to project");
    Check([self.workspace addPaths:@[file] toNode:project error:&error] && [project[@"children"] count] == 1, "deduplicate project references");
    NSString *xml = [self.storage.directory stringByAppendingPathComponent:@"workspace.xml"];
    error = nil;
    Check([self.workspace saveWorkspace:xml panel:0 error:&error], "export Notepad++ workspace XML");
    NSString *xmlText = [[NSString alloc] initWithContentsOfFile:xml encoding:NSUTF8StringEncoding error:nil];
    Check([xmlText containsString:@"&amp;"] && ![xmlText containsString:self.storage.directory], "escape XML and export portable relative paths");
    Check([self.workspace loadWorkspace:xml panel:1 error:&error], "import workspace into independent second panel");
    NSDictionary *imported = self.workspace.projects[1][@"children"][0][@"children"][0];
    Check([imported[@"path"] isEqual:file], "resolve relative Unicode workspace paths");
    [self.workspace showPanel:3]; [self.workspace addProjectNamed:@"Third panel"];
    Check([self.workspace.projects[2][@"children"] count] == 1 && [self.workspace.projects[0][@"children"] count] == 1, "three independent project panels");
    NXWorkspace *restored = [[NXWorkspace alloc] initWithState:self.workspace.record];
    Check([restored.record isEqual:self.workspace.record], "round-trip all project trees and panel settings");
    [project[@"children"] removeAllObjects];
    Check([[NSData dataWithContentsOfFile:file] isEqual:fileBytes], "removing a project reference does not delete its file");
    NSData *foreign = [@"<NotepadPlus><Project name=\"External\"/></NotepadPlus>" dataUsingEncoding:NSUTF8StringEncoding];
    Check(NXWrite(foreign, xml, &error), "simulate externally modified workspace");
    error = nil;
    Check(![self.workspace saveWorkspace:xml panel:0 error:&error] && [[NSData dataWithContentsOfFile:xml] isEqual:foreign], "protect externally changed workspace XML");
    NSString *badXML = [self.storage.directory stringByAppendingPathComponent:@"bad-workspace.xml"];
    Check(NXWrite([@"<!DOCTYPE NotepadPlus [<!ENTITY x SYSTEM 'file:///etc/passwd'>]><NotepadPlus><Project name='x'>&x;</Project></NotepadPlus>" dataUsingEncoding:NSUTF8StringEncoding], badXML, &error), "create synthetic unsafe workspace XML");
    error = nil; Check(!NXReadWorkspace(badXML, &error), "reject workspace external entities");

    NSString *source = NSProcessInfo.processInfo.environment[@"NOTEX_TEST_SOURCE"];
    Check(source.length > 0, "explicit source directory for upstream fixtures");
    BOOL allFunctionFixturesMatch = YES; NSUInteger fixtureCount = 0;
    NSArray *languages = [[NSFileManager.defaultManager contentsOfDirectoryAtPath:[source stringByAppendingPathComponent:@"PowerEditor/Test/FunctionList"] error:nil] sortedArrayUsingSelector:@selector(compare:)];
    for (NSString *language in languages) {
        NSString *ruleName = [language isEqual:@"javascript"] ? @"javascript.js" : [language isEqual:@"udl-NppExec"] ? @"nppexec" : language;
        NSString *rule = [NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:[NSString stringWithFormat:@"functionList/%@.xml", ruleName]];
        if (![NSFileManager.defaultManager fileExistsAtPath:rule]) continue;
        NSString *fixture = [source stringByAppendingPathComponent:[@"PowerEditor/Test/FunctionList" stringByAppendingPathComponent:language]];
        NSData *input = [NSData dataWithContentsOfFile:[fixture stringByAppendingPathComponent:@"unitTest"]];
        NSDictionary *expected = [NSJSONSerialization JSONObjectWithData:[NSData dataWithContentsOfFile:[fixture stringByAppendingPathComponent:@"unitTest.expected.result"]] options:0 error:nil];
        ++fixtureCount;
        error = nil; NSArray *symbols = NXFunctionIndex(input, rule, &error);
        if (error) fprintf(stderr, "Function List error %s: %s\n", language.UTF8String, error.localizedDescription.UTF8String);
        if (!symbols || error) { allFunctionFixturesMatch = NO; continue; }
        NSMutableDictionary *actual = [NSMutableDictionary new], *wanted = [NSMutableDictionary new];
        actual[@""] = [NSMutableArray new]; wanted[@""] = expected[@"leaves"] ?: @[];
        for (NSDictionary *group in expected[@"nodes"]) wanted[group[@"name"]] = group[@"leaves"];
        // Upstream funcParse/classParse drop a match that ends at the range end.
        // Keep valid EOF definitions visible instead of reproducing that omission.
        if ([language isEqual:@"batch"]) wanted[@""] = [wanted[@""] arrayByAddingObject:@"exit"];
        if ([language isEqual:@"inno"]) wanted[@"Code"] = @[@"InitializeSetup"];
        for (NSDictionary *symbol in symbols) if ([symbol[@"kind"] isEqual:@"function"]) {
            NSString *scope = symbol[@"scope"];
            if (!actual[scope]) actual[scope] = [NSMutableArray new];
            [actual[scope] addObject:symbol[@"name"]];
            Check([symbol[@"position"] unsignedLongLongValue] < input.length, "Function List byte position is inside source");
        }
        // Fixtures checked out on macOS have LF; Windows expected JSON includes CRLF.
        for (NSString *key in [actual.allKeys copy]) actual[key] = NormalizedSymbolNames(actual[key]);
        for (NSString *key in [wanted.allKeys copy]) wanted[key] = NormalizedSymbolNames(wanted[key]);
        if (![actual isEqual:wanted]) fprintf(stderr, "Function List mismatch %s\nActual: %s\nExpected: %s\n", language.UTF8String, actual.description.UTF8String, wanted.description.UTF8String);
        if (![actual isEqual:wanted]) allFunctionFixturesMatch = NO;
        else Check(YES, [NSString stringWithFormat:@"validate upstream %@ Function List fixture", language].UTF8String);
    }
    Check(fixtureCount == 41, "exercise every bundled parser with an upstream fixture");
    Check(allFunctionFixturesMatch, "all upstream Function List fixtures match documented expectations");
    NSString *rule = NXFunctionRule(@"unicode.py", @"python");
    NSData *unicode = [@"# café 日本語\n# def fake():\n#     pass\ndef real(value):\n    return value\n" dataUsingEncoding:NSUTF8StringEncoding];
    NSArray *symbols = NXFunctionIndex(unicode, rule, &error);
    Check(symbols.count == 1 && [symbols[0][@"name"] hasPrefix:@"real"], "ignore commented-out function definitions");
    NSUInteger position = [symbols[0][@"position"] unsignedIntegerValue];
    Check(!memcmp(static_cast<const char *>(unicode.bytes) + position, "real", 4), "Unicode prefix preserves Scintilla byte offset");

    NXDocument *doc = [self newDocument:nil]; SetText(doc.editor, [@"word wordMore\n    indented\n" dataUsingEncoding:NSUTF8StringEncoding]);
    [self startRecording:nil];
    Send(doc.editor, SCI_GOTOPOS, 0); Send(doc.editor, SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>("hello "));
    [self stopRecording:nil];
    Check(self.recordingActions.count > 0, "record native Scintilla editing actions");
    SetText(doc.editor, NSData.data);
    [self playActions:self.recordingActions repetitions:3];
    Check([doc.editor.string isEqual:@"hello hello hello "], "replay editing macro multiple times");
    Send(doc.editor, SCI_UNDO); Check(Text(doc.editor).length == 0, "undo complete macro playback as one action");
    self.preferences[@"tabWidth"] = @2; self.preferences[@"lineNumbers"] = @NO; self.preferences[@"whitespace"] = @YES;
    [self applyPreferences:doc];
    Check(Send(doc.editor, SCI_GETTABWIDTH) == 2 && Send(doc.editor, SCI_GETMARGINWIDTHN, 0) == 0, "apply native editor preferences");
    Check(Send(doc.editor, SCI_GETVIEWWS) == SCWS_VISIBLEALWAYS, "apply visible whitespace preference");
    SetText(doc.editor, [@"a\nb\n" dataUsingEncoding:NSUTF8StringEncoding]);
    Send(doc.editor, SCI_GOTOPOS, 0);
    [self bookmark:TestCommand(0)]; [self bookmark:TestCommand(1)];
    Check(Send(doc.editor, SCI_MARKERGET, 0) & 1, "bookmark margin and navigation commands");
    [self convertEOL:TestCommand(SC_EOL_CRLF)];
    Check([doc.editor.string isEqual:@"a\r\nb\r\n"], "convert line endings to CRLF");
    [self convertEncoding:TestCommand(2)];
    Check(doc.encoding == NSUTF16LittleEndianStringEncoding && doc.bom && doc.dirty, "encoding conversion marks document dirty");
    SetText(doc.editor, [@"b\na\nkeep\n" dataUsingEncoding:NSUTF8StringEncoding]); Send(doc.editor, SCI_SETEOLMODE, SC_EOL_LF);
    Send(doc.editor, SCI_SETSEL, 0, 4); [self transformLines:TestCommand(1)];
    Check([doc.editor.string isEqual:@"a\nb\nkeep\n"], "sort selected lines without consuming next unselected line");
    Send(doc.editor, SCI_UNDO); Check([doc.editor.string isEqual:@"b\na\nkeep\n"], "undo line sorting in one action");
    SetText(doc.editor, [@"Rich café" dataUsingEncoding:NSUTF8StringEncoding]); Bold24(doc.editor);
    NSAttributedString *rich = [self formattedDocument];
    NSData *rtf = [rich dataFromRange:NSMakeRange(0, rich.length) documentAttributes:@{NSDocumentTypeDocumentAttribute:NSRTFTextDocumentType} error:&error];
    NSAttributedString *roundTrip = [[NSAttributedString alloc] initWithRTF:rtf documentAttributes:nil];
    Check([roundTrip.string isEqual:@"Rich café"] && [[roundTrip attribute:NSFontAttributeName atIndex:0 effectiveRange:nil] pointSize] == 24, "RTF export preserves Unicode and formatted size");
    Send(doc.editor, SCI_SETSEL, 0, Send(doc.editor, SCI_GETLENGTH)); [self copyFormat:nil];
    [self clearFormat:nil]; [self pasteFormat:nil];
    Check(State(doc.editor).formats[State(doc.editor).runs[0].style].bold, "copy and paste formatting restores bold style");
    [self.workspace showPanel:0];
}
@end
