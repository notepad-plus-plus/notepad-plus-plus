// hash_tools.mm — Hash tools (MD5, SHA-1, SHA-256, SHA-512)
// Part of the Notepad++ macOS port.
//
// Hashes the current selection if non-empty, otherwise the entire document.
// Shows result in a modal NSPanel with Copy / Insert at Cursor / Close buttons.

#import <Cocoa/Cocoa.h>
#include <CommonCrypto/CommonDigest.h>
#include <cstdio>
#include <string>
#include <vector>

#include "hash_tools.h"
#include "app_state.h"
#include "npp_constants.h"
#include "scintilla_bridge.h"

// ------------------------------------------------------------------
// Helper: compute a hex-encoded hash digest
// ------------------------------------------------------------------

// Use incremental CommonCrypto APIs (Init/Update/Final) to avoid CC_LONG
// truncation on inputs larger than UINT32_MAX. Data is fed in chunks.
static std::string computeHash(const void* data, size_t length, const std::string& algorithm)
{
	unsigned char digest[CC_SHA512_DIGEST_LENGTH];
	unsigned int digestLen = 0;
	constexpr size_t kChunkSize = 1024 * 1024; // 1 MB
	const auto* bytes = static_cast<const unsigned char*>(data);

	if (algorithm == "MD5")
	{
		CC_MD5_CTX ctx;
		CC_MD5_Init(&ctx);
		for (size_t offset = 0; offset < length; offset += kChunkSize)
		{
			size_t chunk = (length - offset < kChunkSize) ? (length - offset) : kChunkSize;
			CC_MD5_Update(&ctx, bytes + offset, static_cast<CC_LONG>(chunk));
		}
		CC_MD5_Final(digest, &ctx);
		digestLen = CC_MD5_DIGEST_LENGTH;
	}
	else if (algorithm == "SHA1")
	{
		CC_SHA1_CTX ctx;
		CC_SHA1_Init(&ctx);
		for (size_t offset = 0; offset < length; offset += kChunkSize)
		{
			size_t chunk = (length - offset < kChunkSize) ? (length - offset) : kChunkSize;
			CC_SHA1_Update(&ctx, bytes + offset, static_cast<CC_LONG>(chunk));
		}
		CC_SHA1_Final(digest, &ctx);
		digestLen = CC_SHA1_DIGEST_LENGTH;
	}
	else if (algorithm == "SHA256")
	{
		CC_SHA256_CTX ctx;
		CC_SHA256_Init(&ctx);
		for (size_t offset = 0; offset < length; offset += kChunkSize)
		{
			size_t chunk = (length - offset < kChunkSize) ? (length - offset) : kChunkSize;
			CC_SHA256_Update(&ctx, bytes + offset, static_cast<CC_LONG>(chunk));
		}
		CC_SHA256_Final(digest, &ctx);
		digestLen = CC_SHA256_DIGEST_LENGTH;
	}
	else if (algorithm == "SHA512")
	{
		CC_SHA512_CTX ctx;
		CC_SHA512_Init(&ctx);
		for (size_t offset = 0; offset < length; offset += kChunkSize)
		{
			size_t chunk = (length - offset < kChunkSize) ? (length - offset) : kChunkSize;
			CC_SHA512_Update(&ctx, bytes + offset, static_cast<CC_LONG>(chunk));
		}
		CC_SHA512_Final(digest, &ctx);
		digestLen = CC_SHA512_DIGEST_LENGTH;
	}
	else
	{
		return "(unsupported algorithm)";
	}

	// Convert to lowercase hex string
	static const char hexDigits[] = "0123456789abcdef";
	std::string hex;
	hex.reserve(digestLen * 2);
	for (unsigned int i = 0; i < digestLen; ++i)
	{
		hex += hexDigits[digest[i] >> 4];
		hex += hexDigits[digest[i] & 0x0F];
	}
	return hex;
}

// ------------------------------------------------------------------
// Helper: get input bytes (selection or whole document)
// ------------------------------------------------------------------

static std::vector<char> getInputBytes()
{
	void* sci = ctx().activeScintillaView();
	if (!sci)
		return {};

	intptr_t selStart = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONSTART, 0, 0);
	intptr_t selEnd = ScintillaBridge_sendMessage(sci, SCI_GETSELECTIONEND, 0, 0);

	if (selEnd > selStart)
	{
		// Hash the selection
		intptr_t selLen = selEnd - selStart;
		std::vector<char> buf(selLen + 1, '\0');
		ScintillaBridge_sendMessage(sci, SCI_GETSELTEXT, 0, reinterpret_cast<intptr_t>(buf.data()));
		buf.resize(selLen); // trim null terminator
		return buf;
	}

	// Hash the entire document
	intptr_t docLen = ScintillaBridge_sendMessage(sci, SCI_GETTEXTLENGTH, 0, 0);
	if (docLen <= 0)
		return {};

	std::vector<char> buf(docLen + 1, '\0');
	ScintillaBridge_sendMessage(sci, SCI_GETTEXT, docLen + 1, reinterpret_cast<intptr_t>(buf.data()));
	buf.resize(docLen); // trim null terminator
	return buf;
}

// ------------------------------------------------------------------
// Helper: show the hash result dialog
// ------------------------------------------------------------------

