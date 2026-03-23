// app_state.h — Shared application state (globals + AppContext)
// Part of the Notepad++ macOS port modular refactor.
//
// Design note: All mutable application state is grouped in AppContext.
// A single global instance is accessed via ctx(). This centralizes
// coupling and makes it straightforward to refactor toward dependency
// injection for testability in the future.

#pragma once

#import <Cocoa/Cocoa.h>
#include <string>
#include <vector>

// Include windows.h AFTER Cocoa — the shim skips its BOOL typedef
// in ObjC++ because ObjC defines BOOL as bool.
#include "windows.h"

#include "document_data.h"

class FileMonitorMac;

struct AppContext
{
	// Main editor
	void* scintillaView = nullptr;
	NSWindow* mainWindow = nil;
	HWND mainHwnd = nullptr;
	HWND tabHwnd = nullptr;
	HWND statusBarHwnd = nullptr;
	std::vector<DocumentData> documents;
	int activeTab = -1;
	FileMonitorMac* fileMonitor = nullptr;

	// Find/Replace state
	HWND findDlgHwnd = nullptr;
	std::wstring findText;
	std::wstring replaceText;
	bool matchCase = false;
	bool wholeWord = false;
	bool useRegex = false;
	bool findMode = true; // true = Find, false = Replace

	// Recent files
	static const int MAX_RECENT_FILES = 10;
	std::vector<std::wstring> recentFiles;
	HMENU recentMenu = nullptr;

	// Preferences state
	int fontSize = 13;
	int tabWidth = 4;
	std::string fontName = "Menlo";
	bool showLineNumbers = true;
	int zoomLevel = 0;
	bool showCaretLine = true;
	bool autoIndent = true;
	bool useTabs = false;
	bool showWhitespace = false;
	bool showEol = false;
	bool showIndentGuides = false;
	bool autoCloseBrackets = true;

	// Split view state
	void* scintillaView2 = nullptr;
	NSSplitView* splitView = nil;
	bool isSplit = false;
	int activeView = 0; // 0=main, 1=sub
	std::vector<DocumentData> documents2;
	int activeTab2 = -1;
	HWND tabHwnd2 = nullptr;
	NSView* editorContainer = nil;
	NSView* editorContainer2 = nil;
	NSView* sciContainer2 = nil;
	bool syncScrolling = false;
	bool syncScrollReentrant = false;

	// Document map state
	bool documentMapEnabled = false;
	int documentMapWidth = 140;
	NSView* documentMapContainer = nil;
	NSView* documentMapOverlay = nil;
	void* documentMapScintilla = nullptr;

	// Notification suppression (prevents false dirty indicators during tab switches)
	bool suppressSavePointNotifications = false;

	// Auto-close internal edit guard (prevents recursive handling on programmatic edits)
	bool autoCloseInternalEdit = false;

	// Incremental search bar state
	NSView* incrementalSearchBar = nil;
	bool incrSearchVisible = false;
	int incrSearchCurrentMatch = -1;
	int incrSearchTotalMatches = 0;

	// Accessor helpers for split view
	void*& activeScintillaView() { return activeView == 0 ? scintillaView : scintillaView2; }
	std::vector<DocumentData>& activeDocuments() { return activeView == 0 ? documents : documents2; }
	int& activeTabIndex() { return activeView == 0 ? activeTab : activeTab2; }
	HWND& activeTabHwnd() { return activeView == 0 ? tabHwnd : tabHwnd2; }
};

// Global application context accessor
AppContext& ctx();
