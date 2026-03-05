// This file is part of Notepad++ project
// Copyright (C)2023 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#import "NotepadPlusWindowController.h"
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import "Scintilla.h"
#import "ScintillaView.h"
#include <stdlib.h>
#include "ILexer.h"
#import "SciLexer.h"
#include "Lexilla.h"

@interface NotepadPlusWindowController ()
- (void)applySyntaxHighlightingForPath:(NSString *)filePath;
- (NSString *)normalizedExtensionForPath:(NSString *)filePath content:(NSString *)content;
- (NSString *)lexerNameForFileExtension:(NSString *)extension;
- (NSString *)displayLanguageNameForLexerName:(NSString *)lexerName extension:(NSString *)ext;
- (void)applyLexerPaletteForLexerName:(NSString *)lexerName fileExtension:(NSString *)ext;
- (void)applyDefaultTheme;
- (void)updateStatusBar;
- (void)updateCursorStatus;
- (void)applyReadOnlyModeForPath:(NSString *)filePath;
- (NSString *)displayNameForEncoding:(NSStringEncoding)encoding;
- (long)detectedEOLModeForContent:(NSString *)content;
@end

@implementation NotepadPlusWindowController

- (instancetype)init {
  // Create the window
  NSRect contentRect = NSMakeRect(100, 100, 800, 600);
  NSWindowStyleMask styleMask =
      NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
      NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;

  NSWindow *window =
      [[NSWindow alloc] initWithContentRect:contentRect
                                  styleMask:styleMask
                                    backing:NSBackingStoreBuffered
                                      defer:NO];

  self = [super initWithWindow:window];
  if (self) {
    _currentFilePath = nil;
    _isDocumentModified = NO;
    _currentLanguageName = @"Plain Text";
    _currentLexerName = @"null";
    _currentLanguageSource = @"default";
    _currentEncodingName = @"UTF-8";

    [self setupWindow];
    [self setupTextView];
    [self setupStatusBar];
    [self setupToolbar];
    [self updateWindowTitle];
  }

  return self;
}

#pragma mark - Window Setup

- (void)setupWindow {
  NSWindow *window = self.window;

  window.title = @"Untitled";
  window.delegate = self;

  // Center the window on screen
  [window center];

  // Set minimum size
  window.minSize = NSMakeSize(400, 300);

  // Enable full screen mode
  window.collectionBehavior = NSWindowCollectionBehaviorFullScreenPrimary;

  // Set appearance to support dark mode
  window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
}

- (void)setupTextView {
  static const CGFloat kStatusBarHeight = 24.0;
  // Create ScintillaView - it manages its own scroll view
  NSRect contentFrame = [[self.window contentView] bounds];
  contentFrame.origin.y = kStatusBarHeight;
  contentFrame.size.height -= kStatusBarHeight;

  self.textView = [[ScintillaView alloc] initWithFrame:contentFrame];
  self.textView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

  // Get the internal scroll view for reference
  self.scrollView = self.textView.scrollView;

  // Configure basic Scintilla properties
  // Set monospaced font
  [self.textView setStringProperty:SCI_STYLESETFONT
                         parameter:STYLE_DEFAULT
                             value:@"Menlo"];
  [self.textView setGeneralProperty:SCI_STYLESETSIZE
                          parameter:STYLE_DEFAULT
                              value:12];

  // Apply default style to all styles
  [self.textView setGeneralProperty:SCI_STYLECLEARALL parameter:0 value:0];

  // Enable undo collection
  [self.textView setGeneralProperty:SCI_SETUNDOCOLLECTION parameter:1 value:0];

  // Set UTF-8 encoding
  [self.textView setGeneralProperty:SCI_SETCODEPAGE
                          parameter:SC_CP_UTF8
                              value:0];

  // Enable line numbers (margin 0)
  [self.textView setGeneralProperty:SCI_SETMARGINTYPEN
                          parameter:0
                              value:SC_MARGIN_NUMBER];
  [self.textView setGeneralProperty:SCI_SETMARGINWIDTHN parameter:0 value:40];

  // Set tab width
  [self.textView setGeneralProperty:SCI_SETTABWIDTH parameter:4 value:0];
  [self.textView setGeneralProperty:SCI_SETINDENT parameter:4 value:0];
  [self.textView setGeneralProperty:SCI_SETUSETABS parameter:0 value:0];

  // Enable caret line highlighting
  [self.textView setGeneralProperty:SCI_SETCARETLINEVISIBLE
                          parameter:1
                              value:0];

  // Set selection colors
  [self.textView setColorProperty:SCI_SETSELBACK
                        parameter:1
                            value:[NSColor selectedTextBackgroundColor]];

  // Set up notification for text changes
  self.textView.delegate = self;

  // Add ScintillaView to window
  [self.window.contentView addSubview:self.textView];

  // Default to plain text lexer on new/untitled documents.
  [self applySyntaxHighlightingForPath:nil];
}

