#import "Update.h"
#include "ColumnNumbers.h"
// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native NoteQu application, created 2026-11-01.
#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>
#import "ScintillaView.h"
#import "Storage.h"
#import "MenuLayout.h"
#import "Workspace.h"
#import "FunctionIndex.h"
#import "Configuration.h"
#import "FileSearch.h"
#include "ILexer.h"
#include "Lexilla.h"
#include "SciLexer.h"
#include <cstdio>
#include <cmath>
#include "ListEditing.h"
#include "NumericSort.h"
#import "NativeEditor.h"
#import "TabView.h"
#import "Chooser.h"
#import "DocumentMap.h"
#import "RunTool.h"
#import "UserLanguage.h"
#import "Codecs.h"
#import <CommonCrypto/CommonDigest.h>

static sptr_t Send(ScintillaView *view, unsigned message, uptr_t w = 0, sptr_t l = 0) {
    return [view message:message wParam:w lParam:l];
}
static NSData *Text(ScintillaView *view) {
    auto size = Send(view, SCI_GETLENGTH);
    return [NSData dataWithBytes:reinterpret_cast<const void *>(Send(view, SCI_GETCHARACTERPOINTER)) length:size];
}
static void SetText(ScintillaView *view, NSData *data) {
    Send(view, SCI_CLEARALL);
    Send(view, SCI_ADDTEXT, data.length, reinterpret_cast<sptr_t>(data.bytes));
    Send(view, SCI_EMPTYUNDOBUFFER);
    Send(view, SCI_SETSAVEPOINT);
}
static NoteQu::State State(ScintillaView *view) {
    NoteQu::State state;
    Send(view, NoteQu::GetState, 0, reinterpret_cast<sptr_t>(&state));
    return state;
}
static void Restore(ScintillaView *view, NoteQu::State state) {
    NoteQu::Restore restore{std::move(state), false};
    Send(view, NoteQu::SetState, 0, reinterpret_cast<sptr_t>(&restore));
}
static void Alert(NSString *message) {
    NSAlert *alert = [NSAlert new];
    alert.messageText = @"NoteQu";
    alert.informativeText = message ?: @"The operation could not be completed.";
    [alert runModal];
}

@interface NXDocument : NSObject
@property(nonatomic) ScintillaView *editor;
@property ScintillaView *activeEditor;
@property(nonatomic, readonly) ScintillaView *primaryEditor;
@property NSString *customLanguage;
@property NSString *builtinLanguage;
@property NSTabViewItem *tab;
@property NSString *identifier;
@property NSString *path;
@property NSString *title;
@property NSString *lexer;
@property NSData *diskHash;
@property NSStringEncoding encoding;
@property BOOL bom;
@property BOOL restoredDirty;
@property BOOL pinned;
@property NSInteger tabColor;
@property BOOL monitoring;
@property BOOL secondaryOnly;
- (BOOL)dirty;
- (NSDictionary *)record;
@end
@implementation NXDocument
@synthesize editor = _editor;
- (ScintillaView *)editor { return self.activeEditor ?: _editor; }
- (ScintillaView *)primaryEditor { return _editor; }
- (BOOL)dirty { return self.restoredDirty || Send(self.editor, SCI_GETMODIFY); }
- (NSDictionary *)record {
    NSData *text = Text(self.primaryEditor);
    NSMutableArray *bookmarks = [NSMutableArray new];
    for (sptr_t line = Send(self.primaryEditor, SCI_MARKERNEXT, 0, 1); line >= 0; line = Send(self.primaryEditor, SCI_MARKERNEXT, line + 1, 1)) [bookmarks addObject:@(line)];
    return @{@"id":self.identifier, @"path":self.path, @"title":self.title, @"lexer":self.lexer,
             @"builtinLanguage":self.builtinLanguage ?: @"", @"customLanguage":self.customLanguage ?: @"", @"secondaryOnly":@(self.secondaryOnly), @"pinned":@(self.pinned), @"tabColor":@(self.tabColor),
             @"bookmarks":bookmarks, @"eol":@(Send(self.primaryEditor, SCI_GETEOLMODE)),
             @"text":text, @"format":NXCompanion(text, State(self.primaryEditor)), @"diskHash":self.diskHash,
             @"encoding":@(self.encoding), @"bom":@(self.bom), @"dirty":@(self.dirty),
             @"caret":@(Send(self.primaryEditor, SCI_GETCURRENTPOS)), @"anchor":@(Send(self.primaryEditor, SCI_GETANCHOR)),
             @"zoom":@(Send(self.primaryEditor,SCI_GETZOOM)), @"xOffset":@(Send(self.primaryEditor,SCI_GETXOFFSET)), @"firstLine":@(Send(self.primaryEditor, SCI_GETFIRSTVISIBLELINE))};
}
@end

@interface NXApp : NSObject <NSApplicationDelegate, NSWindowDelegate, NSTabViewDelegate, ScintillaNotificationProtocol, NSTextFieldDelegate>
@property NSWindow *window;
@property NSTabView *tabs;
@property NSMutableArray<NXDocument *> *documents;
@property NXStorage *storage;
@property NSDictionary *lastSnapshot;
@property NSTextField *status;
@property NSComboBox *family;
@property NSComboBox *size;
@property NSButton *bulletListButton;
@property NSButton *numberedListButton;
@property NSTextField *findText;
@property NSTextField *replaceText;
@property NSStackView *findBar;
@property NSPopUpButton *language;
@property NSTimer *timer;
@property BOOL restoring;
@property BOOL dark;
@property BOOL wrap;
@property BOOL snapshotFailed;
@property unsigned colorField;
@property NSMutableArray<NSString *> *pendingFiles;
@property NXMenuLayout *menuLayout;
@property NXWorkspace *workspace;
@property NSSplitView *split;
@property BOOL sidebarVisible;
@property NSMutableDictionary *preferences;
@property NSMutableArray *macros;
@property NSMutableArray *recordingActions;
@property BOOL recording;
@property BOOL playingMacro;
@property NSData *paintFormat;
@property NSMutableDictionary *configuration;
@property NSMutableDictionary *syntaxHashes;
@property NSSplitView *editorSplit;
@property NSView *secondaryHost;
@property ScintillaView *secondaryEditor;
@property NSPopUpButton *secondaryChoice;
@property NXDocument *secondaryDocument;
@property BOOL secondaryFocused;
@property BOOL secondaryVisible;
@property BOOL synchronizedScroll;
@property BOOL syncingScroll;
@property NXFileSearch *fileSearch;
@property NSMutableArray *closedPaths;
@property NSMutableDictionary *searchOptions;
@property NSDictionary *searchScopeRecord;
@property NSNumber *lastZeroMatch;
@property NSStringEncoding openingEncoding;
@property NXDocumentMap *documentMap;
@property NSMutableArray *clipboardHistory;
@property NXMenuLayout *contextLayout;
@property NSMenu *editorContextMenu;
@property NXRunTool *runTool;
@property sptr_t selectionAnchor;
@property NSString *selectionDocument;
@property NSStackView *toolbar;
@property BOOL focusMode;
@property NSString *hintName;
@property NSInteger hintIndex;
@property BOOL searchTyping;
@property BOOL syncVertical;
@property BOOL syncHorizontal;
@property NSMutableArray *recentTabIDs;
@property id tabKeyMonitor;
- (NXDocument *)current;
- (void)runToolCommand:(id)sender;
- (NXDocument *)restoreDocumentRecord:(NSDictionary *)record;
- (NXDocument *)newDocument:(id)sender;
- (void)configureLexer:(NXDocument *)doc;
- (BOOL)snapshot:(NSError **)error;
- (BOOL)openPath:(NSString *)path error:(NSError **)error;
- (BOOL)saveDocument:(NXDocument *)doc path:(NSString *)path error:(NSError **)error;
@end

