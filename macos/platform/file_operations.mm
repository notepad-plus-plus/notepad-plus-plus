// file_operations.mm — File I/O, encoding detection, open/save
// Part of the Notepad++ macOS port modular refactor.

#include "file_operations.h"
#include <vector>
#include "npp_constants.h"
#include "document_data.h"
#include "app_state.h"
#include "string_utils.h"
#include "language_defs.h"
#include "lexer_styles.h"
#include "document_manager.h"
#include "function_list_panel.h"
#include "recent_files.h"
#include "status_bar.h"
#include "scintilla_bridge.h"
#include "file_monitor_mac.h"
#include "uchardet.h"
#include "plugin_manager.h"
#include "Notepad_plus_msgs.h"
#include "windows.h"
#include "commdlg.h"
#include "commctrl.h"

int detectEncoding(NSData* data)
{
	const unsigned char* bytes = (const unsigned char*)[data bytes];
	NSUInteger len = [data length];
	if (len >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF)
		return ENC_UTF8_BOM;
	if (len >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE)
		return ENC_UTF16_LE;
	if (len >= 2 && bytes[0] == 0xFE && bytes[1] == 0xFF)
		return ENC_UTF16_BE;

	uchardet_t ud = uchardet_new();
	uchardet_handle_data(ud, (const char*)bytes, (size_t)len);
	uchardet_data_end(ud);
	const char* charset = uchardet_get_charset(ud);
	int result = ENC_UTF8;
	if (charset && strlen(charset) > 0)
	{
		NSString* cs = [[NSString stringWithUTF8String:charset] uppercaseString];
		if ([cs containsString:@"UTF-16LE"]) result = ENC_UTF16_LE;
		else if ([cs containsString:@"UTF-16BE"]) result = ENC_UTF16_BE;
		else if ([cs containsString:@"ASCII"] || [cs containsString:@"ISO-8859"] ||
		         [cs containsString:@"WINDOWS-1252"]) result = ENC_ANSI;
	}
	uchardet_delete(ud);
	return result;
}

int detectEOLMode(const char* text, size_t len)
{
	for (size_t i = 0; i < len; ++i)
	{
		if (text[i] == '\r')
		{
			if (i + 1 < len && text[i + 1] == '\n')
				return SC_EOL_CRLF;
			return SC_EOL_CR;
		}
		if (text[i] == '\n')
			return SC_EOL_LF;
	}
	return SC_EOL_LF;
}

std::string decodeFileData(NSData* data, int encoding)
{
	const unsigned char* bytes = (const unsigned char*)[data bytes];
	NSUInteger len = [data length];
	NSString* str = nil;

	switch (encoding)
	{
		case ENC_UTF8_BOM:
			if (len >= 3)
				str = [[NSString alloc] initWithBytes:bytes + 3 length:len - 3 encoding:NSUTF8StringEncoding];
			break;
		case ENC_UTF16_LE:
			str = [[NSString alloc] initWithBytes:bytes length:len encoding:NSUTF16LittleEndianStringEncoding];
			break;
		case ENC_UTF16_BE:
			str = [[NSString alloc] initWithBytes:bytes length:len encoding:NSUTF16BigEndianStringEncoding];
			break;
		case ENC_ANSI:
			str = [[NSString alloc] initWithBytes:bytes length:len encoding:NSWindowsCP1252StringEncoding];
			break;
		default:
			str = [[NSString alloc] initWithBytes:bytes length:len encoding:NSUTF8StringEncoding];
			break;
	}
	if (!str)
		str = [[NSString alloc] initWithBytes:bytes length:len encoding:NSUTF8StringEncoding];
	if (!str)
		str = [[NSString alloc] initWithBytes:bytes length:len encoding:NSISOLatin1StringEncoding];
	return str ? std::string([str UTF8String]) : std::string();
}

