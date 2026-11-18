// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Shared language catalog integration, added 2026-11-01.
static NSArray *NXLanguageCatalog(void) {
    static NSArray *catalog;static dispatch_once_t once;dispatch_once(&once,^{NSData *data=[NSData dataWithContentsOfURL:[NSBundle.mainBundle URLForResource:@"languages" withExtension:@"json"]];catalog=data?[NSJSONSerialization JSONObjectWithData:data options:0 error:nil]:@[];});return catalog;
}
static NSDictionary *NXBuiltIn(NSString *identifier) {for(NSDictionary *record in NXLanguageCatalog())if([record[@"id"] isEqual:identifier])return record;return nil;}
@implementation NXApp (LanguageCatalog)
- (void)chooseBuiltinLanguage:(id)sender {NSDictionary *language=NXBuiltIn([sender representedObject]);if(!language)return;self.current.builtinLanguage=language[@"id"];self.current.customLanguage=@"";self.current.lexer=language[@"lexer"];[self refreshEditorSettings];[self tick:nil];}
- (void)buildLanguageMenu {NSMenu *menu=[self menu:@"Language" parent:NSApp.mainMenu];for(NSDictionary *language in NXLanguageCatalog()){[self item:menu title:language[@"name"] action:@selector(chooseBuiltinLanguage:) key:@"" tag:0];menu.itemArray.lastObject.representedObject=language[@"id"];} }
@end