@interface NXApp (Features)
- (void)buildFeatureMenus;
- (void)applyPreferences:(NXDocument *)doc;
- (void)recordNotification:(SCNotification *)notification;
- (void)refreshFunctions;
- (void)playActions:(NSArray *)actions repetitions:(NSUInteger)repetitions;
- (void)preferencesDialog:(id)sender;
- (void)customizeMenus:(id)sender;
- (void)resetMenus:(id)sender;
- (void)showPanel:(id)sender;
- (void)toggleSidebar:(id)sender;
- (void)applySidebarVisibility;
- (void)updateLineNumberMargin:(ScintillaView *)view;
@end
@interface NXApp (FeatureTests)
- (void)featureTests;
@end
@interface NXApp (Workbench)
- (void)buildWorkbenchMenus;
- (void)buildSecondaryView;
- (void)cloneView:(id)sender;
- (void)closeSecondary:(id)sender;
- (void)selectSecondary:(id)sender;
- (void)bindSecondary:(NXDocument *)document;
- (void)updateSecondaryChoices;
- (NSDictionary *)secondaryRecord;
- (void)restoreSecondary:(NSDictionary *)record;
- (void)refreshEditorSettings;
- (void)refreshLanguageChoices;
- (void)updateUserSyntax;
- (void)preferencesEditor:(id)sender;
- (void)languageEditor:(id)sender;
- (void)themeEditor:(id)sender;
- (void)shortcutEditor:(id)sender;
- (void)chooseTheme:(id)sender;
- (void)chooseLanguage:(id)sender;
- (NSDictionary *)userLanguage:(NXDocument *)document;
- (void)applyUserTheme:(NXDocument *)document;
- (void)findInFiles:(id)sender;
- (BOOL)commitConfiguration:(NSDictionary *)configuration;
- (NSArray *)searchScope:(NSInteger)scope;
- (void)navigateResult:(NSDictionary *)result;
- (NSString *)applySearchResults:(NSArray *)results;
@end
@interface NXApp (WorkbenchTests)
- (void)workbenchTests;
- (void)seedWorkbench;
- (void)verifyWorkbenchRestore;
- (void)workbenchUI:(NSString *)mode;
@end
@interface NXApp (Tests)
- (void)selfTest:(NSString *)mode;
@end

@interface NXApp (Editing)
- (BOOL)insertEditorNewline:(ScintillaView *)view;
- (void)toggleList:(BOOL)numbered;
@end

@interface NXApp (DocumentCommands)
- (void)moveTabFrom:(NSInteger)from to:(NSInteger)to;
- (void)chooseOpenDocument:(id)sender;
- (void)tabCommand:(id)sender;
- (NSMenu *)tabMenu:(NSInteger)index;
- (void)closeDocuments:(id)sender;
- (void)recentDocument:(id)sender;
- (void)fileCommand:(id)sender;
- (void)readOnly:(id)sender;
@end
@interface NXApp (CommandMenus)
- (void)buildCommandMenus;
@end
@interface NXApp (TextCommands)
- (void)selectionMode:(id)sender;
- (void)lineOperation:(id)sender;
- (void)caseOperation:(id)sender;
- (void)commentOperation:(id)sender;
- (void)insertDate:(id)sender;
- (void)columnEditor:(id)sender;
- (void)characterViewer:(id)sender;
- (void)viewCommand:(id)sender;
- (void)foldLevel:(id)sender;
- (void)bookmarkLines:(id)sender;
- (void)hashCommand:(id)sender;
@end