- (void)setupStatusBar {
  static const CGFloat kStatusBarHeight = 24.0;
  NSRect contentBounds = [[self.window contentView] bounds];
  NSRect statusFrame = NSMakeRect(0, 0, contentBounds.size.width, kStatusBarHeight);

  self.statusBarView = [[NSView alloc] initWithFrame:statusFrame];
  self.statusBarView.autoresizingMask = NSViewWidthSizable | NSViewMaxYMargin;
  self.statusBarView.wantsLayer = YES;
  self.statusBarView.layer.backgroundColor = [NSColor controlBackgroundColor].CGColor;

  self.languageLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 4, 220, 16)];
  self.languageLabel.bezeled = NO;
  self.languageLabel.drawsBackground = NO;
  self.languageLabel.editable = NO;
  self.languageLabel.selectable = NO;
  self.languageLabel.font = [NSFont systemFontOfSize:11 weight:NSFontWeightRegular];
  self.languageLabel.textColor = [NSColor secondaryLabelColor];

  self.modifiedLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(240, 4, 120, 16)];
  self.modifiedLabel.bezeled = NO;
  self.modifiedLabel.drawsBackground = NO;
  self.modifiedLabel.editable = NO;
  self.modifiedLabel.selectable = NO;
  self.modifiedLabel.font = [NSFont systemFontOfSize:11 weight:NSFontWeightRegular];
  self.modifiedLabel.textColor = [NSColor secondaryLabelColor];

  self.encodingLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(370, 4, 90, 16)];
  self.encodingLabel.bezeled = NO;
  self.encodingLabel.drawsBackground = NO;
  self.encodingLabel.editable = NO;
  self.encodingLabel.selectable = NO;
  self.encodingLabel.font = [NSFont systemFontOfSize:11 weight:NSFontWeightRegular];
  self.encodingLabel.textColor = [NSColor secondaryLabelColor];

  self.eolLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(470, 4, 90, 16)];
  self.eolLabel.bezeled = NO;
  self.eolLabel.drawsBackground = NO;
  self.eolLabel.editable = NO;
  self.eolLabel.selectable = NO;
  self.eolLabel.font = [NSFont systemFontOfSize:11 weight:NSFontWeightRegular];
  self.eolLabel.textColor = [NSColor secondaryLabelColor];

  self.readOnlyLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(570, 4, 120, 16)];
  self.readOnlyLabel.bezeled = NO;
  self.readOnlyLabel.drawsBackground = NO;
  self.readOnlyLabel.editable = NO;
  self.readOnlyLabel.selectable = NO;
  self.readOnlyLabel.font = [NSFont systemFontOfSize:11 weight:NSFontWeightRegular];
  self.readOnlyLabel.textColor = [NSColor secondaryLabelColor];

  CGFloat rightWidth = 120;
  self.metricsLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(contentBounds.size.width - rightWidth - 400, 4, 120, 16)];
  self.metricsLabel.autoresizingMask = NSViewMinXMargin;
  self.metricsLabel.alignment = NSTextAlignmentRight;
  self.metricsLabel.bezeled = NO;
  self.metricsLabel.drawsBackground = NO;
  self.metricsLabel.editable = NO;
  self.metricsLabel.selectable = NO;
  self.metricsLabel.font = [NSFont systemFontOfSize:11 weight:NSFontWeightRegular];
  self.metricsLabel.textColor = [NSColor secondaryLabelColor];
  self.metricsLabel.stringValue = @"Lines: 1 Len: 0";

  self.indentLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(contentBounds.size.width - rightWidth - 270, 4, 120, 16)];
  self.indentLabel.autoresizingMask = NSViewMinXMargin;
  self.indentLabel.alignment = NSTextAlignmentRight;
  self.indentLabel.bezeled = NO;
  self.indentLabel.drawsBackground = NO;
  self.indentLabel.editable = NO;
  self.indentLabel.selectable = NO;
  self.indentLabel.font = [NSFont systemFontOfSize:11 weight:NSFontWeightRegular];
  self.indentLabel.textColor = [NSColor secondaryLabelColor];
  self.indentLabel.stringValue = @"Indent: Spaces/4";

  self.selectionLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(contentBounds.size.width - rightWidth - 140, 4, 120, 16)];
  self.selectionLabel.autoresizingMask = NSViewMinXMargin;
  self.selectionLabel.alignment = NSTextAlignmentRight;
  self.selectionLabel.bezeled = NO;
  self.selectionLabel.drawsBackground = NO;
  self.selectionLabel.editable = NO;
  self.selectionLabel.selectable = NO;
  self.selectionLabel.font = [NSFont systemFontOfSize:11 weight:NSFontWeightRegular];
  self.selectionLabel.textColor = [NSColor secondaryLabelColor];
  self.selectionLabel.stringValue = @"Sel: 0 | Carets: 1";

  self.cursorLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(contentBounds.size.width - rightWidth - 10, 4, rightWidth, 16)];
  self.cursorLabel.autoresizingMask = NSViewMinXMargin;
  self.cursorLabel.alignment = NSTextAlignmentRight;
  self.cursorLabel.bezeled = NO;
  self.cursorLabel.drawsBackground = NO;
  self.cursorLabel.editable = NO;
  self.cursorLabel.selectable = NO;
  self.cursorLabel.font = [NSFont systemFontOfSize:11 weight:NSFontWeightRegular];
  self.cursorLabel.textColor = [NSColor secondaryLabelColor];
  self.cursorLabel.stringValue = @"Ln -, Col -";

  [self.statusBarView addSubview:self.languageLabel];
  [self.statusBarView addSubview:self.modifiedLabel];
  [self.statusBarView addSubview:self.encodingLabel];
  [self.statusBarView addSubview:self.eolLabel];
  [self.statusBarView addSubview:self.readOnlyLabel];
  [self.statusBarView addSubview:self.metricsLabel];
  [self.statusBarView addSubview:self.indentLabel];
  [self.statusBarView addSubview:self.selectionLabel];
  [self.statusBarView addSubview:self.cursorLabel];
  [self.window.contentView addSubview:self.statusBarView];

  [self updateStatusBar];
}

