// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native split views, configuration, and search integration, created 2026-11-01.
@implementation NXApp (Workbench)
- (void)buildWorkbenchMenus {
    NSMenu *root=NSApp.mainMenu,*app=[root itemAtIndex:0].submenu,*view=[root itemWithTitle:@"View"].submenu,*edit=[root itemWithTitle:@"Edit"].submenu;
    [self item:app title:@"Shortcut Editor…" action:@selector(shortcutEditor:) key:@"" tag:0];
    [self item:app title:@"Language Editor…" action:@selector(languageEditor:) key:@"" tag:0];
    [self item:app title:@"Theme Editor…" action:@selector(themeEditor:) key:@"" tag:0];
    [self item:view title:@"Clone to Other View" action:@selector(cloneView:) key:@"" tag:0];
    [self item:view title:@"Split Vertically" action:@selector(cloneView:) key:@"" tag:1];
    [self item:view title:@"Split Horizontally" action:@selector(cloneView:) key:@"" tag:2];
    [self item:view title:@"Close Other View" action:@selector(closeSecondary:) key:@"" tag:0];
    [self item:view title:@"Synchronize Scrolling" action:@selector(toggleSynchronizedScroll:) key:@"" tag:0];
    [self item:view title:@"Choose Theme…" action:@selector(chooseTheme:) key:@"" tag:0];
    [self item:view title:@"Choose User-Defined Language…" action:@selector(chooseLanguage:) key:@"" tag:0];
    [self item:edit title:@"Find in Files…" action:@selector(findInFiles:) key:@"F" tag:0];
}
- (void)buildSecondaryView {
    self.editorSplit=[[NSSplitView alloc]initWithFrame:NSMakeRect(286,0,834,600)];self.editorSplit.vertical=YES;self.editorSplit.dividerStyle=NSSplitViewDividerStyleThin;self.editorSplit.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;
    self.tabs.frame=self.editorSplit.bounds;[self.editorSplit addSubview:self.tabs];
    self.secondaryHost=[[NSView alloc]initWithFrame:NSMakeRect(0,0,410,600)];self.secondaryHost.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;
    self.secondaryChoice=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(6,568,290,28) pullsDown:NO];self.secondaryChoice.autoresizingMask=NSViewWidthSizable|NSViewMinYMargin;self.secondaryChoice.target=self;self.secondaryChoice.action=@selector(selectSecondary:);[self.secondaryHost addSubview:self.secondaryChoice];
    NSButton *close=[NSButton buttonWithTitle:@"Close View" target:self action:@selector(closeSecondary:)];close.frame=NSMakeRect(300,568,110,28);close.autoresizingMask=NSViewMinXMargin|NSViewMinYMargin;[self.secondaryHost addSubview:close];
    self.secondaryEditor=[[NXEditor alloc]initWithFrame:NSMakeRect(0,0,410,560)];__weak NXApp *owner=self;((NXEditor *)self.secondaryEditor).editorContextMenu=^NSMenu *{return owner.editorContextMenu;};((NXEditor *)self.secondaryEditor).typedKey=^BOOL(ScintillaView *view,NSString *key){return [owner pairedInput:view key:key];};((NXEditor *)self.secondaryEditor).newline=^BOOL(ScintillaView *view){return [owner insertEditorNewline:view];};self.secondaryEditor.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;Send(self.secondaryEditor,SCI_SETCODEPAGE,SC_CP_UTF8);self.secondaryEditor.delegate=self;[self.secondaryHost addSubview:self.secondaryEditor];
}
- (void)bindSecondary:(NXDocument *)document {
    if(!document)return;
    self.secondaryDocument.activeEditor=nil;self.secondaryDocument=document;
    Send(self.secondaryEditor,SCI_SETDOCPOINTER,0,Send(document.primaryEditor,SCI_GETDOCPOINTER));Send(self.secondaryEditor,SCI_SETREADONLY,Send(document.primaryEditor,SCI_GETREADONLY));
    ScintillaView *previous=document.activeEditor;document.activeEditor=self.secondaryEditor;[self applyPreferences:document];[self theme:document];document.activeEditor=previous;
    [self.secondaryChoice removeAllItems];for(NXDocument *doc in self.documents){[self.secondaryChoice addItemWithTitle:doc.title];self.secondaryChoice.lastItem.representedObject=doc.identifier;}
    [self.secondaryChoice selectItemAtIndex:[self.documents indexOfObject:document]];
    Send(self.secondaryEditor,SCI_SETWRAPMODE,self.wrap?SC_WRAP_WORD:SC_WRAP_NONE);
    if(self.recording)Send(self.secondaryEditor,SCI_STARTRECORD);
    if(self.secondaryFocused)document.activeEditor=self.secondaryEditor;
}
- (void)updateSecondaryChoices {
    if(!self.secondaryVisible)return;NSArray *titles=[self.documents valueForKey:@"title"];
    if(![self.secondaryChoice.itemTitles isEqual:titles]){[self.secondaryChoice removeAllItems];[self.secondaryChoice addItemsWithTitles:titles];NSInteger index=[self.documents indexOfObject:self.secondaryDocument];if(index>=0&&index<(NSInteger)self.documents.count)[self.secondaryChoice selectItemAtIndex:index];}
}
- (void)cloneView:(id)sender {
    NXDocument *document=self.current;if(!document)return;
    if(!self.secondaryVisible){[self.editorSplit addSubview:self.secondaryHost];self.secondaryVisible=YES;}
    if([sender tag]==1)self.editorSplit.vertical=YES;else if([sender tag]==2)self.editorSplit.vertical=NO;
    [self bindSecondary:document];[self.editorSplit adjustSubviews];[self.editorSplit setPosition:(self.editorSplit.vertical?self.editorSplit.bounds.size.width:self.editorSplit.bounds.size.height)/2 ofDividerAtIndex:0];
    Send(self.secondaryEditor,SCI_SETSEL,Send(document.primaryEditor,SCI_GETANCHOR),Send(document.primaryEditor,SCI_GETCURRENTPOS));
    self.secondaryFocused=YES;document.activeEditor=self.secondaryEditor;[self.window makeFirstResponder:self.secondaryEditor];[self tick:nil];
}
- (void)selectSecondary:(id)sender {NSInteger i=self.secondaryChoice.indexOfSelectedItem;if(i>=0&&i<(NSInteger)self.documents.count){[self bindSecondary:self.documents[i]];self.secondaryFocused=YES;self.secondaryDocument.activeEditor=self.secondaryEditor;[self.window makeFirstResponder:self.secondaryEditor];[self tick:nil];}}
- (void)closeSecondary:(id)sender {
    for(NXDocument *doc in self.documents)doc.secondaryOnly=NO;
    self.secondaryDocument.activeEditor=nil;self.secondaryFocused=NO;self.secondaryVisible=NO;self.secondaryDocument=nil;
    [self.secondaryHost removeFromSuperview];Send(self.secondaryEditor,SCI_SETDOCPOINTER,0,0);[self.editorSplit adjustSubviews];[self.window makeFirstResponder:self.current.primaryEditor];[self tick:nil];
}
- (void)toggleSynchronizedScroll:(id)sender {self.synchronizedScroll=!self.synchronizedScroll;self.syncVertical=self.synchronizedScroll;self.syncHorizontal=self.synchronizedScroll;[self tick:nil];}
- (NSDictionary *)secondaryRecord {
    if(!self.secondaryVisible||!self.secondaryDocument)return @{};
    CGFloat total=self.editorSplit.vertical?self.editorSplit.bounds.size.width:self.editorSplit.bounds.size.height;
    CGFloat first=self.editorSplit.vertical?self.tabs.frame.size.width:self.tabs.frame.size.height;
    return @{@"document":self.secondaryDocument.identifier,@"vertical":@(self.editorSplit.vertical),@"ratio":@(total>0?first/total:0.5),@"zoom":@(Send(self.secondaryEditor,SCI_GETZOOM)),@"xOffset":@(Send(self.secondaryEditor,SCI_GETXOFFSET)),@"focused":@(self.secondaryFocused),@"synchronized":@(self.synchronizedScroll),@"syncVertical":@(self.syncVertical),@"syncHorizontal":@(self.syncHorizontal),@"caret":@(Send(self.secondaryEditor,SCI_GETCURRENTPOS)),@"anchor":@(Send(self.secondaryEditor,SCI_GETANCHOR)),@"firstLine":@(Send(self.secondaryEditor,SCI_GETFIRSTVISIBLELINE))};
}
- (void)restoreSecondary:(NSDictionary *)record {
    if(![record[@"document"] length])return;
    for(NXDocument *doc in self.documents)if([doc.identifier isEqual:record[@"document"]]){
        [self.editorSplit addSubview:self.secondaryHost];self.secondaryVisible=YES;self.editorSplit.vertical=[record[@"vertical"] boolValue];[self bindSecondary:doc];[self.editorSplit adjustSubviews];
        Send(self.secondaryEditor,SCI_SETSEL,[record[@"anchor"] unsignedLongLongValue],[record[@"caret"] longLongValue]);Send(self.secondaryEditor,SCI_SETFIRSTVISIBLELINE,[record[@"firstLine"] unsignedLongLongValue]);Send(self.secondaryEditor,SCI_SETZOOM,[record[@"zoom"] integerValue]);Send(self.secondaryEditor,SCI_SETXOFFSET,[record[@"xOffset"] integerValue]);CGFloat total=self.editorSplit.vertical?self.editorSplit.bounds.size.width:self.editorSplit.bounds.size.height;[self.editorSplit setPosition:total*std::clamp<double>(record[@"ratio"]?[record[@"ratio"] doubleValue]:0.5,0.15,0.85) ofDividerAtIndex:0];self.synchronizedScroll=[record[@"synchronized"] boolValue];self.syncVertical=record[@"syncVertical"]?[record[@"syncVertical"] boolValue]:YES;self.syncHorizontal=record[@"syncHorizontal"]?[record[@"syncHorizontal"] boolValue]:YES;self.secondaryFocused=[record[@"focused"] boolValue];if(self.secondaryFocused)doc.activeEditor=self.secondaryEditor;break;
    }
}
- (NSDictionary *)userLanguage:(NXDocument *)document {for(NSDictionary *language in self.configuration[@"languages"])if([language[@"id"] isEqual:document.customLanguage])return language;return nil;}
- (void)applyUserTheme:(NXDocument *)document {
    NSDictionary *theme=nil;for(NSDictionary *candidate in self.configuration[@"themes"])if([candidate[@"id"] isEqual:self.configuration[@"selectedTheme"]])theme=candidate;
    if(!theme){NSMutableDictionary *standard=[NXDefaultTheme() mutableCopy];if(!self.dark)[standard addEntriesFromDictionary:@{@"background":@"FFFFFF",@"foreground":@"282828",@"caret":@"222222",@"selection":@"B7DBEE",@"lineNumbers":@"777777",@"currentLine":@"F1F5F9",@"keyword1":@"6040A0",@"string":@"996622",@"comment":@"337733",@"number":@"985600",@"operator":@"444444"}];theme=standard;}
    NXApplyTheme(document.editor,theme,document.lexer,[self userLanguage:document]!=nil);
    if([[self userLanguage:document][@"advancedRules"] boolValue])NXApplyUserLanguageTheme(document.editor,theme);
    NXApplyStyleOverrides(document.editor,theme[@"styles"],document.customLanguage.length?[@"user:" stringByAppendingString:document.customLanguage]:document.lexer);
}
- (void)updateUserSyntax {
    for(NXDocument *doc in self.documents){NSDictionary *language=[self userLanguage:doc];if(!language||[language[@"advancedRules"] boolValue])continue;NSData *hash=NXDigest(Text(doc.primaryEditor));
        if(![self.syntaxHashes[doc.identifier] isEqual:hash]){NXStyleLanguage(doc.primaryEditor,language);self.syntaxHashes[doc.identifier]=hash;}
    }
}
- (void)refreshEditorSettings {
    [self refreshLanguageChoices];
    BOOL focused=self.secondaryFocused;for(NXDocument *doc in self.documents){doc.activeEditor=nil;[self applyPreferences:doc];[self theme:doc];}
    if(self.secondaryVisible){self.secondaryDocument.activeEditor=self.secondaryEditor;[self applyPreferences:self.secondaryDocument];[self theme:self.secondaryDocument];if(!focused)self.secondaryDocument.activeEditor=nil;}
    [self.syntaxHashes removeAllObjects];[self updateUserSyntax];[self updateControls];
}
- (void)refreshLanguageChoices {
    for(NSMenuItem *item in [self.language.itemArray copy])if(item.representedObject)[self.language.menu removeItem:item];
    for(NSDictionary *language in self.configuration[@"languages"]){[self.language addItemWithTitle:[@"User: " stringByAppendingString:language[@"name"]]];self.language.lastItem.representedObject=language[@"id"];}
}
- (BOOL)commitConfiguration:(NSDictionary *)configuration {
    if(!NXValidateConfiguration(configuration)){Alert(@"The configuration is invalid.");return NO;}
    NSMutableDictionary *old=self.configuration;self.configuration=[configuration mutableCopy];NSError *error=nil;
    if(![self snapshot:&error]){self.configuration=old;Alert(error.localizedDescription);return NO;}
    [self refreshEditorSettings];return YES;
}
- (void)preferencesEditor:(id)sender {
    NXRecordEditor *editor=[[NXRecordEditor alloc]initWithKind:@"preferences" records:@[self.preferences]];NSArray *result=[editor runForWindow:self.window];if(!result)return;
    NSMutableDictionary *previous=self.preferences,*next=[self.preferences mutableCopy];[next addEntriesFromDictionary:result[0]];
    if(![NSFontManager.sharedFontManager.availableFontFamilies containsObject:next[@"font"]]){Alert(@"Choose an installed font family.");return;}
    self.preferences=next;NSError *error=nil;if(![self snapshot:&error]){self.preferences=previous;Alert(error.localizedDescription);return;}
    self.dark=[next[@"dark"] boolValue];self.wrap=[next[@"wrap"] boolValue];self.window.appearance=[NSAppearance appearanceNamed:self.dark?NSAppearanceNameDarkAqua:NSAppearanceNameAqua];
    [self.timer invalidate];self.timer=[NSTimer scheduledTimerWithTimeInterval:[next[@"snapshotSeconds"] doubleValue] target:self selector:@selector(tick:) userInfo:nil repeats:YES];[self refreshEditorSettings];[self tick:nil];
}
- (void)languageEditor:(id)sender {
    NXRecordEditor *editor=[[NXRecordEditor alloc]initWithKind:@"languages" records:self.configuration[@"languages"]];NSArray *result=[editor runForWindow:self.window];if(!result)return;
    NSMutableDictionary *next=[self.configuration mutableCopy];next[@"languages"]=result;if(![self commitConfiguration:next])return;
    for(NXDocument *doc in self.documents){if(![self userLanguage:doc])doc.customLanguage=NXLanguageForPath(self.configuration,doc.path)[@"id"] ?: @"";}[self refreshEditorSettings];[self tick:nil];
}
- (void)themeEditor:(id)sender {
    NXRecordEditor *editor=[[NXRecordEditor alloc]initWithKind:@"themes" records:self.configuration[@"themes"]];NSArray *result=[editor runForWindow:self.window];if(!result)return;
    NSMutableDictionary *next=[self.configuration mutableCopy];next[@"themes"]=result;if(![[result valueForKey:@"id"] containsObject:next[@"selectedTheme"]])next[@"selectedTheme"]=@"";if([self commitConfiguration:next]&&result.count)[self chooseTheme:nil];
}
- (void)shortcutEditor:(id)sender {[self.menuLayout editShortcutsForWindow:self.window];}
- (NSInteger)chooseRecord:(NSArray *)records title:(NSString *)title first:(NSString *)first {
    NSAlert *alert=[NSAlert new];alert.messageText=title;NSPopUpButton *choice=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(0,0,340,28) pullsDown:NO];[choice addItemWithTitle:first];[choice addItemsWithTitles:[records valueForKey:@"name"]];alert.accessoryView=choice;[alert addButtonWithTitle:@"Apply"];[alert addButtonWithTitle:@"Cancel"];return [alert runModal]==NSAlertFirstButtonReturn?choice.indexOfSelectedItem:-1;
}
- (void)chooseTheme:(id)sender {NSArray *themes=self.configuration[@"themes"];NSInteger index=[self chooseRecord:themes title:@"Choose Theme" first:@"Default Dark / Light"];if(index<0)return;NSMutableDictionary *next=[self.configuration mutableCopy];next[@"selectedTheme"]=index?themes[index-1][@"id"]:@"";[self commitConfiguration:next];}
- (void)chooseLanguage:(id)sender {NSArray *languages=self.configuration[@"languages"];NSInteger index=[self chooseRecord:languages title:@"User-Defined Language" first:@"Use Selected Syntax Lexer"];if(index<0)return;self.current.customLanguage=index?languages[index-1][@"id"]:@"";[self refreshEditorSettings];[self tick:nil];}
- (void)collectProjectFiles:(NSDictionary *)node into:(NSMutableArray *)files {if([node[@"kind"] isEqual:@"file"])[files addObject:node[@"path"]];for(NSDictionary *child in node[@"children"])[self collectProjectFiles:child into:files];}
- (NSArray *)searchScope:(NSInteger)scope {
    if(scope==3){NXDocument *doc=self.current;return @[@{@"id":doc.identifier,@"path":doc.path,@"title":doc.title,@"text":Text(doc.primaryEditor)}];}
    NSMutableArray *files=[NSMutableArray new];if(scope==1){for(NXDocument *doc in self.documents)[files addObject:@{@"id":doc.identifier,@"path":doc.path,@"title":doc.title,@"text":Text(doc.primaryEditor)}];}
    else for(NSDictionary *root in self.workspace.projects)[self collectProjectFiles:root into:files];return files;
}
- (void)findInFiles:(id)sender {
    if(!self.fileSearch){self.fileSearch=[NXFileSearch new];__weak NXApp *weak=self;self.fileSearch.scopeFiles=^NSArray *(NSInteger scope){return [weak searchScope:scope];};self.fileSearch.navigate=^(NSDictionary *result){[weak navigateResult:result];};self.fileSearch.replaceResults=^NSString *(NSArray *results){return [weak applySearchResults:results];};self.fileSearch.showBackups=^{NSString *path=[weak.storage.directory stringByAppendingPathComponent:@"ReplaceBackups"];[NSFileManager.defaultManager createDirectoryAtPath:path withIntermediateDirectories:YES attributes:@{NSFilePosixPermissions:@0700} error:nil];[NSWorkspace.sharedWorkspace openURL:[NSURL fileURLWithPath:path]];};}
    [self.fileSearch showForWindow:self.window];
}
- (NXDocument *)documentForResult:(NSDictionary *)result {for(NXDocument *doc in self.documents)if(([result[@"id"] length]&&[doc.identifier isEqual:result[@"id"]])||([result[@"path"] length]&&[NXCanonicalPath(doc.path) isEqual:NXCanonicalPath(result[@"path"])]))return doc;return nil;}
- (void)navigateResult:(NSDictionary *)result {
    NXDocument *doc=[self documentForResult:result];NSError *error=nil;
    if(!doc){self.openingEncoding=[result[@"encoding"] unsignedLongLongValue];BOOL opened=[result[@"path"] length]&&[self openPath:result[@"path"] error:&error];self.openingEncoding=0;if(!opened){Alert(error.localizedDescription ?: @"This tab is no longer open. Run the search again.");return;}doc=self.current;}
    if(![NXDigest(Text(doc.primaryEditor)) isEqual:result[@"textHash"]]){Alert(@"This document changed after the search. Run the search again before navigating to the match.");return;}
    [self activateDocument:doc];Send(doc.editor,SCI_SETSEL,[result[@"position"] unsignedLongLongValue],[result[@"position"] longLongValue]+[result[@"length"] longLongValue]);[self.window makeKeyAndOrderFront:nil];[self.window makeFirstResponder:doc.editor];
}
- (NSString *)applySearchResults:(NSArray *)results {
    NSError *error=nil;if(![self snapshot:&error])return error.localizedDescription;
    NSMutableDictionary *groups=[NSMutableDictionary new];for(NSDictionary *r in results){NSString *key=[r[@"id"] length]?r[@"id"]:r[@"path"];if(!groups[key])groups[key]=[NSMutableArray new];[groups[key] addObject:r];}
    if(groups.count>1000)return @"More than 1,000 documents. Narrow the search before replacing.";
    NSString *backup=[self.storage.directory stringByAppendingPathComponent:[@"ReplaceBackups/" stringByAppendingString:NSUUID.UUID.UUIDString]];
    if(![NSFileManager.defaultManager createDirectoryAtPath:backup withIntermediateDirectories:YES attributes:@{NSFilePosixPermissions:@0700} error:&error])return error.localizedDescription;
    NSMutableArray *report=[NSMutableArray new],*recovery=[NSMutableArray new];NSUInteger changed=0,skipped=0;NXDocument *selected=self.current;
    for(NSString *key in [[groups allKeys] sortedArrayUsingSelector:@selector(compare:)]){@autoreleasepool{
        NSArray *matches=[groups[key] sortedArrayUsingComparator:^NSComparisonResult(NSDictionary *a,NSDictionary *b){return [b[@"position"] compare:a[@"position"]];}];NSDictionary *first=matches[0];NSString *path=first[@"path"];BOOL inMemory=[first[@"id"] length]>0;
        NXDocument *doc=[self documentForResult:first];BOOL wasOpen=doc!=nil;NSString *reason=nil;NSData *disk=path.length&&!inMemory?[NSData dataWithContentsOfFile:path]:nil;
        if([first[@"id"] length]&&!doc)reason=@"The searched tab was closed.";
        if(path.length&&!inMemory&&(!disk||(![first[@"id"] length]&&![NXDigest(disk) isEqual:first[@"diskHash"]])))reason=@"The file changed or is unavailable.";
        if(doc&&path.length&&!inMemory&&![NXDigest(disk) isEqual:doc.diskHash])reason=@"The file changed outside the editor.";
        if(doc&&path.length&&!inMemory&&doc.dirty)reason=@"The open document has unsaved changes.";
        if(doc&&![NXDigest(Text(doc.primaryEditor)) isEqual:first[@"textHash"]])reason=@"The document changed after the preview.";
        NSString *companion=[path stringByAppendingString:@".notex"];NSData *sidecar=path.length&&!inMemory?[NSData dataWithContentsOfFile:companion]:nil;
        if(sidecar){NoteX::State state;NSString *text=NXDecodeText(disk) ?: NXDecodeEncoding(disk,[first[@"encoding"] unsignedLongLongValue]);if(!text||!NXReadCompanion(sidecar,[text dataUsingEncoding:NSUTF8StringEncoding],state))reason=@"The formatting companion is invalid or belongs to different text.";}
        NSString *prefix=[NSString stringWithFormat:@"%04lu-",(unsigned long)(changed+skipped)];
        if(!reason&&path.length&&!inMemory){if(!NXWrite(disk,[backup stringByAppendingPathComponent:[prefix stringByAppendingString:path.lastPathComponent]],&error)||(sidecar&&!NXWrite(sidecar,[backup stringByAppendingPathComponent:[[prefix stringByAppendingString:path.lastPathComponent] stringByAppendingString:@".notex"]],&error)))reason=error.localizedDescription;}
        if(!reason&&path.length&&!inMemory){
            NSDictionary *attributes=[NSFileManager.defaultManager attributesOfItemAtPath:path error:&error];
            [recovery addObject:@{@"path":path,@"backup":[prefix stringByAppendingString:path.lastPathComponent],@"hasCompanion":@(sidecar!=nil),@"permissions":attributes[NSFilePosixPermissions] ?: @0600}];
            NSData *journal=[NSJSONSerialization dataWithJSONObject:recovery options:NSJSONWritingPrettyPrinted error:&error];
            if(!journal||!NXWrite(journal,[backup stringByAppendingPathComponent:@"recovery.json"],&error))reason=error.localizedDescription ?: @"The recovery map could not be saved.";
        }
        if(!reason&&!doc){self.openingEncoding=[first[@"encoding"] unsignedLongLongValue];if(![self openPath:path error:&error])reason=error.localizedDescription;else doc=self.current;self.openingEncoding=0;}
        if(!reason&&![NXDigest(Text(doc.primaryEditor)) isEqual:first[@"textHash"]])reason=@"The file changed while it was being opened.";
        if(reason){++skipped;[report addObject:@{@"path":path,@"status":@"skipped",@"reason":reason}];continue;}
        auto *view=doc.primaryEditor;Send(view,SCI_BEGINUNDOACTION);for(NSDictionary *match in matches){NSData *replacement=[match[@"replacement"] dataUsingEncoding:NSUTF8StringEncoding];Send(view,SCI_SETTARGETSTART,[match[@"position"] unsignedLongLongValue]);Send(view,SCI_SETTARGETEND,[match[@"position"] unsignedLongLongValue]+[match[@"length"] unsignedLongLongValue]);Send(view,SCI_REPLACETARGET,replacement.length,reinterpret_cast<sptr_t>(replacement.bytes));}Send(view,SCI_ENDUNDOACTION);
        if(path.length&&!inMemory&&![self saveDocument:doc path:path error:&error]){++skipped;[report addObject:@{@"path":path,@"status":@"save failed; edited tab retained",@"reason":error.localizedDescription ?: @"Unknown save error"}];continue;}
        ++changed;[report addObject:@{@"path":path,@"title":doc.title,@"status":path.length&&!inMemory?@"saved":@"edited with undo",@"backupPrefix":prefix}];
        if(!wasOpen&&doc!=self.secondaryDocument){[self.tabs removeTabViewItem:doc.tab];[self.documents removeObject:doc];}
    }}
    NSData *manifest=[NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted error:nil];BOOL recorded=NXWrite(manifest,[backup stringByAppendingPathComponent:@"report.json"],&error);
    if([self.documents containsObject:selected])[self activateDocument:selected];if(!self.documents.count)[self newDocument:nil];[self tick:nil];
    return [NSString stringWithFormat:@"%lu documents updated; %lu skipped or failed. Backups %@: %@",(unsigned long)changed,(unsigned long)skipped,recorded?@"and report":@"(report could not be saved)",backup];
}
@end
