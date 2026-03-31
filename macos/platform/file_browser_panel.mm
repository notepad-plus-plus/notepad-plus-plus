// file_browser_panel.mm — File browser panel with tree view, FSEvents, and context menu
// Shows a folder tree in the left zone (top, above File Switcher).
// Uses NSOutlineView with lazy-loaded children and icon caching.

#import <Cocoa/Cocoa.h>
#include <CoreServices/CoreServices.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "file_browser_panel.h"
#include "panel_layout.h"
#include "app_state.h"
#include "string_utils.h"
#include "file_switcher_panel.h"
#include "file_operations.h"
#include "npp_constants.h"

// ---------------------------------------------------------------------------
// Serial dispatch queue for thread safety
// ---------------------------------------------------------------------------
static dispatch_queue_t sFileBrowserQueue = nil;
static dispatch_queue_t fileBrowserQueue()
{
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		sFileBrowserQueue = dispatch_queue_create("com.macnotepp.filebrowser", DISPATCH_QUEUE_SERIAL);
	});
	return sFileBrowserQueue;
}

// ---------------------------------------------------------------------------
// Icon cache (by extension, not full path)
// ---------------------------------------------------------------------------
static NSCache<NSString*, NSImage*>* sIconCache = nil;
static NSCache* iconCache()
{
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		sIconCache = [[NSCache alloc] init];
		sIconCache.countLimit = 200;
	});
	return sIconCache;
}

static NSImage* cachedIconForPath(NSString* path, bool isDirectory)
{
	NSString* cacheKey = isDirectory ? @"__folder__" : [path pathExtension];
	if (cacheKey.length == 0) cacheKey = @"__noext__";
	NSImage* cached = [iconCache() objectForKey:cacheKey];
	if (cached) return cached;
	NSImage* icon = [[NSWorkspace sharedWorkspace] iconForFile:path];
	[icon setSize:NSMakeSize(16, 16)];
	[iconCache() setObject:icon forKey:cacheKey];
	return icon;
}

// ---------------------------------------------------------------------------
// Data Model
// ---------------------------------------------------------------------------
struct FileBrowserNode
{
	std::string path;
	std::string filename;
	bool isDirectory = false;
	bool isExpanded = false;
	bool childrenLoaded = false;
	std::vector<std::shared_ptr<FileBrowserNode>> children;
};

static FileBrowserNode sRootNode;

static bool shouldIgnore(const std::string& name)
{
	for (const auto& pattern : ctx().fileBrowserIgnorePatterns)
	{
		if (name == pattern)
			return true;
	}
	return false;
}

static void loadChildren(FileBrowserNode& node)
{
	if (node.childrenLoaded || !node.isDirectory)
		return;

	node.childrenLoaded = true;
	node.children.clear();

	NSString* dirPath = [NSString stringWithUTF8String:node.path.c_str()];
	NSFileManager* fm = [NSFileManager defaultManager];
	NSArray<NSString*>* contents = [fm contentsOfDirectoryAtPath:dirPath error:nil];
	if (!contents)
		return;

	std::vector<std::shared_ptr<FileBrowserNode>> dirs;
	std::vector<std::shared_ptr<FileBrowserNode>> files;

	for (NSString* name in contents)
	{
		std::string nameStr = [name UTF8String];
		if (shouldIgnore(nameStr))
			continue;
		// Skip hidden files (starting with '.')
		if ([name hasPrefix:@"."])
			continue;

		NSString* fullPath = [dirPath stringByAppendingPathComponent:name];
		BOOL isDir = NO;
		[fm fileExistsAtPath:fullPath isDirectory:&isDir];

		auto child = std::make_shared<FileBrowserNode>();
		child->path = [fullPath UTF8String];
		child->filename = nameStr;
		child->isDirectory = isDir ? true : false;

		if (child->isDirectory)
			dirs.push_back(std::move(child));
		else
			files.push_back(std::move(child));
	}

	// Sort directories and files alphabetically (case-insensitive)
	auto cmpFunc = [](const std::shared_ptr<FileBrowserNode>& a,
	                  const std::shared_ptr<FileBrowserNode>& b) {
		NSString* sa = [NSString stringWithUTF8String:a->filename.c_str()];
		NSString* sb = [NSString stringWithUTF8String:b->filename.c_str()];
		return [sa caseInsensitiveCompare:sb] == NSOrderedAscending;
	};

	std::sort(dirs.begin(), dirs.end(), cmpFunc);
	std::sort(files.begin(), files.end(), cmpFunc);

	// Directories first, then files
	node.children.reserve(dirs.size() + files.size());
	for (auto& d : dirs)
		node.children.push_back(std::move(d));
	for (auto& f : files)
		node.children.push_back(std::move(f));
}

