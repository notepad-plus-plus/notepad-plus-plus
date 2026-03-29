// macro_manager.mm — Macro recording, playback, and persistence
// Part of the Notepad++ macOS port.

#import <Cocoa/Cocoa.h>
#include "macro_manager.h"
#include "scintilla_bridge.h"
#include "npp_constants.h"

// Messages that use lParam as a string pointer and need text copying during record.
// This list matches Notepad++ upstream's recordedMacroStep handling for Scintilla
// messages that pass string data via lParam.
static bool isStringMessage(int message)
{
	switch (message)
	{
		case SCI_REPLACESEL:      // 2170
		case SCI_INSERTTEXT:      // 2003
		case SCI_SEARCHINTARGET:  // 2197
		case SCI_SETTEXT:         // 2181
		case SCI_REPLACETARGET:   // 2194 — used by Find & Replace
		case SCI_REPLACETARGETRE: // 2195
		case SCI_SEARCHNEXT:      // 2367
		case SCI_SEARCHPREV:      // 2368
		case SCI_ADDTEXT:         // 2001 (via length + lParam)
		case SCI_TEXTWIDTH:       // 2276
		case SCI_STYLESETFONT:    // 2056
			return true;
		default:
			return false;
	}
}

MacroManager& MacroManager::instance()
{
	static MacroManager mgr;
	return mgr;
}

// ============================================================
// Recording
// ============================================================

void MacroManager::startRecording(void* sci)
{
	_currentMacro.clear();
	_recording = true;
	ScintillaBridge_sendMessage(sci, SCI_STARTRECORD, 0, 0);
}

void MacroManager::stopRecording(void* sci)
{
	_recording = false;
	ScintillaBridge_sendMessage(sci, SCI_STOPRECORD, 0, 0);
}

bool MacroManager::isRecording() const
{
	return _recording;
}

void MacroManager::recordStep(int message, uintptr_t wParam, intptr_t lParam)
{
	// Normalize newline insertions to SCI_NEWLINE so playback uses the
	// document's EOL mode rather than replaying a raw \r or \n byte.
	// This matches Notepad++ upstream behavior.
	if (message == SCI_REPLACESEL && lParam != 0)
	{
		const char* str = reinterpret_cast<const char*>(lParam);
		if ((str[0] == '\n' || str[0] == '\r') && str[1] == '\0')
		{
			MacroStep nlStep;
			nlStep.message = SCI_NEWLINE;
			nlStep.wParam = 0;
			nlStep.lParam = 0;
			_currentMacro.push_back(nlStep);
			return;
		}
	}

	MacroStep step;
	step.message = message;
	step.wParam = wParam;

	if (isStringMessage(message) && lParam != 0)
	{
		const char* str = reinterpret_cast<const char*>(lParam);
		// For messages where wParam is an explicit byte length and the buffer
		// is not guaranteed NUL-terminated, copy using the length to avoid
		// reading past the buffer. Fall back to NUL-terminated for the rest.
		switch (message)
		{
			case SCI_ADDTEXT:         // wParam = length
			case SCI_REPLACETARGET:   // wParam = length (-1 for NUL-term)
			case SCI_REPLACETARGETRE: // wParam = length (-1 for NUL-term)
				if (wParam != static_cast<uintptr_t>(-1))
					step.text.assign(str, static_cast<size_t>(wParam));
				else
					step.text = str;
				break;
			default:
				step.text = str;
				break;
		}
		step.lParam = 0; // Marker: use text field on playback
	}
	else
	{
		step.lParam = lParam;
	}

	_currentMacro.push_back(step);
}

// ============================================================
// Playback
// ============================================================

void MacroManager::playback(void* sci)
{
	if (_currentMacro.empty() || !sci)
		return;

	for (const auto& step : _currentMacro)
	{
		intptr_t lp = step.lParam;
		if (!step.text.empty())
			lp = reinterpret_cast<intptr_t>(step.text.c_str());
		ScintillaBridge_sendMessage(sci, step.message, step.wParam, lp);
	}
}

