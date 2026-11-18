// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native language, theme, and preferences editors, created 2026-11-01.
#import "Configuration.h"
#import "Storage.h"
#import "UserLanguage.h"
#include <string>
#include <unordered_set>
#include <cctype>
#include <cmath>

static sptr_t S(ScintillaView *v, unsigned m, uptr_t w=0, sptr_t l=0) { return [v message:m wParam:w lParam:l]; }
NSDictionary *NXDefaultLanguage(void) {
    return @{@"id":NSUUID.UUID.UUIDString, @"name":@"My Language", @"extensions":@"example", @"caseSensitive":@YES,
        @"lineComment":@"//", @"blockOpen":@"/*", @"blockClose":@"*/", @"quotes":@"\"'", @"escape":@"\\",
        @"foldOpen":@"{", @"foldClose":@"}", @"keywords1":@"", @"keywords2":@"", @"keywords3":@"", @"keywords4":@"",
        @"keywords5":@"", @"keywords6":@"", @"keywords7":@"", @"keywords8":@""};
}
NSDictionary *NXDefaultTheme(void) {
    return @{@"id":NSUUID.UUID.UUIDString, @"name":@"My Theme", @"background":@"1C2126", @"foreground":@"E8ECEF",
        @"caret":@"FFD9C5", @"selection":@"375676", @"lineNumbers":@"99A2AA", @"currentLine":@"252D34",
        @"keyword1":@"A0BFE6", @"keyword2":@"C4A7E7", @"keyword3":@"EBA0AC", @"keyword4":@"F5C2E7",
        @"keyword5":@"89DCEB", @"keyword6":@"F9E2AF", @"keyword7":@"FAB387", @"keyword8":@"94E2D5",
        @"string":@"DFC89A", @"comment":@"88AA88", @"number":@"B4BEFE", @"operator":@"F5E0DC",
        @"boldKeywords":@NO, @"italicComments":@YES};
}
NSDictionary *NXDefaultStyle(void) {return @{@"id":NSUUID.UUID.UUIDString,@"name":@"Style",@"lexer":@"null",@"style":@0,@"foreground":@"E8ECEF",@"background":@"1C2126",@"font":@"JetBrains Mono",@"fontSize":@13,@"bold":@NO,@"italic":@NO,@"underline":@NO};}
BOOL NXValidateStyles(id styles) {
    if(![styles isKindOfClass:NSArray.class]||[styles count]>32768)return NO;NSMutableSet *seen=[NSMutableSet new];
    for(id style in styles){if(![style isKindOfClass:NSDictionary.class])return NO;for(NSString *key in @[@"id",@"name",@"lexer",@"foreground",@"background",@"font"])if(![style[key] isKindOfClass:NSString.class]||[style[key] length]>256)return NO;for(NSString *key in @[@"style",@"fontSize",@"bold",@"italic",@"underline"])if(![style[key] isKindOfClass:NSNumber.class])return NO;if([style[@"style"] integerValue]<0||[style[@"style"] integerValue]>255||!std::isfinite([style[@"fontSize"] doubleValue])||[style[@"fontSize"] doubleValue]<6||[style[@"fontSize"] doubleValue]>96||[style[@"foreground"] length]!=6||[style[@"background"] length]!=6)return NO;for(NSString *key in @[@"foreground",@"background"])if([style[key] rangeOfCharacterFromSet:[NSCharacterSet characterSetWithCharactersInString:@"0123456789abcdefABCDEF"].invertedSet].location!=NSNotFound)return NO;NSString *key=[NSString stringWithFormat:@"%@:%@",style[@"lexer"],style[@"style"]];if([seen containsObject:key])return NO;[seen addObject:key];}return YES;
}
NSDictionary *NXDefaultConfiguration(void) { return @{@"languages":@[], @"themes":@[], @"selectedTheme":@""}; }
NSDictionary *NXDefaultPreferences(void) {return @{@"font":@"JetBrains Mono",@"fontSize":@13,@"tabWidth":@4,@"useTabs":@NO,@"lineNumbers":@YES,@"whitespace":@NO,@"autoIndent":@YES,@"braceMatch":@YES,@"autoComplete":@NO,@"autoPairs":@NO,@"functionCompletion":@NO,@"parameterHints":@NO,@"nonPrinting":@NO,@"caretWidth":@1,@"blinkRate":@500,@"edgeColumn":@0,@"snapshotSeconds":@1,@"defaultEncoding":@0,@"defaultEOL":@(SC_EOL_LF),@"indentGuides":@YES,@"showEOL":@NO,@"highlightLine":@NO,@"wrap":@YES,@"dark":@YES};}
static BOOL Color(id value) {
    if (![value isKindOfClass:NSString.class] || [value length] != 6) return NO;
    return [value rangeOfCharacterFromSet:[NSCharacterSet characterSetWithCharactersInString:@"0123456789abcdefABCDEF"].invertedSet].location == NSNotFound;
}
static BOOL Record(id record, NSDictionary *model) {
    if (![record isKindOfClass:NSDictionary.class]) return NO;
    for (NSString *key in model) {
        if ([model[key] isKindOfClass:NSString.class]) {
            if (![record[key] isKindOfClass:NSString.class] || [record[key] length] > 65536) return NO;
        } else if (![record[key] isKindOfClass:NSNumber.class]) return NO;
    }
    return [record[@"name"] length] && [record[@"name"] length] <= 100 && [record[@"id"] length];
}
BOOL NXValidateConfiguration(id value) {
    if (![value isKindOfClass:NSDictionary.class] || ![value[@"selectedTheme"] isKindOfClass:NSString.class]) return NO;
    for (NSString *kind in @[@"languages", @"themes"]) {
        id records = value[kind]; if (![records isKindOfClass:NSArray.class] || [records count] > 100) return NO;
        NSMutableSet *ids = [NSMutableSet new], *names = [NSMutableSet new];
        for (id record in records) {
            if (!Record(record, [kind isEqual:@"languages"] ? NXDefaultLanguage() : NXDefaultTheme())) return NO;
            if ([ids containsObject:record[@"id"]] || [names containsObject:[record[@"name"] lowercaseString]]) return NO;
            [ids addObject:record[@"id"]]; [names addObject:[record[@"name"] lowercaseString]];
            if([kind isEqual:@"themes"]&&record[@"styles"]&&!NXValidateStyles(record[@"styles"]))return NO;
            if ([kind isEqual:@"themes"]) for (NSString *key in NXDefaultTheme()) {
                if (![@[@"id", @"name", @"boldKeywords", @"italicComments"] containsObject:key] && !Color(record[key])) return NO;
            }
            if([kind isEqual:@"languages"]&&!NXValidateAdvancedLanguage(record))return NO;
            if ([kind isEqual:@"languages"] && ([record[@"blockOpen"] length] == 0) != ([record[@"blockClose"] length] == 0)) return NO;
            if([kind isEqual:@"languages"]){for(NSString *key in @[@"lineComment",@"blockOpen",@"blockClose",@"foldOpen",@"foldClose"])if([record[key] length]>32)return NO;
                if([record[@"escape"] length]>1||![record[@"quotes"] canBeConvertedToEncoding:NSASCIIStringEncoding]||![record[@"escape"] canBeConvertedToEncoding:NSASCIIStringEncoding])return NO;
            }
        }
    }
    if(value[@"runCommands"]){if(![value[@"runCommands"] isKindOfClass:NSArray.class]||[value[@"runCommands"] count]>100)return NO;NSMutableSet *names=[NSMutableSet new];for(id command in value[@"runCommands"]){if(![command isKindOfClass:NSDictionary.class]||![command[@"name"] isKindOfClass:NSString.class]||![command[@"command"] isKindOfClass:NSString.class]||![command[@"name"] length]||[command[@"name"] length]>100||![command[@"command"] length]||[command[@"command"] length]>65536||[names containsObject:[command[@"name"] lowercaseString]])return NO;[names addObject:[command[@"name"] lowercaseString]];}}
    if ([value[@"selectedTheme"] length] && ![[value[@"themes"] valueForKey:@"id"] containsObject:value[@"selectedTheme"]]) return NO;
    return YES;
}
BOOL NXValidatePreferences(id value) {
    if (![value isKindOfClass:NSDictionary.class] || ![value[@"font"] isKindOfClass:NSString.class] || ![value[@"font"] length]) return NO;
    NSDictionary *ranges = @{@"fontSize":@[@6,@96], @"tabWidth":@[@1,@16], @"caretWidth":@[@1,@3], @"blinkRate":@[@0,@2000],
        @"edgeColumn":@[@0,@500], @"snapshotSeconds":@[@1,@60], @"defaultEncoding":@[@0,@3], @"defaultEOL":@[@0,@2]};
    for (NSString *key in ranges) if (value[key]) {
        if (![value[key] isKindOfClass:NSNumber.class] || !std::isfinite([value[key] doubleValue]) || [value[key] doubleValue] < [ranges[key][0] doubleValue] || [value[key] doubleValue] > [ranges[key][1] doubleValue]) return NO;
    }
    for (NSString *key in @[@"useTabs", @"lineNumbers", @"whitespace", @"autoIndent", @"braceMatch", @"autoComplete",@"autoPairs",@"functionCompletion",@"parameterHints",@"nonPrinting", @"indentGuides", @"showEOL", @"highlightLine", @"wrap", @"dark"])
        if (value[key] && ![value[key] isKindOfClass:NSNumber.class]) return NO;
    return value[@"fontSize"] && value[@"tabWidth"];
}
NSDictionary *NXLanguageForPath(NSDictionary *configuration, NSString *path) {
    for (NSDictionary *language in configuration[@"languages"]) {
        NSArray *extensions = [[language[@"extensions"] lowercaseString] componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@" ;,\t\n"]];
        if (path.pathExtension.length && [extensions containsObject:path.pathExtension.lowercaseString]) return language;
    }
    return nil;
}
static unsigned RGB(NSString *hex) {
    unsigned value = 0; [[NSScanner scannerWithString:hex] scanHexInt:&value];
    return ((value >> 16) & 255) | (value & 0xff00) | ((value & 255) << 16);
}
void NXApplyStyleOverrides(ScintillaView *view, NSArray *styles, NSString *lexer) {
    for(NSDictionary *style in styles)if([style[@"lexer"] isEqual:lexer]){auto index=[style[@"style"] integerValue];S(view,SCI_STYLESETFORE,index,RGB(style[@"foreground"]));S(view,SCI_STYLESETBACK,index,RGB(style[@"background"]));S(view,SCI_STYLESETFONT,index,reinterpret_cast<sptr_t>([style[@"font"] UTF8String]));S(view,SCI_STYLESETSIZEFRACTIONAL,index,lround([style[@"fontSize"] doubleValue]*100));S(view,SCI_STYLESETBOLD,index,[style[@"bold"] boolValue]);S(view,SCI_STYLESETITALIC,index,[style[@"italic"] boolValue]);S(view,SCI_STYLESETUNDERLINE,index,[style[@"underline"] boolValue]);}
}
void NXApplyTheme(ScintillaView *v, NSDictionary *theme, NSString *lexer, BOOL custom) {
    if (!theme) return;
    unsigned back = RGB(theme[@"background"]), fore = RGB(theme[@"foreground"]);
    S(v, SCI_STYLESETBACK, STYLE_DEFAULT, back); S(v, SCI_STYLESETFORE, STYLE_DEFAULT, fore); S(v, SCI_STYLECLEARALL);
    S(v, SCI_STYLESETBACK, STYLE_LINENUMBER, back); S(v, SCI_STYLESETFORE, STYLE_LINENUMBER, RGB(theme[@"lineNumbers"]));
    S(v, SCI_SETFOLDMARGINCOLOUR, 1, back); S(v, SCI_SETFOLDMARGINHICOLOUR, 1, back);
    S(v, SCI_SETCARETFORE, RGB(theme[@"caret"])); S(v, SCI_SETSELBACK, 1, RGB(theme[@"selection"]));
    S(v, SCI_SETCARETLINEBACK, RGB(theme[@"currentLine"]));
    if (custom) {
        for (unsigned i = 1; i <= 8; ++i) { S(v, SCI_STYLESETFORE, i, RGB(theme[[NSString stringWithFormat:@"keyword%u", i]])); S(v, SCI_STYLESETBOLD, i, [theme[@"boldKeywords"] boolValue]); }
        S(v, SCI_STYLESETFORE, 9, RGB(theme[@"string"]));
        for (unsigned i : {10u,11u}) { S(v, SCI_STYLESETFORE, i, RGB(theme[@"comment"])); S(v, SCI_STYLESETITALIC, i, [theme[@"italicComments"] boolValue]); }
        S(v, SCI_STYLESETFORE, 12, RGB(theme[@"number"])); S(v, SCI_STYLESETFORE, 13, RGB(theme[@"operator"]));
    } else {
        // Lexilla publishes the semantic name of each named style; do not assume
        // that the same numeric style means a keyword in every language.
        for (int i = 0, count = static_cast<int>(S(v, SCI_GETNAMEDSTYLES)); i < count && i < 256; ++i) {
            auto length=S(v,SCI_NAMEOFSTYLE,i,0);if(length<0||length>4096)continue;std::vector<char> name(length+1);S(v, SCI_NAMEOFSTYLE, i, reinterpret_cast<sptr_t>(name.data()));
            NSString *label = [@(name.data()) lowercaseString], *key = nil;
            if ([label containsString:@"comment"]) key = @"comment";
            else if ([label containsString:@"string"] || [label containsString:@"character"]) key = @"string";
            else if ([label containsString:@"number"]) key = @"number";
            else if ([label containsString:@"operator"]) key = @"operator";
            else if ([label containsString:@"keyword"] || [label containsString:@"word"]) key = @"keyword1";
            if (key) S(v, SCI_STYLESETFORE, i, RGB(theme[key]));
            if ([key isEqual:@"keyword1"]) S(v, SCI_STYLESETBOLD, i, [theme[@"boldKeywords"] boolValue]);
            if ([key isEqual:@"comment"]) S(v, SCI_STYLESETITALIC, i, [theme[@"italicComments"] boolValue]);
        }
    }
}
void NXStyleLanguage(ScintillaView *v, NSDictionary *language) {
    size_t length = S(v, SCI_GETLENGTH); if (length > 16 * 1024 * 1024) return;
    std::string text(reinterpret_cast<const char *>(S(v, SCI_GETCHARACTERPOINTER)), length);
    std::vector<unsigned char> styles(length, 0);
    std::unordered_set<std::string> keywords[8];
    bool sensitive = [language[@"caseSensitive"] boolValue];
    for (unsigned i=0;i<8;++i) for (NSString *word in [language[[NSString stringWithFormat:@"keywords%u",i+1]] componentsSeparatedByCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet])
        if (word.length) keywords[i].insert((sensitive ? word : word.lowercaseString).UTF8String);
    std::string line=[language[@"lineComment"] UTF8String], open=[language[@"blockOpen"] UTF8String], close=[language[@"blockClose"] UTF8String], quotes=[language[@"quotes"] UTF8String], escape=[language[@"escape"] UTF8String];
    auto at=[&](size_t p,const std::string &s){return !s.empty() && text.compare(p,s.size(),s)==0;};
    for(size_t p=0;p<length;) {
        size_t start=p; unsigned char style=0, c=static_cast<unsigned char>(text[p]);
        if(at(p,line)){style=10;while(p<length && text[p]!='\n' && text[p]!='\r')++p;}
        else if(at(p,open)){style=11;p+=open.size();auto end=text.find(close,p);p=end==std::string::npos?length:end+close.size();}
        else if(quotes.find(text[p])!=std::string::npos){style=9;char quote=text[p++];while(p<length){if(at(p,escape)){p=std::min(length,p+escape.size()+1);continue;}if(text[p++]==quote)break;}}
        else if(std::isdigit(c)){style=12;++p;while(p<length&&(std::isalnum(static_cast<unsigned char>(text[p]))||text[p]=='.'||text[p]=='_'))++p;}
        else if(std::isalpha(c)||c=='_'||c>=128){++p;while(p<length&&(std::isalnum(static_cast<unsigned char>(text[p]))||text[p]=='_'||static_cast<unsigned char>(text[p])>=128))++p;
            NSString *word=[[NSString alloc] initWithBytes:text.data()+start length:p-start encoding:NSUTF8StringEncoding];std::string key=(sensitive?word:word.lowercaseString).UTF8String ?: "";
            for(unsigned i=0;i<8;++i)if(keywords[i].contains(key)){style=i+1;break;}}
        else {if(!std::isspace(c))style=13;++p;}
        std::fill(styles.begin()+start,styles.begin()+p,style);
    }
    S(v,SCI_STARTSTYLING,0);S(v,SCI_SETSTYLINGEX,length,reinterpret_cast<sptr_t>(styles.data()));
    std::string foldOpen=[language[@"foldOpen"] UTF8String],foldClose=[language[@"foldClose"] UTF8String];
    int depth=SC_FOLDLEVELBASE, initial=depth; size_t row=0;
    for(size_t p=0;p<=length;++p){
        if(p==length||text[p]=='\n'){S(v,SCI_SETFOLDLEVEL,row++,initial|(depth>initial?SC_FOLDLEVELHEADERFLAG:0));initial=depth;}
        else if(styles[p]!=9&&styles[p]!=10&&styles[p]!=11){if(at(p,foldOpen))++depth;else if(at(p,foldClose))depth=std::max(SC_FOLDLEVELBASE,depth-1);}
    }
}

