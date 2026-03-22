// incremental_search.mm — Incremental search bar implementation
#import <Cocoa/Cocoa.h>
#include "incremental_search.h"
#include "npp_constants.h"
#include "app_state.h"
#include "string_utils.h"
#include "scintilla_bridge.h"

// Forward-declare free functions called by IncrSearchTextField
void hideIncrementalSearch();
void doIncrSearchNext();
void doIncrSearchPrev();

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
}

- (void)wordToggled:(id)sender
{
	ctx().wholeWord = (_wordButton.state == NSControlStateValueOn);
}

- (void)regexToggled:(id)sender
{
	ctx().useRegex = (_regexButton.state == NSControlStateValueOn);
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
	// Will be filled in Task 3
}

void updateIncrementalSearchTarget()
{
	// Will be filled in Task 5
}

void doIncrSearchNext()
{
	// Will be filled in Task 3
}

void doIncrSearchPrev()
{
	// Will be filled in Task 3
}
