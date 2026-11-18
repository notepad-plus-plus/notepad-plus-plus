// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native navigation, selection, marks, and display commands, added 2026-11-01.
@implementation NXApp (NavigationCommands)
- (void)extendedSelection:(id)sender {
    ScintillaView *view=self.current.editor;NSInteger action=[sender tag];
    if(action<2){if(self.selectionAnchor<0){self.selectionAnchor=Send(view,SCI_GETCURRENTPOS);self.selectionDocument=self.current.identifier;}else{if([self.selectionDocument isEqual:self.current.identifier]){Send(view,SCI_SETSELECTIONMODE,action?SC_SEL_RECTANGLE:SC_SEL_STREAM);Send(view,SCI_SETSEL,self.selectionAnchor,Send(view,SCI_GETCURRENTPOS));}self.selectionAnchor=-1;}return;}
    if(action==2){auto count=Send(view,SCI_GETSELECTIONS);if(count>1)Send(view,SCI_DROPSELECTIONN,count-1);return;}
    if(action==3){auto count=Send(view,SCI_GETSELECTIONS);Send(view,SCI_MULTIPLESELECTADDNEXT);if(Send(view,SCI_GETSELECTIONS)>count)Send(view,SCI_DROPSELECTIONN,Send(view,SCI_GETMAINSELECTION));return;}
    NSInteger flags=(action-10)%4;BOOL all=action>=14;NSString *text=view.selectedString;if(!text.length)return;NSError *error=nil;
    NSArray *matches=NXSearchMatches(view.string,@{@"query":text,@"wholeWord":@(flags>=2),@"matchCase":@(flags==1||flags==3)},&error);if(!matches){Alert(error.localizedDescription);return;}
    auto current=Send(view,SCI_GETSELECTIONEND);NSDictionary *next=nil;
    if(all)Send(view,SCI_CLEARSELECTIONS);BOOL first=YES;
    for(NSDictionary *match in matches){auto a=[match[@"position"] longLongValue],b=a+[match[@"length"] longLongValue];if(all){Send(view,first?SCI_SETSELECTION:SCI_ADDSELECTION,b,a);first=NO;}else if(!next&&a>=current)next=match;}
    if(!all&&(next ?: matches.firstObject)){NSDictionary *match=next ?: matches.firstObject;Send(view,SCI_ADDSELECTION,[match[@"position"] longLongValue]+[match[@"length"] longLongValue],[match[@"position"] longLongValue]);}
}
- (void)selectedSearch:(id)sender {
    NSInteger action=[sender tag];NSString *query=self.current.editor.selectedString;if(!query.length){auto pos=Send(self.current.editor,SCI_GETCURRENTPOS),start=Send(self.current.editor,SCI_WORDSTARTPOSITION,pos,1),end=Send(self.current.editor,SCI_WORDENDPOSITION,pos,1);query=[[NSString alloc]initWithData:[Text(self.current.editor) subdataWithRange:NSMakeRange(start,end-start)] encoding:NSUTF8StringEncoding];}if(!query.length)return;
    NSString *previous=self.findText.stringValue;NSMutableDictionary *options=self.searchOptions;self.findText.stringValue=query;self.searchOptions=[@{@"matchCase":@NO,@"wholeWord":@NO,@"regex":@NO,@"selection":@NO} mutableCopy];NSMenuItem *command=[NSMenuItem new];command.tag=action%2?-1:1;[self searchNavigate:command];self.searchOptions=options;if(action>=2)self.findText.stringValue=previous;
}
- (void)markCommand:(id)sender {
    ScintillaView *view=self.current.editor;NSInteger action=[sender tag]/10,style=[sender tag]%10;int indicator=10+(int)style;auto length=Send(view,SCI_GETLENGTH);
    Send(view,SCI_SETINDICATORCURRENT,indicator);
    if(action==0){unsigned colors[]={0x80ffff,0xffc080,0xa0ffa0,0xc080ff,0x8080ff,0xffff80};Send(view,SCI_INDICSETSTYLE,indicator,INDIC_ROUNDBOX);Send(view,SCI_INDICSETFORE,indicator,colors[style]);Send(view,SCI_INDICSETALPHA,indicator,70);auto a=Send(view,SCI_GETSELECTIONSTART),b=Send(view,SCI_GETSELECTIONEND);Send(view,SCI_INDICATORFILLRANGE,a,b-a);return;}
    if(action==1){Send(view,SCI_INDICATORCLEARRANGE,0,length);return;}
    NSMutableArray *ranges=[NSMutableArray new];for(sptr_t pos=0;pos<length;){auto end=Send(view,SCI_INDICATOREND,indicator,pos);if(end<=pos)break;if(Send(view,SCI_INDICATORVALUEAT,indicator,pos))[ranges addObject:@[@(pos),@(end)]];pos=end;}
    if(action==4){NSMutableArray *parts=[NSMutableArray new];NSData *text=Text(view);for(NSArray *r in ranges)[parts addObject:[[NSString alloc]initWithData:[text subdataWithRange:NSMakeRange([r[0] unsignedLongLongValue],[r[1] unsignedLongLongValue]-[r[0] unsignedLongLongValue])] encoding:NSUTF8StringEncoding]];[NSPasteboard.generalPasteboard clearContents];[NSPasteboard.generalPasteboard setString:[parts componentsJoinedByString:NXEOL(view)] forType:NSPasteboardTypeString];return;}
    NSArray *selected=nil;auto caret=Send(view,SCI_GETCURRENTPOS);for(NSArray *range in action==3?ranges.reverseObjectEnumerator:ranges)if(action==3?[range[0] longLongValue]<caret:[range[0] longLongValue]>caret){selected=range;break;}if(!selected)selected=action==3?ranges.lastObject:ranges.firstObject;if(selected){Send(view,SCI_SETSEL,[selected[0] longLongValue],[selected[1] longLongValue]);Send(view,SCI_SCROLLCARET);}else NSBeep();
}
- (void)displayCommand:(id)sender {
    NSInteger action=[sender tag];ScintillaView *view=self.current.editor;
    if(action==0)self.window.level=self.window.level==NSFloatingWindowLevel?NSNormalWindowLevel:NSFloatingWindowLevel;
    else if(action==1){self.focusMode=!self.focusMode;self.toolbar.hidden=self.focusMode;self.findBar.hidden=YES;self.status.hidden=self.focusMode;if(self.focusMode&&self.sidebarVisible)[self toggleSidebar:nil];}
    else if(action==2){BOOL borderless=(self.window.styleMask&NSWindowStyleMaskTitled)==0;self.window.styleMask=borderless?NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable:NSWindowStyleMaskBorderless|NSWindowStyleMaskResizable;self.window.movableByWindowBackground=!borderless;}
    else if(action>=10&&action<=14){NSString *key=@[@"whitespace",@"showEOL",@"indentGuides",@"lineNumbers",@"highlightLine"][action-10];self.preferences[key]=@(![self.preferences[key] boolValue]);[self refreshEditorSettings];}
    else if(action==17||action==18){self.preferences[@"nonPrinting"]=@(![self.preferences[@"nonPrinting"] boolValue]);if(action==18)self.preferences[@"showEOL"]=self.preferences[@"nonPrinting"];[self refreshEditorSettings];}
    else if(action==15)Send(view,SCI_SETWRAPVISUALFLAGS,Send(view,SCI_GETWRAPVISUALFLAGS)?0:SC_WRAPVISUALFLAG_END);
    else if(action==16){BOOL show=Send(view,SCI_GETVIEWWS)==SCWS_INVISIBLE;self.preferences[@"whitespace"]=@(show);self.preferences[@"showEOL"]=@(show);[self refreshEditorSettings];}
    else if(action==20){auto first=Send(view,SCI_LINEFROMPOSITION,Send(view,SCI_GETSELECTIONSTART)),last=Send(view,SCI_LINEFROMPOSITION,Send(view,SCI_GETSELECTIONEND));Send(view,SCI_HIDELINES,MAX(1,first),last);}
    else if(action==21)Send(view,SCI_SHOWLINES,0,Send(view,SCI_GETLINECOUNT)-1);
    else if(action==22||action==23)Send(view,SCI_FOLDLINE,Send(view,SCI_LINEFROMPOSITION,Send(view,SCI_GETCURRENTPOS)),action==22?SC_FOLDACTION_CONTRACT:SC_FOLDACTION_EXPAND);
    else if(action==30&&self.secondaryVisible){self.secondaryFocused=!self.secondaryFocused;for(NXDocument *doc in self.documents)doc.activeEditor=nil;if(self.secondaryFocused)self.secondaryDocument.activeEditor=self.secondaryEditor;[self.window makeFirstResponder:self.current.editor];}
    else if(action==31&&self.secondaryVisible){Send(self.secondaryEditor,SCI_SETZOOM,Send(self.current.editor,SCI_GETZOOM));Send((ScintillaView *)self.tabs.selectedTabViewItem.view,SCI_SETZOOM,Send(self.current.editor,SCI_GETZOOM));}
    [self tick:nil];
}
- (void)changeHistory:(id)sender {
    ScintillaView *view=self.current.editor;NSInteger action=[sender tag];if(!action){BOOL dirty=self.current.dirty;Send(view,SCI_EMPTYUNDOBUFFER);self.current.restoredDirty=dirty;Send(view,SCI_SETCHANGEHISTORY,0);Send(view,SCI_SETCHANGEHISTORY,SC_CHANGE_HISTORY_ENABLED|SC_CHANGE_HISTORY_MARKERS);return;}
    auto line=Send(view,SCI_LINEFROMPOSITION,Send(view,SCI_GETCURRENTPOS));unsigned mask=0;for(int marker=SC_MARKNUM_HISTORY_REVERTED_TO_ORIGIN;marker<=SC_MARKNUM_HISTORY_REVERTED_TO_MODIFIED;++marker)mask|=1u<<marker;
    auto found=Send(view,action>0?SCI_MARKERNEXT:SCI_MARKERPREVIOUS,action>0?line+1:line-1,mask);if(found<0)found=Send(view,action>0?SCI_MARKERNEXT:SCI_MARKERPREVIOUS,action>0?0:Send(view,SCI_GETLINECOUNT)-1,mask);if(found>=0)Send(view,SCI_GOTOLINE,found);else NSBeep();
}
- (void)directTab:(id)sender {NSInteger index=[sender tag]-1;if(index>=0&&index<(NSInteger)self.documents.count)[self activateDocument:self.documents[index]];}
- (void)sortTabs:(id)sender {
    NSInteger action=[sender tag];NXDocument *current=self.current;NSArray *sorted=[self.documents sortedArrayUsingComparator:^NSComparisonResult(NXDocument *a,NXDocument *b){NSInteger field=action/2;NSComparisonResult result;
        if(field==3)result=[@(Send(a.primaryEditor,SCI_GETLENGTH)) compare:@(Send(b.primaryEditor,SCI_GETLENGTH))];
        else if(field==4){NSDate *x=[NSFileManager.defaultManager attributesOfItemAtPath:a.path error:nil][NSFileModificationDate] ?: NSDate.distantPast,*y=[NSFileManager.defaultManager attributesOfItemAtPath:b.path error:nil][NSFileModificationDate] ?: NSDate.distantPast;result=[x compare:y];}
        else result=[field==0?a.title:field==1?a.path:a.path.pathExtension localizedStandardCompare:field==0?b.title:field==1?b.path:b.path.pathExtension];return action%2?(NSComparisonResult)-result:result;
    }];BOOL restoring=self.restoring;self.restoring=YES;for(NSTabViewItem *item in self.tabs.tabViewItems.copy)[self.tabs removeTabViewItem:item];self.documents=[sorted mutableCopy];for(NXDocument *doc in self.documents)[self.tabs addTabViewItem:doc.tab];[self activateDocument:current];self.restoring=restoring;[self tick:nil];
}
- (void)buildNavigationMenus {
    NSMenu *edit=[NSApp.mainMenu itemWithTitle:@"Edit"].submenu,*view=[NSApp.mainMenu itemWithTitle:@"View"].submenu;
    [self item:edit title:@"Begin / End Selection" action:@selector(extendedSelection:) key:@"" tag:0];[self item:edit title:@"Begin / End Column Selection" action:@selector(extendedSelection:) key:@"" tag:1];[self item:edit title:@"Undo Last Added Selection" action:@selector(extendedSelection:) key:@"" tag:2];[self item:edit title:@"Skip Current Occurrence" action:@selector(extendedSelection:) key:@"" tag:3];
    NSMenu *multi=[self menu:@"Multiple Selections" parent:edit];NSArray *options=@[@"Ignore Case",@"Match Case",@"Whole Word",@"Match Case and Whole Word"];for(int i=0;i<8;++i)[self item:multi title:[NSString stringWithFormat:@"%@ — %@",i<4?@"Next":@"All",options[i%4]] action:@selector(extendedSelection:) key:@"" tag:10+i];
    NSArray *selected=@[@"Select and Find Next",@"Select and Find Previous",@"Volatile Find Next",@"Volatile Find Previous"];for(NSUInteger i=0;i<selected.count;++i)[self item:edit title:selected[i] action:@selector(selectedSearch:) key:@"" tag:i];
    NSMenu *styles=[self menu:@"Styled Text" parent:edit];for(NSInteger i=0;i<6;++i){NSMenu *style=[self menu:i==5?@"Find Mark Style":[NSString stringWithFormat:@"Style %ld",(long)i+1] parent:styles];NSArray *actions=@[@"Mark Selection",@"Clear Style",@"Next",@"Previous",@"Copy Styled Text"];for(NSInteger j=0;j<5;++j)[self item:style title:actions[j] action:@selector(markCommand:) key:@"" tag:j*10+i];}
    [self item:edit title:@"Next Change" action:@selector(changeHistory:) key:@"" tag:1];[self item:edit title:@"Previous Change" action:@selector(changeHistory:) key:@"" tag:-1];[self item:edit title:@"Clear Change and Undo History" action:@selector(changeHistory:) key:@"" tag:0];
    NSArray *displays=@[@[@"Always on Top",@0],@[@"Distraction-Free Mode",@1],@[@"Post-It Mode",@2],@[@"Show Whitespace",@10],@[@"Show Line Endings",@11],@[@"Show Indent Guides",@12],@[@"Show Line Numbers",@13],@[@"Highlight Current Line",@14],@[@"Show Wrap Symbols",@15],@[@"Show All Characters",@16],@[@"Show Non-Printing Characters",@17],@[@"Show Control Characters and Unicode Line Endings",@18],@[@"Hide Selected Lines",@20],@[@"Show Hidden Lines",@21],@[@"Fold Current Level",@22],@[@"Unfold Current Level",@23],@[@"Focus Other View",@30],@[@"Synchronize Zoom",@31]];
    for(NSArray *item in displays)[self item:view title:item[0] action:@selector(displayCommand:) key:@"" tag:[item[1] integerValue]];
    NSMenu *tabs=[self menu:@"Go to Tab" parent:view];for(int i=1;i<=9;++i)[self item:tabs title:[NSString stringWithFormat:@"Tab %d",i] action:@selector(directTab:) key:[NSString stringWithFormat:@"%d",i] tag:i];
    NSMenu *sort=[self menu:@"Sort Tabs" parent:view];NSArray *fields=@[@"Name",@"Path",@"Type",@"Content Length",@"Modified Time"];for(int i=0;i<10;++i)[self item:sort title:[NSString stringWithFormat:@"%@ — %@",fields[i/2],i%2?@"Descending":@"Ascending"] action:@selector(sortTabs:) key:@"" tag:i];
}
@end
