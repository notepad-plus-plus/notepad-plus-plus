// find_in_files.mm — Find in Files dialog and search engine implementation
#include "find_in_files.h"
#include "file_operations.h"
#include "search_results_panel.h"
#include "app_state.h"
#include "string_utils.h"
#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>
#include <fnmatch.h>
#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include <cstring>
#include <atomic>
#include <dispatch/dispatch.h>

// ---------------------------------------------------------------------------
// Search state
// ---------------------------------------------------------------------------
static std::atomic<bool> sFIFRunning{false};
static std::atomic<bool> sFIFCancel{false};

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
static constexpr int kMaxMatches = 10000;
static constexpr int kBatchSize = 20;
static constexpr size_t kMaxLineTextLength = 500;

// ---------------------------------------------------------------------------
// Callback type
// ---------------------------------------------------------------------------
using FIFCallback = std::function<void(const FIFSearchResult&, bool complete)>;

// ---------------------------------------------------------------------------
// isWordChar — helper for whole-word boundary checking
// ---------------------------------------------------------------------------
static bool isWordChar(char c)
{
	return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// ---------------------------------------------------------------------------
// enumerateFiles — collect file paths matching glob filters under a directory
// ---------------------------------------------------------------------------
static std::vector<std::string> enumerateFiles(NSString* directory,
                                                const std::string& filters,
                                                bool recursive,
                                                bool includeHidden)
{
	static constexpr int kMaxFiles = 100000;
	std::vector<std::string> result;

	NSFileManager* fm = [NSFileManager defaultManager];
	NSURL* dirURL = [NSURL fileURLWithPath:directory isDirectory:YES];

	// Build enumeration options
	NSDirectoryEnumerationOptions options = 0;
	if (!recursive)
	{
		options |= NSDirectoryEnumerationSkipsSubdirectoryDescendants;
	}
	if (!includeHidden)
	{
		options |= NSDirectoryEnumerationSkipsHiddenFiles;
	}

	NSArray<NSURLResourceKey>* keys = @[NSURLIsRegularFileKey, NSURLNameKey];

	NSDirectoryEnumerator<NSURL*>* enumerator =
		[fm enumeratorAtURL:dirURL
		 includingPropertiesForKeys:keys
		 options:options
		 errorHandler:nil];

	// Parse filter patterns by splitting on ";"
	bool acceptAll = false;
	std::vector<std::string> patterns;

	if (filters.empty() || filters == "*.*" || filters == "*")
	{
		acceptAll = true;
	}
	else
	{
		std::istringstream stream(filters);
		std::string token;
		while (std::getline(stream, token, ';'))
		{
			// Trim whitespace
			size_t start = token.find_first_not_of(" \t");
			size_t end = token.find_last_not_of(" \t");
			if (start != std::string::npos)
			{
				patterns.push_back(token.substr(start, end - start + 1));
			}
		}
		if (patterns.empty())
		{
			acceptAll = true;
		}
	}

	for (NSURL* fileURL in enumerator)
	{
		if (static_cast<int>(result.size()) >= kMaxFiles)
		{
			break;
		}

		// Skip directories — only include regular files
		NSNumber* isRegularFile = nil;
		[fileURL getResourceValue:&isRegularFile forKey:NSURLIsRegularFileKey error:nil];
		if (![isRegularFile boolValue])
		{
			continue;
		}

		if (!acceptAll)
		{
			NSString* fileName = [fileURL lastPathComponent];
			const char* name = [fileName UTF8String];
			bool matched = false;
			for (const auto& pattern : patterns)
			{
				if (fnmatch(pattern.c_str(), name, FNM_CASEFOLD) == 0)
				{
					matched = true;
					break;
				}
			}
			if (!matched)
			{
				continue;
			}
		}

		result.push_back([[fileURL path] UTF8String]);
	}

	return result;
}

// ---------------------------------------------------------------------------
// isBinaryFile — returns true if first 8KB of file contains null bytes
// ---------------------------------------------------------------------------
static bool isBinaryFile(NSString* path)
{
	NSError* error = nil;
	NSData* data = [NSData dataWithContentsOfFile:path
	                                      options:NSDataReadingMappedIfSafe
	                                        error:&error];
	if (!data)
	{
		return false;
	}

	static constexpr NSUInteger kCheckSize = 8192;
	NSUInteger length = [data length];
	if (length > kCheckSize)
	{
		length = kCheckSize;
	}

	const uint8_t* bytes = static_cast<const uint8_t*>([data bytes]);
	for (NSUInteger i = 0; i < length; ++i)
	{
		if (bytes[i] == 0x00)
		{
			return true;
		}
	}

	return false;
}

// ---------------------------------------------------------------------------
// searchLinePlainText — search a single line for plain text matches
// ---------------------------------------------------------------------------
static void searchLinePlainText(const std::string& line,
                                const std::string& searchTerm,
                                const std::string& searchTermLower,
                                bool matchCase,
                                bool wholeWord,
                                int lineNumber,
                                FIFFileResult& fileResult,
                                int& totalMatches)
{
	const char* linePtr = line.c_str();
	size_t lineLen = line.size();
	size_t termLen = searchTerm.size();
	size_t pos = 0;

	while (pos + termLen <= lineLen && totalMatches < kMaxMatches)
	{
		const char* found = nullptr;

		if (matchCase)
		{
			found = strstr(linePtr + pos, searchTerm.c_str());
		}
		else
		{
			found = strcasestr(linePtr + pos, searchTerm.c_str());
		}

		if (!found)
		{
			break;
		}

		size_t matchPos = static_cast<size_t>(found - linePtr);

		// Whole-word boundary check
		if (wholeWord)
		{
			bool leftBoundary = (matchPos == 0) || !isWordChar(linePtr[matchPos - 1]);
			bool rightBoundary = (matchPos + termLen >= lineLen) ||
			                     !isWordChar(linePtr[matchPos + termLen]);
			if (!leftBoundary || !rightBoundary)
			{
				pos = matchPos + 1;
				continue;
			}
		}

		// Trim line text
		std::string lineText = line;
		if (lineText.size() > kMaxLineTextLength)
		{
			lineText = lineText.substr(0, kMaxLineTextLength);
		}

		FIFMatch match;
		match.lineNumber = lineNumber;
		match.column = static_cast<int>(matchPos);
		match.matchLength = static_cast<int>(termLen);
		match.lineText = lineText;
		fileResult.matches.push_back(std::move(match));
		++totalMatches;

		pos = matchPos + termLen;
	}
}

// ---------------------------------------------------------------------------
// searchLineRegex — search a single line for regex matches
// ---------------------------------------------------------------------------
static void searchLineRegex(const std::string& line,
                            const std::regex& pattern,
                            bool wholeWord,
                            int lineNumber,
                            FIFFileResult& fileResult,
                            int& totalMatches)
{
	std::sregex_iterator it(line.begin(), line.end(), pattern);
	std::sregex_iterator end;

	for (; it != end && totalMatches < kMaxMatches; ++it)
	{
		const std::smatch& m = *it;
		size_t matchPos = static_cast<size_t>(m.position());
		size_t matchLen = static_cast<size_t>(m.length());

		// Whole-word boundary check
		if (wholeWord)
		{
			bool leftBoundary = (matchPos == 0) ||
			                    !isWordChar(line[matchPos - 1]);
			bool rightBoundary = (matchPos + matchLen >= line.size()) ||
			                     !isWordChar(line[matchPos + matchLen]);
			if (!leftBoundary || !rightBoundary)
			{
				continue;
			}
		}

		// Trim line text
		std::string lineText = line;
		if (lineText.size() > kMaxLineTextLength)
		{
			lineText = lineText.substr(0, kMaxLineTextLength);
		}

		FIFMatch match;
		match.lineNumber = lineNumber;
		match.column = static_cast<int>(matchPos);
		match.matchLength = static_cast<int>(matchLen);
		match.lineText = lineText;
		fileResult.matches.push_back(std::move(match));
		++totalMatches;
	}
}

// ---------------------------------------------------------------------------
// doFindInFiles — background search engine
// ---------------------------------------------------------------------------
static void doFindInFiles(const std::string& searchTerm,
                          const std::string& directory,
                          const std::string& filters,
                          bool matchCase,
                          bool wholeWord,
                          bool useRegex,
                          bool recursive,
                          bool includeHidden,
                          FIFCallback callback)
{
	// Prevent concurrent searches
	if (sFIFRunning.load())
	{
		return;
	}

	sFIFRunning.store(true);
	sFIFCancel.store(false);

	// Enumerate files on the calling thread (fast enough, avoids threading issues)
	NSString* dirNS = [NSString stringWithUTF8String:directory.c_str()];
	std::vector<std::string> files = enumerateFiles(dirNS, filters, recursive, includeHidden);

	// Capture everything needed by the block
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
		@autoreleasepool
		{
			FIFSearchResult result;
			result.searchTerm = searchTerm;
			result.directory = directory;
			result.filters = filters;
			int totalMatches = 0;

			// Prepare regex if needed
			std::regex regexPattern;
			bool regexValid = true;
			if (useRegex)
			{
				try
				{
					auto flags = std::regex_constants::ECMAScript;
					if (!matchCase)
					{
						flags |= std::regex_constants::icase;
					}
					regexPattern = std::regex(searchTerm, flags);
				}
				catch (const std::regex_error&)
				{
					regexValid = false;
				}
			}

			// Prepare lowercase search term for case-insensitive plain text search
			std::string searchTermLower;
			if (!useRegex && !matchCase)
			{
				searchTermLower.resize(searchTerm.size());
				for (size_t i = 0; i < searchTerm.size(); ++i)
				{
					searchTermLower[i] = static_cast<char>(
						std::tolower(static_cast<unsigned char>(searchTerm[i])));
				}
			}

			int filesProcessed = 0;

			for (const auto& filePath : files)
			{
				// Check cancel flag
				if (sFIFCancel.load())
				{
					break;
				}

				// Check match cap
				if (totalMatches >= kMaxMatches)
				{
					break;
				}

				@autoreleasepool
				{
					NSString* pathNS = [NSString stringWithUTF8String:filePath.c_str()];

					// Skip binary files
					if (isBinaryFile(pathNS))
					{
						++filesProcessed;
						++result.filesSearched;
						continue;
					}

					// Read file data
					NSData* data = [NSData dataWithContentsOfFile:pathNS];
					if (!data || [data length] == 0)
					{
						++filesProcessed;
						++result.filesSearched;
						continue;
					}

					// Detect encoding and decode to UTF-8
					int encoding = detectEncoding(data);
					std::string utf8Text = decodeFileData(data, encoding);

					if (utf8Text.empty())
					{
						++filesProcessed;
						++result.filesSearched;
						continue;
					}

					// Search line-by-line
					FIFFileResult fileResult;
					fileResult.filePath = filePath;

					std::istringstream textStream(utf8Text);
					std::string line;
					int lineNumber = 0;

					while (std::getline(textStream, line) && totalMatches < kMaxMatches)
					{
						++lineNumber;

						// Remove trailing \r if present (for \r\n line endings)
						if (!line.empty() && line.back() == '\r')
						{
							line.pop_back();
						}

						if (useRegex)
						{
							if (regexValid)
							{
								searchLineRegex(line, regexPattern, wholeWord,
								                lineNumber, fileResult, totalMatches);
							}
						}
						else
						{
							searchLinePlainText(line, searchTerm, searchTermLower,
							                    matchCase, wholeWord,
							                    lineNumber, fileResult, totalMatches);
						}
					}

					++result.filesSearched;

					if (!fileResult.matches.empty())
					{
						++result.filesWithMatches;
						result.totalMatches += static_cast<int>(fileResult.matches.size());
						result.fileResults.push_back(std::move(fileResult));
					}

					++filesProcessed;

					// Post batch update to main thread every kBatchSize files
					if (filesProcessed % kBatchSize == 0)
					{
						FIFSearchResult batchResult = result;
						FIFCallback cb = callback;
						dispatch_async(dispatch_get_main_queue(), ^{
							cb(batchResult, false);
						});
					}
				}
			}

			// Post final result on main thread
			FIFSearchResult finalResult = std::move(result);
			FIFCallback cb = callback;
			dispatch_async(dispatch_get_main_queue(), ^{
				sFIFRunning.store(false);
				cb(finalResult, true);
			});
		}
	});
}