NSData* encodeForSave(const char* utf8Text, size_t len, int encoding)
{
	NSString* str = [[NSString alloc] initWithBytes:utf8Text length:len encoding:NSUTF8StringEncoding];
	if (!str) return nil;

	switch (encoding)
	{
		case ENC_UTF8_BOM:
		{
			NSMutableData* d = [NSMutableData data];
			unsigned char bom[] = {0xEF, 0xBB, 0xBF};
			[d appendBytes:bom length:3];
			NSData* body = [str dataUsingEncoding:NSUTF8StringEncoding];
			if (body) [d appendData:body];
			return d;
		}
		case ENC_UTF16_LE:
			return [str dataUsingEncoding:NSUTF16LittleEndianStringEncoding];
		case ENC_UTF16_BE:
			return [str dataUsingEncoding:NSUTF16BigEndianStringEncoding];
		case ENC_ANSI:
			return [str dataUsingEncoding:NSWindowsCP1252StringEncoding];
		default:
			return [str dataUsingEncoding:NSUTF8StringEncoding];
	}
}

bool switchToFileIfOpen(const std::wstring& filePath)
{
	if (filePath.empty()) return false;

	// Canonicalize the path for comparison
	NSString* nsPath = WideToNSString(filePath.c_str());
	NSString* canonical = [[nsPath stringByStandardizingPath] stringByResolvingSymlinksInPath];
	std::wstring canonicalPath = NSStringToWide(canonical);

	// Check main view
	for (int i = 0; i < static_cast<int>(ctx().documents.size()); ++i)
	{
		if (ctx().documents[i].filePath.empty()) continue;
		NSString* docPath = WideToNSString(ctx().documents[i].filePath.c_str());
		NSString* docCanonical = [[docPath stringByStandardizingPath] stringByResolvingSymlinksInPath];
		if ([canonical isEqualToString:docCanonical])
		{
			if (ctx().activeView != 0) ctx().activeView = 0;
			switchToTabInView(0, i);
			return true;
		}
	}
	// Check split view
	if (ctx().isSplit)
	{
		for (int i = 0; i < static_cast<int>(ctx().documents2.size()); ++i)
		{
			if (ctx().documents2[i].filePath.empty()) continue;
			NSString* docPath = WideToNSString(ctx().documents2[i].filePath.c_str());
			NSString* docCanonical = [[docPath stringByStandardizingPath] stringByResolvingSymlinksInPath];
			if ([canonical isEqualToString:docCanonical])
			{
				if (ctx().activeView != 1) ctx().activeView = 1;
				switchToTabInView(1, i);
				return true;
			}
		}
	}
	return false;
}

bool openFileAtPath(NSString* path)
{
	// Check if already open; switch to it instead of duplicating
	std::wstring wcheck = NSStringToWide(path);
	if (switchToFileIfOpen(wcheck))
		return true;

	NSData* rawData = [NSData dataWithContentsOfFile:path];
	if (!rawData) return false;

	int enc = detectEncoding(rawData);
	std::string utf8Content = decodeFileData(rawData, enc);
	int eol = detectEOLMode(utf8Content.c_str(), utf8Content.size());

	std::wstring wpath = NSStringToWide(path);
	std::wstring title = wpath;
	size_t lastSlash = title.rfind(L'/');
	if (lastSlash != std::wstring::npos)
		title = title.substr(lastSlash + 1);

	int langIdx = guessLanguage(wpath);
	int tabIdx = addNewTab(title, utf8Content, wpath, langIdx);

	auto& docs = ctx().activeDocuments();
	if (tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()))
	{
		docs[tabIdx].encoding = enc;
		docs[tabIdx].eolMode = eol;
		void* sci = ctx().activeScintillaView();
		if (sci)
			ScintillaBridge_sendMessage(sci, SCI_SETEOLMODE, eol, 0);
	}

	if (ctx().fileMonitor)
	{
		std::wstring dir = wpath.substr(0, wpath.rfind(L'/'));
		ctx().fileMonitor->addDirectory(dir);
	}

	addRecentFile(wpath);
	rebuildRecentMenu();
	updateStatusBar();

	// Notify plugins that a file was opened
	{
		auto& activeDocs = ctx().activeDocuments();
		int activeIdx = ctx().activeTabIndex();
		uint64_t bufId = (activeIdx >= 0 && activeIdx < static_cast<int>(activeDocs.size()))
			? activeDocs[activeIdx].bufferId : 0;
		SCNotification notif{};
		notif.nmhdr.hwndFrom = ctx().mainHwnd;
		notif.nmhdr.code = NPPN_FILEOPENED;
		notif.nmhdr.idFrom = static_cast<uintptr_t>(bufId);
		pluginManager().notify(&notif);
	}

	return true;
}

