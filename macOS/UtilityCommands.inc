// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native text and document utilities, added 2026-11-01.
@implementation NXApp (UtilityCommands)
- (void)binaryClipboard:(id)sender {
    ScintillaView *view=self.current.editor;NSInteger action=[sender tag];NSString *type=@"com.berkkivircik.NoteX.binary";
    if(action==2){NSData *data=[NSPasteboard.generalPasteboard dataForType:type];NSString *text=NXDecodeEncoding(data,self.current.encoding);if(!text){Alert(@"The binary clipboard cannot be decoded in this document's encoding. Choose a compatible encoding first.");return;}NXReplaceRange(view,Send(view,SCI_GETSELECTIONSTART),Send(view,SCI_GETSELECTIONEND),text);}
    else {NSData *data=NXEncodeText(view.selectedString,self.current.encoding);if(!data){Alert(@"The selection cannot be represented in this document's encoding.");return;}[NSPasteboard.generalPasteboard clearContents];[NSPasteboard.generalPasteboard setData:data forType:type];if(action==1)Send(view,SCI_CLEAR);}
}
- (void)redactSelection:(id)sender {
    ScintillaView *view=self.current.editor;NSString *text=view.selectedString;if(!text.length)return;NSMutableString *replacement=[NSMutableString new];NSString *symbol=(NSApp.currentEvent.modifierFlags&NSEventModifierFlagShift)?@"●":@"█";
    [text enumerateSubstringsInRange:NSMakeRange(0,text.length) options:NSStringEnumerationByComposedCharacterSequences usingBlock:^(NSString *part,NSRange range,NSRange enclosing,BOOL *stop){[replacement appendString:[part rangeOfCharacterFromSet:NSCharacterSet.newlineCharacterSet].location!=NSNotFound?part:symbol];}];NXReplaceRange(view,Send(view,SCI_GETSELECTIONSTART),Send(view,SCI_GETSELECTIONEND),replacement);
}
- (void)openSelectedPath:(id)sender {
    NSString *path=[self.current.editor.selectedString stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@" \t\r\n\"'"]];if(!path.length){NSBeep();return;}
    if([path hasPrefix:@"file://"])path=[NSURL URLWithString:path].path;path=path.stringByExpandingTildeInPath;if(!path.isAbsolutePath)path=[(self.current.path.length?self.current.path.stringByDeletingLastPathComponent:NSHomeDirectory()) stringByAppendingPathComponent:path];
    if([sender tag])[NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:@[[NSURL fileURLWithPath:path]]];else{NSError *error=nil;if(![self openPath:path error:&error])Alert(error.localizedDescription);}
}
- (void)findCharacterRange:(id)sender {
    NSString *input=NXPrompt(@"Unicode Range (hexadecimal, such as 0020-007F)",@"0080-10FFFF");if(!input)return;unsigned first=0,last=0;char trailing;
    if(sscanf(input.UTF8String,"%x-%x%c",&first,&last,&trailing)!=2||first>last||last>0x10ffff||(first>=0xd800&&first<=0xdfff)||(last>=0xd800&&last<=0xdfff)){Alert(@"Enter a valid Unicode scalar range that excludes D800-DFFF.");return;}
    self.findText.stringValue=[NSString stringWithFormat:@"[\\x{%X}-\\x{%X}]",first,last];self.searchOptions=[@{@"regex":@YES,@"matchCase":@YES,@"selection":@NO,@"engine":@"icu"} mutableCopy];[self searchNavigate:nil];
}
- (void)copyAllStyledText:(id)sender {
    ScintillaView *view=self.current.editor;auto length=Send(view,SCI_GETLENGTH);std::vector<std::pair<sptr_t,sptr_t>> ranges;
    for(int indicator=10;indicator<16;++indicator)for(sptr_t pos=0;pos<length;){auto end=Send(view,SCI_INDICATOREND,indicator,pos);if(end<=pos)break;if(Send(view,SCI_INDICATORVALUEAT,indicator,pos))ranges.emplace_back(pos,end);pos=end;}
    std::sort(ranges.begin(),ranges.end());std::vector<std::pair<sptr_t,sptr_t>> merged;for(auto range:ranges){if(!merged.empty()&&range.first<=merged.back().second)merged.back().second=std::max(merged.back().second,range.second);else merged.push_back(range);}
    NSData *data=Text(view);NSMutableArray *text=[NSMutableArray new];for(auto range:merged)[text addObject:[[NSString alloc]initWithData:[data subdataWithRange:NSMakeRange(range.first,range.second-range.first)] encoding:NSUTF8StringEncoding]];[NSPasteboard.generalPasteboard clearContents];[NSPasteboard.generalPasteboard setString:[text componentsJoinedByString:NXEOL(view)] forType:NSPasteboardTypeString];
}
- (void)windowHistory:(id)sender {
    NSMutableArray *documents=[NSMutableArray new],*names=[NSMutableArray new];for(NSString *identifier in self.recentTabIDs)for(NXDocument *doc in self.documents)if([doc.identifier isEqual:identifier]){[documents addObject:doc];[names addObject:doc.title];}
    NSInteger selected=NXChoose(@"Recent Documents in This Window",names,self.window);if(selected>=0)[self activateDocument:documents[selected]];
}
- (void)debugInformation:(id)sender {Alert([NSString stringWithFormat:@"NoteX %@ (%@)\n%@\nNative Apple Silicon\nScintilla / Lexilla / AppKit\nGPL version 3",[NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"],[NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleVersion"],NSProcessInfo.processInfo.operatingSystemVersionString]);}
- (void)buildUtilityMenus {
    NSMenu *edit=[NSApp.mainMenu itemWithTitle:@"Edit"].submenu;NSArray *binary=@[@"Copy Binary Content",@"Cut Binary Content",@"Paste Binary Content"];for(NSInteger i=0;i<3;++i)[self item:edit title:binary[i] action:@selector(binaryClipboard:) key:@"" tag:i];
    [self item:edit title:@"Redact Selection" action:@selector(redactSelection:) key:@"" tag:0];[self item:edit title:@"Open Selected File" action:@selector(openSelectedPath:) key:@"" tag:0];[self item:edit title:@"Reveal Selected Path in Finder" action:@selector(openSelectedPath:) key:@"" tag:1];
    [self item:edit title:@"Find Unicode Character Range…" action:@selector(findCharacterRange:) key:@"" tag:0];[self item:edit title:@"Copy All Styled Text" action:@selector(copyAllStyledText:) key:@"" tag:0];
    [self item:[NSApp.mainMenu itemWithTitle:@"Bookmarks"].submenu title:@"Replace Bookmarked Lines with Clipboard" action:@selector(bookmarkLines:) key:@"" tag:5];
    [self item:[NSApp.mainMenu itemWithTitle:@"File"].submenu title:@"Clear Read-Only for All Documents" action:@selector(readOnly:) key:@"" tag:2];
    [self item:[NSApp.mainMenu itemWithTitle:@"View"].submenu title:@"Recent Window Documents…" action:@selector(windowHistory:) key:@"" tag:0];
    [self item:[NSApp.mainMenu itemWithTitle:@"Help"].submenu title:@"Debug Information" action:@selector(debugInformation:) key:@"" tag:0];
    [self item:[NSApp.mainMenu itemWithTitle:@"Format"].submenu title:@"Background Color…" action:@selector(pickColor:) key:@"" tag:NoteX::Back];
}
@end
