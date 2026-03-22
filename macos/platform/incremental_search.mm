// incremental_search.mm — Incremental search bar implementation
#import <Cocoa/Cocoa.h>
#include "incremental_search.h"
#include "smart_highlight.h"
#include "npp_constants.h"
#include "app_state.h"
#include "string_utils.h"
#include "scintilla_bridge.h"
#include <cstring>

// Forward-declare free functions called by IncrSearchTextField and IncrementalSearchBar
void hideIncrementalSearch();
void doIncrSearchNext();
void doIncrSearchPrev();
static void performIncrementalSearch();

// ============================================================
// IncrSearchTextField — custom NSTextField that intercepts keys
// ============================================================
@interface IncrSearchTextField : NSTextField
@end

@implementation IncrSearchTextField
- (void)cancelOperation:(id)sender
{
	hideIncrementalSearch();
}

- (BOOL)performKeyEquivalent:(NSEvent*)event
{
	if (event.keyCode == 36) // Return
	{
		if (event.modifierFlags & NSEventModifierFlagShift)
			doIncrSearchPrev();
		else
			doIncrSearchNext();
		return YES;
	}
	return [super performKeyEquivalent:event];
}
@end

// ============================================================
// IncrementalSearchBar — NSView containing search controls
// ============================================================
@interface IncrementalSearchBar : NSView <NSTextFieldDelegate>
@property (strong) IncrSearchTextField* searchField;
@property (strong) NSTextField* matchLabel;
@property (strong) NSButton* prevButton;
@property (strong) NSButton* nextButton;
@property (strong) NSButton* caseButton;
@property (strong) NSButton* wordButton;
@property (strong) NSButton* regexButton;
@property (strong) NSButton* closeButton;
@end

@implementation IncrementalSearchBar

- (instancetype)initWithFrame:(NSRect)frameRect
{
	self = [super initWithFrame:frameRect];
	if (!self) return nil;

	self.wantsLayer = YES;
	self.layer.backgroundColor = NSColor.windowBackgroundColor.CGColor;

	CGFloat x = 8.0;
	CGFloat fieldHeight = 22.0;
	CGFloat yOffset = (frameRect.size.height - fieldHeight) / 2.0;

	// Close button
	_closeButton = [NSButton buttonWithImage:[NSImage imageNamed:NSImageNameStopProgressFreestandingTemplate]
	                                  target:self
	                                  action:@selector(closeClicked:)];
	_closeButton.bezelStyle = NSBezelStyleInline;
	_closeButton.bordered = NO;
	_closeButton.frame = NSMakeRect(x, yOffset, 20, fieldHeight);
	[_closeButton setToolTip:@"Close search bar (Esc)"];
	[self addSubview:_closeButton];
	x += 24;

	// Search field
	_searchField = [[IncrSearchTextField alloc] initWithFrame:NSMakeRect(x, yOffset, 250, fieldHeight)];
	_searchField.placeholderString = @"Incremental Search";
	_searchField.font = [NSFont systemFontOfSize:12];
	_searchField.bezelStyle = NSTextFieldRoundedBezel;
	_searchField.delegate = self;
	[self addSubview:_searchField];
	x += 256;

	// Previous button
	_prevButton = [NSButton buttonWithTitle:@"\u25C0"
	                                 target:self
	                                 action:@selector(prevClicked:)];
	_prevButton.bezelStyle = NSBezelStyleInline;
	_prevButton.frame = NSMakeRect(x, yOffset, 28, fieldHeight);
	[_prevButton setToolTip:@"Previous match (Shift+Enter)"];
	[self addSubview:_prevButton];
	x += 30;

	// Next button
	_nextButton = [NSButton buttonWithTitle:@"\u25B6"
	                                 target:self
	                                 action:@selector(nextClicked:)];
	_nextButton.bezelStyle = NSBezelStyleInline;
	_nextButton.frame = NSMakeRect(x, yOffset, 28, fieldHeight);
	[_nextButton setToolTip:@"Next match (Enter)"];
	[self addSubview:_nextButton];
	x += 34;

	// Case-sensitive toggle
	_caseButton = [NSButton buttonWithTitle:@"Aa"
	                                 target:self
	                                 action:@selector(caseToggled:)];
	_caseButton.bezelStyle = NSBezelStyleRecessed;
	[_caseButton setButtonType:NSButtonTypeToggle];
	_caseButton.frame = NSMakeRect(x, yOffset, 32, fieldHeight);
	[_caseButton setToolTip:@"Match Case"];
	_caseButton.state = ctx().matchCase ? NSControlStateValueOn : NSControlStateValueOff;
	[self addSubview:_caseButton];
	x += 36;

	// Whole word toggle
	_wordButton = [NSButton buttonWithTitle:@"W"
	                                 target:self
	                                 action:@selector(wordToggled:)];
	_wordButton.bezelStyle = NSBezelStyleRecessed;
	[_wordButton setButtonType:NSButtonTypeToggle];
	_wordButton.frame = NSMakeRect(x, yOffset, 28, fieldHeight);
	[_wordButton setToolTip:@"Whole Word"];
	_wordButton.state = ctx().wholeWord ? NSControlStateValueOn : NSControlStateValueOff;
	[self addSubview:_wordButton];
	x += 32;

	// Regex toggle
	_regexButton = [NSButton buttonWithTitle:@".*"
	                                  target:self
	                                  action:@selector(regexToggled:)];
	_regexButton.bezelStyle = NSBezelStyleRecessed;
	[_regexButton setButtonType:NSButtonTypeToggle];
	_regexButton.frame = NSMakeRect(x, yOffset, 32, fieldHeight);
	[_regexButton setToolTip:@"Regular Expression"];
	_regexButton.state = ctx().useRegex ? NSControlStateValueOn : NSControlStateValueOff;
	[self addSubview:_regexButton];
	x += 38;

	// Match count label
	_matchLabel = [NSTextField labelWithString:@""];
	_matchLabel.font = [NSFont systemFontOfSize:11];
	_matchLabel.textColor = NSColor.secondaryLabelColor;
	_matchLabel.frame = NSMakeRect(x, yOffset, 120, fieldHeight);
	[self addSubview:_matchLabel];

	return self;
}