static void invalidateChildrenRecursive(FileBrowserNode& node)
{
	node.childrenLoaded = false;
	for (auto& child : node.children)
		invalidateChildrenRecursive(*child);
}

// ---------------------------------------------------------------------------
// Obj-C wrapper for NSOutlineView (following FunctionListItem pattern)
// ---------------------------------------------------------------------------
@interface FileBrowserNodeWrapper : NSObject
{
	std::shared_ptr<FileBrowserNode> _sharedNode;
}
@property (nonatomic, readonly) FileBrowserNode* node;
+ (instancetype)wrapperForNode:(std::shared_ptr<FileBrowserNode>)sharedNode;
@end

@implementation FileBrowserNodeWrapper
+ (instancetype)wrapperForNode:(std::shared_ptr<FileBrowserNode>)sharedNode
{
	FileBrowserNodeWrapper* wrapper = [[FileBrowserNodeWrapper alloc] init];
	wrapper->_sharedNode = sharedNode;
	return wrapper;
}
- (FileBrowserNode*)node
{
	return _sharedNode.get();
}
@end

// ---------------------------------------------------------------------------
// Custom NSOutlineView subclass for Enter-key activation
// (Matches FunctionListOutlineView pattern)
// ---------------------------------------------------------------------------
@interface FileBrowserOutlineView : NSOutlineView
@end

@implementation FileBrowserOutlineView
- (void)keyDown:(NSEvent*)event
{
	if (event.keyCode == 36) // Return key
	{
		id target = self.target;
		SEL action = self.action;
		if (target && action)
		{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
			[target performSelector:action withObject:self];
#pragma clang diagnostic pop
			return;
		}
	}
	[super keyDown:event];
}
@end

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
@class FileBrowserController;
static FileBrowserController* sController = nil;
static NSView* sContainer = nil;
static NSScrollView* sScrollView = nil;
static NSOutlineView* sOutlineView = nil;
static NSView* sPlaceholderView = nil;
static NSTextField* sTitleLabel = nil;

// ---------------------------------------------------------------------------
// FSEvents monitoring
// ---------------------------------------------------------------------------
static FSEventStreamRef sEventStream = NULL;
static dispatch_source_t sDebounceTimer = nil;

static void fsEventsCallback(ConstFSEventStreamRef streamRef,
                             void* clientCallBackInfo,
                             size_t numEvents,
                             void* eventPaths,
                             const FSEventStreamEventFlags eventFlags[],
                             const FSEventStreamEventId eventIds[])
{
	dispatch_async(fileBrowserQueue(), ^{
		// Cancel any existing debounce timer
		if (sDebounceTimer)
		{
			dispatch_source_cancel(sDebounceTimer);
			sDebounceTimer = nil;
		}

		sDebounceTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, fileBrowserQueue());
		dispatch_source_set_timer(sDebounceTimer,
			dispatch_time(DISPATCH_TIME_NOW, 500 * NSEC_PER_MSEC),
			DISPATCH_TIME_FOREVER, 100 * NSEC_PER_MSEC);
		dispatch_source_set_event_handler(sDebounceTimer, ^{
			// Perform model invalidation and UI reload on the main queue to avoid data races
			dispatch_async(dispatch_get_main_queue(), ^{
				invalidateChildrenRecursive(sRootNode);
				if (sOutlineView)
					[sOutlineView reloadData];
			});

			if (sDebounceTimer)
			{
				dispatch_source_cancel(sDebounceTimer);
				sDebounceTimer = nil;
			}
		});
		dispatch_resume(sDebounceTimer);
	});
}