void MacroManager::playbackMultiple(void* sci, int count)
{
	if (_currentMacro.empty() || !sci)
		return;

	// If count is 0 or negative, prompt the user for a repeat count
	if (count <= 0)
	{
		NSAlert* alert = [[NSAlert alloc] init];
		alert.messageText = @"Run Macro Multiple Times";
		alert.informativeText = @"Enter number of times to repeat:";
		[alert addButtonWithTitle:@"OK"];
		[alert addButtonWithTitle:@"Cancel"];

		NSTextField* input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 200, 24)];
		input.stringValue = @"10";
		alert.accessoryView = input;

		NSModalResponse response = [alert runModal];
		if (response != NSAlertFirstButtonReturn)
			return;

		count = [input intValue];
		if (count <= 0)
			return;
	}

	for (int i = 0; i < count; ++i)
	{
		playback(sci);
	}
}

bool MacroManager::hasRecordedMacro() const
{
	return !_currentMacro.empty();
}

// ============================================================
// Persistence
// ============================================================

std::string MacroManager::macrosDir() const
{
	NSString* home = NSHomeDirectory();
	NSString* dir = [home stringByAppendingPathComponent:@".npp-macos"];
	const char* fs = [dir fileSystemRepresentation];
	if (!fs) return "";
	return std::string(fs);
}

std::string MacroManager::macrosPath() const
{
	std::string dir = macrosDir();
	if (dir.empty()) return "";
	return dir + "/macros.json";
}

static NSString* stringFromFileSystemPath(const std::string& path)
{
	if (path.empty()) return nil;
	return [[NSFileManager defaultManager] stringWithFileSystemRepresentation:path.c_str()
	                                                                   length:path.size()];
}

void MacroManager::saveMacro(const std::string& name)
{
	if (_currentMacro.empty())
		return;

	NSString* macroName = nil;

	if (name.empty())
	{
		// Prompt for macro name
		NSAlert* alert = [[NSAlert alloc] init];
		alert.messageText = @"Save Macro";
		alert.informativeText = @"Enter a name for this macro:";
		[alert addButtonWithTitle:@"Save"];
		[alert addButtonWithTitle:@"Cancel"];

		NSTextField* input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 200, 24)];
		input.stringValue = @"My Macro";
		alert.accessoryView = input;

		NSModalResponse response = [alert runModal];
		if (response != NSAlertFirstButtonReturn)
			return;

		macroName = input.stringValue;
		if (macroName.length == 0)
			return;
	}
	else
	{
		macroName = [NSString stringWithUTF8String:name.c_str()];
	}

	// Ensure directory exists
	NSString* dir = stringFromFileSystemPath(macrosDir());
	if (!dir) return;

	[[NSFileManager defaultManager] createDirectoryAtPath:dir
	                           withIntermediateDirectories:YES
	                                           attributes:nil
	                                                error:nil];

	// Load existing macros file
	NSMutableDictionary* allMacros = [NSMutableDictionary dictionary];
	NSString* path = stringFromFileSystemPath(macrosPath());
	if (path)
	{
		NSData* existingData = [NSData dataWithContentsOfFile:path];
		if (existingData)
		{
			id parsed = [NSJSONSerialization JSONObjectWithData:existingData options:0 error:nil];
			if ([parsed isKindOfClass:[NSDictionary class]])
				allMacros = [parsed mutableCopy];
		}
	}

	// Convert current macro to JSON array
	NSMutableArray* stepsArray = [NSMutableArray array];
	for (const auto& step : _currentMacro)
	{
		NSMutableDictionary* stepDict = [NSMutableDictionary dictionary];
		stepDict[@"message"] = @(step.message);
		stepDict[@"wParam"] = @(static_cast<long long>(step.wParam));
		stepDict[@"lParam"] = @(static_cast<long long>(step.lParam));
		if (!step.text.empty())
		{
			NSString* text = [NSString stringWithUTF8String:step.text.c_str()];
			if (text)
				stepDict[@"text"] = text;
		}
		[stepsArray addObject:stepDict];
	}

	allMacros[macroName] = stepsArray;

	// Write back
	NSError* error = nil;
	NSData* data = [NSJSONSerialization dataWithJSONObject:allMacros
	                                              options:NSJSONWritingPrettyPrinted
	                                                error:&error];
	if (!data || error)
	{
		NSAlert* errAlert = [[NSAlert alloc] init];
		errAlert.messageText = @"Save Failed";
		errAlert.informativeText = error ? error.localizedDescription : @"Could not serialize macro data.";
		[errAlert runModal];
		return;
	}

	if (path && ![data writeToFile:path atomically:YES])
	{
		NSAlert* errAlert = [[NSAlert alloc] init];
		errAlert.messageText = @"Save Failed";
		errAlert.informativeText = @"Could not write macros file to disk.";
		[errAlert runModal];
	}
}

