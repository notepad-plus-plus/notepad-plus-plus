// file_path_ops.h — Reveal in Finder and copy-path commands
// Part of the Notepad++ macOS port modular refactor.

#pragma once

#ifdef __OBJC__
@class NSString;
// Returns the active document's file path as an NSString, or nil if untitled.
NSString* activeDocumentPath();
#endif

// Returns true if the active document has a saved file path.
bool hasActiveFilePath();

void doRevealInFinder();
void doCopyFullPath();
void doCopyFilename();
void doCopyDirPath();

// Enable/disable file-path menu items based on active document state.
void updateFilePathMenuState();