static void startFSEventsMonitoring(const std::string& path)
{
	if (sEventStream)
		return;

	NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
	if (!nsPath)
		return;

	CFArrayRef pathsToWatch = (__bridge CFArrayRef)@[nsPath];
	FSEventStreamContext context = {0, NULL, NULL, NULL, NULL};

	sEventStream = FSEventStreamCreate(
		kCFAllocatorDefault,
		&fsEventsCallback,
		&context,
		pathsToWatch,
		kFSEventStreamEventIdSinceNow,
		1.0, // 1 second latency
		kFSEventStreamCreateFlagUseCFTypes | kFSEventStreamCreateFlagFileEvents
	);

	if (sEventStream)
	{
		FSEventStreamScheduleWithRunLoop(sEventStream, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
		FSEventStreamStart(sEventStream);
	}
}

static void stopFSEventsMonitoring()
{
	if (sDebounceTimer)
	{
		dispatch_source_cancel(sDebounceTimer);
		sDebounceTimer = nil;
	}

	if (sEventStream)
	{
		FSEventStreamStop(sEventStream);
		FSEventStreamInvalidate(sEventStream);
		FSEventStreamRelease(sEventStream);
		sEventStream = NULL;
	}
}

// ---------------------------------------------------------------------------
// FileBrowserController — NSOutlineViewDataSource + NSOutlineViewDelegate
// ---------------------------------------------------------------------------
@interface FileBrowserController : NSObject <NSOutlineViewDataSource, NSOutlineViewDelegate, NSMenuDelegate>
- (void)outlineViewClicked:(id)sender;
- (void)newFile:(id)sender;
- (void)newFolder:(id)sender;
- (void)renameItem:(id)sender;
- (void)deleteItem:(id)sender;
- (void)revealInFinder:(id)sender;
- (void)copyPath:(id)sender;
@end

@implementation FileBrowserController

// ---------------------------------------------------------------------------
// NSOutlineViewDataSource
// ---------------------------------------------------------------------------
- (NSInteger)outlineView:(NSOutlineView*)outlineView numberOfChildrenOfItem:(id)item
{
	if (!item)
	{
		loadChildren(sRootNode);
		return static_cast<NSInteger>(sRootNode.children.size());
	}

	FileBrowserNodeWrapper* wrapper = (FileBrowserNodeWrapper*)item;
	FileBrowserNode* node = wrapper.node;
	if (!node)
		return 0;

	loadChildren(*node);
	return static_cast<NSInteger>(node->children.size());
}

- (id)outlineView:(NSOutlineView*)outlineView child:(NSInteger)index ofItem:(id)item
{
	FileBrowserNode* parent = nullptr;
	if (!item)
		parent = &sRootNode;
	else
		parent = ((FileBrowserNodeWrapper*)item).node;

	if (!parent || index < 0 || index >= static_cast<NSInteger>(parent->children.size()))
		return nil;

	return [FileBrowserNodeWrapper wrapperForNode:parent->children[static_cast<size_t>(index)]];
}

- (BOOL)outlineView:(NSOutlineView*)outlineView isItemExpandable:(id)item
{
	FileBrowserNodeWrapper* wrapper = (FileBrowserNodeWrapper*)item;
	if (!wrapper || !wrapper.node)
		return NO;
	return wrapper.node->isDirectory ? YES : NO;
}

// ---------------------------------------------------------------------------
// NSOutlineViewDelegate
// ---------------------------------------------------------------------------
- (NSView*)outlineView:(NSOutlineView*)outlineView
	viewForTableColumn:(NSTableColumn*)tableColumn
	              item:(id)item
{
	FileBrowserNodeWrapper* wrapper = (FileBrowserNodeWrapper*)item;
	if (!wrapper || !wrapper.node)
		return nil;

	FileBrowserNode* node = wrapper.node;

	NSTableCellView* cell = [outlineView makeViewWithIdentifier:@"FileBrowserCell" owner:self];
	if (!cell)
	{
		cell = [[NSTableCellView alloc] initWithFrame:NSMakeRect(0, 0, 200, 20)];

		NSImageView* imageView = [[NSImageView alloc] initWithFrame:NSMakeRect(0, 2, 16, 16)];
		imageView.translatesAutoresizingMaskIntoConstraints = NO;
		[cell addSubview:imageView];
		cell.imageView = imageView;

		NSTextField* text = [NSTextField labelWithString:@""];
		text.translatesAutoresizingMaskIntoConstraints = NO;
		text.lineBreakMode = NSLineBreakByTruncatingTail;
		[text setContentHuggingPriority:NSLayoutPriorityDefaultLow
		                 forOrientation:NSLayoutConstraintOrientationHorizontal];
		[text setContentCompressionResistancePriority:NSLayoutPriorityDefaultLow
		                              forOrientation:NSLayoutConstraintOrientationHorizontal];
		[cell addSubview:text];
		cell.textField = text;

		cell.identifier = @"FileBrowserCell";
		[NSLayoutConstraint activateConstraints:@[
			[imageView.leadingAnchor constraintEqualToAnchor:cell.leadingAnchor constant:2],
			[imageView.centerYAnchor constraintEqualToAnchor:cell.centerYAnchor],
			[imageView.widthAnchor constraintEqualToConstant:16],
			[imageView.heightAnchor constraintEqualToConstant:16],
			[text.leadingAnchor constraintEqualToAnchor:imageView.trailingAnchor constant:4],
			[text.trailingAnchor constraintEqualToAnchor:cell.trailingAnchor constant:-4],
			[text.centerYAnchor constraintEqualToAnchor:cell.centerYAnchor]
		]];
	}

	NSString* nsPath = [NSString stringWithUTF8String:node->path.c_str()];
	cell.imageView.image = cachedIconForPath(nsPath, node->isDirectory);

	cell.textField.stringValue = [NSString stringWithUTF8String:node->filename.c_str()] ?: @"";
	cell.textField.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];

	return cell;
}

