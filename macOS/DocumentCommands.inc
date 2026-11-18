// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native document navigation and file commands, added 2026-11-01.
@implementation NXApp (DocumentCommands)
- (void)moveTabFrom:(NSInteger)from to:(NSInteger)to {
    if(from<0||to<0||from>=(NSInteger)self.documents.count||to>=(NSInteger)self.documents.count||from==to)return;
    NXDocument *doc=self.documents[from];BOOL restoring=self.restoring;self.restoring=YES;
    [self.tabs removeTabViewItem:doc.tab];[self.documents removeObjectAtIndex:from];
    [self.documents insertObject:doc atIndex:to];[self.tabs insertTabViewItem:doc.tab atIndex:to];[self activateDocument:doc];
    self.restoring=restoring;[self tick:nil];
}
- (void)chooseOpenDocument:(id)sender {
    NSMutableArray *items=[NSMutableArray new];for(NXDocument *doc in self.documents)[items addObject:[NSString stringWithFormat:@"%@%@%@",doc.title,doc.dirty?@" •":@"",doc.path.length?[@" — " stringByAppendingString:doc.path]:@""]];
    NSInteger i=NXChoose(@"Open Documents",items,self.window);if(i>=0)[self activateDocument:self.documents[i]];
}
- (void)tabCommand:(id)sender {
    NSInteger action=[sender tag],index=[self.documents indexOfObject:self.current];
    if(action==0)[self activateDocument:self.documents.firstObject];
    else if(action==1)[self activateDocument:self.documents.lastObject];
    else if(action==2)[self moveTabFrom:index to:MAX(0,index-1)];
    else if(action==3)[self moveTabFrom:index to:MIN((NSInteger)self.documents.count-1,index+1)];
    else if(action==5)[self moveTabFrom:index to:0];
    else if(action==6)[self moveTabFrom:index to:self.documents.count-1];
    else if(action==4)self.current.pinned=!self.current.pinned;
    else if(action>=10&&action<=15)self.current.tabColor=action-10;
    [self tick:nil];
}
- (NSMenu *)tabMenu:(NSInteger)index {
    [self.tabs selectTabViewItemAtIndex:index];NSMenu *menu=[NSMenu new];
    [self item:menu title:self.current.pinned?@"Unpin Tab":@"Pin Tab" action:@selector(tabCommand:) key:@"" tag:4];
    [self item:menu title:@"Close Tab…" action:@selector(closeTab:) key:@"" tag:0];
    [self item:menu title:@"Close Other Tabs…" action:@selector(closeDocuments:) key:@"" tag:1];
    [self item:menu title:@"Close Tabs to the Left…" action:@selector(closeDocuments:) key:@"" tag:2];
    [self item:menu title:@"Close Tabs to the Right…" action:@selector(closeDocuments:) key:@"" tag:3];
    NSMenu *colors=[self menu:@"Tab Color" parent:menu];NSArray *names=@[@"Default",@"Red",@"Orange",@"Green",@"Blue",@"Purple"];
    for(NSUInteger i=0;i<names.count;++i)[self item:colors title:names[i] action:@selector(tabCommand:) key:@"" tag:10+i];return menu;
}
- (void)closeDocuments:(id)sender {
    NXDocument *original=self.current;NSInteger index=[self.documents indexOfObject:original],action=[sender tag];NSArray *documents=self.documents.copy;
    for(NSUInteger i=0;i<documents.count;++i){NXDocument *doc=documents[i];if(doc.pinned&&action!=5)continue;
        BOOL close=action==5||action==0||(action==1&&doc!=original)||(action==2&&(NSInteger)i<index)||(action==3&&(NSInteger)i>index)||(action==4&&!doc.dirty);
        if(close){[self activateDocument:doc];[self closeTab:nil];if([self.documents containsObject:doc])break;}
    }
    if([self.documents containsObject:original])[self activateDocument:original];
}
- (void)recentDocument:(id)sender {
    NSArray *paths=[sender tag]?self.closedPaths:NSDocumentController.sharedDocumentController.recentDocumentURLs;
    NSMutableArray *names=[NSMutableArray new];for(id value in paths)[names addObject:[value isKindOfClass:NSURL.class]?[value path]:value];
    if(!names.count){NSBeep();return;}NSInteger index=[sender tag]?names.count-1:NXChoose(@"Recent Documents",names,self.window);
    if(index>=0){NSError *error=nil;if(![self openPath:names[index] error:&error])Alert(error.localizedDescription);else if([sender tag])[self.closedPaths removeObjectAtIndex:index];}
}
- (void)fileCommand:(id)sender {
    NXDocument *doc=self.current;NSInteger action=[sender tag];NSError *error=nil;
    if(action==0){ // Save a copy without changing the current document's identity or dirty state.
        NSSavePanel *panel=[NSSavePanel savePanel];panel.nameFieldStringValue=doc.title;if([panel runModal]!=NSModalResponseOK)return;
        NSString *path=NXCanonicalPath(panel.URL.path);if([path isEqual:doc.path]){Alert(@"Choose a different path for the copy.");return;}
        NSString *sidecar=[path stringByAppendingString:@".notex"];
        if([NSFileManager.defaultManager fileExistsAtPath:sidecar]&&!NXOwnedCompanion([NSData dataWithContentsOfFile:sidecar])){Alert(@"An unrelated file occupies the formatting companion path.");return;}
        NSData *data=NXEncodeText(doc.editor.string,doc.encoding);if(!data){Alert(@"The text cannot be represented in the selected encoding.");return;}
        NSMutableData *encoded=[NSMutableData new];if(doc.bom){if(doc.encoding==NSUTF8StringEncoding)[encoded appendBytes:"\xef\xbb\xbf" length:3];else if(doc.encoding==NSUTF16LittleEndianStringEncoding)[encoded appendBytes:"\xff\xfe" length:2];else [encoded appendBytes:"\xfe\xff" length:2];}[encoded appendData:data];
        if(!NXWrite(encoded,path,&error)||!NXWrite(NXCompanion(Text(doc.editor),State(doc.editor)),sidecar,&error))Alert(error.localizedDescription);return;
    }
    if(!doc.path.length){Alert(@"Save this document to a file first.");return;}
    NSURL *url=[NSURL fileURLWithPath:doc.path];
    if(action==1){
        NSSavePanel *panel=[NSSavePanel savePanel];panel.nameFieldStringValue=doc.title;panel.directoryURL=url.URLByDeletingLastPathComponent;if([panel runModal]!=NSModalResponseOK)return;
        NSString *destination=NXCanonicalPath(panel.URL.path),*oldCompanion=[doc.path stringByAppendingString:@".notex"],*newCompanion=[destination stringByAppendingString:@".notex"];
        if([destination isEqual:doc.path])return;
        if([NSFileManager.defaultManager fileExistsAtPath:destination]||[NSFileManager.defaultManager fileExistsAtPath:newCompanion]){Alert(@"Choose an unused filename. Rename does not replace existing documents or companions.");return;}
        if(![self snapshot:&error]){Alert(error.localizedDescription);return;}
        BOOL companion=[NSFileManager.defaultManager fileExistsAtPath:oldCompanion];
        if(companion&&!NXOwnedCompanion([NSData dataWithContentsOfFile:oldCompanion])){Alert(@"The companion is not a NoteX file. Rename was canceled.");return;}
        if(![NSFileManager.defaultManager moveItemAtPath:doc.path toPath:destination error:&error]){Alert(error.localizedDescription);return;}
        if(companion&&![NSFileManager.defaultManager moveItemAtPath:oldCompanion toPath:newCompanion error:&error]){NSError *rollback=nil;[NSFileManager.defaultManager moveItemAtPath:destination toPath:doc.path error:&rollback];Alert(rollback?[NSString stringWithFormat:@"Rename failed. The document is at %@; its companion is at %@.",destination,oldCompanion]:error.localizedDescription);if(rollback){doc.path=destination;doc.title=destination.lastPathComponent;}return;}
        doc.path=destination;doc.title=destination.lastPathComponent;[self tick:nil];
    } else if(action==2)[NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:@[url]];
    else if(action==3)[NSWorkspace.sharedWorkspace openURL:url];
    else if(action==4){NSURL *terminal=[NSWorkspace.sharedWorkspace URLForApplicationWithBundleIdentifier:@"com.apple.Terminal"];if(terminal)[NSWorkspace.sharedWorkspace openURLs:@[url.URLByDeletingLastPathComponent] withApplicationAtURL:terminal configuration:NSWorkspaceOpenConfiguration.configuration completionHandler:nil];}
    else if(action>=5&&action<=9){NSMutableArray *values=[NSMutableArray new];if(action<=7)[values addObject:action==5?doc.path:action==6?doc.title:doc.path.stringByDeletingLastPathComponent];else for(NXDocument *item in self.documents)[values addObject:action==8?item.title:item.path];[NSPasteboard.generalPasteboard clearContents];[NSPasteboard.generalPasteboard setString:[values componentsJoinedByString:@"\n"] forType:NSPasteboardTypeString];}
}
- (void)readOnly:(id)sender {BOOL readOnly=[sender tag]?[sender tag]==1:!Send(self.current.editor,SCI_GETREADONLY);for(NXDocument *doc in ([sender tag]?self.documents:@[self.current]))Send(doc.primaryEditor,SCI_SETREADONLY,readOnly);if(self.secondaryVisible)Send(self.secondaryEditor,SCI_SETREADONLY,Send(self.secondaryDocument.primaryEditor,SCI_GETREADONLY));[self updateControls];}
@end
