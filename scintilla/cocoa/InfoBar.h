
/**
 * Scintilla source code edit control
 * InfoBar.h - Implements special info bar with zoom info, caret position etc. to be used with
 *             ScintillaView.
 *
 * Mike Lischke <mlischke@sun.com>
 *
 * Copyright 2009 Sun Microsystems, Inc. All rights reserved.
 * This file is dual licensed under LGPL v2.1 and the Scintilla license (http://www.scintilla.org/License.txt).
 */

#import <Cocoa/Cocoa.h>
#import "InfoBarCommunicator.h"

/**
 * Extended text cell for vertically aligned text.
 */
@interface VerticallyCenteredTextFieldCell : NSTextFieldCell
{
	BOOL mIsEditingOrSelecting;
}

@end

@interface InfoBar : NSView <InfoBarCommunicator>
{
@private
  NSImage* mBackground;
  IBDisplay mDisplayMask;
  
  float mScaleFactor;
  NSPopUpButton* mZoomPopup;
  
  int mCurrentCaretX;
  int mCurrentCaretY;
  NSTextField* mCaretPositionLabel;
  NSTextField* mStatusTextLabel;
  
  id <InfoBarCommunicator> mCallback;
}

- (void) notify: (NotificationType) type message: (NSString*) message location: (NSPoint) location
          value: (float) value;
- (void) setCallback: (id <InfoBarCommunicator>) callback;

- (void) createItems;
- (void) positionSubViews;
- (void) setDisplay: (IBDisplay) display;
- (void) zoomItemAction: (id) sender;
- (void) setScaleFactor: (float) newScaleFactor adjustPopup: (BOOL) flag;
- (void) setCaretPosition: (NSPoint) position;
- (void) sizeToFit;

@end