void MacroManager::loadMacro(const std::string& name)
{
	NSString* path = stringFromFileSystemPath(macrosPath());
	if (!path) return;

	NSData* data = [NSData dataWithContentsOfFile:path];
	if (!data)
	{
		NSAlert* errAlert = [[NSAlert alloc] init];
		errAlert.messageText = @"Load Failed";
		errAlert.informativeText = @"No saved macros file found.";
		[errAlert runModal];
		return;
	}

	NSError* jsonError = nil;
	id parsed = [NSJSONSerialization JSONObjectWithData:data options:0 error:&jsonError];
	if (![parsed isKindOfClass:[NSDictionary class]])
	{
		NSAlert* errAlert = [[NSAlert alloc] init];
		errAlert.messageText = @"Load Failed";
		errAlert.informativeText = jsonError ? jsonError.localizedDescription : @"Macros file is corrupted.";
		[errAlert runModal];
		return;
	}
	NSDictionary* allMacros = parsed;

	if (allMacros.count == 0)
	{
		NSAlert* errAlert = [[NSAlert alloc] init];
		errAlert.messageText = @"No Macros";
		errAlert.informativeText = @"No saved macros found.";
		[errAlert runModal];
		return;
	}

	NSString* macroName = nil;

	if (name.empty())
	{
		// Prompt user to select a macro
		NSArray<NSString*>* names = [allMacros.allKeys sortedArrayUsingSelector:@selector(compare:)];
		if (names.count == 0)
			return;

		NSAlert* alert = [[NSAlert alloc] init];
		alert.messageText = @"Load Macro";
		alert.informativeText = @"Select a macro to load:";
		[alert addButtonWithTitle:@"Load"];
		[alert addButtonWithTitle:@"Cancel"];

		NSPopUpButton* popup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(0, 0, 200, 24) pullsDown:NO];
		for (NSString* n in names)
			[popup addItemWithTitle:n];
		alert.accessoryView = popup;

		NSModalResponse response = [alert runModal];
		if (response != NSAlertFirstButtonReturn)
			return;

		macroName = popup.titleOfSelectedItem;
	}
	else
	{
		macroName = [NSString stringWithUTF8String:name.c_str()];
	}

	if (!macroName) return;

	NSArray* steps = allMacros[macroName];
	if (![steps isKindOfClass:[NSArray class]])
		return;

	_currentMacro.clear();
	for (NSDictionary* stepDict in steps)
	{
		if (![stepDict isKindOfClass:[NSDictionary class]])
			continue;

		MacroStep step;
		step.message = [stepDict[@"message"] intValue];
		step.wParam = static_cast<uintptr_t>([stepDict[@"wParam"] longLongValue]);
		step.lParam = static_cast<intptr_t>([stepDict[@"lParam"] longLongValue]);

		NSString* text = stepDict[@"text"];
		if ([text isKindOfClass:[NSString class]])
		{
			const char* utf8 = [text UTF8String];
			if (utf8)
				step.text = utf8;
		}

		_currentMacro.push_back(step);
	}
}

std::vector<std::string> MacroManager::savedMacroNames() const
{
	std::vector<std::string> result;

	NSString* path = stringFromFileSystemPath(macrosPath());
	if (!path) return result;

	NSData* data = [NSData dataWithContentsOfFile:path];
	if (!data) return result;

	id parsed = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
	if (![parsed isKindOfClass:[NSDictionary class]]) return result;

	NSDictionary* allMacros = parsed;
	for (NSString* key in [allMacros.allKeys sortedArrayUsingSelector:@selector(compare:)])
	{
		const char* utf8 = [key UTF8String];
		if (utf8)
			result.push_back(utf8);
	}

	return result;
}
