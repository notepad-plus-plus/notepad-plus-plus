// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Per-language syntax styles, added 2026-11-01.
@implementation NXApp (StyleOverrides)
- (void)styleEditor:(id)sender {
    ScintillaView *view=self.current.editor;NSString *lexer=self.current.customLanguage.length?[@"user:" stringByAppendingString:self.current.customLanguage]:self.current.lexer;
    NSMutableDictionary *theme=nil;for(NSDictionary *item in self.configuration[@"themes"])if([item[@"id"] isEqual:self.configuration[@"selectedTheme"]])theme=[item mutableCopy];
    if(!theme){theme=[NXDefaultTheme() mutableCopy];theme[@"name"]=[NSString stringWithFormat:@"Custom Styles %lu",(unsigned long)[self.configuration[@"themes"] count]+1];}
    NSMutableArray *records=[NSMutableArray new];NSInteger count=Send(view,SCI_GETNAMEDSTYLES);if(count<=0)count=self.current.customLanguage.length?25:1;
    for(NSInteger i=0;i<MIN(256,count);++i){NSMutableDictionary *record=[NXDefaultStyle() mutableCopy];record[@"style"]=@(i);record[@"lexer"]=lexer;auto length=Send(view,SCI_NAMEOFSTYLE,i);std::vector<char> name(length>0&&length<4096?length+1:1);if(length>0&&length<4096)Send(view,SCI_NAMEOFSTYLE,i,reinterpret_cast<sptr_t>(name.data()));record[@"name"]=length>0&&length<4096?@(name.data()):[NSString stringWithFormat:@"Style %ld",(long)i];
        for(NSString *key in @[@"foreground",@"background"]){unsigned color=(unsigned)Send(view,[key isEqual:@"foreground"]?SCI_STYLEGETFORE:SCI_STYLEGETBACK,i);record[key]=[NSString stringWithFormat:@"%02X%02X%02X",color&255,(color>>8)&255,(color>>16)&255];}
        auto fontLength=Send(view,SCI_STYLEGETFONT,i);std::vector<char> font(MAX((sptr_t)0,fontLength)+1);Send(view,SCI_STYLEGETFONT,i,reinterpret_cast<sptr_t>(font.data()));record[@"font"]=@(font.data());record[@"fontSize"]=@(Send(view,SCI_STYLEGETSIZEFRACTIONAL,i)/100.0);record[@"bold"]=@(Send(view,SCI_STYLEGETBOLD,i)!=0);record[@"italic"]=@(Send(view,SCI_STYLEGETITALIC,i)!=0);record[@"underline"]=@(Send(view,SCI_STYLEGETUNDERLINE,i)!=0);
        for(NSDictionary *saved in theme[@"styles"])if([saved[@"lexer"] isEqual:lexer]&&[saved[@"style"] integerValue]==i){record=[saved mutableCopy];break;}[records addObject:record];
    }
    NXRecordEditor *editor=[[NXRecordEditor alloc]initWithKind:@"styles" records:records];NSArray *result=[editor runForWindow:self.window];if(!result)return;
    NSMutableArray *styles=[NSMutableArray new];for(NSDictionary *style in theme[@"styles"])if(![style[@"lexer"] isEqual:lexer])[styles addObject:style];[styles addObjectsFromArray:result];theme[@"styles"]=styles;
    NSMutableDictionary *configuration=[self.configuration mutableCopy];NSMutableArray *themes=[configuration[@"themes"] mutableCopy];NSUInteger index=[themes indexOfObjectPassingTest:^BOOL(NSDictionary *item,NSUInteger i,BOOL *stop){return [item[@"id"] isEqual:theme[@"id"]];}];if(index==NSNotFound)[themes addObject:theme];else themes[index]=theme;configuration[@"themes"]=themes;configuration[@"selectedTheme"]=theme[@"id"];[self commitConfiguration:configuration];
}
@end
