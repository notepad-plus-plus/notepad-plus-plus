// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native column sequences, folding, and application integrations, added 2026-11-01.
@implementation NXApp (ExtraCommands)
- (BOOL)pairedInput:(ScintillaView *)view key:(NSString *)key {
    if(![self.preferences[@"autoPairs"] boolValue]||key.length!=1||[key characterAtIndex:0]>127||[key characterAtIndex:0]==0||Send(view,SCI_GETREADONLY)||Send(view,SCI_GETSELECTIONS)!=1||Send(view,SCI_AUTOCACTIVE))return NO;
    NSString *close=@{@"(":@")",@"[":@"]",@"{":@"}",@"\"":@"\"",@"'":@"'"}[key];auto a=Send(view,SCI_GETSELECTIONSTART),b=Send(view,SCI_GETSELECTIONEND);char typed=[key characterAtIndex:0];
    if(a==b&&strchr(")]}\"'",typed)&&Send(view,SCI_GETCHARAT,a)==typed){Send(view,SCI_GOTOPOS,a+1);return YES;}
    if(!close)return NO;if((typed=='\"'||typed=='\'')&&a>0){auto previous=Send(view,SCI_GETCHARAT,a-1);if(previous=='\\'||(typed=='\''&&(std::isalnum((unsigned char)previous)||previous>=128)))return NO;}
    NSString *selected=[[NSString alloc]initWithData:[Text(view) subdataWithRange:NSMakeRange(a,b-a)] encoding:NSUTF8StringEncoding];NXReplaceRange(view,a,b,[NSString stringWithFormat:@"%@%@%@",key,selected,close]);Send(view,SCI_SETSEL,a+1,b+1);return YES;
}

