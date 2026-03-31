// panel_layout.mm — Central panel layout orchestrator
// Positions all panels (left zone, editor, doc map, right zone) and dividers.
//
// Layout order (left to right):
//   [Left Zone] [|] [Editor] [DocMap] [|] [Right Zone]
//
// Left zone = File Browser (top) + File Switcher (bottom)
// Right zone = Function List (top) + Clipboard History (bottom)
// | = PanelDividerView (3px vertical dividers)

#import <Cocoa/Cocoa.h>
#include <algorithm>

#include "panel_layout.h"
#include "panel_divider.h"
#include "app_state.h"
#include "clipboard_history_panel.h"
#include "file_browser_panel.h"
#include "file_switcher_panel.h"
#include "function_list_panel.h"
#include "npp_constants.h"
#include "scintilla_bridge.h"
#include "split_view.h"

// ---------------------------------------------------------------------------
// Static divider views — created lazily on first use
// ---------------------------------------------------------------------------
static PanelDividerView* sLeftVerticalDivider = nil;
static PanelDividerView* sRightVerticalDivider = nil;
static PanelDividerView* sLeftHorizontalDivider = nil;
static PanelDividerView* sRightHorizontalDivider = nil;

// ---------------------------------------------------------------------------
// Divider creation helpers
// ---------------------------------------------------------------------------

static void ensureLeftVerticalDivider(NSView* contentView)
{
	if (sLeftVerticalDivider)
		return;

	sLeftVerticalDivider = createPanelDivider(
		DividerOrientation::Vertical,
		contentView,
		^(CGFloat delta) {
			ctx().leftPanelWidth += static_cast<int>(delta);
			if (ctx().leftPanelWidth < static_cast<int>(kPanelCollapseThreshold))
			{
				ctx().fileBrowserEnabled = false;
				ctx().fileSwitcherEnabled = false;
				ctx().leftPanelWidth = kDefaultLeftPanelWidth;
			}
			relayoutPanels();
		},
		^{
			ctx().leftPanelWidth = kDefaultLeftPanelWidth;
			relayoutPanels();
		}
	);
}

static void ensureRightVerticalDivider(NSView* contentView)
{
	if (sRightVerticalDivider)
		return;

	sRightVerticalDivider = createPanelDivider(
		DividerOrientation::Vertical,
		contentView,
		^(CGFloat delta) {
			// Dragging right = zone gets smaller (negative adjustment)
			ctx().rightPanelWidth -= static_cast<int>(delta);
			if (ctx().rightPanelWidth < static_cast<int>(kPanelCollapseThreshold))
			{
				ctx().functionListEnabled = false;
				ctx().clipboardHistoryEnabled = false;
				ctx().rightPanelWidth = kDefaultRightPanelWidth;
			}
			relayoutPanels();
		},
		^{
			ctx().rightPanelWidth = kDefaultRightPanelWidth;
			relayoutPanels();
		}
	);
}

static void ensureLeftHorizontalDivider(NSView* contentView)
{
	if (sLeftHorizontalDivider)
		return;

	sLeftHorizontalDivider = createPanelDivider(
		DividerOrientation::Horizontal,
		contentView,
		^(CGFloat delta) {
			// delta is in points; convert to ratio change based on zone height
			const CGFloat tabHeight = NPP_TAB_BAR_HEIGHT;
			const CGFloat statusHeight = NPP_STATUS_BAR_HEIGHT;
			CGFloat zoneHeight = contentView.bounds.size.height - tabHeight - statusHeight;
			if (zoneHeight > 0)
			{
				ctx().fileBrowserHeightRatio += static_cast<CGFloat>(delta) / zoneHeight;
				if (ctx().fileBrowserHeightRatio < 0.15)
					ctx().fileBrowserHeightRatio = 0.15;
				if (ctx().fileBrowserHeightRatio > 0.85)
					ctx().fileBrowserHeightRatio = 0.85;
			}
			relayoutPanels();
		},
		^{
			ctx().fileBrowserHeightRatio = 0.6;
			relayoutPanels();
		}
	);
}

static void ensureRightHorizontalDivider(NSView* contentView)
{
	if (sRightHorizontalDivider)
		return;

	sRightHorizontalDivider = createPanelDivider(
		DividerOrientation::Horizontal,
		contentView,
		^(CGFloat delta) {
			// delta is in points; convert to ratio change based on zone height
			const CGFloat tabHeight = NPP_TAB_BAR_HEIGHT;
			const CGFloat statusHeight = NPP_STATUS_BAR_HEIGHT;
			CGFloat zoneHeight = contentView.bounds.size.height - tabHeight - statusHeight;
			if (zoneHeight > 0)
			{
				ctx().functionListHeightRatio += static_cast<CGFloat>(delta) / zoneHeight;
				if (ctx().functionListHeightRatio < 0.15)
					ctx().functionListHeightRatio = 0.15;
				if (ctx().functionListHeightRatio > 0.85)
					ctx().functionListHeightRatio = 0.85;
			}
			relayoutPanels();
		},
		^{
			ctx().functionListHeightRatio = 0.5;
			relayoutPanels();
		}
	);
}