- (void)setupToolbar {
  // Create toolbar
  NSToolbar *toolbar =
      [[NSToolbar alloc] initWithIdentifier:@"NotepadPlusToolbar"];
  toolbar.displayMode = NSToolbarDisplayModeIconOnly;
  toolbar.allowsUserCustomization = YES;
  toolbar.autosavesConfiguration = YES;

  self.toolbar = toolbar;
  self.window.toolbar = toolbar;

  // TODO: Add toolbar items (New, Open, Save, etc.)
}

- (void)updateWindowTitle {
  NSString *title;

  if (self.currentFilePath) {
    title = [self.currentFilePath lastPathComponent];
  } else {
    title = @"Untitled";
  }

  if (self.currentLanguageName.length > 0) {
    title = [NSString stringWithFormat:@"%@ [%@]", title, self.currentLanguageName];
  }

  if (self.isDocumentModified) {
    title = [NSString stringWithFormat:@"%@ — Edited", title];
  }

  self.window.title = title;
  self.window.representedURL =
      self.currentFilePath ? [NSURL fileURLWithPath:self.currentFilePath] : nil;
  [self updateStatusBar];
}

- (void)updateStatusBar {
  self.languageLabel.stringValue = [NSString stringWithFormat:@"Language: %@ (%@)",
                                                              self.currentLanguageName ?: @"Plain Text",
                                                              self.currentLanguageSource ?: @"default"];
  self.modifiedLabel.stringValue = self.isDocumentModified ? @"State: Edited" : @"State: Saved";
  self.encodingLabel.stringValue = [NSString stringWithFormat:@"Encoding: %@",
                                                              self.currentEncodingName ?: @"UTF-8"];

  long eolMode = [self.textView getGeneralProperty:SCI_GETEOLMODE];
  NSString *eol = @"LF";
  if (eolMode == SC_EOL_CRLF) {
    eol = @"CRLF";
  } else if (eolMode == SC_EOL_CR) {
    eol = @"CR";
  }
  self.eolLabel.stringValue = [NSString stringWithFormat:@"EOL: %@", eol];

  long useTabs = [self.textView getGeneralProperty:SCI_GETUSETABS];
  long tabWidth = [self.textView getGeneralProperty:SCI_GETTABWIDTH];
  self.indentLabel.stringValue = [NSString stringWithFormat:@"Indent: %@/%ld",
                                                            useTabs ? @"Tabs" : @"Spaces",
                                                            tabWidth];

  long isReadOnly = [self.textView getGeneralProperty:SCI_GETREADONLY];
  self.readOnlyLabel.stringValue = isReadOnly ? @"ReadOnly: On" : @"ReadOnly: Off";
  [self updateCursorStatus];
}

- (void)applyReadOnlyModeForPath:(NSString *)filePath {
  BOOL readOnly = NO;
  if (filePath.length > 0) {
    readOnly = ![[NSFileManager defaultManager] isWritableFileAtPath:filePath];
  }
  [self.textView setGeneralProperty:SCI_SETREADONLY parameter:(readOnly ? 1 : 0) value:0];
}

- (NSString *)displayNameForEncoding:(NSStringEncoding)encoding {
  if (encoding == NSUTF8StringEncoding) return @"UTF-8";
  if (encoding == NSUTF16StringEncoding) return @"UTF-16";
  if (encoding == NSUTF16LittleEndianStringEncoding) return @"UTF-16LE";
  if (encoding == NSUTF16BigEndianStringEncoding) return @"UTF-16BE";
  if (encoding == NSUTF32StringEncoding) return @"UTF-32";
  if (encoding == NSMacOSRomanStringEncoding) return @"MacRoman";
  if (encoding == NSISOLatin1StringEncoding) return @"ISO-8859-1";
  return [NSString localizedNameOfStringEncoding:encoding] ?: @"Unknown";
}

- (long)detectedEOLModeForContent:(NSString *)content {
  if (content.length == 0) {
    return SC_EOL_LF;
  }
  if ([content rangeOfString:@"\r\n"].location != NSNotFound) {
    return SC_EOL_CRLF;
  }
  if ([content rangeOfString:@"\r"].location != NSNotFound) {
    return SC_EOL_CR;
  }
  return SC_EOL_LF;
}

- (void)updateCursorStatus {
  if (!self.textView) {
    self.cursorLabel.stringValue = @"Ln -, Col -";
    return;
  }

  sptr_t pos = [self.textView message:SCI_GETCURRENTPOS wParam:0 lParam:0];
  sptr_t line = [self.textView message:SCI_LINEFROMPOSITION wParam:pos lParam:0];
  sptr_t col = [self.textView message:SCI_GETCOLUMN wParam:pos lParam:0];
  sptr_t selStart = [self.textView message:SCI_GETSELECTIONSTART wParam:0 lParam:0];
  sptr_t selEnd = [self.textView message:SCI_GETSELECTIONEND wParam:0 lParam:0];
  sptr_t carets = [self.textView message:SCI_GETSELECTIONS wParam:0 lParam:0];
  sptr_t lineCount = [self.textView message:SCI_GETLINECOUNT wParam:0 lParam:0];
  sptr_t length = [self.textView message:SCI_GETLENGTH wParam:0 lParam:0];
  long long selLen = llabs((long long)selEnd - (long long)selStart);
  if (carets < 1) {
    carets = 1;
  }
  self.metricsLabel.stringValue = [NSString stringWithFormat:@"Lines: %ld Len: %ld",
                                                              (long)lineCount,
                                                              (long)length];
  self.selectionLabel.stringValue = [NSString stringWithFormat:@"Sel: %lld | Carets: %ld",
                                                               selLen, (long)carets];
  self.cursorLabel.stringValue = [NSString stringWithFormat:@"Ln %ld, Col %ld",
                                                             (long)(line + 1),
                                                             (long)(col + 1)];
}

