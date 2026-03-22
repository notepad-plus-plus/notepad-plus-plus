// find_in_files.mm — Find in Files dialog implementation
#include "find_in_files.h"
#import <Cocoa/Cocoa.h>
#include <fnmatch.h>
#include <string>
#include <vector>
#include <sstream>

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
// Public API stubs
// ---------------------------------------------------------------------------

void showFindInFilesDlg()
{
	// Stub — will be implemented in Task 8
}

bool isFIFSearchRunning()
{
	return false;
}

void cancelFIFSearch()
{
	// No-op — will be implemented when search engine is added
}

// Suppress unused-function warnings for static helpers used in later tasks
__attribute__((used)) static auto _fifRef1 = &enumerateFiles;
__attribute__((used)) static auto _fifRef2 = &isBinaryFile;
