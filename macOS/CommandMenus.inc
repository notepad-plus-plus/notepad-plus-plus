// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native command expansion, added 2026-11-01.
@implementation NXApp (CommandMenus)
- (void)buildCommandMenus {
    NSMenu *root=NSApp.mainMenu,*file=[root itemWithTitle:@"File"].submenu,*edit=[root itemWithTitle:@"Edit"].submenu,*view=[root itemWithTitle:@"View"].submenu;
    NSArray *files=@[@"Save a Copy As…",@"Rename File…",@"Reveal in Finder",@"Open with Default Application",@"Open Terminal Here",@"Copy Full Path",@"Copy Filename",@"Copy Directory",@"Copy All Filenames",@"Copy All Paths"];
    for(NSUInteger i=0;i<files.count;++i)[self item:file title:files[i] action:@selector(fileCommand:) key:@"" tag:i];
    NSArray *close=@[@"Close All Unpinned Tabs…",@"Close Other Tabs…",@"Close Tabs to the Left…",@"Close Tabs to the Right…",@"Close Unchanged Tabs"];
    for(NSUInteger i=0;i<close.count;++i)[self item:file title:close[i] action:@selector(closeDocuments:) key:@"" tag:i];
    [self item:file title:@"Recent Documents…" action:@selector(recentDocument:) key:@"" tag:0];
    [self item:file title:@"Reopen Closed File" action:@selector(recentDocument:) key:@"T" tag:1];
    [self item:file title:@"Read-Only Document" action:@selector(readOnly:) key:@"" tag:0];
    [self item:file title:@"Read-Only All Documents" action:@selector(readOnly:) key:@"" tag:1];
    NSArray *tabs=@[@"First Tab",@"Last Tab",@"Move Tab Left",@"Move Tab Right",@"Pin / Unpin Tab"];
    for(NSUInteger i=0;i<tabs.count;++i)[self item:view title:tabs[i] action:@selector(tabCommand:) key:@"" tag:i];
    [self item:view title:@"Open Documents…" action:@selector(chooseOpenDocument:) key:@"" tag:0];
    NSMenu *lines=[self menu:@"Line Operations" parent:edit];
    NSArray *commands=@[@[@"Indent",@(SCI_TAB)],@[@"Outdent",@(SCI_BACKTAB)],@[@"Move Lines Up",@(SCI_MOVESELECTEDLINESUP)],@[@"Move Lines Down",@(SCI_MOVESELECTEDLINESDOWN)],@[@"Copy Line",@(SCI_LINECOPY)],@[@"Cut Line",@(SCI_LINECUT)],@[@"Transpose Line",@(SCI_LINETRANSPOSE)],@[@"Duplicate Selection",@(SCI_SELECTIONDUPLICATE)],@[@"Delete to Line Start",@(SCI_DELLINELEFT)],@[@"Delete to Line End",@(SCI_DELLINERIGHT)]];
    for(NSArray *c in commands)[self item:lines title:c[0] action:@selector(editorCommand:) key:@"" tag:[c[1] integerValue]];
    NSArray *transforms=@[@"Trim Leading Whitespace",@"Trim Leading and Trailing Whitespace",@"Remove Empty Lines",@"Remove Blank Lines",@"Reverse Line Order",@"Sort Lexicographically",@"Sort Ignoring Case",@"Sort Naturally",@"Sort Descending",@"Remove Consecutive Duplicate Lines",@"Trim and Join Lines",@"Tabs to Spaces",@"Leading Spaces to Tabs"];
    for(NSUInteger i=0;i<transforms.count;++i)[self item:lines title:transforms[i] action:@selector(lineOperation:) key:@"" tag:i];
    NSArray *moreLines=@[@"Randomize Line Order",@"Sort Integers Ascending",@"Sort Integers Descending",@"Sort Decimal Dot Ascending",@"Sort Decimal Dot Descending",@"Sort Decimal Comma Ascending",@"Sort Decimal Comma Descending",@"Sort by Length Ascending",@"Sort by Length Descending",@"Sort by Locale Ascending",@"Sort by Locale Descending",@"Sort Ignoring Case Descending",@"All Spaces to Tabs",@"Line Endings to Spaces",@"Insert Blank Line Above",@"Insert Blank Line Below",@"Split Lines at Column Guide"];
    for(NSUInteger i=0;i<moreLines.count;++i)[self item:lines title:moreLines[i] action:@selector(lineOperation:) key:@"" tag:13+i];
    NSMenu *selection=[self menu:@"Selection" parent:edit];
    for(NSArray *c in @[@[@"Add Next Occurrence",@(SCI_MULTIPLESELECTADDNEXT)],@[@"Select All Occurrences",@(SCI_MULTIPLESELECTADDEACH)],@[@"Rotate Main Selection",@(SCI_ROTATESELECTION)],@[@"Swap Anchor and Caret",@(SCI_SWAPMAINANCHORCARET)],@[@"Cancel Additional Selections",@(SCI_CANCEL)]])[self item:selection title:c[0] action:@selector(editorCommand:) key:@"" tag:[c[1] integerValue]];
    [self item:selection title:@"Rectangular Selection" action:@selector(selectionMode:) key:@"" tag:SC_SEL_RECTANGLE];[self item:selection title:@"Normal Selection" action:@selector(selectionMode:) key:@"" tag:SC_SEL_STREAM];
    NSMenu *cases=[self menu:@"Convert Case" parent:edit];NSArray *names=@[@"Title Case",@"Sentence Case",@"Invert Case",@"Random Case",@"Title Case (Preserve Existing Case)",@"Sentence Case (Preserve Existing Case)"];
    for(NSUInteger i=0;i<names.count;++i)[self item:cases title:names[i] action:@selector(caseOperation:) key:@"" tag:i];
    [self item:edit title:@"Toggle Line Comment" action:@selector(commentOperation:) key:@"/" tag:0];
    [self item:edit title:@"Block Comment Selection" action:@selector(commentOperation:) key:@"" tag:1];
    [self item:edit title:@"Add Line Comments" action:@selector(commentOperation:) key:@"" tag:2];[self item:edit title:@"Remove Line Comments" action:@selector(commentOperation:) key:@"" tag:3];[self item:edit title:@"Add Block Comment" action:@selector(commentOperation:) key:@"" tag:4];[self item:edit title:@"Remove Block Comment" action:@selector(commentOperation:) key:@"" tag:5];
    [self item:edit title:@"Insert Date and Time" action:@selector(insertDate:) key:@"" tag:0];
    [self item:edit title:@"Insert Custom Date and Time…" action:@selector(insertDate:) key:@"" tag:1];
    [self item:edit title:@"Column Editor…" action:@selector(columnEditor:) key:@"" tag:0];
    [self item:edit title:@"Character Viewer" action:@selector(characterViewer:) key:@"" tag:0];
    [self item:view title:@"Full Screen" action:@selector(viewCommand:) key:@"" tag:0];
    [self item:view title:@"Document Map" action:@selector(viewCommand:) key:@"" tag:4];
    [self item:view title:@"Document Summary" action:@selector(viewCommand:) key:@"" tag:1];
    [self item:view title:@"Left-to-Right Text" action:@selector(viewCommand:) key:@"" tag:2];
    [self item:view title:@"Right-to-Left Text" action:@selector(viewCommand:) key:@"" tag:3];
    NSMenu *fold=[self menu:@"Fold Level" parent:view];for(NSInteger i=1;i<=8;++i)[self item:fold title:[NSString stringWithFormat:@"Level %ld",(long)i] action:@selector(foldLevel:) key:@"" tag:i];
    NSMenu *bookmarks=[root itemWithTitle:@"Bookmarks"].submenu;NSArray *bookmarkNames=@[@"Copy Bookmarked Lines",@"Cut Bookmarked Lines",@"Remove Bookmarked Lines",@"Remove Unbookmarked Lines",@"Invert Bookmarks"];
    for(NSUInteger i=0;i<bookmarkNames.count;++i)[self item:bookmarks title:bookmarkNames[i] action:@selector(bookmarkLines:) key:@"" tag:i];
    NSMenu *tools=[self menu:@"Tools" parent:root];[self item:tools title:@"Run Command…" action:@selector(runToolCommand:) key:@"" tag:0];for(NSString *algorithm in @[@"MD5",@"SHA-1",@"SHA-256",@"SHA-512"]){NSMenu *hash=[self menu:algorithm parent:tools];NSInteger type=[@[@"MD5",@"SHA-1",@"SHA-256",@"SHA-512"] indexOfObject:algorithm];[self item:hash title:@"Hash Text…" action:@selector(hashCommand:) key:@"" tag:type+8];[self item:hash title:@"Hash Selection to Clipboard" action:@selector(hashCommand:) key:@"" tag:type];[self item:hash title:@"Hash File…" action:@selector(hashCommand:) key:@"" tag:type+4];}
}
@end
