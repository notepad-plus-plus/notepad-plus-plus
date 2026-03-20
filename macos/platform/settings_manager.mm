// Settings Manager for Notepad++ macOS Port
// Persists user settings to ~/.npp-macos/settings.json using NSJSONSerialization.

#import <Foundation/Foundation.h>
#include "settings_manager.h"

namespace
{
static NSString* stringFromFileSystemPath(const std::string& fileSystemPath)
{
	if (fileSystemPath.empty())
		return nil;

	return [[NSFileManager defaultManager] stringWithFileSystemRepresentation:fileSystemPath.c_str()
	                                                                   length:fileSystemPath.size()];
}
}

SettingsManager& SettingsManager::instance()
{
	static SettingsManager mgr;
	return mgr;
}

std::string SettingsManager::settingsDir() const
{
	NSString* home = NSHomeDirectory();
	NSString* dir = [home stringByAppendingPathComponent:@".npp-macos"];
	const char* fs = [dir fileSystemRepresentation];
	if (!fs) return "";
	return std::string(fs);
}

std::string SettingsManager::settingsPath() const
{
	std::string dir = settingsDir();
	if (dir.empty()) return "";
	return dir + "/settings.json";
}

bool SettingsManager::load()
{
	NSString* path = stringFromFileSystemPath(settingsPath());
	if (!path) return false;

	NSData* data = [NSData dataWithContentsOfFile:path];
	if (!data) return false;

	NSError* error = nil;
	id parsed = [NSJSONSerialization JSONObjectWithData:data
	                                            options:0
	                                              error:&error];
	if (!parsed || error) return false;
	if (![parsed isKindOfClass:[NSDictionary class]]) return false;
	NSDictionary* json = parsed;

	// Window geometry
	if ([json[@"windowX"] isKindOfClass:[NSNumber class]])      settings.windowX = [json[@"windowX"] doubleValue];
	if ([json[@"windowY"] isKindOfClass:[NSNumber class]])      settings.windowY = [json[@"windowY"] doubleValue];
	if ([json[@"windowWidth"] isKindOfClass:[NSNumber class]])  settings.windowWidth = [json[@"windowWidth"] doubleValue];
	if ([json[@"windowHeight"] isKindOfClass:[NSNumber class]]) settings.windowHeight = [json[@"windowHeight"] doubleValue];

	// Editor preferences
	if ([json[@"fontName"] isKindOfClass:[NSString class]])
	{
		const char* fontName = [json[@"fontName"] UTF8String];
		if (fontName)
			settings.fontName = fontName;
	}
	if ([json[@"fontSize"] isKindOfClass:[NSNumber class]])  settings.fontSize = [json[@"fontSize"] intValue];
	if ([json[@"tabWidth"] isKindOfClass:[NSNumber class]])  settings.tabWidth = [json[@"tabWidth"] intValue];

	// View state
	if ([json[@"wordWrap"] isKindOfClass:[NSNumber class]])        settings.wordWrap = [json[@"wordWrap"] boolValue];
	if ([json[@"showLineNumbers"] isKindOfClass:[NSNumber class]]) settings.showLineNumbers = [json[@"showLineNumbers"] boolValue];
	if ([json[@"zoomLevel"] isKindOfClass:[NSNumber class]])       settings.zoomLevel = [json[@"zoomLevel"] intValue];
	if ([json[@"showCaretLine"] isKindOfClass:[NSNumber class]])   settings.showCaretLine = [json[@"showCaretLine"] boolValue];
	if ([json[@"autoIndent"] isKindOfClass:[NSNumber class]])      settings.autoIndent = [json[@"autoIndent"] boolValue];
	if ([json[@"useTabs"] isKindOfClass:[NSNumber class]])         settings.useTabs = [json[@"useTabs"] boolValue];
	if ([json[@"showWhitespace"] isKindOfClass:[NSNumber class]])  settings.showWhitespace = [json[@"showWhitespace"] boolValue];
	if ([json[@"showEol"] isKindOfClass:[NSNumber class]])         settings.showEol = [json[@"showEol"] boolValue];
	if ([json[@"showIndentGuides"] isKindOfClass:[NSNumber class]])settings.showIndentGuides = [json[@"showIndentGuides"] boolValue];

	// Recent files
	settings.recentFiles.clear();
	NSArray* recent = json[@"recentFiles"];
	if ([recent isKindOfClass:[NSArray class]])
	{
		for (NSString* f in recent)
		{
			if ([f isKindOfClass:[NSString class]])
			{
				const char* recentFile = [f UTF8String];
				if (recentFile)
					settings.recentFiles.push_back(recentFile);
			}
		}
	}

	return true;
}

bool SettingsManager::save()
{
	// Ensure directory exists
	NSString* dir = stringFromFileSystemPath(settingsDir());
	if (!dir) return false;

	if (![[NSFileManager defaultManager] createDirectoryAtPath:dir
	                               withIntermediateDirectories:YES
	                                               attributes:nil
	                                                    error:nil])
	{
		return false;
	}

	// Build JSON dictionary
	NSMutableArray* recentArr = [NSMutableArray array];
	for (const auto& f : settings.recentFiles)
	{
		NSString* recent = [NSString stringWithUTF8String:f.c_str()];
		if (recent)
			[recentArr addObject:recent];
	}

	NSString* fontName = [NSString stringWithUTF8String:settings.fontName.c_str()];
	if (!fontName)
		fontName = @"";

	NSDictionary* json = @{
		@"windowX":      @(settings.windowX),
		@"windowY":      @(settings.windowY),
		@"windowWidth":  @(settings.windowWidth),
		@"windowHeight": @(settings.windowHeight),
		@"fontName":     fontName,
		@"fontSize":     @(settings.fontSize),
		@"tabWidth":     @(settings.tabWidth),
		@"wordWrap":     @(settings.wordWrap),
		@"showLineNumbers": @(settings.showLineNumbers),
		@"zoomLevel":    @(settings.zoomLevel),
		@"showCaretLine": @(settings.showCaretLine),
		@"autoIndent":   @(settings.autoIndent),
		@"useTabs":      @(settings.useTabs),
		@"showWhitespace": @(settings.showWhitespace),
		@"showEol":      @(settings.showEol),
		@"showIndentGuides": @(settings.showIndentGuides),
		@"recentFiles":  recentArr
	};

	NSError* error = nil;
	NSData* data = [NSJSONSerialization dataWithJSONObject:json
	                                              options:NSJSONWritingPrettyPrinted
	                                                error:&error];
	if (!data || error) return false;

	NSString* path = stringFromFileSystemPath(settingsPath());
	if (!path) return false;

	return [data writeToFile:path atomically:YES];
}