- (void)closeClicked:(id)sender
{
	hideIncrementalSearch();
}

- (void)prevClicked:(id)sender
{
	doIncrSearchPrev();
}

- (void)nextClicked:(id)sender
{
	doIncrSearchNext();
}

- (void)caseToggled:(id)sender
{
	ctx().matchCase = (_caseButton.state == NSControlStateValueOn);
	performIncrementalSearch();
}

- (void)wordToggled:(id)sender
{
	ctx().wholeWord = (_wordButton.state == NSControlStateValueOn);
	performIncrementalSearch();
}

- (void)regexToggled:(id)sender
{
	ctx().useRegex = (_regexButton.state == NSControlStateValueOn);
	performIncrementalSearch();
}

// Debounced text change handler — re-runs search on every keystroke
static unsigned int sIncrSearchGeneration = 0;

- (void)controlTextDidChange:(NSNotification*)notification
{
	unsigned int gen = ++sIncrSearchGeneration;
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC),
		dispatch_get_main_queue(), ^{
			if (gen == sIncrSearchGeneration)
				performIncrementalSearch();
		});
}

@end

// ============================================================
// Helper: get the search bar as its typed class
// ============================================================
static IncrementalSearchBar* getSearchBar()
{
	return (IncrementalSearchBar*)ctx().incrementalSearchBar;
}

// ============================================================
// Helper: get the active editor container
// ============================================================
static NSView* activeEditorContainer()
{
	return ctx().activeView == 0 ? ctx().editorContainer : ctx().editorContainer2;
}

// ============================================================
// Helper: get the ScintillaView as NSView
// ============================================================
static NSView* activeScintillaNSView()
{
	void* sciPtr = ctx().activeScintillaView();
	if (!sciPtr) return nil;
	return (__bridge NSView*)sciPtr;
}

// ============================================================
// Helper: get selected text from Scintilla (up to maxLen chars)
// ============================================================
static NSString* getSelectionText(int maxLen)
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return nil;

	intptr_t selStart = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONSTART, 0, 0);
	intptr_t selEnd = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONEND, 0, 0);
	intptr_t selLen = selEnd - selStart;

	if (selLen <= 0 || selLen > maxLen)
		return nil;

	// SCI_GETSELTEXT writes selLen+1 bytes (null-terminated)
	std::vector<char> buf(selLen + 1, 0);
	ScintillaBridge_sendMessage(sci, SCI_GETSELTEXT, 0, (intptr_t)buf.data());

	return [NSString stringWithUTF8String:buf.data()];
}

// ============================================================
// Core search logic
// ============================================================

