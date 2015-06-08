
/**
 * Scintilla source code edit control
 * ScintillaCocoa.mm - Cocoa subclass of ScintillaBase
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

#import <Cocoa/Cocoa.h>
#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_5
#import <QuartzCore/CAGradientLayer.h>
#endif
#import <QuartzCore/CAAnimation.h>
#import <QuartzCore/CATransaction.h>

#import "Platform.h"
#import "ScintillaView.h"
#import "ScintillaCocoa.h"
#import "PlatCocoa.h"

using namespace Scintilla;

NSString* ScintillaRecPboardType = @"com.scintilla.utf16-plain-text.rectangular";

//--------------------------------------------------------------------------------------------------

// Define keyboard shortcuts (equivalents) the Mac way.
#define SCI_CMD ( SCI_CTRL)
#define SCI_SCMD ( SCI_CMD | SCI_SHIFT)
#define SCI_SMETA ( SCI_META | SCI_SHIFT)

static const KeyToCommand macMapDefault[] =
{
  // OS X specific
  {SCK_DOWN,      SCI_CTRL,   SCI_DOCUMENTEND},
  {SCK_DOWN,      SCI_CSHIFT, SCI_DOCUMENTENDEXTEND},
  {SCK_UP,        SCI_CTRL,   SCI_DOCUMENTSTART},
  {SCK_UP,        SCI_CSHIFT, SCI_DOCUMENTSTARTEXTEND},
  {SCK_LEFT,      SCI_CTRL,   SCI_VCHOME},
  {SCK_LEFT,      SCI_CSHIFT, SCI_VCHOMEEXTEND},
  {SCK_RIGHT,     SCI_CTRL,   SCI_LINEEND},
  {SCK_RIGHT,     SCI_CSHIFT, SCI_LINEENDEXTEND},

  // Similar to Windows and GTK+
  // Where equivalent clashes with OS X standard, use Meta instead
  {SCK_DOWN,      SCI_NORM,   SCI_LINEDOWN},
  {SCK_DOWN,      SCI_SHIFT,  SCI_LINEDOWNEXTEND},
  {SCK_DOWN,      SCI_META,   SCI_LINESCROLLDOWN},
  {SCK_DOWN,      SCI_ASHIFT, SCI_LINEDOWNRECTEXTEND},
  {SCK_UP,        SCI_NORM,   SCI_LINEUP},
  {SCK_UP,        SCI_SHIFT,  SCI_LINEUPEXTEND},
  {SCK_UP,        SCI_META,   SCI_LINESCROLLUP},
  {SCK_UP,        SCI_ASHIFT, SCI_LINEUPRECTEXTEND},
  {'[',           SCI_CTRL,   SCI_PARAUP},
  {'[',           SCI_CSHIFT, SCI_PARAUPEXTEND},
  {']',           SCI_CTRL,   SCI_PARADOWN},
  {']',           SCI_CSHIFT, SCI_PARADOWNEXTEND},
  {SCK_LEFT,      SCI_NORM,   SCI_CHARLEFT},
  {SCK_LEFT,      SCI_SHIFT,  SCI_CHARLEFTEXTEND},
  {SCK_LEFT,      SCI_ALT,    SCI_WORDLEFT},
  {SCK_LEFT,      SCI_META,   SCI_WORDLEFT},
  {SCK_LEFT,      SCI_SMETA,  SCI_WORDLEFTEXTEND},
  {SCK_LEFT,      SCI_ASHIFT, SCI_CHARLEFTRECTEXTEND},
  {SCK_RIGHT,     SCI_NORM,   SCI_CHARRIGHT},
  {SCK_RIGHT,     SCI_SHIFT,  SCI_CHARRIGHTEXTEND},
  {SCK_RIGHT,     SCI_ALT,    SCI_WORDRIGHT},
  {SCK_RIGHT,     SCI_META,   SCI_WORDRIGHT},
  {SCK_RIGHT,     SCI_SMETA,  SCI_WORDRIGHTEXTEND},
  {SCK_RIGHT,     SCI_ASHIFT, SCI_CHARRIGHTRECTEXTEND},
  {'/',           SCI_CTRL,   SCI_WORDPARTLEFT},
  {'/',           SCI_CSHIFT, SCI_WORDPARTLEFTEXTEND},
  {'\\',          SCI_CTRL,   SCI_WORDPARTRIGHT},
  {'\\',          SCI_CSHIFT, SCI_WORDPARTRIGHTEXTEND},
  {SCK_HOME,      SCI_NORM,   SCI_VCHOME},
  {SCK_HOME,      SCI_SHIFT,  SCI_VCHOMEEXTEND},
  {SCK_HOME,      SCI_CTRL,   SCI_DOCUMENTSTART},
  {SCK_HOME,      SCI_CSHIFT, SCI_DOCUMENTSTARTEXTEND},
  {SCK_HOME,      SCI_ALT,    SCI_HOMEDISPLAY},
  {SCK_HOME,      SCI_ASHIFT, SCI_VCHOMERECTEXTEND},
  {SCK_END,       SCI_NORM,   SCI_LINEEND},
  {SCK_END,       SCI_SHIFT,  SCI_LINEENDEXTEND},
  {SCK_END,       SCI_CTRL,   SCI_DOCUMENTEND},
  {SCK_END,       SCI_CSHIFT, SCI_DOCUMENTENDEXTEND},
  {SCK_END,       SCI_ALT,    SCI_LINEENDDISPLAY},
  {SCK_END,       SCI_ASHIFT, SCI_LINEENDRECTEXTEND},
  {SCK_PRIOR,     SCI_NORM,   SCI_PAGEUP},
  {SCK_PRIOR,     SCI_SHIFT,  SCI_PAGEUPEXTEND},
  {SCK_PRIOR,     SCI_ASHIFT, SCI_PAGEUPRECTEXTEND},
  {SCK_NEXT,      SCI_NORM,   SCI_PAGEDOWN},
  {SCK_NEXT,      SCI_SHIFT,  SCI_PAGEDOWNEXTEND},
  {SCK_NEXT,      SCI_ASHIFT, SCI_PAGEDOWNRECTEXTEND},
  {SCK_DELETE,    SCI_NORM,   SCI_CLEAR},
  {SCK_DELETE,    SCI_SHIFT,  SCI_CUT},
  {SCK_DELETE,    SCI_CTRL,   SCI_DELWORDRIGHT},
  {SCK_DELETE,    SCI_CSHIFT, SCI_DELLINERIGHT},
  {SCK_INSERT,    SCI_NORM,   SCI_EDITTOGGLEOVERTYPE},
  {SCK_INSERT,    SCI_SHIFT,  SCI_PASTE},
  {SCK_INSERT,    SCI_CTRL,   SCI_COPY},
  {SCK_ESCAPE,    SCI_NORM,   SCI_CANCEL},
  {SCK_BACK,      SCI_NORM,   SCI_DELETEBACK},
  {SCK_BACK,      SCI_SHIFT,  SCI_DELETEBACK},
  {SCK_BACK,      SCI_CTRL,   SCI_DELWORDLEFT},
  {SCK_BACK,      SCI_ALT,    SCI_DELWORDLEFT},
  {SCK_BACK,      SCI_CSHIFT, SCI_DELLINELEFT},
  {'z',           SCI_CMD,    SCI_UNDO},
  {'z',           SCI_SCMD,   SCI_REDO},
  {'x',           SCI_CMD,    SCI_CUT},
  {'c',           SCI_CMD,    SCI_COPY},
  {'v',           SCI_CMD,    SCI_PASTE},
  {'a',           SCI_CMD,    SCI_SELECTALL},
  {SCK_TAB,       SCI_NORM,   SCI_TAB},
  {SCK_TAB,       SCI_SHIFT,  SCI_BACKTAB},
  {SCK_RETURN,    SCI_NORM,   SCI_NEWLINE},
  {SCK_RETURN,    SCI_SHIFT,  SCI_NEWLINE},
  {SCK_ADD,       SCI_CMD,    SCI_ZOOMIN},
  {SCK_SUBTRACT,  SCI_CMD,    SCI_ZOOMOUT},
  {SCK_DIVIDE,    SCI_CMD,    SCI_SETZOOM},
  {'l',           SCI_CMD,    SCI_LINECUT},
  {'l',           SCI_CSHIFT, SCI_LINEDELETE},
  {'t',           SCI_CSHIFT, SCI_LINECOPY},
  {'t',           SCI_CTRL,   SCI_LINETRANSPOSE},
  {'d',           SCI_CTRL,   SCI_SELECTIONDUPLICATE},
  {'u',           SCI_CTRL,   SCI_LOWERCASE},
  {'u',           SCI_CSHIFT, SCI_UPPERCASE},
  {0, 0, 0},
};

//--------------------------------------------------------------------------------------------------

#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_5

// Only implement FindHighlightLayer on OS X 10.6+

/**
 * Class to display the animated gold roundrect used on OS X for matches.
 */
@interface FindHighlightLayer : CAGradientLayer
{
@private
	NSString *sFind;
	int positionFind;
	BOOL retaining;
	CGFloat widthText;
	CGFloat heightLine;
	NSString *sFont;
	CGFloat fontSize;
}

@property (copy) NSString *sFind;
@property (assign) int positionFind;
@property (assign) BOOL retaining;
@property (assign) CGFloat widthText;
@property (assign) CGFloat heightLine;
@property (copy) NSString *sFont;
@property (assign) CGFloat fontSize;

- (void) animateMatch: (CGPoint)ptText bounce:(BOOL)bounce;
- (void) hideMatch;

