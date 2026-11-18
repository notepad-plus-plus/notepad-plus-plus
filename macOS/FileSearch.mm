// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Searches are read-only; replacement plans are committed by the editor's save path.
#import "FileSearch.h"
#import "Storage.h"
#import "Codecs.h"
#include <fnmatch.h>

NSString *NXDecodeText(NSData *data) {
    if(!data)return nil;NSStringEncoding encoding=NSUTF8StringEncoding;NSUInteger skip=0;
    const unsigned char *b=static_cast<const unsigned char *>(data.bytes);
    if(data.length>=3&&!memcmp(b,"\xef\xbb\xbf",3))skip=3;
    else if(data.length>=2&&b[0]==0xff&&b[1]==0xfe){skip=2;encoding=NSUTF16LittleEndianStringEncoding;}
    else if(data.length>=2&&b[0]==0xfe&&b[1]==0xff){skip=2;encoding=NSUTF16BigEndianStringEncoding;}
    NSString *text=[[NSString alloc]initWithData:[data subdataWithRange:NSMakeRange(skip,data.length-skip)] encoding:encoding];
    return text&&[text rangeOfString:[NSString stringWithFormat:@"%C",(unichar)0]].location==NSNotFound?text:nil;
}
NSArray *NXSearchMatches(NSString *text, NSDictionary *options, NSError **error) {
    NSString *query=options[@"query"];
    if(!query.length){if(error)*error=NXError(@"Enter text to find.");return nil;}
    if([options[@"regex"] boolValue]&&[options[@"engine"] isEqual:@"boost"])return NXBoostMatches(text,options,error);
    NSString *pattern=[options[@"regex"] boolValue]?query:[NSRegularExpression escapedPatternForString:query];
    if([options[@"wholeWord"] boolValue])pattern=[NSString stringWithFormat:@"(?<![\\p{L}\\p{N}_])(?:%@)(?![\\p{L}\\p{N}_])",pattern];
    NSRegularExpressionOptions flags=[options[@"matchCase"] boolValue]?0:NSRegularExpressionCaseInsensitive;
    NSRegularExpression *regex=[NSRegularExpression regularExpressionWithPattern:pattern options:flags|NSRegularExpressionAnchorsMatchLines error:error];if(!regex)return nil;
    NSMutableArray *results=[NSMutableArray new];
    __block NSUInteger cursor=0,bytePosition=0,line=1;__block BOOL timedOut=NO;NSTimeInterval deadline=NSProcessInfo.processInfo.systemUptime+2;
    @try {
    [regex enumerateMatchesInString:text options:NSMatchingReportProgress range:NSMakeRange(0,text.length) usingBlock:^(NSTextCheckingResult *match,NSMatchingFlags flags,BOOL *stop){
        if(NSProcessInfo.processInfo.systemUptime>deadline){timedOut=YES;*stop=YES;return;}
        if(!match)return;
        NSRange r=match.range;
        bytePosition+=[[text substringWithRange:NSMakeRange(cursor,r.location-cursor)] lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
        for(NSUInteger i=cursor;i<r.location;++i)if([text characterAtIndex:i]=='\n'||([text characterAtIndex:i]=='\r'&&(i+1==text.length||[text characterAtIndex:i+1]!='\n')))++line;
        cursor=r.location;NSUInteger position=bytePosition;
        NSUInteger length=[[text substringWithRange:r] lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
        NSString *replacement=[options[@"regex"] boolValue]?[regex replacementStringForResult:match inString:text offset:0 template:options[@"replacement"] ?: @""]:options[@"replacement"] ?: @"";
        NSRange lineRange=[text lineRangeForRange:NSMakeRange(r.location,0)];NSString *excerpt=[[text substringWithRange:lineRange] stringByTrimmingCharactersInSet:NSCharacterSet.newlineCharacterSet];
        if(excerpt.length>240)excerpt=[[excerpt substringToIndex:240] stringByAppendingString:@"…"];
        [results addObject:@{@"position":@(position),@"length":@(length),@"line":@(line),@"excerpt":excerpt,@"replacement":replacement}];
        if(results.count>=20000)*stop=YES;
    }];
    } @catch(NSException *exception){if(error)*error=NXError(@"The regular expression or replacement template could not be evaluated.");return nil;}
    if(timedOut){if(error)*error=NXError(@"This search exceeded the per-document time limit. Simplify the expression or narrow the search.");return nil;}return results;
}
static BOOL MatchesFilter(NSString *path, NSString *patterns, BOOL fallback) {
    if(!patterns.length)return fallback;
    for(NSString *pattern in [patterns componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@";\n"]]) {
        NSString *p=[pattern stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet];
        if(p.length&&(fnmatch(p.UTF8String,path.lastPathComponent.UTF8String,0)==0||fnmatch(p.UTF8String,path.UTF8String,0)==0))return YES;
    }return NO;
}
NSArray *NXSearchFiles(NSArray *inputs, NSDictionary *options, BOOL (^cancelled)(void), NSError **error) {
    if(!NXSearchMatches(@"",options,error))return nil;
    NSMutableArray *files=[NSMutableArray new],*results=[NSMutableArray new];NSMutableSet *seen=[NSMutableSet new];
    for(id input in inputs){
        if(cancelled&&cancelled())return @[];
        if([input isKindOfClass:NSDictionary.class]){[files addObject:input];continue;}
        NSString *path=input;if([[[NSFileManager.defaultManager attributesOfItemAtPath:path error:nil] objectForKey:NSFileType] isEqual:NSFileTypeSymbolicLink])continue;BOOL directory=NO;[NSFileManager.defaultManager fileExistsAtPath:path isDirectory:&directory];
        if(directory){
            NSDirectoryEnumerationOptions flags=NSDirectoryEnumerationSkipsPackageDescendants;
            if(![options[@"hidden"] boolValue])flags|=NSDirectoryEnumerationSkipsHiddenFiles;
            if(![options[@"recursive"] boolValue])flags|=NSDirectoryEnumerationSkipsSubdirectoryDescendants;
            NSDirectoryEnumerator *walker=[NSFileManager.defaultManager enumeratorAtURL:[NSURL fileURLWithPath:path] includingPropertiesForKeys:@[NSURLIsDirectoryKey,NSURLIsSymbolicLinkKey,NSURLFileSizeKey] options:flags errorHandler:^BOOL(NSURL *url,NSError *failure){return YES;}];
            for(NSURL *url in walker){
                if(cancelled&&cancelled())return @[];
                NSNumber *folder=nil,*link=nil;[url getResourceValue:&folder forKey:NSURLIsDirectoryKey error:nil];[url getResourceValue:&link forKey:NSURLIsSymbolicLinkKey error:nil];
                if(link.boolValue||MatchesFilter(url.path,options[@"exclude"],NO)||[url.lastPathComponent isEqual:@".git"]){if(folder.boolValue)[walker skipDescendants];continue;}
                if(!folder.boolValue&&MatchesFilter(url.path,options[@"include"],YES))[files addObject:url.path];
                if(files.count>10000){if(error)*error=NXError(@"More than 10,000 files. Narrow the folder or file filters.");return nil;}
            }
        }else [files addObject:path];
    }
    if(files.count>10000){if(error)*error=NXError(@"More than 10,000 files. Narrow the search scope.");return nil;}
    for(id input in files){@autoreleasepool{
        if(cancelled&&cancelled())return @[];
        NSDictionary *document=[input isKindOfClass:NSDictionary.class]?input:nil;NSString *path=document?document[@"path"]:input;
        NSString *identity=document?document[@"id"]:NXCanonicalPath(path);
        if([seen containsObject:identity])continue;[seen addObject:identity];
        if(!MatchesFilter(path.length?path:document[@"title"],options[@"include"],YES)||MatchesFilter(path,options[@"exclude"],NO)||[path.pathExtension isEqual:@"notex"])continue;
        if(!document){NSDictionary *attributes=[NSFileManager.defaultManager attributesOfItemAtPath:path error:nil];if([attributes[NSFileType] isEqual:NSFileTypeSymbolicLink]||[attributes[NSFileSize] unsignedLongLongValue]>16*1024*1024)continue;}
        NSData *disk=document?document[@"text"]:[NSData dataWithContentsOfFile:path];
        if(disk.length>16*1024*1024)continue;
        NSString *text=document?[[NSString alloc]initWithData:disk encoding:NSUTF8StringEncoding]:NXDecodeText(disk);NSStringEncoding fallback=0;if(!text&&!document&&[options[@"encoding"] unsignedLongLongValue]){fallback=[options[@"encoding"] unsignedLongLongValue];text=NXDecodeEncoding(disk,fallback);}if(!text||[text rangeOfString:[NSString stringWithFormat:@"%C",(unichar)0]].location!=NSNotFound)continue;
        NSArray *matches=NXSearchMatches(text,options,error);if(!matches)return nil;
        NSData *textHash=NXDigest([text dataUsingEncoding:NSUTF8StringEncoding]),*diskHash=NXDigest(disk);
        for(NSDictionary *match in matches){NSMutableDictionary *r=[match mutableCopy];r[@"path"]=path ?: @"";r[@"id"]=document?document[@"id"]:@"";r[@"title"]=document?document[@"title"]:path.lastPathComponent;r[@"textHash"]=textHash;r[@"diskHash"]=diskHash;r[@"encoding"]=@(fallback);[results addObject:r];
            if(results.count>=20000){if(error)*error=NXError(@"The search reached 20,000 matches. Narrow the search before replacing.");return nil;}}
    }}return results;
}

@implementation NXFileSearch {
    NSTextField *_query,*_replacement,*_folder,*_include,*_exclude,*_status;NSPopUpButton *_scope,*_engine,*_encoding;
    NSButton *_regex,*_case,*_word,*_recursive,*_hidden,*_find,*_apply;
    NSTableView *_table;NSArray *_results;NSOperationQueue *_queue;NSUInteger _generation;BOOL _busy;
}
- (void)showMatches:(NSArray *)matches document:(NSDictionary *)document parent:(NSWindow *)parent {
    [self showForWindow:parent];[self cancelSearch:nil];NSMutableArray *results=[NSMutableArray new];
    for(NSDictionary *match in matches){NSMutableDictionary *row=[match mutableCopy];row[@"id"]=document[@"id"];row[@"path"]=document[@"path"];row[@"title"]=document[@"title"];row[@"textHash"]=NXDigest(document[@"text"]);row[@"diskHash"]=NSData.data;[results addObject:row];}
    _results=results;[_table reloadData];_apply.enabled=results.count>0&&results.count<20000;_status.stringValue=[NSString stringWithFormat:@"%lu matches in the captured document scope. Double-click to navigate.",(unsigned long)results.count];
}
- (void)navigateBy:(NSInteger)direction {if(!_results.count)return;NSInteger row=(_table.selectedRow+direction+_results.count)%_results.count;[_table selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];[_table scrollRowToVisible:row];if(self.navigate)self.navigate(_results[row]);}
- (instancetype)init {self=[super initWithWindow:nil];if(self){_queue=[NSOperationQueue new];_queue.maxConcurrentOperationCount=1;_results=@[];}return self;}
- (NSInteger)numberOfRowsInTableView:(NSTableView *)table{return _results.count;}
- (NSView *)tableView:(NSTableView *)table viewForTableColumn:(NSTableColumn *)column row:(NSInteger)row {NSDictionary *r=_results[row];NSString *value=[column.identifier isEqual:@"file"]?([r[@"path"] length]?r[@"path"]:r[@"title"]):[r[column.identifier] description];NSTextField *text=[NSTextField labelWithString:value ?: @""];text.toolTip=value;return text;}
- (NSTextField *)field:(NSString *)placeholder value:(NSString *)value y:(CGFloat)y {NSTextField *field=[[NSTextField alloc]initWithFrame:NSMakeRect(130,y,710,26)];field.placeholderString=placeholder;field.stringValue=value;[self.window.contentView addSubview:field];NSTextField *label=[NSTextField labelWithString:placeholder];label.frame=NSMakeRect(16,y,110,26);[self.window.contentView addSubview:label];return field;}
- (void)showForWindow:(NSWindow *)parent {
    if(!self.window){self.window=[[NSWindow alloc]initWithContentRect:NSMakeRect(0,0,920,650) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:NO];self.window.releasedWhenClosed=NO;self.window.title=@"Find in Files";
        _query=[self field:@"Find" value:@"" y:604];_replacement=[self field:@"Replace with" value:@"" y:570];_folder=[self field:@"Folder" value:@"" y:536];_include=[self field:@"File filters" value:@"*" y:502];_exclude=[self field:@"Exclude" value:@".git;*.notex;*.app" y:468];
        NSButton *browse=[NSButton buttonWithTitle:@"Choose…" target:self action:@selector(browse:)];_folder.frame=NSMakeRect(130,536,680,26);browse.frame=NSMakeRect(814,536,100,26);[self.window.contentView addSubview:browse];
        _scope=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(16,426,180,28) pullsDown:NO];[_scope addItemsWithTitles:@[@"Folder",@"Open Documents",@"All Project Panels",@"Current Document"]];[self.window.contentView addSubview:_scope];
        NSArray *labels=@[@"Regex",@"Match case",@"Whole word",@"Subfolders",@"Hidden files"];NSMutableArray *checks=[NSMutableArray new];
        for(NSUInteger i=0;i<labels.count;++i){NSButton *b=[NSButton checkboxWithTitle:labels[i] target:nil action:nil];b.frame=NSMakeRect(210+i*137,426,135,28);[self.window.contentView addSubview:b];[checks addObject:b];}
        _regex=checks[0];_case=checks[1];_word=checks[2];_recursive=checks[3];_hidden=checks[4];_recursive.state=NSControlStateValueOn;
        _find=[NSButton buttonWithTitle:@"Find / Preview" target:self action:@selector(search:)];_find.frame=NSMakeRect(16,382,150,30);[self.window.contentView addSubview:_find];
        NSButton *cancel=[NSButton buttonWithTitle:@"Cancel Search" target:self action:@selector(cancelSearch:)];cancel.frame=NSMakeRect(178,382,140,30);[self.window.contentView addSubview:cancel];
        _apply=[NSButton buttonWithTitle:@"Apply Previewed Replacements…" target:self action:@selector(replace:)];_apply.frame=NSMakeRect(330,382,290,30);_apply.enabled=NO;[self.window.contentView addSubview:_apply];
        _engine=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(16,30,280,24) pullsDown:NO];[_engine addItemsWithTitles:@[@"ICU regular expressions",@"Boost.Regex (Windows-compatible)"]];[self.window.contentView addSubview:_engine];
        _encoding=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(16,4,280,24) pullsDown:NO];[_encoding addItemWithTitle:@"UTF-8 / UTF-16 (Automatic)"];for(NSDictionary *encoding in NXSingleByteEncodings()){[_encoding addItemWithTitle:[@"Fallback: " stringByAppendingString:encoding[@"name"]]];_encoding.lastItem.representedObject=encoding[@"encoding"];}[self.window.contentView addSubview:_encoding];
        NSButton *backups=[NSButton buttonWithTitle:@"Open Backups" target:self action:@selector(backups:)];backups.frame=NSMakeRect(640,382,160,30);[self.window.contentView addSubview:backups];
        NSScrollView *scroll=[[NSScrollView alloc]initWithFrame:NSMakeRect(16,60,888,308)];scroll.hasVerticalScroller=YES;scroll.hasHorizontalScroller=YES;_table=[[NSTableView alloc]initWithFrame:scroll.bounds];
        for(NSArray *entry in @[@[@"file",@"File",@350],@[@"line",@"Line",@55],@[@"excerpt",@"Matching Text",@400],@[@"replacement",@"Replacement",@240]]){NSTableColumn *c=[[NSTableColumn alloc]initWithIdentifier:entry[0]];c.title=entry[1];c.width=[entry[2] doubleValue];[_table addTableColumn:c];}
        _table.delegate=self;_table.dataSource=self;_table.target=self;_table.doubleAction=@selector(activate:);scroll.documentView=_table;[self.window.contentView addSubview:scroll];
        _status=[NSTextField wrappingLabelWithString:@"Filters use semicolon-separated wildcards. Double-click a match to open it. Preview captures the replacement text; edit and search again to change the plan."];_status.frame=NSMakeRect(310,10,590,44);[self.window.contentView addSubview:_status];[self.window center];
    }self.window.appearance=parent.appearance;[self showWindow:nil];[self.window makeKeyAndOrderFront:nil];
}
- (void)browse:(id)sender {NSOpenPanel *panel=[NSOpenPanel openPanel];panel.canChooseFiles=NO;panel.canChooseDirectories=YES;if([panel runModal]==NSModalResponseOK)_folder.stringValue=panel.URL.path;}
- (void)backups:(id)sender {if(self.showBackups)self.showBackups();}
- (void)cancelSearch:(id)sender {++_generation;[_queue cancelAllOperations];_busy=NO;_find.enabled=YES;_apply.enabled=NO;_status.stringValue=@"Search canceled. No files were changed.";}
- (void)search:(id)sender {
    [self cancelSearch:nil];_results=@[];[_table reloadData];
    NSDictionary *options=@{@"encoding":_encoding.selectedItem.representedObject ?: @0,@"engine":_engine.indexOfSelectedItem?@"boost":@"icu",@"query":_query.stringValue,@"replacement":_replacement.stringValue,@"include":_include.stringValue,@"exclude":_exclude.stringValue,@"regex":@(_regex.state==NSControlStateValueOn),@"matchCase":@(_case.state==NSControlStateValueOn),@"wholeWord":@(_word.state==NSControlStateValueOn),@"recursive":@(_recursive.state==NSControlStateValueOn),@"hidden":@(_hidden.state==NSControlStateValueOn)};
    NSArray *inputs;
    if(_scope.indexOfSelectedItem==0){BOOL directory=NO;if(![NSFileManager.defaultManager fileExistsAtPath:_folder.stringValue isDirectory:&directory]||!directory){_status.stringValue=@"Choose an existing folder.";return;}inputs=@[_folder.stringValue];}
    else inputs=self.scopeFiles?self.scopeFiles(_scope.indexOfSelectedItem):@[];
    _busy=YES;_find.enabled=NO;_status.stringValue=@"Searching… UTF-8 and BOM-marked UTF-16 text up to 16 MB per file; symbolic links and app packages are skipped.";
    NSUInteger generation=_generation;__weak NXFileSearch *weak=self;NSBlockOperation *operation=[NSBlockOperation new];__weak NSBlockOperation *weakOperation=operation;
    [operation addExecutionBlock:^{NSError *error=nil;NSArray *results=NXSearchFiles(inputs,options,^BOOL{return weakOperation.cancelled;},&error);
        [NSOperationQueue.mainQueue addOperationWithBlock:^{NXFileSearch *owner=weak;if(!owner||owner->_generation!=generation)return;owner->_busy=NO;owner->_find.enabled=YES;owner->_results=results ?: @[];owner->_apply.enabled=!error&&results.count>0;[owner->_table reloadData];owner->_status.stringValue=error?error.localizedDescription:[NSString stringWithFormat:@"%lu matches. Double-click to navigate. Replacement uses the preview shown here; changed files are not overwritten.",(unsigned long)results.count];}];}];[_queue addOperation:operation];
}
- (void)activate:(id)sender {NSInteger row=_table.clickedRow>=0?_table.clickedRow:_table.selectedRow;if(row>=0&&row<(NSInteger)_results.count&&self.navigate)self.navigate(_results[row]);}
- (void)replace:(id)sender {
    if(_busy||!_results.count)return;NSAlert *alert=[NSAlert new];alert.messageText=@"Apply these previewed replacements?";alert.informativeText=@"Folder and project files are backed up before saving; changed files and dirty tabs are skipped. Open Documents results are edited in memory with undo and are not saved to disk. Review the result report if any files are skipped.";
    [alert addButtonWithTitle:@"Cancel"];[alert addButtonWithTitle:@"Apply Replacements"];if([alert runModal]!=NSAlertSecondButtonReturn)return;
    if(self.replaceResults)_status.stringValue=self.replaceResults(_results);_apply.enabled=NO;
}
@end
