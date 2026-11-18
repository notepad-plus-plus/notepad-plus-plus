// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Shared API completion and native path completion, added 2026-11-01.
static NSDictionary *NXAPI(NSString *language) {
    static NSMutableDictionary *cache;static dispatch_once_t once;dispatch_once(&once,^{cache=[NSMutableDictionary new];});if(cache[language])return cache[language];
    NSURL *url=[NSBundle.mainBundle.resourceURL URLByAppendingPathComponent:[NSString stringWithFormat:@"APIs/%@.xml",language]];NSData *data=[NSData dataWithContentsOfURL:url];if(!data||data.length>16*1024*1024)return @{};
    NSXMLDocument *xml=[[NSXMLDocument alloc]initWithData:data options:NSXMLNodeLoadExternalEntitiesNever error:nil];NSMutableDictionary *api=[NSMutableDictionary new];
    for(NSXMLElement *word in [xml nodesForXPath:@"/NotepadPlus/AutoComplete/KeyWord" error:nil]){NSString *name=[word attributeForName:@"name"].stringValue;if(!name.length)continue;NSMutableArray *signatures=[NSMutableArray new];for(NSXMLElement *overload in [word elementsForName:@"Overload"]){NSMutableArray *parameters=[NSMutableArray new];for(NSXMLElement *param in [overload elementsForName:@"Param"])[parameters addObject:[param attributeForName:@"name"].stringValue ?: @""];NSString *ret=[overload attributeForName:@"retVal"].stringValue ?: @"",*description=[overload attributeForName:@"descr"].stringValue ?: @"";[signatures addObject:[NSString stringWithFormat:@"%@ %@(%@)%@%@",ret,name,[parameters componentsJoinedByString:@", "],description.length?@"\n":@"",description]];}api[name]=signatures;}
    cache[language]=api;return api;
}
@implementation NXApp (Completion)
- (void)completeFunction:(id)sender {
    ScintillaView *view=self.current.editor;auto end=Send(view,SCI_GETCURRENTPOS),start=Send(view,SCI_WORDSTARTPOSITION,end,1);NSString *prefix=[[NSString alloc]initWithData:[Text(view) subdataWithRange:NSMakeRange(start,end-start)] encoding:NSUTF8StringEncoding];
    NSDictionary *api=NXAPI(self.current.builtinLanguage.length?self.current.builtinLanguage:self.current.lexer);NSMutableArray *matches=[NSMutableArray new];for(NSString *name in api)if([name hasPrefix:prefix])[matches addObject:name];
    if(!matches.count)return;[matches sortUsingSelector:@selector(compare:)];NSString *list=[matches componentsJoinedByString:@"\n"];Send(view,SCI_AUTOCSETSEPARATOR,'\n');Send(view,SCI_AUTOCSHOW,end-start,reinterpret_cast<sptr_t>(list.UTF8String));
}
- (void)parameterHint:(id)sender {
    ScintillaView *view=self.current.editor;auto caret=Send(view,SCI_GETCURRENTPOS);std::string text(static_cast<const char *>(Text(view).bytes),caret);int depth=0;sptr_t open=-1;
    for(sptr_t i=caret-1;i>=0&&caret-i<65536;--i){if(text[i]==')')++depth;else if(text[i]=='('){if(!depth){open=i;break;}--depth;}}
    if(open<0)return;sptr_t end=open;while(end>0&&std::isspace(static_cast<unsigned char>(text[end-1])))--end;sptr_t start=end;while(start>0&&(std::isalnum(static_cast<unsigned char>(text[start-1]))||text[start-1]=='_'||text[start-1]=='.'))--start;
    NSString *name=[[NSString alloc]initWithBytes:text.data()+start length:end-start encoding:NSUTF8StringEncoding];NSArray *hints=NXAPI(self.current.builtinLanguage.length?self.current.builtinLanguage:self.current.lexer)[name];if(!hints.count)return;
    if(![self.hintName isEqual:name]||![sender tag])self.hintIndex=0;else self.hintIndex=(self.hintIndex+[sender tag]+hints.count)%hints.count;self.hintName=name;
    NSString *hint=[NSString stringWithFormat:@"%ld/%lu  %@",(long)self.hintIndex+1,(unsigned long)hints.count,hints[self.hintIndex]];Send(view,SCI_CALLTIPSHOW,start,reinterpret_cast<sptr_t>(hint.UTF8String));
}
- (void)completePath:(id)sender {
    ScintillaView *view=self.current.editor;auto caret=Send(view,SCI_GETCURRENTPOS),start=caret;std::string text(static_cast<const char *>(Text(view).bytes),caret);
    while(start>0&&text[start-1]!='\n'&&text[start-1]!='\r'&&text[start-1]!='"'&&text[start-1]!='\'')--start;
    if(start==0||(text[start-1]!='"'&&text[start-1]!='\'')){start=caret;while(start>0&&!std::isspace(static_cast<unsigned char>(text[start-1])))--start;}
    NSString *prefix=[[NSString alloc]initWithBytes:text.data()+start length:caret-start encoding:NSUTF8StringEncoding];NSString *directory=[prefix stringByDeletingLastPathComponent],*name=prefix.lastPathComponent;
    if([prefix hasSuffix:@"/"]){directory=prefix;name=@"";}
    directory=directory.stringByExpandingTildeInPath;if(!directory.isAbsolutePath)directory=[(self.current.path.length?self.current.path.stringByDeletingLastPathComponent:NSHomeDirectory()) stringByAppendingPathComponent:directory];
    NSArray *entries=[NSFileManager.defaultManager contentsOfDirectoryAtPath:directory error:nil];NSMutableArray *matches=[NSMutableArray new];for(NSString *entry in entries)if([entry hasPrefix:name]&&![entry containsString:@"\n"]){BOOL folder=NO;[NSFileManager.defaultManager fileExistsAtPath:[directory stringByAppendingPathComponent:entry] isDirectory:&folder];[matches addObject:folder?[entry stringByAppendingString:@"/"]:entry];if(matches.count>=1000)break;}
    if(!matches.count)return;[matches sortUsingSelector:@selector(compare:)];NSString *list=[matches componentsJoinedByString:@"\n"];Send(view,SCI_AUTOCSETSEPARATOR,'\n');Send(view,SCI_AUTOCSHOW,[name lengthOfBytesUsingEncoding:NSUTF8StringEncoding],reinterpret_cast<sptr_t>(list.UTF8String));
}
- (void)buildCompletionMenus {
    NSMenu *edit=[NSApp.mainMenu itemWithTitle:@"Edit"].submenu;[self item:edit title:@"Function Completion" action:@selector(completeFunction:) key:@"" tag:0];[self item:edit title:@"Function Parameters" action:@selector(parameterHint:) key:@"" tag:0];[self item:edit title:@"Next Function Signature" action:@selector(parameterHint:) key:@"" tag:1];[self item:edit title:@"Previous Function Signature" action:@selector(parameterHint:) key:@"" tag:-1];[self item:edit title:@"Path Completion" action:@selector(completePath:) key:@"" tag:0];
}
@end