#pragma mark - Document Operations

- (void)newDocument:(id)sender {
  NSLog(@"Creating new document");

  // Check if current document needs saving
  if (self.isDocumentModified) {
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = @"Do you want to save changes?";
    alert.informativeText =
        @"Your changes will be lost if you don't save them.";
    [alert addButtonWithTitle:@"Save"];
    [alert addButtonWithTitle:@"Don't Save"];
    [alert addButtonWithTitle:@"Cancel"];

    NSModalResponse response = [alert runModal];

    if (response == NSAlertFirstButtonReturn) {
      [self saveDocument:sender];
    } else if (response == NSAlertThirdButtonReturn) {
      return; // Cancel
    }
  }

  // Clear the text view
  self.textView.string = @"";
  [self.textView setGeneralProperty:SCI_SETEOLMODE parameter:SC_EOL_LF value:0];
  [self.textView setGeneralProperty:SCI_SETSAVEPOINT parameter:0 value:0];
  self.currentFilePath = nil;
  self.isDocumentModified = NO;
  self.currentEncodingName = @"UTF-8";
  [self applyReadOnlyModeForPath:nil];
  [self applySyntaxHighlightingForPath:nil];
  [self updateWindowTitle];
}

- (void)openDocument:(id)sender {
  NSLog(@"Opening document");

  NSOpenPanel *openPanel = [NSOpenPanel openPanel];
  openPanel.canChooseFiles = YES;
  openPanel.canChooseDirectories = NO;
  openPanel.allowsMultipleSelection = NO;

  [openPanel beginSheetModalForWindow:self.window
                    completionHandler:^(NSModalResponse result) {
                      if (result == NSModalResponseOK) {
                        NSURL *url = openPanel.URL;
                        if (url) {
                          [self openFile:url.path];
                        }
                      }
                    }];
}

- (void)openFile:(NSString *)filePath {
  NSLog(@"Opening file: %@", filePath);

  NSError *error = nil;
  NSStringEncoding usedEncoding = NSUTF8StringEncoding;
  NSString *content = [NSString stringWithContentsOfFile:filePath
                                             usedEncoding:&usedEncoding
                                                    error:&error];

  if (error) {
    NSLog(@"Error opening file: %@", error.localizedDescription);

    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = @"Error Opening File";
    alert.informativeText = error.localizedDescription;
    [alert addButtonWithTitle:@"OK"];
    alert.alertStyle = NSAlertStyleCritical;
    [alert runModal];

    return;
  }

  self.textView.string = content;
  [self.textView setGeneralProperty:SCI_SETEOLMODE
                          parameter:[self detectedEOLModeForContent:content]
                              value:0];
  [self applySyntaxHighlightingForPath:filePath];
  [self.textView setGeneralProperty:SCI_SETSAVEPOINT parameter:0 value:0];
  self.currentFilePath = filePath;
  self.isDocumentModified = NO;
  self.currentEncodingName = [self displayNameForEncoding:usedEncoding];
  [self applyReadOnlyModeForPath:filePath];
  [self updateWindowTitle];
}

- (void)saveDocument:(id)sender {
  NSLog(@"Saving document");

  if (self.currentFilePath) {
    [self saveToFile:self.currentFilePath];
  } else {
    [self saveDocumentAs:sender];
  }
}

- (void)saveDocumentAs:(id)sender {
  NSLog(@"Save document as");

  NSSavePanel *savePanel = [NSSavePanel savePanel];
  if (@available(macOS 12.0, *)) {
    NSMutableArray<UTType *> *types = [NSMutableArray array];
    NSArray<NSString *> *extensions = @[
      @"txt", @"c", @"cpp", @"h", @"hpp", @"m", @"mm", @"py", @"js", @"ts",
      @"html", @"xml", @"css", @"json", @"md", @"sh", @"sql", @"rs", @"toml",
      @"yaml", @"yml"
    ];
    for (NSString *ext in extensions) {
      UTType *type = [UTType typeWithFilenameExtension:ext];
      if (type != nil) {
        [types addObject:type];
      }
    }
    savePanel.allowedContentTypes = types;
  } else {
    savePanel.allowedFileTypes = @[
      @"txt", @"c", @"cpp", @"h", @"hpp", @"m", @"mm", @"py", @"js", @"ts",
      @"html", @"xml", @"css", @"json", @"md", @"sh", @"sql", @"rs", @"toml",
      @"yaml", @"yml"
    ];
  }
  savePanel.allowsOtherFileTypes = YES;

  if (self.currentFilePath) {
    savePanel.nameFieldStringValue = [self.currentFilePath lastPathComponent];
  }

  [savePanel beginSheetModalForWindow:self.window
                    completionHandler:^(NSModalResponse result) {
                      if (result == NSModalResponseOK) {
                        NSURL *url = savePanel.URL;
                        if (url) {
                          [self saveToFile:url.path];
                        }
                      }
                    }];
}

