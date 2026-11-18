/*
 * InfoBarCommunicator.h - Definitions of a communication protocol and other data types used for
 *                         the info bar implementation.
 *
 * Mike Lischke <mlischke@sun.com>
 *
 * Copyright 2009 Sun Microsystems, Inc. All rights reserved.
 * This file is dual licensed under LGPL v2.1 and the Scintilla license (http://www.scintilla.org/License.txt).
 */

typedef NS_OPTIONS(NSUInteger, IBDisplay) {
	IBShowZoom          = 0x01,
	IBShowCaretPosition = 0x02,
	IBShowStatusText    = 0x04,
	IBShowAll           = 0xFF
};

/**
 * The info bar communicator protocol is used for communication between ScintillaView and its
 * information bar component. Using this protocol decouples any potential info target from the main
 * ScintillaView implementation. The protocol is used two-way.
 */

typedef NS_ENUM(NSInteger, NotificationType) {
	IBNZoomChanged,    // The user selected another zoom value.
	IBNCaretChanged,   // The caret in the editor changed.
	IBNStatusChanged,  // The application set a new status message.
};

@protocol InfoBarCommunicator
- (void) notify: (NotificationType) type message: (NSString *) message location: (NSPoint) location
	  value: (float) value;
- (void) setCallback: (id <InfoBarCommunicator>) callback;
@end