@interface HashDialogController : NSObject <NSWindowDelegate>
@property (nonatomic, copy) NSString* hashValue;
@end

@implementation HashDialogController

- (void)copyHash:(id)sender
{
	NSPasteboard* pb = [NSPasteboard generalPasteboard];
	[pb clearContents];
	[pb setString:_hashValue forType:NSPasteboardTypeString];
}

- (void)insertAtCursor:(id)sender
{
	void* sci = ctx().activeScintillaView();
	if (sci && _hashValue)
	{
		const char* utf8 = [_hashValue UTF8String];
		ScintillaBridge_sendMessage(sci, SCI_REPLACESEL, 0, reinterpret_cast<intptr_t>(utf8));
	}
	[NSApp stopModal];
}

- (void)closePanel:(id)sender
{
	[NSApp stopModal];
}

- (void)windowWillClose:(NSNotification*)notification
{
	[NSApp stopModal];
}

@end

static void showHashResult(const std::string& algorithm, const std::string& hex)
{
	@autoreleasepool {
		NSString* title = [NSString stringWithFormat:@"%s Hash", algorithm.c_str()];
		NSString* hashStr = [NSString stringWithUTF8String:hex.c_str()];

		NSPanel* panel = [[NSPanel alloc]
			initWithContentRect:NSMakeRect(0, 0, 480, 160)
			styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
			backing:NSBackingStoreBuffered
			defer:NO];
		[panel setTitle:title];
		[panel center];
		[panel setReleasedWhenClosed:NO];

		HashDialogController* controller = [[HashDialogController alloc] init];
		controller.hashValue = hashStr;

		NSView* contentView = [panel contentView];

		// Algorithm label
		NSTextField* algoLabel = [NSTextField labelWithString:title];
		[algoLabel setFont:[NSFont boldSystemFontOfSize:14]];
		[algoLabel setFrame:NSMakeRect(20, 120, 440, 20)];
		[contentView addSubview:algoLabel];

		// Hash value field (selectable, not editable)
		NSTextField* hashField = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 70, 440, 40)];
		[hashField setStringValue:hashStr];
		[hashField setEditable:NO];
		[hashField setSelectable:YES];
		[hashField setBezeled:YES];
		[hashField setBezelStyle:NSTextFieldSquareBezel];
		[hashField setFont:[NSFont monospacedSystemFontOfSize:12 weight:NSFontWeightRegular]];
		[hashField setLineBreakMode:NSLineBreakByCharWrapping];
		[hashField setUsesSingleLineMode:NO];
		[[hashField cell] setWraps:YES];
		[contentView addSubview:hashField];

		// Buttons
		CGFloat btnY = 16;
		CGFloat btnW = 130;
		CGFloat spacing = 14;
		CGFloat totalW = btnW * 3 + spacing * 2;
		CGFloat startX = (480 - totalW) / 2;

		NSButton* copyBtn = [[NSButton alloc] initWithFrame:NSMakeRect(startX, btnY, btnW, 32)];
		[copyBtn setTitle:@"Copy"];
		[copyBtn setBezelStyle:NSBezelStyleRounded];
		[copyBtn setTarget:controller];
		[copyBtn setAction:@selector(copyHash:)];
		[contentView addSubview:copyBtn];

		NSButton* insertBtn = [[NSButton alloc] initWithFrame:NSMakeRect(startX + btnW + spacing, btnY, btnW, 32)];
		[insertBtn setTitle:@"Insert at Cursor"];
		[insertBtn setBezelStyle:NSBezelStyleRounded];
		[insertBtn setTarget:controller];
		[insertBtn setAction:@selector(insertAtCursor:)];
		[contentView addSubview:insertBtn];

		NSButton* closeBtn = [[NSButton alloc] initWithFrame:NSMakeRect(startX + (btnW + spacing) * 2, btnY, btnW, 32)];
		[closeBtn setTitle:@"Close"];
		[closeBtn setBezelStyle:NSBezelStyleRounded];
		[closeBtn setTarget:controller];
		[closeBtn setAction:@selector(closePanel:)];
		[closeBtn setKeyEquivalent:@"\r"];
		[contentView addSubview:closeBtn];

		[panel setDefaultButtonCell:[closeBtn cell]];
		[panel setDelegate:controller];

		[NSApp runModalForWindow:panel];
		[panel close];

		// Prevent ARC from releasing controller during modal
		(void)controller;
	}
}

// ------------------------------------------------------------------
// Shared pipeline: get input -> compute hash -> show result
// ------------------------------------------------------------------

static void doHash(const std::string& algorithm)
{
	std::vector<char> input = getInputBytes();
	if (input.empty())
	{
		NSAlert* alert = [[NSAlert alloc] init];
		[alert setMessageText:@"Nothing to Hash"];
		[alert setInformativeText:@"The document is empty and there is no selection."];
		[alert addButtonWithTitle:@"OK"];
		[alert runModal];
		return;
	}

	std::string hex = computeHash(input.data(), input.size(), algorithm);
	showHashResult(algorithm, hex);
}

// ------------------------------------------------------------------
// Public API
// ------------------------------------------------------------------

void doHashMD5()
{
	doHash("MD5");
}

void doHashSHA1()
{
	doHash("SHA1");
}

void doHashSHA256()
{
	doHash("SHA256");
}

void doHashSHA512()
{
	doHash("SHA512");
}