- (void)saveToFile:(NSString *)filePath {
  NSError *error = nil;
  NSString *content = self.textView.string;

  [content writeToFile:filePath
            atomically:YES
              encoding:NSUTF8StringEncoding
                 error:&error];

  if (error) {
    NSLog(@"Error saving file: %@", error.localizedDescription);

    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = @"Error Saving File";
    alert.informativeText = error.localizedDescription;
    [alert addButtonWithTitle:@"OK"];
    alert.alertStyle = NSAlertStyleCritical;
    [alert runModal];

    return;
  }

  self.currentFilePath = filePath;
  self.currentEncodingName = @"UTF-8";
  [self applySyntaxHighlightingForPath:filePath];
  [self.textView setGeneralProperty:SCI_SETSAVEPOINT parameter:0 value:0];
  self.isDocumentModified = NO;
  [self applyReadOnlyModeForPath:filePath];
  [self updateWindowTitle];

  NSLog(@"File saved successfully: %@", filePath);
}

#pragma mark - Text Change Notification

- (void)textDidChange:(NSNotification *)notification {
#pragma unused(notification)
  if (!self.isDocumentModified) {
    self.isDocumentModified = YES;
    [self updateWindowTitle];
  }
}

- (void)notification:(SCNotification *)notification {
  switch (notification->nmhdr.code) {
  case SCN_SAVEPOINTREACHED:
    NSLog(@"[NPP][State] Savepoint reached");
    self.isDocumentModified = NO;
    [self updateWindowTitle];
    break;
  case SCN_SAVEPOINTLEFT:
    NSLog(@"[NPP][State] Savepoint left");
    self.isDocumentModified = YES;
    [self updateWindowTitle];
    break;
  case SCN_MODIFIED:
    if (notification->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)) {
      NSLog(@"[NPP][State] Modified event type=%d", notification->modificationType);
      self.isDocumentModified = YES;
      [self updateWindowTitle];
    }
    break;
  case SCN_UPDATEUI:
    [self updateCursorStatus];
    break;
  default:
    break;
  }
}

#pragma mark - Syntax Highlighting

- (NSString *)normalizedExtensionForPath:(NSString *)filePath content:(NSString *)content {
  NSString *name = filePath.lastPathComponent.lowercaseString ?: @"";
  if ([name isEqualToString:@"cmakelists.txt"]) {
    self.currentLanguageSource = @"filename";
    return @"cmake";
  }
  if ([name isEqualToString:@"makefile"] || [name isEqualToString:@"gnumakefile"]) {
    self.currentLanguageSource = @"filename";
    return @"mk";
  }

  NSString *ext = filePath.pathExtension.lowercaseString ?: @"";
  if (ext.length > 0) {
    self.currentLanguageSource = @"extension";
    return ext;
  }

  if ([name hasPrefix:@".bash"] || [name isEqualToString:@".profile"] ||
      [name isEqualToString:@".zshrc"]) {
    self.currentLanguageSource = @"filename";
    return @"sh";
  }

  if ([content hasPrefix:@"#!"]) {
    NSRange lineBreak = [content rangeOfString:@"\n"];
    NSString *shebang = (lineBreak.location == NSNotFound)
                            ? content
                            : [content substringToIndex:lineBreak.location];
    NSString *lower = shebang.lowercaseString;
    if ([lower containsString:@"python"]) {
      self.currentLanguageSource = @"shebang";
      return @"py";
    }
    if ([lower containsString:@"bash"] || [lower containsString:@"zsh"] ||
        [lower containsString:@"sh"]) {
      self.currentLanguageSource = @"shebang";
      return @"sh";
    }
    if ([lower containsString:@"node"]) {
      self.currentLanguageSource = @"shebang";
      return @"js";
    }
  }

  self.currentLanguageSource = @"default";
  return @"";
}

- (NSString *)lexerNameForFileExtension:(NSString *)extension {
  if (extension.length == 0) {
    return @"null";
  }

  NSString *ext = extension.lowercaseString;
  if ([ext isEqualToString:@"m"] || [ext isEqualToString:@"mm"]) {
    return @"objc";
  }
  if ([ext isEqualToString:@"c"] || [ext isEqualToString:@"cc"] ||
      [ext isEqualToString:@"cpp"] || [ext isEqualToString:@"cxx"] ||
      [ext isEqualToString:@"h"] || [ext isEqualToString:@"hpp"] ||
      [ext isEqualToString:@"hh"]) {
    return @"cpp";
  }
  if ([ext isEqualToString:@"py"] || [ext isEqualToString:@"pyw"]) {
    return @"python";
  }
  // Lexilla does not provide a standalone "javascript"/"typescript" lexer
  // module in this tree. Use cpp lexer with JS/TS keyword sets.
  if ([ext isEqualToString:@"js"] || [ext isEqualToString:@"jsx"] ||
      [ext isEqualToString:@"ts"] || [ext isEqualToString:@"tsx"]) {
    return @"cpp";
  }
  if ([ext isEqualToString:@"html"] || [ext isEqualToString:@"htm"] ||
      [ext isEqualToString:@"xhtml"] || [ext isEqualToString:@"php"]) {
    return @"hypertext";
  }
  if ([ext isEqualToString:@"xml"] || [ext isEqualToString:@"xsd"] ||
      [ext isEqualToString:@"xsl"] || [ext isEqualToString:@"svg"]) {
    return @"xml";
  }
  if ([ext isEqualToString:@"css"]) {
    return @"css";
  }
  if ([ext isEqualToString:@"cmake"]) {
    return @"cmake";
  }
  if ([ext isEqualToString:@"mk"]) {
    return @"makefile";
  }
  if ([ext isEqualToString:@"yml"] || [ext isEqualToString:@"yaml"]) {
    return @"yaml";
  }
  if ([ext isEqualToString:@"toml"]) {
    return @"toml";
  }
  if ([ext isEqualToString:@"json"]) {
    return @"json";
  }
  if ([ext isEqualToString:@"sql"]) {
    return @"sql";
  }
  if ([ext isEqualToString:@"md"] || [ext isEqualToString:@"markdown"]) {
    return @"markdown";
  }
  if ([ext isEqualToString:@"sh"] || [ext isEqualToString:@"bash"] ||
      [ext isEqualToString:@"zsh"]) {
    return @"bash";
  }
  if ([ext isEqualToString:@"rs"]) {
    return @"rust";
  }
  return @"null";
}

