// Win32 File Dialogs shim: NSOpenPanel/NSSavePanel backing
// Extracted from win32_menu.mm as part of shim modularization.

#import <Cocoa/Cocoa.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include "windows.h"
#include "commdlg.h"
#include "handle_registry.h"
#include "win32_string_helpers.h"

// ============================================================
// File filter parsing helpers
// ============================================================

// Parse Win32 filter string: "Desc\0*.ext;*.ext2\0Desc2\0*.ext3\0\0"
static NSArray<NSArray<NSString*>*>* parseFilterPairs(const wchar_t* filter)
{
	NSMutableArray<NSArray<NSString*>*>* pairs = [NSMutableArray array];
	if (!filter) return pairs;

	const wchar_t* p = filter;
	while (*p)
	{
		NSString* desc = WideToNS(p);
		p += wcslen(p) + 1;
		if (!*p) break;
		NSString* patterns = WideToNS(p);
		p += wcslen(p) + 1;
		[pairs addObject:@[desc, patterns]];
	}
	return pairs;
}

// Extract file extensions from pattern string like "*.cpp;*.h;*.hpp"
static NSArray<NSString*>* extractExtensions(NSString* patterns)
{
	NSMutableArray<NSString*>* exts = [NSMutableArray array];
	NSArray<NSString*>* parts = [patterns componentsSeparatedByString:@";"];
	for (NSString* part in parts)
	{
		NSString* p = [part stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
		if ([p isEqualToString:@"*.*"])
			return @[]; // All files
		if ([p hasPrefix:@"*."])
		{
			NSString* ext = [p substringFromIndex:2];
			if (ext.length > 0)
				[exts addObject:ext];
		}
	}
	return exts;
}

// ============================================================
// File Open dialog
// ============================================================

BOOL GetOpenFileNameW(LPOPENFILENAMEW lpofn)
{
	if (!lpofn) return FALSE;

	@autoreleasepool {
		NSOpenPanel* panel = [NSOpenPanel openPanel];
		[panel setCanChooseFiles:YES];
		[panel setCanChooseDirectories:NO];
		[panel setAllowsMultipleSelection:(lpofn->Flags & OFN_ALLOWMULTISELECT) != 0];

		// Title
		if (lpofn->lpstrTitle)
			[panel setTitle:WideToNS(lpofn->lpstrTitle)];

		// Initial directory
		if (lpofn->lpstrInitialDir)
			[panel setDirectoryURL:[NSURL fileURLWithPath:WideToNS(lpofn->lpstrInitialDir)]];

		// File type filter
		NSArray<NSArray<NSString*>*>* filterPairs = parseFilterPairs(lpofn->lpstrFilter);
		if (filterPairs.count > 0)
		{
			// Use the filter at nFilterIndex (1-based) or first filter
			NSUInteger filterIdx = (lpofn->nFilterIndex > 0) ? lpofn->nFilterIndex - 1 : 0;
			if (filterIdx < filterPairs.count)
			{
				NSArray<NSString*>* exts = extractExtensions(filterPairs[filterIdx][1]);
				if (exts.count > 0)
				{
					NSMutableArray<UTType*>* types = [NSMutableArray array];
					for (NSString* ext in exts)
					{
						UTType* type = [UTType typeWithFilenameExtension:ext];
						if (type) [types addObject:type];
					}
					if (types.count > 0)
						[panel setAllowedContentTypes:types];
				}
			}
		}

		// Default extension
		if (lpofn->lpstrDefExt)
		{
			NSString* defExt = WideToNS(lpofn->lpstrDefExt);
			if (defExt.length > 0)
			{
				UTType* defType = [UTType typeWithFilenameExtension:defExt];
				if (defType && panel.allowedContentTypes.count == 0)
					[panel setAllowedContentTypes:@[defType]];
			}
		}

		NSModalResponse response = [panel runModal];
		if (response != NSModalResponseOK)
			return FALSE;

		NSArray<NSURL*>* urls = panel.URLs;
		if (!urls || urls.count == 0) return FALSE;

		if (lpofn->lpstrFile && lpofn->nMaxFile > 0)
		{
			if (urls.count == 1)
			{
				// Single file: standard behavior
				NSString* path = [urls[0] path];
				NSStringToWide(path, lpofn->lpstrFile, lpofn->nMaxFile);

				NSString* fileName = [path lastPathComponent];
				NSString* dirPath = [path stringByDeletingLastPathComponent];
				lpofn->nFileOffset = static_cast<WORD>(dirPath.length + 1);
				NSRange dotRange = [fileName rangeOfString:@"." options:NSBackwardsSearch];
				if (dotRange.location != NSNotFound)
					lpofn->nFileExtension = static_cast<WORD>(lpofn->nFileOffset + dotRange.location + 1);
				else
					lpofn->nFileExtension = 0;
			}
			else
			{
				// Multiple files: null-delimited full paths, double-null terminated
				wchar_t* buf = lpofn->lpstrFile;
				DWORD remaining = lpofn->nMaxFile;

				for (NSURL* fileUrl in urls)
				{
					NSString* path = [fileUrl path];
					NSData* data = [path dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
					size_t charCount = data.length / sizeof(wchar_t);

					if (charCount + 2 > remaining)
					{
						NSLog(@"Warning: multi-select buffer overflow, %lu files could not be included",
						      (unsigned long)(urls.count));
						break;
					}

					memcpy(buf, data.bytes, charCount * sizeof(wchar_t));
					buf[charCount] = L'\0';
					buf += charCount + 1;
					remaining -= (charCount + 1);
				}
				*buf = L'\0'; // Double-null terminator

				lpofn->Flags |= OFN_ALLOWMULTISELECT;
				lpofn->nFileOffset = 0;
				lpofn->nFileExtension = 0;
			}
		}

		if (urls.count == 1 && lpofn->lpstrFileTitle && lpofn->nMaxFileTitle > 0)
		{
			NSString* fileName = [[urls[0] path] lastPathComponent];
			NSStringToWide(fileName, lpofn->lpstrFileTitle, lpofn->nMaxFileTitle);
		}

		return TRUE;
	}
}

// ============================================================
// File Save dialog
// ============================================================

BOOL GetSaveFileNameW(LPOPENFILENAMEW lpofn)
{
	if (!lpofn) return FALSE;

	@autoreleasepool {
		NSSavePanel* panel = [NSSavePanel savePanel];

		// Title
		if (lpofn->lpstrTitle)
			[panel setTitle:WideToNS(lpofn->lpstrTitle)];

		// Initial directory
		if (lpofn->lpstrInitialDir)
			[panel setDirectoryURL:[NSURL fileURLWithPath:WideToNS(lpofn->lpstrInitialDir)]];

		// Default filename
		if (lpofn->lpstrFile && lpofn->lpstrFile[0] != L'\0')
		{
			NSString* path = WideToNS(lpofn->lpstrFile);
			NSString* fileName = [path lastPathComponent];
			if (fileName.length > 0)
				[panel setNameFieldStringValue:fileName];
		}

		// Overwrite prompt
		if (!(lpofn->Flags & OFN_OVERWRITEPROMPT))
			[panel setCanCreateDirectories:YES];

		// File type filter
		NSArray<NSArray<NSString*>*>* filterPairs = parseFilterPairs(lpofn->lpstrFilter);
		if (filterPairs.count > 0)
		{
			NSUInteger filterIdx = (lpofn->nFilterIndex > 0) ? lpofn->nFilterIndex - 1 : 0;
			if (filterIdx < filterPairs.count)
			{
				NSArray<NSString*>* exts = extractExtensions(filterPairs[filterIdx][1]);
				if (exts.count > 0)
				{
					NSMutableArray<UTType*>* types = [NSMutableArray array];
					for (NSString* ext in exts)
					{
						UTType* type = [UTType typeWithFilenameExtension:ext];
						if (type) [types addObject:type];
					}
					if (types.count > 0)
						[panel setAllowedContentTypes:types];
				}
			}
		}

		// Default extension
		if (lpofn->lpstrDefExt)
		{
			NSString* defExt = WideToNS(lpofn->lpstrDefExt);
			if (defExt.length > 0)
			{
				UTType* defType = [UTType typeWithFilenameExtension:defExt];
				if (defType && panel.allowedContentTypes.count == 0)
					[panel setAllowedContentTypes:@[defType]];
			}
		}

		NSModalResponse response = [panel runModal];
		if (response != NSModalResponseOK)
			return FALSE;

		NSURL* url = panel.URL;
		if (!url) return FALSE;

		NSString* path = [url path];
		if (lpofn->lpstrFile && lpofn->nMaxFile > 0)
		{
			NSStringToWide(path, lpofn->lpstrFile, lpofn->nMaxFile);

			NSString* fileName = [path lastPathComponent];
			NSString* dirPath = [path stringByDeletingLastPathComponent];
			lpofn->nFileOffset = static_cast<WORD>(dirPath.length + 1);
			NSRange dotRange = [fileName rangeOfString:@"." options:NSBackwardsSearch];
			if (dotRange.location != NSNotFound)
				lpofn->nFileExtension = static_cast<WORD>(lpofn->nFileOffset + dotRange.location + 1);
			else
				lpofn->nFileExtension = 0;
		}

		if (lpofn->lpstrFileTitle && lpofn->nMaxFileTitle > 0)
		{
			NSString* fileName = [path lastPathComponent];
			NSStringToWide(fileName, lpofn->lpstrFileTitle, lpofn->nMaxFileTitle);
		}

		return TRUE;
	}
}