- (void)numberColumns:(id)sender {
    NSAlert *alert=[NSAlert new];alert.messageText=@"Insert Number Sequence";NSView *form=[[NSView alloc]initWithFrame:NSMakeRect(0,0,340,172)];NSMutableArray *fields=[NSMutableArray new];
    NSArray *labels=@[@"Initial value",@"Increment",@"Repeat each value",@"Minimum digits"],*defaults=@[@"1",@"1",@"1",@"0"];
    for(NSUInteger i=0;i<labels.count;++i){NSTextField *label=[NSTextField labelWithString:labels[i]];label.frame=NSMakeRect(0,142-i*32,180,24);[form addSubview:label];NSTextField *field=[[NSTextField alloc]initWithFrame:NSMakeRect(192,142-i*32,148,24)];field.stringValue=defaults[i];[form addSubview:field];[fields addObject:field];}
    NSPopUpButton *base=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(192,0,148,28) pullsDown:NO];[base addItemsWithTitles:@[@"Decimal",@"Hexadecimal",@"Octal",@"Binary"]];[form addSubview:base];alert.accessoryView=form;[alert addButtonWithTitle:@"Insert"];[alert addButtonWithTitle:@"Cancel"];if([alert runModal]!=NSAlertFirstButtonReturn)return;
    long long values[4]{};for(int i=0;i<4;++i){NSScanner *scanner=[NSScanner scannerWithString:[fields[i] stringValue]];if(![scanner scanLongLong:&values[i]]||!scanner.isAtEnd){Alert(@"Enter whole numbers in every field.");return;}}
    ScintillaView *view=self.current.editor;auto start=Send(view,SCI_GETSELECTIONSTART),end=Send(view,SCI_GETSELECTIONEND),column=Send(view,SCI_GETCOLUMN,start),first=Send(view,SCI_LINEFROMPOSITION,start),last=Send(view,SCI_LINEFROMPOSITION,end);if(last>first&&end==Send(view,SCI_POSITIONFROMLINE,last))--last;
    std::vector<std::string> numbers;unsigned bases[]={10,16,8,2};if(values[2]<1||values[2]>1000000||values[3]<0||values[3]>64||!NXColumnNumbers(values[0],values[1],values[2],bases[base.indexOfSelectedItem],values[3],last-first+1,numbers)){Alert(@"The sequence exceeds the supported numeric or line limits. No text was changed.");return;}
    Send(view,SCI_BEGINUNDOACTION);for(auto line=last;line>=first;--line){auto pos=Send(view,SCI_FINDCOLUMN,line,column),actual=Send(view,SCI_GETCOLUMN,pos);NSString *padding=[@"" stringByPaddingToLength:MAX(0,column-actual) withString:@" " startingAtIndex:0];NXReplaceRange(view,pos,pos,[padding stringByAppendingString:@(numbers[line-first].c_str())]);}Send(view,SCI_ENDUNDOACTION);
}
- (void)foldSpecific:(id)sender {ScintillaView *view=self.current.editor;NSInteger tag=[sender tag],depth=tag%10;for(sptr_t line=0;line<Send(view,SCI_GETLINECOUNT);++line){auto level=Send(view,SCI_GETFOLDLEVEL,line);if((level&SC_FOLDLEVELHEADERFLAG)&&((level&SC_FOLDLEVELNUMBERMASK)-SC_FOLDLEVELBASE)==depth-1)Send(view,SCI_FOLDLINE,line,tag>=10?SC_FOLDACTION_EXPAND:SC_FOLDACTION_CONTRACT);}}
- (void)openBrowser:(id)sender {
    if(!self.current.path.length){Alert(@"Save this document before opening it in a browser.");return;}NSArray *identifiers=@[@"com.apple.Safari",@"com.google.Chrome",@"org.mozilla.firefox",@"com.microsoft.edgemac"];NSURL *app=[NSWorkspace.sharedWorkspace URLForApplicationWithBundleIdentifier:identifiers[[sender tag]]];if(!app){Alert(@"This browser is not installed.");return;}
    [NSWorkspace.sharedWorkspace openURLs:@[[NSURL fileURLWithPath:self.current.path]] withApplicationAtURL:app configuration:NSWorkspaceOpenConfiguration.configuration completionHandler:^(NSRunningApplication *application,NSError *error){if(error)dispatch_async(dispatch_get_main_queue(),^{Alert(error.localizedDescription);});}];
}
- (void)folderWorkspace:(id)sender {
    NSString *path=self.current.path.stringByDeletingLastPathComponent;if(![sender tag]||!path.length){NSOpenPanel *panel=[NSOpenPanel openPanel];panel.canChooseDirectories=YES;panel.canChooseFiles=NO;if([panel runModal]!=NSModalResponseOK)return;path=panel.URL.path;}
    [self.workspace addFolderRoot:path];self.sidebarVisible=YES;[self.workspace showPanel:4];[self applySidebarVisibility];[self tick:nil];
}
- (void)buildExtraMenus {
    NSMenu *file=[NSApp.mainMenu itemWithTitle:@"File"].submenu,*edit=[NSApp.mainMenu itemWithTitle:@"Edit"].submenu,*view=[NSApp.mainMenu itemWithTitle:@"View"].submenu;
    [self item:file title:@"Close All Tabs…" action:@selector(closeDocuments:) key:@"" tag:5];[self item:file title:@"Print Now" action:@selector(printDocument:) key:@"" tag:1];
    [self item:file title:@"Open Folder as Workspace…" action:@selector(folderWorkspace:) key:@"" tag:0];[self item:file title:@"Open Containing Folder as Workspace" action:@selector(folderWorkspace:) key:@"" tag:1];
    [self item:edit title:@"Number Column Editor…" action:@selector(numberColumns:) key:@"" tag:0];[self item:edit title:@"Insert Short Date and Time" action:@selector(insertDate:) key:@"" tag:2];[self item:edit title:@"Insert Long Date and Time" action:@selector(insertDate:) key:@"" tag:3];
    [self item:view title:@"Move Tab to Start" action:@selector(tabCommand:) key:@"" tag:5];[self item:view title:@"Move Tab to End" action:@selector(tabCommand:) key:@"" tag:6];
    for(int expand=0;expand<2;++expand){NSMenu *fold=[self menu:expand?@"Unfold Level":@"Collapse Level" parent:view];for(int i=1;i<=8;++i)[self item:fold title:[NSString stringWithFormat:@"Level %d",i] action:@selector(foldSpecific:) key:@"" tag:expand*10+i];}
    NSMenu *browser=[self menu:@"View in Browser" parent:view];NSArray *names=@[@"Safari",@"Chrome",@"Firefox",@"Edge"];for(NSUInteger i=0;i<names.count;++i)[self item:browser title:names[i] action:@selector(openBrowser:) key:@"" tag:i];
}
@end
