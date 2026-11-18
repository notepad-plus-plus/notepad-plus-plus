// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Behavioral tests use isolated profiles and synthetic documents only.
static void CancelNextDialog(void) {
    NSTimer *timer=[NSTimer timerWithTimeInterval:0.15 repeats:NO block:^(NSTimer *timer){for(NSView *view in NSApp.modalWindow.contentView.subviews)if([view isKindOfClass:NSButton.class]&&[[(NSButton *)view title] isEqual:@"Cancel"]){[(NSButton *)view performClick:nil];return;}}];
    [NSRunLoop.mainRunLoop addTimer:timer forMode:NSModalPanelRunLoopMode];
}
@implementation NXApp (WorkbenchTests)
- (void)seedWorkbench {
    NSMutableDictionary *language=[NXDefaultLanguage() mutableCopy];language[@"id"]=@"test-language";language[@"name"]=@"Test Language";language[@"extensions"]=@"nxtest";language[@"keywords1"]=@"NoteX begin end café";
    NSMutableDictionary *theme=[NXDefaultTheme() mutableCopy];theme[@"id"]=@"test-theme";theme[@"name"]=@"Test Theme";theme[@"keyword1"]=@"FF4040";
    self.configuration=[@{@"languages":@[language],@"themes":@[theme],@"selectedTheme":@"test-theme"} mutableCopy];self.current.customLanguage=@"test-language";
    self.preferences[@"caretWidth"]=@2;
    NSError *error=nil;Check([self.menuLayout setShortcuts:@{@"/menu:Edit/showFind::0":@{@"key":@"f",@"modifiers":@(NSEventModifierFlagControl|NSEventModifierFlagOption)}} error:&error],"persist customized shortcut");
    self.sidebarVisible=NO;[self applySidebarVisibility];
    [self refreshEditorSettings];[self cloneView:TestCommand(2)];Send(self.secondaryEditor,SCI_SETZOOM,2);
}
- (void)verifyWorkbenchRestore {
    Check(!self.sidebarVisible&&self.workspace.view.superview==nil,"restore hidden sidebar without reserved width");
    Check(self.secondaryVisible&&!self.editorSplit.vertical,"restore horizontal split view");
    Check(Send(self.secondaryEditor,SCI_GETDOCPOINTER)==Send(self.secondaryDocument.primaryEditor,SCI_GETDOCPOINTER),"restored clone shares the original document");
    Check(Send(self.secondaryEditor,SCI_GETZOOM)==2,"restore independent clone zoom");
    Check([self.configuration[@"selectedTheme"] isEqual:@"test-theme"]&&[self.configuration[@"languages"] count]==1,"restore language and theme definitions");
    Check([self.current.customLanguage isEqual:@"test-language"]&&Send(self.current.editor,SCI_GETCARETWIDTH)==2,"restore language assignment and preferences");
    NSMenuItem *find=FindMenuCommand(NSApp.mainMenu,@selector(showFind:),0);
    Check(find.keyEquivalentModifierMask==(NSEventModifierFlagControl|NSEventModifierFlagOption),"restore shortcut assignment");
}
- (void)workbenchTests {
    self.sidebarVisible=NO;[self applySidebarVisibility];
    NSMenuItem *panelItem=FindMenuCommand(NSApp.mainMenu,@selector(showPanel:),0);
    [NSApp sendAction:panelItem.action to:panelItem.target from:panelItem];[self validateMenuItem:panelItem];
    Check(self.sidebarVisible&&self.workspace.view.superview==self.split&&panelItem.state==NSControlStateValueOn,"Function List menu opens and checks the sidebar");
    [NSApp sendAction:panelItem.action to:panelItem.target from:panelItem];[self validateMenuItem:panelItem];
    Check(!self.sidebarVisible&&self.workspace.view.superview==nil&&panelItem.state==NSControlStateValueOff,"Function List menu hides sidebar and clears its checkmark");
    Check(self.editorSplit.frame.origin.x<1&&fabs(self.editorSplit.frame.size.width-self.split.bounds.size.width)<1,"hidden sidebar releases its full width to the editor");
    [self showPanel:TestCommand(1)];[self showPanel:TestCommand(0)];
    Check(self.sidebarVisible&&self.workspace.activePanel==0,"switching to another panel keeps sidebar visible");
    [self showPanel:TestCommand(0)];
    NXDocument *listDoc=[self newDocument:nil];SetText(listDoc.primaryEditor,[@"first\nsecond\nkeep\n" dataUsingEncoding:NSUTF8StringEncoding]);Send(listDoc.primaryEditor,SCI_SETSEL,0,13);
    [self.bulletListButton performClick:nil];
    Check([listDoc.primaryEditor.string isEqual:@"• first\n• second\nkeep\n"],"toolbar bullet button changes only selected lines");
    Send(listDoc.primaryEditor,SCI_UNDO);Check([listDoc.primaryEditor.string isEqual:@"first\nsecond\nkeep\n"],"toolbar bullet insertion undoes in one action");
    Send(listDoc.primaryEditor,SCI_SETSEL,0,13);[self.numberedListButton performClick:nil];
    Check([listDoc.primaryEditor.string isEqual:@"1. first\n2. second\nkeep\n"],"toolbar numbered button numbers selected lines");
    Send(listDoc.primaryEditor,SCI_UNDO);[self cloneView:TestCommand(1)];Send(self.secondaryEditor,SCI_SETSEL,0,5);[self.bulletListButton performClick:nil];
    Check([listDoc.primaryEditor.string hasPrefix:@"• first"]&&self.current.editor==self.secondaryEditor,"list toolbar commands edit the focused clone");[self closeSecondary:nil];
    self.preferences[@"lineNumbers"]=@YES;[self refreshEditorSettings];SetText(listDoc.primaryEditor,[@"one" dataUsingEncoding:NSUTF8StringEncoding]);[self updateControls];
    auto narrow=Send(listDoc.primaryEditor,SCI_GETMARGINWIDTHN,0);Check(narrow>0&&narrow<56,"short document uses a compact line-number gutter");
    SetText(listDoc.primaryEditor,[[[@"\n" stringByPaddingToLength:1000 withString:@"\n" startingAtIndex:0] stringByAppendingString:@"last"] dataUsingEncoding:NSUTF8StringEncoding]);[self updateControls];
    Check(Send(listDoc.primaryEditor,SCI_GETMARGINWIDTHN,0)>narrow,"line-number gutter expands for four-digit line counts");
    SetText(listDoc.primaryEditor,NSData.data);[self updateControls];Check(Send(listDoc.primaryEditor,SCI_GETMARGINWIDTHN,0)==narrow,"line-number gutter shrinks when line count falls");
    NSError *error=nil;NXDocument *doc=[self newDocument:nil];SetText(doc.primaryEditor,[@"alpha café\nsecond\n" dataUsingEncoding:NSUTF8StringEncoding]);Bold24(doc.primaryEditor);
    Send(doc.primaryEditor,SCI_GOTOPOS,0);[self cloneView:TestCommand(1)];
    Check(self.secondaryVisible&&self.editorSplit.vertical&&Send(self.secondaryEditor,SCI_GETDOCPOINTER)==Send(doc.primaryEditor,SCI_GETDOCPOINTER),"split clone shares Scintilla document ownership");
    Send(self.secondaryEditor,SCI_GOTOPOS,Send(self.secondaryEditor,SCI_GETLENGTH));Send(self.secondaryEditor,SCI_REPLACESEL,0,reinterpret_cast<sptr_t>("tail"));
    Check([doc.primaryEditor.string hasSuffix:@"tail"]&&Send(doc.primaryEditor,SCI_GETCURRENTPOS)==0,"clone edits update primary text without moving its caret");
    Send(doc.primaryEditor,SCI_UNDO);Check(![self.secondaryEditor.string hasSuffix:@"tail"],"undo from primary updates clone");
    Check(State(self.secondaryEditor).runs.size()==State(doc.primaryEditor).runs.size(),"clone shares rich formatting");
    [self cloneView:TestCommand(2)];Check(!self.editorSplit.vertical,"switch to horizontal split");
    NXDocument *other=[self newDocument:nil];SetText(other.primaryEditor,[@"different document" dataUsingEncoding:NSUTF8StringEncoding]);[self bindSecondary:doc];
    Send(self.secondaryEditor,SCI_GOTOPOS,0);Send(self.secondaryEditor,SCI_REPLACESEL,0,reinterpret_cast<sptr_t>("X"));
    Check([other.primaryEditor.string isEqual:@"different document"]&&[doc.primaryEditor.string hasPrefix:@"X"],"two panes can display independent documents");
    [self closeSecondary:nil];
    Check([self.documents containsObject:doc]&&[doc.primaryEditor.string hasPrefix:@"X"],"closing clone retains source and edits");
    [self.tabs selectTabViewItem:doc.tab];
    NSMutableDictionary *language=[NXDefaultLanguage() mutableCopy];language[@"name"]=@"Fixture Language";language[@"extensions"]=@"fixture";language[@"keywords1"]=@"begin café end";
    NSMutableDictionary *theme=[NXDefaultTheme() mutableCopy];theme[@"name"]=@"Fixture Theme";theme[@"keyword1"]=@"FF0000";
    self.configuration=[@{@"languages":@[language],@"themes":@[theme],@"selectedTheme":theme[@"id"]} mutableCopy];doc.customLanguage=language[@"id"];
    SetText(doc.primaryEditor,[@"begin café {\n// begin\n\"begin\" 123\n}\n" dataUsingEncoding:NSUTF8StringEncoding]);[self refreshEditorSettings];
    Check(Send(doc.primaryEditor,SCI_GETSTYLEAT,0)==1&&Send(doc.primaryEditor,SCI_GETSTYLEAT,6)==1,"user language highlights ASCII and Unicode keywords");
    Check(Send(doc.primaryEditor,SCI_GETSTYLEAT,14)==10,"user language distinguishes line comments");
    Check(Send(doc.primaryEditor,SCI_STYLEGETFORE,1)==255,"custom theme applies keyword color");
    Check(Send(doc.primaryEditor,SCI_GETFOLDLEVEL,0)&SC_FOLDLEVELHEADERFLAG,"user language creates fold levels");
    Check([NXLanguageForPath(self.configuration,@"test.fixture")[@"id"] isEqual:language[@"id"]],"custom extension selects user language");
    NSData *exported=[NSJSONSerialization dataWithJSONObject:self.configuration options:0 error:&error];Check(NXValidateConfiguration([NSJSONSerialization JSONObjectWithData:exported options:0 error:&error]),"language and theme settings survive JSON round trip");
    NSMutableDictionary *broken=[self.configuration mutableCopy];broken[@"selectedTheme"]=@"missing";Check(!NXValidateConfiguration(broken),"reject unknown active theme");
    NSDictionary *before=self.configuration;CancelNextDialog();[self languageEditor:nil];Check([self.configuration isEqual:before],"cancel language editor preserves configuration");
    CancelNextDialog();[self preferencesEditor:nil];Check([self.preferences[@"font"] length]>0,"native preferences dialog cancels without losing settings");
    error=nil;NSDictionary *previous=self.menuLayout.settings;
    Check(![self.menuLayout setShortcuts:@{@"/menu:Edit/showFind::0":@{@"key":@"q",@"modifiers":@(NSEventModifierFlagCommand)}} error:&error]&&[self.menuLayout.settings isEqual:previous],"shortcut collision leaves saved layout unchanged");
    error=nil;Check([self.menuLayout setShortcuts:@{@"/menu:Edit/showFind::0":@{@"key":@"f",@"modifiers":@(NSEventModifierFlagControl|NSEventModifierFlagOption)}} error:&error],"assign alternate native shortcut");
    self.findBar.hidden=YES;NSEvent *event=[NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:NSEventModifierFlagControl|NSEventModifierFlagOption timestamp:0 windowNumber:self.window.windowNumber context:nil characters:@"f" charactersIgnoringModifiers:@"f" isARepeat:NO keyCode:3];
    Check([NSApp.mainMenu performKeyEquivalent:event]&&!self.findBar.hidden,"custom keyboard shortcut invokes the real command");
    NSDictionary *shortcutLayout=self.menuLayout.settings;CancelNextDialog();[self shortcutEditor:nil];Check([self.menuLayout.settings isEqual:shortcutLayout],"cancel shortcut editor preserves assignments");
    self.preferences[@"showEOL"]=@YES;self.preferences[@"caretWidth"]=@3;self.preferences[@"edgeColumn"]=@80;[self refreshEditorSettings];
    Check(Send(doc.primaryEditor,SCI_GETVIEWEOL)&&Send(doc.primaryEditor,SCI_GETCARETWIDTH)==3&&Send(doc.primaryEditor,SCI_GETEDGECOLUMN)==80,"preferences affect actual editor behavior");
    self.preferences[@"fontSize"]=@13.5;[self refreshEditorSettings];Check(Send(doc.primaryEditor,SCI_STYLEGETSIZEFRACTIONAL,STYLE_DEFAULT)==1350,"preferences preserve fractional font sizes");
    NSDictionary *options=@{@"query":@"café (\\d+)",@"replacement":@"note-$1",@"regex":@YES,@"matchCase":@NO,@"wholeWord":@NO,@"recursive":@YES,@"hidden":@NO,@"include":@"*.txt",@"exclude":@"skip*"};
    NSString *folder=[self.storage.directory stringByAppendingPathComponent:@"search-fixtures"],*file=[folder stringByAppendingPathComponent:@"sample.txt"];
    Check([NSFileManager.defaultManager createDirectoryAtPath:folder withIntermediateDirectories:YES attributes:nil error:&error],"create isolated search fixture directory");
    NXDocument *saved=[self newDocument:nil];NSData *original=[@"日本語 café 12\nkeep café 34\n" dataUsingEncoding:NSUTF8StringEncoding];SetText(saved.primaryEditor,original);Bold24(saved.primaryEditor);Check([self saveDocument:saved path:file error:&error],"save formatted search fixture");
    NSArray *results=NXSearchFiles(@[folder],options,nil,&error);Check(results.count==2&&[results[0][@"position"] unsignedIntegerValue]==10&&[results[0][@"replacement"] isEqual:@"note-12"],"file search preserves UTF-8 offsets and regex captures");
    NSString *report=[self applySearchResults:results];
    if(![report hasPrefix:@"1 documents updated"]){fprintf(stderr,"Replacement report: %s\n",report.UTF8String);for(NSString *name in [NSFileManager.defaultManager subpathsAtPath:[self.storage.directory stringByAppendingPathComponent:@"ReplaceBackups"]])if([name hasSuffix:@"report.json"])fprintf(stderr,"%s\n",[[NSString alloc]initWithContentsOfFile:[[self.storage.directory stringByAppendingPathComponent:@"ReplaceBackups"] stringByAppendingPathComponent:name] encoding:NSUTF8StringEncoding error:nil].UTF8String);}
    Check([report hasPrefix:@"1 documents updated"]&&[saved.primaryEditor.string containsString:@"note-34"],"previewed file replacements use normal save path");
    NoteX::State state;NSData *newBytes=[NSData dataWithContentsOfFile:file];Check(NXReadCompanion([NSData dataWithContentsOfFile:[file stringByAppendingString:@".notex"]],newBytes,state)&&!state.runs.empty(),"replacement updates matching formatting companion");
    NSArray *backups=[NSFileManager.defaultManager subpathsAtPath:[self.storage.directory stringByAppendingPathComponent:@"ReplaceBackups"]];BOOL foundBackup=NO;
    for(NSString *path in backups)if([path hasSuffix:@"sample.txt"]&&[[NSData dataWithContentsOfFile:[[self.storage.directory stringByAppendingPathComponent:@"ReplaceBackups"] stringByAppendingPathComponent:path]] isEqual:original])foundBackup=YES;
    Check(foundBackup,"replace-in-files retains recoverable original bytes");
    BOOL foundMap=NO;for(NSString *path in backups)if([path hasSuffix:@"recovery.json"]){NSArray *map=[NSJSONSerialization JSONObjectWithData:[NSData dataWithContentsOfFile:[[self.storage.directory stringByAppendingPathComponent:@"ReplaceBackups"] stringByAppendingPathComponent:path]] options:0 error:nil];for(NSDictionary *entry in map)if([NXCanonicalPath(entry[@"path"]) isEqual:NXCanonicalPath(file)]&&[entry[@"hasCompanion"] boolValue])foundMap=YES;}
    Check(foundMap,"replacement recovery map retains original path and companion association");
    NSMutableDictionary *literal=[options mutableCopy];literal[@"query"]=@"note";literal[@"regex"]=@NO;literal[@"replacement"]=@"changed";results=NXSearchFiles(@[file],literal,nil,&error);
    NSData *external=[@"external edit" dataUsingEncoding:NSUTF8StringEncoding];Check(NXWrite(external,file,&error),"simulate external change after preview");report=[self applySearchResults:results];Check([report hasPrefix:@"0 documents updated"]&&[[NSData dataWithContentsOfFile:file] isEqual:external],"replace preview refuses externally changed file");
    NSData *dirtyBefore=Text(saved.primaryEditor);Send(saved.primaryEditor,SCI_APPENDTEXT,5,reinterpret_cast<sptr_t>(" note"));
    results=NXSearchFiles(@[@{@"id":saved.identifier,@"path":saved.path,@"title":saved.title,@"text":Text(saved.primaryEditor)}],literal,nil,&error);
    report=[self applySearchResults:results];Check([report hasPrefix:@"1 documents updated"]&&[saved.primaryEditor.string hasSuffix:@" changed"]&&[[NSData dataWithContentsOfFile:file] isEqual:external],"Open Documents replacement edits dirty buffers without overwriting disk");
    Send(saved.primaryEditor,SCI_UNDO);Check([saved.primaryEditor.string hasSuffix:@" note"],"undo buffer replacement preserves earlier unsaved edit");
    SetText(saved.primaryEditor,dirtyBefore);
    NXDocument *untitled=[self newDocument:nil];SetText(untitled.primaryEditor,[@"note note" dataUsingEncoding:NSUTF8StringEncoding]);results=NXSearchFiles(@[@{@"id":untitled.identifier,@"path":@"",@"title":@"draft.txt",@"text":Text(untitled.primaryEditor)}],literal,nil,&error);
    report=[self applySearchResults:results];Check([untitled.primaryEditor.string isEqual:@"changed changed"]&&untitled.path.length==0,"search and replace unsaved tabs without creating disk files");Send(untitled.primaryEditor,SCI_UNDO);Check([untitled.primaryEditor.string isEqual:@"note note"],"undo all previewed changes in a tab together");
    NSMutableData *utf16=[NSMutableData dataWithBytes:"\xff\xfe" length:2];[utf16 appendData:[@"café 77" dataUsingEncoding:NSUTF16LittleEndianStringEncoding]];NSString *utf16File=[folder stringByAppendingPathComponent:@"utf16.txt"];Check(NXWrite(utf16,utf16File,&error),"create UTF-16 search fixture");
    Check([NSFileManager.defaultManager setAttributes:@{NSFilePosixPermissions:@0640} ofItemAtPath:utf16File error:&error],"set fixture permissions before replacement");
    results=NXSearchFiles(@[utf16File],options,nil,&error);Check(results.count==1,"search BOM-marked UTF-16 files");report=[self applySearchResults:results];NSData *converted=[NSData dataWithContentsOfFile:utf16File];Check(converted.length>=2&&!memcmp(converted.bytes,"\xff\xfe",2)&&[NXDecodeText(converted) isEqual:@"note-77"],"replacement preserves UTF-16 encoding and BOM");
    Check([[[NSFileManager.defaultManager attributesOfItemAtPath:utf16File error:&error] objectForKey:NSFilePosixPermissions] unsignedIntegerValue]==0640,"atomic replacement preserves existing permission bits");
    Check([NXSearchFiles(@[folder],options,^BOOL{return YES;},&error) count]==0,"cancel file enumeration without changes");
    literal[@"regex"]=@YES;literal[@"query"]=@"(";error=nil;Check(!NXSearchMatches(@"text",literal,&error)&&error,"invalid regular expression is reported");
    // Earlier fixtures are synthetic and complete; isolate the two dirty Save All targets.
    for(NXDocument *existing in self.documents){Send(existing.primaryEditor,SCI_SETSAVEPOINT);existing.restoredDirty=NO;}
    NXDocument *firstSave=[self newDocument:nil];SetText(firstSave.primaryEditor,[@"first" dataUsingEncoding:NSUTF8StringEncoding]);NSString *firstPath=[folder stringByAppendingPathComponent:@"save-all-first.txt"];
    Check([self saveDocument:firstSave path:firstPath error:&error],"save first split-view Save All fixture");
    NXDocument *secondSave=[self newDocument:nil];SetText(secondSave.primaryEditor,[@"second" dataUsingEncoding:NSUTF8StringEncoding]);NSString *secondPath=[folder stringByAppendingPathComponent:@"save-all-second.txt"];
    Check([self saveDocument:secondSave path:secondPath error:&error],"save second split-view Save All fixture");
    [self cloneView:TestCommand(1)];[self.tabs selectTabViewItem:firstSave.tab];self.secondaryFocused=YES;secondSave.activeEditor=self.secondaryEditor;
    Send(firstSave.primaryEditor,SCI_APPENDTEXT,1,reinterpret_cast<sptr_t>("1"));Send(self.secondaryEditor,SCI_APPENDTEXT,1,reinterpret_cast<sptr_t>("2"));[self saveAll:nil];
    Check([NXDecodeText([NSData dataWithContentsOfFile:firstPath]) isEqual:@"first1"]&&[NXDecodeText([NSData dataWithContentsOfFile:secondPath]) isEqual:@"second2"],"Save All saves both documents when the second pane has focus");
    Check(self.secondaryFocused&&self.current==secondSave&&self.tabs.selectedTabViewItem==firstSave.tab,"Save All restores both pane selections and focus");[self closeSecondary:nil];
    Check([self snapshot:&error],"persist complete workbench state");
}
- (void)workbenchUI:(NSString *)mode {
    NXDocument *doc=self.current;SetText(doc.primaryEditor,[@"NoteX Workbench\n\nbegin café {\n    // Highlight a user-defined language\n    print(\"Hello, NoteX\");\n}\n" dataUsingEncoding:NSUTF8StringEncoding]);[self seedWorkbench];
    if(![mode isEqual:@"ui-split"])[self closeSecondary:nil];
    if([mode isEqual:@"ui-sidebar-hidden"]){SetText(doc.primaryEditor,NSData.data);[self updateControls];Check(!self.sidebarVisible&&self.editorSplit.frame.origin.x<1,"hidden-sidebar UI uses full editor width");}
    if([mode isEqual:@"ui-toolbar"]){SetText(doc.primaryEditor,[@"First item\nSecond item\n\nFirst step\nSecond step" dataUsingEncoding:NSUTF8StringEncoding]);Send(doc.primaryEditor,SCI_SETSEL,0,Send(doc.primaryEditor,SCI_POSITIONFROMLINE,2));[self.bulletListButton performClick:nil];Send(doc.primaryEditor,SCI_SETSEL,Send(doc.primaryEditor,SCI_POSITIONFROMLINE,3),Send(doc.primaryEditor,SCI_GETLENGTH));[self.numberedListButton performClick:nil];Send(doc.primaryEditor,SCI_GOTOPOS,0);}
    if([mode isEqual:@"ui-tabs"]){for(NSUInteger i=0;i<30;++i){NXDocument *tab=[self newDocument:nil];tab.title=[NSString stringWithFormat:@"Document %lu.txt",(unsigned long)i+1];}[self.tabs selectTabViewItemAtIndex:3];[self tick:nil];}
    if([mode isEqual:@"ui-document-map"])[self viewCommand:TestCommand(4)];
    if([mode isEqual:@"ui-search"]){[self findInFiles:nil];NSTextField *query=[self.fileSearch valueForKey:@"query"];query.stringValue=@"NoteX";[(NSPopUpButton *)[self.fileSearch valueForKey:@"scope"] selectItemAtIndex:1];[NSApp sendAction:NSSelectorFromString(@"search:") to:self.fileSearch from:nil];}
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,2*NSEC_PER_SEC),dispatch_get_main_queue(),^{
        if([mode isEqual:@"ui-sidebar-hidden"]||[mode isEqual:@"ui-toolbar"]){
            printf("Layout: content=%s stack=%s outer=%s editor=%s tabs=%s status=%s\n",NSStringFromRect(self.window.contentView.bounds).UTF8String,NSStringFromRect(self.split.superview.frame).UTF8String,NSStringFromRect(self.split.frame).UTF8String,NSStringFromRect(self.editorSplit.frame).UTF8String,NSStringFromRect(self.tabs.frame).UTF8String,NSStringFromRect(self.status.frame).UTF8String);fflush(stdout);
            NSString *imagePath=NSProcessInfo.processInfo.environment[@"NOTEX_UI_DIAGNOSTIC"];
            if(imagePath.length){NSBitmapImageRep *bitmap=[self.window.contentView bitmapImageRepForCachingDisplayInRect:self.window.contentView.bounds];[self.window.contentView cacheDisplayInRect:self.window.contentView.bounds toBitmapImageRep:bitmap];[[bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}] writeToFile:imagePath atomically:YES];}
            Check(self.editorSplit.frame.size.height>self.window.contentView.bounds.size.height*0.75,"hidden-sidebar editor fills the available window height");
        }
        if([mode isEqual:@"ui-search"])Check([[self.fileSearch valueForKey:@"results"] count]==2,"native file search displays unsaved document results");
        printf("READY\n");fflush(stdout);
        if([mode isEqual:@"ui-preferences"])[self preferencesEditor:nil];
        else if([mode isEqual:@"ui-shortcuts"])[self shortcutEditor:nil];
        else if([mode isEqual:@"ui-language"])[self languageEditor:nil];
        else if([mode isEqual:@"ui-theme"])[self themeEditor:nil];
    });
}
@end
