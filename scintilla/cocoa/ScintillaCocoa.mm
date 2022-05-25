
/**
 * Scintilla source code edit control
 * @file ScintillaCocoa.mm - Cocoa subclass of ScintillaBase
 *
 * Written by Mike Lischke <mlischke@sun.com>
 *
 * Loosely based on ScintillaMacOSX.cxx.
 * Copyright 2003 by Evan Jones <ejones@uwaterloo.ca>
 * Based on ScintillaGTK.cxx Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
 * The License.txt file describes the conditions under which this software may be distributed.
  *
 * Copyright (c) 2009, 2010 Sun Microsystems, Inc. All rights reserved.
 * This file is dual licensed under LGPL v2.1 and the Scintilla license (http://www.scintilla.org/License.txt).
 */

#include <cmath>

#include <string_view>
#include <vector>
#include <optional>

#import <Cocoa/Cocoa.h>
#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_5
#import <QuartzCore/CAGradientLayer.h>
#endif
#import <QuartzCore/CAAnimation.h>
#import <QuartzCore/CATransaction.h>

#import "ScintillaTypes.h"
#import "ScintillaMessages.h"
#import "ScintillaStructures.h"

#import "Debugging.h"
#import "Geometry.h"
#import "Platform.h"
#import "ScintillaView.h"
#import "ScintillaCocoa.h"
#import "PlatCocoa.h"

using namespace Scintilla;
using namespace Scintilla::Internal;

NSString *ScintillaRecPboardType = @"com.scintilla.utf16-plain-text.rectangular";

//--------------------------------------------------------------------------------------------------

// Define keyboard shortcuts (equivalents) the Mac way.
#define SCI_CMD ( SCI_CTRL)
#define SCI_SCMD ( SCI_CMD | SCI_SHIFT)
#define SCI_SMETA ( SCI_META | SCI_SHIFT)

namespace {

constexpr Keys Key(char ch) {
    return static_cast<Keys>(ch);
}

}

static const KeyToCommand macMapDefault[] = {
	// macOS specific
	{Keys::Down,      SCI_CTRL,   Message::DocumentEnd},
	{Keys::Down,      SCI_CSHIFT, Message::DocumentEndExtend},
	{Keys::Up,        SCI_CTRL,   Message::DocumentStart},
	{Keys::Up,        SCI_CSHIFT, Message::DocumentStartExtend},
	{Keys::Left,      SCI_CTRL,   Message::VCHome},
	{Keys::Left,      SCI_CSHIFT, Message::VCHomeExtend},
	{Keys::Right,     SCI_CTRL,   Message::LineEnd},
	{Keys::Right,     SCI_CSHIFT, Message::LineEndExtend},

	// Similar to Windows and GTK+
	// Where equivalent clashes with macOS standard, use Meta instead
	{Keys::Down,      SCI_NORM,   Message::LineDown},
	{Keys::Down,      SCI_SHIFT,  Message::LineDownExtend},
	{Keys::Down,      SCI_META,   Message::LineScrollDown},
	{Keys::Down,      SCI_ASHIFT, Message::LineDownRectExtend},
	{Keys::Up,        SCI_NORM,   Message::LineUp},
	{Keys::Up,        SCI_SHIFT,  Message::LineUpExtend},
	{Keys::Up,        SCI_META,   Message::LineScrollUp},
	{Keys::Up,        SCI_ASHIFT, Message::LineUpRectExtend},
	{Key('['),           SCI_CTRL,   Message::ParaUp},
	{Key('['),           SCI_CSHIFT, Message::ParaUpExtend},
	{Key(']'),           SCI_CTRL,   Message::ParaDown},
	{Key(']'),           SCI_CSHIFT, Message::ParaDownExtend},
	{Keys::Left,      SCI_NORM,   Message::CharLeft},
	{Keys::Left,      SCI_SHIFT,  Message::CharLeftExtend},
	{Keys::Left,      SCI_ALT,    Message::WordLeft},
	{Keys::Left,      SCI_META,   Message::WordLeft},
	{Keys::Left,      SCI_SMETA,  Message::WordLeftExtend},
	{Keys::Left,      SCI_ASHIFT, Message::CharLeftRectExtend},
	{Keys::Right,     SCI_NORM,   Message::CharRight},
	{Keys::Right,     SCI_SHIFT,  Message::CharRightExtend},
	{Keys::Right,     SCI_ALT,    Message::WordRight},
	{Keys::Right,     SCI_META,   Message::WordRight},
	{Keys::Right,     SCI_SMETA,  Message::WordRightExtend},
	{Keys::Right,     SCI_ASHIFT, Message::CharRightRectExtend},
	{Key('/'),           SCI_CTRL,   Message::WordPartLeft},
	{Key('/'),           SCI_CSHIFT, Message::WordPartLeftExtend},
	{Key('\\'),          SCI_CTRL,   Message::WordPartRight},
	{Key('\\'),          SCI_CSHIFT, Message::WordPartRightExtend},
	{Keys::Home,      SCI_NORM,   Message::VCHome},
	{Keys::Home,      SCI_SHIFT,  Message::VCHomeExtend},
	{Keys::Home,      SCI_CTRL,   Message::DocumentStart},
	{Keys::Home,      SCI_CSHIFT, Message::DocumentStartExtend},
	{Keys::Home,      SCI_ALT,    Message::HomeDisplay},
	{Keys::Home,      SCI_ASHIFT, Message::VCHomeRectExtend},
	{Keys::End,       SCI_NORM,   Message::LineEnd},
	{Keys::End,       SCI_SHIFT,  Message::LineEndExtend},
	{Keys::End,       SCI_CTRL,   Message::DocumentEnd},
	{Keys::End,       SCI_CSHIFT, Message::DocumentEndExtend},
	{Keys::End,       SCI_ALT,    Message::LineEndDisplay},
	{Keys::End,       SCI_ASHIFT, Message::LineEndRectExtend},
	{Keys::Prior,     SCI_NORM,   Message::PageUp},
	{Keys::Prior,     SCI_SHIFT,  Message::PageUpExtend},
	{Keys::Prior,     SCI_ASHIFT, Message::PageUpRectExtend},
	{Keys::Next,      SCI_NORM,   Message::PageDown},
	{Keys::Next,      SCI_SHIFT,  Message::PageDownExtend},
	{Keys::Next,      SCI_ASHIFT, Message::PageDownRectExtend},
	{Keys::Delete,    SCI_NORM,   Message::Clear},
	{Keys::Delete,    SCI_SHIFT,  Message::Cut},
	{Keys::Delete,    SCI_CTRL,   Message::DelWordRight},
	{Keys::Delete,    SCI_CSHIFT, Message::DelLineRight},
	{Keys::Insert,    SCI_NORM,   Message::EditToggleOvertype},
	{Keys::Insert,    SCI_SHIFT,  Message::Paste},
	{Keys::Insert,    SCI_CTRL,   Message::Copy},
	{Keys::Escape,    SCI_NORM,   Message::Cancel},
	{Keys::Back,      SCI_NORM,   Message::DeleteBack},
	{Keys::Back,      SCI_SHIFT,  Message::DeleteBack},
	{Keys::Back,      SCI_CTRL,   Message::DelWordLeft},
	{Keys::Back,      SCI_ALT,    Message::DelWordLeft},
	{Keys::Back,      SCI_CSHIFT, Message::DelLineLeft},
	{Key('z'),           SCI_CMD,    Message::Undo},
	{Key('z'),           SCI_SCMD,   Message::Redo},
	{Key('x'),           SCI_CMD,    Message::Cut},
	{Key('c'),           SCI_CMD,    Message::Copy},
	{Key('v'),           SCI_CMD,    Message::Paste},
	{Key('a'),           SCI_CMD,    Message::SelectAll},
	{Keys::Tab,       SCI_NORM,   Message::Tab},
	{Keys::Tab,       SCI_SHIFT,  Message::BackTab},
	{Keys::Return,    SCI_NORM,   Message::NewLine},
	{Keys::Return,    SCI_SHIFT,  Message::NewLine},
	{Keys::Add,       SCI_CMD,    Message::ZoomIn},
	{Keys::Subtract,  SCI_CMD,    Message::ZoomOut},
	{Keys::Divide,    SCI_CMD,    Message::SetZoom},
	{Key('l'),           SCI_CMD,    Message::LineCut},
	{Key('l'),           SCI_CSHIFT, Message::LineDelete},
	{Key('t'),           SCI_CSHIFT, Message::LineCopy},
	{Key('t'),           SCI_CTRL,   Message::LineTranspose},
	{Key('d'),           SCI_CTRL,   Message::SelectionDuplicate},
	{Key('u'),           SCI_CTRL,   Message::LowerCase},
	{Key('u'),           SCI_CSHIFT, Message::UpperCase},
	{Key(0), KeyMod::Norm, static_cast<Message>(0)},
};

//--------------------------------------------------------------------------------------------------

#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_5

// Only implement FindHighlightLayer on macOS 10.6+

/**
 * Class to display the animated gold roundrect used on macOS for matches.
 */
@interface FindHighlightLayer : CAGradientLayer {
@private
	NSString *sFind;
	long positionFind;
	BOOL retaining;
	CGFloat widthText;
	CGFloat heightLine;
	NSString *sFont;
	CGFloat fontSize;
}

@property(copy) NSString *sFind;
@property(assign) long positionFind;
@property(assign) BOOL retaining;
@property(assign) CGFloat widthText;
@property(assign) CGFloat heightLine;
@property(copy) NSString *sFont;
@property(assign) CGFloat fontSize;

- (void) animateMatch: (CGPoint) ptText bounce: (BOOL) bounce;
- (void) hideMatch;

@end

//--------------------------------------------------------------------------------------------------

@implementation FindHighlightLayer

@synthesize sFind, positionFind, retaining, widthText, heightLine, sFont, fontSize;

- (id) init {
	if (self = [super init]) {
		[self setNeedsDisplayOnBoundsChange: YES];
		// A gold to slightly redder gradient to match other applications
		CGColorRef colGold = CGColorCreateGenericRGB(1.0, 1.0, 0, 1.0);
		CGColorRef colGoldRed = CGColorCreateGenericRGB(1.0, 0.8, 0, 1.0);
		self.colors = @[(__bridge id)colGoldRed, (__bridge id)colGold];
		CGColorRelease(colGoldRed);
		CGColorRelease(colGold);

		CGColorRef colGreyBorder = CGColorCreateGenericGray(0.756f, 0.5f);
		self.borderColor = colGreyBorder;
		CGColorRelease(colGreyBorder);

		self.borderWidth = 1.0;
		self.cornerRadius = 5.0f;
		self.shadowRadius = 1.0f;
		self.shadowOpacity = 0.9f;
		self.shadowOffset = CGSizeMake(0.0f, -2.0f);
		self.anchorPoint = CGPointMake(0.5, 0.5);
	}
	return self;

}


const CGFloat paddingHighlightX = 4;
const CGFloat paddingHighlightY = 2;

- (void) drawInContext: (CGContextRef) context {
	if (!sFind || !sFont)
		return;

	CFStringRef str = (__bridge CFStringRef)(sFind);

	CFMutableDictionaryRef styleDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 2,
					   &kCFTypeDictionaryKeyCallBacks,
					   &kCFTypeDictionaryValueCallBacks);
	CGColorRef color = CGColorCreateGenericRGB(0.0, 0.0, 0.0, 1.0);
	CFDictionarySetValue(styleDict, kCTForegroundColorAttributeName, color);
	CTFontRef fontRef = ::CTFontCreateWithName((CFStringRef)sFont, fontSize, NULL);
	CFDictionaryAddValue(styleDict, kCTFontAttributeName, fontRef);

	CFAttributedStringRef attrString = ::CFAttributedStringCreate(NULL, str, styleDict);
	CTLineRef textLine = ::CTLineCreateWithAttributedString(attrString);
	// Indent from corner of bounds
	CGContextSetTextPosition(context, paddingHighlightX, 3 + paddingHighlightY);
	CTLineDraw(textLine, context);

	CFRelease(textLine);
	CFRelease(attrString);
	CFRelease(fontRef);
	CGColorRelease(color);
	CFRelease(styleDict);
}