@end

//--------------------------------------------------------------------------------------------------

@implementation FindHighlightLayer

@synthesize sFind, positionFind, retaining, widthText, heightLine, sFont, fontSize;

-(id) init {
	if (self = [super init]) {
		[self setNeedsDisplayOnBoundsChange: YES];
		// A gold to slightly redder gradient to match other applications
		CGColorRef colGold = CGColorCreateGenericRGB(1.0, 1.0, 0, 1.0);
		CGColorRef colGoldRed = CGColorCreateGenericRGB(1.0, 0.8, 0, 1.0);
		self.colors = [NSArray arrayWithObjects:(id)colGoldRed, (id)colGold, nil];
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

-(void) drawInContext:(CGContextRef)context {
	if (!sFind || !sFont)
		return;

	CFStringRef str = CFStringRef(sFind);

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

- (void) animateMatch: (CGPoint)ptText bounce:(BOOL)bounce {
	if (!self.sFind || ![self.sFind length]) {
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
	[CATransaction setValue:[NSNumber numberWithFloat:0.0] forKey:kCATransactionAnimationDuration];
	self.bounds = CGRectMake(0,0, width, height);
	self.position = ptText;
	if (bounce) {
		// Do not reset visibility when just moving
		self.hidden = NO;
		self.opacity = 1.0;
	}
	[self setNeedsDisplay];
	[CATransaction commit];

	if (bounce) {
		CABasicAnimation *animBounce = [CABasicAnimation animationWithKeyPath:@"transform.scale"];
		animBounce.duration = 0.15;
		animBounce.autoreverses = YES;
		animBounce.removedOnCompletion = NO;
		animBounce.fromValue = [NSNumber numberWithFloat: 1.0];
		animBounce.toValue = [NSNumber numberWithFloat: 1.25];

		if (self.retaining) {

			[self addAnimation: animBounce forKey:@"animateFound"];

		} else {

			CABasicAnimation *animFade = [CABasicAnimation animationWithKeyPath:@"opacity"];
			animFade.duration = 0.1;
			animFade.beginTime = 0.4;
			animFade.removedOnCompletion = NO;
			animFade.fromValue = [NSNumber numberWithFloat: 1.0];
			animFade.toValue = [NSNumber numberWithFloat: 0.0];

			CAAnimationGroup *group = [CAAnimationGroup animation];
			[group setDuration:0.5];
			group.removedOnCompletion = NO;
			group.fillMode = kCAFillModeForwards;
			[group setAnimations:[NSArray arrayWithObjects:animBounce, animFade, nil]];

			[self addAnimation:group forKey:@"animateFound"];
		}
	}
}

- (void) hideMatch {
	self.sFind = @"";
	self.positionFind = INVALID_POSITION;
	self.hidden = YES;
}

@end

#endif

//--------------------------------------------------------------------------------------------------

@implementation TimerTarget

- (id) init: (void*) target
{
  self = [super init];
  if (self != nil)
  {
    mTarget = target;

    // Get the default notification queue for the thread which created the instance (usually the
    // main thread). We need that later for idle event processing.
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    notificationQueue = [[NSNotificationQueue alloc] initWithNotificationCenter: center];
    [center addObserver: self selector: @selector(idleTriggered:) name: @"Idle" object: nil];
  }
  return self;
}

//--------------------------------------------------------------------------------------------------

- (void) dealloc
{
  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  [center removeObserver:self];
  [notificationQueue release];
  [super dealloc];
}

//--------------------------------------------------------------------------------------------------

/**
 * Method called by a timer installed by ScintillaCocoa. This two step approach is needed because
 * a native Obj-C class is required as target for the timer.
 */
- (void) timerFired: (NSTimer*) timer
{
  reinterpret_cast<ScintillaCocoa*>(mTarget)->TimerFired(timer);
}

//--------------------------------------------------------------------------------------------------

/**
 * Another timer callback for the idle timer.
 */
- (void) idleTimerFired: (NSTimer*) timer
{
#pragma unused(timer)
  // Idle timer event.
  // Post a new idle notification, which gets executed when the run loop is idle.
  // Since we are coalescing on name and sender there will always be only one actual notification
  // even for multiple requests.
  NSNotification *notification = [NSNotification notificationWithName: @"Idle" object: self];
  [notificationQueue enqueueNotification: notification
                            postingStyle: NSPostWhenIdle
                            coalesceMask: (NSNotificationCoalescingOnName | NSNotificationCoalescingOnSender)
                                forModes: nil];
}

//--------------------------------------------------------------------------------------------------

/**
 * Another step for idle events. The timer (for idle events) simply requests a notification on
 * idle time. Only when this notification is send we actually call back the editor.
 */
- (void) idleTriggered: (NSNotification*) notification
{
#pragma unused(notification)
  reinterpret_cast<ScintillaCocoa*>(mTarget)->IdleTimerFired();
}

@end

//----------------- ScintillaCocoa -----------------------------------------------------------------

ScintillaCocoa::ScintillaCocoa(SCIContentView* view, SCIMarginView* viewMargin)
{
  vs.marginInside = false;
  wMain = view; // Don't retain since we're owned by view, which would cause a cycle
  wMargin = viewMargin;
  timerTarget = [[TimerTarget alloc] init: this];
  lastMouseEvent = NULL;
  delegate = NULL;
  notifyObj = NULL;
  notifyProc = NULL;
  capturedMouse = false;
  enteredSetScrollingSize = false;
  scrollSpeed = 1;
  scrollTicks = 2000;
  tickTimer = NULL;
  idleTimer = NULL;
  observer = NULL;
  layerFindIndicator = NULL;
  imeInteraction = imeInline;
  for (TickReason tr=tickCaret; tr<=tickPlatform; tr = static_cast<TickReason>(tr+1))
  {
    timers[tr] = nil;
  }
  Initialise();
}

//--------------------------------------------------------------------------------------------------

ScintillaCocoa::~ScintillaCocoa()
{
  Finalise();
  [timerTarget release];
}

//--------------------------------------------------------------------------------------------------

/**
 * Core initialization of the control. Everything that needs to be set up happens here.
 */
void ScintillaCocoa::Initialise()
{
  Scintilla_LinkLexers();

  // Tell Scintilla not to buffer: Quartz buffers drawing for us.
  WndProc(SCI_SETBUFFEREDDRAW, 0, 0);

  // We are working with Unicode exclusively.
  WndProc(SCI_SETCODEPAGE, SC_CP_UTF8, 0);

  // Add Mac specific key bindings.
  for (int i = 0; macMapDefault[i].key; i++)
    kmap.AssignCmdKey(macMapDefault[i].key, macMapDefault[i].modifiers, macMapDefault[i].msg);

}

//--------------------------------------------------------------------------------------------------

/**
 * We need some clean up. Do it here.
 */
void ScintillaCocoa::Finalise()
{
  ObserverRemove();
  for (TickReason tr=tickCaret; tr<=tickPlatform; tr = static_cast<TickReason>(tr+1))
  {
    FineTickerCancel(tr);
  }
  ScintillaBase::Finalise();
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::UpdateObserver(CFRunLoopObserverRef /* observer */, CFRunLoopActivity /* activity */, void *info) {
  ScintillaCocoa* sci = reinterpret_cast<ScintillaCocoa*>(info);
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

void ScintillaCocoa::QueueIdleWork(WorkNeeded::workItems items, int upTo) {
  Editor::QueueIdleWork(items, upTo);
  ObserverAdd();
}

//--------------------------------------------------------------------------------------------------

/**
 * Convert a core foundation string into an array of bytes in a particular encoding
 */

static char *EncodedBytes(CFStringRef cfsRef, CFStringEncoding encoding) {
    CFRange rangeAll = {0, CFStringGetLength(cfsRef)};
    CFIndex usedLen = 0;
    CFStringGetBytes(cfsRef, rangeAll, encoding, '?',
                     false, NULL, 0, &usedLen);

    char *buffer = new char[usedLen+1];
    CFStringGetBytes(cfsRef, rangeAll, encoding, '?',
                     false, (UInt8 *)buffer,usedLen, NULL);
    buffer[usedLen] = '\0';
    return buffer;
}

//--------------------------------------------------------------------------------------------------

/**
 * Convert a core foundation string into a std::string in a particular encoding
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
 * Case folders.
 */

class CaseFolderDBCS : public CaseFolderTable {
	CFStringEncoding encoding;
public:
	explicit CaseFolderDBCS(CFStringEncoding encoding_) : encoding(encoding_) {
		StandardASCII();
	}
	virtual size_t Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) {
		if ((lenMixed == 1) && (sizeFolded > 0)) {
			folded[0] = mapping[static_cast<unsigned char>(mixed[0])];
			return 1;
		} else {
            CFStringRef cfsVal = CFStringCreateWithBytes(kCFAllocatorDefault,
                                                         reinterpret_cast<const UInt8 *>(mixed),
                                                         lenMixed, encoding, false);

            NSString *sMapped = [(NSString *)cfsVal stringByFoldingWithOptions:NSCaseInsensitiveSearch
                                                                        locale:[NSLocale currentLocale]];

            char *encoded = EncodedBytes((CFStringRef)sMapped, encoding);

			size_t lenMapped = strlen(encoded);
            if (lenMapped < sizeFolded) {
                memcpy(folded, encoded,  lenMapped);
            } else {
                folded[0] = '\0';
                lenMapped = 1;
            }
            delete []encoded;
            CFRelease(cfsVal);
			return lenMapped;
		}
	}
};

CaseFolder *ScintillaCocoa::CaseFolderForEncoding() {
	if (pdoc->dbcsCodePage == SC_CP_UTF8) {
		return new CaseFolderUnicode();
	} else {
        CFStringEncoding encoding = EncodingFromCharacterSet(IsUnicodeMode(),
                                                             vs.styles[STYLE_DEFAULT].characterSet);
        if (pdoc->dbcsCodePage == 0) {
            CaseFolderTable *pcf = new CaseFolderTable();
            pcf->StandardASCII();
            // Only for single byte encodings
            for (int i=0x80; i<0x100; i++) {
                char sCharacter[2] = "A";
                sCharacter[0] = static_cast<char>(i);
                CFStringRef cfsVal = CFStringCreateWithBytes(kCFAllocatorDefault,
                                                             reinterpret_cast<const UInt8 *>(sCharacter),
                                                             1, encoding, false);
                if (!cfsVal)
                        continue;

                NSString *sMapped = [(NSString *)cfsVal stringByFoldingWithOptions:NSCaseInsensitiveSearch
                                                                            locale:[NSLocale currentLocale]];

                char *encoded = EncodedBytes((CFStringRef)sMapped, encoding);

                if (strlen(encoded) == 1) {
                    pcf->SetTranslation(sCharacter[0], encoded[0]);
                }

                delete []encoded;
                CFRelease(cfsVal);
            }
            return pcf;
        } else {
            return new CaseFolderDBCS(encoding);
        }
		return 0;
	}
}


//--------------------------------------------------------------------------------------------------

/**
 * Case-fold the given string depending on the specified case mapping type.
 */
std::string ScintillaCocoa::CaseMapString(const std::string &s, int caseMapping)
{
  if ((s.size() == 0) || (caseMapping == cmSame))
    return s;

  if (IsUnicodeMode()) {
    std::string retMapped(s.length() * maxExpansionCaseConversion, 0);
    size_t lenMapped = CaseConvertString(&retMapped[0], retMapped.length(), s.c_str(), s.length(),
      (caseMapping == cmUpper) ? CaseConversionUpper : CaseConversionLower);
    retMapped.resize(lenMapped);
    return retMapped;
  }

  CFStringEncoding encoding = EncodingFromCharacterSet(IsUnicodeMode(),
                                                       vs.styles[STYLE_DEFAULT].characterSet);
  CFStringRef cfsVal = CFStringCreateWithBytes(kCFAllocatorDefault,
                                               reinterpret_cast<const UInt8 *>(s.c_str()),
                                               s.length(), encoding, false);

  NSString *sMapped;
  switch (caseMapping)
  {
    case cmUpper:
      sMapped = [(NSString *)cfsVal uppercaseString];
      break;
    case cmLower:
      sMapped = [(NSString *)cfsVal lowercaseString];
      break;
    default:
      sMapped = (NSString *)cfsVal;
  }

  // Back to encoding
  char *encoded = EncodedBytes((CFStringRef)sMapped, encoding);
  std::string result(encoded);
  delete []encoded;
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
NSScrollView* ScintillaCocoa::ScrollContainer() const {
  NSView* container = static_cast<NSView*>(wMain.GetID());
  return static_cast<NSScrollView*>([[container superview] superview]);
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper function to get the inner container which represents the actual "canvas" we work with.
 */
SCIContentView* ScintillaCocoa::ContentView()
{
  return static_cast<SCIContentView*>(wMain.GetID());
}

//--------------------------------------------------------------------------------------------------

/**
 * Return the top left visible point relative to the origin point of the whole document.
 */
Scintilla::Point ScintillaCocoa::GetVisibleOriginInMain() const
{
  NSScrollView *scrollView = ScrollContainer();
  NSRect contentRect = [[scrollView contentView] bounds];
  return Point(static_cast<XYPOSITION>(contentRect.origin.x), static_cast<XYPOSITION>(contentRect.origin.y));
}

//--------------------------------------------------------------------------------------------------

/**
 * Instead of returning the size of the inner view we have to return the visible part of it
 * in order to make scrolling working properly.
 * The returned value is in document coordinates.
 */
PRectangle ScintillaCocoa::GetClientRectangle() const
{
  NSScrollView *scrollView = ScrollContainer();
  NSSize size = [[scrollView contentView] bounds].size;
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
    NSRect rcPrepared = [content preparedContentRect];
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
Scintilla::Point ScintillaCocoa::ConvertPoint(NSPoint point)
{
  NSView* container = ContentView();
  NSPoint result = [container convertPoint: point fromView: nil];
  Scintilla::Point ptOrigin = GetVisibleOriginInMain();
  return Point(static_cast<XYPOSITION>(result.x - ptOrigin.x), static_cast<XYPOSITION>(result.y - ptOrigin.y));
}

//--------------------------------------------------------------------------------------------------

/**
 * Do not clip like superclass as Cocoa is not reporting all of prepared area.
 */
void ScintillaCocoa::RedrawRect(PRectangle rc)
{
  if (!rc.Empty())
    wMain.InvalidateRectangle(rc);
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::DiscardOverdraw()
{
#if MAC_OS_X_VERSION_MAX_ALLOWED > 1080
  // If running on 10.9, reset prepared area to visible area
  NSView *content = ContentView();
  if ([content respondsToSelector: @selector(setPreparedContentRect:)]) {
    content.preparedContentRect = [content visibleRect];
  }
#endif
}

//--------------------------------------------------------------------------------------------------

/**
 * Ensure all of prepared content is also redrawn.
 */
void ScintillaCocoa::Redraw()
{
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
                                      sptr_t lParam)
{
  return reinterpret_cast<ScintillaCocoa *>(ptr)->WndProc(iMessage, wParam, lParam);
}

//--------------------------------------------------------------------------------------------------

/**
 * This method is very similar to DirectFunction. On Windows it sends a message (not in the Obj-C sense)
 * to the target window. Here we simply call our fake window proc.
 */
sptr_t scintilla_send_message(void* sci, unsigned int iMessage, uptr_t wParam, sptr_t lParam)
{
  ScintillaView *control = reinterpret_cast<ScintillaView*>(sci);
  return [control message:iMessage wParam:wParam lParam:lParam];
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
sptr_t ScintillaCocoa::WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam)
{
  switch (iMessage)
  {
    case SCI_GETDIRECTFUNCTION:
      return reinterpret_cast<sptr_t>(DirectFunction);

    case SCI_GETDIRECTPOINTER:
      return reinterpret_cast<sptr_t>(this);

    case SCI_TARGETASUTF8:
      return TargetAsUTF8(reinterpret_cast<char*>(lParam));

    case SCI_ENCODEDFROMUTF8:
      return EncodedFromUTF8(reinterpret_cast<char*>(wParam),
                             reinterpret_cast<char*>(lParam));

    case SCI_SETIMEINTERACTION:
      // Only inline IME supported on Cocoa
      break;

    case SCI_GRABFOCUS:
      [[ContentView() window] makeFirstResponder:ContentView()];
      break;

    case SCI_SETBUFFEREDDRAW:
      // Buffered drawing not supported on Cocoa
      view.bufferedDraw = false;
      break;

    case SCI_FINDINDICATORSHOW:
      ShowFindIndicatorForRange(NSMakeRange(wParam, lParam-wParam), YES);
      return 0;

    case SCI_FINDINDICATORFLASH:
      ShowFindIndicatorForRange(NSMakeRange(wParam, lParam-wParam), NO);
      return 0;

    case SCI_FINDINDICATORHIDE:
      HideFindIndicator();
      return 0;

    default:
      sptr_t r = ScintillaBase::WndProc(iMessage, wParam, lParam);

      return r;
  }
  return 0l;
}

//--------------------------------------------------------------------------------------------------

/**
 * In Windows lingo this is the handler which handles anything that wasn't handled in the normal
 * window proc which would usually send the message back to generic window proc that Windows uses.
 */
sptr_t ScintillaCocoa::DefWndProc(unsigned int, uptr_t, sptr_t)
{
  return 0;
}

//--------------------------------------------------------------------------------------------------

/**
 * Handle any ScintillaCocoa-specific ticking or call superclass.
 */
void ScintillaCocoa::TickFor(TickReason reason)
{
  if (reason == tickPlatform)
  {
    DragScroll();
  }
  else
  {
    Editor::TickFor(reason);
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Report that this Editor subclass has a working implementation of FineTickerStart.
 */
bool ScintillaCocoa::FineTickerAvailable()
{
  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Is a particular timer currently running?
 */
bool ScintillaCocoa::FineTickerRunning(TickReason reason)
{
  return timers[reason] != nil;
}

//--------------------------------------------------------------------------------------------------

/**
 * Start a fine-grained timer.
 */
void ScintillaCocoa::FineTickerStart(TickReason reason, int millis, int tolerance)
{
  FineTickerCancel(reason);
  NSTimer *fineTimer = [NSTimer scheduledTimerWithTimeInterval: millis / 1000.0
                                                        target: timerTarget
                                                      selector: @selector(timerFired:)
                                                      userInfo: nil
                                                       repeats: YES];
  if (tolerance && [fineTimer respondsToSelector: @selector(setTolerance:)])
  {
    [fineTimer setTolerance: tolerance / 1000.0];
  }
  timers[reason] = fineTimer;
}

//--------------------------------------------------------------------------------------------------

/**
 * Cancel a fine-grained timer.
 */
void ScintillaCocoa::FineTickerCancel(TickReason reason)
{
  if (timers[reason])
  {
    [timers[reason] invalidate];
    timers[reason] = nil;
  }
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::SetIdle(bool on)
{
  if (idler.state != on)
  {
    idler.state = on;
    if (idler.state)
    {
      // Scintilla ticks = milliseconds
      idleTimer = [NSTimer scheduledTimerWithTimeInterval: timer.tickSize / 1000.0
						   target: timerTarget
						 selector: @selector(idleTimerFired:)
						 userInfo: nil
						  repeats: YES];
      idler.idlerID = reinterpret_cast<IdlerID>(idleTimer);
    }
    else
      if (idler.idlerID != NULL)
      {
        [reinterpret_cast<NSTimer*>(idler.idlerID) invalidate];
        idler.idlerID = 0;
      }
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::CopyToClipboard(const SelectionText &selectedText)
{
  SetPasteboardData([NSPasteboard generalPasteboard], selectedText);
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::Copy()
{
  if (!sel.Empty())
  {
    SelectionText selectedText;
    CopySelectionRange(&selectedText);
    CopyToClipboard(selectedText);
  }
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::CanPaste()
{
  if (!Editor::CanPaste())
    return false;

  return GetPasteboardData([NSPasteboard generalPasteboard], NULL);
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::Paste()
{
  Paste(false);
}

//--------------------------------------------------------------------------------------------------

/**
 * Pastes data from the paste board into the editor.
 */
void ScintillaCocoa::Paste(bool forceRectangular)
{
  SelectionText selectedText;
  bool ok = GetPasteboardData([NSPasteboard generalPasteboard], &selectedText);
  if (forceRectangular)
    selectedText.rectangular = forceRectangular;

  if (!ok || selectedText.Empty())
    // No data or no flavor we support.
    return;

  pdoc->BeginUndoAction();
  ClearSelection(false);
  InsertPasteShape(selectedText.Data(), static_cast<int>(selectedText.Length()),
	  selectedText.rectangular ? pasteRectangular : pasteStream);
  pdoc->EndUndoAction();

  Redraw();
  EnsureCaretVisible();
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::CTPaint(void* gc, NSRect rc) {
#pragma unused(rc)
    Surface *surfaceWindow = Surface::Allocate(SC_TECHNOLOGY_DEFAULT);
    if (surfaceWindow) {
        surfaceWindow->Init(gc, wMain.GetID());
        surfaceWindow->SetUnicodeMode(SC_CP_UTF8 == ct.codePage);
        surfaceWindow->SetDBCSMode(ct.codePage);
        ct.PaintCT(surfaceWindow);
        surfaceWindow->Release();
        delete surfaceWindow;
    }
}

@interface CallTipView : NSControl {
    ScintillaCocoa *sci;
}

@end

@implementation CallTipView

- (NSView*) initWithFrame: (NSRect) frame {
	self = [super initWithFrame: frame];

	if (self) {
        sci = NULL;
	}

	return self;
}

- (void) dealloc {
	[super dealloc];
}

- (BOOL) isFlipped {
	return YES;
}

- (void) setSci: (ScintillaCocoa *) sci_ {
    sci = sci_;
}

- (void) drawRect: (NSRect) needsDisplayInRect {
    if (sci) {
        CGContextRef context = (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort];
        sci->CTPaint(context, needsDisplayInRect);
    }
}

- (void) mouseDown: (NSEvent *) event {
    if (sci) {
        sci->CallTipMouseDown([event locationInWindow]);
    }
}

// On OS X, only the key view should modify the cursor so the calltip can't.
// This view does not become key so resetCursorRects never called.
- (void) resetCursorRects {
    //[super resetCursorRects];
    //[self addCursorRect: [self bounds] cursor: [NSCursor arrowCursor]];
}

@end

void ScintillaCocoa::CallTipMouseDown(NSPoint pt) {
    NSRect rectBounds = [(NSView *)(ct.wDraw.GetID()) bounds];
    Point location(static_cast<XYPOSITION>(pt.x),
		   static_cast<XYPOSITION>(rectBounds.size.height - pt.y));
    ct.MouseClick(location);
    CallTipClick();
}

static bool HeightDifferent(WindowID wCallTip, PRectangle rc) {
	NSWindow *callTip = (NSWindow *)wCallTip;
	CGFloat height = NSHeight([callTip frame]);
	return height != rc.Height();
}

void ScintillaCocoa::CreateCallTipWindow(PRectangle rc) {
    if (ct.wCallTip.Created() && HeightDifferent(ct.wCallTip.GetID(), rc)) {
        ct.wCallTip.Destroy();
    }
    if (!ct.wCallTip.Created()) {
        NSRect ctRect = NSMakeRect(rc.top,rc.bottom, rc.Width(), rc.Height());
        NSWindow *callTip = [[NSWindow alloc] initWithContentRect: ctRect
                                                        styleMask: NSBorderlessWindowMask
                                                          backing: NSBackingStoreBuffered
                                                            defer: NO];
        [callTip setLevel:NSFloatingWindowLevel];
        [callTip setHasShadow:YES];
        NSRect ctContent = NSMakeRect(0,0, rc.Width(), rc.Height());
        CallTipView *caption = [[CallTipView alloc] initWithFrame: ctContent];
        [caption setAutoresizingMask: NSViewWidthSizable | NSViewMaxYMargin];
        [caption setSci: this];
        [[callTip contentView] addSubview: caption];
        [callTip orderFront:caption];
        ct.wCallTip = callTip;
        ct.wDraw = caption;
    }
}

void ScintillaCocoa::AddToPopUp(const char *label, int cmd, bool enabled)
{
  NSMenuItem* item;
  ScintillaContextMenu *menu= reinterpret_cast<ScintillaContextMenu*>(popup.GetID());
  [menu setOwner: this];
  [menu setAutoenablesItems: NO];

  if (cmd == 0) {
    item = [NSMenuItem separatorItem];
  } else {
    item = [[[NSMenuItem alloc] init] autorelease];
    [item setTitle: [NSString stringWithUTF8String: label]];
  }
  [item setTarget: menu];
  [item setAction: @selector(handleCommand:)];
  [item setTag: cmd];
  [item setEnabled: enabled];

  [menu addItem: item];
}

// -------------------------------------------------------------------------------------------------

void ScintillaCocoa::ClaimSelection()
{
  // Mac OS X does not have a primary selection.
}

// -------------------------------------------------------------------------------------------------

/**
 * Returns the current caret position (which is tracked as an offset into the entire text string)
 * as a row:column pair. The result is zero-based.
 */
NSPoint ScintillaCocoa::GetCaretPosition()
{
  const int line = pdoc->LineFromPosition(sel.RangeMain().caret.Position());
  NSPoint result;

  result.y = line;
  result.x = sel.RangeMain().caret.Position() - pdoc->LineStart(line);
  return result;
}

// -------------------------------------------------------------------------------------------------

#pragma mark Drag

/**
 * Triggered by the tick timer on a regular basis to scroll the content during a drag operation.
 */
void ScintillaCocoa::DragScroll()
{
  if (!posDrag.IsValid())
  {
    scrollSpeed = 1;
    scrollTicks = 2000;
    return;
  }

  // TODO: does not work for wrapped lines, fix it.
  int line = pdoc->LineFromPosition(posDrag.Position());
  int currentVisibleLine = cs.DisplayFromDoc(line);
  int lastVisibleLine = Platform::Minimum(topLine + LinesOnScreen(), cs.LinesDisplayed()) - 2;

  if (currentVisibleLine <= topLine && topLine > 0)
    ScrollTo(topLine - scrollSpeed);
  else
    if (currentVisibleLine >= lastVisibleLine)
      ScrollTo(topLine + scrollSpeed);
    else
    {
      scrollSpeed = 1;
      scrollTicks = 2000;
      return;
    }

  // TODO: also handle horizontal scrolling.

  if (scrollSpeed == 1)
  {
    scrollTicks -= timer.tickSize;
    if (scrollTicks <= 0)
    {
      scrollSpeed = 5;
      scrollTicks = 2000;
    }
  }

}

//----------------- DragProviderSource -------------------------------------------------------

@interface DragProviderSource : NSObject <NSPasteboardItemDataProvider>
{
  SelectionText selectedText;
}

@end

@implementation DragProviderSource

- (id)initWithSelectedText:(const SelectionText *)other
{
  self = [super init];
  
  if (self) {
    selectedText.Copy(*other);
  }
  
  return self;
}

- (void)pasteboard:(NSPasteboard *)pasteboard item:(NSPasteboardItem *)item provideDataForType:(NSString *)type
{
  if (selectedText.Length() == 0)
    return;

  if (([type compare: NSPasteboardTypeString] != NSOrderedSame) &&
    ([type compare: ScintillaRecPboardType] != NSOrderedSame))
    return;

  CFStringEncoding encoding = EncodingFromCharacterSet(selectedText.codePage == SC_CP_UTF8,
                                                       selectedText.characterSet);
  CFStringRef cfsVal = CFStringCreateWithBytes(kCFAllocatorDefault,
                                               reinterpret_cast<const UInt8 *>(selectedText.Data()),
                                               selectedText.Length(), encoding, false);
  
  if ([type compare: NSPasteboardTypeString] == NSOrderedSame)
  {
    [pasteboard setString:(NSString *)cfsVal forType: NSStringPboardType];
  }
  else if ([type compare: ScintillaRecPboardType] == NSOrderedSame)
  {
    // This is specific to scintilla, allows us to drag rectangular selections around the document.
    if (selectedText.rectangular)
      [pasteboard setString:(NSString *)cfsVal forType: ScintillaRecPboardType];
  }

  if (cfsVal)
    CFRelease(cfsVal);  
}

@end

//--------------------------------------------------------------------------------------------------

/**
 * Called when a drag operation was initiated from within Scintilla.
 */
void ScintillaCocoa::StartDrag()
{
  if (sel.Empty())
    return;

  inDragDrop = ddDragging;

  FineTickerStart(tickPlatform, timer.tickSize, 0);

  // Put the data to be dragged on the drag pasteboard.
  SelectionText selectedText;
  NSPasteboard* pasteboard = [NSPasteboard pasteboardWithName: NSDragPboard];
  CopySelectionRange(&selectedText);
  SetPasteboardData(pasteboard, selectedText);

  // calculate the bounds of the selection
  PRectangle client = GetTextRectangle();
  int selStart = sel.RangeMain().Start().Position();
  int selEnd = sel.RangeMain().End().Position();
  int startLine = pdoc->LineFromPosition(selStart);
  int endLine = pdoc->LineFromPosition(selEnd);
  Point pt;
  long startPos, endPos, ep;
  PRectangle rcSel;

  if (startLine==endLine && WndProc(SCI_GETWRAPMODE, 0, 0) != SC_WRAP_NONE) {
    // Komodo bug http://bugs.activestate.com/show_bug.cgi?id=87571
    // Scintilla bug https://sourceforge.net/tracker/?func=detail&atid=102439&aid=3040200&group_id=2439
    // If the width on a wrapped-line selection is negative,
    // find a better bounding rectangle.

    Point ptStart, ptEnd;
    startPos = WndProc(SCI_GETLINESELSTARTPOSITION, startLine, 0);
    endPos =   WndProc(SCI_GETLINESELENDPOSITION,   startLine, 0);
    // step back a position if we're counting the newline
    ep =       WndProc(SCI_GETLINEENDPOSITION,      startLine, 0);
    if (endPos > ep) endPos = ep;
    ptStart = LocationFromPosition(static_cast<int>(startPos));
    ptEnd =   LocationFromPosition(static_cast<int>(endPos));
    if (ptStart.y == ptEnd.y) {
      // We're just selecting part of one visible line
      rcSel.left = ptStart.x;
      rcSel.right = ptEnd.x < client.right ? ptEnd.x : client.right;
    } else {
      // Find the bounding box.
      startPos = WndProc(SCI_POSITIONFROMLINE, startLine, 0);
      rcSel.left = LocationFromPosition(static_cast<int>(startPos)).x;
      rcSel.right = client.right;
    }
    rcSel.top = ptStart.y;
    rcSel.bottom = ptEnd.y + vs.lineHeight;
    if (rcSel.bottom > client.bottom) {
      rcSel.bottom = client.bottom;
    }
  } else {
    rcSel.top = rcSel.bottom = rcSel.right = rcSel.left = -1;
    for (int l = startLine; l <= endLine; l++) {
      startPos = WndProc(SCI_GETLINESELSTARTPOSITION, l, 0);
      endPos = WndProc(SCI_GETLINESELENDPOSITION, l, 0);
      if (endPos == startPos) continue;
      // step back a position if we're counting the newline
      ep = WndProc(SCI_GETLINEENDPOSITION, l, 0);
      if (endPos > ep) endPos = ep;
      pt = LocationFromPosition(static_cast<int>(startPos)); // top left of line selection
      if (pt.x < rcSel.left || rcSel.left < 0) rcSel.left = pt.x;
      if (pt.y < rcSel.top || rcSel.top < 0) rcSel.top = pt.y;
      pt = LocationFromPosition(static_cast<int>(endPos)); // top right of line selection
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

  SCIContentView* content = ContentView();

  // To get a bitmap of the text we're dragging, we just use Paint on a pixmap surface.
  SurfaceImpl *sw = new SurfaceImpl();
  SurfaceImpl *pixmap = NULL;

  bool lastHideSelection = view.hideSelection;
  view.hideSelection = true;
  if (sw)
  {
    pixmap = new SurfaceImpl();
    if (pixmap)
    {
      PRectangle imageRect = rcSel;
      paintState = painting;
      sw->InitPixMap(static_cast<int>(client.Width()), static_cast<int>(client.Height()), NULL, NULL);
      paintingAllText = true;
      // Have to create a new context and make current as text drawing goes
      // to the current context, not a passed context.
      CGContextRef gcsw = sw->GetContext();
      NSGraphicsContext *nsgc = [NSGraphicsContext graphicsContextWithGraphicsPort: gcsw
                                                                           flipped: YES];
      [NSGraphicsContext setCurrentContext:nsgc];
      CGContextTranslateCTM(gcsw, -client.left, -client.top);
      Paint(sw, client);
      paintState = notPainting;

      pixmap->InitPixMap(static_cast<int>(imageRect.Width()), static_cast<int>(imageRect.Height()), NULL, NULL);

      CGContextRef gc = pixmap->GetContext();
      // To make Paint() work on a bitmap, we have to flip our coordinates and translate the origin
      CGContextTranslateCTM(gc, 0, imageRect.Height());
      CGContextScaleCTM(gc, 1.0, -1.0);

      pixmap->CopyImageRectangle(*sw, imageRect, PRectangle(0.0f, 0.0f, imageRect.Width(), imageRect.Height()));
      // XXX TODO: overwrite any part of the image that is not part of the
      //           selection to make it transparent.  right now we just use
      //           the full rectangle which may include non-selected text.
    }
    sw->Release();
    delete sw;
  }
  view.hideSelection = lastHideSelection;

  NSBitmapImageRep* bitmap = NULL;
  if (pixmap)
  {
    CGImageRef imagePixmap = pixmap->GetImage();
    if (imagePixmap)
      bitmap = [[[NSBitmapImageRep alloc] initWithCGImage: imagePixmap] autorelease];
    CGImageRelease(imagePixmap);
    pixmap->Release();
    delete pixmap;
  }

  NSImage* image = [[[NSImage alloc] initWithSize: selectionRectangle.size] autorelease];
  [image addRepresentation: bitmap];

  NSImage* dragImage = [[[NSImage alloc] initWithSize: selectionRectangle.size] autorelease];
  [dragImage setBackgroundColor: [NSColor clearColor]];
  [dragImage lockFocus];
  [image drawAtPoint: NSZeroPoint fromRect: NSZeroRect operation: NSCompositeSourceOver fraction: 0.5];
  [dragImage unlockFocus];

  NSPoint startPoint;
  startPoint.x = selectionRectangle.origin.x + client.left;
  startPoint.y = selectionRectangle.origin.y + selectionRectangle.size.height + client.top;
  
  NSPasteboardItem *pbItem = [NSPasteboardItem new];
  DragProviderSource *dps = [[[DragProviderSource alloc] initWithSelectedText:&selectedText] autorelease];
  
  NSArray *pbTypes = selectedText.rectangular ?
  @[NSPasteboardTypeString, ScintillaRecPboardType] :
  @[NSPasteboardTypeString];
  [pbItem setDataProvider:dps forTypes:pbTypes];
  NSDraggingItem *dragItem = [[NSDraggingItem alloc ]initWithPasteboardWriter:pbItem];
  [pbItem release];
  
  NSScrollView *scrollContainer = ScrollContainer();
  NSRect contentRect = [[scrollContainer contentView] bounds];
  NSRect draggingRect = NSOffsetRect(selectionRectangle, contentRect.origin.x, contentRect.origin.y);
  [dragItem setDraggingFrame:draggingRect contents:dragImage];
  NSDraggingSession *dragSession =
  [content beginDraggingSessionWithItems:@[[dragItem autorelease]]
                                   event:lastMouseEvent
                                  source:content];
  dragSession.animatesToStartingPositionsOnCancelOrFail = YES;
  dragSession.draggingFormation = NSDraggingFormationNone;
}

//--------------------------------------------------------------------------------------------------

/**
 * Called when a drag operation reaches the control which was initiated outside.
 */
NSDragOperation ScintillaCocoa::DraggingEntered(id <NSDraggingInfo> info)
{
  FineTickerStart(tickPlatform, timer.tickSize, 0);
  return DraggingUpdated(info);
}

//--------------------------------------------------------------------------------------------------

/**
 * Called frequently during a drag operation if we are the target. Keep telling the caller
 * what drag operation we accept and update the drop caret position to indicate the
 * potential insertion point of the dragged data.
 */
NSDragOperation ScintillaCocoa::DraggingUpdated(id <NSDraggingInfo> info)
{
  // Convert the drag location from window coordinates to view coordinates and
  // from there to a text position to finally set the drag position.
  Point location = ConvertPoint([info draggingLocation]);
  SetDragPosition(SPositionFromLocation(location));

  NSDragOperation sourceDragMask = [info draggingSourceOperationMask];
  if (sourceDragMask == NSDragOperationNone)
    return sourceDragMask;

  NSPasteboard* pasteboard = [info draggingPasteboard];

  // Return what type of operation we will perform. Prefer move over copy.
  if ([[pasteboard types] containsObject: NSStringPboardType] ||
      [[pasteboard types] containsObject: ScintillaRecPboardType])
    return (sourceDragMask & NSDragOperationMove) ? NSDragOperationMove : NSDragOperationCopy;

  if ([[pasteboard types] containsObject: NSFilenamesPboardType])
    return (sourceDragMask & NSDragOperationGeneric);
  return NSDragOperationNone;
}

//--------------------------------------------------------------------------------------------------

/**
 * Resets the current drag position as we are no longer the drag target.
 */
void ScintillaCocoa::DraggingExited(id <NSDraggingInfo> info)
{
#pragma unused(info)
  SetDragPosition(SelectionPosition(invalidPosition));
  FineTickerCancel(tickPlatform);
  inDragDrop = ddNone;
}

//--------------------------------------------------------------------------------------------------

/**
 * Here is where the real work is done. Insert the text from the pasteboard.
 */
bool ScintillaCocoa::PerformDragOperation(id <NSDraggingInfo> info)
{
  NSPasteboard* pasteboard = [info draggingPasteboard];

  if ([[pasteboard types] containsObject: NSFilenamesPboardType])
  {
    NSArray* files = [pasteboard propertyListForType: NSFilenamesPboardType];
    for (NSString* uri in files)
      NotifyURIDropped([uri UTF8String]);
  }
  else
  {
    SelectionText text;
    GetPasteboardData(pasteboard, &text);

    if (text.Length() > 0)
    {
      NSDragOperation operation = [info draggingSourceOperationMask];
      bool moving = (operation & NSDragOperationMove) != 0;

      DropAt(posDrag, text.Data(), text.Length(), moving, text.rectangular);
    };
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::SetPasteboardData(NSPasteboard* board, const SelectionText &selectedText)
{
  if (selectedText.Length() == 0)
    return;

  CFStringEncoding encoding = EncodingFromCharacterSet(selectedText.codePage == SC_CP_UTF8,
                                                       selectedText.characterSet);
  CFStringRef cfsVal = CFStringCreateWithBytes(kCFAllocatorDefault,
                                               reinterpret_cast<const UInt8 *>(selectedText.Data()),
                                               selectedText.Length(), encoding, false);

  NSArray *pbTypes = selectedText.rectangular ?
    [NSArray arrayWithObjects: NSStringPboardType, ScintillaRecPboardType, nil] :
    [NSArray arrayWithObjects: NSStringPboardType, nil];
  [board declareTypes:pbTypes owner:nil];

  if (selectedText.rectangular)
  {
    // This is specific to scintilla, allows us to drag rectangular selections around the document.
    [board setString: (NSString *)cfsVal forType: ScintillaRecPboardType];
  }

  [board setString: (NSString *)cfsVal forType: NSStringPboardType];

  if (cfsVal)
    CFRelease(cfsVal);
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper method to retrieve the best fitting alternative from the general pasteboard.
 */
bool ScintillaCocoa::GetPasteboardData(NSPasteboard* board, SelectionText* selectedText)
{
  NSArray* supportedTypes = [NSArray arrayWithObjects: ScintillaRecPboardType,
                             NSStringPboardType,
                             nil];
  NSString *bestType = [board availableTypeFromArray: supportedTypes];
  NSString* data = [board stringForType: bestType];

  if (data != nil)
  {
    if (selectedText != nil)
    {
      CFStringEncoding encoding = EncodingFromCharacterSet(IsUnicodeMode(),
                                                           vs.styles[STYLE_DEFAULT].characterSet);
      CFRange rangeAll = {0, static_cast<CFIndex>([data length])};
      CFIndex usedLen = 0;
      CFStringGetBytes((CFStringRef)data, rangeAll, encoding, '?',
                       false, NULL, 0, &usedLen);

      std::vector<UInt8> buffer(usedLen);

      CFStringGetBytes((CFStringRef)data, rangeAll, encoding, '?',
                       false, buffer.data(),usedLen, NULL);

      bool rectangular = bestType == ScintillaRecPboardType;

      std::string dest(reinterpret_cast<const char *>(buffer.data()), usedLen);

      selectedText->Copy(dest, pdoc->dbcsCodePage,
                         vs.styles[STYLE_DEFAULT].characterSet , rectangular, false);
    }
    return true;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

// Returns the target converted to UTF8.
// Return the length in bytes.
int ScintillaCocoa::TargetAsUTF8(char *text)
{
  const int targetLength = targetEnd - targetStart;
  if (IsUnicodeMode())
  {
    if (text)
      pdoc->GetCharRange(text, targetStart, targetLength);
  }
  else
  {
    // Need to convert
    const CFStringEncoding encoding = EncodingFromCharacterSet(IsUnicodeMode(),
                                                         vs.styles[STYLE_DEFAULT].characterSet);
    const std::string s = RangeText(targetStart, targetEnd);
    CFStringRef cfsVal = CFStringCreateWithBytes(kCFAllocatorDefault,
                                                 reinterpret_cast<const UInt8 *>(s.c_str()),
                                                 s.length(), encoding, false);
	  
    const std::string tmputf = EncodedBytesString(cfsVal, kCFStringEncodingUTF8);
    
    if (text)
      memcpy(text, tmputf.c_str(), tmputf.length());
    CFRelease(cfsVal);
    return tmputf.length();
  }
  return targetLength;
}

//--------------------------------------------------------------------------------------------------

// Translates a UTF8 string into the document encoding.
// Return the length of the result in bytes.
int ScintillaCocoa::EncodedFromUTF8(char *utf8, char *encoded) const
{
  const int inputLength = (lengthForEncode >= 0) ? lengthForEncode : strlen(utf8);
  if (IsUnicodeMode())
  {
    if (encoded)
      memcpy(encoded, utf8, inputLength);
    return inputLength;
  }
  else
  {
    // Need to convert
    const CFStringEncoding encoding = EncodingFromCharacterSet(IsUnicodeMode(),
                                                         vs.styles[STYLE_DEFAULT].characterSet);
    
    CFStringRef cfsVal = CFStringCreateWithBytes(kCFAllocatorDefault,
                                                 reinterpret_cast<const UInt8 *>(utf8),
                                                 inputLength, kCFStringEncodingUTF8, false);
    const std::string sEncoded = EncodedBytesString(cfsVal, encoding);
    if (encoded)
      memcpy(encoded, sEncoded.c_str(), sEncoded.length());
    CFRelease(cfsVal);
    return sEncoded.length();
  }
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::SetMouseCapture(bool on)
{
  capturedMouse = on;
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::HaveMouseCapture()
{
  return capturedMouse;
}

//--------------------------------------------------------------------------------------------------

/**
 * Synchronously paint a rectangle of the window.
 */
bool ScintillaCocoa::SyncPaint(void* gc, PRectangle rc)
{
  paintState = painting;
  rcPaint = rc;
  PRectangle rcText = GetTextRectangle();
  paintingAllText = rcPaint.Contains(rcText);
  bool succeeded = true;
  Surface *sw = Surface::Allocate(SC_TECHNOLOGY_DEFAULT);
  if (sw)
  {
    CGContextSetAllowsAntialiasing((CGContextRef)gc,
                                   vs.extraFontFlag != SC_EFF_QUALITY_NON_ANTIALIASED);
    CGContextSetAllowsFontSmoothing((CGContextRef)gc,
                                    vs.extraFontFlag == SC_EFF_QUALITY_LCD_OPTIMIZED);
    CGContextSetAllowsFontSubpixelPositioning((CGContextRef)gc,
                                              vs.extraFontFlag == SC_EFF_QUALITY_DEFAULT ||
                                              vs.extraFontFlag == SC_EFF_QUALITY_LCD_OPTIMIZED);
    sw->Init(gc, wMain.GetID());
    Paint(sw, rc);
    succeeded = paintState != paintAbandoned;
    sw->Release();
    delete sw;
  }
  paintState = notPainting;
  if (!succeeded)
  {
    NSView *marginView = static_cast<NSView*>(wMargin.GetID());
    [marginView setNeedsDisplay:YES];
  }
  return succeeded;
}

//--------------------------------------------------------------------------------------------------

/**
 * Paint the margin into the SCIMarginView space.
 */
void ScintillaCocoa::PaintMargin(NSRect aRect)
{
  CGContextRef gc = (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort];

  PRectangle rc = NSRectToPRectangle(aRect);
  rcPaint = rc;
  Surface *sw = Surface::Allocate(SC_TECHNOLOGY_DEFAULT);
  if (sw)
  {
    CGContextSetAllowsAntialiasing(gc,
                                   vs.extraFontFlag != SC_EFF_QUALITY_NON_ANTIALIASED);
    CGContextSetAllowsFontSmoothing(gc,
                                    vs.extraFontFlag == SC_EFF_QUALITY_LCD_OPTIMIZED);
    CGContextSetAllowsFontSubpixelPositioning(gc,
                                              vs.extraFontFlag == SC_EFF_QUALITY_DEFAULT ||
                                              vs.extraFontFlag == SC_EFF_QUALITY_LCD_OPTIMIZED);
    sw->Init(gc, wMargin.GetID());
    PaintSelMargin(sw, rc);
    sw->Release();
    delete sw;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Prepare for drawing.
 *
 * @param rect The area that will be drawn, given in the sender's coordinate system.
 */
void ScintillaCocoa::WillDraw(NSRect rect)
{
  RefreshStyleData();
  PRectangle rcWillDraw = NSRectToPRectangle(rect);
  int positionAfterRect = PositionAfterArea(rcWillDraw);
  pdoc->EnsureStyledTo(positionAfterRect);
  NotifyUpdateUI();
  if (WrapLines(wsVisible)) {
    // Wrap may have reduced number of lines so more lines may need to be styled
    positionAfterRect = PositionAfterArea(rcWillDraw);
    pdoc->EnsureStyledTo(positionAfterRect);
    // The wrapping process has changed the height of some lines so redraw all.
    Redraw();
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * ScrollText is empty because scrolling is handled by the NSScrollView.
 */
void ScintillaCocoa::ScrollText(int)
{
}

//--------------------------------------------------------------------------------------------------

/**
 * Modifies the vertical scroll position to make the current top line show up as such.
 */
void ScintillaCocoa::SetVerticalScrollPos()
{
  NSScrollView *scrollView = ScrollContainer();
  if (scrollView) {
    NSClipView *clipView = [scrollView contentView];
    NSRect contentRect = [clipView bounds];
    [clipView scrollToPoint: NSMakePoint(contentRect.origin.x, topLine * vs.lineHeight)];
    [scrollView reflectScrolledClipView:clipView];
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Modifies the horizontal scroll position to match xOffset.
 */
void ScintillaCocoa::SetHorizontalScrollPos()
{
  PRectangle textRect = GetTextRectangle();

  int maxXOffset = scrollWidth - static_cast<int>(textRect.Width());
  if (maxXOffset < 0)
    maxXOffset = 0;
  if (xOffset > maxXOffset)
    xOffset = maxXOffset;
  NSScrollView *scrollView = ScrollContainer();
  if (scrollView) {
    NSClipView * clipView = [scrollView contentView];
    NSRect contentRect = [clipView bounds];
    [clipView scrollToPoint: NSMakePoint(xOffset, contentRect.origin.y)];
    [scrollView reflectScrolledClipView:clipView];
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
bool ScintillaCocoa::ModifyScrollBars(int nMax, int nPage)
{
#pragma unused(nMax, nPage)
  return SetScrollingSize();
}

bool ScintillaCocoa::SetScrollingSize(void) {
	bool changes = false;
	SCIContentView *inner = ContentView();
	if (!enteredSetScrollingSize) {
		enteredSetScrollingSize = true;
		NSScrollView *scrollView = ScrollContainer();
		NSClipView *clipView = [ScrollContainer() contentView];
		NSRect clipRect = [clipView bounds];
		CGFloat docHeight = cs.LinesDisplayed() * vs.lineHeight;
		if (!endAtLastLine)
			docHeight += (int([scrollView bounds].size.height / vs.lineHeight)-3) * vs.lineHeight;
		// Allow extra space so that last scroll position places whole line at top
		int clipExtra = int(clipRect.size.height) % vs.lineHeight;
		docHeight += clipExtra;
		// Ensure all of clipRect covered by Scintilla drawing
		if (docHeight < clipRect.size.height)
			docHeight = clipRect.size.height;
		CGFloat docWidth = scrollWidth;
		bool showHorizontalScroll = horizontalScrollBarVisible &&
			!Wrapping();
		if (!showHorizontalScroll)
			docWidth = clipRect.size.width;
		NSRect contentRect = {{0, 0}, {docWidth, docHeight}};
		NSRect contentRectNow = [inner frame];
		changes = (contentRect.size.width != contentRectNow.size.width) ||
			(contentRect.size.height != contentRectNow.size.height);
		if (changes) {
			[inner setFrame: contentRect];
		}
		[scrollView setHasVerticalScroller: verticalScrollBarVisible];
		[scrollView setHasHorizontalScroller: showHorizontalScroll];
		SetVerticalScrollPos();
		enteredSetScrollingSize = false;
	}
	[inner.owner setMarginWidth: vs.fixedColumnWidth];
	return changes;
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::Resize()
{
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
  int newTop = Platform::Minimum(static_cast<int>(ptOrigin.y / vs.lineHeight), MaxScrollPos());
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

void ScintillaCocoa::SetDelegate(id<ScintillaNotificationProtocol> delegate_)
{
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
void ScintillaCocoa::RegisterNotifyCallback(intptr_t windowid, SciNotifyFunc callback)
{
  notifyObj = windowid;
  notifyProc = callback;
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::NotifyChange()
{
  if (notifyProc != NULL)
    notifyProc(notifyObj, WM_COMMAND, Platform::LongFromTwoShorts(static_cast<short>(GetCtrlID()), SCEN_CHANGE),
	       (uintptr_t) this);
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::NotifyFocus(bool focus)
{
  if (notifyProc != NULL)
    notifyProc(notifyObj, WM_COMMAND, Platform::LongFromTwoShorts(static_cast<short>(GetCtrlID()),
	       (focus ? SCEN_SETFOCUS : SCEN_KILLFOCUS)),
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
void ScintillaCocoa::NotifyParent(SCNotification scn)
{
  scn.nmhdr.hwndFrom = (void*) this;
  scn.nmhdr.idFrom = GetCtrlID();
  if (notifyProc != NULL)
    notifyProc(notifyObj, WM_NOTIFY, GetCtrlID(), (uintptr_t) &scn);
  if (delegate)
    [delegate notification:&scn];
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::NotifyURIDropped(const char *uri)
{
  SCNotification scn;
  scn.nmhdr.code = SCN_URIDROPPED;
  scn.text = uri;

  NotifyParent(scn);
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::HasSelection()
{
  return !sel.Empty();
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::CanUndo()
{
  return pdoc->CanUndo();
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::CanRedo()
{
  return pdoc->CanRedo();
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::TimerFired(NSTimer* timer)
{
  for (TickReason tr=tickCaret; tr<=tickPlatform; tr = static_cast<TickReason>(tr+1))
  {
    if (timers[tr] == timer)
    {
      TickFor(tr);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::IdleTimerFired()
{
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
bool ScintillaCocoa::Draw(NSRect rect, CGContextRef gc)
{
  return SyncPaint(gc, NSRectToPRectangle(rect));
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper function to translate OS X key codes to Scintilla key codes.
 */
static inline UniChar KeyTranslate(UniChar unicodeChar)
{
  switch (unicodeChar)
  {
    case NSDownArrowFunctionKey:
      return SCK_DOWN;
    case NSUpArrowFunctionKey:
      return SCK_UP;
    case NSLeftArrowFunctionKey:
      return SCK_LEFT;
    case NSRightArrowFunctionKey:
      return SCK_RIGHT;
    case NSHomeFunctionKey:
      return SCK_HOME;
    case NSEndFunctionKey:
      return SCK_END;
    case NSPageUpFunctionKey:
      return SCK_PRIOR;
    case NSPageDownFunctionKey:
      return SCK_NEXT;
    case NSDeleteFunctionKey:
      return SCK_DELETE;
    case NSInsertFunctionKey:
      return SCK_INSERT;
    case '\n':
    case 3:
      return SCK_RETURN;
    case 27:
      return SCK_ESCAPE;
    case 127:
      return SCK_BACK;
    case '\t':
    case 25: // Shift tab, return to unmodified tab and handle that via modifiers.
      return SCK_TAB;
    default:
      return unicodeChar;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Translate NSEvent modifier flags into SCI_* modifier flags.
 *
 * @param modifiers An integer bit set of NSSEvent modifier flags.
 * @return A set of SCI_* modifier flags.
 */
static int TranslateModifierFlags(NSUInteger modifiers)
{
  // Signal Control as SCI_META
  return
    (((modifiers & NSShiftKeyMask) != 0) ? SCI_SHIFT : 0) |
    (((modifiers & NSCommandKeyMask) != 0) ? SCI_CTRL : 0) |
    (((modifiers & NSAlternateKeyMask) != 0) ? SCI_ALT : 0) |
    (((modifiers & NSControlKeyMask) != 0) ? SCI_META : 0);
}

//--------------------------------------------------------------------------------------------------

/**
 * Main keyboard input handling method. It is called for any key down event, including function keys,
 * numeric keypad input and whatnot.
 *
 * @param event The event instance associated with the key down event.
 * @return True if the input was handled, false otherwise.
 */
bool ScintillaCocoa::KeyboardInput(NSEvent* event)
{
  // For now filter out function keys.
  NSString* input = [event characters];

  bool handled = false;

  // Handle each entry individually. Usually we only have one entry anyway.
  for (size_t i = 0; i < input.length; i++)
  {
    const UniChar originalKey = [input characterAtIndex: i];
    UniChar key = KeyTranslate(originalKey);

    bool consumed = false; // Consumed as command?

    if (KeyDownWithModifiers(key, TranslateModifierFlags([event modifierFlags]), &consumed))
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
int ScintillaCocoa::InsertText(NSString* input)
{
  CFStringEncoding encoding = EncodingFromCharacterSet(IsUnicodeMode(),
                                                       vs.styles[STYLE_DEFAULT].characterSet);
  std::string encoded = EncodedBytesString((CFStringRef)input, encoding);
  
  if (encoded.length() > 0)
  {
    AddCharUTF((char*) encoded.c_str(), static_cast<unsigned int>(encoded.length()), false);
  }
  return static_cast<int>(encoded.length());
}

//--------------------------------------------------------------------------------------------------

/**
 * Convert from a range of characters to a range of bytes.
 */
NSRange ScintillaCocoa::PositionsFromCharacters(NSRange range) const
{
  long start = pdoc->GetRelativePositionUTF16(0, range.location);
  if (start == INVALID_POSITION)
    start = pdoc->Length();
  long end = pdoc->GetRelativePositionUTF16(start, range.length);
  if (end == INVALID_POSITION)
    end = pdoc->Length();
  return NSMakeRange(start, end - start);
}

//--------------------------------------------------------------------------------------------------

/**
 * Convert from a range of characters from a range of bytes.
 */
NSRange ScintillaCocoa::CharactersFromPositions(NSRange range) const
{
  const long start = pdoc->CountUTF16(0, range.location);
  const long len = pdoc->CountUTF16(range.location, NSMaxRange(range));
  return NSMakeRange(start, len);
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to ensure that only one selection is active for input composition as composition
 * does not support multi-typing.
 */
void ScintillaCocoa::SelectOnlyMainSelection()
{
  sel.SetSelection(sel.RangeMain());
  Redraw();
}

//--------------------------------------------------------------------------------------------------

/**
 * Convert virtual space before selection into real space.
 */
void ScintillaCocoa::ConvertSelectionVirtualSpace()
{
  FillVirtualSpace();
}

//--------------------------------------------------------------------------------------------------

/**
 * Erase all selected text and return whether the selection is now empty.
 * The selection may not be empty if the selection contained protected text.
 */
bool ScintillaCocoa::ClearAllSelections()
{
  ClearSelection(true);
  return sel.Empty();
}

//--------------------------------------------------------------------------------------------------

/**
 * Start composing for IME.
 */
void ScintillaCocoa::CompositionStart()
{
  if (!sel.Empty())
  {
    NSLog(@"Selection not empty when starting composition");
  }
  pdoc->TentativeStart();
}

//--------------------------------------------------------------------------------------------------

/**
 * Commit the IME text.
 */
void ScintillaCocoa::CompositionCommit()
{
  pdoc->TentativeCommit();
  pdoc->decorations.SetCurrentIndicator(INDIC_IME);
  pdoc->DecorationFillRange(0, 0, pdoc->Length());
}

//--------------------------------------------------------------------------------------------------

/**
 * Remove the IME text.
 */
void ScintillaCocoa::CompositionUndo()
{
  pdoc->TentativeUndo();
}

//--------------------------------------------------------------------------------------------------
/**
 * When switching documents discard any incomplete character composition state as otherwise tries to
 * act on the new document.
 */
void ScintillaCocoa::SetDocPointer(Document *document)
{
  // Drop input composition.
  NSTextInputContext *inctxt = [NSTextInputContext currentInputContext];
  [inctxt discardMarkedText];
  SCIContentView *inner = ContentView();
  [inner unmarkText];
  Editor::SetDocPointer(document);
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by the owning view when the mouse pointer enters the control.
 */
void ScintillaCocoa::MouseEntered(NSEvent* event)
{
  if (!HaveMouseCapture())
  {
    WndProc(SCI_SETCURSOR, (long int)SC_CURSORNORMAL, 0);

    // Mouse location is given in screen coordinates and might also be outside of our bounds.
    Point location = ConvertPoint([event locationInWindow]);
    ButtonMove(location);
  }
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::MouseExited(NSEvent* /* event */)
{
  // Nothing to do here.
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::MouseDown(NSEvent* event)
{
  Point location = ConvertPoint([event locationInWindow]);
  NSTimeInterval time = [event timestamp];
  bool command = ([event modifierFlags] & NSCommandKeyMask) != 0;
  bool shift = ([event modifierFlags] & NSShiftKeyMask) != 0;
  bool alt = ([event modifierFlags] & NSAlternateKeyMask) != 0;

  ButtonDown(Point(location.x, location.y), (int) (time * 1000), shift, command, alt);
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::MouseMove(NSEvent* event)
{
  lastMouseEvent = event;

  ButtonMoveWithModifiers(ConvertPoint([event locationInWindow]), TranslateModifierFlags([event modifierFlags]));
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::MouseUp(NSEvent* event)
{
  NSTimeInterval time = [event timestamp];
  bool control = ([event modifierFlags] & NSControlKeyMask) != 0;

  ButtonUp(ConvertPoint([event locationInWindow]), (int) (time * 1000), control);
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::MouseWheel(NSEvent* event)
{
  bool command = ([event modifierFlags] & NSCommandKeyMask) != 0;
  int dY = 0;

    // In order to make scrolling with larger offset smoother we scroll less lines the larger the
    // delta value is.
    if ([event deltaY] < 0)
    dY = -(int) sqrt(-10.0 * [event deltaY]);
    else
    dY = (int) sqrt(10.0 * [event deltaY]);

  if (command)
  {
    // Zoom! We play with the font sizes in the styles.
    // Number of steps/line is ignored, we just care if sizing up or down.
    if (dY > 0.5)
      KeyCommand(SCI_ZOOMIN);
    else if (dY < -0.5)
      KeyCommand(SCI_ZOOMOUT);
  }
  else
  {
  }
}

//--------------------------------------------------------------------------------------------------

// Helper methods for NSResponder actions.

void ScintillaCocoa::SelectAll()
{
  Editor::SelectAll();
}

void ScintillaCocoa::DeleteBackward()
{
  KeyDown(SCK_BACK, false, false, false, nil);
}

void ScintillaCocoa::Cut()
{
  Editor::Cut();
}

void ScintillaCocoa::Undo()
{
  Editor::Undo();
}

void ScintillaCocoa::Redo()
{
  Editor::Redo();
}

//--------------------------------------------------------------------------------------------------

/**
 * Creates and returns a popup menu, which is then displayed by the Cocoa framework.
 */
NSMenu* ScintillaCocoa::CreateContextMenu(NSEvent* /* event */)
{
  // Call ScintillaBase to create the context menu.
  ContextMenu(Point(0, 0));

  return reinterpret_cast<NSMenu*>(popup.GetID());
}

//--------------------------------------------------------------------------------------------------

/**
 * An intermediate function to forward context menu commands from the menu action handler to
 * scintilla.
 */
void ScintillaCocoa::HandleCommand(NSInteger command)
{
  Command(static_cast<int>(command));
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::ActiveStateChanged(bool isActive)
{
  // If the window is being deactivated, lose the focus and turn off the ticking
  if (!isActive) {
    DropCaret();
    //SetFocusState( false );
    FineTickerCancel(tickCaret);
  } else {
    ShowCaretAtCurrentPosition();
  }
}

// If building with old SDK, need to define version number for 10.8
#ifndef NSAppKitVersionNumber10_8
#define NSAppKitVersionNumber10_8 1187
#endif

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::ShowFindIndicatorForRange(NSRange charRange, BOOL retaining)
{
#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_5
  NSView *content = ContentView();
  if (!layerFindIndicator)
  {
    layerFindIndicator = [[FindHighlightLayer alloc] init];
    [content setWantsLayer: YES];
    layerFindIndicator.geometryFlipped = content.layer.geometryFlipped;
    if (floor(NSAppKitVersionNumber) > NSAppKitVersionNumber10_8)
    {
      // Content layer is unflipped on 10.9, but the indicator shows wrong unless flipped
      layerFindIndicator.geometryFlipped = YES;
    }
    [[content layer] addSublayer:layerFindIndicator];
  }
  [layerFindIndicator removeAnimationForKey:@"animateFound"];

  if (charRange.length)
  {
    CFStringEncoding encoding = EncodingFromCharacterSet(IsUnicodeMode(),
							 vs.styles[STYLE_DEFAULT].characterSet);
    std::vector<char> buffer(charRange.length);
    pdoc->GetCharRange(&buffer[0], static_cast<int>(charRange.location), static_cast<int>(charRange.length));

    CFStringRef cfsFind = CFStringCreateWithBytes(kCFAllocatorDefault,
						  reinterpret_cast<const UInt8 *>(&buffer[0]),
						  charRange.length, encoding, false);
    layerFindIndicator.sFind = (NSString *)cfsFind;
    if (cfsFind)
        CFRelease(cfsFind);
    layerFindIndicator.retaining = retaining;
    layerFindIndicator.positionFind = static_cast<int>(charRange.location);
    long style = WndProc(SCI_GETSTYLEAT, charRange.location, 0);
    std::vector<char> bufferFontName(WndProc(SCI_STYLEGETFONT, style, 0) + 1);
    WndProc(SCI_STYLEGETFONT, style, (sptr_t)&bufferFontName[0]);
    layerFindIndicator.sFont = [NSString stringWithUTF8String: &bufferFontName[0]];

    layerFindIndicator.fontSize = WndProc(SCI_STYLEGETSIZEFRACTIONAL, style, 0) /
      (float)SC_FONT_SIZE_MULTIPLIER;
    layerFindIndicator.widthText = WndProc(SCI_POINTXFROMPOSITION, 0, charRange.location + charRange.length) -
      WndProc(SCI_POINTXFROMPOSITION, 0, charRange.location);
    layerFindIndicator.heightLine = WndProc(SCI_TEXTHEIGHT, 0, 0);
    MoveFindIndicatorWithBounce(YES);
  }
  else
  {
    [layerFindIndicator hideMatch];
  }
#endif
}

void ScintillaCocoa::MoveFindIndicatorWithBounce(BOOL bounce)
{
#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_5
  if (layerFindIndicator)
  {
    CGPoint ptText = CGPointMake(
      WndProc(SCI_POINTXFROMPOSITION, 0, layerFindIndicator.positionFind),
      WndProc(SCI_POINTYFROMPOSITION, 0, layerFindIndicator.positionFind));
    ptText.x = ptText.x - vs.fixedColumnWidth + xOffset;
    ptText.y += topLine * vs.lineHeight;
    if (!layerFindIndicator.geometryFlipped)
    {
      NSView *content = ContentView();
      ptText.y = content.bounds.size.height - ptText.y;
    }
    [layerFindIndicator animateMatch:ptText bounce:bounce];
  }
#endif
}

void ScintillaCocoa::HideFindIndicator()
{
#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_5
  if (layerFindIndicator)
  {
    [layerFindIndicator hideMatch];
  }
#endif
}