// Build search flags from current toggle button state
static int buildIncrSearchFlags()
{
	int flags = 0;
	if (ctx().matchCase) flags |= SCFIND_MATCHCASE;
	if (ctx().wholeWord) flags |= SCFIND_WHOLEWORD;
	if (ctx().useRegex)  flags |= SCFIND_REGEXP | SCFIND_CXX11REGEX;
	return flags;
}

// Count which match index the given position corresponds to (1-based).
// Searches from document start to the given position, counting matches.
static int countMatchIndex(void* sci, const char* utf8Text, size_t textLen, int flags, intptr_t matchPos)
{
	intptr_t docLen = ScintillaBridge_sendMessage(sci, SCI_GETLENGTH, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETSEARCHFLAGS, flags, 0);

	int index = 0;
	intptr_t searchStart = 0;

	while (searchStart < docLen)
	{
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, searchStart, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, docLen, 0);

		intptr_t pos = ScintillaBridge_sendMessage(sci, SCI_SEARCHINTARGET,
		                                           textLen, (intptr_t)utf8Text);
		if (pos < 0) break;

		++index;
		if (pos == matchPos)
			return index;

		intptr_t targetEnd = ScintillaBridge_sendMessage(sci, SCI_GETTARGETEND, 0, 0);
		if (targetEnd <= searchStart)
		{
			++searchStart;
			continue;
		}
		searchStart = targetEnd;
	}

	return 0; // not found
}

static void performIncrementalSearch()
{
	IncrementalSearchBar* bar = getSearchBar();
	if (!bar) return;

	NSString* searchText = bar.searchField.stringValue;
	void* sci = ctx().activeScintillaView();
	if (!sci) return;

	// If search text is empty, clear highlights and reset label
	if (!searchText || searchText.length == 0)
	{
		intptr_t docLen = ScintillaBridge_sendMessage(sci, SCI_GETLENGTH, 0, 0);
		if (docLen > 0)
		{
			ScintillaBridge_sendMessage(sci, SCI_SETINDICATORCURRENT, INDIC_INCREMENTAL_SEARCH, 0);
			ScintillaBridge_sendMessage(sci, SCI_INDICATORCLEARRANGE, 0, docLen);
		}
		bar.matchLabel.stringValue = @"";
		ctx().incrSearchCurrentMatch = -1;
		ctx().incrSearchTotalMatches = 0;
		return;
	}

	const char* utf8Find = [searchText UTF8String];
	size_t textLen = strlen(utf8Find);

	// Sync search text to ctx().findText
	ctx().findText = NSStringToWide(searchText);

	// Build search flags
	int flags = buildIncrSearchFlags();

	// Clear previous highlights
	intptr_t docLen = ScintillaBridge_sendMessage(sci, SCI_GETLENGTH, 0, 0);
	if (docLen > 0)
	{
		ScintillaBridge_sendMessage(sci, SCI_SETINDICATORCURRENT, INDIC_INCREMENTAL_SEARCH, 0);
		ScintillaBridge_sendMessage(sci, SCI_INDICATORCLEARRANGE, 0, docLen);
	}

	// Highlight all occurrences
	int count = highlightAllOccurrences(sci, utf8Find, flags, INDIC_INCREMENTAL_SEARCH, 10000);

	if (count == 0)
	{
		bar.matchLabel.stringValue = @"No matches";
		ctx().incrSearchCurrentMatch = -1;
		ctx().incrSearchTotalMatches = 0;
		return;
	}

	ctx().incrSearchTotalMatches = count;

	// Find the match nearest to (at or after) current cursor position
	intptr_t cursorPos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);

	// Search forward from cursor to end
	ScintillaBridge_sendMessage(sci, SCI_SETSEARCHFLAGS, flags, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, cursorPos, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, docLen, 0);

	intptr_t foundPos = ScintillaBridge_sendMessage(sci, SCI_SEARCHINTARGET,
	                                                textLen, (intptr_t)utf8Find);

	if (foundPos < 0)
	{
		// Wrap: search from beginning
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, 0, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, docLen, 0);
		foundPos = ScintillaBridge_sendMessage(sci, SCI_SEARCHINTARGET,
		                                       textLen, (intptr_t)utf8Find);
	}

	if (foundPos >= 0)
	{
		intptr_t targetEnd = ScintillaBridge_sendMessage(sci, SCI_GETTARGETEND, 0, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETSEL, foundPos, targetEnd);
		ScintillaBridge_sendMessage(sci, SCI_SCROLLCARET, 0, 0);

		// Count which match this is
		int matchIndex = countMatchIndex(sci, utf8Find, textLen, flags, foundPos);
		ctx().incrSearchCurrentMatch = matchIndex;

		bar.matchLabel.stringValue = [NSString stringWithFormat:@"%d of %d", matchIndex, count];
	}
	else
	{
		bar.matchLabel.stringValue = @"No matches";
		ctx().incrSearchCurrentMatch = -1;
	}
}