- (void) animateMatch: (CGPoint) ptText bounce: (BOOL) bounce {
	if (!self.sFind || !(self.sFind).length) {
		[self hideMatch];
		return;
	}

	CGFloat width = self.widthText + paddingHighlightX * 2;
	CGFloat height = self.heightLine + paddingHighlightY * 2;

	CGFloat flipper = self.geometryFlipped ? -1.0 : 1.0;

	// Adjust for padding
	ptText.x -= paddingHighlightX;
	ptText.y += flipper * paddingHighlightY;

	// Shift point to centre as expanding about centre
	ptText.x += width / 2.0;
	ptText.y -= flipper * height / 2.0;

	[CATransaction begin];
	[CATransaction setValue: @0.0f forKey: kCATransactionAnimationDuration];
	self.bounds = CGRectMake(0, 0, width, height);
	self.position = ptText;
	if (bounce) {
		// Do not reset visibility when just moving
		self.hidden = NO;
		self.opacity = 1.0;
	}
	[self setNeedsDisplay];
	[CATransaction commit];

	if (bounce) {
		CABasicAnimation *animBounce = [CABasicAnimation animationWithKeyPath: @"transform.scale"];
		animBounce.duration = 0.15;
		animBounce.autoreverses = YES;
		animBounce.removedOnCompletion = NO;
		animBounce.fromValue = @1.0f;
		animBounce.toValue = @1.25f;

		if (self.retaining) {

			[self addAnimation: animBounce forKey: @"animateFound"];

		} else {

			CABasicAnimation *animFade = [CABasicAnimation animationWithKeyPath: @"opacity"];
			animFade.duration = 0.1;
			animFade.beginTime = 0.4;
			animFade.removedOnCompletion = NO;
			animFade.fromValue = @1.0f;
			animFade.toValue = @0.0f;

			CAAnimationGroup *group = [CAAnimationGroup animation];
			group.duration = 0.5;
			group.removedOnCompletion = NO;
			group.fillMode = kCAFillModeForwards;
			group.animations = @[animBounce, animFade];

			[self addAnimation: group forKey: @"animateFound"];
		}
	}
}

- (void) hideMatch {
	self.sFind = @"";
	self.positionFind = Sci::invalidPosition;
	self.hidden = YES;
}

@end

#endif

//--------------------------------------------------------------------------------------------------

@implementation TimerTarget

- (id) init: (void *) target {
	self = [super init];
	if (self != nil) {
		mTarget = target;

		// Get the default notification queue for the thread which created the instance (usually the
		// main thread). We need that later for idle event processing.
		NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
		notificationQueue = [[NSNotificationQueue alloc] initWithNotificationCenter: center];
		[center addObserver: self selector: @selector(idleTriggered:) name: @"Idle" object: self];
	}
	return self;
}

//--------------------------------------------------------------------------------------------------

- (void) dealloc {
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	[center removeObserver: self];
}

//--------------------------------------------------------------------------------------------------

/**
 * Method called by owning ScintillaCocoa object when it is destroyed.
 */
- (void) ownerDestroyed {
	mTarget = NULL;
	notificationQueue = nil;
}

//--------------------------------------------------------------------------------------------------

/**
 * Method called by a timer installed by ScintillaCocoa. This two step approach is needed because
 * a native Obj-C class is required as target for the timer.
 */
- (void) timerFired: (NSTimer *) timer {
	if (mTarget)
		static_cast<ScintillaCocoa *>(mTarget)->TimerFired(timer);
}

//--------------------------------------------------------------------------------------------------

/**
 * Another timer callback for the idle timer.
 */
- (void) idleTimerFired: (NSTimer *) timer {
#pragma unused(timer)
	// Idle timer event.
	// Post a new idle notification, which gets executed when the run loop is idle.
	// Since we are coalescing on name and sender there will always be only one actual notification
	// even for multiple requests.
	NSNotification *notification = [NSNotification notificationWithName: @"Idle" object: self];
	[notificationQueue enqueueNotification: notification
				  postingStyle: NSPostWhenIdle
				  coalesceMask: (NSNotificationCoalescingOnName | NSNotificationCoalescingOnSender)
				      forModes: @[NSDefaultRunLoopMode, NSModalPanelRunLoopMode]];
}

//--------------------------------------------------------------------------------------------------

/**
 * Another step for idle events. The timer (for idle events) simply requests a notification on
 * idle time. Only when this notification is send we actually call back the editor.
 */
- (void) idleTriggered: (NSNotification *) notification {
#pragma unused(notification)
	if (mTarget)
		static_cast<ScintillaCocoa *>(mTarget)->IdleTimerFired();
}

@end

//----------------- ScintillaCocoa -----------------------------------------------------------------

ScintillaCocoa::ScintillaCocoa(ScintillaView *sciView_, SCIContentView *viewContent, SCIMarginView *viewMargin) {
	vs.marginInside = false;

	// Don't retain since we're owned by view, which would cause a cycle
	sciView = sciView_;
	wMain = (__bridge WindowID)viewContent;
	wMargin = (__bridge WindowID)viewMargin;

	timerTarget = [[TimerTarget alloc] init: this];
	lastMouseEvent = NULL;
	delegate = NULL;
	notifyObj = NULL;
	notifyProc = NULL;
	capturedMouse = false;
	isFirstResponder = false;
	isActive = NSApp.isActive;
	enteredSetScrollingSize = false;
	scrollSpeed = 1;
	scrollTicks = 2000;
	observer = NULL;
	layerFindIndicator = NULL;
	imeInteraction = IMEInteraction::Inline;
	std::fill(timers, std::end(timers), nil);
	Init();
}

//--------------------------------------------------------------------------------------------------

ScintillaCocoa::~ScintillaCocoa() {
	[timerTarget ownerDestroyed];
}

//--------------------------------------------------------------------------------------------------

/**
 * Core initialization of the control. Everything that needs to be set up happens here.
 */
void ScintillaCocoa::Init() {

	// Tell Scintilla not to buffer: Quartz buffers drawing for us.
	WndProc(Message::SetBufferedDraw, 0, 0);

	// We are working with Unicode exclusively.
	WndProc(Message::SetCodePage, SC_CP_UTF8, 0);

	// Add Mac specific key bindings.
	for (int i = 0; static_cast<int>(macMapDefault[i].key); i++)
		kmap.AssignCmdKey(macMapDefault[i].key, macMapDefault[i].modifiers, macMapDefault[i].msg);

}

//--------------------------------------------------------------------------------------------------

/**
 * We need some clean up. Do it here.
 */