@interface NXApp (Instances)
- (void)buildInstanceMenus;
- (NSTask *)launchInstanceWithDocument:(NXDocument *)document testMode:(BOOL)test error:(NSError **)error;
@end
@interface NXApp (ExtraCommands)
- (void)buildExtraMenus;
- (BOOL)pairedInput:(ScintillaView *)view key:(NSString *)key;
- (void)numberColumns:(id)sender;
- (void)foldSpecific:(id)sender;
- (void)openBrowser:(id)sender;
- (void)folderWorkspace:(id)sender;
@end
@interface NXApp (ParityTests)
- (void)parityTests;
@end
@interface NXApp (SearchCommands)
- (void)searchSettings:(id)sender;
- (NSArray *)documentMatches;
- (void)searchNavigate:(id)sender;
- (void)searchReplace:(id)sender;
- (void)searchUtility:(id)sender;
- (void)matchingBrace:(id)sender;
- (void)buildSearchMenus;
- (void)searchResult:(id)sender;
- (void)incrementalSearch;
@end
@interface NXApp (EncodingCommands)
- (void)legacyEncoding:(id)sender;
- (void)buildEncodingMenus;
@end
static NSArray *NXLanguageCatalog(void);
static NSDictionary *NXBuiltIn(NSString *identifier);
@interface NXApp (LanguageCatalog)
- (void)chooseBuiltinLanguage:(id)sender;
- (void)buildLanguageMenu;
@end
@interface NXApp (SessionCommands)
- (void)sessionFile:(id)sender;
- (void)trashDocument:(id)sender;
- (void)toggleMonitoring:(id)sender;
- (void)checkMonitoredFiles;
- (void)buildSessionMenus;
@end
@interface NXApp (ClipboardCommands)
- (void)cut:(id)sender;
- (void)copy:(id)sender;
- (void)paste:(id)sender;
- (void)clipboardCommand:(id)sender;
- (void)clipboardHistoryCommand:(id)sender;
- (void)pasteRich:(id)sender;
- (void)customizeContextMenu:(id)sender;
- (void)buildClipboardMenus;
@end
@interface NXApp (NavigationCommands)
- (void)extendedSelection:(id)sender;
- (void)selectedSearch:(id)sender;
- (void)markCommand:(id)sender;
- (void)displayCommand:(id)sender;
- (void)changeHistory:(id)sender;
- (void)directTab:(id)sender;
- (void)sortTabs:(id)sender;
- (void)buildNavigationMenus;
@end
@interface NXApp (Completion)
- (void)completeFunction:(id)sender;
- (void)parameterHint:(id)sender;
- (void)completePath:(id)sender;
- (void)buildCompletionMenus;
@end
@interface NXApp (StyleOverrides)
- (void)styleEditor:(id)sender;
@end
@interface NXApp (ViewTransfer)
- (void)activateDocument:(NXDocument *)doc;
- (void)moveOtherView:(id)sender;
- (void)synchronizeAxis:(id)sender;
- (void)buildTransferMenus;
@end
@interface NXApp (UtilityCommands)
- (void)binaryClipboard:(id)sender;
- (void)redactSelection:(id)sender;
- (void)openSelectedPath:(id)sender;
- (void)findCharacterRange:(id)sender;
- (void)copyAllStyledText:(id)sender;
- (void)windowHistory:(id)sender;
- (void)debugInformation:(id)sender;
- (void)buildUtilityMenus;
@end
@implementation NXApp
- (NXDocument *)current {
    if(self.secondaryVisible && self.secondaryFocused && self.secondaryDocument) return self.secondaryDocument;
    NSInteger index = [self.tabs indexOfTabViewItem:self.tabs.selectedTabViewItem];
    return index >= 0 && index < (NSInteger)self.documents.count ? self.documents[index] : nil;
}
- (NSButton *)button:(NSString *)title action:(SEL)action tag:(NSInteger)tag {
    NSButton *button = [NSButton buttonWithTitle:title target:self action:action];
    button.tag = tag;
    button.bezelStyle = NSBezelStyleRounded;
    return button;
}
- (void)item:(NSMenu *)menu title:(NSString *)title action:(SEL)action key:(NSString *)key tag:(NSInteger)tag {
    NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:title action:action keyEquivalent:key];
    item.target = self; item.tag = tag;
    [menu addItem:item];
}
- (NSMenu *)menu:(NSString *)title parent:(NSMenu *)parent {
    NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:title action:nil keyEquivalent:@""];
    NSMenu *menu = [[NSMenu alloc] initWithTitle:title];
    item.submenu = menu;
    [parent addItem:item];
    return menu;
}
- (void)buildMenus {
    NSMenu *root = [NSMenu new];
    NSMenu *app = [self menu:@"NoteQu" parent:root];
    [self item:app title:@"About NoteQu" action:@selector(about:) key:@"" tag:0];
    [self item:app title:@"Check for Updates…" action:@selector(checkUpdates:) key:@"" tag:0];
    [self item:app title:@"Automatic Update Checks…" action:@selector(updateSettings:) key:@"" tag:0];
    [app addItem:NSMenuItem.separatorItem];
    NSMenuItem *hide = [[NSMenuItem alloc] initWithTitle:@"Hide NoteQu" action:@selector(hide:) keyEquivalent:@"h"];
    [app addItem:hide];
    NSMenuItem *quit = [[NSMenuItem alloc] initWithTitle:@"Quit NoteQu" action:@selector(terminate:) keyEquivalent:@"q"];
    [app addItem:quit];
    NSMenu *file = [self menu:@"File" parent:root];
    [self item:file title:@"New Tab" action:@selector(newDocument:) key:@"n" tag:0];
    [self item:file title:@"Open…" action:@selector(open:) key:@"o" tag:0];
    [self item:file title:@"Save" action:@selector(save:) key:@"s" tag:0];
    [self item:file title:@"Save As…" action:@selector(saveAs:) key:@"S" tag:0];
    [self item:file title:@"Close Tab…" action:@selector(closeTab:) key:@"w" tag:0];
    [file addItem:NSMenuItem.separatorItem];
    [self item:file title:@"Show Session Folder" action:@selector(showSession:) key:@"" tag:0];
    NSMenu *edit = [self menu:@"Edit" parent:root];
    for (NSArray *entry in @[@[@"Undo", @"undo:", @"z"], @[@"Redo", @"redo:", @"Z"],
                            @[@"Cut", @"cut:", @"x"], @[@"Copy", @"copy:", @"c"],
                            @[@"Paste", @"paste:", @"v"], @[@"Select All", @"selectAll:", @"a"]]) {
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:entry[0] action:NSSelectorFromString(entry[1]) keyEquivalent:entry[2]];
        [edit addItem:item];
    }
    [edit addItem:NSMenuItem.separatorItem];
    [self item:edit title:@"Find and Replace…" action:@selector(showFind:) key:@"f" tag:0];
    [self item:edit title:@"Find Next" action:@selector(findNext:) key:@"g" tag:0];
    [self item:edit title:@"Go to Line…" action:@selector(goToLine:) key:@"l" tag:0];
    NSMenu *format = [self menu:@"Format" parent:root];
    [self item:format title:@"Bold" action:@selector(toggleFormat:) key:@"b" tag:NoteQu::Bold];
    [self item:format title:@"Italic" action:@selector(toggleFormat:) key:@"i" tag:NoteQu::Italic];
    [self item:format title:@"Underline" action:@selector(toggleFormat:) key:@"u" tag:NoteQu::Underline];
    [self item:format title:@"Strikethrough" action:@selector(toggleFormat:) key:@"" tag:NoteQu::Strike];
    [self item:format title:@"Text Color…" action:@selector(pickColor:) key:@"" tag:NoteQu::Fore];
    [self item:format title:@"Highlight Color…" action:@selector(pickColor:) key:@"" tag:NoteQu::Highlight];
    [self item:format title:@"Clear Formatting" action:@selector(clearFormat:) key:@"" tag:0];
    [self item:format title:@"Bulleted List" action:@selector(makeList:) key:@"" tag:0];
    [self item:format title:@"Numbered List" action:@selector(makeList:) key:@"" tag:1];
    NSMenu *view = [self menu:@"View" parent:root];
    [self item:view title:@"Toggle Word Wrap" action:@selector(toggleWrap:) key:@"" tag:0];
    [self item:view title:@"Toggle Dark Appearance" action:@selector(toggleDark:) key:@"" tag:0];
    [self item:view title:@"Zoom In" action:@selector(editorCommand:) key:@"+" tag:SCI_ZOOMIN];
    [self item:view title:@"Zoom Out" action:@selector(editorCommand:) key:@"-" tag:SCI_ZOOMOUT];
    [self item:view title:@"Reset Zoom" action:@selector(editorCommand:) key:@"0" tag:SCI_SETZOOM];
    [self item:view title:@"Next Tab" action:@selector(nextTab:) key:@"]" tag:1];
    [self item:view title:@"Previous Tab" action:@selector(nextTab:) key:@"[" tag:-1];
    NSMenu *help = [self menu:@"Help" parent:root];
    [self item:help title:@"macOS Guide and Limitations" action:@selector(guide:) key:@"" tag:0];
    [self item:help title:@"Source Code and License" action:@selector(source:) key:@"" tag:0];
    NSApp.mainMenu = root;
    [self buildFeatureMenus];
    [self buildWorkbenchMenus];
    [self buildCommandMenus];
    [self buildSearchMenus];
    [self buildEncodingMenus];
    [self buildLanguageMenu];
    [self buildSessionMenus];
    [self buildClipboardMenus];
    [self buildNavigationMenus];
    [self buildCompletionMenus];
    [self buildTransferMenus];
    [self buildUtilityMenus];[self buildExtraMenus];[self buildInstanceMenus];
    [self item:[NSApp.mainMenu itemAtIndex:0].submenu title:@"Syntax Style Editor…" action:@selector(styleEditor:) key:@"" tag:0];
}
- (void)buildWindow {
    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1120, 740)
        styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
        backing:NSBackingStoreBuffered defer:NO];
    self.window.title = @"NoteQu";
    self.window.minSize = NSMakeSize(900, 450);
    self.window.delegate = self;
    self.window.releasedWhenClosed = NO;
    self.window.frameAutosaveName = @"NoteXMainWindow";
    [self.window center];
    NSView *content = self.window.contentView;
    self.family = [[NSComboBox alloc] initWithFrame:NSMakeRect(0, 0, 170, 26)];
    [self.family addItemsWithObjectValues:[NSFontManager.sharedFontManager.availableFontFamilies sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)]];
    self.family.stringValue = @"JetBrains Mono"; self.family.target = self; self.family.action = @selector(changeFamily:);
    [self.family.widthAnchor constraintEqualToConstant:170].active = YES;
    self.size = [[NSComboBox alloc] initWithFrame:NSMakeRect(0, 0, 64, 26)];
    [self.size addItemsWithObjectValues:@[@"8", @"9", @"10", @"11", @"12", @"13", @"14", @"16", @"18", @"24", @"32", @"48", @"72", @"96"]];
    self.size.stringValue = @"13"; self.size.target = self; self.size.action = @selector(changeSize:);
    [self.size.widthAnchor constraintEqualToConstant:64].active = YES;
    self.language = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
    [self.language addItemWithTitle:@"Plain Text"];
    for (int i = 0; i < GetLexerCount(); ++i) { char name[128]{}; GetLexerName(i, name, sizeof name); [self.language addItemWithTitle:@(name)]; }
    [self refreshLanguageChoices];
    self.language.target = self; self.language.action = @selector(changeLanguage:);
    [self.language.widthAnchor constraintEqualToConstant:140].active = YES;
    self.bulletListButton=[self button:@"• ≡" action:@selector(makeList:) tag:0];
    self.numberedListButton=[self button:@"1. ≡" action:@selector(makeList:) tag:1];
    for(NSButton *button in @[self.bulletListButton,self.numberedListButton]){
        NSString *label=button.tag?@"Numbered List":@"Bulleted List";
        NSImage *icon=[NSImage imageWithSystemSymbolName:button.tag?@"list.number":@"list.bullet" accessibilityDescription:label];
        if(icon){button.image=icon;button.imagePosition=NSImageOnly;}
        button.toolTip=label;[button setAccessibilityLabel:label];[button.widthAnchor constraintEqualToConstant:32].active=YES;
    }
    NSStackView *toolbar = [NSStackView stackViewWithViews:@[
        [self button:@"+" action:@selector(newDocument:) tag:0], self.family, self.size,
        [self button:@"B" action:@selector(toggleFormat:) tag:NoteQu::Bold],
        [self button:@"I" action:@selector(toggleFormat:) tag:NoteQu::Italic],
        [self button:@"U" action:@selector(toggleFormat:) tag:NoteQu::Underline],
        [self button:@"S̶" action:@selector(toggleFormat:) tag:NoteQu::Strike],
        [self button:@"Color" action:@selector(pickColor:) tag:NoteQu::Fore],
        [self button:@"Highlight" action:@selector(pickColor:) tag:NoteQu::Highlight], self.bulletListButton, self.numberedListButton, self.language]];
    self.toolbar=toolbar;
    toolbar.spacing = 6;
    toolbar.edgeInsets = NSEdgeInsetsMake(8, 10, 8, 10);
    self.findText = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 180, 24)];
    self.findText.delegate=self;
    self.findText.placeholderString = @"Find";
    self.findText.target = self; self.findText.action = @selector(findNext:);
    self.replaceText = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 180, 24)];
    self.replaceText.placeholderString = @"Replace with";
    for (NSTextField *field in @[self.findText, self.replaceText]) [field.widthAnchor constraintGreaterThanOrEqualToConstant:160].active = YES;
    self.findBar = [NSStackView stackViewWithViews:@[self.findText, self.replaceText,
        [self button:@"Next" action:@selector(findNext:) tag:0],
        [self button:@"Replace" action:@selector(replace:) tag:0],
        [self button:@"Replace All" action:@selector(replace:) tag:1],
        [self button:@"Options" action:@selector(searchSettings:) tag:0],
        [self button:@"Done" action:@selector(hideFind:) tag:0]]];
    self.findBar.edgeInsets = NSEdgeInsetsMake(4, 10, 4, 10);
    self.findBar.hidden = YES;
    self.tabs = [[NXTabView alloc] initWithFrame:NSZeroRect];
    self.tabs.delegate = self;
    __weak NXApp *tabOwner=self;
    ((NXTabView *)self.tabs).closeDocument=^(NSInteger index){[tabOwner.tabs selectTabViewItemAtIndex:index];[tabOwner closeTab:nil];};
    ((NXTabView *)self.tabs).moveDocument=^(NSInteger from,NSInteger to){[tabOwner moveTabFrom:from to:to];};
    ((NXTabView *)self.tabs).chooseDocument=^{[tabOwner chooseOpenDocument:nil];};
    ((NXTabView *)self.tabs).documentMenu=^NSMenu *(NSInteger index){return [tabOwner tabMenu:index];};
    self.status = [NSTextField labelWithString:@"UTF-8"];
    self.split = [[NSSplitView alloc] initWithFrame:NSMakeRect(0, 0, 1120, 600)];
    self.split.vertical = YES; self.split.dividerStyle = NSSplitViewDividerStyleThin;
    self.workspace.view.frame = NSMakeRect(0, 0, 285, 600);
    self.tabs.frame = NSMakeRect(286, 0, 834, 600);
    self.tabs.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [self buildSecondaryView];
    [self.split addSubview:self.workspace.view]; [self.split addSubview:self.editorSplit];
    [self.split setHoldingPriority:NSLayoutPriorityDefaultHigh forSubviewAtIndex:0];
    NSStackView *stack = [NSStackView stackViewWithViews:@[toolbar, self.findBar, self.split, self.status]];
    stack.orientation = NSUserInterfaceLayoutOrientationVertical;
    stack.distribution = NSStackViewDistributionFill;
    stack.alignment = NSLayoutAttributeLeading;
    stack.spacing = 4;
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    [content addSubview:stack];
    [NSLayoutConstraint activateConstraints:@[
        [stack.leadingAnchor constraintEqualToAnchor:content.leadingAnchor], [stack.trailingAnchor constraintEqualToAnchor:content.trailingAnchor],
        [stack.topAnchor constraintEqualToAnchor:content.topAnchor], [stack.bottomAnchor constraintEqualToAnchor:content.bottomAnchor constant:-6],
        [self.status.bottomAnchor constraintEqualToAnchor:stack.bottomAnchor],
        [toolbar.heightAnchor constraintEqualToConstant:44], [self.findBar.heightAnchor constraintEqualToConstant:36],
        [toolbar.widthAnchor constraintEqualToAnchor:stack.widthAnchor], [self.split.widthAnchor constraintEqualToAnchor:stack.widthAnchor],
        [self.split.heightAnchor constraintGreaterThanOrEqualToConstant:200]]];
    [self.split setContentHuggingPriority:1 forOrientation:NSLayoutConstraintOrientationVertical];
}
- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    for (NSString *font in @[@"JetBrainsMono.ttf", @"Inter.ttf"]) {
        NSURL *url = [NSBundle.mainBundle.resourceURL URLByAppendingPathComponent:[@"Fonts" stringByAppendingPathComponent:font]];
        CTFontManagerRegisterFontsForURL((__bridge CFURLRef)url, kCTFontManagerScopeProcess, nil);
    }
    self.documents = [NSMutableArray new];self.selectionAnchor=-1;self.syncVertical=YES;self.syncHorizontal=YES;
    self.closedPaths=[NSMutableArray new];self.recentTabIDs=[NSMutableArray new];self.clipboardHistory=[NSMutableArray new];
    self.searchOptions=[@{@"matchCase":@NO,@"wholeWord":@NO,@"regex":@NO,@"engine":@"boost",@"selection":@NO} mutableCopy];
    self.dark = YES; self.wrap = YES; self.restoring = YES;
    NSError *error = nil;
    NSString *profile = NSProcessInfo.processInfo.environment[@"NOTEX_PROFILE"];
    if (!profile.length) {
        NSString *support = [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Application Support"];
        NSString *current = [support stringByAppendingPathComponent:@"NoteQu"];
        NSString *previous = [support stringByAppendingPathComponent:@"NoteX"];
        NSFileManager *files = NSFileManager.defaultManager;
        profile = ![files fileExistsAtPath:current] && [files fileExistsAtPath:previous] ? previous : current;
    }
    self.storage = [[NXStorage alloc] initWithDirectory:profile error:&error];
    NSDictionary *session = self.storage ? [self.storage load:&error] : nil;
    if (error) { Alert(error.localizedDescription); exit(1); }
    if (session[@"dark"]) self.dark = [session[@"dark"] boolValue];
    if (session[@"wrap"]) self.wrap = [session[@"wrap"] boolValue];
    self.preferences = [@{@"font":@"JetBrains Mono", @"fontSize":@13, @"tabWidth":@4, @"useTabs":@NO, @"lineNumbers":@YES,
        @"whitespace":@NO, @"autoIndent":@YES, @"braceMatch":@YES, @"autoComplete":@NO} mutableCopy];
    if ([session[@"preferences"] isKindOfClass:NSDictionary.class]) [self.preferences addEntriesFromDictionary:session[@"preferences"]];
    self.macros = [session[@"macros"] isKindOfClass:NSArray.class] ? [session[@"macros"] mutableCopy] : [NSMutableArray new];
    self.recordingActions = [NSMutableArray new];
    self.configuration=[(session[@"configuration"] ?: NXDefaultConfiguration()) mutableCopy];
    self.syntaxHashes=[NSMutableDictionary new];
    NSDictionary *extra=@{@"autoPairs":@NO,@"functionCompletion":@NO,@"parameterHints":@NO,@"nonPrinting":@NO,@"caretWidth":@1,@"blinkRate":@500,@"edgeColumn":@0,@"snapshotSeconds":@1,@"defaultEncoding":@0,@"defaultEOL":@(SC_EOL_LF),@"indentGuides":@YES,@"showEOL":@NO,@"highlightLine":@NO,@"wrap":@(self.wrap),@"dark":@(self.dark)};
    for(NSString *key in extra)if(!self.preferences[key])self.preferences[key]=extra[key];
    self.sidebarVisible = session[@"sidebarVisible"] ? [session[@"sidebarVisible"] boolValue] : NO;
    self.workspace = [[NXWorkspace alloc] initWithState:[session[@"workspace"] isKindOfClass:NSDictionary.class] ? session[@"workspace"] : @{}];
    __weak NXApp *weakSelf = self;
    self.workspace.openFile = ^(NSString *path) { NSError *openError = nil; if (![weakSelf openPath:path error:&openError]) Alert(openError.localizedDescription); };
    self.workspace.goToPosition = ^(NSInteger position) { NXApp *owner = weakSelf; Send(owner.current.editor, SCI_GOTOPOS, position); [owner.window makeFirstResponder:owner.current.editor]; };
    self.workspace.changed = ^{ if (!weakSelf.restoring) [weakSelf tick:nil]; };
    self.workspace.refreshRequested = ^{ [weakSelf refreshFunctions]; };
    [self buildMenus];
    self.menuLayout = [[NXMenuLayout alloc] initWithMenu:NSApp.mainMenu settings:session[@"menuLayout"]];
    self.menuLayout.commit = ^BOOL(NSError **saveError) { return [weakSelf snapshot:saveError]; };
    self.editorContextMenu=[NSMenu new];
    for(NSString *title in @[@"Edit",@"Format",@"Bookmarks"]){NSMenuItem *item=[NSMenuItem new];item.title=title;item.submenu=[[NSApp.mainMenu itemWithTitle:title].submenu copy];[self.editorContextMenu addItem:item];}
    self.contextLayout=[[NXMenuLayout alloc]initWithMenu:self.editorContextMenu settings:session[@"contextLayout"]];self.contextLayout.commit=^BOOL(NSError **saveError){return [weakSelf snapshot:saveError];};[self.contextLayout apply];
    [self.menuLayout apply];
    [self buildWindow];
    self.tabKeyMonitor=[NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown handler:^NSEvent *(NSEvent *event){if(event.window==weakSelf.window&&!NSApp.modalWindow&&event.keyCode==48&&(event.modifierFlags&NSEventModifierFlagControl)){NSMenuItem *command=[NSMenuItem new];command.tag=(event.modifierFlags&NSEventModifierFlagShift)?-1:1;[weakSelf nextTab:command];return nil;}return event;}];
    self.window.appearance = [NSAppearance appearanceNamed:self.dark ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua];
    for (NSDictionary *record in session[@"documents"]) [self restoreDocumentRecord:record];
    if (!self.documents.count) [self newDocument:nil];
    NSInteger selected = [session[@"selected"] integerValue];
    if (selected >= 0 && selected < (NSInteger)self.documents.count) [self.tabs selectTabViewItemAtIndex:selected];
    [self restoreSecondary:session[@"secondary"]];
    if(!self.secondaryVisible)for(NXDocument *doc in self.documents)doc.secondaryOnly=NO;
    self.restoring = NO;
    [self applySidebarVisibility];
    self.timer = [NSTimer scheduledTimerWithTimeInterval:[self.preferences[@"snapshotSeconds"] doubleValue] target:self selector:@selector(tick:) userInfo:nil repeats:YES];
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    [self.window makeFirstResponder:self.current.editor];
    [self tick:nil];
    if (self.storage.recoveredPrevious) Alert(@"The latest session snapshot could not be read. NoteQu restored the previous snapshot and retained the damaged file in the session folder. The most recent edits may be missing.");
    for (NSString *path in self.pendingFiles) {
        NSError *openError = nil;
        if (![self openPath:path error:&openError]) Alert(openError.localizedDescription);
    }
    self.pendingFiles = nil;
    NXUpdateHealthy();
    if (![NSProcessInfo.processInfo.arguments containsObject:@"--self-test"]) NXUpdate(@"auto",self.storage.directory);
    NSArray *arguments = NSProcessInfo.processInfo.arguments;
    NSUInteger index = [arguments indexOfObject:@"--self-test"];
    if (index != NSNotFound && index + 1 < arguments.count) {
        NSString *mode = arguments[index + 1];
        dispatch_async(dispatch_get_main_queue(), ^{ [self selfTest:mode]; });
    }
}
- (NXDocument *)restoreDocumentRecord:(NSDictionary *)record {
        NXDocument *doc = [self newDocument:nil];
        doc.identifier = record[@"id"];doc.tab.identifier=doc.identifier;doc.secondaryOnly=[record[@"secondaryOnly"] boolValue]; doc.path = NXCanonicalPath(record[@"path"]); doc.title = record[@"title"];
        doc.diskHash = record[@"diskHash"]; doc.encoding = [record[@"encoding"] unsignedIntegerValue]; doc.bom = [record[@"bom"] boolValue];
        doc.lexer = record[@"lexer"];
        doc.customLanguage=record[@"customLanguage"] ?: @"";doc.builtinLanguage=record[@"builtinLanguage"] ?: @"";doc.pinned=[record[@"pinned"] boolValue];doc.tabColor=[record[@"tabColor"] integerValue];
        SetText(doc.editor, record[@"text"]);
        if ([record[@"eol"] isKindOfClass:NSNumber.class]) Send(doc.editor, SCI_SETEOLMODE, [record[@"eol"] integerValue]);
        for (NSNumber *line in record[@"bookmarks"]) if (line.longLongValue >= 0 && line.longLongValue < Send(doc.editor, SCI_GETLINECOUNT)) Send(doc.editor, SCI_MARKERADD, line.unsignedLongLongValue, 0);
        NoteQu::State state;
        NXReadCompanion(record[@"format"], record[@"text"], state);
        Restore(doc.editor, std::move(state));
        Send(doc.editor, SCI_EMPTYUNDOBUFFER); Send(doc.editor, SCI_SETSAVEPOINT);
        doc.restoredDirty = [record[@"dirty"] boolValue];
        auto length = Send(doc.editor, SCI_GETLENGTH);
        Send(doc.editor, SCI_SETSEL, std::clamp<sptr_t>([record[@"anchor"] longLongValue], 0, length),
             std::clamp<sptr_t>([record[@"caret"] longLongValue], 0, length));
        Send(doc.editor, SCI_SETFIRSTVISIBLELINE, std::max<long long>(0, [record[@"firstLine"] longLongValue]));
        Send(doc.primaryEditor,SCI_SETZOOM,[record[@"zoom"] integerValue]); Send(doc.primaryEditor,SCI_SETXOFFSET,[record[@"xOffset"] integerValue]);
        [self configureLexer:doc];
    return doc;
}
- (NXDocument *)newDocument:(id)sender {
    NXDocument *doc = [NXDocument new];
    doc.identifier = NSUUID.UUID.UUIDString; doc.path = @""; doc.lexer = @"null";
    doc.customLanguage=@"";
    doc.title = [NSString stringWithFormat:@"Untitled %lu", (unsigned long)self.documents.count + 1];
    doc.diskHash = NSData.data; doc.encoding = NSUTF8StringEncoding;
    NSInteger encoding=[self.preferences[@"defaultEncoding"] integerValue];doc.encoding=encoding<2?NSUTF8StringEncoding:encoding==2?NSUTF16LittleEndianStringEncoding:NSUTF16BigEndianStringEncoding;doc.bom=encoding!=0;
    doc.editor = [[NXEditor alloc] initWithFrame:NSMakeRect(0, 0, 1000, 620)];
    __weak NXApp *owner = self;
    ((NXEditor *)doc.editor).editorContextMenu=^NSMenu *{return owner.editorContextMenu;};
    ((NXEditor *)doc.editor).typedKey=^BOOL(ScintillaView *view,NSString *key){return [owner pairedInput:view key:key];};
    ((NXEditor *)doc.editor).newline = ^BOOL(ScintillaView *view) { return [owner insertEditorNewline:view]; };
    doc.editor.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    Send(doc.editor,SCI_SETCHANGEHISTORY,SC_CHANGE_HISTORY_ENABLED|SC_CHANGE_HISTORY_MARKERS);
    Send(doc.editor, SCI_SETCODEPAGE, SC_CP_UTF8);
    Send(doc.editor, SCI_SETEOLMODE, [self.preferences[@"defaultEOL"] integerValue]);
    Send(doc.editor,SCI_SETMULTIPLESELECTION,1);Send(doc.editor,SCI_SETADDITIONALSELECTIONTYPING,1);Send(doc.editor,SCI_SETMULTIPASTE,SC_MULTIPASTE_EACH);
    Send(doc.editor, SCI_SETTABWIDTH, 4); Send(doc.editor, SCI_SETUSETABS, 0);
    Send(doc.editor, SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER); Send(doc.editor, SCI_SETMARGINWIDTHN, 0, 0);
    Send(doc.editor, SCI_SETWRAPMODE, self.wrap ? SC_WRAP_WORD : SC_WRAP_NONE);
    Send(doc.editor, SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<sptr_t>("JetBrains Mono"));
    Send(doc.editor, SCI_STYLESETSIZEFRACTIONAL, STYLE_DEFAULT, 1300);
    [self applyPreferences:doc];
    [self theme:doc];
    doc.editor.delegate = self;
    if (self.recording) Send(doc.editor, SCI_STARTRECORD);
    doc.tab = [[NSTabViewItem alloc] initWithIdentifier:doc.identifier];
    doc.tab.label = doc.title; doc.tab.view = doc.editor;
    [self.documents addObject:doc]; [self.tabs addTabViewItem:doc.tab]; [self.tabs selectTabViewItem:doc.tab];
    [self.window makeFirstResponder:doc.editor];
    return doc;
}
- (void)theme:(NXDocument *)doc {
    int back = self.dark ? 0x26211c : 0xffffff;
    int fore = self.dark ? 0xefece8 : 0x282828;
    Send(doc.editor, SCI_STYLESETBACK, STYLE_DEFAULT, back);
    Send(doc.editor, SCI_STYLESETFORE, STYLE_DEFAULT, fore);
    Send(doc.editor, SCI_STYLECLEARALL);
    Send(doc.editor, SCI_STYLESETFORE, STYLE_LINENUMBER, self.dark ? 0xaaa299 : 0x777777);
    Send(doc.editor, SCI_STYLESETBACK, STYLE_LINENUMBER, back);
    Send(doc.editor, SCI_SETFOLDMARGINCOLOUR, 1, back);
    Send(doc.editor, SCI_SETFOLDMARGINHICOLOUR, 1, back);
    Send(doc.editor, SCI_SETCARETFORE, self.dark ? 0xc5d9ff : 0x222222);
    Send(doc.editor, SCI_SETSELBACK, 1, self.dark ? 0x765637 : 0xeedbb7);
    Send(doc.editor, SCI_SETSELFORE, 0, 0);
    [self configureLexer:doc];
}
- (void)configureLexer:(NXDocument *)doc {
    NSDictionary *custom=[self userLanguage:doc];
    auto *lexer = custom ? ([custom[@"advancedRules"] boolValue]?CreateLexer("user"):nullptr) : CreateLexer(doc.lexer.UTF8String);
    Send(doc.editor, SCI_SETILEXER, 0, reinterpret_cast<sptr_t>(lexer));
    Send(doc.editor, SCI_SETPROPERTY, reinterpret_cast<uptr_t>("fold"), reinterpret_cast<sptr_t>("1"));
    // Common lexer styles; users can select any bundled Lexilla lexer.
    if ([doc.lexer isEqual:@"cpp"] || [doc.lexer isEqual:@"python"]) {
        const char *keywords = [doc.lexer isEqual:@"python"] ?
            "False None True and as assert async await break class continue def del elif else except finally for from global if import in is lambda nonlocal not or pass raise return try while with yield" :
            "alignas auto bool break case catch char class const constexpr continue default delete do double else enum explicit extern false float for friend if inline int long namespace new nullptr operator private protected public return short signed sizeof static struct switch template this throw true try typedef typename union unsigned using virtual void volatile while export import function let var async await function return typeof null undefined";
        Send(doc.editor, SCI_SETKEYWORDS, 0, reinterpret_cast<sptr_t>(keywords));
        Send(doc.editor, SCI_STYLESETFORE, 1, self.dark ? 0x88aa88 : 0x337733);
        Send(doc.editor, SCI_STYLESETFORE, 2, self.dark ? 0x88aa88 : 0x337733);
        Send(doc.editor, SCI_STYLESETFORE, 5, self.dark ? 0xe6bfa0 : 0xa04060);
        Send(doc.editor, SCI_STYLESETFORE, 6, self.dark ? 0x9ac8df : 0x226699);
    }
    NSDictionary *builtIn=NXBuiltIn(doc.builtinLanguage);NSUInteger keyword=0;for(NSString *words in builtIn[@"keywords"])Send(doc.editor,SCI_SETKEYWORDS,keyword++,reinterpret_cast<sptr_t>(words.UTF8String));
    if([custom[@"advancedRules"] boolValue])NXConfigureUserLanguage(doc.editor,custom,doc.identifier);
    Send(doc.editor, SCI_COLOURISE, 0, -1);
    [self applyUserTheme:doc];
    [self.syntaxHashes removeObjectForKey:doc.identifier];
}
- (void)notification:(SCNotification *)notification {
    if (self.restoring) return;
    if(notification->nmhdr.code==SCN_FOCUSIN){
        for(NXDocument *doc in self.documents)doc.activeEditor=nil;
        self.secondaryFocused=self.secondaryVisible && notification->nmhdr.hwndFrom==reinterpret_cast<void *>(Send(self.secondaryEditor,SCI_GETDIRECTPOINTER));
        if(self.secondaryFocused)self.secondaryDocument.activeEditor=self.secondaryEditor;
    }
    if(notification->nmhdr.hwndFrom!=reinterpret_cast<void *>(Send(self.current.editor,SCI_GETDIRECTPOINTER)))return;
    if(notification->nmhdr.code==SCN_UPDATEUI && self.synchronizedScroll && self.secondaryVisible && !self.syncingScroll){
        self.syncingScroll=YES;ScintillaView *other=self.secondaryFocused?(ScintillaView *)self.tabs.selectedTabViewItem.view:self.secondaryEditor;
        if(self.syncVertical)Send(other,SCI_SETFIRSTVISIBLELINE,Send(self.current.editor,SCI_GETFIRSTVISIBLELINE));if(self.syncHorizontal)Send(other,SCI_SETXOFFSET,Send(self.current.editor,SCI_GETXOFFSET));self.syncingScroll=NO;
    }
    if(notification->nmhdr.code==SCN_MODIFIED && (notification->modificationType & (SC_MOD_INSERTTEXT|SC_MOD_DELETETEXT))){[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(updateUserSyntax) object:nil];[self performSelector:@selector(updateUserSyntax) withObject:nil afterDelay:0.15];}
    if (notification->nmhdr.code == SCN_UPDATEUI) [self updateControls];
    [self recordNotification:notification];
}
- (void)updateControls {
    if(self.documentMap){self.documentMap.editor=self.current.editor;self.documentMap.needsDisplay=YES;}
    NXDocument *doc = self.current;
    if (!doc) return;
    for(NXDocument *document in self.documents)[self updateLineNumberMargin:document.primaryEditor];
    if(self.secondaryVisible)[self updateLineNumberMargin:self.secondaryEditor];
    // Leave the active field editor alone while a font value is being entered.
    if (self.window.firstResponder == self.family.currentEditor || self.window.firstResponder == self.size.currentEditor) return;
    NoteQu::SelectionFormat query;
    query.start = Send(doc.editor, SCI_GETSELECTIONSTART); query.end = Send(doc.editor, SCI_GETSELECTIONEND);
    Send(doc.editor, NoteQu::GetFormat, 0, reinterpret_cast<sptr_t>(&query));
    self.family.stringValue = (query.value.mask & NoteQu::Family) ? @(query.value.family.c_str()) : self.preferences[@"font"];
    self.size.stringValue = [NSString stringWithFormat:@"%g", (query.value.mask & NoteQu::Size) ? query.value.size / 100.0 : [self.preferences[@"fontSize"] doubleValue]];
    NSDictionary *custom=[self userLanguage:doc];
    [self.language selectItemWithTitle:custom?[@"User: " stringByAppendingString:custom[@"name"]]:[doc.lexer isEqual:@"null"] ? @"Plain Text" : doc.lexer];
    self.window.title = [NSString stringWithFormat:@"%@%@ — NoteQu", doc.title, doc.dirty ? @" •" : @""];
    if (!self.snapshotFailed) self.status.stringValue = [NSString stringWithFormat:@"  Line %ld   Column %ld   %@   %lu characters",
        (long)Send(doc.editor, SCI_LINEFROMPOSITION, Send(doc.editor, SCI_GETCURRENTPOS)) + 1,
        (long)Send(doc.editor, SCI_GETCOLUMN, Send(doc.editor, SCI_GETCURRENTPOS)) + 1,
        NXEncodingName(doc.encoding), (unsigned long)doc.editor.string.length];
}
- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem {
    self.secondaryFocused=NO;for(NXDocument *doc in self.documents)doc.activeEditor=nil;
    if(!self.restoring&&self.current.identifier){[self.recentTabIDs removeObject:self.current.identifier];[self.recentTabIDs insertObject:self.current.identifier atIndex:0];if(self.recentTabIDs.count>1000)[self.recentTabIDs removeLastObject];}
    [self updateControls];
    [self refreshFunctions];
    [self.window makeFirstResponder:self.current.editor];
}
- (BOOL)snapshot:(NSError **)error {
    if (self.restoring) return YES;
    NSMutableArray *documents = [NSMutableArray new];
    for (NXDocument *doc in self.documents) [documents addObject:doc.record];
    NSDictionary *session = @{@"schema":@1, @"documents":documents,
        @"selected":@([self.tabs indexOfTabViewItem:self.tabs.selectedTabViewItem]), @"dark":@(self.dark), @"wrap":@(self.wrap),
        @"contextLayout":self.contextLayout.settings ?: @{}, @"workspace":self.workspace.record ?: @{}, @"menuLayout":self.menuLayout.settings ?: @{},
        @"preferences":[self.preferences copy] ?: @{}, @"macros":[self.macros copy] ?: @[], @"sidebarVisible":@(self.sidebarVisible),
        @"configuration":[self.configuration copy],@"secondary":[self secondaryRecord]};
    if ([session isEqual:self.lastSnapshot]) return YES;
    if (![self.storage save:session error:error]) return NO;
    self.lastSnapshot = session;
    return YES;
}
- (void)tick:(id)sender {
    NSMutableSet *hidden=[NSMutableSet new];for(NXDocument *doc in self.documents)if(doc.secondaryOnly)[hidden addObject:doc.tab.identifier];((NXTabView *)self.tabs).hiddenDocuments=hidden;
    [self checkMonitoredFiles];
    [self updateSecondaryChoices];
    [(NXTabView *)self.tabs refreshTabs];
    NSError *error = nil;
    self.snapshotFailed = ![self snapshot:&error];
    if (self.snapshotFailed) self.status.stringValue = [@"Recovery snapshot failed: " stringByAppendingString:error.localizedDescription];
    for (NXDocument *doc in self.documents) {doc.tab.label = [NSString stringWithFormat:@"%@%@%@%@",doc.pinned?@"◆ ":@"",doc.tabColor?@[@"",@"🔴 ",@"🟠 ",@"🟢 ",@"🔵 ",@"🟣 "][MIN(5,MAX(0,doc.tabColor))]:@"",doc.title,doc.dirty?@" •":@""];doc.tab.toolTip=doc.path.length?doc.path:doc.title;}
    [(NXTabView *)self.tabs refreshTabs];
    [self updateControls];
    [self refreshFunctions];
    [self updateUserSyntax];
}
- (void)checkUpdates:(id)sender { NXUpdate(@"manual",self.storage.directory); }
- (void)updateSettings:(id)sender { NXUpdate(@"settings",self.storage.directory); }
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)app {
    NSError *error = nil;
    if (![self snapshot:&error]) { Alert(error.localizedDescription); return NSTerminateCancel; }
    return NSTerminateNow;
}
- (BOOL)windowShouldClose:(NSWindow *)window { [NSApp terminate:nil]; return NO; }
- (void)applicationDidResignActive:(NSNotification *)notification { [self tick:nil]; }
- (BOOL)application:(NSApplication *)application openFile:(NSString *)filename {
    if (!self.window) {
        if (!self.pendingFiles) self.pendingFiles = [NSMutableArray new];
        [self.pendingFiles addObject:filename];
        return YES;
    }
    NSError *error = nil;
    BOOL ok = [self openPath:filename error:&error];
    if (!ok) Alert(error.localizedDescription);
    return ok;
}
- (BOOL)openPath:(NSString *)path error:(NSError **)error {
    path=NXCanonicalPath(path);
    for (NXDocument *existing in self.documents) if ([existing.path isEqual:path]) { self.secondaryFocused=NO; self.secondaryDocument.activeEditor=nil; [self activateDocument:existing]; return YES; }
    NSData *disk = [NSData dataWithContentsOfFile:path options:0 error:error];
    if (!disk) return NO;
    NSStringEncoding encoding = NSUTF8StringEncoding; BOOL bom = NO;
    NSData *content = disk;
    const auto *bytes = static_cast<const unsigned char *>(disk.bytes);
    if (disk.length >= 3 && !memcmp(bytes, "\xef\xbb\xbf", 3)) { content = [disk subdataWithRange:NSMakeRange(3, disk.length - 3)]; bom = YES; }
    else if (disk.length >= 2 && bytes[0] == 0xff && bytes[1] == 0xfe) { encoding = NSUTF16LittleEndianStringEncoding; bom = YES; content = [disk subdataWithRange:NSMakeRange(2, disk.length - 2)]; }
    else if (disk.length >= 2 && bytes[0] == 0xfe && bytes[1] == 0xff) { encoding = NSUTF16BigEndianStringEncoding; bom = YES; content = [disk subdataWithRange:NSMakeRange(2, disk.length - 2)]; }
    if(self.openingEncoding&&!bom)encoding=self.openingEncoding;
    NSString *string = NXDecodeEncoding(content,encoding);
    if (!string || [string rangeOfString:[NSString stringWithFormat:@"%C", (unichar)0]].location != NSNotFound) {
        if (error) *error = NXError(@"This file could not be decoded as text. Use Encoding > Open with Legacy Encoding to select its encoding. It has not been modified."); return NO;
    }
    NXDocument *doc = [self newDocument:nil];
    doc.path = path; doc.title = path.lastPathComponent; doc.encoding = encoding; doc.bom = bom; doc.diskHash = NXDigest(disk);
    SetText(doc.editor, [string dataUsingEncoding:NSUTF8StringEncoding]);
    Send(doc.editor, SCI_SETEOLMODE, [string containsString:@"\r\n"] ? SC_EOL_CRLF : [string containsString:@"\r"] && ![string containsString:@"\n"] ? SC_EOL_CR : SC_EOL_LF);
    NSDictionary *lexers = @{@"c":@"cpp", @"h":@"cpp", @"cc":@"cpp", @"cpp":@"cpp", @"hpp":@"cpp", @"m":@"cpp", @"mm":@"cpp", @"js":@"cpp", @"ts":@"cpp", @"java":@"cpp", @"py":@"python", @"json":@"json", @"xml":@"xml", @"html":@"html", @"css":@"css", @"md":@"markdown", @"sh":@"bash", @"sql":@"sql", @"yaml":@"yaml", @"yml":@"yaml", @"swift":@"swift", @"rs":@"rust"};
    doc.lexer = lexers[path.pathExtension.lowercaseString] ?: @"null";
    for(NSDictionary *language in NXLanguageCatalog())if([language[@"extensions"] containsObject:path.pathExtension.lowercaseString]){doc.builtinLanguage=language[@"id"];doc.lexer=language[@"lexer"];break;}
    doc.customLanguage=NXLanguageForPath(self.configuration,path)[@"id"] ?: @"";
    [self configureLexer:doc];
    NSString *sidecar = [path stringByAppendingString:@".notex"];
    if (![NSFileManager.defaultManager fileExistsAtPath:sidecar]) {
        auto old = NoteQu::previousSidecarExtension();
        std::string extension(old.begin(), old.end());
        sidecar = [path stringByAppendingString:@(extension.c_str())];
    }
    if ([NSFileManager.defaultManager fileExistsAtPath:sidecar]) {
        NSData *data = [NSData dataWithContentsOfFile:sidecar];
        NoteQu::State state;
        if (NXReadCompanion(data, Text(doc.editor), state)) Restore(doc.editor, std::move(state));
        else Alert(@"The formatting companion does not match this document or could not be read. The text was opened without applying it; the companion was preserved.");
    }
    Send(doc.editor, SCI_EMPTYUNDOBUFFER); Send(doc.editor, SCI_SETSAVEPOINT);
    [NSDocumentController.sharedDocumentController noteNewRecentDocumentURL:[NSURL fileURLWithPath:path]];
    [self tick:nil];
    return YES;
}
- (void)open:(id)sender {
    NSOpenPanel *panel = [NSOpenPanel openPanel]; panel.allowsMultipleSelection = YES;
    if ([panel runModal] != NSModalResponseOK) return;
    for (NSURL *url in panel.URLs) { NSError *error = nil; if (![self openPath:url.path error:&error]) Alert(error.localizedDescription); }
}
- (BOOL)saveDocument:(NXDocument *)doc path:(NSString *)path error:(NSError **)error {
    path=NXCanonicalPath(path);
    // A recovery snapshot precedes the two-file save; failures leave the tab dirty.
    if (![self snapshot:error]) return NO;
    if ([path isEqual:doc.path]) {
        NSData *existing = [NSData dataWithContentsOfFile:path options:0 error:error];
        if (!existing || ![NXDigest(existing) isEqual:doc.diskHash]) {
            if (error) *error = NXError(@"The file changed outside NoteQu or is no longer readable. Use Save As to preserve your edits without overwriting it."); return NO;
        }
    }
    NSString *companion = [path stringByAppendingString:@".notex"];
    if ([NSFileManager.defaultManager fileExistsAtPath:companion] && !NXOwnedCompanion([NSData dataWithContentsOfFile:companion])) {
        if (error) *error = NXError(@"An unrelated file occupies the .notex companion path. Neither file was overwritten. Choose another filename."); return NO;
    }
    NSData *text = Text(doc.editor);
    NSString *string = [[NSString alloc] initWithData:text encoding:NSUTF8StringEncoding];
    NSData *encoded = NXEncodeText(string,doc.encoding);
    if (!encoded) { if (error) *error = NXError(@"The text cannot be represented in the selected encoding."); return NO; }
    NSMutableData *disk = [NSMutableData new];
    if (doc.bom) {
        if (doc.encoding == NSUTF8StringEncoding) [disk appendBytes:"\xef\xbb\xbf" length:3];
        else if (doc.encoding == NSUTF16LittleEndianStringEncoding) [disk appendBytes:"\xff\xfe" length:2];
        else [disk appendBytes:"\xfe\xff" length:2];
    }
    [disk appendData:encoded];
    if (!NXWrite(disk, path, error)) return NO;
    doc.path = path; doc.title = path.lastPathComponent; doc.diskHash = NXDigest(disk); doc.restoredDirty = YES;
    if (!NXWrite(NXCompanion(text, State(doc.editor)), companion, error)) return NO;
    Send(doc.editor, SCI_SETSAVEPOINT); doc.restoredDirty = NO;
    [self tick:nil];
    return YES;
}
- (BOOL)saveCurrentAs:(BOOL)saveAs {
    NXDocument *doc = self.current;
    NSString *path = doc.path;
    if (saveAs || !path.length) {
        NSSavePanel *panel = [NSSavePanel savePanel]; panel.nameFieldStringValue = doc.title;
        if ([panel runModal] != NSModalResponseOK) return NO;
        path = panel.URL.path;
    }
    NSError *error = nil;
    if (![self saveDocument:doc path:path error:&error]) { Alert(error.localizedDescription); return NO; }
    return YES;
}
- (void)save:(id)sender { [self saveCurrentAs:NO]; }
- (void)saveAs:(id)sender { [self saveCurrentAs:YES]; }
- (void)closeTab:(id)sender {
    NXDocument *doc = self.current;
    if (doc.dirty) {
        NSAlert *alert = [NSAlert new]; alert.messageText = [NSString stringWithFormat:@"Save changes to %@?", doc.title];
        alert.informativeText = @"Closing this tab discards its unsaved changes. Quitting NoteQu instead preserves open tabs for your next session.";
        [alert addButtonWithTitle:@"Save"]; [alert addButtonWithTitle:@"Cancel"]; [alert addButtonWithTitle:@"Discard"];
        auto response = [alert runModal];
        if (response == NSAlertSecondButtonReturn || (response == NSAlertFirstButtonReturn && ![self saveCurrentAs:NO])) return;
    }
    NSError *error = nil;
    if (![self snapshot:&error]) { Alert(error.localizedDescription); return; }
    if(doc==self.secondaryDocument)[self closeSecondary:nil];
    if(doc.path.length){[self.closedPaths addObject:doc.path];if(self.closedPaths.count>100)[self.closedPaths removeObjectAtIndex:0];}
    [self.tabs removeTabViewItem:doc.tab]; [self.documents removeObject:doc];
    if (!self.documents.count) [self newDocument:nil];
    [self tick:nil];
}
- (void)apply:(NoteQu::Format)format fields:(unsigned)fields {
    auto *view = self.current.editor;
    NoteQu::Change change; change.start = Send(view, SCI_GETSELECTIONSTART); change.end = Send(view, SCI_GETSELECTIONEND);
    change.value = format; change.fields = fields;
    Send(view, NoteQu::ApplyFormat, 0, reinterpret_cast<sptr_t>(&change));
    [self.window makeFirstResponder:view]; [self tick:nil];
}
- (void)toggleFormat:(id)sender {
    unsigned field = static_cast<unsigned>([sender tag]);
    NoteQu::SelectionFormat query; auto *view = self.current.editor;
    query.start = Send(view, SCI_GETSELECTIONSTART); query.end = Send(view, SCI_GETSELECTIONEND);
    Send(view, NoteQu::GetFormat, 0, reinterpret_cast<sptr_t>(&query));
    NoteQu::Format format = query.value; format.mask |= field;
    bool mixed = query.mixed & field;
    if (field == NoteQu::Bold) format.bold = mixed || !format.bold;
    if (field == NoteQu::Italic) format.italic = mixed || !format.italic;
    if (field == NoteQu::Underline) format.underline = mixed || !format.underline;
    if (field == NoteQu::Strike) format.strike = mixed || !format.strike;
    [self apply:format fields:field];
}
- (void)changeFamily:(id)sender {
    if (![NSFontManager.sharedFontManager.availableFontFamilies containsObject:self.family.stringValue]) { NSBeep(); return; }
    NoteQu::Format format; format.mask = NoteQu::Family; format.family = self.family.stringValue.UTF8String;
    [self apply:format fields:NoteQu::Family];
}
- (void)changeSize:(id)sender {
    double size = self.size.doubleValue;
    if (!std::isfinite(size) || size < 6 || size > 96) { NSBeep(); return; }
    NoteQu::Format format; format.mask = NoteQu::Size; format.size = static_cast<int>(size * 100);
    [self apply:format fields:NoteQu::Size];
}
- (void)clearFormat:(id)sender { [self apply:NoteQu::Format{} fields:NoteQu::All]; }
- (void)pickColor:(id)sender {
    self.colorField = static_cast<unsigned>([sender tag]);
    NSColorPanel *panel = NSColorPanel.sharedColorPanel; panel.target = self; panel.action = @selector(changeColor:);
    panel.continuous = NO; panel.showsAlpha = NO; [panel orderFront:nil];
}
- (void)changeColor:(NSColorPanel *)panel {
    NSColor *color = [panel.color colorUsingColorSpace:NSColorSpace.deviceRGBColorSpace];
    unsigned rgb = (unsigned)lround(color.redComponent * 255) | ((unsigned)lround(color.greenComponent * 255) << 8) | ((unsigned)lround(color.blueComponent * 255) << 16);
    NoteQu::Format format; format.mask = self.colorField;
    if (self.colorField == NoteQu::Fore) format.fore = rgb; else format.highlight = rgb;
    [self apply:format fields:self.colorField];
}
- (void)makeList:(id)sender { [self toggleList:[sender tag] != 0]; }
- (void)changeLanguage:(id)sender { self.current.builtinLanguage=@""; self.current.customLanguage=self.language.selectedItem.representedObject ?: @"";if(!self.current.customLanguage.length)self.current.lexer = self.language.indexOfSelectedItem == 0 ? @"null" : self.language.titleOfSelectedItem; [self refreshEditorSettings];[self tick:nil]; }
- (void)toggleWrap:(id)sender { self.wrap=!self.wrap; self.preferences[@"wrap"]=@(self.wrap); [self refreshEditorSettings]; [self tick:nil]; }
- (void)toggleDark:(id)sender {
    self.dark = !self.dark;
    self.window.appearance = [NSAppearance appearanceNamed:self.dark ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua];
    self.preferences[@"dark"]=@(self.dark); [self refreshEditorSettings]; [self tick:nil];
}
- (void)editorCommand:(id)sender { Send(self.current.editor, static_cast<unsigned>([sender tag])); }
- (void)nextTab:(id)sender {if([sender tag]<0)[(NXTabView *)self.tabs previous:sender];else[(NXTabView *)self.tabs next:sender];}
- (void)showFind:(id)sender { self.findBar.hidden = NO; [self.window makeFirstResponder:self.findText]; }
- (void)hideFind:(id)sender { self.findBar.hidden = YES; [self.window makeFirstResponder:self.current.editor]; }
- (void)findNext:(id)sender { [self searchNavigate:sender]; }
- (void)replace:(id)sender { [self searchReplace:sender]; }
- (void)goToLine:(id)sender {
    NSAlert *alert = [NSAlert new]; alert.messageText = @"Go to Line";
    NSTextField *field = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 240, 24)]; field.stringValue = @"1";
    alert.accessoryView = field; [alert addButtonWithTitle:@"Go"]; [alert addButtonWithTitle:@"Cancel"];
    if ([alert runModal] == NSAlertFirstButtonReturn) Send(self.current.editor, SCI_GOTOLINE, std::max<NSInteger>(0, field.integerValue - 1));
}
- (void)about:(id)sender {
    [NSApp orderFrontStandardAboutPanelWithOptions:@{NSAboutPanelOptionApplicationName:@"NoteQu",
        NSAboutPanelOptionApplicationVersion:[NSString stringWithFormat:@"%@ — Native macOS Preview %@",[NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"],[NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleVersion"]],
        NSAboutPanelOptionCredits:[[NSAttributedString alloc] initWithString:@"Native AppKit • Scintilla • Lexilla\nCopyright © 2026 Berk Kivircik and contributors.\nGNU General Public License version 3.\nSee Help for source, licensing, and preview limitations."]}];
}
- (void)runToolCommand:(id)sender {
    if(!self.runTool){self.runTool=[NXRunTool new];self.runTool.commands=self.configuration[@"runCommands"] ?: @[];__weak NXApp *owner=self;
        self.runTool.saveCommands=^BOOL(NSArray *commands){NSMutableDictionary *configuration=[owner.configuration mutableCopy];configuration[@"runCommands"]=commands;return [owner commitConfiguration:configuration];};
        self.runTool.documentEnvironment=^NSDictionary *{NXApp *app=owner;NSString *selection=app.current.editor.selectedString;return @{@"NOTEX_FILE":app.current.path ?: @"",@"NOTEX_DIRECTORY":app.current.path.length?app.current.path.stringByDeletingLastPathComponent:NSHomeDirectory(),@"NOTEX_SELECTION":selection,@"NOTEX_LINE":[NSString stringWithFormat:@"%ld",(long)Send(app.current.editor,SCI_LINEFROMPOSITION,Send(app.current.editor,SCI_GETCURRENTPOS))+1]};};
    }[self.runTool showForWindow:self.window];
}
- (void)showSession:(id)sender { [NSWorkspace.sharedWorkspace openURL:[NSURL fileURLWithPath:self.storage.directory]]; }
- (void)guide:(id)sender { [NSWorkspace.sharedWorkspace openURL:[NSBundle.mainBundle URLForResource:@"README" withExtension:@"md"]]; }
- (void)source:(id)sender { [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:@"https://github.com/berkkivircik/NoteQu"]]; }
@end

