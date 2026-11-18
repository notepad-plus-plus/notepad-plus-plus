// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Document search options, navigation, replacement, and marking, added 2026-11-01.
@implementation NXApp (SearchCommands)
- (void)searchSettings:(id)sender {
    NSAlert *alert=[NSAlert new];alert.messageText=@"Search Options";NSView *form=[[NSView alloc]initWithFrame:NSMakeRect(0,0,340,180)];NSMutableArray *buttons=[NSMutableArray new];NSArray *keys=@[@"matchCase",@"wholeWord",@"regex",@"selection",@"incremental"];
    NSArray *labels=@[@"Match case",@"Whole word",@"Regular expression",@"Search current selection",@"Find while typing"];
    for(NSUInteger i=0;i<keys.count;++i){NSButton *button=[NSButton checkboxWithTitle:labels[i] target:nil action:nil];button.frame=NSMakeRect(0,150-i*28,320,26);button.state=[self.searchOptions[keys[i]] boolValue]?NSControlStateValueOn:NSControlStateValueOff;[form addSubview:button];[buttons addObject:button];}
    NSPopUpButton *engine=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(0,0,320,28) pullsDown:NO];[engine addItemsWithTitles:@[@"Boost.Regex (Windows-compatible)",@"ICU (previous macOS behavior)"]];[engine selectItemAtIndex:[self.searchOptions[@"engine"] isEqual:@"icu"]?1:0];[form addSubview:engine];alert.accessoryView=form;[alert addButtonWithTitle:@"Apply"];[alert addButtonWithTitle:@"Cancel"];
    if([alert runModal]!=NSAlertFirstButtonReturn)return;
    for(NSUInteger i=0;i<keys.count;++i)self.searchOptions[keys[i]]=@([buttons[i] state]==NSControlStateValueOn);self.searchOptions[@"engine"]=engine.indexOfSelectedItem?@"icu":@"boost";
    if([self.searchOptions[@"selection"] boolValue])self.searchScopeRecord=@{@"id":self.current.identifier,@"hash":NXDigest(Text(self.current.editor)),@"start":@(Send(self.current.editor,SCI_GETSELECTIONSTART)),@"end":@(Send(self.current.editor,SCI_GETSELECTIONEND))};else self.searchScopeRecord=nil;
}
- (NSArray *)documentMatches {
    if(!self.findText.stringValue.length){[self showFind:nil];return nil;}
    NSMutableDictionary *options=[self.searchOptions mutableCopy];options[@"query"]=self.findText.stringValue;options[@"replacement"]=self.replaceText.stringValue;
    NSData *data=Text(self.current.editor);NSUInteger offset=0;
    if([options[@"selection"] boolValue]){NSDictionary *scope=self.searchScopeRecord;if(![scope[@"id"] isEqual:self.current.identifier]||![scope[@"hash"] isEqual:NXDigest(data)]){Alert(@"The selection or document changed. Choose Search Options to capture a new selection.");return nil;}offset=[scope[@"start"] unsignedIntegerValue];NSUInteger end=[scope[@"end"] unsignedIntegerValue];if(end<=offset){Alert(@"Select a nonempty range to search.");return nil;}data=[data subdataWithRange:NSMakeRange(offset,end-offset)];}
    if(data.length>16*1024*1024){Alert(@"Narrow this search to a selection of at most 16 MB.");return nil;}
    NSError *error=nil;NSArray *matches=NXSearchMatches([[NSString alloc]initWithData:data encoding:NSUTF8StringEncoding],options,&error);if(!matches){Alert(error.localizedDescription);return nil;}
    NSMutableArray *result=[NSMutableArray new];for(NSDictionary *match in matches){NSMutableDictionary *item=[match mutableCopy];item[@"position"]=@([item[@"position"] unsignedIntegerValue]+offset);item[@"line"]=@(Send(self.current.editor,SCI_LINEFROMPOSITION,[item[@"position"] unsignedLongLongValue])+1);[result addObject:item];}return result;
}
- (void)searchNavigate:(id)sender {
    NSArray *matches=[self documentMatches];if(!matches)return;if(!matches.count){NSBeep();return;}ScintillaView *view=self.current.editor;BOOL previous=[sender tag]<0;
    auto start=Send(view,SCI_GETSELECTIONSTART),end=Send(view,SCI_GETSELECTIONEND);NSDictionary *chosen=nil;
    for(NSDictionary *match in previous?matches.reverseObjectEnumerator:matches){auto pos=[match[@"position"] longLongValue];if(previous?pos<start:pos>=end){if(pos==start&&start==end&&[self.lastZeroMatch isEqual:@(pos)])continue;chosen=match;break;}}
    if(!chosen)chosen=previous?matches.lastObject:matches.firstObject;auto position=[chosen[@"position"] longLongValue],length=[chosen[@"length"] longLongValue];self.lastZeroMatch=length?nil:@(position);Send(view,SCI_SETSEL,position,position+length);Send(view,SCI_SCROLLCARET);if(!self.searchTyping)[self.window makeFirstResponder:view];
}
- (void)searchReplace:(id)sender {
    ScintillaView *view=self.current.editor;if(Send(view,SCI_GETREADONLY))return;NSArray *matches=[self documentMatches];if(!matches)return;
    if(matches.count>=20000){Alert(@"This replacement reached the 20,000-match limit. Narrow the search before replacing.");return;}
    BOOL all=[sender tag]!=0;auto start=Send(view,SCI_GETSELECTIONSTART),end=Send(view,SCI_GETSELECTIONEND);NSUInteger changed=0;Send(view,SCI_BEGINUNDOACTION);
    for(NSDictionary *match in matches.reverseObjectEnumerator){auto a=[match[@"position"] longLongValue],b=a+[match[@"length"] longLongValue];if(all||(a==start&&b==end)){NXReplaceRange(view,a,b,match[@"replacement"]);++changed;}}
    Send(view,SCI_ENDUNDOACTION);if([self.searchOptions[@"selection"] boolValue]&&changed)self.searchScopeRecord=nil;
    if(!all&&![self.searchOptions[@"selection"] boolValue])[self searchNavigate:nil];[self tick:nil];
}
- (void)searchUtility:(id)sender {
    NSInteger action=[sender tag];ScintillaView *view=self.current.editor;
    if(action==9){for(int i=10;i<16;++i){Send(view,SCI_SETINDICATORCURRENT,i);Send(view,SCI_INDICATORCLEARRANGE,0,Send(view,SCI_GETLENGTH));}return;}
    NSArray *matches=[self documentMatches];if(!matches)return;
    if(action==0){Alert([NSString stringWithFormat:@"%lu matches%@",(unsigned long)matches.count,matches.count>=20000?@" (limit reached)":@""]);return;}
    if(action==1){[self findInFiles:nil];NXDocument *doc=self.current;[self.fileSearch showMatches:matches document:@{@"id":doc.identifier,@"path":doc.path,@"title":doc.title,@"text":Text(doc.editor)} parent:self.window];return;}
    if(action>=2&&action<=6||action==8){int slot=action==8?5:(int)action-2;int indicator=10+slot;unsigned colors[]={0x80ffff,0xffc080,0xa0ffa0,0xc080ff,0x8080ff,0xffff80};Send(view,SCI_INDICSETSTYLE,indicator,INDIC_ROUNDBOX);Send(view,SCI_INDICSETFORE,indicator,colors[slot]);Send(view,SCI_INDICSETALPHA,indicator,70);Send(view,SCI_SETINDICATORCURRENT,indicator);for(NSDictionary *m in matches)Send(view,SCI_INDICATORFILLRANGE,[m[@"position"] unsignedLongLongValue],[m[@"length"] longLongValue]);}
    if(action==7)for(NSDictionary *m in matches)Send(view,SCI_MARKERADD,Send(view,SCI_LINEFROMPOSITION,[m[@"position"] unsignedLongLongValue]),0);
}
- (void)matchingBrace:(id)sender {ScintillaView *view=self.current.editor;auto pos=Send(view,SCI_GETCURRENTPOS),match=Send(view,SCI_BRACEMATCH,pos);if(match<0&&pos>0){--pos;match=Send(view,SCI_BRACEMATCH,pos);}if(match<0){NSBeep();return;}if([sender tag])Send(view,SCI_SETSEL,MIN(pos,match)+1,MAX(pos,match));else Send(view,SCI_GOTOPOS,match);}
- (void)searchResult:(id)sender {if(!self.fileSearch){[self findInFiles:nil];return;}if(![sender tag])[self.fileSearch showForWindow:self.window];else[self.fileSearch navigateBy:[sender tag]];}
- (void)incrementalSearch {self.searchTyping=YES;if(self.findText.stringValue.length)[self searchNavigate:nil];self.searchTyping=NO;}
- (void)controlTextDidChange:(NSNotification *)notification {if(notification.object==self.findText&&[self.searchOptions[@"incremental"] boolValue]){[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(incrementalSearch) object:nil];[self performSelector:@selector(incrementalSearch) withObject:nil afterDelay:0.15];}}
- (void)buildSearchMenus {
    NSMenu *edit=[NSApp.mainMenu itemWithTitle:@"Edit"].submenu;[self item:edit title:@"Find Previous" action:@selector(searchNavigate:) key:@"G" tag:-1];[self item:edit title:@"Search Options…" action:@selector(searchSettings:) key:@"" tag:0];
    [self item:edit title:@"Count Matches" action:@selector(searchUtility:) key:@"" tag:0];[self item:edit title:@"Find All in Document…" action:@selector(searchUtility:) key:@"" tag:1];NSMenu *mark=[self menu:@"Mark Matches" parent:edit];for(int i=1;i<=5;++i)[self item:mark title:[NSString stringWithFormat:@"Style %d",i] action:@selector(searchUtility:) key:@"" tag:i+1];[self item:mark title:@"Find Mark Style" action:@selector(searchUtility:) key:@"" tag:8];[self item:mark title:@"Bookmark Matching Lines" action:@selector(searchUtility:) key:@"" tag:7];[self item:mark title:@"Clear All Marks" action:@selector(searchUtility:) key:@"" tag:9];
    [self item:edit title:@"Search Results Window" action:@selector(searchResult:) key:@"" tag:0];[self item:edit title:@"Next Search Result" action:@selector(searchResult:) key:@"" tag:1];[self item:edit title:@"Previous Search Result" action:@selector(searchResult:) key:@"" tag:-1];
    [self item:edit title:@"Go to Matching Brace" action:@selector(matchingBrace:) key:@"" tag:0];[self item:edit title:@"Select Between Braces" action:@selector(matchingBrace:) key:@"" tag:1];
}
@end
