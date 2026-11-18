// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Uses the shared Notepad++ LexUser property and word-list contract.
#import "UserLanguage.h"
#include "SciLexer.h"
#include <string>
static sptr_t U(ScintillaView *v,unsigned m,uptr_t w=0,sptr_t l=0){return [v message:m wParam:w lParam:l];}
static NSDictionary *NestNames(void){return @{@"delimiter1":@1,@"delimiter2":@2,@"delimiter3":@4,@"delimiter4":@8,@"delimiter5":@16,@"delimiter6":@32,@"delimiter7":@64,@"delimiter8":@128,@"comment":@256,@"lineComment":@512,@"keyword1":@1024,@"keyword2":@2048,@"keyword3":@4096,@"keyword4":@8192,@"keyword5":@16384,@"keyword6":@32768,@"keyword7":@65536,@"keyword8":@131072,@"operators1":@(SCE_USER_MASK_NESTING_OPERATORS1),@"operators2":@(SCE_USER_MASK_NESTING_OPERATORS2),@"numbers":@(SCE_USER_MASK_NESTING_NUMBERS)};}
NSDictionary *NXAdvancedLanguageDefaults(void){
    NSMutableDictionary *d=[@{@"advancedRules":@NO,@"allowFoldOfComments":@YES,@"foldCompact":@NO,@"forcePureLC":@0,@"decimalSeparator":@0,@"commentLineContinue":@"",@"commentLineClose":@"",@"operators1":@"+ - * / = { } [ ] ( )",@"operators2":@"",@"foldersInCode1Middle":@"",@"foldersInCode2Open":@"",@"foldersInCode2Middle":@"",@"foldersInCode2Close":@"",@"foldersInCommentOpen":@"",@"foldersInCommentMiddle":@"",@"foldersInCommentClose":@"",@"commentNested":@"",@"lineCommentNested":@""} mutableCopy];
    for(NSString *key in @[@"numberPrefix1",@"numberPrefix2",@"numberExtras1",@"numberExtras2",@"numberSuffix1",@"numberSuffix2",@"numberRange"])d[key]=@"";
    for(int i=1;i<=8;++i){d[[NSString stringWithFormat:@"prefixKeywords%d",i]]=@NO;for(NSString *part in @[@"Open",@"Escape",@"Close",@"Nested"])d[[NSString stringWithFormat:@"delimiter%d%@",i,part]]=@"";}
    d[@"delimiter1Open"]=@"\"";d[@"delimiter1Close"]=@"\"";d[@"delimiter1Escape"]=@"\\";d[@"delimiter2Open"]=@"'";d[@"delimiter2Close"]=@"'";d[@"delimiter2Escape"]=@"\\";return d;
}
BOOL NXValidateAdvancedLanguage(NSDictionary *language){
    NSDictionary *defaults=NXAdvancedLanguageDefaults();
    for(NSString *key in defaults)if(language[key]){if([defaults[key] isKindOfClass:NSString.class]){if(![language[key] isKindOfClass:NSString.class]||[language[key] length]>65536)return NO;if([key hasSuffix:@"Nested"])for(NSString *token in [language[key] componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@" ,\n\t"]])if(token.length&&!NestNames()[token])return NO;}else if(![language[key] isKindOfClass:NSNumber.class])return NO;}
    for(NSString *key in @[@"forcePureLC",@"decimalSeparator"])if([language[key] integerValue]<0||[language[key] integerValue]>2)return NO;
    for(int i=1;i<=8;++i)if(([language[[NSString stringWithFormat:@"delimiter%dOpen",i]] length]==0)!=([language[[NSString stringWithFormat:@"delimiter%dClose",i]] length]==0))return NO;return YES;
}
static std::string Words(NSString *value){std::string input=value.UTF8String ?: "",output;char quote=0;for(size_t i=0;i<input.size();++i){char c=input[i];if(c=='\\'&&i+1<input.size()&&(input[i+1]=='\\'||input[i+1]=='\''||input[i+1]=='"')){output+=input[++i];continue;}if((c=='\''||c=='"')&&(!quote||quote==c)){quote=quote?0:c;continue;}if(quote&&(c==' '||c=='\t')){if(!output.empty()&&output.back()!='\v'&&output.back()!='\b')output+=quote=='"'?'\v':'\b';}else output+=c;}return output;}
void NXConfigureUserLanguage(ScintillaView *view,NSDictionary *language,NSString *documentID){
    NSMutableDictionary *d=[NXAdvancedLanguageDefaults() mutableCopy];[d addEntriesFromDictionary:language];
    auto property=[&](NSString *name,NSString *value){U(view,SCI_SETPROPERTY,reinterpret_cast<uptr_t>(name.UTF8String),reinterpret_cast<sptr_t>(value.UTF8String));};
    property(@"userDefine.isCaseIgnored",[d[@"caseSensitive"] boolValue]?@"0":@"1");property(@"fold",@"1");
    for(NSString *key in @[@"allowFoldOfComments",@"foldCompact",@"forcePureLC",@"decimalSeparator",@"operators1",@"foldersInCode1Middle",@"numberPrefix1",@"numberPrefix2",@"numberExtras1",@"numberExtras2",@"numberSuffix1",@"numberSuffix2",@"numberRange"])property([@"userDefine." stringByAppendingString:key],[d[key] description]);
    property(@"userDefine.foldersInCode1Open",d[@"foldOpen"]);property(@"userDefine.foldersInCode1Close",d[@"foldClose"]);
    property(@"userDefine.comments",[NSString stringWithFormat:@"00%@ 01%@ 02%@ 03%@ 04%@",d[@"lineComment"],d[@"commentLineContinue"],d[@"commentLineClose"],d[@"blockOpen"],d[@"blockClose"]]);
    NSMutableArray *delimiters=[NSMutableArray new];for(int i=1;i<=8;++i){int j=0;for(NSString *part in @[@"Open",@"Escape",@"Close"])[delimiters addObject:[NSString stringWithFormat:@"%02d%@",(i-1)*3+j++,d[[NSString stringWithFormat:@"delimiter%d%@",i,part]]]];property([NSString stringWithFormat:@"userDefine.prefixKeywords%d",i],[d[[NSString stringWithFormat:@"prefixKeywords%d",i]] boolValue]?@"1":@"0");}property(@"userDefine.delimiters",[delimiters componentsJoinedByString:@" "]);
    NSMutableArray *keywords=[NSMutableArray arrayWithArray:@[@"operators2",@"foldersInCode2Open",@"foldersInCode2Middle",@"foldersInCode2Close",@"foldersInCommentOpen",@"foldersInCommentMiddle",@"foldersInCommentClose"]];for(int i=1;i<=8;++i)[keywords addObject:[NSString stringWithFormat:@"keywords%d",i]];NSUInteger index=0;for(NSString *key in keywords){std::string words=Words(d[key]);U(view,SCI_SETKEYWORDS,index++,reinterpret_cast<sptr_t>(words.c_str()));}
    static NSMutableDictionary *identifiers;static NSUInteger next=1;if(!identifiers)identifiers=[NSMutableDictionary new];for(NSString *key in @[documentID,d[@"id"]])if(!identifiers[key])identifiers[key]=@(next++);
    property(@"userDefine.currentBufferID",[identifiers[documentID] description]);property(@"userDefine.udlName",[identifiers[d[@"id"]] description]);
    for(int style=0;style<=24;++style){NSString *key=style==1?@"commentNested":style==2?@"lineCommentNested":style>=16&&style<=23?[NSString stringWithFormat:@"delimiter%dNested",style-15]:@"";unsigned mask=0;for(NSString *token in [d[key] componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@" ,\n\t"]])mask|=[NestNames()[token] unsignedIntValue];property([NSString stringWithFormat:@"userDefine.nesting.%02d",style],[@(mask) description]);}
    U(view,SCI_COLOURISE,0,-1);
}
void NXApplyUserLanguageTheme(ScintillaView *view,NSDictionary *theme){
    auto color=[](NSString *hex){unsigned value=0;[[NSScanner scannerWithString:hex] scanHexInt:&value];return ((value>>16)&255)|(value&0xff00)|((value&255)<<16);};
    for(int i=0;i<8;++i){U(view,SCI_STYLESETFORE,4+i,color(theme[[NSString stringWithFormat:@"keyword%d",i+1]]));U(view,SCI_STYLESETBOLD,4+i,[theme[@"boldKeywords"] boolValue]);U(view,SCI_STYLESETFORE,16+i,color(theme[@"string"]));}
    for(int i=1;i<=2;++i){U(view,SCI_STYLESETFORE,i,color(theme[@"comment"]));U(view,SCI_STYLESETITALIC,i,[theme[@"italicComments"] boolValue]);}U(view,SCI_STYLESETFORE,3,color(theme[@"number"]));U(view,SCI_STYLESETFORE,12,color(theme[@"operator"]));
}