// ---------------------------------------------------------------------------
// Click action — open file on single click
// ---------------------------------------------------------------------------
- (void)outlineViewClicked:(id)sender
{
	NSInteger row = sOutlineView ? sOutlineView.clickedRow : -1;
	if (row < 0 && sOutlineView)
		row = sOutlineView.selectedRow;
	if (row < 0)
		return;

	FileBrowserNodeWrapper* wrapper = (FileBrowserNodeWrapper*)[sOutlineView itemAtRow:row];
	if (!wrapper || !wrapper.node)
		return;

	FileBrowserNode* node = wrapper.node;

	// Directories: expand/collapse is handled natively by NSOutlineView
	if (node->isDirectory)
		return;

	// Open the file in the editor
	NSString* filePath = [NSString stringWithUTF8String:node->path.c_str()];
	if (filePath)
		openFileAtPath(filePath);
}

// ---------------------------------------------------------------------------
// Context menu actions
// ---------------------------------------------------------------------------
- (void)newFile:(id)sender
{
	NSInteger row = sOutlineView ? sOutlineView.clickedRow : -1;
	std::string parentDir;

	if (row >= 0)
	{
		FileBrowserNodeWrapper* wrapper = (FileBrowserNodeWrapper*)[sOutlineView itemAtRow:row];
		if (wrapper && wrapper.node)
		{
			if (wrapper.node->isDirectory)
				parentDir = wrapper.node->path;
			else
			{
				NSString* p = [NSString stringWithUTF8String:wrapper.node->path.c_str()];
				parentDir = [[p stringByDeletingLastPathComponent] UTF8String];
			}
		}
	}

	if (parentDir.empty())
		parentDir = sRootNode.path;

	NSAlert* alert = [[NSAlert alloc] init];
	alert.messageText = @"New File";
	alert.informativeText = @"Enter a name for the new file:";
	[alert addButtonWithTitle:@"Create"];
	[alert addButtonWithTitle:@"Cancel"];

	NSTextField* input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 260, 24)];
	input.stringValue = @"";
	input.placeholderString = @"filename.txt";
	alert.accessoryView = input;
	[alert.window setInitialFirstResponder:input];

	if ([alert runModal] != NSAlertFirstButtonReturn)
		return;

	NSString* name = input.stringValue;
	if (name.length == 0)
		return;

	// Validate name
	if ([name containsString:@"/"] || [name containsString:@":"])
	{
		NSAlert* err = [[NSAlert alloc] init];
		err.messageText = @"Invalid Name";
		err.informativeText = @"File name cannot contain '/' or ':' characters.";
		[err runModal];
		return;
	}

	NSString* dir = [NSString stringWithUTF8String:parentDir.c_str()];
	NSString* fullPath = [dir stringByAppendingPathComponent:name];

	if ([[NSFileManager defaultManager] fileExistsAtPath:fullPath])
	{
		NSAlert* err = [[NSAlert alloc] init];
		err.messageText = @"Already Exists";
		err.informativeText = [NSString stringWithFormat:@"'%@' already exists.", name];
		[err runModal];
		return;
	}

	BOOL created = [[NSFileManager defaultManager] createFileAtPath:fullPath contents:[NSData data] attributes:nil];
	if (!created)
	{
		NSAlert* err = [[NSAlert alloc] init];
		err.messageText = @"Error";
		err.informativeText = [NSString stringWithFormat:@"Could not create file '%@'.", name];
		[err runModal];
	}
}