// ---------------------------------------------------------------------------
// Dialog state (file-static, reused across calls)
// ---------------------------------------------------------------------------
static NSPanel* sFIFPanel = nil;
static NSTextField* sFIFSearchField = nil;
static NSTextField* sFIFDirField = nil;
static NSTextField* sFIFFilterField = nil;
static NSButton* sFIFMatchCaseCheck = nil;
static NSButton* sFIFWholeWordCheck = nil;
static NSButton* sFIFRegexCheck = nil;
static NSButton* sFIFRecursiveCheck = nil;
static NSButton* sFIFHiddenCheck = nil;
static NSButton* sFIFFindAllBtn = nil;
static NSButton* sFIFCancelBtn = nil;
static NSTextField* sFIFStatusLabel = nil;

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void showFindInFilesDlg()
{
	// Reuse existing panel if already created
	if (sFIFPanel)
	{
		// Update search field from ctx() in case it changed
		if (!ctx().findText.empty())
		{
			sFIFSearchField.stringValue = WideToNSString(ctx().findText.c_str());
		}

		[sFIFPanel makeKeyAndOrderFront:nil];
		[sFIFPanel makeFirstResponder:sFIFSearchField];
		return;
	}

	// -----------------------------------------------------------------------
	// Create the floating panel
	// -----------------------------------------------------------------------
	CGFloat panelW = 480;
	CGFloat panelH = 320;

	NSRect panelRect = NSMakeRect(0, 0, panelW, panelH);
	sFIFPanel = [[NSPanel alloc]
		initWithContentRect:panelRect
		          styleMask:(NSWindowStyleMaskTitled |
		                     NSWindowStyleMaskClosable |
		                     NSWindowStyleMaskUtilityWindow)
		            backing:NSBackingStoreBuffered
		              defer:NO];
	[sFIFPanel setTitle:@"Find in Files"];
	[sFIFPanel setReleasedWhenClosed:NO];
	[sFIFPanel setFloatingPanel:YES];
	[sFIFPanel setBecomesKeyOnlyIfNeeded:NO];

	// Center relative to main window
	if (ctx().mainWindow)
	{
		NSRect mainFrame = [ctx().mainWindow frame];
		CGFloat x = NSMidX(mainFrame) - panelW / 2;
		CGFloat y = NSMidY(mainFrame) - panelH / 2;
		[sFIFPanel setFrameOrigin:NSMakePoint(x, y)];
	}
	else
	{
		[sFIFPanel center];
	}

	NSView* content = [sFIFPanel contentView];

	// -----------------------------------------------------------------------
	// Layout constants
	// -----------------------------------------------------------------------
	CGFloat leftMargin = 16;
	CGFloat rightMargin = 16;
	CGFloat labelW = 72;
	CGFloat fieldH = 24;
	CGFloat rowSpacing = 8;
	CGFloat fieldX = leftMargin + labelW + 4;
	CGFloat fieldW = panelW - fieldX - rightMargin;
	CGFloat browseW = 70;
	CGFloat dirFieldW = fieldW - browseW - 8;

	// Start from top of content area (flipped-style: compute from top)
	CGFloat topY = panelH - 16;

	// -----------------------------------------------------------------------
	// Row 1: Find label + field
	// -----------------------------------------------------------------------
	topY -= fieldH;
	NSTextField* findLabel = [NSTextField labelWithString:@"Find:"];
	findLabel.frame = NSMakeRect(leftMargin, topY, labelW, fieldH);
	findLabel.alignment = NSTextAlignmentRight;
	[content addSubview:findLabel];

	sFIFSearchField = [[NSTextField alloc] initWithFrame:NSMakeRect(fieldX, topY, fieldW, fieldH)];
	sFIFSearchField.placeholderString = @"Search term";
	if (!ctx().findText.empty())
	{
		sFIFSearchField.stringValue = WideToNSString(ctx().findText.c_str());
	}
	[content addSubview:sFIFSearchField];

	// -----------------------------------------------------------------------
	// Row 2: Directory label + field + Browse button
	// -----------------------------------------------------------------------
	topY -= (fieldH + rowSpacing);
	NSTextField* dirLabel = [NSTextField labelWithString:@"Directory:"];
	dirLabel.frame = NSMakeRect(leftMargin, topY, labelW, fieldH);
	dirLabel.alignment = NSTextAlignmentRight;
	[content addSubview:dirLabel];

	sFIFDirField = [[NSTextField alloc] initWithFrame:NSMakeRect(fieldX, topY, dirFieldW, fieldH)];
	sFIFDirField.placeholderString = @"/path/to/search";

	// Pre-fill directory from current file or home
	NSString* dirDefault = NSHomeDirectory();
	auto& docs = ctx().activeDocuments();
	int tabIdx = ctx().activeTabIndex();
	if (tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()) && !docs[tabIdx].filePath.empty())
	{
		NSString* path = WideToNSString(docs[tabIdx].filePath.c_str());
		dirDefault = [path stringByDeletingLastPathComponent];
	}
	sFIFDirField.stringValue = dirDefault;
	[content addSubview:sFIFDirField];

	NSButton* browseBtn = [NSButton buttonWithTitle:@"Browse..."
	                                         target:nil
	                                         action:nil];
	browseBtn.frame = NSMakeRect(fieldX + dirFieldW + 8, topY, browseW, fieldH);
	[content addSubview:browseBtn];

	// -----------------------------------------------------------------------
	// Row 3: Filters label + field
	// -----------------------------------------------------------------------
	topY -= (fieldH + rowSpacing);
	NSTextField* filterLabel = [NSTextField labelWithString:@"Filters:"];
	filterLabel.frame = NSMakeRect(leftMargin, topY, labelW, fieldH);
	filterLabel.alignment = NSTextAlignmentRight;
	[content addSubview:filterLabel];

	sFIFFilterField = [[NSTextField alloc] initWithFrame:NSMakeRect(fieldX, topY, fieldW, fieldH)];
	sFIFFilterField.stringValue = @"*.*";
	sFIFFilterField.placeholderString = @"*.cpp;*.h;*.mm";
	[content addSubview:sFIFFilterField];

	// -----------------------------------------------------------------------
	// Row 4: Checkboxes — Match case, Whole word, Regex
	// -----------------------------------------------------------------------
	topY -= (fieldH + rowSpacing + 4);
	CGFloat checkW = 110;
	CGFloat checkX = fieldX;

	sFIFMatchCaseCheck = [NSButton checkboxWithTitle:@"Match case" target:nil action:nil];
	sFIFMatchCaseCheck.frame = NSMakeRect(checkX, topY, checkW, fieldH);
	sFIFMatchCaseCheck.state = ctx().matchCase ? NSControlStateValueOn : NSControlStateValueOff;
	[content addSubview:sFIFMatchCaseCheck];

	checkX += checkW + 8;
	sFIFWholeWordCheck = [NSButton checkboxWithTitle:@"Whole word" target:nil action:nil];
	sFIFWholeWordCheck.frame = NSMakeRect(checkX, topY, checkW, fieldH);
	sFIFWholeWordCheck.state = ctx().wholeWord ? NSControlStateValueOn : NSControlStateValueOff;
	[content addSubview:sFIFWholeWordCheck];

	checkX += checkW + 8;
	sFIFRegexCheck = [NSButton checkboxWithTitle:@"Regex" target:nil action:nil];
	sFIFRegexCheck.frame = NSMakeRect(checkX, topY, 80, fieldH);
	sFIFRegexCheck.state = ctx().useRegex ? NSControlStateValueOn : NSControlStateValueOff;
	[content addSubview:sFIFRegexCheck];

	// -----------------------------------------------------------------------
	// Row 5: Checkboxes — Recursive, Hidden
	// -----------------------------------------------------------------------
	topY -= (fieldH + rowSpacing);
	checkX = fieldX;

	sFIFRecursiveCheck = [NSButton checkboxWithTitle:@"In sub-folders" target:nil action:nil];
	sFIFRecursiveCheck.frame = NSMakeRect(checkX, topY, 130, fieldH);
	sFIFRecursiveCheck.state = NSControlStateValueOn;
	[content addSubview:sFIFRecursiveCheck];

	checkX += 130 + 8;
	sFIFHiddenCheck = [NSButton checkboxWithTitle:@"In hidden folders" target:nil action:nil];
	sFIFHiddenCheck.frame = NSMakeRect(checkX, topY, 140, fieldH);
	sFIFHiddenCheck.state = NSControlStateValueOff;
	[content addSubview:sFIFHiddenCheck];

	// -----------------------------------------------------------------------
	// Row 6: Buttons — Find All, Cancel, Close
	// -----------------------------------------------------------------------
	topY -= (fieldH + rowSpacing + 8);
	CGFloat btnW = 80;
	CGFloat btnH = 32;
	CGFloat btnX = leftMargin;

	sFIFFindAllBtn = [NSButton buttonWithTitle:@"Find All" target:nil action:nil];
	sFIFFindAllBtn.frame = NSMakeRect(btnX, topY, btnW, btnH);
	sFIFFindAllBtn.bezelStyle = NSBezelStyleRounded;
	sFIFFindAllBtn.keyEquivalent = @"\r"; // Enter key
	[content addSubview:sFIFFindAllBtn];

	btnX += btnW + 12;
	sFIFCancelBtn = [NSButton buttonWithTitle:@"Cancel" target:nil action:nil];
	sFIFCancelBtn.frame = NSMakeRect(btnX, topY, btnW, btnH);
	sFIFCancelBtn.bezelStyle = NSBezelStyleRounded;
	sFIFCancelBtn.enabled = NO;
	[content addSubview:sFIFCancelBtn];

	NSButton* closeBtn = [NSButton buttonWithTitle:@"Close" target:nil action:nil];
	closeBtn.frame = NSMakeRect(panelW - rightMargin - btnW, topY, btnW, btnH);
	closeBtn.bezelStyle = NSBezelStyleRounded;
	closeBtn.keyEquivalent = @"\033"; // Escape key
	[content addSubview:closeBtn];

	// -----------------------------------------------------------------------
	// Row 7: Status label
	// -----------------------------------------------------------------------
	topY -= (btnH + rowSpacing);
	sFIFStatusLabel = [NSTextField labelWithString:@"Status: Ready"];
	sFIFStatusLabel.frame = NSMakeRect(leftMargin, topY, panelW - leftMargin - rightMargin, fieldH);
	sFIFStatusLabel.textColor = [NSColor secondaryLabelColor];
	[content addSubview:sFIFStatusLabel];

	// -----------------------------------------------------------------------
	// FIFActionHandler — lightweight ObjC class for button actions
	// Defined once via the runtime and reused across calls.
	// -----------------------------------------------------------------------
	static dispatch_once_t onceToken;
	static Class fifHandlerClass = nil;
	dispatch_once(&onceToken, ^{
		fifHandlerClass = objc_allocateClassPair([NSObject class], "FIFActionHandler", 0);

		// Browse action
		class_addMethod(fifHandlerClass, @selector(browseAction:), imp_implementationWithBlock(^(id self, id sender) {
			NSOpenPanel* panel = [NSOpenPanel openPanel];
			panel.canChooseDirectories = YES;
			panel.canChooseFiles = NO;
			panel.allowsMultipleSelection = NO;
			panel.prompt = @"Select";
			[panel beginSheetModalForWindow:sFIFPanel completionHandler:^(NSModalResponse result) {
				if (result == NSModalResponseOK)
				{
					sFIFDirField.stringValue = panel.URL.path;
				}
			}];
		}), "v@:@");

		// Find All action
		class_addMethod(fifHandlerClass, @selector(findAllAction:), imp_implementationWithBlock(^(id self, id sender) {
			NSString* searchText = sFIFSearchField.stringValue;
			if (searchText.length == 0)
			{
				sFIFStatusLabel.stringValue = @"Status: Enter a search term";
				return;
			}

			NSString* directory = sFIFDirField.stringValue;
			if (directory.length == 0)
			{
				sFIFStatusLabel.stringValue = @"Status: Enter a directory";
				return;
			}

			NSString* filters = sFIFFilterField.stringValue;
			if (filters.length == 0)
			{
				filters = @"*.*";
			}

			bool matchCase = (sFIFMatchCaseCheck.state == NSControlStateValueOn);
			bool wholeWord = (sFIFWholeWordCheck.state == NSControlStateValueOn);
			bool useRegex = (sFIFRegexCheck.state == NSControlStateValueOn);
			bool recursive = (sFIFRecursiveCheck.state == NSControlStateValueOn);
			bool hidden = (sFIFHiddenCheck.state == NSControlStateValueOn);

			// Sync search options back to ctx()
			ctx().findText = NSStringToWide(searchText);
			ctx().matchCase = matchCase;
			ctx().wholeWord = wholeWord;
			ctx().useRegex = useRegex;

			// Update UI state
			sFIFStatusLabel.stringValue = @"Status: Searching...";
			sFIFFindAllBtn.enabled = NO;
			sFIFCancelBtn.enabled = YES;

			std::string searchTermStr = [searchText UTF8String];
			std::string dirStr = [directory UTF8String];
			std::string filterStr = [filters UTF8String];

			doFindInFiles(searchTermStr, dirStr, filterStr,
			              matchCase, wholeWord, useRegex, recursive, hidden,
			              [](const FIFSearchResult& result, bool complete) {
				if (!complete)
				{
					// Progress update
					NSString* status = [NSString stringWithFormat:
						@"Status: Searching... %d files, %d matches",
						result.filesSearched, result.totalMatches];
					sFIFStatusLabel.stringValue = status;
				}
				else
				{
					// Search complete
					sFIFFindAllBtn.enabled = YES;
					sFIFCancelBtn.enabled = NO;

					NSString* status = [NSString stringWithFormat:
						@"Status: Done: %d matches in %d files of %d searched",
						result.totalMatches, result.filesWithMatches, result.filesSearched];
					sFIFStatusLabel.stringValue = status;

					// Show results panel (stub for now, Task 9 implements it)
					showSearchResultsPanel(result);
				}
			});
		}), "v@:@");

		// Cancel action
		class_addMethod(fifHandlerClass, @selector(cancelAction:), imp_implementationWithBlock(^(id self, id sender) {
			cancelFIFSearch();
			sFIFStatusLabel.stringValue = @"Status: Cancelling...";
		}), "v@:@");

		// Close action
		class_addMethod(fifHandlerClass, @selector(closeAction:), imp_implementationWithBlock(^(id self, id sender) {
			[sFIFPanel orderOut:nil];
		}), "v@:@");

		objc_registerClassPair(fifHandlerClass);
	});

	static id fifHandler = [[fifHandlerClass alloc] init];

	browseBtn.target = fifHandler;
	browseBtn.action = @selector(browseAction:);

	sFIFFindAllBtn.target = fifHandler;
	sFIFFindAllBtn.action = @selector(findAllAction:);

	sFIFCancelBtn.target = fifHandler;
	sFIFCancelBtn.action = @selector(cancelAction:);

	closeBtn.target = fifHandler;
	closeBtn.action = @selector(closeAction:);

	// -----------------------------------------------------------------------
	// Show the panel
	// -----------------------------------------------------------------------
	[sFIFPanel makeKeyAndOrderFront:nil];
	[sFIFPanel makeFirstResponder:sFIFSearchField];
}

bool isFIFSearchRunning()
{
	return sFIFRunning.load();
}

void cancelFIFSearch()
{
	sFIFCancel.store(true);
}