// ============================================================
// Public API
// ============================================================

static const CGFloat kSearchBarHeight = 30.0;

void showIncrementalSearch()
{
	NSView* container = activeEditorContainer();
	if (!container) return;

	// If bar already visible, just focus the field and select all
	if (ctx().incrSearchVisible)
	{
		IncrementalSearchBar* bar = getSearchBar();
		if (bar)
		{
			[bar.window makeFirstResponder:bar.searchField];
			[bar.searchField selectText:nil];
		}
		return;
	}

	// Create the bar if it doesn't exist yet
	if (ctx().incrementalSearchBar == nil)
	{
		NSRect barFrame = NSMakeRect(0, container.bounds.size.height - kSearchBarHeight,
		                             container.bounds.size.width, kSearchBarHeight);
		IncrementalSearchBar* bar = [[IncrementalSearchBar alloc] initWithFrame:barFrame];
		bar.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
		ctx().incrementalSearchBar = bar;
	}

	IncrementalSearchBar* bar = getSearchBar();

	// Add bar as subview at the top of the editor container
	[container addSubview:bar];
	bar.frame = NSMakeRect(0, container.bounds.size.height - kSearchBarHeight,
	                       container.bounds.size.width, kSearchBarHeight);

	// Adjust the ScintillaView frame to leave space for the bar
	NSView* sciView = activeScintillaNSView();
	if (sciView && sciView.superview == container)
	{
		NSRect sciFrame = container.bounds;
		sciFrame.size.height -= kSearchBarHeight;
		sciView.frame = sciFrame;
	}

	// Pre-fill search field from current selection (if < 256 chars) or ctx().findText
	NSString* selText = getSelectionText(256);
	if (selText && selText.length > 0)
	{
		bar.searchField.stringValue = selText;
	}
	else if (!ctx().findText.empty())
	{
		bar.searchField.stringValue = WideToNSString(ctx().findText.c_str());
	}

	// Make search field first responder
	[bar.window makeFirstResponder:bar.searchField];
	[bar.searchField selectText:nil];

	ctx().incrSearchVisible = true;
}

void hideIncrementalSearch()
{
	clearIncrementalSearchHighlights();

	IncrementalSearchBar* bar = getSearchBar();
	if (bar)
	{
		NSView* container = bar.superview;
		[bar removeFromSuperview];

		// Restore ScintillaView to fill the container
		NSView* sciView = activeScintillaNSView();
		if (sciView && container && sciView.superview == container)
		{
			sciView.frame = container.bounds;
		}
	}

	// Return focus to Scintilla
	NSView* sciView = activeScintillaNSView();
	if (sciView)
	{
		[sciView.window makeFirstResponder:sciView];
	}

	ctx().incrSearchVisible = false;
}

bool isIncrementalSearchVisible()
{
	return ctx().incrSearchVisible;
}

void clearIncrementalSearchHighlights()
{
	void* views[] = { ctx().scintillaView, ctx().scintillaView2 };
	for (void* sci : views)
	{
		if (!sci) continue;
		intptr_t docLen = ScintillaBridge_sendMessage(sci, SCI_GETLENGTH, 0, 0);
		if (docLen <= 0) continue;
		ScintillaBridge_sendMessage(sci, SCI_SETINDICATORCURRENT, INDIC_INCREMENTAL_SEARCH, 0);
		ScintillaBridge_sendMessage(sci, SCI_INDICATORCLEARRANGE, 0, docLen);
	}
	ctx().incrSearchCurrentMatch = -1;
	ctx().incrSearchTotalMatches = 0;
}

