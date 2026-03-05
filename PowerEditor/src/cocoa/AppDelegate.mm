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

#import "AppDelegate.h"
#import "NotepadPlusWindowController.h"
#import <Foundation/Foundation.h>
#include <stdlib.h>

@implementation AppDelegate

- (instancetype)init {
    self = [super init];
    if (self) {
        _mainWindowController = nil;
    }
    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSLog(@"Notepad++ for macOS - Application launching");
    
    // Create the main window controller
    self.mainWindowController = [[NotepadPlusWindowController alloc] init];
    
    // Show the main window
    [self.mainWindowController showWindow:self];
    
    // Set up the application menu
    [self setupApplicationMenu];

    // Optional non-interactive self-test mode for CI/automation.
    const char *selfTestEnv = getenv("NPP_SELFTEST");
    if (selfTestEnv && strcmp(selfTestEnv, "1") == 0) {
        const char *selfTestFile = getenv("NPP_SELFTEST_FILE");
        NSString *filePath = selfTestFile ? [NSString stringWithUTF8String:selfTestFile] : @"";
        BOOL ok = [self.mainWindowController runSelfTestWithFilePath:filePath];
        NSLog(@"[NPP][SelfTest] EXIT=%d", ok ? 0 : 1);
        fflush(stdout);
        fflush(stderr);
        exit(ok ? 0 : 1);
        return;
    }
    
    NSLog(@"Notepad++ main window created and shown");
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    NSLog(@"Notepad++ for macOS - Application terminating");
    
    // TODO: Save session, preferences, etc.
    // TODO: Close all open documents
    // TODO: Cleanup resources
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    // On macOS, apps typically stay running even when all windows are closed
    // But for a text editor, it makes sense to quit when the last window closes
    return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
    // TODO: Check for unsaved documents
    // TODO: Prompt user to save if needed
    
    // For now, allow termination
    return NSTerminateNow;
}

#pragma mark - File Handling

- (BOOL)application:(NSApplication *)sender openFile:(NSString *)filename {
    NSLog(@"Opening file: %@", filename);
    
    if (self.mainWindowController) {
        [self.mainWindowController openFile:filename];
        return YES;
    }
    
    return NO;
}

