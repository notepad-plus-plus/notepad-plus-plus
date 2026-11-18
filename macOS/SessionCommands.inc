// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Explicit native session files and safe file monitoring, added 2026-11-01.
@implementation NXApp (SessionCommands)
- (void)sessionFile:(id)sender {
    NSError *error=nil;
    if(![sender tag]){
        NSSavePanel *panel=[NSSavePanel savePanel];panel.nameFieldStringValue=@"Workspace.notexsession";panel.message=@"This session includes the text of open documents, including unsaved changes.";if([panel runModal]!=NSModalResponseOK)return;
        if(![self snapshot:&error]){Alert(error.localizedDescription);return;}
        NSMutableDictionary *session=[self.lastSnapshot mutableCopy];[session removeObjectForKey:@"macros"];
        NSData *data=[NSPropertyListSerialization dataWithPropertyList:session format:NSPropertyListBinaryFormat_v1_0 options:0 error:&error];if(!data||!NXWrite(data,panel.URL.path,&error))Alert(error.localizedDescription);
    }else {
        NSOpenPanel *panel=[NSOpenPanel openPanel];panel.message=@"Open a native NoteX session. Existing tabs remain open.";if([panel runModal]!=NSModalResponseOK)return;
        NSData *data=[NSData dataWithContentsOfURL:panel.URL options:NSDataReadingMappedIfSafe error:&error];NSDictionary *session=data.length<=256*1024*1024?NXDecodeSession(data):nil;if(!session){Alert(@"This is not a valid native NoteX session, or it exceeds 256 MB.");return;}
        if(self.documents.count+[session[@"documents"] count]>1000){Alert(@"Close some documents before opening this session. The session limit is 1,000 documents.");return;}
        if(![self snapshot:&error]){Alert(error.localizedDescription);return;}
        BOOL wasRestoring=self.restoring;self.restoring=YES;
        for(NSDictionary *record in session[@"documents"]){NXDocument *doc=[self restoreDocumentRecord:record];doc.identifier=NSUUID.UUID.UUIDString;doc.tab.identifier=doc.identifier;doc.secondaryOnly=NO;
            for(NXDocument *other in self.documents)if(other!=doc&&doc.path.length&&[other.path isEqual:doc.path]){doc.path=@"";doc.title=[@"Recovered — " stringByAppendingString:doc.title];doc.restoredDirty=YES;break;}
        }
        self.restoring=wasRestoring;[self tick:nil];
    }
}
- (void)trashDocument:(id)sender {
    NXDocument *doc=self.current;if(!doc.path.length){NSBeep();return;}
    NSAlert *alert=[NSAlert new];alert.messageText=[NSString stringWithFormat:@"Move %@ to Trash?",doc.title];alert.informativeText=@"The open document will remain available as an unsaved tab.";[alert addButtonWithTitle:@"Move to Trash"];[alert addButtonWithTitle:@"Cancel"];if([alert runModal]!=NSAlertFirstButtonReturn)return;
    NSError *error=nil;if(![self snapshot:&error]){Alert(error.localizedDescription);return;}
    NSString *companion=[doc.path stringByAppendingString:@".notex"];
    if(![NSFileManager.defaultManager trashItemAtURL:[NSURL fileURLWithPath:doc.path] resultingItemURL:nil error:&error]){Alert(error.localizedDescription);return;}
    doc.path=@"";doc.diskHash=NSData.data;doc.restoredDirty=YES;doc.monitoring=NO;
    if(NXOwnedCompanion([NSData dataWithContentsOfFile:companion])&&![NSFileManager.defaultManager trashItemAtURL:[NSURL fileURLWithPath:companion] resultingItemURL:nil error:&error])Alert([@"The document was moved to Trash, but its formatting companion could not be moved: " stringByAppendingString:error.localizedDescription]);[self tick:nil];
}
- (void)toggleMonitoring:(id)sender {NXDocument *doc=self.current;if(!doc.path.length||doc.dirty){Alert(@"Save this document before monitoring its file.");return;}doc.monitoring=!doc.monitoring;Send(doc.editor,SCI_SETREADONLY,doc.monitoring);}
- (void)checkMonitoredFiles {
    for(NXDocument *doc in self.documents)if(doc.monitoring){
        NSData *disk=[NSData dataWithContentsOfFile:doc.path options:NSDataReadingMappedIfSafe error:nil];if(!disk||disk.length>16*1024*1024){doc.monitoring=NO;Send(doc.primaryEditor,SCI_SETREADONLY,0);self.status.stringValue=@"File monitoring stopped: the file is missing, unreadable, or larger than 16 MB.";continue;}
        NSData *hash=NXDigest(disk);if([hash isEqual:doc.diskHash])continue;
        NSString *text=NXDecodeText(disk);if(!text)text=NXDecodeEncoding(disk,doc.encoding);if(!text||doc.dirty){doc.monitoring=NO;Send(doc.primaryEditor,SCI_SETREADONLY,0);self.status.stringValue=@"File monitoring stopped to preserve the document.";continue;}
        Send(doc.primaryEditor,SCI_SETREADONLY,0);SetText(doc.primaryEditor,[text dataUsingEncoding:NSUTF8StringEncoding]);doc.diskHash=hash;NoteX::State state;
        if(NXReadCompanion([NSData dataWithContentsOfFile:[doc.path stringByAppendingString:@".notex"]],Text(doc.primaryEditor),state))Restore(doc.primaryEditor,state);
        Send(doc.primaryEditor,SCI_GOTOPOS,Send(doc.primaryEditor,SCI_GETLENGTH));Send(doc.primaryEditor,SCI_SETREADONLY,1);
    }
}
- (void)buildSessionMenus {
    NSMenu *file=[NSApp.mainMenu itemWithTitle:@"File"].submenu;[self item:file title:@"Save Session As…" action:@selector(sessionFile:) key:@"" tag:0];[self item:file title:@"Open Session…" action:@selector(sessionFile:) key:@"" tag:1];[self item:file title:@"Move File to Trash…" action:@selector(trashDocument:) key:@"" tag:0];
    [self item:[NSApp.mainMenu itemWithTitle:@"View"].submenu title:@"Monitor File (Follow Tail)" action:@selector(toggleMonitoring:) key:@"" tag:0];
}
@end