- (void)newFolder:(id)sender
{
	NSInteger row = sOutlineView ? sOutlineView.clickedRow : -1;
	std::string parentDir;

	if (row >= 0)
	{
		FileBrowserNodeWrapper* wrapper = (FileBrowserNodeWrapper*)[sOutlineView itemAtRow:row];
		if (wrapper && wrapper.node)
		{
			if (wrapper.node->isDirectory)
				parentDir = wrapper.node->path;
			else
			{
				NSString* p = [NSString stringWithUTF8String:wrapper.node->path.c_str()];
				parentDir = [[p stringByDeletingLastPathComponent] UTF8String];
			}
		}
	}

	if (parentDir.empty())
		parentDir = sRootNode.path;

	NSAlert* alert = [[NSAlert alloc] init];
	alert.messageText = @"New Folder";
	alert.informativeText = @"Enter a name for the new folder:";
	[alert addButtonWithTitle:@"Create"];
	[alert addButtonWithTitle:@"Cancel"];

	NSTextField* input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 260, 24)];
	input.stringValue = @"";
	input.placeholderString = @"folder-name";
	alert.accessoryView = input;
	[alert.window setInitialFirstResponder:input];

	if ([alert runModal] != NSAlertFirstButtonReturn)
		return;

	NSString* name = input.stringValue;
	if (name.length == 0)
		return;

	if ([name containsString:@"/"] || [name containsString:@":"])
	{
		NSAlert* err = [[NSAlert alloc] init];
		err.messageText = @"Invalid Name";
		err.informativeText = @"Folder name cannot contain '/' or ':' characters.";
		[err runModal];
		return;
	}

	NSString* dir = [NSString stringWithUTF8String:parentDir.c_str()];
	NSString* fullPath = [dir stringByAppendingPathComponent:name];

	if ([[NSFileManager defaultManager] fileExistsAtPath:fullPath])
	{
		NSAlert* err = [[NSAlert alloc] init];
		err.messageText = @"Already Exists";
		err.informativeText = [NSString stringWithFormat:@"'%@' already exists.", name];
		[err runModal];
		return;
	}

	NSError* error = nil;
	BOOL created = [[NSFileManager defaultManager] createDirectoryAtPath:fullPath
	                                        withIntermediateDirectories:NO
	                                                        attributes:nil
	                                                             error:&error];
	if (!created)
	{
		NSAlert* err = error ? [NSAlert alertWithError:error] : [[NSAlert alloc] init];
		if (!error)
		{
			err.messageText = @"Error";
			err.informativeText = [NSString stringWithFormat:@"Could not create folder '%@'.", name];
		}
		[err runModal];
	}
}

- (void)renameItem:(id)sender
{
	NSInteger row = sOutlineView ? sOutlineView.clickedRow : -1;
	if (row < 0)
		return;

	FileBrowserNodeWrapper* wrapper = (FileBrowserNodeWrapper*)[sOutlineView itemAtRow:row];
	if (!wrapper || !wrapper.node)
		return;

	FileBrowserNode* node = wrapper.node;
	NSString* oldPath = [NSString stringWithUTF8String:node->path.c_str()];
	NSString* oldName = [NSString stringWithUTF8String:node->filename.c_str()];

	NSAlert* alert = [[NSAlert alloc] init];
	alert.messageText = @"Rename";
	alert.informativeText = [NSString stringWithFormat:@"Enter a new name for '%@':", oldName];
	[alert addButtonWithTitle:@"Rename"];
	[alert addButtonWithTitle:@"Cancel"];

	NSTextField* input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 260, 24)];
	input.stringValue = oldName;
	alert.accessoryView = input;
	[alert.window setInitialFirstResponder:input];

	if ([alert runModal] != NSAlertFirstButtonReturn)
		return;

	NSString* newName = input.stringValue;
	if (newName.length == 0 || [newName isEqualToString:oldName])
		return;

	if ([newName containsString:@"/"] || [newName containsString:@":"])
	{
		NSAlert* err = [[NSAlert alloc] init];
		err.messageText = @"Invalid Name";
		err.informativeText = @"Name cannot contain '/' or ':' characters.";
		[err runModal];
		return;
	}

	NSString* parentDir = [oldPath stringByDeletingLastPathComponent];
	NSString* newPath = [parentDir stringByAppendingPathComponent:newName];

	if ([[NSFileManager defaultManager] fileExistsAtPath:newPath])
	{
		NSAlert* err = [[NSAlert alloc] init];
		err.messageText = @"Already Exists";
		err.informativeText = [NSString stringWithFormat:@"'%@' already exists.", newName];
		[err runModal];
		return;
	}

	NSError* error = nil;
	[[NSFileManager defaultManager] moveItemAtPath:oldPath toPath:newPath error:&error];
	if (error)
	{
		NSAlert* err = [NSAlert alertWithError:error];
		[err runModal];
	}
}