void updateIncrementalSearchTarget()
{
	if (!ctx().incrSearchVisible) return;

	IncrementalSearchBar* bar = getSearchBar();
	if (!bar) return;

	// Remove bar from current superview
	[bar removeFromSuperview];

	// Re-add to the active editor container
	NSView* container = activeEditorContainer();
	if (!container) return;

	bar.frame = NSMakeRect(0, container.bounds.size.height - kSearchBarHeight,
	                       container.bounds.size.width, kSearchBarHeight);
	[container addSubview:bar];

	// Adjust ScintillaView to leave space for bar
	NSView* sciView = activeScintillaNSView();
	if (sciView && sciView.superview == container)
	{
		NSRect sciFrame = container.bounds;
		sciFrame.size.height -= kSearchBarHeight;
		sciView.frame = sciFrame;
	}

	// Re-run search on the new document
	performIncrementalSearch();
}

void doIncrSearchNext()
{
	if (ctx().findText.empty()) return;

	void* sci = ctx().activeScintillaView();
	if (!sci) return;

	NSString* nsFind = WideToNSString(ctx().findText.c_str());
	const char* utf8Find = [nsFind UTF8String];
	size_t textLen = strlen(utf8Find);
	int flags = buildIncrSearchFlags();
	intptr_t docLen = ScintillaBridge_sendMessage(sci, SCI_GETLENGTH, 0, 0);

	// Search forward from selection end
	intptr_t selEnd = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONEND, 0, 0);

	ScintillaBridge_sendMessage(sci, SCI_SETSEARCHFLAGS, flags, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, selEnd, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, docLen, 0);

	intptr_t foundPos = ScintillaBridge_sendMessage(sci, SCI_SEARCHINTARGET,
	                                                textLen, (intptr_t)utf8Find);

	if (foundPos < 0)
	{
		// Wrap: search from beginning to selection end
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, 0, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, selEnd, 0);
		foundPos = ScintillaBridge_sendMessage(sci, SCI_SEARCHINTARGET,
		                                       textLen, (intptr_t)utf8Find);
	}

	if (foundPos >= 0)
	{
		intptr_t targetEnd = ScintillaBridge_sendMessage(sci, SCI_GETTARGETEND, 0, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETSEL, foundPos, targetEnd);
		ScintillaBridge_sendMessage(sci, SCI_SCROLLCARET, 0, 0);

		// Update match index label
		int matchIndex = countMatchIndex(sci, utf8Find, textLen, flags, foundPos);
		ctx().incrSearchCurrentMatch = matchIndex;

		IncrementalSearchBar* bar = getSearchBar();
		if (bar)
		{
			bar.matchLabel.stringValue = [NSString stringWithFormat:@"%d of %d",
			                              matchIndex, ctx().incrSearchTotalMatches];
		}
	}
}

void doIncrSearchPrev()
{
	if (ctx().findText.empty()) return;

	void* sci = ctx().activeScintillaView();
	if (!sci) return;

	NSString* nsFind = WideToNSString(ctx().findText.c_str());
	const char* utf8Find = [nsFind UTF8String];
	size_t textLen = strlen(utf8Find);
	int flags = buildIncrSearchFlags();
	intptr_t docLen = ScintillaBridge_sendMessage(sci, SCI_GETLENGTH, 0, 0);

	// Search backward from selection start to document start
	intptr_t selStart = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONSTART, 0, 0);

	ScintillaBridge_sendMessage(sci, SCI_SETSEARCHFLAGS, flags, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, selStart, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, 0, 0);

	intptr_t foundPos = ScintillaBridge_sendMessage(sci, SCI_SEARCHINTARGET,
	                                                textLen, (intptr_t)utf8Find);

	if (foundPos < 0)
	{
		// Wrap: search backward from document end to selection start
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETSTART, docLen, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETTARGETEND, selStart, 0);
		foundPos = ScintillaBridge_sendMessage(sci, SCI_SEARCHINTARGET,
		                                       textLen, (intptr_t)utf8Find);
	}

	if (foundPos >= 0)
	{
		intptr_t targetEnd = ScintillaBridge_sendMessage(sci, SCI_GETTARGETEND, 0, 0);
		ScintillaBridge_sendMessage(sci, SCI_SETSEL, foundPos, targetEnd);
		ScintillaBridge_sendMessage(sci, SCI_SCROLLCARET, 0, 0);

		// Update match index label
		int matchIndex = countMatchIndex(sci, utf8Find, textLen, flags, foundPos);
		ctx().incrSearchCurrentMatch = matchIndex;

		IncrementalSearchBar* bar = getSearchBar();
		if (bar)
		{
			bar.matchLabel.stringValue = [NSString stringWithFormat:@"%d of %d",
			                              matchIndex, ctx().incrSearchTotalMatches];
		}
	}
}
