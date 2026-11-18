
/**
 * Declaration of the native Cocoa View that serves as container for the scintilla parts.
 * @file ScintillaView.h
 *
 * Created by Mike Lischke.
 *
 * Copyright 2011, 2013, Oracle and/or its affiliates. All rights reserved.
 * Copyright 2009, 2011 Sun Microsystems, Inc. All rights reserved.
 * This file is dual licensed under LGPL v2.1 and the Scintilla license (http://www.scintilla.org/License.txt).
 */

#import <Cocoa/Cocoa.h>

#import "Scintilla.h"

#import "InfoBarCommunicator.h"

/**
 * Scintilla sends these two messages to the notify handler. Please refer
 * to the Windows API doc for details about the message format.
 */
#define WM_COMMAND 1001
#define WM_NOTIFY 1002

/**
 * On the Mac, there is no WM_COMMAND or WM_NOTIFY message that can be sent
 * back to the parent. Therefore, there must be a callback handler that acts
 * like a Windows WndProc, where Scintilla can send notifications to. Use
 * ScintillaView registerNotifyCallback() to register such a handler.
 * Message format is:
 * <br>
 * WM_COMMAND: HIWORD (wParam) = notification code, LOWORD (wParam) = control ID, lParam = ScintillaCocoa*
 * <br>
 * WM_NOTIFY: wParam = control ID, lParam = ptr to SCNotification structure, with hwndFrom set to ScintillaCocoa*
 */
typedef void(*SciNotifyFunc)(intptr_t windowid, unsigned int iMessage, uintptr_t wParam, uintptr_t lParam);

extern NSString *const SCIUpdateUINotification;

@protocol ScintillaNotificationProtocol
- (void) notification: (SCNotification *) notification;
@end

/**
 * SCIScrollView provides pre-macOS 10.14 tiling behavior.
 */
@interface SCIScrollView : NSScrollView;

@end

/**
 * SCIMarginView draws line numbers and other margins next to the text view.
 */
@interface SCIMarginView : NSRulerView;

@end

/**
 * SCIContentView is the Cocoa interface to the Scintilla backend. It handles text input and
 * provides a canvas for painting the output.
 */
@interface SCIContentView : NSView <
	NSTextInputClient,
	NSUserInterfaceValidations,
	NSDraggingSource,
	NSDraggingDestination,
	NSAccessibilityStaticText>;

- (void) setCursor: (int) cursor; // Needed by ScintillaCocoa

@end

/**
 * ScintillaView is the class instantiated by client code.
 * It contains an NSScrollView which contains a SCIMarginView and a SCIContentView.
 * It is responsible for providing an API and communicating to a delegate.
 */
@interface ScintillaView : NSView <InfoBarCommunicator, ScintillaNotificationProtocol>;

@property(nonatomic, unsafe_unretained) id<ScintillaNotificationProtocol> delegate;
@property(nonatomic, readonly) NSScrollView *scrollView;

+ (Class) contentViewClass;

- (void) notify: (NotificationType) type message: (NSString *) message location: (NSPoint) location
	  value: (float) value;
- (void) setCallback: (id <InfoBarCommunicator>) callback;

- (void) suspendDrawing: (BOOL) suspend;
- (void) notification: (SCNotification *) notification;

- (void) updateIndicatorIME;

// Scroller handling
- (void) setMarginWidth: (int) width;
- (SCIContentView *) content;
- (void) updateMarginCursors;

// NSTextView compatibility layer.
- (NSString *) string;
- (void) setString: (NSString *) aString;
- (void) insertText: (id) aString;
- (void) setEditable: (BOOL) editable;
- (BOOL) isEditable;
- (NSRange) selectedRange;
- (NSRange) selectedRangePositions;

- (NSString *) selectedString;

- (void) deleteRange: (NSRange) range;

- (void) setFontName: (NSString *) font
		size: (int) size
		bold: (BOOL) bold
	      italic: (BOOL) italic;

// Native call through to the backend.
+ (sptr_t) directCall: (ScintillaView *) sender message: (unsigned int) message wParam: (uptr_t) wParam
	       lParam: (sptr_t) lParam;
- (sptr_t) message: (unsigned int) message wParam: (uptr_t) wParam lParam: (sptr_t) lParam;
- (sptr_t) message: (unsigned int) message wParam: (uptr_t) wParam;
- (sptr_t) message: (unsigned int) message;

// Back end properties getters and setters.
- (void) setGeneralProperty: (int) property parameter: (long) parameter value: (long) value;
- (void) setGeneralProperty: (int) property value: (long) value;

- (long) getGeneralProperty: (int) property;
- (long) getGeneralProperty: (int) property parameter: (long) parameter;
- (long) getGeneralProperty: (int) property parameter: (long) parameter extra: (long) extra;
- (long) getGeneralProperty: (int) property ref: (const void *) ref;
- (void) setColorProperty: (int) property parameter: (long) parameter value: (NSColor *) value;
- (void) setColorProperty: (int) property parameter: (long) parameter fromHTML: (NSString *) fromHTML;
- (NSColor *) getColorProperty: (int) property parameter: (long) parameter;
- (void) setReferenceProperty: (int) property parameter: (long) parameter value: (const void *) value;
- (const void *) getReferenceProperty: (int) property parameter: (long) parameter;
- (void) setStringProperty: (int) property parameter: (long) parameter value: (NSString *) value;
- (NSString *) getStringProperty: (int) property parameter: (long) parameter;
- (void) setLexerProperty: (NSString *) name value: (NSString *) value;
- (NSString *) getLexerProperty: (NSString *) name;

// The delegate property should be used instead of registerNotifyCallback which is deprecated.
- (void) registerNotifyCallback: (intptr_t) windowid value: (SciNotifyFunc) callback __attribute__ ( (deprecated)) ;

- (void) setInfoBar: (NSView <InfoBarCommunicator> *) aView top: (BOOL) top;
- (void) setStatusText: (NSString *) text;

- (BOOL) findAndHighlightText: (NSString *) searchText
		    matchCase: (BOOL) matchCase
		    wholeWord: (BOOL) wholeWord
		     scrollTo: (BOOL) scrollTo
			 wrap: (BOOL) wrap;

- (BOOL) findAndHighlightText: (NSString *) searchText
		    matchCase: (BOOL) matchCase
		    wholeWord: (BOOL) wholeWord
		     scrollTo: (BOOL) scrollTo
			 wrap: (BOOL) wrap
		    backwards: (BOOL) backwards;

- (int) findAndReplaceText: (NSString *) searchText
		    byText: (NSString *) newText
		 matchCase: (BOOL) matchCase
		 wholeWord: (BOOL) wholeWord
		     doAll: (BOOL) doAll;

@end