- (void)applyDefaultTheme {
  // Re-apply base styling so lexer-specific styles inherit sane defaults.
  [self.textView setStringProperty:SCI_STYLESETFONT parameter:STYLE_DEFAULT value:@"Menlo"];
  [self.textView setGeneralProperty:SCI_STYLESETSIZE parameter:STYLE_DEFAULT value:12];
  [self.textView setColorProperty:SCI_STYLESETFORE parameter:STYLE_DEFAULT value:[NSColor textColor]];
  [self.textView setColorProperty:SCI_STYLESETBACK parameter:STYLE_DEFAULT value:[NSColor textBackgroundColor]];
  [self.textView setGeneralProperty:SCI_STYLECLEARALL parameter:0 value:0];

  [self.textView setColorProperty:SCI_STYLESETFORE parameter:STYLE_LINENUMBER value:[NSColor secondaryLabelColor]];
  [self.textView setColorProperty:SCI_STYLESETBACK parameter:STYLE_LINENUMBER value:[NSColor controlBackgroundColor]];
}

- (NSString *)displayLanguageNameForLexerName:(NSString *)lexerName extension:(NSString *)ext {
  if ([lexerName isEqualToString:@"objc"]) return @"Objective-C";
  if ([lexerName isEqualToString:@"cpp"]) {
    if ([ext isEqualToString:@"js"] || [ext isEqualToString:@"jsx"]) return @"JavaScript";
    if ([ext isEqualToString:@"ts"] || [ext isEqualToString:@"tsx"]) return @"TypeScript";
    return @"C/C++";
  }
  if ([lexerName isEqualToString:@"python"]) return @"Python";
  if ([lexerName isEqualToString:@"rust"]) return @"Rust";
  if ([lexerName isEqualToString:@"bash"]) return @"Shell";
  if ([lexerName isEqualToString:@"sql"]) return @"SQL";
  if ([lexerName isEqualToString:@"json"]) return @"JSON";
  if ([lexerName isEqualToString:@"markdown"]) return @"Markdown";
  if ([lexerName isEqualToString:@"hypertext"]) return @"HTML";
  if ([lexerName isEqualToString:@"xml"]) return @"XML";
  if ([lexerName isEqualToString:@"css"]) return @"CSS";
  if ([lexerName isEqualToString:@"yaml"]) return @"YAML";
  if ([lexerName isEqualToString:@"toml"]) return @"TOML";
  if ([lexerName isEqualToString:@"cmake"]) return @"CMake";
  if ([lexerName isEqualToString:@"makefile"]) return @"Makefile";
  return @"Plain Text";
}

