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

#import <Cocoa/Cocoa.h>
#import "ScintillaView.h"

// Forward declaration
@class ScintillaView;

// Window controller for the main Notepad++ window
// This manages the main editing window and its components
@interface NotepadPlusWindowController : NSWindowController <NSWindowDelegate, ScintillaNotificationProtocol>

// Window components
@property (strong, nonatomic) ScintillaView* textView;
@property (strong, nonatomic) NSScrollView* scrollView;
@property (strong, nonatomic) NSToolbar* toolbar;

// Current document
@property (strong, nonatomic) NSString* currentFilePath;
@property (assign, nonatomic) BOOL isDocumentModified;
@property (strong, nonatomic) NSString* currentLanguageName;

// Initialization
- (instancetype)init;

// Document operations
- (void)newDocument:(id)sender;
- (void)openDocument:(id)sender;
- (void)openFile:(NSString*)filePath;
- (void)saveDocument:(id)sender;
- (void)saveDocumentAs:(id)sender;

// Window management
- (void)setupWindow;
- (void)setupToolbar;
- (void)setupTextView;
- (void)updateWindowTitle;

@end
