// document_map.mm — Document Map (minimap) support.

#import <Cocoa/Cocoa.h>
#include "document_map.h"
#include "app_state.h"
#include "document_manager.h"
#include "lexer_styles.h"
#include "npp_constants.h"
#include "scintilla_bridge.h"
#include "split_view.h"
#include <algorithm>

@interface DocumentMapOverlayView : NSView
@end

namespace
{
static NSRect sViewportRect = NSZeroRect;

static void configureDocumentMapView(void* sci)
{
	if (!sci)
		return;

	ScintillaBridge_sendMessage(sci, SCI_SETREADONLY, 1, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETZOOM, -10, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETWRAPMODE, SC_WRAP_NONE, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETCARETLINEVISIBLE, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINWIDTHN, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINWIDTHN, 1, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETMARGINWIDTHN, 2, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETVIEWWS, SCWS_INVISIBLE, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETVIEWEOL, 0, 0);
	ScintillaBridge_sendMessage(sci, SCI_SETINDENTATIONGUIDES, SC_IV_NONE, 0);
}

static void jumpActiveViewToOverlayPoint(NSPoint p)
{
	void* active = ctx().activeScintillaView();
	if (!active || !ctx().documentMapEnabled || !ctx().documentMapContainer)
		return;

	NSView* mapView = ctx().documentMapContainer;
	CGFloat h = mapView.bounds.size.height;
	if (h <= 1)
		return;

	intptr_t lineCount = ScintillaBridge_sendMessage(active, SCI_GETLINECOUNT, 0, 0);
	if (lineCount <= 0)
		return;

	double ratioFromTop = 1.0 - (p.y / h);
	if (ratioFromTop < 0.0) ratioFromTop = 0.0;
	if (ratioFromTop > 1.0) ratioFromTop = 1.0;

	intptr_t targetDocLine = static_cast<intptr_t>(ratioFromTop * static_cast<double>(lineCount - 1));
	intptr_t visibleTarget = ScintillaBridge_sendMessage(active, SCI_VISIBLEFROMDOCLINE, targetDocLine, 0);
	intptr_t linesOnScreen = ScintillaBridge_sendMessage(active, SCI_LINESONSCREEN, 0, 0);
	if (linesOnScreen < 1) linesOnScreen = 1;
	intptr_t topVisible = visibleTarget - (linesOnScreen / 2);
	if (topVisible < 0) topVisible = 0;

	ScintillaBridge_sendMessage(active, SCI_SETFIRSTVISIBLELINE, topVisible, 0);
	updateDocumentMapViewport();
}
}

@implementation DocumentMapOverlayView

- (BOOL)isOpaque
{
	return NO;
}

- (void)drawRect:(NSRect)dirtyRect
{
	[[NSColor clearColor] setFill];
	NSRectFill(dirtyRect);

	if (NSIsEmptyRect(sViewportRect))
		return;

	[[[NSColor systemBlueColor] colorWithAlphaComponent:0.15] setFill];
	NSRectFill(sViewportRect);

	[[[NSColor systemBlueColor] colorWithAlphaComponent:0.45] setStroke];
	NSBezierPath* path = [NSBezierPath bezierPathWithRect:sViewportRect];
	[path setLineWidth:1.0];
	[path stroke];
}

- (void)mouseDown:(NSEvent*)event
{
	NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
	jumpActiveViewToOverlayPoint(p);
}

- (void)mouseDragged:(NSEvent*)event
{
	NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
	jumpActiveViewToOverlayPoint(p);
}

@end

