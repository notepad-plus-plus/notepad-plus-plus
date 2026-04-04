// about_dialog.mm — About dialog
// Part of the Notepad++ macOS port modular refactor.

#import <Cocoa/Cocoa.h>
#include "about_dialog.h"

@interface AboutDialogController : NSObject <NSWindowDelegate>
- (void)openRepository:(id)sender;
@end

@implementation AboutDialogController
- (void)openRepository:(id)sender
{
	[[NSWorkspace sharedWorkspace] openURL:
		[NSURL URLWithString:@"https://github.com/hybridmachine/MacNotePlusPlus"]];
}

- (void)windowWillClose:(NSNotification*)notification
{
	[NSApp stopModal];
}
@end

void showAboutDlg()
{
	@autoreleasepool {
		NSPanel* panel = [[NSPanel alloc]
			initWithContentRect:NSMakeRect(0, 0, 400, 360)
			styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
			backing:NSBackingStoreBuffered
			defer:NO];
		[panel setTitle:@"About MacNote++"];
		[panel center];
		[panel setReleasedWhenClosed:NO];

		// Keep the controller alive during modal
		AboutDialogController* controller = [[AboutDialogController alloc] init];

		NSView* contentView = [panel contentView];
		CGFloat y = 310;

		// App icon
		NSImage* logo = [NSApp applicationIconImage];
		if (!logo)
		{
			NSString* dir = [[[NSBundle mainBundle] executablePath] stringByDeletingLastPathComponent];
			logo = [[NSImage alloc] initWithContentsOfFile:[dir stringByAppendingPathComponent:@"logo.png"]];
		}
		if (logo)
		{
			NSImageView* iconView = [[NSImageView alloc] initWithFrame:NSMakeRect(168, y - 64, 64, 64)];
			[iconView setImage:logo];
			[iconView setImageScaling:NSImageScaleProportionallyUpOrDown];
			[contentView addSubview:iconView];
		}
		y -= 80;

		// App name
		NSTextField* nameLabel = [NSTextField labelWithString:@"MacNote++"];
		[nameLabel setFont:[NSFont boldSystemFontOfSize:18]];
		[nameLabel setAlignment:NSTextAlignmentCenter];
		[nameLabel setFrame:NSMakeRect(20, y, 360, 24)];
		[contentView addSubview:nameLabel];
		y -= 22;

		// Version — read from bundle at runtime
		NSString* version = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
		if (!version) version = @"1.0.0";
		NSString* build = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"];
		NSString* versionStr = build && ![build isEqualToString:version]
			? [NSString stringWithFormat:@"Version %@ (%@)", version, build]
			: [NSString stringWithFormat:@"Version %@", version];
		NSTextField* versionLabel = [NSTextField labelWithString:versionStr];
		[versionLabel setFont:[NSFont systemFontOfSize:13]];
		[versionLabel setTextColor:[NSColor secondaryLabelColor]];
		[versionLabel setAlignment:NSTextAlignmentCenter];
		[versionLabel setFrame:NSMakeRect(20, y, 360, 18)];
		[contentView addSubview:versionLabel];
		y -= 28;

		// Attribution
		NSTextField* creditLabel = [NSTextField labelWithString:@"Based on Notepad++ by Don Ho"];
		[creditLabel setFont:[NSFont systemFontOfSize:13]];
		[creditLabel setAlignment:NSTextAlignmentCenter];
		[creditLabel setFrame:NSMakeRect(20, y, 360, 18)];
		[contentView addSubview:creditLabel];
		y -= 30;

		// Separator
		NSBox* separator = [[NSBox alloc] initWithFrame:NSMakeRect(40, y, 320, 1)];
		[separator setBoxType:NSBoxSeparator];
		[contentView addSubview:separator];
		y -= 24;

		// Component versions
		NSTextField* compLabel = [NSTextField labelWithString:
			@"Scintilla 5.5.3  \u2022  Lexilla 5.4.0  \u2022  Boost.Regex 1.90.0"];
		[compLabel setFont:[NSFont systemFontOfSize:11]];
		[compLabel setTextColor:[NSColor tertiaryLabelColor]];
		[compLabel setAlignment:NSTextAlignmentCenter];
		[compLabel setFrame:NSMakeRect(20, y, 360, 16)];
		[contentView addSubview:compLabel];
		y -= 22;

		// Build date
		NSString* buildDate = [NSString stringWithFormat:@"Built: %s", __DATE__];
		NSTextField* buildLabel = [NSTextField labelWithString:buildDate];
		[buildLabel setFont:[NSFont systemFontOfSize:11]];
		[buildLabel setTextColor:[NSColor tertiaryLabelColor]];
		[buildLabel setAlignment:NSTextAlignmentCenter];
		[buildLabel setFrame:NSMakeRect(20, y, 360, 16)];
		[contentView addSubview:buildLabel];
		y -= 28;

		// Repository link (clickable button styled as link)
		NSButton* linkButton = [[NSButton alloc] initWithFrame:NSMakeRect(60, y, 280, 20)];
		[linkButton setTitle:@"github.com/hybridmachine/MacNotePlusPlus"];
		[linkButton setBezelStyle:NSBezelStyleInline];
		[linkButton setBordered:NO];
		[linkButton setTarget:controller];
		[linkButton setAction:@selector(openRepository:)];
		NSMutableAttributedString* linkAttr = [[NSMutableAttributedString alloc]
			initWithString:@"github.com/hybridmachine/MacNotePlusPlus"];
		[linkAttr addAttribute:NSForegroundColorAttributeName
		                 value:[NSColor linkColor]
		                 range:NSMakeRange(0, linkAttr.length)];
		[linkAttr addAttribute:NSUnderlineStyleAttributeName
		                 value:@(NSUnderlineStyleSingle)
		                 range:NSMakeRange(0, linkAttr.length)];
		[linkAttr addAttribute:NSFontAttributeName
		                 value:[NSFont systemFontOfSize:11]
		                 range:NSMakeRange(0, linkAttr.length)];
		[linkButton setAttributedTitle:linkAttr];
		[linkButton setAlignment:NSTextAlignmentCenter];
		[contentView addSubview:linkButton];
		y -= 24;

		// License
		NSTextField* licenseLabel = [NSTextField labelWithString:@"GNU General Public License v3"];
		[licenseLabel setFont:[NSFont systemFontOfSize:11]];
		[licenseLabel setTextColor:[NSColor secondaryLabelColor]];
		[licenseLabel setAlignment:NSTextAlignmentCenter];
		[licenseLabel setFrame:NSMakeRect(20, y, 360, 16)];
		[contentView addSubview:licenseLabel];

		// OK button
		NSButton* okButton = [[NSButton alloc] initWithFrame:NSMakeRect(160, 12, 80, 32)];
		[okButton setTitle:@"OK"];
		[okButton setBezelStyle:NSBezelStyleRounded];
		[okButton setTarget:NSApp];
		[okButton setAction:@selector(stopModal)];
		[okButton setKeyEquivalent:@"\r"];
		[contentView addSubview:okButton];

		// Escape triggers the panel's close, which calls windowWillClose: → stopModal
		[panel setDefaultButtonCell:[okButton cell]];
		[panel setDelegate:controller];

		[NSApp runModalForWindow:panel];
		[panel close];

		// prevent ARC from releasing controller during modal
		(void)controller;
	}
}