- (void)applyLexerPaletteForLexerName:(NSString *)lexerName fileExtension:(NSString *)ext {
  if ([lexerName isEqualToString:@"cpp"] || [lexerName isEqualToString:@"objc"]) {
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_C_COMMENT fromHTML:@"#5A8F58"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_C_COMMENTLINE fromHTML:@"#5A8F58"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_C_COMMENTDOC fromHTML:@"#5A8F58"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_C_WORD fromHTML:@"#1F5FBF"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_C_WORD2 fromHTML:@"#7A3E9D"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_C_STRING fromHTML:@"#B2561A"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_C_CHARACTER fromHTML:@"#B2561A"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_C_NUMBER fromHTML:@"#7A3E9D"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_C_PREPROCESSOR fromHTML:@"#A03E8A"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_C_OPERATOR fromHTML:@"#3A3F45"];
  } else if ([lexerName isEqualToString:@"python"]) {
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_P_COMMENTLINE fromHTML:@"#5A8F58"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_P_COMMENTBLOCK fromHTML:@"#5A8F58"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_P_WORD fromHTML:@"#1F5FBF"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_P_WORD2 fromHTML:@"#7A3E9D"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_P_STRING fromHTML:@"#B2561A"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_P_CHARACTER fromHTML:@"#B2561A"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_P_NUMBER fromHTML:@"#7A3E9D"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_P_DECORATOR fromHTML:@"#A03E8A"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_P_OPERATOR fromHTML:@"#3A3F45"];
  } else if ([lexerName isEqualToString:@"sql"]) {
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_SQL_COMMENT fromHTML:@"#5A8F58"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_SQL_COMMENTLINE fromHTML:@"#5A8F58"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_SQL_WORD fromHTML:@"#1F5FBF"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_SQL_WORD2 fromHTML:@"#7A3E9D"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_SQL_STRING fromHTML:@"#B2561A"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_SQL_CHARACTER fromHTML:@"#B2561A"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_SQL_NUMBER fromHTML:@"#7A3E9D"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_SQL_OPERATOR fromHTML:@"#3A3F45"];
  } else if ([lexerName isEqualToString:@"bash"]) {
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_SH_COMMENTLINE fromHTML:@"#5A8F58"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_SH_WORD fromHTML:@"#1F5FBF"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_SH_STRING fromHTML:@"#B2561A"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_SH_CHARACTER fromHTML:@"#B2561A"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_SH_NUMBER fromHTML:@"#7A3E9D"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_SH_OPERATOR fromHTML:@"#3A3F45"];
  } else if ([lexerName isEqualToString:@"json"]) {
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_JSON_PROPERTYNAME fromHTML:@"#186E86"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_JSON_STRING fromHTML:@"#B2561A"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_JSON_NUMBER fromHTML:@"#7A3E9D"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_JSON_KEYWORD fromHTML:@"#1F5FBF"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_JSON_LINECOMMENT fromHTML:@"#5A8F58"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_JSON_BLOCKCOMMENT fromHTML:@"#5A8F58"];
  } else if ([lexerName isEqualToString:@"markdown"]) {
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_MARKDOWN_HEADER1 fromHTML:@"#1F5FBF"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_MARKDOWN_HEADER2 fromHTML:@"#1F5FBF"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_MARKDOWN_HEADER3 fromHTML:@"#1F5FBF"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_MARKDOWN_STRONG1 fromHTML:@"#B2561A"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_MARKDOWN_STRONG2 fromHTML:@"#B2561A"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_MARKDOWN_CODE fromHTML:@"#7A3E9D"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_MARKDOWN_CODE2 fromHTML:@"#7A3E9D"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_MARKDOWN_LINK fromHTML:@"#186E86"];
  } else if ([lexerName isEqualToString:@"hypertext"] || [lexerName isEqualToString:@"xml"]) {
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_H_TAG fromHTML:@"#1F5FBF"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_H_TAGEND fromHTML:@"#1F5FBF"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_H_ATTRIBUTE fromHTML:@"#7A3E9D"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_H_DOUBLESTRING fromHTML:@"#B2561A"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_H_SINGLESTRING fromHTML:@"#B2561A"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_H_COMMENT fromHTML:@"#5A8F58"];
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_H_NUMBER fromHTML:@"#7A3E9D"];
  }

  if ([lexerName isEqualToString:@"cpp"] &&
      ([ext isEqualToString:@"js"] || [ext isEqualToString:@"jsx"] ||
       [ext isEqualToString:@"ts"] || [ext isEqualToString:@"tsx"])) {
    // Differentiate JS/TS files sharing cpp lexer.
    [self.textView setColorProperty:SCI_STYLESETFORE parameter:SCE_C_WORD fromHTML:@"#0E7490"];
  }
}

- (void)applySyntaxHighlightingForPath:(NSString *)filePath {
  NSString *ext = [self normalizedExtensionForPath:filePath content:self.textView.string];
  NSString *lexerName = @"null";
  if (ext.length > 0) {
    lexerName = [self lexerNameForFileExtension:ext];
  }
  self.currentLanguageName = [self displayLanguageNameForLexerName:lexerName extension:ext];
  self.currentLexerName = lexerName;
  NSLog(@"[NPP][Lexer] %@ -> %@", ext.length ? ext : @"(none)", lexerName);
  Scintilla::ILexer5 *lexer = CreateLexer(lexerName.UTF8String);
  [self.textView message:SCI_SETILEXER wParam:0 lParam:reinterpret_cast<sptr_t>(lexer)];

  // Basic keyword sets for lexers that benefit immediately.
  if ([lexerName isEqualToString:@"cpp"] &&
      !([ext isEqualToString:@"js"] || [ext isEqualToString:@"jsx"] ||
        [ext isEqualToString:@"ts"] || [ext isEqualToString:@"tsx"])) {
    const char *cppKeywords =
        "alignas alignof and and_eq asm auto bitand bitor bool break case catch "
        "char class const constexpr consteval constinit continue decltype default "
        "delete do double dynamic_cast else enum explicit export extern false float "
        "for friend goto if inline int long mutable namespace new noexcept not "
        "nullptr operator or private protected public register reinterpret_cast "
        "return short signed sizeof static static_assert struct switch template this "
        "thread_local throw true try typedef typeid typename union unsigned using "
        "virtual void volatile wchar_t while xor";
    [self.textView setReferenceProperty:SCI_SETKEYWORDS parameter:0 value:cppKeywords];
  } else if ([lexerName isEqualToString:@"cpp"] &&
             ([ext isEqualToString:@"js"] || [ext isEqualToString:@"jsx"])) {
    const char *jsKeywords =
        "await break case catch class const continue debugger default delete do else "
        "export extends finally for function if import in instanceof let new return "
        "super switch this throw try typeof var void while with yield async true false null";
    [self.textView setReferenceProperty:SCI_SETKEYWORDS parameter:0 value:jsKeywords];
  } else if ([lexerName isEqualToString:@"cpp"] &&
             ([ext isEqualToString:@"ts"] || [ext isEqualToString:@"tsx"])) {
    const char *tsKeywords =
        "abstract any as asserts async await bigint boolean break case catch class "
        "const constructor continue debugger declare default delete do else enum export "
        "extends false finally for from function get if implements import in infer "
        "instanceof interface is keyof let module namespace never new null number object "
        "of out override private protected public readonly return set static string super "
        "switch symbol this throw true try type typeof undefined unique unknown var void "
        "while with yield";
    [self.textView setReferenceProperty:SCI_SETKEYWORDS parameter:0 value:tsKeywords];
  } else if ([lexerName isEqualToString:@"python"]) {
    const char *pyKeywords =
        "and as assert async await break class continue def del elif else except "
        "False finally for from global if import in is lambda None nonlocal not or "
        "pass raise return True try while with yield";
    [self.textView setReferenceProperty:SCI_SETKEYWORDS parameter:0 value:pyKeywords];
  } else if ([lexerName isEqualToString:@"rust"]) {
    const char *rustKeywords =
        "as async await break const continue crate dyn else enum extern false fn for if "
        "impl in let loop match mod move mut pub ref return self Self static struct super "
        "trait true type unsafe use where while";
    [self.textView setReferenceProperty:SCI_SETKEYWORDS parameter:0 value:rustKeywords];
  } else if ([lexerName isEqualToString:@"bash"]) {
    const char *bashKeywords =
        "if then elif else fi case esac for while until do done function in select time "
        "coproc readonly local declare typeset export unset return break continue";
    [self.textView setReferenceProperty:SCI_SETKEYWORDS parameter:0 value:bashKeywords];
  } else if ([lexerName isEqualToString:@"sql"]) {
    const char *sqlKeywords =
        "select from where insert update delete create alter drop table view index join "
        "left right inner outer on group by order having distinct union all into values "
        "primary key foreign not null default check constraint";
    [self.textView setReferenceProperty:SCI_SETKEYWORDS parameter:0 value:sqlKeywords];
  }

  [self applyDefaultTheme];
  [self applyLexerPaletteForLexerName:lexerName fileExtension:ext];
  [self.textView setGeneralProperty:SCI_COLOURISE parameter:0 value:-1];
  [self updateStatusBar];
}