// ---------------------------------------------------------------------------
// relayoutPanels — main layout orchestrator
// ---------------------------------------------------------------------------

void relayoutPanels()
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

	// -- Calculate zone widths ------------------------------------------------

	bool leftZoneVisible = ctx().fileBrowserEnabled || ctx().fileSwitcherEnabled;
	bool rightZoneVisible = ctx().functionListEnabled || ctx().clipboardHistoryEnabled;

	CGFloat leftWidth = leftZoneVisible ? static_cast<CGFloat>(ctx().leftPanelWidth) : 0.0;
	CGFloat mapWidth = ctx().documentMapEnabled ? static_cast<CGFloat>(ctx().documentMapWidth) : 0.0;
	CGFloat rightWidth = rightZoneVisible ? static_cast<CGFloat>(ctx().rightPanelWidth) : 0.0;

	if (leftWidth < 0) leftWidth = 0;
	if (mapWidth < 0) mapWidth = 0;
	if (rightWidth < 0) rightWidth = 0;

	// Divider widths for visible zone boundaries
	CGFloat leftDividerWidth = leftZoneVisible ? kDividerThickness : 0.0;
	CGFloat rightDividerWidth = rightZoneVisible ? kDividerThickness : 0.0;

	// -- Clamp: total side panels cannot exceed (window width - min editor) ---

	CGFloat totalSide = leftWidth + leftDividerWidth + mapWidth + rightDividerWidth + rightWidth;
	CGFloat maxSide = std::max<CGFloat>(0.0, baseEditorFrame.size.width - kMinEditorWidth);

	if (totalSide > maxSide)
	{
		// Proportionally scale down widths
		CGFloat dividerTotal = leftDividerWidth + rightDividerWidth;
		CGFloat panelTotal = leftWidth + mapWidth + rightWidth;
		CGFloat available = maxSide - dividerTotal;
		if (available < 0) available = 0;

		if (panelTotal > 0)
		{
			CGFloat scale = available / panelTotal;
			leftWidth = std::max<CGFloat>(0.0, leftWidth * scale);
			mapWidth = std::max<CGFloat>(0.0, mapWidth * scale);
			rightWidth = std::max<CGFloat>(0.0, rightWidth * scale);

			if (leftZoneVisible)
				ctx().leftPanelWidth = static_cast<int>(leftWidth);
			if (ctx().documentMapEnabled)
				ctx().documentMapWidth = static_cast<int>(mapWidth);
			if (rightZoneVisible)
				ctx().rightPanelWidth = static_cast<int>(rightWidth);
		}
	}

	// -- Position left-to-right -----------------------------------------------

	CGFloat curX = baseEditorFrame.origin.x;
	CGFloat zoneY = baseEditorFrame.origin.y;
	CGFloat zoneH = baseEditorFrame.size.height;

	// 1. Left zone
	if (leftZoneVisible)
	{
		ensureLeftVerticalDivider(contentView);

		NSView* fbContainer = (__bridge NSView*)fileBrowserContainerView();
		NSView* fsContainer = (__bridge NSView*)fileSwitcherContainerView();
		bool hasBoth = (fbContainer != nil) && (fsContainer != nil) &&
		               ctx().fileBrowserEnabled && ctx().fileSwitcherEnabled;

		if (hasBoth)
		{
			ensureLeftHorizontalDivider(contentView);

			CGFloat topH = (zoneH - kDividerThickness) * ctx().fileBrowserHeightRatio;
			CGFloat bottomH = zoneH - kDividerThickness - topH;

			// File browser on top
			fbContainer.frame = NSMakeRect(curX, zoneY + bottomH + kDividerThickness, leftWidth, topH);
			fbContainer.hidden = NO;

			// Horizontal divider
			sLeftHorizontalDivider.frame = NSMakeRect(curX, zoneY + bottomH, leftWidth, kDividerThickness);
			sLeftHorizontalDivider.hidden = NO;

			// File switcher on bottom
			fsContainer.frame = NSMakeRect(curX, zoneY, leftWidth, bottomH);
			fsContainer.hidden = NO;
		}
		else
		{
			// Single panel fills the entire zone; hide the other
			if (fbContainer)
			{
				if (ctx().fileBrowserEnabled)
				{
					fbContainer.frame = NSMakeRect(curX, zoneY, leftWidth, zoneH);
					fbContainer.hidden = NO;
				}
				else
				{
					fbContainer.hidden = YES;
				}
			}
			if (fsContainer)
			{
				if (ctx().fileSwitcherEnabled)
				{
					fsContainer.frame = NSMakeRect(curX, zoneY, leftWidth, zoneH);
					fsContainer.hidden = NO;
				}
				else
				{
					fsContainer.hidden = YES;
				}
			}
			if (sLeftHorizontalDivider)
				sLeftHorizontalDivider.hidden = YES;
		}

		curX += leftWidth;

		// Left vertical divider
		sLeftVerticalDivider.frame = NSMakeRect(curX, zoneY, kDividerThickness, zoneH);
		sLeftVerticalDivider.hidden = NO;
		curX += kDividerThickness;
	}
	else
	{
		// Hide left-zone elements
		NSView* fbContainer = (__bridge NSView*)fileBrowserContainerView();
		NSView* fsContainer = (__bridge NSView*)fileSwitcherContainerView();
		if (fbContainer) fbContainer.hidden = YES;
		if (fsContainer) fsContainer.hidden = YES;
		if (sLeftVerticalDivider) sLeftVerticalDivider.hidden = YES;
		if (sLeftHorizontalDivider) sLeftHorizontalDivider.hidden = YES;
	}

	// 2. Editor (+ doc map glued to right edge)
	CGFloat editorWidth = baseEditorFrame.size.width - leftWidth - leftDividerWidth
	                      - mapWidth - rightDividerWidth - rightWidth;
	if (editorWidth < kMinEditorWidth)
		editorWidth = kMinEditorWidth;

	NSRect editorFrame = NSMakeRect(curX, zoneY, editorWidth, zoneH);

	if (ctx().isSplit && ctx().splitView)
	{
		ctx().splitView.frame = editorFrame;
		[ctx().splitView adjustSubviews];
	}
	else
	{
		ctx().editorContainer.frame = editorFrame;
	}

	curX += editorWidth;

	// 3. Document Map (glued to editor right edge, no divider)
	if (ctx().documentMapContainer)
	{
		NSRect mapFrame = NSMakeRect(curX, zoneY, mapWidth, zoneH);
		ctx().documentMapContainer.frame = mapFrame;
		ctx().documentMapContainer.hidden = !ctx().documentMapEnabled;
		curX += mapWidth;
	}

	// 4. Right zone
	if (rightZoneVisible)
	{
		ensureRightVerticalDivider(contentView);

		// Right vertical divider
		sRightVerticalDivider.frame = NSMakeRect(curX, zoneY, kDividerThickness, zoneH);
		sRightVerticalDivider.hidden = NO;
		curX += kDividerThickness;

		NSView* flContainer = (__bridge NSView*)functionListContainerView();
		NSView* chContainer = (__bridge NSView*)clipboardHistoryContainerView();
		bool hasBoth = (flContainer != nil) && (chContainer != nil) &&
		               ctx().functionListEnabled && ctx().clipboardHistoryEnabled;

		if (hasBoth)
		{
			ensureRightHorizontalDivider(contentView);

			CGFloat topH = (zoneH - kDividerThickness) * ctx().functionListHeightRatio;
			CGFloat bottomH = zoneH - kDividerThickness - topH;

			// Function list on top
			flContainer.frame = NSMakeRect(curX, zoneY + bottomH + kDividerThickness, rightWidth, topH);
			flContainer.hidden = NO;

			// Horizontal divider
			sRightHorizontalDivider.frame = NSMakeRect(curX, zoneY + bottomH, rightWidth, kDividerThickness);
			sRightHorizontalDivider.hidden = NO;

			// Clipboard history on bottom
			chContainer.frame = NSMakeRect(curX, zoneY, rightWidth, bottomH);
			chContainer.hidden = NO;
		}
		else
		{
			// Single panel fills the entire right zone
			if (flContainer && ctx().functionListEnabled)
			{
				flContainer.frame = NSMakeRect(curX, zoneY, rightWidth, zoneH);
				flContainer.hidden = NO;
			}
			else if (flContainer)
			{
				flContainer.hidden = YES;
			}

			if (chContainer && ctx().clipboardHistoryEnabled)
			{
				chContainer.frame = NSMakeRect(curX, zoneY, rightWidth, zoneH);
				chContainer.hidden = NO;
			}
			else if (chContainer)
			{
				chContainer.hidden = YES;
			}

			if (sRightHorizontalDivider)
				sRightHorizontalDivider.hidden = YES;
		}
	}
	else
	{
		// Hide right-zone elements
		NSView* flContainer = (__bridge NSView*)functionListContainerView();
		NSView* chContainer = (__bridge NSView*)clipboardHistoryContainerView();
		if (flContainer) flContainer.hidden = !ctx().functionListEnabled;
		if (chContainer) chContainer.hidden = !ctx().clipboardHistoryEnabled;
		if (sRightVerticalDivider) sRightVerticalDivider.hidden = YES;
		if (sRightHorizontalDivider) sRightHorizontalDivider.hidden = YES;
	}

	// -- Finalize: tab bars and Scintilla resize ------------------------------

	layoutSplitTopTabBars();
	if (ctx().documentMapScintilla)
		ScintillaBridge_resizeToFit(ctx().documentMapScintilla);
	if (ctx().scintillaView)
		ScintillaBridge_resizeToFit(ctx().scintillaView);
	if (ctx().isSplit && ctx().scintillaView2)
		ScintillaBridge_resizeToFit(ctx().scintillaView2);
}