void initializeDocumentMap()
{
	if (!ctx().mainWindow || !ctx().editorContainer || ctx().documentMapContainer)
		return;

	NSView* contentView = ctx().mainWindow.contentView;
	if (!contentView)
		return;

	CGFloat width = static_cast<CGFloat>(ctx().documentMapWidth);
	if (width < 80) width = 80;
	if (width > 280) width = 280;
	ctx().documentMapWidth = static_cast<int>(width);

	ctx().documentMapContainer = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, width, 100)];
	ctx().documentMapContainer.autoresizingMask = NSViewMinXMargin | NSViewHeightSizable;
	[contentView addSubview:ctx().documentMapContainer];

	ctx().documentMapScintilla = ScintillaBridge_createView((__bridge void*)ctx().documentMapContainer, 0, 0, 0, 0);
	if (!ctx().documentMapScintilla)
	{
		[ctx().documentMapContainer removeFromSuperview];
		ctx().documentMapContainer = nil;
		ctx().documentMapScintilla = nullptr;
		return;
	}

	configureDocumentMapView(ctx().documentMapScintilla);

	ctx().documentMapOverlay = [[DocumentMapOverlayView alloc] initWithFrame:ctx().documentMapContainer.bounds];
	ctx().documentMapOverlay.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
	[ctx().documentMapContainer addSubview:ctx().documentMapOverlay];

	relayoutDocumentMap();
}

void destroyDocumentMap()
{
	sViewportRect = NSZeroRect;

	if (ctx().documentMapScintilla)
	{
		ScintillaBridge_destroyView(ctx().documentMapScintilla);
		ctx().documentMapScintilla = nullptr;
	}

	if (ctx().documentMapOverlay)
	{
		[ctx().documentMapOverlay removeFromSuperview];
		ctx().documentMapOverlay = nil;
	}

	if (ctx().documentMapContainer)
	{
		[ctx().documentMapContainer removeFromSuperview];
		ctx().documentMapContainer = nil;
	}
}

void relayoutDocumentMap()
{
	if (!ctx().mainWindow || !ctx().editorContainer)
		return;

	NSView* contentView = ctx().mainWindow.contentView;
	if (!contentView)
		return;

	const CGFloat tabHeight = NPP_TAB_BAR_HEIGHT;
	const CGFloat statusHeight = NPP_STATUS_BAR_HEIGHT;
	NSRect baseEditorFrame = NSMakeRect(0, statusHeight,
	                                    contentView.bounds.size.width,
	                                    contentView.bounds.size.height - tabHeight - statusHeight);

	CGFloat mapWidth = ctx().documentMapEnabled ? static_cast<CGFloat>(ctx().documentMapWidth) : 0.0;
	if (mapWidth < 0) mapWidth = 0;
	if (mapWidth > baseEditorFrame.size.width - 120) mapWidth = std::max<CGFloat>(0, baseEditorFrame.size.width - 120);

	NSRect editorFrame = baseEditorFrame;
	editorFrame.size.width -= mapWidth;

	if (ctx().isSplit && ctx().splitView)
	{
		ctx().splitView.frame = editorFrame;
		[ctx().splitView adjustSubviews];
	}
	else
	{
		ctx().editorContainer.frame = editorFrame;
	}

	if (ctx().documentMapContainer)
	{
		NSRect mapFrame = NSMakeRect(NSMaxX(editorFrame), baseEditorFrame.origin.y, mapWidth, baseEditorFrame.size.height);
		ctx().documentMapContainer.frame = mapFrame;
		ctx().documentMapContainer.hidden = !ctx().documentMapEnabled;
	}

	layoutSplitTopTabBars();
	if (ctx().documentMapScintilla)
		ScintillaBridge_resizeToFit(ctx().documentMapScintilla);
	if (ctx().scintillaView)
		ScintillaBridge_resizeToFit(ctx().scintillaView);
	if (ctx().isSplit && ctx().scintillaView2)
		ScintillaBridge_resizeToFit(ctx().scintillaView2);
}

void setDocumentMapEnabled(bool enabled)
{
	ctx().documentMapEnabled = enabled;
	if (!ctx().documentMapContainer)
		initializeDocumentMap();

	relayoutDocumentMap();
	if (enabled)
	{
		bindDocumentMapToActiveView();
		updateDocumentMapViewport();
	}
}