void openFile()
{
	OPENFILENAMEW ofn = {};
	constexpr DWORD bufSize = 32768;
	std::vector<wchar_t> fileBuf(bufSize, 0);
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = ctx().mainHwnd;
	ofn.lpstrFile = fileBuf.data();
	ofn.nMaxFile = bufSize;
	ofn.lpstrTitle = L"Open File";
	ofn.lpstrFilter = L"All Files\0*.*\0"
	                   L"C/C++ Files\0*.c;*.cpp;*.cc;*.h;*.hpp\0"
	                   L"Text Files\0*.txt\0";
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_ALLOWMULTISELECT;

	if (!GetOpenFileNameW(&ofn))
		return;

	// Parse null-delimited paths (each is a full path, double-null terminated)
	const wchar_t* p = fileBuf.data();
	while (*p)
	{
		NSString* path = WideToNSString(p);
		if (path && path.length > 0)
			openFileAtPath(path);
		p += wcslen(p) + 1;
	}
}

void saveCurrentFile()
{
	auto& docs = ctx().activeDocuments();
	int tabIdx = ctx().activeTabIndex();
	if (tabIdx < 0 || tabIdx >= static_cast<int>(docs.size()))
		return;

	auto& doc = docs[tabIdx];
	void* sci = ctx().activeScintillaView();
	HWND tabHwnd = ctx().activeTabHwnd();

	if (doc.filePath.empty())
	{
		OPENFILENAMEW ofn = {};
		wchar_t filePath[MAX_PATH] = {0};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = ctx().mainHwnd;
		ofn.lpstrFile = filePath;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrTitle = L"Save File As";
		ofn.lpstrFilter = L"All Files\0*.*\0";
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_OVERWRITEPROMPT;

		if (!GetSaveFileNameW(&ofn))
			return;

		doc.filePath = filePath;
		doc.title = doc.filePath;
		size_t lastSlash = doc.title.rfind(L'/');
		if (lastSlash != std::wstring::npos)
			doc.title = doc.title.substr(lastSlash + 1);

		if (tabHwnd)
		{
			TCITEMW tcItem = {};
			tcItem.mask = TCIF_TEXT;
			wchar_t titleBuf[256];
			wcsncpy(titleBuf, doc.title.c_str(), 255);
			titleBuf[255] = L'\0';
			tcItem.pszText = titleBuf;
			SendMessageW(tabHwnd, TCM_SETITEMW, tabIdx, reinterpret_cast<LPARAM>(&tcItem));
		}

		int oldLanguageIndex = doc.languageIndex;
		doc.languageIndex = guessLanguage(doc.filePath);
		if (sci)
			applyLanguageToView(sci, doc.languageIndex);
		if (doc.languageIndex != oldLanguageIndex)
		{
			++doc.functionListRevision;
			scheduleFunctionListRefresh();
		}
	}

	if (!sci) return;

	intptr_t len = ScintillaBridge_sendMessage(sci, SCI_GETTEXTLENGTH, 0, 0);
	if (len >= 0)
	{
		char* buf = new char[len + 1];
		ScintillaBridge_sendMessage(sci, SCI_GETTEXT, len + 1, (intptr_t)buf);

		NSString* path = WideToNSString(doc.filePath.c_str());
		NSData* encoded = encodeForSave(buf, len, doc.encoding);
		BOOL ok = NO;
		if (encoded)
			ok = [encoded writeToFile:path atomically:YES];

		if (!ok)
			NSLog(@"Error saving file");
		else
		{
			ScintillaBridge_sendMessage(sci, SCI_SETSAVEPOINT, 0, 0);
			doc.modified = false;
			doc.savePointValid = true;
			updateTabModifiedIndicator(ctx().activeView, tabIdx);
			updateWindowDocumentEdited();
			NSString* nsTitle = WideToNSString(doc.title.c_str());
			[ctx().mainWindow setTitle:[NSString stringWithFormat:@"Notepad++ — %@", nsTitle]];

			addRecentFile(doc.filePath);
			rebuildRecentMenu();

			// Notify plugins that the file was saved
			{
				SCNotification notif{};
				notif.nmhdr.hwndFrom = ctx().mainHwnd;
				notif.nmhdr.code = NPPN_FILESAVED;
				notif.nmhdr.idFrom = static_cast<uintptr_t>(doc.bufferId);
				pluginManager().notify(&notif);
			}
		}

		delete[] buf;
	}
}