- (void)deleteItem:(id)sender
{
	NSInteger row = sOutlineView ? sOutlineView.clickedRow : -1;
	if (row < 0)
		return;

	FileBrowserNodeWrapper* wrapper = (FileBrowserNodeWrapper*)[sOutlineView itemAtRow:row];
	if (!wrapper || !wrapper.node)
		return;

	FileBrowserNode* node = wrapper.node;
	NSString* name = [NSString stringWithUTF8String:node->filename.c_str()];
	NSString* path = [NSString stringWithUTF8String:node->path.c_str()];

	NSAlert* confirm = [[NSAlert alloc] init];
	confirm.messageText = @"Delete";
	confirm.informativeText = [NSString stringWithFormat:@"Move '%@' to Trash?", name];
	[confirm addButtonWithTitle:@"Move to Trash"];
	[confirm addButtonWithTitle:@"Cancel"];
	confirm.alertStyle = NSAlertStyleWarning;

	if ([confirm runModal] != NSAlertFirstButtonReturn)
		return;

	NSURL* url = [NSURL fileURLWithPath:path];
	[[NSWorkspace sharedWorkspace] recycleURLs:@[url] completionHandler:nil];
}

- (void)revealInFinder:(id)sender
{
	NSInteger row = sOutlineView ? sOutlineView.clickedRow : -1;
	if (row < 0)
		return;

	FileBrowserNodeWrapper* wrapper = (FileBrowserNodeWrapper*)[sOutlineView itemAtRow:row];
	if (!wrapper || !wrapper.node)
		return;

	NSString* path = [NSString stringWithUTF8String:wrapper.node->path.c_str()];
	[[NSWorkspace sharedWorkspace] selectFile:path inFileViewerRootedAtPath:@""];
}

- (void)copyPath:(id)sender
{
	NSInteger row = sOutlineView ? sOutlineView.clickedRow : -1;
	if (row < 0)
		return;

	FileBrowserNodeWrapper* wrapper = (FileBrowserNodeWrapper*)[sOutlineView itemAtRow:row];
	if (!wrapper || !wrapper.node)
		return;

	NSString* path = [NSString stringWithUTF8String:wrapper.node->path.c_str()];
	NSPasteboard* pb = [NSPasteboard generalPasteboard];
	[pb clearContents];
	[pb setString:path forType:NSPasteboardTypeString];
}

// ---------------------------------------------------------------------------
// Context menu delegate
// ---------------------------------------------------------------------------
- (void)menuNeedsUpdate:(NSMenu*)menu
{
	[menu removeAllItems];

	NSInteger row = sOutlineView ? sOutlineView.clickedRow : -1;

	// Always offer New File and New Folder
	[menu addItemWithTitle:@"New File" action:@selector(newFile:) keyEquivalent:@""];
	[menu addItemWithTitle:@"New Folder" action:@selector(newFolder:) keyEquivalent:@""];

	if (row >= 0)
	{
		[menu addItem:[NSMenuItem separatorItem]];
		[menu addItemWithTitle:@"Rename" action:@selector(renameItem:) keyEquivalent:@""];
		[menu addItemWithTitle:@"Delete" action:@selector(deleteItem:) keyEquivalent:@""];
		[menu addItem:[NSMenuItem separatorItem]];
		[menu addItemWithTitle:@"Reveal in Finder" action:@selector(revealInFinder:) keyEquivalent:@""];
		[menu addItemWithTitle:@"Copy Path" action:@selector(copyPath:) keyEquivalent:@""];
	}

	for (NSMenuItem* item in menu.itemArray)
	{
		if (item.action)
			item.target = self;
	}
}

@end