#include "DocumentCommands.inc"
#include "CommandMenus.inc"
#include "LanguageCatalog.inc"
#include "SessionCommands.inc"
#include "EncodingCommands.inc"
#include "TextCommands.inc"
#include "ClipboardCommands.inc"
#include "SearchCommands.inc"
#include "NavigationCommands.inc"
#include "Completion.inc"
#include "UtilityCommands.inc"
#include "ExtraCommands.inc"
#include "Instances.inc"
#include "ViewTransfer.inc"
#include "StyleOverrides.inc"
#include "Editing.inc"
#include "Features.inc"
#include "Workbench.inc"
#include "FeatureTests.inc"
#include "WorkbenchTests.inc"
#include "ParityTests.inc"
#include "SelfTest.inc"

int main(int argc, const char *argv[]) {
    @autoreleasepool {
        if ([NSProcessInfo.processInfo.arguments containsObject:@"--self-test"]) {
            NSString *profile = NSProcessInfo.processInfo.environment[@"NOTEX_PROFILE"];
            if (!profile.isAbsolutePath || ![profile.lastPathComponent hasPrefix:@"test-"]) {
                fprintf(stderr, "Self-tests require an absolute isolated NOTEX_PROFILE named test-*.\n");
                return 2;
            }
        }
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        NXApp *delegate = [NXApp new];
        NSApp.delegate = delegate;
        [NSApp run];
    }
    return 0;
}