bool isDocumentMapEnabled()
{
	return ctx().documentMapEnabled;
}

void bindDocumentMapToActiveView()
{
	if (!ctx().documentMapEnabled || !ctx().documentMapScintilla)
		return;

	void* active = ctx().activeScintillaView();
	if (!active)
		return;

	intptr_t docPtr = ScintillaBridge_sendMessage(active, SCI_GETDOCPOINTER, 0, 0);
	ScintillaBridge_sendMessage(ctx().documentMapScintilla, SCI_SETDOCPOINTER, 0, docPtr);
	ScintillaBridge_sendMessage(ctx().documentMapScintilla, SCI_SETREADONLY, 1, 0);

	auto& docs = ctx().activeDocuments();
	int tabIdx = ctx().activeTabIndex();
	if (tabIdx >= 0 && tabIdx < static_cast<int>(docs.size()))
		applyLanguageToView(ctx().documentMapScintilla, docs[tabIdx].languageIndex);
	configureDocumentMapView(ctx().documentMapScintilla);
}

void updateDocumentMapViewport()
{
	if (!ctx().documentMapEnabled || !ctx().documentMapOverlay || !ctx().documentMapContainer)
		return;

	void* active = ctx().activeScintillaView();
	if (!active)
		return;

	intptr_t lineCount = ScintillaBridge_sendMessage(active, SCI_GETLINECOUNT, 0, 0);
	if (lineCount <= 0)
	{
		sViewportRect = NSZeroRect;
		[ctx().documentMapOverlay setNeedsDisplay:YES];
		return;
	}

	intptr_t firstVisible = ScintillaBridge_sendMessage(active, SCI_GETFIRSTVISIBLELINE, 0, 0);
	intptr_t linesOnScreen = ScintillaBridge_sendMessage(active, SCI_LINESONSCREEN, 0, 0);
	if (linesOnScreen < 1) linesOnScreen = 1;
	intptr_t lastVisible = firstVisible + linesOnScreen;

	intptr_t topDocLine = ScintillaBridge_sendMessage(active, SCI_DOCLINEFROMVISIBLE, firstVisible, 0);
	intptr_t bottomDocLine = ScintillaBridge_sendMessage(active, SCI_DOCLINEFROMVISIBLE, lastVisible, 0);

	if (topDocLine < 0) topDocLine = 0;
	if (bottomDocLine < topDocLine) bottomDocLine = topDocLine;
	if (bottomDocLine >= lineCount) bottomDocLine = lineCount - 1;

	CGFloat h = ctx().documentMapContainer.bounds.size.height;
	CGFloat w = ctx().documentMapContainer.bounds.size.width;
	if (h <= 0 || w <= 0)
		return;

	double topRatio = static_cast<double>(topDocLine) / static_cast<double>(lineCount);
	double bottomRatio = static_cast<double>(bottomDocLine + 1) / static_cast<double>(lineCount);
	CGFloat topY = static_cast<CGFloat>((1.0 - topRatio) * h);
	CGFloat bottomY = static_cast<CGFloat>((1.0 - bottomRatio) * h);
	CGFloat rectHeight = topY - bottomY;
	if (rectHeight < 8.0) rectHeight = 8.0;
	if (bottomY + rectHeight > h) bottomY = h - rectHeight;
	if (bottomY < 0) bottomY = 0;

	sViewportRect = NSMakeRect(0, bottomY, w, rectHeight);
	[ctx().documentMapOverlay setNeedsDisplay:YES];
}

void handleDocumentMapUpdateUI(void* sourceSci, int updatedFlags)
{
	if (!ctx().documentMapEnabled)
		return;
	if (sourceSci != ctx().activeScintillaView())
		return;

	if (updatedFlags & (SC_UPDATE_SELECTION | SC_UPDATE_V_SCROLL | SC_UPDATE_H_SCROLL))
		updateDocumentMapViewport();
}
