// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Application-scoped clipboard history and rich paste, added 2026-11-01.
@implementation NXApp (ClipboardCommands)
- (void)cut:(id)sender {NSMenuItem *command=[NSMenuItem new];command.tag=0;[self clipboardCommand:command];}
- (void)copy:(id)sender {NSMenuItem *command=[NSMenuItem new];command.tag=1;[self clipboardCommand:command];}
- (void)paste:(id)sender {NSMenuItem *command=[NSMenuItem new];command.tag=2;[self clipboardCommand:command];}
- (void)clipboardCommand:(id)sender {
    NSInteger action=[sender tag];ScintillaView *view=self.current.editor;
    NSResponder *responder=self.window.firstResponder;BOOL editor=responder==view||([responder isKindOfClass:NSView.class]&&[(NSView *)responder isDescendantOf:view]);
    if(!editor){SEL selector=action==0?@selector(cut:):action==1?@selector(copy:):@selector(paste:);if([responder respondsToSelector:selector])[NSApp sendAction:selector to:responder from:sender];else NSBeep();return;}
    Send(view,action==0?SCI_CUT:action==1?SCI_COPY:SCI_PASTE);
    NSString *text=[NSPasteboard.generalPasteboard stringForType:NSPasteboardTypeString];
    if(text.length&&[text lengthOfBytesUsingEncoding:NSUTF8StringEncoding]<=1024*1024){[self.clipboardHistory removeObject:text];[self.clipboardHistory insertObject:text atIndex:0];if(self.clipboardHistory.count>50)[self.clipboardHistory removeLastObject];}
}
- (void)clipboardHistoryCommand:(id)sender {
    if([sender tag]){[self.clipboardHistory removeAllObjects];return;}
    if(!self.clipboardHistory.count){Alert(@"Clipboard history contains text copied, cut, or pasted in NoteX during this session.");return;}
    NSMutableArray *titles=[NSMutableArray new];for(NSString *text in self.clipboardHistory)[titles addObject:[[text substringToIndex:MIN((NSUInteger)160,text.length)] stringByReplacingOccurrencesOfString:@"\n" withString:@" ↵ "]];NSInteger index=NXChoose(@"Clipboard History",titles,self.window);if(index>=0)NXReplaceRange(self.current.editor,Send(self.current.editor,SCI_GETSELECTIONSTART),Send(self.current.editor,SCI_GETSELECTIONEND),self.clipboardHistory[index]);
}
- (void)pasteRich:(id)sender {
    ScintillaView *view=self.current.editor;if(Send(view,SCI_GETREADONLY))return;
    if([sender tag]){NSString *html=[NSPasteboard.generalPasteboard stringForType:NSPasteboardTypeHTML];if(html)NXReplaceRange(view,Send(view,SCI_GETSELECTIONSTART),Send(view,SCI_GETSELECTIONEND),html);else NSBeep();return;}
    NSData *data=[NSPasteboard.generalPasteboard dataForType:NSPasteboardTypeRTF];if(!data||data.length>16*1024*1024){Alert(@"The clipboard does not contain RTF text smaller than 16 MB.");return;}
    NSAttributedString *rich=[[NSAttributedString alloc]initWithRTF:data documentAttributes:nil];if(!rich){Alert(@"The clipboard's RTF data could not be read.");return;}
    auto start=Send(view,SCI_GETSELECTIONSTART),end=Send(view,SCI_GETSELECTIONEND);Send(view,SCI_BEGINUNDOACTION);NXReplaceRange(view,start,end,rich.string);
    [rich enumerateAttributesInRange:NSMakeRange(0,rich.length) options:0 usingBlock:^(NSDictionary *attributes,NSRange range,BOOL *stop){
        NoteX::Change change;change.start=start+[[rich.string substringToIndex:range.location] lengthOfBytesUsingEncoding:NSUTF8StringEncoding];change.end=change.start+[[rich.string substringWithRange:range] lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
        NSFont *font=attributes[NSFontAttributeName];if(font){auto traits=[NSFontManager.sharedFontManager traitsOfFont:font];change.fields|=NoteX::Family|NoteX::Size|NoteX::Bold|NoteX::Italic;change.value.family=font.familyName.UTF8String;change.value.size=lround(std::clamp<double>(font.pointSize,6,96)*100);change.value.bold=(traits&NSBoldFontMask)!=0;change.value.italic=(traits&NSItalicFontMask)!=0;}
        change.fields|=NoteX::Underline|NoteX::Strike;change.value.underline=[attributes[NSUnderlineStyleAttributeName] integerValue]!=0;change.value.strike=[attributes[NSStrikethroughStyleAttributeName] integerValue]!=0;
        for(NSString *key in @[NSForegroundColorAttributeName,NSBackgroundColorAttributeName]){NSColor *color=[attributes[key] colorUsingColorSpace:NSColorSpace.sRGBColorSpace];if(color){unsigned rgb=(unsigned)lround(color.redComponent*255)|((unsigned)lround(color.greenComponent*255)<<8)|((unsigned)lround(color.blueComponent*255)<<16);if([key isEqual:NSForegroundColorAttributeName]){change.fields|=NoteX::Fore;change.value.fore=rgb;}else{change.fields|=NoteX::Highlight;change.value.highlight=rgb;}}}
        change.value.mask=change.fields;Send(view,NoteX::ApplyFormat,0,reinterpret_cast<sptr_t>(&change));
    }];Send(view,SCI_ENDUNDOACTION);[self tick:nil];
}
- (void)customizeContextMenu:(id)sender {[self.contextLayout customizeForWindow:self.window];}
- (void)buildClipboardMenus {
    NSMenu *edit=[NSApp.mainMenu itemWithTitle:@"Edit"].submenu;
    NSArray *selectors=@[@"cut:",@"copy:",@"paste:"];for(NSMenuItem *item in edit.itemArray){NSUInteger i=[selectors indexOfObject:NSStringFromSelector(item.action) ?: @""];if(i!=NSNotFound){item.target=self;}}
    [self item:edit title:@"Clipboard History…" action:@selector(clipboardHistoryCommand:) key:@"" tag:0];[self item:edit title:@"Clear Clipboard History" action:@selector(clipboardHistoryCommand:) key:@"" tag:1];
    [self item:edit title:@"Paste RTF Formatting" action:@selector(pasteRich:) key:@"" tag:0];[self item:edit title:@"Paste HTML Source" action:@selector(pasteRich:) key:@"" tag:1];
    [self item:[NSApp.mainMenu itemAtIndex:0].submenu title:@"Customize Editor Context Menu…" action:@selector(customizeContextMenu:) key:@"" tag:0];
}
@end
