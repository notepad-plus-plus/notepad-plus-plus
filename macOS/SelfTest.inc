// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native integration checks. Only enabled by an explicit command-line argument
// and an isolated NOTEQU_PROFILE directory supplied by the test driver.
static unsigned checks = 0;
static void Check(bool condition, const char *name) {
    if (!condition) { fprintf(stderr, "FAIL: %s\n", name); fflush(stderr); exit(2); }
    ++checks; printf("PASS: %s\n", name); fflush(stdout);
}
// Published legacy builds seed this exact text for the restore-legacy test.
static NSData *Sample() { return [@"NoteX — café, Türkçe, 日本語, 📝\nUnsaved recovery text.\n" dataUsingEncoding:NSUTF8StringEncoding]; }
static void Bold24(ScintillaView *view) {
    NoteQu::Change change; change.end = Send(view, SCI_GETLENGTH); change.fields = NoteQu::Bold | NoteQu::Size;
    change.value.mask = change.fields; change.value.bold = true; change.value.size = 2400;
    Send(view, NoteQu::ApplyFormat, 0, reinterpret_cast<sptr_t>(&change));
    NoteQu::SelectionFormat query; query.end = change.end;
    Send(view, NoteQu::GetFormat, 0, reinterpret_cast<sptr_t>(&query));
    Check(query.value.bold && query.value.size == 2400 && !(query.mixed & change.fields), "apply shared 24-point bold format");
}
// Sample a small, flat patch of the real AppKit renderer in editor coordinates.
static NSColor *EditorPixel(ScintillaView *editor, CGFloat x, CGFloat y) {
    NSView *content = editor.content;
    NSRect patch = NSMakeRect(x - 1, y - 1, 3, 3);
    if (!NSContainsRect(content.visibleRect, patch)) return nil;
    NSBitmapImageRep *bitmap = [content bitmapImageRepForCachingDisplayInRect:patch];
    if (!bitmap) return nil;
    [content cacheDisplayInRect:patch toBitmapImageRep:bitmap];
    return [[bitmap colorAtX:bitmap.pixelsWide / 2 y:bitmap.pixelsHigh / 2]
        colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
}
static bool SameEditorColor(NSColor *first, NSColor *second) {
    return first && second && std::abs(first.redComponent - second.redComponent) < 0.02 &&
        std::abs(first.greenComponent - second.greenComponent) < 0.02 &&
        std::abs(first.blueComponent - second.blueComponent) < 0.02;
}
static void CaptureEditor(NSWindow *window, NSString *name) {
    NSString *folder = NSProcessInfo.processInfo.environment[@"NOTEQU_TEST_RENDER_DIR"];
    if (!folder.length) return;
    NSView *content = window.contentView;
    [window display];
    NSBitmapImageRep *bitmap = [content bitmapImageRepForCachingDisplayInRect:content.bounds];
    [content cacheDisplayInRect:content.bounds toBitmapImageRep:bitmap];
    NSString *path = [folder stringByAppendingPathComponent:[NSString stringWithFormat:@"editor-render-%@.png", name]];
    Check([[bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:path atomically:YES],
          "capture native editor pixels");
}
@implementation NXApp (Tests)
- (void)testEditorRendering {
    ScintillaView *editor = self.current.editor;
    Check(Text(editor).length == 0, "rendering fixture starts in an empty document");
    if (!self.dark) [self toggleDark:nil];
    self.preferences[@"highlightLine"] = @YES;
    [self refreshEditorSettings];
    [self.window.contentView layoutSubtreeIfNeeded];
    Check([self.window makeFirstResponder:editor.content], "native text view accepts keyboard focus");
    // Hosted AppKit sessions can leave the foreground activation pending.
    if (!Send(editor, SCI_GETFOCUS)) Send(editor, SCI_SETFOCUS, 1);
    [editor insertText:@"deneme\n"];
    [editor insertText:@"deneme2   \n"];
    [editor insertText:@"deneme3\n"];
    Check([editor.string isEqualToString:@"deneme\ndeneme2   \ndeneme3\n"],
          "Cocoa text input retains all typed lines");
    Check(Send(editor, SCI_GETMARGINMASKN, 3) == SC_MASK_HISTORY &&
          Send(editor, SCI_GETMARGINWIDTHN, 3) > 0, "change history owns a visible margin");
    for (int line = 0; line < 3; ++line)
        Check(Send(editor, SCI_MARKERGET, line) & SC_MASK_HISTORY, "typing still records change history");

    const CGFloat lineHeight = Send(editor, SCI_TEXTHEIGHT, 0);
    const CGFloat farX = NSMidX([editor content].visibleRect);
    auto at = [&](int row, CGFloat x) { return EditorPixel(editor, x, (row + 0.5) * lineHeight); };
    [self.window display];
    CaptureEditor(self.window, @"dark");
    NSColor *darkBackground = at(6, farX);
    Check(darkBackground && darkBackground.redComponent < 0.25, "dark editor background is visible");
    for (int line = 0; line < 3; ++line)
        Check(SameEditorColor(at(line, farX), darkBackground), "edited dark line uses the editor background");
    NSColor *caretLine = at(3, farX);
    Check(caretLine && !SameEditorColor(caretLine, darkBackground), "current line retains its highlight");

    const auto historyWidth = Send(editor, SCI_GETMARGINWIDTHN, 3);
    Send(editor, SCI_SETMARGINWIDTHN, 3, 0);
    [self.window display];
    CaptureEditor(self.window, @"previous-margin");
    NSColor *oldBackground = at(1, farX);
    Check(oldBackground && !SameEditorColor(oldBackground, darkBackground) &&
          SameEditorColor(at(3, farX), caretLine), "previous margin setup reproduces the overwritten row background");
    Send(editor, SCI_SETMARGINWIDTHN, 3, historyWidth);

    NSEvent *selectAll = [NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint
        modifierFlags:NSEventModifierFlagCommand timestamp:0 windowNumber:self.window.windowNumber
        context:nil characters:@"a" charactersIgnoringModifiers:@"a" isARepeat:NO keyCode:0];
    [editor.content keyDown:selectAll];
    Check(Send(editor, SCI_GETSELECTIONSTART) == 0 &&
          Send(editor, SCI_GETSELECTIONEND) == Send(editor, SCI_GETLENGTH),
          "Command+A selects the complete document through Cocoa editor keyboard input");
    [self.window display];
    const auto trailingSpace = Send(editor, SCI_GETLINEENDPOSITION, 1) - 2;
    const auto selectedLineStart = Send(editor, SCI_POSITIONFROMLINE, 1);
    const CGFloat selectedX = Send(editor, SCI_POINTXFROMPOSITION, 0, trailingSpace) -
        Send(editor, SCI_POINTXFROMPOSITION, 0, selectedLineStart) + Send(editor, SCI_GETMARGINLEFT) + 2;
    CaptureEditor(self.window, @"dark-command-a");
    NSColor *selected = at(1, selectedX);
    Check(SameEditorColor(at(1, farX), darkBackground) && selected &&
          selected.blueComponent > selected.redComponent && !SameEditorColor(selected, darkBackground),
          "selection stays visible while the rest of the line keeps its dark background");

    Send(editor, SCI_GOTOPOS, Send(editor, SCI_GETLENGTH));
    if (self.dark) [self toggleDark:nil];
    self.preferences[@"lineNumbers"] = @NO;
    [self refreshEditorSettings];
    [self.window display];
    CaptureEditor(self.window, @"light");
    NSColor *lightBackground = at(6, farX);
    Check(lightBackground && lightBackground.redComponent > 0.9 &&
          SameEditorColor(at(1, farX), lightBackground) &&
          !SameEditorColor(at(3, farX), lightBackground), "light theme renders edited and caret lines with line numbers hidden");
}
- (void)selfTest:(NSString *)mode {
    Check(ProfileOverride().length > 0, "explicit isolated test profile");
    Check([self.storage.directory.lastPathComponent hasPrefix:@"test-"], "test profile guard");
    if ([mode isEqual:@"editor-rendering"]) {
        [self testEditorRendering];
        [NSApp terminate:nil];
        return;
    }
    if([mode isEqual:@"suite"])Check(!self.sidebarVisible&&self.workspace.view.superview==nil,"new profiles start with the sidebar hidden");
    NSError *error = nil;
    if([mode isEqual:@"instance-verify"]){Check([Text(self.current.editor) isEqual:Sample()],"independent process restores the exact unsaved Unicode document");Check(!State(self.current.editor).runs.empty(),"independent process restores inline formatting");Check([self snapshot:&error],"independent profile remains recoverable");exit(0);}
    if ([mode isEqual:@"update-release"]) {
        SetText(self.current.editor,Sample()); Bold24(self.current.editor);
        self.preferences[@"tabWidth"]=@7;
        Check([self snapshot:&error],"snapshot before real release update");
        [self checkUpdates:nil];
        [NSTimer scheduledTimerWithTimeInterval:0.5 repeats:YES block:^(NSTimer *timer) {
            NSString *events=[NSString stringWithContentsOfFile:[self.storage.directory stringByAppendingPathComponent:@"update-events.jsonl"] encoding:NSUTF8StringEncoding error:nil];
            if ([events containsString:@"waiting for normal exit"]) { [timer invalidate]; [NSApp terminate:nil]; }
        }];
        return;
    }
    if ([mode isEqual:@"update-verify"]) {
        Check([Text(self.current.editor) isEqual:Sample()],"real release preserves unsaved Unicode document");
        Check(!State(self.current.editor).runs.empty(),"real release preserves inline formatting");
        Check([self.preferences[@"tabWidth"] integerValue]==7,"real release preserves custom preferences");
        Check([self snapshot:&error],"updated session remains writable");
        [NSApp terminate:nil];return;
    }
    if ([@[@"ui-tabs",@"ui-document-map",@"ui-sidebar-hidden",@"ui-toolbar",@"ui-split",@"ui-search",@"ui-preferences",@"ui-shortcuts",@"ui-language",@"ui-theme"] containsObject:mode]) { [self workbenchUI:mode]; return; }
    if ([mode hasPrefix:@"ui-"]) {
        NSString *path = [self.storage.directory stringByAppendingPathComponent:@"example.py"];
        NSData *code = [@"# NoteQu native workspace example\n\nclass Notebook:\n    def add_note(self, title):\n        return title.strip()\n\ndef main():\n    notebook = Notebook()\n    return notebook.add_note('Hello, NoteQu')\n" dataUsingEncoding:NSUTF8StringEncoding];
        Check(NXWrite(code, path, &error) && [self openPath:path error:&error], "open synthetic panel example");
        [self.workspace showPanel:1]; [self.workspace addProjectNamed:@"NoteQu Examples"];
        Check([self.workspace addPaths:@[path] toNode:self.workspace.projects[0][@"children"][0] error:&error], "populate native project panel");
        [self showPanel:TestCommand([mode isEqual:@"ui-project"] ? 1 : 0)];
        NSOutlineView *tree = [self.workspace valueForKey:@"tree"];
        [tree expandItem:nil expandChildren:YES];
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
            [tree expandItem:nil expandChildren:YES];
            if ([mode isEqual:@"ui-function"]) {
                Check(self.workspace.symbols.count >= 3 && tree.numberOfRows >= 3, "asynchronous Function List populates native outline");
                [tree selectRowIndexes:[NSIndexSet indexSetWithIndex:1] byExtendingSelection:NO];
                NSDictionary *symbol = [tree itemAtRow:1];
                [NSApp sendAction:tree.doubleAction to:tree.target from:tree];
                Check(Send(self.current.editor, SCI_GETCURRENTPOS) == [symbol[@"position"] longLongValue], "Function List double action navigates editor");
            } else if ([mode isEqual:@"ui-project"]) {
                [tree selectRowIndexes:[NSIndexSet indexSetWithIndex:2] byExtendingSelection:NO];
                [NSApp sendAction:tree.doubleAction to:tree.target from:tree];
                Check([self.current.path isEqual:NXCanonicalPath(path)] && tree.numberOfRows == 3, "project outline opens its file reference");
            }
            printf("READY\n"); fflush(stdout);
            if ([mode isEqual:@"ui-menu"]) [self customizeMenus:nil];
        });
        return;
    }
    if ([mode isEqual:@"seed"] || [mode isEqual:@"crash-seed"]) {
        NXDocument *doc = self.current;
        NSData *initial = [mode isEqual:@"seed"] ? [Sample() subdataWithRange:NSMakeRange(0, Sample().length - 8)] : Sample();
        SetText(doc.editor, initial); Bold24(doc.editor);
        Send(doc.editor, SCI_GOTOPOS, Sample().length);
        Send(doc.editor, SCI_MARKERADD, 0, 0);
        for (NSInteger panel = 1; panel <= 3; ++panel) {
            [self.workspace showPanel:panel];
            [self.workspace addProjectNamed:[NSString stringWithFormat:@"Recovery Project %ld", (long)panel]];
        }
        self.preferences[@"tabWidth"] = @2;
        [self.macros addObject:@{@"name":@"Recovery Macro", @"actions":@[@{@"message":@(SCI_REPLACESEL), @"w":@0, @"l":@0, @"text":[@"restored" dataUsingEncoding:NSUTF8StringEncoding]}]}];
        Check([self.menuLayout setLayout:@{@"order":@{@"":@[@"/menu:View"]}, @"hidden":@[@"/menu:Format/toggleFormat::1"]} error:&error], "persist feature settings with unsaved session");
        [self seedWorkbench];
        // Use the regular autosnapshot timer, not a separate test storage implementation.
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1500 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
            Check([NSFileManager.defaultManager fileExistsAtPath:[self.storage.directory stringByAppendingPathComponent:@"session.plist"]], "autosnapshot completed");
            printf("READY\n"); fflush(stdout);
            if ([mode isEqual:@"seed"]) {
                NSData *tail = [Sample() subdataWithRange:NSMakeRange(Sample().length - 8, 8)];
                Send(doc.editor, SCI_APPENDTEXT, tail.length, reinterpret_cast<sptr_t>(tail.bytes));
                Bold24(doc.editor);
                Send(doc.editor, SCI_GOTOPOS, Sample().length);
                // Quit before the next autosnapshot: termination must flush the last edit.
                [NSApp terminate:nil];
            }
        });
        return;
    }
    if ([mode isEqual:@"restore"] || [mode isEqual:@"restore-legacy"]) {
        NXDocument *doc = self.current;
        Check([Text(doc.editor) isEqual:Sample()], "restore exact unsaved Unicode bytes");
        auto state = State(doc.editor);
        Check(state.runs.size() == 1 && state.runs[0].end == Sample().length, "restore formatting ranges");
        Check(state.formats[state.runs[0].style].bold && state.formats[state.runs[0].style].size == 2400, "restore bold and size");
        Check(Send(doc.editor, SCI_GETCURRENTPOS) == (sptr_t)Sample().length, "restore caret position");
        Check(doc.dirty, "restored unsaved tab remains dirty");
        if ([mode isEqual:@"restore"]) {
            for (NSInteger panel = 0; panel < 3; ++panel)
                Check([self.workspace.projects[panel][@"children"][0][@"name"] isEqual:[NSString stringWithFormat:@"Recovery Project %ld", (long)panel + 1]], "restore independent project panel across process restart");
            Check([self.preferences[@"tabWidth"] isEqual:@2] && Send(doc.editor, SCI_GETTABWIDTH) == 2, "restore editor preferences");
            Check(self.macros.count == 1 && [self.macros[0][@"name"] isEqual:@"Recovery Macro"], "restore saved macro");
            Check(!FindMenuCommand(NSApp.mainMenu, @selector(toggleFormat:), NoteQu::Bold) && [[NSApp.mainMenu itemAtIndex:1].title isEqual:@"View"], "restore customized native menus");
            Check(Send(doc.editor, SCI_MARKERGET, 0) & 1, "restore bookmarks");
            [self verifyWorkbenchRestore];
        }
        [NSApp terminate:nil]; return;
    }
    Check([mode isEqual:@"suite"], "known test mode");
    Check(self.documents.count == 1 && Text(self.current.editor).length == 0, "fresh launch has an empty document");
    Check(GetLexerCount() > 100, "bundled native syntax lexers");
    NXDocument *doc = self.current;
    SetText(doc.editor, Sample());
    const auto originalHeight = Send(doc.editor, SCI_TEXTHEIGHT, 0);
    Bold24(doc.editor);
    [self.window display];
    Check(Send(doc.editor, SCI_TEXTHEIGHT, 0) > originalHeight, "Cocoa line height follows larger font");
    Check(doc.dirty, "formatting sets dirty state");
    Send(doc.editor, SCI_UNDO);
    Check(State(doc.editor).runs.empty(), "formatting undo");
    Send(doc.editor, SCI_REDO);
    Check(State(doc.editor).runs.size() == 1, "formatting redo");
    NSData *sidecar = NXCompanion(Text(doc.editor), State(doc.editor));
    NoteQu::State decoded;
    Check(NXReadCompanion(sidecar, Sample(), decoded), "Windows-compatible companion round trip");
    Check(!NXReadCompanion(sidecar, [@"changed" dataUsingEncoding:NSUTF8StringEncoding], decoded), "reject companion for different text");
    NSMutableData *legacy = [sidecar mutableCopy];
    memcpy(legacy.mutableBytes, "NPPSIDE1", 8);
    memcpy(static_cast<char *>(legacy.mutableBytes) + 40, NoteQu::previousWireMagic, 8);
    Check(NXReadCompanion(legacy, Sample(), decoded), "previous companion signature compatibility");
    Send(doc.editor, SCI_SETSEL, 0, Send(doc.editor, SCI_GETLENGTH));
    Send(doc.editor, SCI_COPY);
    NXDocument *pasted = [self newDocument:nil];
    Send(pasted.editor, SCI_PASTE);
    Check([Text(pasted.editor) isEqual:Sample()], "native clipboard Unicode text");
    Check(State(pasted.editor).runs.size() == 1 && State(pasted.editor).formats[State(pasted.editor).runs[0].style].size == 2400, "native clipboard formatting");
    Send(pasted.editor, SCI_UNDO);
    Check(Text(pasted.editor).length == 0, "paste and formatting undo together");
    Send(pasted.editor, SCI_REDO);
    Check([Text(pasted.editor) isEqual:Sample()], "paste redo");
    Check([pasted.editor findAndHighlightText:@"日本語" matchCase:YES wholeWord:NO scrollTo:YES wrap:YES], "Unicode find");
    Check([pasted.editor.selectedString isEqual:@"日本語"], "Unicode find selection");
    [pasted.editor findAndReplaceText:@"recovery" byText:@"restore" matchCase:YES wholeWord:NO doAll:YES];
    Check([pasted.editor.string containsString:@"restore text"], "native replace");
    [self.tabs selectTabViewItem:doc.tab];
    NSString *path = [self.storage.directory stringByAppendingPathComponent:@"sample.txt"];
    Check([self saveDocument:doc path:path error:&error], "save document and formatting companion");
    Check(!doc.dirty, "successful save clears dirty state");
    Check([[NSData dataWithContentsOfFile:path] isEqual:Sample()], "saved document bytes unchanged");
    Check(NXReadCompanion([NSData dataWithContentsOfFile:[path stringByAppendingString:@".notex"]], Sample(), decoded), "saved companion matches document");
    NSString *copy = [self.storage.directory stringByAppendingPathComponent:@"copy.txt"];
    Check(NXWrite(Sample(), copy, &error), "prepare external document");
    Check(NXWrite(sidecar, [copy stringByAppendingString:@".notex"], &error), "prepare external companion");
    Check([self openPath:copy error:&error], "open document through native file path");
    Check(State(self.current.editor).runs.size() == 1, "open restores companion formatting");
    Check(NXWrite([@"external edit" dataUsingEncoding:NSUTF8StringEncoding], path, &error), "simulate external edit");
    error = nil;
    Check(![self saveDocument:doc path:path error:&error], "refuse overwriting externally modified file");
    Check([[[NSString alloc] initWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil] isEqual:@"external edit"], "external edit remains intact");
    error = nil;
    Check(![self saveDocument:doc path:[self.storage.directory stringByAppendingPathComponent:@"missing/subfolder/file.txt"] error:&error], "failed file write is reported");
    NSString *collision = [self.storage.directory stringByAppendingPathComponent:@"collision.txt"];
    Check(NXWrite([@"unrelated" dataUsingEncoding:NSUTF8StringEncoding], [collision stringByAppendingString:@".notex"], &error), "prepare companion collision");
    error = nil;
    Check(![self saveDocument:doc path:collision error:&error] && ![NSFileManager.defaultManager fileExistsAtPath:collision], "companion collision prevents document overwrite");
    error = nil;
    NXStorage *locked = [[NXStorage alloc] initWithDirectory:self.storage.directory error:&error];
    Check(!locked && error, "profile lock prevents concurrent writers");
    error = nil;
    Check([self snapshot:&error], "save complete multi-tab session");
    NSDictionary *session = [self.storage load:&error];
    Check([session[@"documents"] count] == self.documents.count, "all tabs in session");
    NSString *backupPath = [self.storage.directory stringByAppendingPathComponent:@"test-backup"];
    NXStorage *backup = [[NXStorage alloc] initWithDirectory:backupPath error:&error];
    Check([backup save:session error:&error] && [backup save:session error:&error], "maintain previous complete snapshot");
    Check(NXWrite([@"corrupt" dataUsingEncoding:NSUTF8StringEncoding], [backupPath stringByAppendingPathComponent:@"session.plist"], &error), "simulate truncated session");
    Check([[backup load:&error] isEqual:session], "recover previous snapshot after corruption");
    auto recoveryFiles = [NSFileManager.defaultManager contentsOfDirectoryAtPath:backupPath error:nil];
    Check([recoveryFiles filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"SELF BEGINSWITH 'recovery-'"]].count == 1, "retain damaged session for recovery");
    Check(NXWrite([@"corrupt too" dataUsingEncoding:NSUTF8StringEncoding], [backupPath stringByAppendingPathComponent:@"session.plist.previous"], &error), "simulate both snapshots damaged");
    error = nil;
    Check(![backup load:&error] && error, "stop instead of resetting an unrecoverable session");
    NSString *bad = [self.storage.directory stringByAppendingPathComponent:@"binary.bin"];
    const unsigned char invalid[] = {0xff, 0x00, 0x99};
    Check(NXWrite([NSData dataWithBytes:invalid length:3], bad, &error), "prepare invalid text");
    error = nil;
    Check(![self openPath:bad error:&error], "unsupported input is rejected without conversion");
    NSString *utf16Path = [self.storage.directory stringByAppendingPathComponent:@"utf16.txt"];
    NSMutableData *utf16 = [NSMutableData dataWithBytes:"\xff\xfe" length:2];
    [utf16 appendData:[@"UTF-16 café 日本語\r\n" dataUsingEncoding:NSUTF16LittleEndianStringEncoding]];
    Check(NXWrite(utf16, utf16Path, &error), "prepare UTF-16 document");
    error = nil;
    Check([self openPath:utf16Path error:&error] && [self saveDocument:self.current path:utf16Path error:&error], "UTF-16 open and save");
    Check([[NSData dataWithContentsOfFile:utf16Path] isEqual:utf16], "preserve UTF-16 BOM and line endings");
    // Test Cocoa text rendering with the shared font/line-height engine.
    [self.tabs selectTabViewItem:doc.tab];
    [self.window.contentView layoutSubtreeIfNeeded];
    [self.window display];
    NSBitmapImageRep *bitmap = [self.window.contentView bitmapImageRepForCachingDisplayInRect:self.window.contentView.bounds];
    [self.window.contentView cacheDisplayInRect:self.window.contentView.bounds toBitmapImageRep:bitmap];
    Check(bitmap.pixelsWide > 800 && bitmap.pixelsHigh > 400, "native view rendered at expected dimensions");
    NSString *imagePath = NSProcessInfo.processInfo.environment[@"NOTEQU_TEST_IMAGE"];
    if (imagePath.length) [[bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:imagePath atomically:YES];
    [self featureTests];
    [self workbenchTests];
    [self parityTests];
    printf("TOTAL %u passed\n", checks); fflush(stdout);
    [NSApp terminate:nil];
}
@end