// ---------------------------------------------------------------------------
// Placeholder view (shown when no folder is open)
// ---------------------------------------------------------------------------
static void createPlaceholderView(NSView* container)
{
	sPlaceholderView = [[NSView alloc] initWithFrame:NSZeroRect];
	sPlaceholderView.translatesAutoresizingMaskIntoConstraints = NO;
	[container addSubview:sPlaceholderView];

	NSTextField* label = [NSTextField labelWithString:@"Open a folder to get started"];
	label.translatesAutoresizingMaskIntoConstraints = NO;
	label.alignment = NSTextAlignmentCenter;
	label.textColor = NSColor.secondaryLabelColor;
	label.font = [NSFont systemFontOfSize:12];
	label.lineBreakMode = NSLineBreakByWordWrapping;
	label.preferredMaxLayoutWidth = 160;
	[sPlaceholderView addSubview:label];

	NSButton* openButton = [NSButton buttonWithTitle:@"Open Folder"
	                                          target:nil
	                                          action:@selector(openFolderButtonClicked:)];
	openButton.translatesAutoresizingMaskIntoConstraints = NO;
	openButton.bezelStyle = NSBezelStyleRounded;
	[sPlaceholderView addSubview:openButton];

	[NSLayoutConstraint activateConstraints:@[
		[sPlaceholderView.topAnchor constraintEqualToAnchor:container.topAnchor],
		[sPlaceholderView.leadingAnchor constraintEqualToAnchor:container.leadingAnchor],
		[sPlaceholderView.trailingAnchor constraintEqualToAnchor:container.trailingAnchor],
		[sPlaceholderView.bottomAnchor constraintEqualToAnchor:container.bottomAnchor],
		[label.centerXAnchor constraintEqualToAnchor:sPlaceholderView.centerXAnchor],
		[label.centerYAnchor constraintEqualToAnchor:sPlaceholderView.centerYAnchor constant:-16],
		[openButton.centerXAnchor constraintEqualToAnchor:sPlaceholderView.centerXAnchor],
		[openButton.topAnchor constraintEqualToAnchor:label.bottomAnchor constant:8]
	]];
}

// ---------------------------------------------------------------------------
// Responder for "Open Folder" button in placeholder
// ---------------------------------------------------------------------------
@interface FileBrowserPlaceholderResponder : NSObject
- (void)openFolderButtonClicked:(id)sender;
@end

static FileBrowserPlaceholderResponder* sPlaceholderResponder = nil;

@implementation FileBrowserPlaceholderResponder
- (void)openFolderButtonClicked:(id)sender
{
	openFolderWithDialog();
}
@end

// ---------------------------------------------------------------------------
// showOutlineView / showPlaceholder — toggle between tree and placeholder
// ---------------------------------------------------------------------------
static void showOutlineView()
{
	if (sScrollView) sScrollView.hidden = NO;
	if (sPlaceholderView) sPlaceholderView.hidden = YES;
}

static void showPlaceholder()
{
	if (sScrollView) sScrollView.hidden = YES;
	if (sPlaceholderView) sPlaceholderView.hidden = NO;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void initializeFileBrowserPanel()
{
	if (!ctx().mainWindow || !ctx().editorContainer || sContainer)
		return;

	NSView* contentView = ctx().mainWindow.contentView;
	if (!contentView)
		return;

	sController = [[FileBrowserController alloc] init];
	sPlaceholderResponder = [[FileBrowserPlaceholderResponder alloc] init];

	CGFloat width = static_cast<CGFloat>(ctx().leftPanelWidth);
	if (width < 120) width = 120;
	if (width > 360) width = 360;
	ctx().leftPanelWidth = static_cast<int>(width);

	sContainer = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, width, 100)];
	sContainer.autoresizingMask = NSViewWidthSizable;
	[contentView addSubview:sContainer];

	// Title label
	sTitleLabel = [NSTextField labelWithString:@"File Browser"];
	sTitleLabel.translatesAutoresizingMaskIntoConstraints = NO;
	sTitleLabel.font = [NSFont systemFontOfSize:11 weight:NSFontWeightSemibold];
	sTitleLabel.textColor = NSColor.secondaryLabelColor;
	[sContainer addSubview:sTitleLabel];

	// Scroll view with outline view
	sScrollView = [[NSScrollView alloc] initWithFrame:NSZeroRect];
	sScrollView.translatesAutoresizingMaskIntoConstraints = NO;
	sScrollView.hasVerticalScroller = YES;
	sScrollView.hasHorizontalScroller = NO;
	sScrollView.autohidesScrollers = YES;
	sScrollView.borderType = NSBezelBorder;
	[sContainer addSubview:sScrollView];

	FileBrowserOutlineView* outline = [[FileBrowserOutlineView alloc] initWithFrame:NSZeroRect];
	outline.headerView = nil;
	outline.usesAlternatingRowBackgroundColors = YES;
	NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:@"FileBrowserColumn"];
	column.resizingMask = NSTableColumnAutoresizingMask;
	[outline addTableColumn:column];
	outline.outlineTableColumn = column;
	outline.dataSource = sController;
	outline.delegate = sController;
	outline.target = sController;
	outline.action = @selector(outlineViewClicked:);
	sOutlineView = outline;
	sScrollView.documentView = outline;

	// Context menu
	NSMenu* contextMenu = [[NSMenu alloc] initWithTitle:@""];
	contextMenu.delegate = sController;
	outline.menu = contextMenu;

	// Placeholder view
	createPlaceholderView(sContainer);

	// Wire up the Open Folder button in the placeholder
	for (NSView* subview in sPlaceholderView.subviews)
	{
		if ([subview isKindOfClass:[NSButton class]])
		{
			NSButton* btn = (NSButton*)subview;
			btn.target = sPlaceholderResponder;
			btn.action = @selector(openFolderButtonClicked:);
		}
	}

	// Layout constraints
	[NSLayoutConstraint activateConstraints:@[
		[sTitleLabel.topAnchor constraintEqualToAnchor:sContainer.topAnchor constant:6],
		[sTitleLabel.leadingAnchor constraintEqualToAnchor:sContainer.leadingAnchor constant:6],
		[sScrollView.topAnchor constraintEqualToAnchor:sTitleLabel.bottomAnchor constant:4],
		[sScrollView.leadingAnchor constraintEqualToAnchor:sContainer.leadingAnchor constant:6],
		[sScrollView.trailingAnchor constraintEqualToAnchor:sContainer.trailingAnchor constant:-6],
		[sScrollView.bottomAnchor constraintEqualToAnchor:sContainer.bottomAnchor constant:-6]
	]];

	// Check if there's a saved root path to auto-open
	if (!ctx().fileBrowserRootPath.empty())
	{
		NSString* rootPath = [NSString stringWithUTF8String:ctx().fileBrowserRootPath.c_str()];
		if ([[NSFileManager defaultManager] fileExistsAtPath:rootPath])
		{
			openFolderInFileBrowser(ctx().fileBrowserRootPath);
			return;
		}
	}

	// Show placeholder if no folder is open
	showPlaceholder();
}

