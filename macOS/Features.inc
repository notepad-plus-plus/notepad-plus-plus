// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native editor commands, preferences, and persistent macros, created 2026-11-01.
static BOOL MacroMessage(unsigned message) {
    switch (message) {
    case SCI_REPLACESEL: case SCI_ADDTEXT: case SCI_INSERTTEXT: case SCI_APPENDTEXT:
    case SCI_CLEAR: case SCI_DELETEBACK: case SCI_DELETEBACKNOTLINE: case SCI_NEWLINE: case SCI_TAB: case SCI_BACKTAB:
    case SCI_CHARLEFT: case SCI_CHARRIGHT: case SCI_CHARLEFTEXTEND: case SCI_CHARRIGHTEXTEND:
    case SCI_WORDLEFT: case SCI_WORDRIGHT: case SCI_WORDLEFTEXTEND: case SCI_WORDRIGHTEXTEND:
    case SCI_LINEUP: case SCI_LINEDOWN: case SCI_LINEUPEXTEND: case SCI_LINEDOWNEXTEND:
    case SCI_HOME: case SCI_HOMEEXTEND: case SCI_VCHOME: case SCI_VCHOMEEXTEND: case SCI_LINEEND: case SCI_LINEENDEXTEND:
    case SCI_DOCUMENTSTART: case SCI_DOCUMENTEND: case SCI_DOCUMENTSTARTEXTEND: case SCI_DOCUMENTENDEXTEND:
    case SCI_LINEDUPLICATE: case SCI_LINEDELETE: case SCI_LINECUT: case SCI_LINETRANSPOSE:
    case SCI_LOWERCASE: case SCI_UPPERCASE: case SCI_DELWORDLEFT: case SCI_DELWORDRIGHT: case SCI_SELECTALL:
    case SCI_LINECOPY: case SCI_COPY: case SCI_CUT: case SCI_PASTE: return YES;
    default: return NO;
    }
}
static BOOL StringMacroMessage(unsigned message) {
    return message == SCI_REPLACESEL || message == SCI_ADDTEXT || message == SCI_APPENDTEXT || message == SCI_INSERTTEXT;
}
@implementation NXApp (Features)
- (void)buildFeatureMenus {
    NSMenu *root = NSApp.mainMenu;
    NSMenu *application = [root itemAtIndex:0].submenu;
    [self item:application title:@"Preferences…" action:@selector(preferencesEditor:) key:@"," tag:0];
    [self item:application title:@"Customize Menus…" action:@selector(customizeMenus:) key:@"" tag:0];
    [self item:application title:@"Reset Menu Layout" action:@selector(resetMenus:) key:@"" tag:0];
    NSMenu *file = [root itemWithTitle:@"File"].submenu;
    [self item:file title:@"Save All" action:@selector(saveAll:) key:@"" tag:0];
    [self item:file title:@"Reload from Disk…" action:@selector(reloadDocument:) key:@"" tag:0];
    [self item:file title:@"Print…" action:@selector(printDocument:) key:@"p" tag:0];
    [self item:file title:@"Export Formatted RTF…" action:@selector(exportFormatted:) key:@"" tag:0];
    [self item:file title:@"Export Formatted HTML…" action:@selector(exportFormatted:) key:@"" tag:1];
    NSMenu *view = [root itemWithTitle:@"View"].submenu;
    [self item:view title:@"Toggle Sidebar" action:@selector(toggleSidebar:) key:@"\\" tag:0];
    NSArray *panels = @[@"Function List", @"Project Panel 1", @"Project Panel 2", @"Project Panel 3", @"Folder as Workspace"];
    for (NSUInteger i = 0; i < panels.count; ++i) [self item:view title:panels[i] action:@selector(showPanel:) key:@"" tag:i];
    [self item:view title:@"Fold All" action:@selector(foldAll:) key:@"" tag:SC_FOLDACTION_CONTRACT];
    [self item:view title:@"Unfold All" action:@selector(foldAll:) key:@"" tag:SC_FOLDACTION_EXPAND];
    NSMenu *format = [root itemWithTitle:@"Format"].submenu;
    [self item:format title:@"Copy Formatting" action:@selector(copyFormat:) key:@"" tag:0];
    [self item:format title:@"Paste Formatting" action:@selector(pasteFormat:) key:@"" tag:0];
    [self item:format title:@"Copy as Rich Text" action:@selector(copyRichText:) key:@"" tag:0];
    NSMenu *edit = [root itemWithTitle:@"Edit"].submenu;
    [self item:edit title:@"Duplicate Line" action:@selector(editorCommand:) key:@"d" tag:SCI_LINEDUPLICATE];
    [self item:edit title:@"Delete Line" action:@selector(editorCommand:) key:@"" tag:SCI_LINEDELETE];
    [self item:edit title:@"Uppercase Selection" action:@selector(editorCommand:) key:@"" tag:SCI_UPPERCASE];
    [self item:edit title:@"Lowercase Selection" action:@selector(editorCommand:) key:@"" tag:SCI_LOWERCASE];
    [self item:edit title:@"Trim Trailing Whitespace" action:@selector(transformLines:) key:@"" tag:0];
    [self item:edit title:@"Sort Selected Lines" action:@selector(transformLines:) key:@"" tag:1];
    [self item:edit title:@"Remove Duplicate Selected Lines" action:@selector(transformLines:) key:@"" tag:2];
    [self item:edit title:@"Complete Word" action:@selector(completeWord:) key:@"" tag:0];
    NSMenu *bookmarks = [self menu:@"Bookmarks" parent:root];
    [self item:bookmarks title:@"Toggle Bookmark" action:@selector(bookmark:) key:@"" tag:0];
    [self item:bookmarks title:@"Next Bookmark" action:@selector(bookmark:) key:@"" tag:1];
    [self item:bookmarks title:@"Previous Bookmark" action:@selector(bookmark:) key:@"" tag:-1];
    [self item:bookmarks title:@"Clear Bookmarks" action:@selector(bookmark:) key:@"" tag:2];
    NSMenu *encoding = [self menu:@"Encoding" parent:root];
    [self item:encoding title:@"Convert to UTF-8" action:@selector(convertEncoding:) key:@"" tag:0];
    [self item:encoding title:@"Convert to UTF-8 BOM" action:@selector(convertEncoding:) key:@"" tag:1];
    [self item:encoding title:@"Convert to UTF-16 LE" action:@selector(convertEncoding:) key:@"" tag:2];
    [self item:encoding title:@"Convert to UTF-16 BE" action:@selector(convertEncoding:) key:@"" tag:3];
    [self item:encoding title:@"Convert Line Endings to LF" action:@selector(convertEOL:) key:@"" tag:SC_EOL_LF];
    [self item:encoding title:@"Convert Line Endings to CRLF" action:@selector(convertEOL:) key:@"" tag:SC_EOL_CRLF];
    [self item:encoding title:@"Convert Line Endings to CR" action:@selector(convertEOL:) key:@"" tag:SC_EOL_CR];
    NSMenu *macro = [self menu:@"Macro" parent:root];
    [self item:macro title:@"Start Recording" action:@selector(startRecording:) key:@"" tag:0];
    [self item:macro title:@"Stop Recording" action:@selector(stopRecording:) key:@"" tag:0];
    [self item:macro title:@"Play Last Recording" action:@selector(playRecording:) key:@"" tag:0];
    [self item:macro title:@"Save Recording…" action:@selector(saveRecording:) key:@"" tag:0];
    [self item:macro title:@"Run or Manage Saved Macros…" action:@selector(manageMacros:) key:@"" tag:0];
}
- (void)refreshFunctions {
    if (self.current && self.sidebarVisible) [self.workspace updateFunctions:Text(self.current.editor) path:self.current.path lexer:self.current.builtinLanguage.length?self.current.builtinLanguage:self.current.lexer];
}
// Modified 2026-11-01: panel commands toggle visibility and release sidebar space.
- (void)applySidebarVisibility {
    if(self.sidebarVisible){
        if(self.workspace.view.superview!=self.split)[self.split addSubview:self.workspace.view positioned:NSWindowBelow relativeTo:self.editorSplit];
        self.workspace.view.hidden=NO;
        [self.split setHoldingPriority:NSLayoutPriorityDefaultHigh forSubviewAtIndex:0];
        [self.split adjustSubviews];[self.split setPosition:MIN(285,self.split.bounds.size.width*0.35) ofDividerAtIndex:0];
    }else{
        [self.workspace.view removeFromSuperview];[self.split adjustSubviews];self.editorSplit.frame=self.split.bounds;
    }
}
- (void)showPanel:(id)sender {
    NSInteger panel=[sender tag];BOOL hide=self.sidebarVisible&&self.workspace.activePanel==panel;
    self.sidebarVisible=!hide;if(!hide)[self.workspace showPanel:panel];
    [self applySidebarVisibility];if(hide)[self.window makeFirstResponder:self.current.editor];[self tick:nil];
}
- (void)toggleSidebar:(id)sender {
    self.sidebarVisible=!self.sidebarVisible;[self applySidebarVisibility];
    if(!self.sidebarVisible)[self.window makeFirstResponder:self.current.editor];[self tick:nil];
}
- (BOOL)validateMenuItem:(NSMenuItem *)item {
    if(item.action==@selector(showPanel:))item.state=self.sidebarVisible&&self.workspace.activePanel==item.tag?NSControlStateValueOn:NSControlStateValueOff;
    else if(item.action==@selector(toggleSidebar:))item.state=self.sidebarVisible?NSControlStateValueOn:NSControlStateValueOff;
    return YES;
}
- (void)updateLineNumberMargin:(ScintillaView *)view {
    if(!view)return;
    NSString *digits=[NSString stringWithFormat:@"%ld",(long)MAX(99,Send(view,SCI_GETLINECOUNT))];
    sptr_t width=[self.preferences[@"lineNumbers"] boolValue]?Send(view,SCI_TEXTWIDTH,STYLE_LINENUMBER,reinterpret_cast<sptr_t>(digits.UTF8String))+8:0;
    if(Send(view,SCI_GETMARGINWIDTHN,0)!=width)Send(view,SCI_SETMARGINWIDTHN,0,width);
}
- (void)customizeMenus:(id)sender { [self.menuLayout customizeForWindow:self.window]; }
- (void)resetMenus:(id)sender { [self.menuLayout reset]; }
- (void)applyPreferences:(NXDocument *)doc {
    NSString *font = [self.preferences[@"font"] isKindOfClass:NSString.class] ? self.preferences[@"font"] : @"JetBrains Mono";
    double size = std::clamp<double>([self.preferences[@"fontSize"] doubleValue], 6, 96);
    Send(doc.editor, SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<sptr_t>(font.UTF8String));
    Send(doc.editor, SCI_STYLESETSIZEFRACTIONAL, STYLE_DEFAULT, size * 100);
    Send(doc.editor, SCI_STYLECLEARALL);
    Send(doc.editor, SCI_SETTABWIDTH, std::clamp<NSInteger>([self.preferences[@"tabWidth"] integerValue], 1, 16));
    Send(doc.editor, SCI_SETUSETABS, [self.preferences[@"useTabs"] boolValue]);
    Send(doc.editor,SCI_SETWRAPMODE,self.wrap?SC_WRAP_WORD:SC_WRAP_NONE);
    Send(doc.editor,SCI_SETINDENTATIONGUIDES,[self.preferences[@"indentGuides"] boolValue]?SC_IV_LOOKBOTH:SC_IV_NONE);
    for(NSArray *pair in @[@[@"\u200B",@"ZWSP"],@[@"\u200C",@"ZWNJ"],@[@"\u200D",@"ZWJ"],@[@"\u2060",@"WJ"],@[@"\uFEFF",@"BOM"],@[@"\u00A0",@"NBSP"],@[@"\u2028",@"LS"],@[@"\u2029",@"PS"],@[@"\u0085",@"NEL"]])Send(doc.editor,[self.preferences[@"nonPrinting"] boolValue]?SCI_SETREPRESENTATION:SCI_CLEARREPRESENTATION,reinterpret_cast<uptr_t>([pair[0] UTF8String]),reinterpret_cast<sptr_t>([pair[1] UTF8String]));
    Send(doc.editor,SCI_SETVIEWEOL,[self.preferences[@"showEOL"] boolValue]);
    Send(doc.editor,SCI_SETCARETLINEVISIBLE,[self.preferences[@"highlightLine"] boolValue]);
    Send(doc.editor,SCI_SETCARETWIDTH,std::clamp<NSInteger>([self.preferences[@"caretWidth"] integerValue],1,3));
    Send(doc.editor,SCI_SETCARETPERIOD,[self.preferences[@"blinkRate"] integerValue]);
    Send(doc.editor,SCI_SETEDGECOLUMN,[self.preferences[@"edgeColumn"] integerValue]);
    Send(doc.editor,SCI_SETEDGEMODE,[self.preferences[@"edgeColumn"] integerValue]>0?EDGE_LINE:EDGE_NONE);
    [self updateLineNumberMargin:doc.editor];
    Send(doc.editor, SCI_SETVIEWWS, [self.preferences[@"whitespace"] boolValue] ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE);
    Send(doc.editor, SCI_SETMULTIPLESELECTION, 1); Send(doc.editor, SCI_SETADDITIONALSELECTIONTYPING, 1); Send(doc.editor, SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH);
    Send(doc.editor, SCI_SETMARGINTYPEN, 1, SC_MARGIN_SYMBOL); Send(doc.editor, SCI_SETMARGINMASKN, 1, 1);
    Send(doc.editor, SCI_SETMARGINWIDTHN, 1, 14); Send(doc.editor, SCI_SETMARGINSENSITIVEN, 1, 1);
    Send(doc.editor, SCI_MARKERDEFINE, 0, SC_MARK_CIRCLE); Send(doc.editor, SCI_MARKERSETBACK, 0, 0x60b9ff);
    Send(doc.editor, SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL); Send(doc.editor, SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
    Send(doc.editor, SCI_SETMARGINWIDTHN, 2, 14); Send(doc.editor, SCI_SETMARGINSENSITIVEN, 2, 1);
    Send(doc.editor, SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS); Send(doc.editor, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);
    for (int marker : {SC_MARKNUM_FOLDEREND, SC_MARKNUM_FOLDERMIDTAIL, SC_MARKNUM_FOLDEROPENMID, SC_MARKNUM_FOLDERSUB, SC_MARKNUM_FOLDERTAIL}) Send(doc.editor, SCI_MARKERDEFINE, marker, SC_MARK_EMPTY);
    Send(doc.editor, SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CHANGE);
}
- (void)preferencesDialog:(id)sender { [self preferencesEditor:sender]; }
- (void)copyFormat:(id)sender {
    NoteX::SelectionFormat query; query.start = Send(self.current.editor, SCI_GETSELECTIONSTART); query.end = Send(self.current.editor, SCI_GETSELECTIONEND);
    Send(self.current.editor, NoteX::GetFormat, 0, reinterpret_cast<sptr_t>(&query));
    NoteX::State state; state.formats.push_back(query.value); state.runs.push_back({0, 1, 1});
    auto bytes = NoteX::encode(state); self.paintFormat = [NSData dataWithBytes:bytes.data() length:bytes.size()];
}
- (void)pasteFormat:(id)sender {
    if (!self.paintFormat) { NSBeep(); return; }
    NoteX::State state;
    if (NoteX::decode(std::string(static_cast<const char *>(self.paintFormat.bytes), self.paintFormat.length), state, 1) && state.formats.size() > 1) [self apply:state.formats[1] fields:NoteX::All];
}
- (void)saveAll:(id)sender {
    NSTabViewItem *selected = self.tabs.selectedTabViewItem;BOOL secondaryWasFocused=self.secondaryFocused;
    self.secondaryFocused=NO;self.secondaryDocument.activeEditor=nil;
    for (NXDocument *doc in [self.documents copy]) if (doc.dirty) { [self.tabs selectTabViewItem:doc.tab]; if (![self saveCurrentAs:NO]) break; }
    [self.tabs selectTabViewItem:selected];
    if(secondaryWasFocused&&self.secondaryVisible){self.secondaryFocused=YES;self.secondaryDocument.activeEditor=self.secondaryEditor;[self.window makeFirstResponder:self.secondaryEditor];}
}
- (void)reloadDocument:(id)sender {
    NXDocument *doc = self.current; if (!doc.path.length) return;
    NSAlert *alert = [NSAlert new]; alert.messageText = @"Reload this document from disk?";
    alert.informativeText = @"Unsaved changes in this tab will be replaced. A session snapshot is kept before reloading.";
    [alert addButtonWithTitle:@"Cancel"]; [alert addButtonWithTitle:@"Reload"];
    if ([alert runModal] != NSAlertSecondButtonReturn) return;
    NSError *error = nil; if (![self snapshot:&error]) { Alert(error.localizedDescription); return; }
    NSString *path = doc.path; doc.path = @"";
    if (![self openPath:path error:&error]) { doc.path = path; Alert(error.localizedDescription); return; }
    if(self.secondaryDocument==doc)[self bindSecondary:self.documents.lastObject];
    [self.tabs removeTabViewItem:doc.tab]; [self.documents removeObject:doc]; [self tick:nil];
}
- (void)convertEncoding:(id)sender {
    NSInteger type = [sender tag]; NXDocument *doc = self.current;
    doc.encoding = type < 2 ? NSUTF8StringEncoding : type == 2 ? NSUTF16LittleEndianStringEncoding : NSUTF16BigEndianStringEncoding;
    doc.bom = type != 0; doc.restoredDirty = YES; [self tick:nil];
}
- (void)convertEOL:(id)sender { Send(self.current.editor, SCI_CONVERTEOLS, [sender tag]); Send(self.current.editor, SCI_SETEOLMODE, [sender tag]); [self tick:nil]; }
- (void)foldAll:(id)sender { Send(self.current.editor, SCI_FOLDALL, [sender tag]); }
- (void)bookmark:(id)sender {
    auto *view = self.current.editor; auto line = Send(view, SCI_LINEFROMPOSITION, Send(view, SCI_GETCURRENTPOS)); NSInteger action = [sender tag];
    if (action == 0) Send(view, Send(view, SCI_MARKERGET, line) & 1 ? SCI_MARKERDELETE : SCI_MARKERADD, line, 0);
    else if (action == 2) Send(view, SCI_MARKERDELETEALL, 0);
    else {
        auto found = action > 0 ? Send(view, SCI_MARKERNEXT, line + 1, 1) : Send(view, SCI_MARKERPREVIOUS, line - 1, 1);
        if (found < 0) found = action > 0 ? Send(view, SCI_MARKERNEXT, 0, 1) : Send(view, SCI_MARKERPREVIOUS, Send(view, SCI_GETLINECOUNT) - 1, 1);
        if (found >= 0) Send(view, SCI_GOTOLINE, found); else NSBeep();
    }
}
- (void)transformLines:(id)sender {
    auto *view = self.current.editor; NSInteger action = [sender tag];
    Send(view, SCI_BEGINUNDOACTION);
    if (action == 0) {
        for (sptr_t line = Send(view, SCI_GETLINECOUNT) - 1; line >= 0; --line) {
            auto end = Send(view, SCI_GETLINEENDPOSITION, line), start = end;
            while (start > Send(view, SCI_POSITIONFROMLINE, line) && (Send(view, SCI_GETCHARAT, start - 1) == ' ' || Send(view, SCI_GETCHARAT, start - 1) == '\t')) --start;
            if (end > start) Send(view, SCI_DELETERANGE, start, end - start);
        }
    } else {
        auto first = Send(view, SCI_LINEFROMPOSITION, Send(view, SCI_GETSELECTIONSTART));
        auto last = Send(view, SCI_LINEFROMPOSITION, Send(view, SCI_GETSELECTIONEND));
        if (last > first && Send(view, SCI_GETSELECTIONEND) == Send(view, SCI_POSITIONFROMLINE, last)) --last;
        auto start = Send(view, SCI_POSITIONFROMLINE, first), end = Send(view, SCI_GETLINEENDPOSITION, last);
        NSData *all = Text(view); NSData *part = [all subdataWithRange:NSMakeRange(start, end - start)];
        NSString *text = [[NSString alloc] initWithData:part encoding:NSUTF8StringEncoding];
        NSString *eol = Send(view, SCI_GETEOLMODE) == SC_EOL_CRLF ? @"\r\n" : Send(view, SCI_GETEOLMODE) == SC_EOL_CR ? @"\r" : @"\n";
        NSArray *lines = [text componentsSeparatedByString:eol];
        if (action == 1) lines = [lines sortedArrayUsingSelector:@selector(localizedStandardCompare:)];
        else lines = [NSOrderedSet orderedSetWithArray:lines].array;
        NSData *replacement = [[lines componentsJoinedByString:eol] dataUsingEncoding:NSUTF8StringEncoding];
        Send(view, SCI_SETTARGETSTART, start); Send(view, SCI_SETTARGETEND, end);
        Send(view, SCI_REPLACETARGET, replacement.length, reinterpret_cast<sptr_t>(replacement.bytes));
    }
    Send(view, SCI_ENDUNDOACTION); [self tick:nil];
}
- (void)completeWord:(id)sender {
    auto *view = self.current.editor; auto end = Send(view, SCI_GETCURRENTPOS), start = Send(view, SCI_WORDSTARTPOSITION, end, 1);
    NSData *data = Text(view); if (end - start < 2 || data.length > 1024 * 1024) return;
    NSString *prefix = [[NSString alloc] initWithData:[data subdataWithRange:NSMakeRange(start, end - start)] encoding:NSUTF8StringEncoding];
    NSArray *words = [self.current.editor.string componentsSeparatedByCharactersInSet:NSCharacterSet.alphanumericCharacterSet.invertedSet];
    NSMutableSet *matches = [NSMutableSet new];
    for (NSString *word in words) if (word.length > prefix.length && [word hasPrefix:prefix]) { [matches addObject:word]; if (matches.count >= 200) break; }
    if (!matches.count) return;
    NSString *list = [[matches.allObjects sortedArrayUsingSelector:@selector(compare:)] componentsJoinedByString:@"\n"];
    Send(view, SCI_AUTOCSETSEPARATOR, '\n'); Send(view, SCI_AUTOCSHOW, end - start, reinterpret_cast<sptr_t>(list.UTF8String));
}
- (void)recordNotification:(SCNotification *)notification {
    auto *view = self.current.editor;
    if (notification->nmhdr.code == SCN_MACRORECORD && self.recording && !self.playingMacro) {
        unsigned message = notification->message;
        if (!MacroMessage(message) || self.recordingActions.count >= 10000) return;
        NSMutableDictionary *action = [@{@"message":@(message), @"w":@(notification->wParam), @"l":@(notification->lParam)} mutableCopy];
        if (StringMacroMessage(message)) {
            const char *text = reinterpret_cast<const char *>(notification->lParam);
            size_t length = message == SCI_ADDTEXT || message == SCI_APPENDTEXT ? notification->wParam : strlen(text ?: "");
            if (length > 1024 * 1024) return;
            action[@"text"] = [NSData dataWithBytes:text length:length]; action[@"l"] = @0;
        }
        [self.recordingActions addObject:action];
    }
    if (self.playingMacro) return;
    if (notification->nmhdr.code == SCN_MARGINCLICK) {
        auto line = Send(view, SCI_LINEFROMPOSITION, notification->position);
        if (notification->margin == 2) Send(view, SCI_TOGGLEFOLD, line);
        if (notification->margin == 1) Send(view, Send(view, SCI_MARKERGET, line) & 1 ? SCI_MARKERDELETE : SCI_MARKERADD, line, 0);
    }
    if (notification->nmhdr.code == SCN_UPDATEUI && [self.preferences[@"braceMatch"] boolValue]) {
        auto caret = Send(view, SCI_GETCURRENTPOS), position = caret > 0 ? caret - 1 : caret;
        auto character = Send(view, SCI_GETCHARAT, position);
        if (character && strchr("()[]{}", static_cast<int>(character))) { auto mate = Send(view, SCI_BRACEMATCH, position); Send(view, SCI_BRACEHIGHLIGHT, position, mate); }
        else Send(view, SCI_BRACEHIGHLIGHT, -1, -1);
    }
    if (notification->nmhdr.code == SCN_CHARADDED) {
        if (notification->ch == '\n' && [self.preferences[@"autoIndent"] boolValue]) {
            auto line = Send(view, SCI_LINEFROMPOSITION, Send(view, SCI_GETCURRENTPOS));
            if (line > 0) { Send(view, SCI_SETLINEINDENTATION, line, Send(view, SCI_GETLINEINDENTATION, line - 1)); Send(view, SCI_GOTOPOS, Send(view, SCI_GETLINEINDENTPOSITION, line)); }
        } else if(notification->ch=='('&&[self.preferences[@"parameterHints"] boolValue])[self parameterHint:nil];
        else if([self.preferences[@"functionCompletion"] boolValue])[self completeFunction:nil];
        else if ([self.preferences[@"autoComplete"] boolValue]) [self completeWord:nil];
    }
}
- (void)startRecording:(id)sender {
    if (self.recording) return;
    self.recordingActions = [NSMutableArray new]; self.recording = YES;
    for (NXDocument *doc in self.documents) Send(doc.primaryEditor, SCI_STARTRECORD);
    if(self.secondaryVisible) Send(self.secondaryEditor, SCI_STARTRECORD);
    self.status.stringValue = @"Recording macro…";
}
- (void)stopRecording:(id)sender { self.recording = NO; for (NXDocument *doc in self.documents) Send(doc.primaryEditor, SCI_STOPRECORD); Send(self.secondaryEditor, SCI_STOPRECORD); [self tick:nil]; }
- (void)playActions:(NSArray *)actions repetitions:(NSUInteger)repetitions {
    if (self.recording || self.playingMacro || actions.count > 10000 || repetitions > 1000) return;
    if (actions.count * repetitions > 100000) { Alert(@"This playback exceeds 100,000 editing actions. Reduce the repeat count."); return; }
    auto *view = self.current.editor; self.playingMacro = YES;
    Send(view, SCI_BEGINUNDOACTION);
    for (NSUInteger iteration = 0; iteration < repetitions; ++iteration) for (NSDictionary *action in actions) {
        unsigned message = [action[@"message"] unsignedIntValue]; if (!MacroMessage(message)) continue;
        uptr_t w = [action[@"w"] unsignedLongLongValue]; sptr_t l = [action[@"l"] longLongValue];
        if (StringMacroMessage(message)) {
            NSData *data = [action[@"text"] isKindOfClass:NSData.class] ? action[@"text"] : NSData.data;
            if (data.length > 1024 * 1024) continue;
            std::string text(static_cast<const char *>(data.bytes), data.length);
            if (message == SCI_ADDTEXT || message == SCI_APPENDTEXT) w = data.length;
            // An insert-at-caret recording must stay relative to the replay caret.
            if (message == SCI_INSERTTEXT) w = static_cast<uptr_t>(-1);
            Send(view, message, w, reinterpret_cast<sptr_t>(text.c_str()));
        } else Send(view, message, w, l);
    }
    Send(view, SCI_ENDUNDOACTION); self.playingMacro = NO; [self tick:nil];
}
- (void)playRecording:(id)sender { [self playActions:self.recordingActions repetitions:1]; }
- (void)saveRecording:(id)sender {
    [self stopRecording:nil]; if (!self.recordingActions.count) { Alert(@"Record at least one editing action first."); return; }
    NSAlert *alert = [NSAlert new]; alert.messageText = @"Save Macro";
    NSTextField *name = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 280, 24)]; name.stringValue = @"My Macro"; alert.accessoryView = name;
    [alert addButtonWithTitle:@"Save"]; [alert addButtonWithTitle:@"Cancel"];
    if ([alert runModal] == NSAlertFirstButtonReturn && name.stringValue.length) { [self.macros addObject:@{@"name":name.stringValue, @"actions":[self.recordingActions copy]}]; [self tick:nil]; }
}
- (void)manageMacros:(id)sender {
    if (!self.macros.count) { Alert(@"No saved macros. Use Start Recording, edit the document, then Save Recording."); return; }
    NSAlert *alert = [NSAlert new]; alert.messageText = @"Saved Macros";
    NSPopUpButton *choice = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(0, 0, 280, 28) pullsDown:NO]; [choice addItemsWithTitles:[self.macros valueForKey:@"name"]];
    NSTextField *repeats = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 70, 24)]; repeats.stringValue = @"1";
    NSStackView *stack = [NSStackView stackViewWithViews:@[choice, [NSTextField labelWithString:@"Repeat (1–1000)"], repeats]]; stack.orientation = NSUserInterfaceLayoutOrientationVertical; stack.frame = NSMakeRect(0, 0, 300, 96); alert.accessoryView = stack;
    [alert addButtonWithTitle:@"Run"]; [alert addButtonWithTitle:@"Cancel"]; [alert addButtonWithTitle:@"Delete"];
    auto result = [alert runModal];
    if (result == NSAlertFirstButtonReturn) [self playActions:self.macros[choice.indexOfSelectedItem][@"actions"] repetitions:std::clamp<NSInteger>(repeats.integerValue, 1, 1000)];
    else if (result == NSAlertThirdButtonReturn) { [self.macros removeObjectAtIndex:choice.indexOfSelectedItem]; [self tick:nil]; }
}
- (NSAttributedString *)formattedDocument {
    NSData *bytes = Text(self.current.editor); NSString *text = [[NSString alloc] initWithData:bytes encoding:NSUTF8StringEncoding];
    NSFont *base = [NSFont fontWithName:self.preferences[@"font"] size:[self.preferences[@"fontSize"] doubleValue]] ?: [NSFont monospacedSystemFontOfSize:13 weight:NSFontWeightRegular];
    NSMutableAttributedString *output = [[NSMutableAttributedString alloc] initWithString:text attributes:@{NSFontAttributeName:base, NSForegroundColorAttributeName:NSColor.blackColor}];
    auto state = State(self.current.editor);
    for (auto run : state.runs) {
        NSString *prefix = [[NSString alloc] initWithBytes:bytes.bytes length:run.start encoding:NSUTF8StringEncoding];
        NSString *part = [[NSString alloc] initWithBytes:static_cast<const char *>(bytes.bytes) + run.start length:run.end - run.start encoding:NSUTF8StringEncoding];
        if (!prefix || !part) continue;
        auto f = state.formats[run.style];
        NSFont *font = [NSFont fontWithName:f.mask & NoteX::Family ? @(f.family.c_str()) : base.fontName size:f.mask & NoteX::Size ? f.size / 100.0 : base.pointSize] ?: base;
        if (f.bold) font = [NSFontManager.sharedFontManager convertFont:font toHaveTrait:NSBoldFontMask];
        if (f.italic) font = [NSFontManager.sharedFontManager convertFont:font toHaveTrait:NSItalicFontMask];
        NSMutableDictionary *attributes = [@{NSFontAttributeName:font, NSUnderlineStyleAttributeName:@(f.underline ? NSUnderlineStyleSingle : 0), NSStrikethroughStyleAttributeName:@(f.strike ? NSUnderlineStyleSingle : 0)} mutableCopy];
        auto color = [](unsigned value) { return [NSColor colorWithSRGBRed:(value & 255) / 255.0 green:((value >> 8) & 255) / 255.0 blue:((value >> 16) & 255) / 255.0 alpha:1]; };
        if (f.mask & NoteX::Fore) attributes[NSForegroundColorAttributeName] = color(f.fore);
        if (f.mask & (NoteX::Back | NoteX::Highlight)) attributes[NSBackgroundColorAttributeName] = color(f.mask & NoteX::Highlight ? f.highlight : f.back);
        [output addAttributes:attributes range:NSMakeRange(prefix.length, part.length)];
    }
    return output;
}
- (void)exportFormatted:(id)sender {
    NSSavePanel *panel = [NSSavePanel savePanel]; panel.nameFieldStringValue = [self.current.title.stringByDeletingPathExtension stringByAppendingPathExtension:[sender tag] ? @"html" : @"rtf"];
    if ([panel runModal] != NSModalResponseOK) return;
    NSAttributedString *text = [self formattedDocument]; NSError *error = nil;
    NSData *data = [text dataFromRange:NSMakeRange(0, text.length) documentAttributes:@{NSDocumentTypeDocumentAttribute:[sender tag] ? NSHTMLTextDocumentType : NSRTFTextDocumentType} error:&error];
    if (!data || !NXWrite(data, panel.URL.path, &error)) Alert(error.localizedDescription);
}
- (void)copyRichText:(id)sender {
    NSAttributedString *all = [self formattedDocument]; NSRange range = self.current.editor.selectedRange;
    if (NSMaxRange(range) > all.length) return;
    NSAttributedString *text = [all attributedSubstringFromRange:range];
    NSData *rtf = [text dataFromRange:NSMakeRange(0, text.length) documentAttributes:@{NSDocumentTypeDocumentAttribute:NSRTFTextDocumentType} error:nil];
    NSPasteboard *board = NSPasteboard.generalPasteboard; [board clearContents]; [board setString:text.string forType:NSPasteboardTypeString]; if (rtf) [board setData:rtf forType:NSPasteboardTypeRTF];
}
- (void)printDocument:(id)sender {
    NSPrintInfo *info = [NSPrintInfo.sharedPrintInfo copy]; info.horizontalPagination = NSPrintingPaginationModeFit;
    CGFloat width = info.paperSize.width - info.leftMargin - info.rightMargin;
    NSTextView *view = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, width, 100)];
    [view.textStorage setAttributedString:[self formattedDocument]]; view.verticallyResizable = YES;
    view.textContainer.containerSize = NSMakeSize(width, CGFLOAT_MAX); view.textContainer.widthTracksTextView = YES;
    [view.layoutManager ensureLayoutForTextContainer:view.textContainer];
    view.frame = NSMakeRect(0, 0, width, [view.layoutManager usedRectForTextContainer:view.textContainer].size.height + 24);
    NSPrintOperation *operation=[NSPrintOperation printOperationWithView:view printInfo:info];operation.showsPrintPanel=[sender tag]!=1;[operation runOperation];
}
@end
