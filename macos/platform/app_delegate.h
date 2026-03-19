// app_delegate.h — Application delegate and lifecycle
// Part of the Notepad++ macOS port modular refactor.

#pragma once

#import <Cocoa/Cocoa.h>

@interface NppDropTargetView : NSView
@end

@interface NppAppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate, NSSplitViewDelegate>
{
	BOOL _finishedLaunching;
	NSMutableArray<NSString*>* _pendingFiles;
}
- (void)performContextAction:(NSMenuItem*)sender;
@end