void destroyFileBrowserPanel()
{
	stopFSEventsMonitoring();

	if (sScrollView)
	{
		[sScrollView removeFromSuperview];
		sScrollView = nil;
	}
	sOutlineView = nil;
	if (sPlaceholderView)
	{
		[sPlaceholderView removeFromSuperview];
		sPlaceholderView = nil;
	}
	if (sTitleLabel)
	{
		[sTitleLabel removeFromSuperview];
		sTitleLabel = nil;
	}
	if (sContainer)
	{
		[sContainer removeFromSuperview];
		sContainer = nil;
	}
	sController = nil;
	sPlaceholderResponder = nil;
	sRootNode = FileBrowserNode();
}

void setFileBrowserEnabled(bool enabled)
{
	ctx().fileBrowserEnabled = enabled;
	if (enabled)
	{
		if (!sContainer)
			initializeFileBrowserPanel();
	}
	relayoutPanels();
}

void openFolderInFileBrowser(const std::string& path)
{
	if (path.empty())
		return;

	// Stop existing monitoring
	stopFSEventsMonitoring();

	// Set root node
	sRootNode = FileBrowserNode();
	sRootNode.path = path;
	sRootNode.isDirectory = true;

	NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
	sRootNode.filename = [[nsPath lastPathComponent] UTF8String];

	// Update title to folder name
	if (sTitleLabel)
		sTitleLabel.stringValue = [NSString stringWithUTF8String:sRootNode.filename.c_str()];

	// Save the root path in context
	ctx().fileBrowserRootPath = path;

	// Show outline, hide placeholder
	showOutlineView();

	// Reload the outline view
	if (sOutlineView)
		[sOutlineView reloadData];

	// Start FSEvents monitoring
	startFSEventsMonitoring(path);

	// Refresh file switcher path display
	reloadFileSwitcherData();
}

void openFolderWithDialog()
{
	NSOpenPanel* panel = [NSOpenPanel openPanel];
	panel.canChooseDirectories = YES;
	panel.canChooseFiles = NO;
	panel.allowsMultipleSelection = NO;
	panel.prompt = @"Open";
	panel.message = @"Choose a folder to browse";

	if ([panel runModal] != NSModalResponseOK)
		return;

	NSURL* url = panel.URL;
	if (!url || !url.path)
		return;

	std::string path = [url.path UTF8String];
	openFolderInFileBrowser(path);

	// Auto-enable the file browser if it is not already enabled
	if (!ctx().fileBrowserEnabled)
		setFileBrowserEnabled(true);
}

void* fileBrowserContainerView()
{
	return (__bridge void*)sContainer;
}