#pragma mark - Self Test

- (BOOL)runSelfTestWithFilePath:(NSString *)filePath {
  NSLog(@"[NPP][SelfTest] START file=%@", filePath ?: @"(nil)");
  if (filePath.length == 0) {
    NSLog(@"[NPP][SelfTest] FAIL reason=missing_file_path");
    return NO;
  }

  [self openFile:filePath];
  if (!self.currentFilePath || ![[self.currentFilePath stringByStandardizingPath] isEqualToString:[filePath stringByStandardizingPath]]) {
    NSLog(@"[NPP][SelfTest] FAIL reason=open_failed");
    return NO;
  }

  NSLog(@"[NPP][SelfTest] LEXER=%@", self.currentLexerName ?: @"null");
  NSLog(@"[NPP][SelfTest] LANG_SOURCE=%@", self.currentLanguageSource ?: @"default");
  long eolMode = [self.textView getGeneralProperty:SCI_GETEOLMODE];
  NSString *eol = @"LF";
  if (eolMode == SC_EOL_CRLF) {
    eol = @"CRLF";
  } else if (eolMode == SC_EOL_CR) {
    eol = @"CR";
  }
  long isReadOnly = [self.textView getGeneralProperty:SCI_GETREADONLY];
  NSLog(@"[NPP][SelfTest] STATUS=encoding:%@;eol:%@;readonly:%ld",
        self.currentEncodingName ?: @"UTF-8", eol, isReadOnly);
  [self.textView setGeneralProperty:SCI_SETREADONLY parameter:0 value:0];
  long modifyBefore = [self.textView getGeneralProperty:SCI_GETMODIFY];
  NSLog(@"[NPP][SelfTest] MODIFY_BEFORE=%ld", modifyBefore);

  const char *probe = "npp-selftest\n";
  [self.textView message:SCI_INSERTTEXT wParam:0 lParam:reinterpret_cast<sptr_t>(probe)];
  long modifyAfterInsert = [self.textView getGeneralProperty:SCI_GETMODIFY];
  NSLog(@"[NPP][SelfTest] MODIFY_AFTER_INSERT=%ld", modifyAfterInsert);

  [self.textView setGeneralProperty:SCI_SETSAVEPOINT parameter:0 value:0];
  long modifyAfterSavepoint = [self.textView getGeneralProperty:SCI_GETMODIFY];
  NSLog(@"[NPP][SelfTest] MODIFY_AFTER_SAVEPOINT=%ld", modifyAfterSavepoint);

  BOOL passed = (modifyBefore == 0 && modifyAfterInsert == 1 && modifyAfterSavepoint == 0);
  NSLog(@"[NPP][SelfTest] %@", passed ? @"PASS" : @"FAIL");
  return passed;
}

#pragma mark - Window Delegate

- (BOOL)windowShouldClose:(NSWindow *)sender {
  if (self.isDocumentModified) {
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = @"Do you want to save changes?";
    alert.informativeText =
        @"Your changes will be lost if you don't save them.";
    [alert addButtonWithTitle:@"Save"];
    [alert addButtonWithTitle:@"Don't Save"];
    [alert addButtonWithTitle:@"Cancel"];

    NSModalResponse response = [alert runModal];

    if (response == NSAlertFirstButtonReturn) {
      [self saveDocument:nil];
      return YES;
    } else if (response == NSAlertThirdButtonReturn) {
      return NO; // Cancel close
    }
  }

  return YES;
}

- (void)windowWillClose:(NSNotification *)notification {
  #pragma unused(notification)
}

@end