@implementation NXRecordEditor {
    NSString *_kind; NSMutableArray *_records; NSMutableDictionary *_controls; NSWindow *_window;
    NSTableView *_list; NSScrollView *_form; NSInteger _selected; BOOL _accepted;
}
- (instancetype)initWithKind:(NSString *)kind records:(NSArray *)records {
    self=[super init];if(self){_kind=kind;_records=[NSMutableArray new];for(NSDictionary *r in records)[_records addObject:[r mutableCopy]];_selected=-1;}return self;
}
- (NSInteger)numberOfRowsInTableView:(NSTableView *)table {return _records.count;}
- (NSView *)tableView:(NSTableView *)table viewForTableColumn:(NSTableColumn *)column row:(NSInteger)row {return [NSTextField labelWithString:_records[row][@"name"] ?: @"Editor Preferences"];}
- (void)stash {
    if(_selected<0||_selected>=(NSInteger)_records.count)return;
    NSMutableDictionary *record=_records[_selected];
    for(NSString *key in _controls){id control=_controls[key];
        if([control isKindOfClass:NSPopUpButton.class])record[key]=@([control indexOfSelectedItem]);
        else if([control isKindOfClass:NSButton.class])record[key]=@([control state]==NSControlStateValueOn);
        else if([control isKindOfClass:NSColorWell.class]){NSColor *c=[[control color] colorUsingColorSpace:NSColorSpace.sRGBColorSpace];record[key]=[NSString stringWithFormat:@"%02X%02X%02X",(int)lround(c.redComponent*255),(int)lround(c.greenComponent*255),(int)lround(c.blueComponent*255)];}
        else if([record[key] isKindOfClass:NSNumber.class])record[key]=@([control doubleValue]);
        else record[key]=[control stringValue];
    }
}
- (void)tableViewSelectionDidChange:(NSNotification *)note {[self stash];_selected=_list.selectedRow;[self showRecord];}
- (void)showRecord {
    _controls=[NSMutableDictionary new];NSView *form=[[NSView alloc]initWithFrame:NSMakeRect(0,0,520,100)];_form.documentView=form;
    if(_selected<0||_selected>=(NSInteger)_records.count)return;
    NSMutableDictionary *record=_records[_selected];if([_kind isEqual:@"preferences"]){for(NSString *key in NXDefaultPreferences())if(!record[key])record[key]=NXDefaultPreferences()[key];}if([_kind isEqual:@"languages"]){NSDictionary *advanced=NXAdvancedLanguageDefaults();for(NSString *key in advanced)if(!record[key])record[key]=advanced[key];}
    NSArray *keys;
    if([_kind isEqual:@"languages"])keys=@[@"name",@"extensions",@"caseSensitive",@"lineComment",@"blockOpen",@"blockClose",@"quotes",@"escape",@"foldOpen",@"foldClose",@"keywords1",@"keywords2",@"keywords3",@"keywords4",@"keywords5",@"keywords6",@"keywords7",@"keywords8"];
    else if([_kind isEqual:@"styles"])keys=@[@"name",@"foreground",@"background",@"font",@"fontSize",@"bold",@"italic",@"underline"];
    else if([_kind isEqual:@"themes"])keys=@[@"name",@"background",@"foreground",@"caret",@"selection",@"lineNumbers",@"currentLine",@"keyword1",@"keyword2",@"keyword3",@"keyword4",@"keyword5",@"keyword6",@"keyword7",@"keyword8",@"string",@"comment",@"number",@"operator",@"boldKeywords",@"italicComments"];
    else keys=@[@"tabWidth",@"useTabs",@"autoIndent",@"braceMatch",@"autoComplete",@"autoPairs",@"functionCompletion",@"parameterHints",@"nonPrinting",@"font",@"fontSize",@"lineNumbers",@"whitespace",@"indentGuides",@"showEOL",@"highlightLine",@"wrap",@"dark",@"caretWidth",@"blinkRate",@"edgeColumn",@"snapshotSeconds",@"defaultEncoding",@"defaultEOL"];
    if([_kind isEqual:@"languages"]){NSMutableArray *expanded=[keys mutableCopy];[expanded addObjectsFromArray:[[NXAdvancedLanguageDefaults() allKeys] sortedArrayUsingSelector:@selector(compare:)]];keys=expanded;}
    NSDictionary *labels=@{@"font":@"Default font family",@"fontSize":@"Font size (6–96)",@"tabWidth":@"Tab width (1–16)",@"useTabs":@"Indent with tabs",@"lineNumbers":@"Line numbers",@"autoIndent":@"Automatic indentation",@"braceMatch":@"Match braces",@"autoComplete":@"Word completion",@"autoPairs":@"Auto-close brackets and quotes",@"functionCompletion":@"Function completion while typing",@"parameterHints":@"Parameter hints while typing",@"nonPrinting":@"Show non-printing characters",@"indentGuides":@"Indent guides",@"showEOL":@"Show line endings",@"highlightLine":@"Highlight current line",@"wrap":@"Word wrap",@"dark":@"Dark appearance",@"caretWidth":@"Caret width (1–3)",@"blinkRate":@"Caret blink, ms (0–2000)",@"edgeColumn":@"Column guide (0 = off)",@"snapshotSeconds":@"Recovery interval, sec (1–60)",@"defaultEncoding":@"New files: 0 UTF8, 1 BOM, 2 LE, 3 BE",@"defaultEOL":@"New lines: 0 CRLF, 1 CR, 2 LF",@"extensions":@"Extensions (space separated)",@"lineComment":@"Line comment",@"blockOpen":@"Block comment start",@"blockClose":@"Block comment end",@"quotes":@"String quote characters",@"escape":@"Escape character",@"foldOpen":@"Fold opening token",@"foldClose":@"Fold closing token",@"caseSensitive":@"Case-sensitive keywords",@"boldKeywords":@"Bold keywords",@"italicComments":@"Italic comments"};
    BOOL preferences=[_kind isEqual:@"preferences"];
    NSMutableArray *rows=[NSMutableArray new];
    CGFloat height=keys.count*40+16;form.frame=NSMakeRect(0,0,540,height);
    NSUInteger index=0;for(NSString *key in keys){CGFloat y=height-40*(++index);NSTextField *label=[NSTextField labelWithString:labels[key] ?: [key capitalizedString]];label.frame=NSMakeRect(8,y,272,26);[form addSubview:label];id control;
        if([key isEqual:@"defaultEncoding"]||[key isEqual:@"defaultEOL"]){NSPopUpButton *choice=[[NSPopUpButton alloc]initWithFrame:NSZeroRect pullsDown:NO];[choice addItemsWithTitles:[key isEqual:@"defaultEncoding"]?@[@"UTF-8",@"UTF-8 with BOM",@"UTF-16 Little Endian",@"UTF-16 Big Endian"]:@[@"Windows (CRLF)",@"Classic Mac (CR)",@"Unix / macOS (LF)"]];[choice selectItemAtIndex:[record[key] integerValue]];control=choice;label.stringValue=[key isEqual:@"defaultEncoding"]?@"New document encoding":@"New document line endings";}
        else if([record[key] isKindOfClass:NSNumber.class]&&CFGetTypeID((__bridge CFTypeRef)record[key])==CFBooleanGetTypeID()){NSButton *check=[NSButton checkboxWithTitle:preferences?@"":@"Enabled" target:nil action:nil];check.state=[record[key] boolValue]?NSControlStateValueOn:NSControlStateValueOff;control=check;}
        else if(([_kind isEqual:@"themes"]||[_kind isEqual:@"styles"])&&Color(record[key])){NSColorWell *well=[NSColorWell new];unsigned n=RGB(record[key]);well.color=[NSColor colorWithSRGBRed:(n&255)/255.0 green:((n>>8)&255)/255.0 blue:((n>>16)&255)/255.0 alpha:1];control=well;}
        else {NSTextField *field=[NSTextField new];field.stringValue=[record[key] description] ?: @"";control=field;}
        [control setFrame:NSMakeRect(285,y,240,26)];[form addSubview:control];_controls[key]=control;
        if(preferences){
            NSDictionary *groups=@{@"tabWidth":@"Editing",@"font":@"Appearance",@"snapshotSeconds":@"Files & Recovery"};
            if(groups[key]){NSTextField *heading=[NSTextField labelWithString:groups[key]];heading.font=[NSFont boldSystemFontOfSize:13];[rows addObject:@[heading,NSGridCell.emptyContentView]];}
            [label removeFromSuperview];[control removeFromSuperview];[rows addObject:@[label,control]];
            [control setAccessibilityLabel:label.stringValue];
        }
    }
    if(preferences){
        form.translatesAutoresizingMaskIntoConstraints=NO;
        NSGridView *grid=[NSGridView gridViewWithViews:rows];grid.translatesAutoresizingMaskIntoConstraints=NO;grid.rowSpacing=8;grid.columnSpacing=16;
        [grid columnAtIndex:0].width=245;[grid columnAtIndex:1].xPlacement=NSGridCellPlacementFill;
        for(NSUInteger row=0;row<rows.count;++row){[grid rowAtIndex:row].yPlacement=NSGridCellPlacementCenter;[grid rowAtIndex:row].height=26;}
        [form addSubview:grid];
        [NSLayoutConstraint activateConstraints:@[[form.widthAnchor constraintEqualToAnchor:_form.contentView.widthAnchor],[grid.leadingAnchor constraintEqualToAnchor:form.leadingAnchor constant:8],[grid.trailingAnchor constraintEqualToAnchor:form.trailingAnchor constant:-12],[grid.topAnchor constraintEqualToAnchor:form.topAnchor constant:12],[grid.bottomAnchor constraintEqualToAnchor:form.bottomAnchor constant:-12]]];
        [form layoutSubtreeIfNeeded];height=form.frame.size.height;
    }
    [_form.contentView scrollToPoint:NSMakePoint(0,MAX(0,height-_form.contentView.bounds.size.height))];
}
- (void)add:(id)sender {[self stash];if([_kind isEqual:@"preferences"]){_records=[NSMutableArray arrayWithObject:[NXDefaultPreferences() mutableCopy]];_selected=0;[self showRecord];return;}if(_records.count>=100)return;NSMutableDictionary *r=[([_kind isEqual:@"languages"]?NXDefaultLanguage():NXDefaultTheme()) mutableCopy];r[@"name"]=[NSString stringWithFormat:@"My %@ %lu",[_kind isEqual:@"languages"]?@"Language":@"Theme",(unsigned long)_records.count+1];[_records addObject:r];[_list reloadData];[_list selectRowIndexes:[NSIndexSet indexSetWithIndex:_records.count-1] byExtendingSelection:NO];}
- (void)duplicate:(id)sender {[self stash];if(_selected<0||_records.count>=100)return;NSMutableDictionary *r=[_records[_selected] mutableCopy];r[@"id"]=NSUUID.UUID.UUIDString;r[@"name"]=[r[@"name"] stringByAppendingString:@" Copy"];[_records addObject:r];[_list reloadData];[_list selectRowIndexes:[NSIndexSet indexSetWithIndex:_records.count-1] byExtendingSelection:NO];}
- (void)remove:(id)sender {if(_selected<0)return;[_records removeObjectAtIndex:_selected];_selected=-1;[_list reloadData];[self showRecord];}
- (BOOL)valid {if([_kind isEqual:@"styles"])return NXValidateStyles(_records);if([_kind isEqual:@"preferences"])return _records.count==1&&NXValidatePreferences(_records[0]);NSMutableDictionary *config=[NXDefaultConfiguration() mutableCopy];config[_kind]=_records;return NXValidateConfiguration(config);}
- (void)accept:(id)sender {[self stash];if(![self valid]){[NSApp presentError:NXError(@"Check unique names, valid numeric ranges, and paired block-comment delimiters.")];return;}_accepted=YES;[NSApp stopModal];[_window orderOut:nil];}
- (void)cancel:(id)sender {[NSApp abortModal];[_window orderOut:nil];}
- (void)exportRecords:(id)sender {[self stash];if(![self valid]){[NSApp presentError:NXError(@"Correct the settings before exporting.")];return;}NSSavePanel *panel=[NSSavePanel savePanel];panel.nameFieldStringValue=[NSString stringWithFormat:@"NoteX-%@.json",_kind];if([panel runModal]!=NSModalResponseOK)return;NSError *error=nil;NSData *data=[NSJSONSerialization dataWithJSONObject:@{@"schema":@1,@"kind":_kind,@"records":_records} options:NSJSONWritingPrettyPrinted error:&error];if(!data||!NXWrite(data,panel.URL.path,&error))[NSApp presentError:error];}
- (void)importRecords:(id)sender {NSOpenPanel *panel=[NSOpenPanel openPanel];if([panel runModal]!=NSModalResponseOK)return;NSData *data=[NSData dataWithContentsOfURL:panel.URL];if(data.length>8*1024*1024){[NSApp presentError:NXError(@"The settings file exceeds 8 MB.")];return;}id root=data?[NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingMutableContainers error:nil]:nil;
    if(![root isKindOfClass:NSDictionary.class]||![root[@"schema"] isEqual:@1]||![root[@"kind"] isEqual:_kind]||![root[@"records"] isKindOfClass:NSArray.class]){[NSApp presentError:NXError(@"Choose a NoteX settings export of this type.")];return;}
    NSMutableArray *old=_records;_records=root[@"records"];if(![self valid]){_records=old;[NSApp presentError:NXError(@"The imported settings are invalid.")];return;}_selected=-1;[_list reloadData];if([_kind isEqual:@"preferences"]){_selected=0;[self showRecord];}else if(_records.count)[_list selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];else[self showRecord];
}
- (NSArray *)runForWindow:(NSWindow *)parent {
    BOOL preferences=[_kind isEqual:@"preferences"];
    _window=[[NSWindow alloc]initWithContentRect:NSMakeRect(0,0,preferences?580:800,600) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskResizable backing:NSBackingStoreBuffered defer:NO];_window.title=[_kind isEqual:@"preferences"]?@"Preferences":[_kind isEqual:@"languages"]?@"Language Editor":[_kind isEqual:@"styles"]?@"Syntax Style Editor":@"Theme Editor";_window.appearance=parent.appearance;
    _list=[[NSTableView alloc]initWithFrame:NSZeroRect];NSTableColumn *column=[[NSTableColumn alloc]initWithIdentifier:@"name"];column.width=210;[_list addTableColumn:column];_list.headerView=nil;_list.delegate=self;_list.dataSource=self;
    NSScrollView *left=[[NSScrollView alloc]initWithFrame:NSMakeRect(12,66,220,480)];left.documentView=_list;left.hasVerticalScroller=YES;if(!preferences)[_window.contentView addSubview:left];
    _form=[[NSScrollView alloc]initWithFrame:NSMakeRect(240,66,548,480)];_form.hasVerticalScroller=YES;_form.hasHorizontalScroller=!preferences;[_window.contentView addSubview:_form];
    NSTextField *help=[NSTextField wrappingLabelWithString:@"Edit a draft, then Apply to save. Cancel leaves your current settings unchanged. Import replaces this draft; Export saves a portable JSON settings file."];help.frame=NSMakeRect(12,552,776,42);[_window.contentView addSubview:help];
    NSArray *names=@[@"New",@"Duplicate",@"Delete",@"Import…",@"Export…",@"Cancel",@"Apply"];SEL actions[]={@selector(add:),@selector(duplicate:),@selector(remove:),@selector(importRecords:),@selector(exportRecords:),@selector(cancel:),@selector(accept:)};
    for(NSUInteger i=0;i<names.count;++i){NSButton *button=[NSButton buttonWithTitle:names[i] target:self action:actions[i]];button.frame=NSMakeRect(12+i*111,18,104,32);if([_kind isEqual:@"styles"]&&i<3)continue;if([_kind isEqual:@"preferences"]){if(i==0)button.title=@"Reset";if(i==1||i==2)continue;}if(i==6)button.keyEquivalent=@"\r";if(i==5)button.keyEquivalent=@"\e";if(preferences){NSUInteger slot=i==0?0:i-2;button.frame=NSMakeRect(16+slot*110,16,104,30);button.autoresizingMask=NSViewMaxYMargin;}
        [_window.contentView addSubview:button];}
    if(preferences){
        _window.minSize=NSMakeSize(560,420);_form.translatesAutoresizingMaskIntoConstraints=NO;help.translatesAutoresizingMaskIntoConstraints=NO;
        [NSLayoutConstraint activateConstraints:@[[_form.leadingAnchor constraintEqualToAnchor:_window.contentView.leadingAnchor constant:16],[_form.trailingAnchor constraintEqualToAnchor:_window.contentView.trailingAnchor constant:-16],[_form.topAnchor constraintEqualToAnchor:_window.contentView.topAnchor constant:58],[_form.bottomAnchor constraintEqualToAnchor:_window.contentView.bottomAnchor constant:-58],[help.leadingAnchor constraintEqualToAnchor:_form.leadingAnchor],[help.trailingAnchor constraintEqualToAnchor:_form.trailingAnchor],[help.topAnchor constraintEqualToAnchor:_window.contentView.topAnchor constant:12],[help.heightAnchor constraintEqualToConstant:38]]];
    }
    [_list reloadData];if(_records.count)[_list selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];[_window center];[NSApp runModalForWindow:_window];return _accepted?[_records copy]:nil;
}
@end