void ScintillaCocoa::Finalise() {
	ObserverRemove();
	for (size_t tr=static_cast<size_t>(TickReason::caret); tr<=static_cast<size_t>(TickReason::platform); tr++) {
		FineTickerCancel(static_cast<TickReason>(tr));
	}
	ScintillaBase::Finalise();
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::UpdateObserver(CFRunLoopObserverRef /* observer */, CFRunLoopActivity /* activity */, void *info) {
	ScintillaCocoa *sci = static_cast<ScintillaCocoa *>(info);
	sci->IdleWork();
}

//--------------------------------------------------------------------------------------------------

/**
 * Add an observer to the run loop to perform styling as high-priority idle task.
 */

void ScintillaCocoa::ObserverAdd() {
	if (!observer) {
		CFRunLoopObserverContext context;
		context.version = 0;
		context.info = this;
		context.retain = NULL;
		context.release = NULL;
		context.copyDescription = NULL;

		CFRunLoopRef mainRunLoop = CFRunLoopGetMain();
		observer = CFRunLoopObserverCreate(NULL, kCFRunLoopEntry | kCFRunLoopBeforeWaiting,
						   true, 0, UpdateObserver, &context);
		CFRunLoopAddObserver(mainRunLoop, observer, kCFRunLoopCommonModes);
	}
}

//--------------------------------------------------------------------------------------------------

/**
 * Remove the run loop observer.
 */
void ScintillaCocoa::ObserverRemove() {
	if (observer) {
		CFRunLoopRef mainRunLoop = CFRunLoopGetMain();
		CFRunLoopRemoveObserver(mainRunLoop, observer, kCFRunLoopCommonModes);
		CFRelease(observer);
	}
	observer = NULL;
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::IdleWork() {
	Editor::IdleWork();
	ObserverRemove();
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::QueueIdleWork(WorkItems items, Sci::Position upTo) {
	Editor::QueueIdleWork(items, upTo);
	ObserverAdd();
}

//--------------------------------------------------------------------------------------------------

/**
 * Convert a Core Foundation string into a std::string in a particular encoding.
 */

static std::string EncodedBytesString(CFStringRef cfsRef, CFStringEncoding encoding) {
	const CFRange rangeAll = {0, CFStringGetLength(cfsRef)};
	CFIndex usedLen = 0;
	CFStringGetBytes(cfsRef, rangeAll, encoding, '?', false,
			 NULL, 0, &usedLen);

	std::string buffer(usedLen, '\0');
	if (usedLen > 0) {
		CFStringGetBytes(cfsRef, rangeAll, encoding, '?', false,
				 reinterpret_cast<UInt8 *>(&buffer[0]), usedLen, NULL);
	}
	return buffer;
}

//--------------------------------------------------------------------------------------------------

/**
 * Create a Core Foundation string from a string.
 * This is a simple wrapper that specifies common arguments (the default allocator and
 * false for isExternalRepresentation) and avoids casting since strings in Scintilla
 * contain char, not UInt8 (unsigned char).
 */

static CFStringRef CFStringFromString(const char *s, size_t len, CFStringEncoding encoding) {
	return CFStringCreateWithBytes(kCFAllocatorDefault,
				       reinterpret_cast<const UInt8 *>(s),
				       len, encoding, false);
}

//--------------------------------------------------------------------------------------------------

/**
 * Case folders.
 */

class CaseFolderDBCS : public CaseFolderTable {
	CFStringEncoding encoding;
public:
	explicit CaseFolderDBCS(CFStringEncoding encoding_) : encoding(encoding_) {
	}
	size_t Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) override {
		if ((lenMixed == 1) && (sizeFolded > 0)) {
			folded[0] = mapping[static_cast<unsigned char>(mixed[0])];
			return 1;
		} else {
			CFStringRef cfsVal = CFStringFromString(mixed, lenMixed, encoding);
			if (!cfsVal) {
				folded[0] = '\0';
				return 1;
			}

			NSString *sMapped = [(__bridge NSString *)cfsVal stringByFoldingWithOptions: NSCaseInsensitiveSearch
											     locale: [NSLocale currentLocale]];

			std::string encoded = EncodedBytesString((__bridge CFStringRef)sMapped, encoding);

			size_t lenMapped = encoded.length();
			if (lenMapped < sizeFolded) {
				memcpy(folded, encoded.c_str(), lenMapped);
			} else {
				folded[0] = '\0';
				lenMapped = 1;
			}
			CFRelease(cfsVal);
			return lenMapped;
		}
	}
};

std::unique_ptr<CaseFolder> ScintillaCocoa::CaseFolderForEncoding() {
	if (pdoc->dbcsCodePage == SC_CP_UTF8) {
		return std::make_unique<CaseFolderUnicode>();
	} else {
		CFStringEncoding encoding = EncodingFromCharacterSet(IsUnicodeMode(),
					    vs.styles[StyleDefault].characterSet);
		if (pdoc->dbcsCodePage == 0) {
			std::unique_ptr<CaseFolderTable> pcf = std::make_unique<CaseFolderTable>();
			// Only for single byte encodings
			for (int i=0x80; i<0x100; i++) {
				char sCharacter[2] = "A";
				sCharacter[0] = static_cast<char>(i);
				CFStringRef cfsVal = CFStringFromString(sCharacter, 1, encoding);
				if (!cfsVal)
					continue;

				NSString *sMapped = [(__bridge NSString *)cfsVal stringByFoldingWithOptions: NSCaseInsensitiveSearch
												     locale: [NSLocale currentLocale]];

				std::string encoded = EncodedBytesString((__bridge CFStringRef)sMapped, encoding);

				if (encoded.length() == 1) {
					pcf->SetTranslation(sCharacter[0], encoded[0]);
				}

				CFRelease(cfsVal);
			}
			return pcf;
		} else {
			return std::make_unique<CaseFolderDBCS>(encoding);
		}
	}
}


//--------------------------------------------------------------------------------------------------

/**
 * Case-fold the given string depending on the specified case mapping type.
 */
std::string ScintillaCocoa::CaseMapString(const std::string &s, CaseMapping caseMapping) {
	if ((s.size() == 0) || (caseMapping == CaseMapping::same))
		return s;

	if (IsUnicodeMode()) {
		std::string retMapped(s.length() * maxExpansionCaseConversion, 0);
		size_t lenMapped = CaseConvertString(&retMapped[0], retMapped.length(), s.c_str(), s.length(),
						     (caseMapping == CaseMapping::upper) ? CaseConversion::upper : CaseConversion::lower);
		retMapped.resize(lenMapped);
		return retMapped;
	}

	CFStringEncoding encoding = EncodingFromCharacterSet(IsUnicodeMode(),
				    vs.styles[StyleDefault].characterSet);

	CFStringRef cfsVal = CFStringFromString(s.c_str(), s.length(), encoding);
	if (!cfsVal) {
		return s;
	}

	NSString *sMapped;
	switch (caseMapping) {
	case CaseMapping::upper:
		sMapped = ((__bridge NSString *)cfsVal).uppercaseString;
		break;
	case CaseMapping::lower:
		sMapped = ((__bridge NSString *)cfsVal).lowercaseString;
		break;
	default:
		sMapped = (__bridge NSString *)cfsVal;
	}

	// Back to encoding
	std::string result = EncodedBytesString((__bridge CFStringRef)sMapped, encoding);
	CFRelease(cfsVal);
	return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Cancel all modes, both for base class and any find indicator.
 */
void ScintillaCocoa::CancelModes() {
	ScintillaBase::CancelModes();
	HideFindIndicator();
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper function to get the scrolling view.
 */
NSScrollView *ScintillaCocoa::ScrollContainer() const {
	NSView *container = (__bridge NSView *)(wMain.GetID());
	return static_cast<NSScrollView *>(container.superview.superview);
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper function to get the inner container which represents the actual "canvas" we work with.
 */
SCIContentView *ScintillaCocoa::ContentView() {
	return (__bridge SCIContentView *)(wMain.GetID());
}

//--------------------------------------------------------------------------------------------------

/**
 * Return the top left visible point relative to the origin point of the whole document.
 */
Scintilla::Internal::Point ScintillaCocoa::GetVisibleOriginInMain() const {
	NSScrollView *scrollView = ScrollContainer();
	NSRect contentRect = scrollView.contentView.bounds;
	return Point(static_cast<XYPOSITION>(contentRect.origin.x), static_cast<XYPOSITION>(contentRect.origin.y));
}

//--------------------------------------------------------------------------------------------------

/**
 * Instead of returning the size of the inner view we have to return the visible part of it
 * in order to make scrolling working properly.
 * The returned value is in document coordinates.
 */
PRectangle ScintillaCocoa::GetClientRectangle() const {
	NSScrollView *scrollView = ScrollContainer();
	NSSize size = scrollView.contentView.bounds.size;
	Point origin = GetVisibleOriginInMain();
	return PRectangle(origin.x, origin.y, static_cast<XYPOSITION>(origin.x+size.width),
			  static_cast<XYPOSITION>(origin.y + size.height));
}

//--------------------------------------------------------------------------------------------------

/**
 * Allow for prepared rectangle
 */
PRectangle ScintillaCocoa::GetClientDrawingRectangle() {
#if MAC_OS_X_VERSION_MAX_ALLOWED > 1080
	NSView *content = ContentView();
	if ([content respondsToSelector: @selector(setPreparedContentRect:)]) {
		NSRect rcPrepared = content.preparedContentRect;
		if (!NSIsEmptyRect(rcPrepared))
			return NSRectToPRectangle(rcPrepared);
	}
#endif
	return ScintillaCocoa::GetClientRectangle();
}

//--------------------------------------------------------------------------------------------------

/**
 * Converts the given point from base coordinates to local coordinates and at the same time into
 * a native Point structure. Base coordinates are used for the top window used in the view hierarchy.
 * Returned value is in view coordinates.
 */
Scintilla::Internal::Point ScintillaCocoa::ConvertPoint(NSPoint point) {
	NSView *container = ContentView();
	NSPoint result = [container convertPoint: point fromView: nil];
	Scintilla::Internal::Point ptOrigin = GetVisibleOriginInMain();
	return Point(static_cast<XYPOSITION>(result.x - ptOrigin.x), static_cast<XYPOSITION>(result.y - ptOrigin.y));
}

//--------------------------------------------------------------------------------------------------

/**
 * Do not clip like superclass as Cocoa is not reporting all of prepared area.
 */
void ScintillaCocoa::RedrawRect(PRectangle rc) {
	if (!rc.Empty())
		wMain.InvalidateRectangle(rc);
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::DiscardOverdraw() {
#if MAC_OS_X_VERSION_MAX_ALLOWED > 1080
	// If running on 10.9, reset prepared area to visible area
	NSView *content = ContentView();
	if ([content respondsToSelector: @selector(setPreparedContentRect:)]) {
		content.preparedContentRect = content.visibleRect;
	}
#endif
}

//--------------------------------------------------------------------------------------------------

/**
 * Ensure all of prepared content is also redrawn.
 */
void ScintillaCocoa::Redraw() {
	wMargin.InvalidateAll();
	DiscardOverdraw();
	wMain.InvalidateAll();
}

//--------------------------------------------------------------------------------------------------

/**
 * A function to directly execute code that would usually go the long way via window messages.
 * However this is a Windows metaphor and not used here, hence we just call our fake
 * window proc. The given parameters directly reflect the message parameters used on Windows.
 *
 * @param ptr The target which is to be called.
 * @param iMessage A code that indicates which message was sent.
 * @param wParam One of the two free parameters for the message. Traditionally a word sized parameter
 *               (hence the w prefix).
 * @param lParam The other of the two free parameters. A signed long.
 */
sptr_t ScintillaCocoa::DirectFunction(sptr_t ptr, unsigned int iMessage, uptr_t wParam,
				      sptr_t lParam) {
	ScintillaCocoa *sci = reinterpret_cast<ScintillaCocoa *>(ptr);
	return sci->WndProc(static_cast<Message>(iMessage), wParam, lParam);
}

//--------------------------------------------------------------------------------------------------

/**
 * A function to directly execute code that would usually go the long way via window messages.
 * Similar to DirectFunction but also returns the status.
 * However this is a Windows metaphor and not used here, hence we just call our fake
 * window proc. The given parameters directly reflect the message parameters used on Windows.
 *
 * @param ptr The target which is to be called.
 * @param iMessage A code that indicates which message was sent.
 * @param wParam One of the two free parameters for the message. Traditionally a word sized parameter
 *               (hence the w prefix).
 * @param lParam The other of the two free parameters. A signed long.
 * @param pStatus Return the status to the caller. A pointer to an int.
 */
sptr_t ScintillaCocoa::DirectStatusFunction(sptr_t ptr, unsigned int iMessage, uptr_t wParam,
				      sptr_t lParam, int *pStatus) {
	ScintillaCocoa *sci = reinterpret_cast<ScintillaCocoa *>(ptr);
	const sptr_t returnValue = sci->WndProc(static_cast<Message>(iMessage), wParam, lParam);
	*pStatus = static_cast<int>(sci->errorStatus);
	return returnValue;
}

//--------------------------------------------------------------------------------------------------

/**
 * This method is very similar to DirectFunction. On Windows it sends a message (not in the Obj-C sense)
 * to the target window. Here we simply call our fake window proc.
 */
sptr_t scintilla_send_message(void *sci, unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	ScintillaView *control = (__bridge ScintillaView *)(sci);
	return [control message: iMessage wParam: wParam lParam: lParam];
}

//--------------------------------------------------------------------------------------------------

namespace {

/**
 * The animated find indicator fails with a "bogus layer size" message on macOS 10.13
 * and causes drawing failures on macOS 10.12.
 */

bool SupportAnimatedFind() {
	return std::floor(NSAppKitVersionNumber) < NSAppKitVersionNumber10_12;
}

}

//--------------------------------------------------------------------------------------------------

/**
 * That's our fake window procedure. On Windows each window has a dedicated procedure to handle
 * commands (also used to synchronize UI and background threads), which is not the case in Cocoa.
 *
 * Messages handled here are almost solely for special commands of the backend. Everything which
 * would be system messages on Windows (e.g. for key down, mouse move etc.) are handled by
 * directly calling appropriate handlers.
 */
sptr_t ScintillaCocoa::WndProc(Message iMessage, uptr_t wParam, sptr_t lParam) {
	try {
		switch (iMessage) {
		case Message::GetDirectFunction:
			return reinterpret_cast<sptr_t>(DirectFunction);

		case Message::GetDirectStatusFunction:
			return reinterpret_cast<sptr_t>(DirectStatusFunction);

		case Message::GetDirectPointer:
			return reinterpret_cast<sptr_t>(this);

		case Message::SetBidirectional:
			bidirectional = static_cast<Bidirectional>(wParam);
			// Invalidate all cached information including layout.
			DropGraphics();
			InvalidateStyleRedraw();
			return 0;

		case Message::TargetAsUTF8:
			return TargetAsUTF8(CharPtrFromSPtr(lParam));

		case Message::EncodedFromUTF8:
			return EncodedFromUTF8(ConstCharPtrFromUPtr(wParam),
					       CharPtrFromSPtr(lParam));

		case Message::SetIMEInteraction:
			// Only inline IME supported on Cocoa
			break;

		case Message::GrabFocus:
			[ContentView().window makeFirstResponder: ContentView()];
			break;

		case Message::SetBufferedDraw:
			// Buffered drawing not supported on Cocoa
			view.bufferedDraw = false;
			break;

		case Message::FindIndicatorShow:
			if (SupportAnimatedFind()) {
				ShowFindIndicatorForRange(NSMakeRange(wParam, lParam-wParam), YES);
			}
			return 0;

		case Message::FindIndicatorFlash:
			if (SupportAnimatedFind()) {
				ShowFindIndicatorForRange(NSMakeRange(wParam, lParam-wParam), NO);
			}
			return 0;

		case Message::FindIndicatorHide:
			HideFindIndicator();
			return 0;

		case Message::SetPhasesDraw: {
				sptr_t r = ScintillaBase::WndProc(iMessage, wParam, lParam);
				[sciView updateIndicatorIME];
				return r;
			}

		case Message::GetAccessibility:
			return static_cast<sptr_t>(Accessibility::Enabled);

		default:
			sptr_t r = ScintillaBase::WndProc(iMessage, wParam, lParam);

			return r;
		}
	} catch (std::bad_alloc &) {
		errorStatus = Status::BadAlloc;
	} catch (...) {
		errorStatus = Status::Failure;
	}
	return 0;
}

//--------------------------------------------------------------------------------------------------

/**
 * In Windows lingo this is the handler which handles anything that wasn't handled in the normal
 * window proc which would usually send the message back to generic window proc that Windows uses.
 */
sptr_t ScintillaCocoa::DefWndProc(Message, uptr_t, sptr_t) {
	return 0;
}

//--------------------------------------------------------------------------------------------------

/**
 * Handle any ScintillaCocoa-specific ticking or call superclass.
 */
void ScintillaCocoa::TickFor(TickReason reason) {
	if (reason == TickReason::platform) {
		DragScroll();
	} else {
		Editor::TickFor(reason);
	}
}

//--------------------------------------------------------------------------------------------------

/**
 * Is a particular timer currently running?
 */
bool ScintillaCocoa::FineTickerRunning(TickReason reason) {
	return timers[static_cast<size_t>(reason)] != nil;
}

//--------------------------------------------------------------------------------------------------

/**
 * Start a fine-grained timer.
 */
void ScintillaCocoa::FineTickerStart(TickReason reason, int millis, int tolerance) {
	FineTickerCancel(reason);
	NSTimer *fineTimer = [NSTimer timerWithTimeInterval: millis / 1000.0
						     target: timerTarget
						   selector: @selector(timerFired:)
						   userInfo: nil
						    repeats: YES];
	if (tolerance && [fineTimer respondsToSelector: @selector(setTolerance:)]) {
		fineTimer.tolerance = tolerance / 1000.0;
	}
	timers[static_cast<size_t>(reason)] = fineTimer;
	[NSRunLoop.currentRunLoop addTimer: fineTimer forMode: NSDefaultRunLoopMode];
	[NSRunLoop.currentRunLoop addTimer: fineTimer forMode: NSModalPanelRunLoopMode];
}

//--------------------------------------------------------------------------------------------------

/**
 * Cancel a fine-grained timer.
 */
void ScintillaCocoa::FineTickerCancel(TickReason reason) {
	const size_t reasonIndex = static_cast<size_t>(reason);
	if (timers[reasonIndex]) {
		[timers[reasonIndex] invalidate];
		timers[reasonIndex] = nil;
	}
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::SetIdle(bool on) {
	if (idler.state != on) {
		idler.state = on;
		if (idler.state) {
			// Scintilla ticks = milliseconds
			NSTimer *idleTimer = [NSTimer scheduledTimerWithTimeInterval: timer.tickSize / 1000.0
									      target: timerTarget
									    selector: @selector(idleTimerFired:)
									    userInfo: nil
									     repeats: YES];
			[NSRunLoop.currentRunLoop addTimer: idleTimer forMode: NSModalPanelRunLoopMode];
			idler.idlerID = (__bridge IdlerID)idleTimer;
		} else if (idler.idlerID != NULL) {
			[(__bridge NSTimer *)(idler.idlerID) invalidate];
			idler.idlerID = 0;
		}
	}
	return true;
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::CopyToClipboard(const SelectionText &selectedText) {
	SetPasteboardData([NSPasteboard generalPasteboard], selectedText);
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::Copy() {
	if (!sel.Empty()) {
		SelectionText selectedText;
		CopySelectionRange(&selectedText);
		CopyToClipboard(selectedText);
	}
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::CanPaste() {
	if (!Editor::CanPaste())
		return false;

	return GetPasteboardData([NSPasteboard generalPasteboard], NULL);
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::Paste() {
	Paste(false);
}

//--------------------------------------------------------------------------------------------------

/**
 * Pastes data from the paste board into the editor.
 */
void ScintillaCocoa::Paste(bool forceRectangular) {
	SelectionText selectedText;
	bool ok = GetPasteboardData([NSPasteboard generalPasteboard], &selectedText);
	if (forceRectangular)
		selectedText.rectangular = forceRectangular;

	if (!ok || selectedText.Empty())
		// No data or no flavor we support.
		return;

	pdoc->BeginUndoAction();
	ClearSelection(false);
	InsertPasteShape(selectedText.Data(), selectedText.Length(),
			 selectedText.rectangular ? PasteShape::rectangular : PasteShape::stream);
	pdoc->EndUndoAction();

	Redraw();
	EnsureCaretVisible();
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::CTPaint(void *gc, NSRect rc) {
#pragma unused(rc)
	std::unique_ptr<Surface> surfaceWindow(Surface::Allocate(Technology::Default));
	surfaceWindow->Init(gc, wMain.GetID());
	surfaceWindow->SetMode(CurrentSurfaceMode());
	ct.PaintCT(surfaceWindow.get());
	surfaceWindow->Release();
}

@interface CallTipView : NSControl {
	ScintillaCocoa *sci;
}

@end

@implementation CallTipView

- (NSView *) initWithFrame: (NSRect) frame {
	self = [super initWithFrame: frame];

	if (self) {
		sci = NULL;
	}

	return self;
}


- (BOOL) isFlipped {
	return YES;
}

- (void) setSci: (ScintillaCocoa *) sci_ {
	sci = sci_;
}

- (void) drawRect: (NSRect) needsDisplayInRect {
	if (sci) {
		CGContextRef context = (CGContextRef) [NSGraphicsContext currentContext].graphicsPort;
		sci->CTPaint(context, needsDisplayInRect);
	}
}

- (void) mouseDown: (NSEvent *) event {
	if (sci) {
		sci->CallTipMouseDown(event.locationInWindow);
	}
}

// On macOS, only the key view should modify the cursor so the calltip can't.
// This view does not become key so resetCursorRects never called.
- (void) resetCursorRects {
	//[super resetCursorRects];
	//[self addCursorRect: [self bounds] cursor: [NSCursor arrowCursor]];
}

@end

void ScintillaCocoa::CallTipMouseDown(NSPoint pt) {
	NSRect rectBounds = ((__bridge NSView *)(ct.wDraw.GetID())).bounds;
	Point location(static_cast<XYPOSITION>(pt.x),
		       static_cast<XYPOSITION>(rectBounds.size.height - pt.y));
	ct.MouseClick(location);
	CallTipClick();
}

static bool HeightDifferent(WindowID wCallTip, PRectangle rc) {
	NSWindow *callTip = (__bridge NSWindow *)wCallTip;
	CGFloat height = NSHeight(callTip.frame);
	return height != rc.Height();
}

void ScintillaCocoa::CreateCallTipWindow(PRectangle rc) {
	if (ct.wCallTip.Created() && HeightDifferent(ct.wCallTip.GetID(), rc)) {
		ct.wCallTip.Destroy();
	}
	if (!ct.wCallTip.Created()) {
		NSRect ctRect = NSMakeRect(rc.top, rc.bottom, rc.Width(), rc.Height());
		NSWindow *callTip = [[NSWindow alloc] initWithContentRect: ctRect
								styleMask: NSWindowStyleMaskBorderless
								  backing: NSBackingStoreBuffered
								    defer: NO];
		[callTip setLevel: NSModalPanelWindowLevel+1];
		[callTip setHasShadow: YES];
		NSRect ctContent = NSMakeRect(0, 0, rc.Width(), rc.Height());
		CallTipView *caption = [[CallTipView alloc] initWithFrame: ctContent];
		caption.autoresizingMask = NSViewWidthSizable | NSViewMaxYMargin;
		[caption setSci: this];
		[callTip.contentView addSubview: caption];
		[callTip orderFront: caption];
		ct.wCallTip = (__bridge_retained WindowID)callTip;
		ct.wDraw = (__bridge WindowID)caption;
	}
}

void ScintillaCocoa::AddToPopUp(const char *label, int cmd, bool enabled) {
	NSMenuItem *item;
	ScintillaContextMenu *menu = (__bridge ScintillaContextMenu *)(popup.GetID());
	[menu setOwner: this];
	[menu setAutoenablesItems: NO];

	if (cmd == 0) {
		item = [NSMenuItem separatorItem];
	} else {
		item = [[NSMenuItem alloc] init];
		item.title = @(label);
	}
	item.target = menu;
	item.action = @selector(handleCommand:);
	item.tag = cmd;
	item.enabled = enabled;

	[menu addItem: item];
}

// -------------------------------------------------------------------------------------------------

void ScintillaCocoa::ClaimSelection() {
	// macOS does not have a primary selection.
}

// -------------------------------------------------------------------------------------------------

/**
 * Returns the current caret position (which is tracked as an offset into the entire text string)
 * as a row:column pair. The result is zero-based.
 */
NSPoint ScintillaCocoa::GetCaretPosition() {
	const Sci::Line line = static_cast<Sci::Line>(
		pdoc->LineFromPosition(sel.RangeMain().caret.Position()));
	NSPoint result;

	result.y = line;
	result.x = sel.RangeMain().caret.Position() - pdoc->LineStart(line);
	return result;
}

// -------------------------------------------------------------------------------------------------

std::string ScintillaCocoa::UTF8FromEncoded(std::string_view encoded) const {
	if (IsUnicodeMode()) {
		return std::string(encoded);
	} else {
		CFStringEncoding encoding = EncodingFromCharacterSet(IsUnicodeMode(),
					    vs.styles[StyleDefault].characterSet);
		CFStringRef cfsVal = CFStringFromString(encoded.data(), encoded.length(), encoding);
		std::string utf = EncodedBytesString(cfsVal, kCFStringEncodingUTF8);
		if (cfsVal)
			CFRelease(cfsVal);
		return utf;
	}
}

// -------------------------------------------------------------------------------------------------

std::string ScintillaCocoa::EncodedFromUTF8(std::string_view utf8) const {
	if (IsUnicodeMode()) {
		return std::string(utf8);
	} else {
		CFStringEncoding encoding = EncodingFromCharacterSet(IsUnicodeMode(),
					    vs.styles[StyleDefault].characterSet);
		CFStringRef cfsVal = CFStringFromString(utf8.data(), utf8.length(), kCFStringEncodingUTF8);
		const std::string sEncoded = EncodedBytesString(cfsVal, encoding);
		if (cfsVal)
			CFRelease(cfsVal);
		return sEncoded;
	}
}

// -------------------------------------------------------------------------------------------------

#pragma mark Drag

/**
 * Triggered by the tick timer on a regular basis to scroll the content during a drag operation.
 */
void ScintillaCocoa::DragScroll() {
	if (!posDrag.IsValid()) {
		scrollSpeed = 1;
		scrollTicks = 2000;
		return;
	}

	// TODO: does not work for wrapped lines, fix it.
	Sci::Line line = static_cast<Sci::Line>(pdoc->LineFromPosition(posDrag.Position()));
	Sci::Line currentVisibleLine = pcs->DisplayFromDoc(line);
	Sci::Line lastVisibleLine = std::min(topLine + LinesOnScreen(), pcs->LinesDisplayed()) - 2;

	if (currentVisibleLine <= topLine && topLine > 0)
		ScrollTo(topLine - scrollSpeed);
	else if (currentVisibleLine >= lastVisibleLine)
		ScrollTo(topLine + scrollSpeed);
	else {
		scrollSpeed = 1;
		scrollTicks = 2000;
		return;
	}

	// TODO: also handle horizontal scrolling.

	if (scrollSpeed == 1) {
		scrollTicks -= timer.tickSize;
		if (scrollTicks <= 0) {
			scrollSpeed = 5;
			scrollTicks = 2000;
		}
	}

}

//----------------- DragProviderSource -------------------------------------------------------

@interface DragProviderSource : NSObject <NSPasteboardItemDataProvider> {
	SelectionText selectedText;
}

@end

@implementation DragProviderSource

- (id) initWithSelectedText: (const SelectionText *) other {
	self = [super init];

	if (self) {
		selectedText.Copy(*other);
	}

	return self;
}

- (void) pasteboard: (NSPasteboard *) pasteboard item: (NSPasteboardItem *) item provideDataForType: (NSString *) type {
#pragma unused(item)
	if (selectedText.Length() == 0)
		return;

	if (([type compare: NSPasteboardTypeString] != NSOrderedSame) &&
			([type compare: ScintillaRecPboardType] != NSOrderedSame))
		return;

	CFStringEncoding encoding = EncodingFromCharacterSet(selectedText.codePage == SC_CP_UTF8,
				    selectedText.characterSet);

	CFStringRef cfsVal = CFStringFromString(selectedText.Data(), selectedText.Length(), encoding);
	if (!cfsVal)
		return;

	if ([type compare: NSPasteboardTypeString] == NSOrderedSame) {
		[pasteboard setString: (__bridge NSString *)cfsVal forType: NSStringPboardType];
	} else if ([type compare: ScintillaRecPboardType] == NSOrderedSame) {
		// This is specific to scintilla, allows us to drag rectangular selections around the document.
		if (selectedText.rectangular)
			[pasteboard setString: (__bridge NSString *)cfsVal forType: ScintillaRecPboardType];
	}

	if (cfsVal)
		CFRelease(cfsVal);
}

@end

//--------------------------------------------------------------------------------------------------

/**
 * Called when a drag operation was initiated from within Scintilla.
 */
void ScintillaCocoa::StartDrag() {
	if (sel.Empty())
		return;

	inDragDrop = DragDrop::dragging;

	FineTickerStart(TickReason::platform, timer.tickSize, 0);

	// Put the data to be dragged on the drag pasteboard.
	SelectionText selectedText;
	NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName: NSDragPboard];
	CopySelectionRange(&selectedText);
	SetPasteboardData(pasteboard, selectedText);

	// calculate the bounds of the selection
	PRectangle client = GetTextRectangle();
	Sci::Position selStart = sel.RangeMain().Start().Position();
	Sci::Position selEnd = sel.RangeMain().End().Position();
	Sci::Line startLine = static_cast<Sci::Line>(pdoc->LineFromPosition(selStart));
	Sci::Line endLine = static_cast<Sci::Line>(pdoc->LineFromPosition(selEnd));
	Point pt;
	Sci::Position startPos;
	Sci::Position endPos;
	Sci::Position ep;
	PRectangle rcSel;

	if (startLine==endLine && WndProc(Message::GetWrapMode, 0, 0) != static_cast<sptr_t>(Wrap::None)) {
		// Komodo bug http://bugs.activestate.com/show_bug.cgi?id=87571
		// Scintilla bug https://sourceforge.net/tracker/?func=detail&atid=102439&aid=3040200&group_id=2439
		// If the width on a wrapped-line selection is negative,
		// find a better bounding rectangle.

		Point ptStart, ptEnd;
		startPos = WndProc(Message::GetLineSelStartPosition, startLine, 0);
		endPos =   WndProc(Message::GetLineSelEndPosition,   startLine, 0);
		// step back a position if we're counting the newline
		ep =       WndProc(Message::GetLineEndPosition,      startLine, 0);
		if (endPos > ep) endPos = ep;
		ptStart = LocationFromPosition(startPos);
		ptEnd =   LocationFromPosition(endPos);
		if (ptStart.y == ptEnd.y) {
			// We're just selecting part of one visible line
			rcSel.left = ptStart.x;
			rcSel.right = ptEnd.x < client.right ? ptEnd.x : client.right;
		} else {
			// Find the bounding box.
			startPos = WndProc(Message::PositionFromLine, startLine, 0);
			rcSel.left = LocationFromPosition(startPos).x;
			rcSel.right = client.right;
		}
		rcSel.top = ptStart.y;
		rcSel.bottom = ptEnd.y + vs.lineHeight;
		if (rcSel.bottom > client.bottom) {
			rcSel.bottom = client.bottom;
		}
	} else {
		rcSel.top = rcSel.bottom = rcSel.right = rcSel.left = -1;
		for (Sci::Line l = startLine; l <= endLine; l++) {
			startPos = WndProc(Message::GetLineSelStartPosition, l, 0);
			endPos = WndProc(Message::GetLineSelEndPosition, l, 0);
			if (endPos == startPos) continue;
			// step back a position if we're counting the newline
			ep = WndProc(Message::GetLineEndPosition, l, 0);
			if (endPos > ep) endPos = ep;
			pt = LocationFromPosition(startPos); // top left of line selection
			if (pt.x < rcSel.left || rcSel.left < 0) rcSel.left = pt.x;
			if (pt.y < rcSel.top || rcSel.top < 0) rcSel.top = pt.y;
			pt = LocationFromPosition(endPos); // top right of line selection
			pt.y += vs.lineHeight; // get to the bottom of the line
			if (pt.x > rcSel.right || rcSel.right < 0) {
				if (pt.x > client.right)
					rcSel.right = client.right;
				else
					rcSel.right = pt.x;
			}
			if (pt.y > rcSel.bottom || rcSel.bottom < 0) {
				if (pt.y > client.bottom)
					rcSel.bottom = client.bottom;
				else
					rcSel.bottom = pt.y;
			}
		}
	}
	// must convert to global coordinates for drag regions, but also save the
	// image rectangle for further calculations and copy operations

	// Prepare drag image.
	NSRect selectionRectangle = PRectangleToNSRect(rcSel);
	if (NSIsEmptyRect(selectionRectangle))
		return;

	SCIContentView *content = ContentView();

	// To get a bitmap of the text we're dragging, we just use Paint on a pixmap surface.
	SurfaceImpl si;
	si.SetMode(CurrentSurfaceMode());
	std::unique_ptr<SurfaceImpl> sw = si.AllocatePixMapImplementation(static_cast<int>(client.Width()), static_cast<int>(client.Height()));

	const bool lastHideSelection = view.hideSelection;
	view.hideSelection = true;
	PRectangle imageRect = rcSel;
	paintState = PaintState::painting;
	paintingAllText = true;
	CGContextRef gcsw = sw->GetContext();
	CGContextTranslateCTM(gcsw, -client.left, -client.top);
	Paint(sw.get(), client);
	paintState = PaintState::notPainting;
	view.hideSelection = lastHideSelection;

	std::unique_ptr<SurfaceImpl> pixmap = si.AllocatePixMapImplementation(static_cast<int>(imageRect.Width()),
								  static_cast<int>(imageRect.Height()));
	CGContextRef gc = pixmap->GetContext();
	// To make Paint() work on a bitmap, we have to flip our coordinates and translate the origin
	CGContextTranslateCTM(gc, 0, imageRect.Height());
	CGContextScaleCTM(gc, 1.0, -1.0);

	pixmap->CopyImageRectangle(sw.get(), imageRect, PRectangle(0.0f, 0.0f, imageRect.Width(), imageRect.Height()));
	// XXX TODO: overwrite any part of the image that is not part of the
	//           selection to make it transparent.  right now we just use
	//           the full rectangle which may include non-selected text.

	NSBitmapImageRep *bitmap = NULL;
	CGImageRef imagePixmap = pixmap->CreateImage();
	if (imagePixmap)
		bitmap = [[NSBitmapImageRep alloc] initWithCGImage: imagePixmap];
	CGImageRelease(imagePixmap);

	NSImage *image = [[NSImage alloc] initWithSize: selectionRectangle.size];
	[image addRepresentation: bitmap];

	NSImage *dragImage = [[NSImage alloc] initWithSize: selectionRectangle.size];
	dragImage.backgroundColor = [NSColor clearColor];
	[dragImage lockFocus];
	[image drawAtPoint: NSZeroPoint fromRect: NSZeroRect operation: NSCompositingOperationSourceOver fraction: 0.5];
	[dragImage unlockFocus];

	NSPoint startPoint;
	startPoint.x = selectionRectangle.origin.x + client.left;
	startPoint.y = selectionRectangle.origin.y + selectionRectangle.size.height + client.top;

	NSPasteboardItem *pbItem = [NSPasteboardItem new];
	DragProviderSource *dps = [[DragProviderSource alloc] initWithSelectedText: &selectedText];

	NSArray *pbTypes = selectedText.rectangular ?
			   @[NSPasteboardTypeString, ScintillaRecPboardType] :
			   @[NSPasteboardTypeString];
	[pbItem setDataProvider: dps forTypes: pbTypes];
	NSDraggingItem *dragItem = [[NSDraggingItem alloc ]initWithPasteboardWriter: pbItem];

	NSScrollView *scrollContainer = ScrollContainer();
	NSRect contentRect = scrollContainer.contentView.bounds;
	NSRect draggingRect = NSOffsetRect(selectionRectangle, contentRect.origin.x, contentRect.origin.y);
	[dragItem setDraggingFrame: draggingRect contents: dragImage];
	NSDraggingSession *dragSession =
		[content beginDraggingSessionWithItems: @[dragItem]
						 event: lastMouseEvent
						source: content];
	dragSession.animatesToStartingPositionsOnCancelOrFail = YES;
	dragSession.draggingFormation = NSDraggingFormationNone;
}

//--------------------------------------------------------------------------------------------------

/**
 * Called when a drag operation reaches the control which was initiated outside.
 */
NSDragOperation ScintillaCocoa::DraggingEntered(id <NSDraggingInfo> info) {
	FineTickerStart(TickReason::platform, timer.tickSize, 0);
	return DraggingUpdated(info);
}

//--------------------------------------------------------------------------------------------------

/**
 * Called frequently during a drag operation if we are the target. Keep telling the caller
 * what drag operation we accept and update the drop caret position to indicate the
 * potential insertion point of the dragged data.
 */
NSDragOperation ScintillaCocoa::DraggingUpdated(id <NSDraggingInfo> info) {
	// Convert the drag location from window coordinates to view coordinates and
	// from there to a text position to finally set the drag position.
	Point location = ConvertPoint([info draggingLocation]);
	SetDragPosition(SPositionFromLocation(location));

	NSDragOperation sourceDragMask = [info draggingSourceOperationMask];
	if (sourceDragMask == NSDragOperationNone)
		return sourceDragMask;

	NSPasteboard *pasteboard = [info draggingPasteboard];

	// Return what type of operation we will perform. Prefer move over copy.
	if ([pasteboard.types containsObject: NSStringPboardType] ||
			[pasteboard.types containsObject: ScintillaRecPboardType])
		return (sourceDragMask & NSDragOperationMove) ? NSDragOperationMove : NSDragOperationCopy;

	if ([pasteboard.types containsObject: NSFilenamesPboardType])
		return (sourceDragMask & NSDragOperationGeneric);
	return NSDragOperationNone;
}

//--------------------------------------------------------------------------------------------------

/**
 * Resets the current drag position as we are no longer the drag target.
 */
void ScintillaCocoa::DraggingExited(id <NSDraggingInfo> info) {
#pragma unused(info)
	SetDragPosition(SelectionPosition(Sci::invalidPosition));
	FineTickerCancel(TickReason::platform);
	inDragDrop = DragDrop::none;
}

//--------------------------------------------------------------------------------------------------

/**
 * Here is where the real work is done. Insert the text from the pasteboard.
 */
bool ScintillaCocoa::PerformDragOperation(id <NSDraggingInfo> info) {
	NSPasteboard *pasteboard = [info draggingPasteboard];

	if ([pasteboard.types containsObject: NSFilenamesPboardType]) {
		NSArray *files = [pasteboard propertyListForType: NSFilenamesPboardType];
		for (NSString* uri in files)
			NotifyURIDropped(uri.UTF8String);
	} else {
		SelectionText text;
		GetPasteboardData(pasteboard, &text);

		if (text.Length() > 0) {
			NSDragOperation operation = [info draggingSourceOperationMask];
			bool moving = (operation & NSDragOperationMove) != 0;

			DropAt(posDrag, text.Data(), text.Length(), moving, text.rectangular);
		};
	}

	return true;
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::SetPasteboardData(NSPasteboard *board, const SelectionText &selectedText) {
	if (selectedText.Length() == 0)
		return;

	CFStringEncoding encoding = EncodingFromCharacterSet(selectedText.codePage == SC_CP_UTF8,
				    selectedText.characterSet);

	CFStringRef cfsVal = CFStringFromString(selectedText.Data(), selectedText.Length(), encoding);
	if (!cfsVal)
		return;

	NSArray *pbTypes = selectedText.rectangular ?
			   @[NSStringPboardType, ScintillaRecPboardType] :
			   @[NSStringPboardType];
	[board declareTypes: pbTypes owner: nil];

	if (selectedText.rectangular) {
		// This is specific to scintilla, allows us to drag rectangular selections around the document.
		[board setString: (__bridge NSString *)cfsVal forType: ScintillaRecPboardType];
	}

	[board setString: (__bridge NSString *)cfsVal forType: NSStringPboardType];

	if (cfsVal)
		CFRelease(cfsVal);
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper method to retrieve the best fitting alternative from the general pasteboard.
 */
bool ScintillaCocoa::GetPasteboardData(NSPasteboard *board, SelectionText *selectedText) {
	NSArray *supportedTypes = @[ScintillaRecPboardType,
				    NSStringPboardType];
	NSString *bestType = [board availableTypeFromArray: supportedTypes];
	NSString *data = [board stringForType: bestType];

	if (data != nil) {
		if (selectedText != nil) {
			CFStringEncoding encoding = EncodingFromCharacterSet(IsUnicodeMode(),
						    vs.styles[StyleDefault].characterSet);
			CFRange rangeAll = {0, static_cast<CFIndex>(data.length)};
			CFIndex usedLen = 0;
			CFStringGetBytes((CFStringRef)data, rangeAll, encoding, '?',
					 false, NULL, 0, &usedLen);

			std::vector<UInt8> buffer(usedLen);

			CFStringGetBytes((CFStringRef)data, rangeAll, encoding, '?',
					 false, buffer.data(), usedLen, NULL);

			bool rectangular = bestType == ScintillaRecPboardType;

			std::string dest(reinterpret_cast<const char *>(buffer.data()), usedLen);

			selectedText->Copy(dest, pdoc->dbcsCodePage,
					   vs.styles[StyleDefault].characterSet, rectangular, false);
		}
		return true;
	}

	return false;
}

//--------------------------------------------------------------------------------------------------

// Returns the target converted to UTF8.
// Return the length in bytes.
Sci::Position ScintillaCocoa::TargetAsUTF8(char *text) const {
	const Sci::Position targetLength = targetRange.Length();
	if (IsUnicodeMode()) {
		if (text)
			pdoc->GetCharRange(text, targetRange.start.Position(), targetLength);
	} else {
		// Need to convert
		const CFStringEncoding encoding = EncodingFromCharacterSet(IsUnicodeMode(),
						  vs.styles[StyleDefault].characterSet);
		const std::string s = RangeText(targetRange.start.Position(), targetRange.end.Position());
		CFStringRef cfsVal = CFStringFromString(s.c_str(), s.length(), encoding);
		if (!cfsVal) {
			return 0;
		}

		const std::string tmputf = EncodedBytesString(cfsVal, kCFStringEncodingUTF8);

		if (text)
			memcpy(text, tmputf.c_str(), tmputf.length());
		CFRelease(cfsVal);
		return tmputf.length();
	}
	return targetLength;
}

//--------------------------------------------------------------------------------------------------

// Returns the text in the range converted to an NSString.
NSString *ScintillaCocoa::RangeTextAsString(NSRange rangePositions) const {
	const std::string text = RangeText(rangePositions.location,
					   NSMaxRange(rangePositions));
	if (IsUnicodeMode()) {
		return @(text.c_str());
	} else {
		// Need to convert
		const CFStringEncoding encoding = EncodingFromCharacterSet(IsUnicodeMode(),
						  vs.styles[StyleDefault].characterSet);
		CFStringRef cfsVal = CFStringFromString(text.c_str(), text.length(), encoding);

		return (__bridge NSString *)cfsVal;
	}
}

//--------------------------------------------------------------------------------------------------

// Return character range of a line.
NSRange ScintillaCocoa::RangeForVisibleLine(NSInteger lineVisible) {
	const Range posRangeLine = RangeDisplayLine(static_cast<Sci::Line>(lineVisible));
	return CharactersFromPositions(NSMakeRange(posRangeLine.First(),
				       posRangeLine.Last() - posRangeLine.First()));
}

//--------------------------------------------------------------------------------------------------

// Returns visible line number of a text position in characters.
NSInteger ScintillaCocoa::VisibleLineForIndex(NSInteger index) {
	const NSRange rangePosition = PositionsFromCharacters(NSMakeRange(index, 0));
	const Sci::Line lineVisible = DisplayFromPosition(rangePosition.location);
	return lineVisible;
}

//--------------------------------------------------------------------------------------------------

// Returns a rectangle that frames the range for use by the VoiceOver cursor.
NSRect ScintillaCocoa::FrameForRange(NSRange rangeCharacters) {
	const NSRange posRange = PositionsFromCharacters(rangeCharacters);

	NSUInteger rangeEnd = NSMaxRange(posRange);
	const bool endsWithLineEnd = rangeCharacters.length &&
				     (pdoc->GetColumn(rangeEnd) == 0);

	Point ptStart = LocationFromPosition(posRange.location);
	Point ptEnd = LocationFromPosition(rangeEnd, PointEnd::endEither);

	NSRect rect = NSMakeRect(ptStart.x, ptStart.y,
				 ptEnd.x - ptStart.x,
				 ptEnd.y - ptStart.y);

	rect.size.width += 2;	// Shows the last character better
	if (endsWithLineEnd) {
		// Add a block to the right to indicate a line end is selected
		rect.size.width += 20;
	}

	rect.size.height += vs.lineHeight;

	// Adjust for margin and scroll
	rect.origin.x = rect.origin.x - vs.textStart + vs.fixedColumnWidth;

	return rect;
}

//--------------------------------------------------------------------------------------------------

// Returns a rectangle that frames the range for use by the VoiceOver cursor.
NSRect ScintillaCocoa::GetBounds() const {
	return PRectangleToNSRect(GetClientRectangle());
}

//--------------------------------------------------------------------------------------------------

// Translates a UTF8 string into the document encoding.
// Return the length of the result in bytes.
Sci::Position ScintillaCocoa::EncodedFromUTF8(const char *utf8, char *encoded) const {
	const size_t inputLength = (lengthForEncode >= 0) ? lengthForEncode : strlen(utf8);
	if (IsUnicodeMode()) {
		if (encoded)
			memcpy(encoded, utf8, inputLength);
		return inputLength;
	} else {
		// Need to convert
		const CFStringEncoding encoding = EncodingFromCharacterSet(IsUnicodeMode(),
						  vs.styles[StyleDefault].characterSet);

		CFStringRef cfsVal = CFStringFromString(utf8, inputLength, kCFStringEncodingUTF8);
		const std::string sEncoded = EncodedBytesString(cfsVal, encoding);
		if (encoded)
			memcpy(encoded, sEncoded.c_str(), sEncoded.length());
		CFRelease(cfsVal);
		return sEncoded.length();
	}
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::SetMouseCapture(bool on) {
	capturedMouse = on;
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::HaveMouseCapture() {
	return capturedMouse;
}

//--------------------------------------------------------------------------------------------------

/**
 * Synchronously paint a rectangle of the window.
 */
bool ScintillaCocoa::SyncPaint(void *gc, PRectangle rc) {
	paintState = PaintState::painting;
	rcPaint = rc;
	PRectangle rcText = GetTextRectangle();
	paintingAllText = rcPaint.Contains(rcText);
	std::unique_ptr<Surface> sw(Surface::Allocate(Technology::Default));
	CGContextSetAllowsAntialiasing((CGContextRef)gc,
				       vs.extraFontFlag != FontQuality::QualityNonAntialiased);
	CGContextSetAllowsFontSmoothing((CGContextRef)gc,
					vs.extraFontFlag == FontQuality::QualityLcdOptimized);
	CGContextSetAllowsFontSubpixelPositioning((CGContextRef)gc,
			vs.extraFontFlag == FontQuality::QualityDefault ||
			vs.extraFontFlag == FontQuality::QualityLcdOptimized);
	sw->Init(gc, wMain.GetID());
	Paint(sw.get(), rc);
	const bool succeeded = paintState != PaintState::abandoned;
	sw->Release();
	paintState = PaintState::notPainting;
	if (!succeeded) {
		NSView *marginView = (__bridge NSView *)(wMargin.GetID());
		[marginView setNeedsDisplay: YES];
	}
	return succeeded;
}

//--------------------------------------------------------------------------------------------------

/**
 * Paint the margin into the SCIMarginView space.
 */
void ScintillaCocoa::PaintMargin(NSRect aRect) {
	CGContextRef gc = (CGContextRef) [NSGraphicsContext currentContext].graphicsPort;

	PRectangle rc = NSRectToPRectangle(aRect);
	rcPaint = rc;
	std::unique_ptr<Surface> sw(Surface::Allocate(Technology::Default));
	if (sw) {
		CGContextSetAllowsAntialiasing(gc,
					       vs.extraFontFlag != FontQuality::QualityNonAntialiased);
		CGContextSetAllowsFontSmoothing(gc,
						vs.extraFontFlag == FontQuality::QualityLcdOptimized);
		CGContextSetAllowsFontSubpixelPositioning(gc,
				vs.extraFontFlag == FontQuality::QualityDefault ||
				vs.extraFontFlag == FontQuality::QualityLcdOptimized);
		sw->Init(gc, wMargin.GetID());
		PaintSelMargin(sw.get(), rc);
		sw->Release();
	}
}

//--------------------------------------------------------------------------------------------------

/**
 * Prepare for drawing.
 *
 * @param rect The area that will be drawn, given in the sender's coordinate system.
 */
void ScintillaCocoa::WillDraw(NSRect rect) {
	RefreshStyleData();
	PRectangle rcWillDraw = NSRectToPRectangle(rect);
	const Sci::Position posAfterArea = PositionAfterArea(rcWillDraw);
	const Sci::Position posAfterMax = PositionAfterMaxStyling(posAfterArea, true);
	pdoc->StyleToAdjustingLineDuration(posAfterMax);
	StartIdleStyling(posAfterMax < posAfterArea);
	NotifyUpdateUI();
	if (WrapLines(WrapScope::wsVisible)) {
		// Wrap may have reduced number of lines so more lines may need to be styled
		const Sci::Position posAfterAreaWrapped = PositionAfterArea(rcWillDraw);
		pdoc->EnsureStyledTo(posAfterAreaWrapped);
		// The wrapping process has changed the height of some lines so redraw all.
		Redraw();
	}
}

//--------------------------------------------------------------------------------------------------

/**
 * ScrollText is empty because scrolling is handled by the NSScrollView.
 */
void ScintillaCocoa::ScrollText(Sci::Line) {
}

//--------------------------------------------------------------------------------------------------

/**
 * Modifies the vertical scroll position to make the current top line show up as such.
 */
void ScintillaCocoa::SetVerticalScrollPos() {
	NSScrollView *scrollView = ScrollContainer();
	if (scrollView) {
		NSClipView *clipView = scrollView.contentView;
		NSRect contentRect = clipView.bounds;
		[clipView scrollToPoint: NSMakePoint(contentRect.origin.x, topLine * vs.lineHeight)];
		[scrollView reflectScrolledClipView: clipView];
	}
}

//--------------------------------------------------------------------------------------------------

/**
 * Modifies the horizontal scroll position to match xOffset.
 */
void ScintillaCocoa::SetHorizontalScrollPos() {
	PRectangle textRect = GetTextRectangle();

	int maxXOffset = scrollWidth - static_cast<int>(textRect.Width());
	if (maxXOffset < 0)
		maxXOffset = 0;
	if (xOffset > maxXOffset)
		xOffset = maxXOffset;
	NSScrollView *scrollView = ScrollContainer();
	if (scrollView) {
		NSClipView *clipView = scrollView.contentView;
		NSRect contentRect = clipView.bounds;
		[clipView scrollToPoint: NSMakePoint(xOffset, contentRect.origin.y)];
		[scrollView reflectScrolledClipView: clipView];
	}
	MoveFindIndicatorWithBounce(NO);
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to adjust both scrollers to reflect the current scroll range and position in the editor.
 * Arguments no longer used as NSScrollView handles details of scroll bar sizes.
 *
 * @param nMax Number of lines in the editor.
 * @param nPage Number of lines per scroll page.
 * @return True if there was a change, otherwise false.
 */
bool ScintillaCocoa::ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) {
#pragma unused(nMax, nPage)
	return SetScrollingSize();
}

//--------------------------------------------------------------------------------------------------

/**
 * Adjust both scrollers to reflect the current scroll ranges and position in the editor.
 * Called when size changes or when scroll ranges change.
 */

bool ScintillaCocoa::SetScrollingSize() {
	bool changes = false;
	SCIContentView *inner = ContentView();
	if (!enteredSetScrollingSize) {
		enteredSetScrollingSize = true;
		NSScrollView *scrollView = ScrollContainer();
		const NSRect clipRect = scrollView.contentView.bounds;
		CGFloat docHeight = pcs->LinesDisplayed() * vs.lineHeight;
		if (!endAtLastLine)
			docHeight += (int(scrollView.bounds.size.height / vs.lineHeight)-3) * vs.lineHeight;
		// Allow extra space so that last scroll position places whole line at top
		const int clipExtra = int(clipRect.size.height) % vs.lineHeight;
		docHeight += clipExtra;
		// Ensure all of clipRect covered by Scintilla drawing
		if (docHeight < clipRect.size.height)
			docHeight = clipRect.size.height;
		const bool showHorizontalScroll = horizontalScrollBarVisible &&
					    !Wrapping();
		const CGFloat docWidth = Wrapping() ? clipRect.size.width : scrollWidth;
		const NSRect contentRect = NSMakeRect(0, 0, docWidth, docHeight);
		changes = !CGSizeEqualToSize(contentRect.size, inner.frame.size);
		if (changes) {
			inner.frame = contentRect;
		}
		scrollView.hasVerticalScroller = verticalScrollBarVisible;
		scrollView.hasHorizontalScroller = showHorizontalScroll;
		SetVerticalScrollPos();
		enteredSetScrollingSize = false;
	}
	[sciView setMarginWidth: vs.fixedColumnWidth];
	return changes;
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::Resize() {
	SetScrollingSize();
	ChangeSize();
}

//--------------------------------------------------------------------------------------------------

/**
 * Update fields to match scroll position after receiving a notification that the user has scrolled.
 */
void ScintillaCocoa::UpdateForScroll() {
	Point ptOrigin = GetVisibleOriginInMain();
	xOffset = static_cast<int>(ptOrigin.x);
	Sci::Line newTop = std::min(static_cast<Sci::Line>(ptOrigin.y / vs.lineHeight), MaxScrollPos());
	SetTopLine(newTop);
}

//--------------------------------------------------------------------------------------------------

/**
 * Register a delegate that will be called for notifications and commands.
 * This provides similar functionality to RegisterNotifyCallback but in an
 * Objective C way.
 *
 * @param delegate_ A pointer to an object that implements ScintillaNotificationProtocol.
 */

void ScintillaCocoa::SetDelegate(id<ScintillaNotificationProtocol> delegate_) {
	delegate = delegate_;
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to register a callback function for a given window. This is used to emulate the way
 * Windows notifies other controls (mainly up in the view hierarchy) about certain events.
 *
 * @param windowid A handle to a window. That value is generic and can be anything. It is passed
 *                 through to the callback.
 * @param callback The callback function to be used for future notifications. If NULL then no
 *                 notifications will be sent anymore.
 */
void ScintillaCocoa::RegisterNotifyCallback(intptr_t windowid, SciNotifyFunc callback) {
	notifyObj = windowid;
	notifyProc = callback;
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::NotifyChange() {
	if (notifyProc != NULL)
		notifyProc(notifyObj, WM_COMMAND, Platform::LongFromTwoShorts(static_cast<short>(GetCtrlID()), static_cast<short>(FocusChange::Change)),
			   (uintptr_t) this);
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::NotifyFocus(bool focus) {
	if (commandEvents && notifyProc)
		notifyProc(notifyObj, WM_COMMAND, Platform::LongFromTwoShorts(static_cast<short>(GetCtrlID()),
				static_cast<short>((focus ? FocusChange::Setfocus : FocusChange::Killfocus))),
			   (uintptr_t) this);

	Editor::NotifyFocus(focus);
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to send a notification (as WM_NOTIFY call) to the procedure, which has been set by the call
 * to RegisterNotifyCallback (so it is not necessarily the parent window).
 *
 * @param scn The notification to send.
 */
void ScintillaCocoa::NotifyParent(NotificationData scn) {
	scn.nmhdr.hwndFrom = (void *) this;
	scn.nmhdr.idFrom = GetCtrlID();
	if (notifyProc != NULL)
		notifyProc(notifyObj, WM_NOTIFY, GetCtrlID(), (uintptr_t) &scn);
	if (delegate)
		[delegate notification: reinterpret_cast<SCNotification *>(&scn)];
	if (scn.nmhdr.code == Notification::UpdateUI) {
		NSView *content = ContentView();
		if (FlagSet(scn.updated, Update::Content)) {
			NSAccessibilityPostNotification(content, NSAccessibilityValueChangedNotification);
		}
		if (FlagSet(scn.updated, Update::Selection)) {
			NSAccessibilityPostNotification(content, NSAccessibilitySelectedTextChangedNotification);
		}
	}
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::NotifyURIDropped(const char *uri) {
	NotificationData scn;
	scn.nmhdr.code = Notification::URIDropped;
	scn.text = uri;

	NotifyParent(scn);
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::HasSelection() {
	return !sel.Empty();
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::CanUndo() {
	return pdoc->CanUndo();
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::CanRedo() {
	return pdoc->CanRedo();
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::TimerFired(NSTimer *timer) {
	for (size_t tr=static_cast<size_t>(TickReason::caret); tr<=static_cast<size_t>(TickReason::platform); tr++) {
		if (timers[tr] == timer) {
			TickFor(static_cast<TickReason>(tr));
		}
	}
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::IdleTimerFired() {
	bool more = Idle();
	if (!more)
		SetIdle(false);
}

//--------------------------------------------------------------------------------------------------

/**
 * Main entry point for drawing the control.
 *
 * @param rect The area to paint, given in the sender's coordinate system.
 * @param gc The context we can use to paint.
 */
bool ScintillaCocoa::Draw(NSRect rect, CGContextRef gc) {
	return SyncPaint(gc, NSRectToPRectangle(rect));
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper function to translate macOS key codes to Scintilla key codes.
 */
static inline Keys KeyTranslate(UniChar unicodeChar, NSEventModifierFlags modifierFlags) {
	switch (unicodeChar) {
	case NSDownArrowFunctionKey:
		return Keys::Down;
	case NSUpArrowFunctionKey:
		return Keys::Up;
	case NSLeftArrowFunctionKey:
		return Keys::Left;
	case NSRightArrowFunctionKey:
		return Keys::Right;
	case NSHomeFunctionKey:
		return Keys::Home;
	case NSEndFunctionKey:
		return Keys::End;
	case NSPageUpFunctionKey:
		return Keys::Prior;
	case NSPageDownFunctionKey:
		return Keys::Next;
	case NSDeleteFunctionKey:
		return Keys::Delete;
	case NSInsertFunctionKey:
		return Keys::Insert;
	case '\n':
	case 3:
		return Keys::Return;
	case 27:
		return Keys::Escape;
	case '+':
		if (modifierFlags & NSEventModifierFlagNumericPad)
			return Keys::Add;
		else
			return static_cast<Keys>(unicodeChar);
	case '-':
		if (modifierFlags & NSEventModifierFlagNumericPad)
			return Keys::Subtract;
		else
			return static_cast<Keys>(unicodeChar);
	case '/':
		if (modifierFlags & NSEventModifierFlagNumericPad)
			return Keys::Divide;
		else
			return static_cast<Keys>(unicodeChar);
	case 127:
		return Keys::Back;
	case '\t':
	case 25: // Shift tab, return to unmodified tab and handle that via modifiers.
		return Keys::Tab;
	default:
		return static_cast<Keys>(unicodeChar);
	}
}

//--------------------------------------------------------------------------------------------------

/**
 * Translate NSEvent modifier flags into SCI_* modifier flags.
 *
 * @param modifiers An integer bit set of NSSEvent modifier flags.
 * @return A set of SCI_* modifier flags.
 */
static KeyMod TranslateModifierFlags(NSUInteger modifiers) {
	// Signal Control as SCI_META
	return
		(((modifiers & NSEventModifierFlagShift) != 0) ? KeyMod::Shift : KeyMod::Norm) |
		(((modifiers & NSEventModifierFlagCommand) != 0) ? KeyMod::Ctrl : KeyMod::Norm) |
		(((modifiers & NSEventModifierFlagOption) != 0) ? KeyMod::Alt : KeyMod::Norm) |
		(((modifiers & NSEventModifierFlagControl) != 0) ? KeyMod::Meta : KeyMod::Norm);
}

//--------------------------------------------------------------------------------------------------

/**
 * Main keyboard input handling method. It is called for any key down event, including function keys,
 * numeric keypad input and whatnot.
 *
 * @param event The event instance associated with the key down event.
 * @return True if the input was handled, false otherwise.
 */
bool ScintillaCocoa::KeyboardInput(NSEvent *event) {
	// For now filter out function keys.
	NSString *input = event.charactersIgnoringModifiers;

	bool handled = false;

	// Handle each entry individually. Usually we only have one entry anyway.
	for (size_t i = 0; i < input.length; i++) {
		const UniChar originalKey = [input characterAtIndex: i];
		NSEventModifierFlags modifierFlags = event.modifierFlags;

		Keys key = KeyTranslate(originalKey, modifierFlags);

		bool consumed = false; // Consumed as command?

		if (KeyDownWithModifiers(key, TranslateModifierFlags(modifierFlags), &consumed))
			handled = true;
		if (consumed)
			handled = true;
	}

	return handled;
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to insert already processed text provided by the Cocoa text input system.
 */
ptrdiff_t ScintillaCocoa::InsertText(NSString *input, CharacterSource charSource) {
	if ([input length] == 0) {
		return 0;
	}

	// There may be multiple characters in input so loop over them
	if (IsUnicodeMode()) {
		// There may be non-BMP characters as 2 elements in NSString so
		// convert to UTF-8 and use UTF8BytesOfLead.
		std::string encoded = EncodedBytesString((__bridge CFStringRef)input,
							 kCFStringEncodingUTF8);
		std::string_view sv = encoded;
		while (sv.length()) {
			const unsigned char leadByte = sv[0];
			const unsigned int bytesInCharacter = UTF8BytesOfLead[leadByte];
			InsertCharacter(sv.substr(0, bytesInCharacter), charSource);
			sv.remove_prefix(bytesInCharacter);
		}
		return encoded.length();
	} else {
		const CFStringEncoding encoding = EncodingFromCharacterSet(IsUnicodeMode(),
									   vs.styles[StyleDefault].characterSet);
		ptrdiff_t lengthInserted = 0;
		for (NSInteger i = 0; i < [input length]; i++) {
			NSString *character = [input substringWithRange:NSMakeRange(i, 1)];
			std::string encoded = EncodedBytesString((__bridge CFStringRef)character,
								 encoding);
			lengthInserted += encoded.length();
			InsertCharacter(encoded, charSource);
		}

		return lengthInserted;
	}
}

//--------------------------------------------------------------------------------------------------

/**
 * Convert from a range of characters to a range of bytes.
 */
NSRange ScintillaCocoa::PositionsFromCharacters(NSRange rangeCharacters) const {
	Sci::Position start = pdoc->GetRelativePositionUTF16(0, rangeCharacters.location);
	if (start == Sci::invalidPosition)
		start = pdoc->Length();
	Sci::Position end = pdoc->GetRelativePositionUTF16(start, rangeCharacters.length);
	if (end == Sci::invalidPosition)
		end = pdoc->Length();
	return NSMakeRange(start, end - start);
}

//--------------------------------------------------------------------------------------------------

/**
 * Convert from a range of characters from a range of bytes.
 */
NSRange ScintillaCocoa::CharactersFromPositions(NSRange rangePositions) const {
	const Sci::Position start = pdoc->CountUTF16(0, rangePositions.location);
	const Sci::Position len = pdoc->CountUTF16(rangePositions.location,
					  NSMaxRange(rangePositions));
	return NSMakeRange(start, len);
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to ensure that only one selection is active for input composition as composition
 * does not support multi-typing.
 */
void ScintillaCocoa::SelectOnlyMainSelection() {
	sel.SetSelection(sel.RangeMain());
	Redraw();
}

//--------------------------------------------------------------------------------------------------

/**
 * Convert virtual space before selection into real space.
 */
void ScintillaCocoa::ConvertSelectionVirtualSpace() {
	ClearBeforeTentativeStart();
}

//--------------------------------------------------------------------------------------------------

/**
 * Erase all selected text and return whether the selection is now empty.
 * The selection may not be empty if the selection contained protected text.
 */
bool ScintillaCocoa::ClearAllSelections() {
	ClearSelection(true);
	return sel.Empty();
}

//--------------------------------------------------------------------------------------------------

/**
 * Start composing for IME.
 */
void ScintillaCocoa::CompositionStart() {
	if (!sel.Empty()) {
		NSLog(@"Selection not empty when starting composition");
	}
	pdoc->TentativeStart();
}

//--------------------------------------------------------------------------------------------------

/**
 * Commit the IME text.
 */
void ScintillaCocoa::CompositionCommit() {
	pdoc->TentativeCommit();
	pdoc->DecorationSetCurrentIndicator(static_cast<int>(IndicatorNumbers::Ime));
	pdoc->DecorationFillRange(0, 0, pdoc->Length());
}

//--------------------------------------------------------------------------------------------------

/**
 * Remove the IME text.
 */
void ScintillaCocoa::CompositionUndo() {
	pdoc->TentativeUndo();
}

//--------------------------------------------------------------------------------------------------
/**
 * When switching documents discard any incomplete character composition state as otherwise tries to
 * act on the new document.
 */
void ScintillaCocoa::SetDocPointer(Document *document) {
	// Drop input composition.
	NSTextInputContext *inctxt = [NSTextInputContext currentInputContext];
	[inctxt discardMarkedText];
	SCIContentView *inner = ContentView();
	[inner unmarkText];
	Editor::SetDocPointer(document);
}

//--------------------------------------------------------------------------------------------------

/**
 * Convert NSEvent timestamp NSTimeInterval into unsigned int milliseconds wanted by Editor methods.
 */

namespace {

unsigned int TimeOfEvent(NSEvent *event) {
	return static_cast<unsigned int>(event.timestamp * 1000);
}

}

//--------------------------------------------------------------------------------------------------

/**
 * Called by the owning view when the mouse pointer enters the control.
 */
void ScintillaCocoa::MouseEntered(NSEvent *event) {
	if (!HaveMouseCapture()) {
		WndProc(Message::SetCursor, (long int)CursorShape::Normal, 0);

		// Mouse location is given in screen coordinates and might also be outside of our bounds.
		Point location = ConvertPoint(event.locationInWindow);
		ButtonMoveWithModifiers(location,
					TimeOfEvent(event),
					TranslateModifierFlags(event.modifierFlags));
	}
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::MouseExited(NSEvent * /* event */) {
	// Nothing to do here.
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::MouseDown(NSEvent *event) {
	Point location = ConvertPoint(event.locationInWindow);
	ButtonDownWithModifiers(location,
				TimeOfEvent(event),
				TranslateModifierFlags(event.modifierFlags));
}

void ScintillaCocoa::RightMouseDown(NSEvent *event) {
	Point location = ConvertPoint(event.locationInWindow);
	RightButtonDownWithModifiers(location,
				     TimeOfEvent(event),
				     TranslateModifierFlags(event.modifierFlags));
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::MouseMove(NSEvent *event) {
	lastMouseEvent = event;

	ButtonMoveWithModifiers(ConvertPoint(event.locationInWindow),
				TimeOfEvent(event),
				TranslateModifierFlags(event.modifierFlags));
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::MouseUp(NSEvent *event) {
	ButtonUpWithModifiers(ConvertPoint(event.locationInWindow),
		 TimeOfEvent(event),
		 TranslateModifierFlags(event.modifierFlags));
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::MouseWheel(NSEvent *event) {
	bool command = (event.modifierFlags & NSEventModifierFlagCommand) != 0;
	int dY = 0;

	// In order to make scrolling with larger offset smoother we scroll less lines the larger the
	// delta value is.
	if (event.deltaY < 0)
		dY = -static_cast<int>(sqrt(-10.0 * event.deltaY));
	else
		dY = static_cast<int>(sqrt(10.0 * event.deltaY));

	if (command) {
		// Zoom! We play with the font sizes in the styles.
		// Number of steps/line is ignored, we just care if sizing up or down.
		if (dY > 0.5)
			KeyCommand(Message::ZoomIn);
		else if (dY < -0.5)
			KeyCommand(Message::ZoomOut);
	} else {
	}
}

//--------------------------------------------------------------------------------------------------

// Helper methods for NSResponder actions.

void ScintillaCocoa::SelectAll() {
	Editor::SelectAll();
}

void ScintillaCocoa::DeleteBackward() {
	KeyDownWithModifiers(Keys::Back, KeyMod::Norm, nil);
}

void ScintillaCocoa::Cut() {
	Editor::Cut();
}

void ScintillaCocoa::Undo() {
	Editor::Undo();
}

void ScintillaCocoa::Redo() {
	Editor::Redo();
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::ShouldDisplayPopupOnMargin() {
	return displayPopupMenu == PopUp::All;
}

bool ScintillaCocoa::ShouldDisplayPopupOnText() {
	return displayPopupMenu == PopUp::All || displayPopupMenu == PopUp::Text;
}

/**
 * Creates and returns a popup menu, which is then displayed by the Cocoa framework.
 */
NSMenu *ScintillaCocoa::CreateContextMenu(NSEvent * /* event */) {
	// Call ScintillaBase to create the context menu.
	ContextMenu(Point(0, 0));

	return (__bridge NSMenu *)(popup.GetID());
}

//--------------------------------------------------------------------------------------------------

/**
 * An intermediate function to forward context menu commands from the menu action handler to
 * scintilla.
 */
void ScintillaCocoa::HandleCommand(NSInteger command) {
	Command(static_cast<int>(command));
}

//--------------------------------------------------------------------------------------------------

/**
 * Update 'isFirstResponder' and possibly change focus state.
 */
void ScintillaCocoa::SetFirstResponder(bool isFirstResponder_) {
	isFirstResponder = isFirstResponder_;
	SetFocusActiveState();
}

//--------------------------------------------------------------------------------------------------

/**
 * Update 'isActive' and possibly change focus state.
 */
void ScintillaCocoa::ActiveStateChanged(bool isActive_) {
	isActive = isActive_;
	SetFocusActiveState();
}

//--------------------------------------------------------------------------------------------------

/**
 * Update 'hasFocus' based on first responder and active status.
 */
void ScintillaCocoa::SetFocusActiveState() {
	SetFocusState(isActive && isFirstResponder);
}

//--------------------------------------------------------------------------------------------------

namespace {

/**
 * Convert from an NSColor into a ColourRGBA
 */
ColourRGBA ColourFromNSColor(NSColor *value) {
	return ColourRGBA(static_cast<unsigned int>(value.redComponent * componentMaximum),
			   static_cast<unsigned int>(value.greenComponent * componentMaximum),
			   static_cast<unsigned int>(value.blueComponent * componentMaximum),
			   static_cast<unsigned int>(value.alphaComponent * componentMaximum));
}

}

//--------------------------------------------------------------------------------------------------

/**
 * Update ViewStyle::elementBaseColours to match system preferences.
 */
void ScintillaCocoa::UpdateBaseElements() {
	NSView *content = ContentView();
	NSAppearance *saved = [NSAppearance currentAppearance];
	[NSAppearance setCurrentAppearance:content.effectiveAppearance];

	bool changed = false;
	if (@available(macOS 10.14, *)) {
		NSColor *textBack = [NSColor.textBackgroundColor colorUsingColorSpaceName: NSCalibratedRGBColorSpace];
		NSColor *noFocusBack = [NSColor.unemphasizedSelectedTextBackgroundColor colorUsingColorSpaceName: NSCalibratedRGBColorSpace];
		if (vs.selection.layer == Layer::Base) {
			NSColor *selBack = [NSColor.selectedTextBackgroundColor colorUsingColorSpaceName: NSCalibratedRGBColorSpace];
			// Additional selection: blend with text background to make weaker version.
			NSColor *modified = [selBack blendedColorWithFraction:0.5 ofColor:textBack];
			changed = vs.SetElementBase(Element::SelectionBack, ColourFromNSColor(selBack));
			changed = vs.SetElementBase(Element::SelectionAdditionalBack, ColourFromNSColor(modified)) || changed;
			changed = vs.SetElementBase(Element::SelectionInactiveBack, ColourFromNSColor(noFocusBack)) || changed;
		} else {
			// Less translucent colour used in dark mode as otherwise less visible
			const int alpha = textBack.brightnessComponent > 0.5 ? 0x40 : 0x60;
			// Make a translucent colour that approximates selectedTextBackgroundColor
			NSColor *accent = [NSColor.controlAccentColor colorUsingColorSpaceName: NSCalibratedRGBColorSpace];
			const ColourRGBA colourAccent = ColourFromNSColor(accent);
			changed = vs.SetElementBase(Element::SelectionBack, ColourRGBA(colourAccent, alpha));
			changed = vs.SetElementBase(Element::SelectionAdditionalBack, ColourRGBA(colourAccent, alpha/2)) || changed;
			changed = vs.SetElementBase(Element::SelectionInactiveBack, ColourRGBA(ColourFromNSColor(noFocusBack), alpha)) || changed;

		}
	}
	if (changed) {
		Redraw();
	}

	[NSAppearance setCurrentAppearance:saved];
}

//--------------------------------------------------------------------------------------------------

/**
 * When the window is about to move, the calltip and autcoimpletion stay in the same spot,
 * so cancel them.
 */
void ScintillaCocoa::WindowWillMove() {
	AutoCompleteCancel();
	ct.CallTipCancel();
}

// If building with old SDK, need to define version number for 10.8
#ifndef NSAppKitVersionNumber10_8
#define NSAppKitVersionNumber10_8 1187
#endif

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::ShowFindIndicatorForRange(NSRange charRange, BOOL retaining) {
#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_5
	NSView *content = ContentView();
	if (!layerFindIndicator) {
		layerFindIndicator = [[FindHighlightLayer alloc] init];
		[content setWantsLayer: YES];
		layerFindIndicator.geometryFlipped = content.layer.geometryFlipped;
		if (std::floor(NSAppKitVersionNumber) > NSAppKitVersionNumber10_8) {
			// Content layer is unflipped on 10.9, but the indicator shows wrong unless flipped
			layerFindIndicator.geometryFlipped = YES;
		}
		[content.layer addSublayer: layerFindIndicator];
	}
	[layerFindIndicator removeAnimationForKey: @"animateFound"];

	if (charRange.length) {
		CFStringEncoding encoding = EncodingFromCharacterSet(IsUnicodeMode(),
					    vs.styles[StyleDefault].characterSet);
		std::vector<char> buffer(charRange.length);
		pdoc->GetCharRange(&buffer[0], charRange.location, charRange.length);

		CFStringRef cfsFind = CFStringFromString(&buffer[0], charRange.length, encoding);
		layerFindIndicator.sFind = (__bridge NSString *)cfsFind;
		if (cfsFind)
			CFRelease(cfsFind);
		layerFindIndicator.retaining = retaining;
		layerFindIndicator.positionFind = charRange.location;
		// Message::GetStyleAt reports a signed byte but want an unsigned to index into styles
		const char styleByte = static_cast<char>(WndProc(Message::GetStyleAt, charRange.location, 0));
		const long style = static_cast<unsigned char>(styleByte);
		std::vector<char> bufferFontName(WndProc(Message::StyleGetFont, style, 0) + 1);
		WndProc(Message::StyleGetFont, style, (sptr_t)&bufferFontName[0]);
		layerFindIndicator.sFont = @(&bufferFontName[0]);

		layerFindIndicator.fontSize = WndProc(Message::StyleGetSizeFractional, style, 0) /
					      (float)FontSizeMultiplier;
		layerFindIndicator.widthText = WndProc(Message::PointXFromPosition, 0, charRange.location + charRange.length) -
					       WndProc(Message::PointXFromPosition, 0, charRange.location);
		layerFindIndicator.heightLine = WndProc(Message::TextHeight, 0, 0);
		MoveFindIndicatorWithBounce(YES);
	} else {
		[layerFindIndicator hideMatch];
	}
#endif
}

void ScintillaCocoa::MoveFindIndicatorWithBounce(BOOL bounce) {
#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_5
	if (layerFindIndicator) {
		CGPoint ptText = CGPointMake(
					 WndProc(Message::PointXFromPosition, 0, layerFindIndicator.positionFind),
					 WndProc(Message::PointYFromPosition, 0, layerFindIndicator.positionFind));
		ptText.x = ptText.x - vs.fixedColumnWidth + xOffset;
		ptText.y += topLine * vs.lineHeight;
		if (!layerFindIndicator.geometryFlipped) {
			NSView *content = ContentView();
			ptText.y = content.bounds.size.height - ptText.y;
		}
		[layerFindIndicator animateMatch: ptText bounce: bounce];
	}
#endif
}

void ScintillaCocoa::HideFindIndicator() {
#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_5
	if (layerFindIndicator) {
		[layerFindIndicator hideMatch];
	}
#endif
}