- (void)application:(NSApplication *)sender openFiles:(NSArray<NSString *> *)filenames {
    NSLog(@"Opening %lu files", (unsigned long)[filenames count]);
    
    if (self.mainWindowController) {
        for (NSString* filename in filenames) {
            [self.mainWindowController openFile:filename];
        }
        [sender replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
    } else {
        [sender replyToOpenOrPrint:NSApplicationDelegateReplyFailure];
    }
}

#pragma mark - Menu Actions

- (IBAction)newDocument:(id)sender {
    NSLog(@"New document requested");
    if (self.mainWindowController) {
        [self.mainWindowController newDocument:sender];
    }
}

- (IBAction)openDocument:(id)sender {
    NSLog(@"Open document requested");
    if (self.mainWindowController) {
        [self.mainWindowController openDocument:sender];
    }
}

- (IBAction)showPreferences:(id)sender {
    NSLog(@"Preferences requested");
    // TODO: Show preferences window
    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"Preferences";
    alert.informativeText = @"Preferences dialog not yet implemented.";
    [alert addButtonWithTitle:@"OK"];
    [alert runModal];
}

- (IBAction)showAbout:(id)sender {
    NSLog(@"About requested");
    
    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"Notepad++ for macOS";
    alert.informativeText = @"Version 8.6.0\n\nA free source code editor and Notepad replacement.\n\nCopyright © 2023 Notepad++ Team\nLicensed under GPL v3";
    [alert addButtonWithTitle:@"OK"];
    alert.alertStyle = NSAlertStyleInformational;
    [alert runModal];
}

#pragma mark - Menu Setup

- (void)setupApplicationMenu {
    // Create the main menu bar
    NSMenu* mainMenu = [[NSMenu alloc] initWithTitle:@"MainMenu"];
    
    // Application menu (Notepad++)
    NSMenuItem* appMenuItem = [[NSMenuItem alloc] init];
    NSMenu* appMenu = [[NSMenu alloc] initWithTitle:@"Notepad++"];
    
    [appMenu addItemWithTitle:@"About Notepad++"
                       action:@selector(showAbout:)
                keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    
    [appMenu addItemWithTitle:@"Preferences..."
                       action:@selector(showPreferences:)
                keyEquivalent:@","];
    [appMenu addItem:[NSMenuItem separatorItem]];
    
    [appMenu addItemWithTitle:@"Hide Notepad++"
                       action:@selector(hide:)
                keyEquivalent:@"h"];
    
    NSMenuItem* hideOthersItem = [appMenu addItemWithTitle:@"Hide Others"
                                                    action:@selector(hideOtherApplications:)
                                             keyEquivalent:@"h"];
    hideOthersItem.keyEquivalentModifierMask = NSEventModifierFlagOption | NSEventModifierFlagCommand;
    
    [appMenu addItemWithTitle:@"Show All"
                       action:@selector(unhideAllApplications:)
                keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    
    [appMenu addItemWithTitle:@"Quit Notepad++"
                       action:@selector(terminate:)
                keyEquivalent:@"q"];
    
    [appMenuItem setSubmenu:appMenu];
    [mainMenu addItem:appMenuItem];
    
    // File menu
    NSMenuItem* fileMenuItem = [[NSMenuItem alloc] init];
    NSMenu* fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
    
    [fileMenu addItemWithTitle:@"New"
                        action:@selector(newDocument:)
                 keyEquivalent:@"n"];
    
    [fileMenu addItemWithTitle:@"Open..."
                        action:@selector(openDocument:)
                 keyEquivalent:@"o"];
    
    [fileMenu addItem:[NSMenuItem separatorItem]];
    
    [fileMenu addItemWithTitle:@"Save"
                        action:@selector(saveDocument:)
                 keyEquivalent:@"s"];
    
    [fileMenu addItemWithTitle:@"Save As..."
                        action:@selector(saveDocumentAs:)
                 keyEquivalent:@"S"];
    
    [fileMenu addItem:[NSMenuItem separatorItem]];
    
    [fileMenu addItemWithTitle:@"Close"
                        action:@selector(performClose:)
                 keyEquivalent:@"w"];
    
    [fileMenuItem setSubmenu:fileMenu];
    [mainMenu addItem:fileMenuItem];
    
    // Edit menu
    NSMenuItem* editMenuItem = [[NSMenuItem alloc] init];
    NSMenu* editMenu = [[NSMenu alloc] initWithTitle:@"Edit"];
    
    [editMenu addItemWithTitle:@"Undo"
                        action:@selector(undo:)
                 keyEquivalent:@"z"];
    
    [editMenu addItemWithTitle:@"Redo"
                        action:@selector(redo:)
                 keyEquivalent:@"Z"];
    
    [editMenu addItem:[NSMenuItem separatorItem]];
    
    [editMenu addItemWithTitle:@"Cut"
                        action:@selector(cut:)
                 keyEquivalent:@"x"];
    
    [editMenu addItemWithTitle:@"Copy"
                        action:@selector(copy:)
                 keyEquivalent:@"c"];
    
    [editMenu addItemWithTitle:@"Paste"
                        action:@selector(paste:)
                 keyEquivalent:@"v"];
    
    [editMenu addItemWithTitle:@"Select All"
                        action:@selector(selectAll:)
                 keyEquivalent:@"a"];
    
    [editMenuItem setSubmenu:editMenu];
    [mainMenu addItem:editMenuItem];
    
    // Window menu
    NSMenuItem* windowMenuItem = [[NSMenuItem alloc] init];
    NSMenu* windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
    
    [windowMenu addItemWithTitle:@"Minimize"
                          action:@selector(performMiniaturize:)
                   keyEquivalent:@"m"];
    
    [windowMenu addItemWithTitle:@"Zoom"
                          action:@selector(performZoom:)
                   keyEquivalent:@""];
    
    [windowMenu addItem:[NSMenuItem separatorItem]];
    
    [windowMenu addItemWithTitle:@"Bring All to Front"
                          action:@selector(arrangeInFront:)
                   keyEquivalent:@""];
    
    [windowMenuItem setSubmenu:windowMenu];
    [mainMenu addItem:windowMenuItem];
    
    // Set the main menu
    [NSApp setMainMenu:mainMenu];
}

@end
