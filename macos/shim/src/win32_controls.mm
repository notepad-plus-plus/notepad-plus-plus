// Win32 Controls Shim: Tab, StatusBar, Toolbar, ReBar, ImageList for macOS
// Phase 3: Real NSView-backed tab control and status bar.
// Toolbar and ReBar are data-only (N++ does custom drawing).

#import <Cocoa/Cocoa.h>
#include "windows.h"
#include "commctrl.h"
#include "handle_registry.h"
#include "win32_string_helpers.h"
#include "win32_controls_impl.h"
#include "win32_tab_control_impl.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

// ============================================================
// Control type detection
// ============================================================

bool Win32Controls_IsControlClass(const std::wstring& className)
{
	return Win32Controls_GetControlType(className) != ControlType::None;
}

ControlType Win32Controls_GetControlType(const std::wstring& className)
{
	if (className == L"SysTabControl32") return ControlType::TabControl;
	if (className == L"msctls_statusbar32") return ControlType::StatusBar;
	if (className == L"ToolbarWindow32") return ControlType::Toolbar;
	if (className == L"ReBarWindow32") return ControlType::ReBar;
	if (className == L"tooltips_class32") return ControlType::Tooltip;
	if (className == L"SysListView32") return ControlType::ListView;
	if (className == L"SysTreeView32") return ControlType::TreeView;
	if (className == L"Button" || className == L"button") return ControlType::Button;
	if (className == L"Edit" || className == L"edit") return ControlType::Edit;
	if (className == L"Static" || className == L"static") return ControlType::Static;
	if (className == L"ComboBox" || className == L"combobox") return ControlType::ComboBox;
	if (className == L"ListBox" || className == L"listbox") return ControlType::ListBox;
	if (className == L"msctls_updown32") return ControlType::UpDown;
	return ControlType::None;
}

// ============================================================
// StatusBar data
// ============================================================

// Custom NSView that draws a simple status bar with partitions
@interface Win32StatusBarView : NSView
@property (strong) NSMutableArray<NSString*>* partTexts;
@property (strong) NSMutableArray<NSNumber*>* partWidths;
@property (assign) int partCount;
@end

@implementation Win32StatusBarView

- (instancetype)initWithFrame:(NSRect)frame
{
	self = [super initWithFrame:frame];
	if (self)
	{
		_partTexts = [NSMutableArray array];
		_partWidths = [NSMutableArray array];
		_partCount = 0;
	}
	return self;
}

- (BOOL)isFlipped
{
	return YES; // Use top-left origin like Win32
}

- (void)drawRect:(NSRect)dirtyRect
{
	// Background
	[[NSColor windowBackgroundColor] setFill];
	NSRectFill(self.bounds);

	// Top border line
	[[NSColor separatorColor] setStroke];
	NSBezierPath* topLine = [NSBezierPath bezierPath];
	[topLine moveToPoint:NSMakePoint(0, 0)];
	[topLine lineToPoint:NSMakePoint(self.bounds.size.width, 0)];
	[topLine stroke];

	if (_partCount == 0) return;

	NSDictionary* attrs = @{
		NSFontAttributeName: [NSFont systemFontOfSize:11],
		NSForegroundColorAttributeName: [NSColor labelColor]
	};

	CGFloat x = 4;
	CGFloat totalWidth = self.bounds.size.width;

	for (int i = 0; i < _partCount; ++i)
	{
		// Calculate part width
		CGFloat partWidth;
		if (i < (int)_partWidths.count)
		{
			int w = _partWidths[i].intValue;
			if (w < 0 || i == _partCount - 1)
				partWidth = totalWidth - x; // last part or -1 fills remaining
			else
				partWidth = w;
		}
		else
		{
			partWidth = totalWidth - x;
		}

		// Draw separator
		if (i > 0)
		{
			[[NSColor separatorColor] setStroke];
			NSBezierPath* sep = [NSBezierPath bezierPath];
			[sep moveToPoint:NSMakePoint(x - 2, 3)];
			[sep lineToPoint:NSMakePoint(x - 2, self.bounds.size.height - 3)];
			[sep stroke];
		}

		// Draw text
		if (i < (int)_partTexts.count)
		{
			NSString* text = _partTexts[i];
			if (text.length > 0)
			{
				NSRect textRect = NSMakeRect(x + 2, 2, partWidth - 6,
				                             self.bounds.size.height - 4);
				[text drawInRect:textRect withAttributes:attrs];
			}
		}

		x += partWidth;
	}
}

@end

struct StatusBarData
{
	HWND hwnd = nullptr;
	std::vector<int> partWidths;
	std::vector<std::wstring> partTexts;
};

static std::unordered_map<uintptr_t, StatusBarData> s_statusBars;

// ============================================================
// Toolbar data (data-only, no visual rendering)
// ============================================================

struct ToolbarButtonData
{
	int iBitmap = 0;
	int idCommand = 0;
	BYTE fsState = TBSTATE_ENABLED;
	BYTE fsStyle = TBSTYLE_BUTTON;
	DWORD_PTR dwData = 0;
	INT_PTR iString = 0;
};

struct ToolbarData
{
	HWND hwnd = nullptr;
	std::vector<ToolbarButtonData> buttons;
	int buttonSizeCx = 24;
	int buttonSizeCy = 22;
};

static std::unordered_map<uintptr_t, ToolbarData> s_toolBars;

// ============================================================
// ObjC helper: Routes NSButton click → WM_COMMAND/BN_CLICKED to parent
// ============================================================

@interface Win32ButtonTarget : NSObject
@property (assign) HWND buttonHwnd;
- (void)buttonClicked:(id)sender;
@end

@implementation Win32ButtonTarget
- (void)buttonClicked:(id)sender
{
	auto* info = HandleRegistry::getWindowInfo(self.buttonHwnd);
	if (!info) return;

	HWND parentHwnd = info->parent;
	if (parentHwnd)
	{
		auto* parentInfo = HandleRegistry::getWindowInfo(parentHwnd);
		if (parentInfo && parentInfo->wndProc)
		{
			WPARAM wp = MAKEWPARAM(info->controlId, BN_CLICKED);
			parentInfo->wndProc(parentHwnd, WM_COMMAND, wp,
			                    reinterpret_cast<LPARAM>(self.buttonHwnd));
		}
	}
}
@end

static NSMutableDictionary<NSNumber*, Win32ButtonTarget*>* s_buttonTargets = nil;

// ============================================================
// ObjC helper: Routes NSTextField changes → WM_COMMAND/EN_CHANGE to parent
// ============================================================

@interface Win32EditTarget : NSObject <NSTextFieldDelegate>
@property (assign) HWND editHwnd;
@end

@implementation Win32EditTarget
- (void)controlTextDidChange:(NSNotification*)notification
{
	auto* info = HandleRegistry::getWindowInfo(self.editHwnd);
	if (!info) return;

	// Update the windowName in the registry
	NSTextField* tf = (__bridge NSTextField*)info->nativeView;
	if (tf)
	{
		NSString* text = tf.stringValue;
		NSData* data = [text dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
		if (data && data.length >= sizeof(wchar_t))
			info->windowName = std::wstring(reinterpret_cast<const wchar_t*>(data.bytes),
			                                data.length / sizeof(wchar_t));
		else
			info->windowName.clear();
	}

	HWND parentHwnd = info->parent;
	if (parentHwnd)
	{
		auto* parentInfo = HandleRegistry::getWindowInfo(parentHwnd);
		if (parentInfo && parentInfo->wndProc)
		{
			WPARAM wp = MAKEWPARAM(info->controlId, EN_CHANGE);
			parentInfo->wndProc(parentHwnd, WM_COMMAND, wp,
			                    reinterpret_cast<LPARAM>(self.editHwnd));
		}
	}
}
@end

static NSMutableDictionary<NSNumber*, Win32EditTarget*>* s_editTargets = nil;

// ============================================================
// ReBar data (data-only)
// ============================================================

struct ReBarBandData
{
	UINT fMask = 0;
	UINT fStyle = 0;
	HWND hwndChild = nullptr;
	UINT cxMinChild = 0;
	UINT cyMinChild = 0;
	UINT cx = 0;
	UINT wID = 0;
	std::wstring text;
};

struct ReBarData
{
	HWND hwnd = nullptr;
	std::vector<ReBarBandData> bands;
};

static std::unordered_map<uintptr_t, ReBarData> s_reBars;

// ============================================================
// Control creation
// ============================================================

void* Win32Controls_CreateControl(ControlType type, void* parentView,
                                   int x, int y, int width, int height,
                                   unsigned long style, unsigned long exStyle)
{
	NSView* parent = (__bridge NSView*)parentView;
	if (!parent) return nullptr;

	CGFloat parentH = parent.bounds.size.height;

	switch (type)
	{
		case ControlType::TabControl:
		{
			// NSSegmentedControl for tabs
			NSRect frame = NSMakeRect(x, parentH - y - height, width, 28);
			NSSegmentedControl* seg = [[NSSegmentedControl alloc] initWithFrame:frame];
			seg.segmentStyle = NSSegmentStyleAutomatic;
			seg.trackingMode = NSSegmentSwitchTrackingSelectOne;
			seg.segmentCount = 0;
			seg.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
			[parent addSubview:seg];
			return (__bridge void*)seg;
		}

		case ControlType::StatusBar:
		{
			// Custom status bar view at bottom
			CGFloat barHeight = 22;
			NSRect frame = NSMakeRect(0, 0, parent.bounds.size.width, barHeight);
			Win32StatusBarView* bar = [[Win32StatusBarView alloc] initWithFrame:frame];
			bar.autoresizingMask = NSViewWidthSizable | NSViewMaxYMargin;
			[parent addSubview:bar];
			return (__bridge void*)bar;
		}

		case ControlType::Button:
		{
			NSRect frame = NSMakeRect(x, parentH - y - height, width, height);
			NSButton* btn = [[NSButton alloc] initWithFrame:frame];

			unsigned long btnType = style & BS_TYPEMASK;

			if (btnType == BS_GROUPBOX)
			{
				btn.bezelStyle = NSBezelStyleSmallSquare;
				[btn setButtonType:NSButtonTypeMomentaryLight];
				btn.bordered = NO;
				btn.title = @"";
				// Group box: use an NSBox instead
				NSBox* box = [[NSBox alloc] initWithFrame:frame];
				box.boxType = NSBoxPrimary;
				box.titlePosition = NSAtTop;
				box.title = @"";
				[parent addSubview:box];
				return (__bridge void*)box;
			}
			else if (btnType == BS_AUTOCHECKBOX || btnType == BS_CHECKBOX ||
			         btnType == BS_AUTO3STATE || btnType == BS_3STATE)
			{
				[btn setButtonType:NSButtonTypeSwitch];
				btn.bezelStyle = NSBezelStyleRegularSquare;
			}
			else if (btnType == BS_AUTORADIOBUTTON || btnType == BS_RADIOBUTTON)
			{
				[btn setButtonType:NSButtonTypeRadio];
				btn.bezelStyle = NSBezelStyleRegularSquare;
			}
			else
			{
				// Push button (BS_PUSHBUTTON or BS_DEFPUSHBUTTON)
				[btn setButtonType:NSButtonTypeMomentaryPushIn];
				btn.bezelStyle = NSBezelStyleRounded;
				if (btnType == BS_DEFPUSHBUTTON)
					btn.keyEquivalent = @"\r";
			}

			btn.title = @"";
			[parent addSubview:btn];
			return (__bridge void*)btn;
		}

		case ControlType::Edit:
		{
			NSRect frame = NSMakeRect(x, parentH - y - height, width, height);

			if (style & ES_MULTILINE)
			{
				// Multi-line edit: NSScrollView wrapping NSTextView
				NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:frame];
				NSTextView* textView = [[NSTextView alloc] initWithFrame:
					NSMakeRect(0, 0, width, height)];
				textView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
				textView.editable = !(style & ES_READONLY);
				textView.font = [NSFont systemFontOfSize:13];
				scrollView.documentView = textView;
				scrollView.hasVerticalScroller = YES;
				scrollView.hasHorizontalScroller = (style & ES_AUTOHSCROLL) ? YES : NO;
				scrollView.borderType = NSBezelBorder;
				[parent addSubview:scrollView];
				return (__bridge void*)scrollView;
			}
			else
			{
				// Single-line edit: NSTextField
				NSTextField* tf = [[NSTextField alloc] initWithFrame:frame];
				tf.editable = !(style & ES_READONLY);
				tf.bordered = YES;
				tf.bezeled = YES;
				tf.bezelStyle = NSTextFieldSquareBezel;
				tf.font = [NSFont systemFontOfSize:13];
				if (style & ES_PASSWORD)
				{
					NSSecureTextField* stf = [[NSSecureTextField alloc] initWithFrame:frame];
					stf.font = [NSFont systemFontOfSize:13];
					[parent addSubview:stf];
					return (__bridge void*)stf;
				}
				[parent addSubview:tf];
				return (__bridge void*)tf;
			}
		}

		case ControlType::Static:
		{
			NSRect frame = NSMakeRect(x, parentH - y - height, width, height);
			NSTextField* label = [[NSTextField alloc] initWithFrame:frame];
			label.editable = NO;
			label.bordered = NO;
			label.drawsBackground = NO;
			label.selectable = NO;
			label.font = [NSFont systemFontOfSize:13];
			label.lineBreakMode = NSLineBreakByTruncatingTail;
			[parent addSubview:label];
			return (__bridge void*)label;
		}

		case ControlType::ComboBox:
		{
			NSRect frame = NSMakeRect(x, parentH - y - height, width, height);
			if (style & CBS_DROPDOWNLIST)
			{
				// Drop-down list (non-editable): NSPopUpButton
				NSPopUpButton* popup = [[NSPopUpButton alloc] initWithFrame:frame pullsDown:NO];
				popup.font = [NSFont systemFontOfSize:13];
				[parent addSubview:popup];
				return (__bridge void*)popup;
			}
			else
			{
				// Editable combo box: NSComboBox
				NSComboBox* combo = [[NSComboBox alloc] initWithFrame:frame];
				combo.font = [NSFont systemFontOfSize:13];
				combo.usesDataSource = NO;
				[parent addSubview:combo];
				return (__bridge void*)combo;
			}
		}

		case ControlType::ListBox:
		{
			NSRect frame = NSMakeRect(x, parentH - y - height, width, height);
			NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:frame];
			NSTableView* tableView = [[NSTableView alloc] initWithFrame:
				NSMakeRect(0, 0, width, height)];
			NSTableColumn* col = [[NSTableColumn alloc] initWithIdentifier:@"main"];
			col.width = width - 20;
			[tableView addTableColumn:col];
			tableView.headerView = nil;
			scrollView.documentView = tableView;
			scrollView.hasVerticalScroller = YES;
			scrollView.borderType = NSBezelBorder;
			[parent addSubview:scrollView];
			return (__bridge void*)scrollView;
		}

		case ControlType::Toolbar:
		case ControlType::ReBar:
		case ControlType::Tooltip:
		case ControlType::ListView:
		case ControlType::TreeView:
		case ControlType::UpDown:
		{
			// Data-only: create a hidden placeholder view
			NSRect frame = NSMakeRect(x, parentH - y - height, width, height);
			NSView* view = [[NSView alloc] initWithFrame:frame];
			[view setHidden:YES];
			[parent addSubview:view];
			return (__bridge void*)view;
		}

		default:
			return nullptr;
	}
}

// ============================================================
// StatusBar message handling
// ============================================================

static bool HandleStatusBarMessage(HWND hwnd, unsigned int msg, uintptr_t wParam, intptr_t lParam,
                                    intptr_t& result)
{
	uintptr_t key = reinterpret_cast<uintptr_t>(hwnd);
	auto it = s_statusBars.find(key);
	if (it == s_statusBars.end()) return false;

	auto& sb = it->second;
	auto* info = HandleRegistry::getWindowInfo(hwnd);
	Win32StatusBarView* barView = info ? (__bridge Win32StatusBarView*)info->nativeView : nil;

	switch (msg)
	{
		case SB_SETPARTS:
		{
			int count = static_cast<int>(wParam);
			const int* edges = reinterpret_cast<const int*>(lParam);
			sb.partWidths.clear();
			sb.partTexts.resize(count);

			if (barView)
			{
				barView.partCount = count;
				[barView.partWidths removeAllObjects];
				[barView.partTexts removeAllObjects];
			}

			// Win32 SB_SETPARTS takes cumulative right-edge coordinates.
			// Convert to per-part widths for internal storage.
			int prevRight = 0;
			for (int i = 0; i < count; ++i)
			{
				int partWidth = -1;
				if (edges)
				{
					int edge = edges[i];
					if (edge == -1)
					{
						partWidth = -1; // fills remaining space
					}
					else
					{
						partWidth = edge - prevRight;
						if (partWidth < 0)
							partWidth = 0;
						prevRight = edge;
					}
				}
				sb.partWidths.push_back(partWidth);
				if (barView)
				{
					[barView.partWidths addObject:@(partWidth)];
					[barView.partTexts addObject:@""];
				}
			}

			if (barView) [barView setNeedsDisplay:YES];
			result = TRUE;
			return true;
		}

		case SB_SETTEXTW:
		{
			int part = static_cast<int>(wParam & 0xFF);
			// bits 8-15 are drawing type (SBT_OWNERDRAW etc.)
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);

			if (part >= 0 && part < static_cast<int>(sb.partTexts.size()))
			{
				sb.partTexts[part] = text ? text : L"";
				if (barView && part < (int)barView.partTexts.count)
				{
					barView.partTexts[part] = WideToNS(text);
					[barView setNeedsDisplay:YES];
				}
			}
			result = TRUE;
			return true;
		}

		case SB_GETTEXTW:
		{
			int part = static_cast<int>(wParam);
			wchar_t* buf = reinterpret_cast<wchar_t*>(lParam);
			if (part >= 0 && part < static_cast<int>(sb.partTexts.size()) && buf)
			{
				wcscpy(buf, sb.partTexts[part].c_str());
				result = static_cast<intptr_t>(sb.partTexts[part].size());
			}
			else
			{
				result = 0;
			}
			return true;
		}

		case SB_GETTEXTLENGTHW:
		{
			int part = static_cast<int>(wParam);
			if (part >= 0 && part < static_cast<int>(sb.partTexts.size()))
			{
				// Win32: LOWORD = text length, HIWORD = drawing type flags
				// We always use type 0 (no owner-draw), so high word is 0.
				result = static_cast<intptr_t>(MAKELONG(sb.partTexts[part].size(), 0));
			}
			else
				result = 0;
			return true;
		}

		case SB_GETPARTS:
		{
			int maxParts = static_cast<int>(wParam);
			int* widths = reinterpret_cast<int*>(lParam);
			int count = static_cast<int>(sb.partWidths.size());
			if (widths)
			{
				int n = (std::min)(maxParts, count);
				for (int i = 0; i < n; ++i)
					widths[i] = sb.partWidths[i];
			}
			result = count;
			return true;
		}

		case SB_GETRECT:
		{
			int part = static_cast<int>(wParam);
			RECT* pRect = reinterpret_cast<RECT*>(lParam);
			if (pRect && barView)
			{
				CGFloat x = 0;
				for (int i = 0; i < part && i < static_cast<int>(sb.partWidths.size()); ++i)
				{
					int w = sb.partWidths[i];
					x += (w > 0) ? w : (barView.bounds.size.width - x);
				}
				int w = (part < static_cast<int>(sb.partWidths.size())) ? sb.partWidths[part] : -1;
				CGFloat width = (w > 0) ? w : (barView.bounds.size.width - x);
				pRect->left = static_cast<LONG>(x);
				pRect->top = 0;
				pRect->right = static_cast<LONG>(x + width);
				pRect->bottom = static_cast<LONG>(barView.bounds.size.height);
				result = TRUE;
			}
			else
			{
				result = FALSE;
			}
			return true;
		}

		case SB_SETMINHEIGHT:
		{
			if (barView)
			{
				CGFloat h = static_cast<CGFloat>(wParam);
				NSRect f = barView.frame;
				f.size.height = h;
				barView.frame = f;
			}
			result = 0;
			return true;
		}

		case SB_SIMPLE:
		case SB_ISSIMPLE:
		case SB_SETICON:
		case SB_SETTIPTEXTW:
		case SB_GETTIPTEXTW:
		case SB_GETBORDERS:
			result = 0;
			return true;
	}

	return false;
}

// ============================================================
// Toolbar message handling (data-only)
// ============================================================

static bool HandleToolbarMessage(HWND hwnd, unsigned int msg, uintptr_t wParam, intptr_t lParam,
                                  intptr_t& result)
{
	uintptr_t key = reinterpret_cast<uintptr_t>(hwnd);
	auto it = s_toolBars.find(key);
	if (it == s_toolBars.end()) return false;

	auto& tb = it->second;

	switch (msg)
	{
		case TB_BUTTONSTRUCTSIZE:
			result = 0;
			return true;

		case TB_ADDBUTTONS:
		{
			int count = static_cast<int>(wParam);
			const TBBUTTON* buttons = reinterpret_cast<const TBBUTTON*>(lParam);
			if (buttons)
			{
				for (int i = 0; i < count; ++i)
				{
					ToolbarButtonData btn;
					btn.iBitmap = buttons[i].iBitmap;
					btn.idCommand = buttons[i].idCommand;
					btn.fsState = buttons[i].fsState;
					btn.fsStyle = buttons[i].fsStyle;
					btn.dwData = buttons[i].dwData;
					btn.iString = buttons[i].iString;
					tb.buttons.push_back(btn);
				}
			}
			result = TRUE;
			return true;
		}

		case TB_INSERTBUTTONW:
		{
			int index = static_cast<int>(wParam);
			const TBBUTTON* pBtn = reinterpret_cast<const TBBUTTON*>(lParam);
			if (pBtn)
			{
				ToolbarButtonData btn;
				btn.iBitmap = pBtn->iBitmap;
				btn.idCommand = pBtn->idCommand;
				btn.fsState = pBtn->fsState;
				btn.fsStyle = pBtn->fsStyle;
				btn.dwData = pBtn->dwData;
				btn.iString = pBtn->iString;

				if (index < 0 || index > static_cast<int>(tb.buttons.size()))
					index = static_cast<int>(tb.buttons.size());
				tb.buttons.insert(tb.buttons.begin() + index, btn);
			}
			result = TRUE;
			return true;
		}

		case TB_DELETEBUTTON:
		{
			int index = static_cast<int>(wParam);
			if (index >= 0 && index < static_cast<int>(tb.buttons.size()))
			{
				tb.buttons.erase(tb.buttons.begin() + index);
				result = TRUE;
			}
			else
			{
				result = FALSE;
			}
			return true;
		}

		case TB_BUTTONCOUNT:
			result = static_cast<intptr_t>(tb.buttons.size());
			return true;

		case TB_GETBUTTON:
		{
			int index = static_cast<int>(wParam);
			TBBUTTON* pBtn = reinterpret_cast<TBBUTTON*>(lParam);
			if (pBtn && index >= 0 && index < static_cast<int>(tb.buttons.size()))
			{
				const auto& btn = tb.buttons[index];
				pBtn->iBitmap = btn.iBitmap;
				pBtn->idCommand = btn.idCommand;
				pBtn->fsState = btn.fsState;
				pBtn->fsStyle = btn.fsStyle;
				pBtn->dwData = btn.dwData;
				pBtn->iString = btn.iString;
				result = TRUE;
			}
			else
			{
				result = FALSE;
			}
			return true;
		}

		case TB_COMMANDTOINDEX:
		{
			int cmdId = static_cast<int>(wParam);
			for (int i = 0; i < static_cast<int>(tb.buttons.size()); ++i)
			{
				if (tb.buttons[i].idCommand == cmdId)
				{
					result = i;
					return true;
				}
			}
			result = -1;
			return true;
		}

		case TB_ENABLEBUTTON:
		{
			int cmdId = static_cast<int>(wParam);
			bool enable = lParam != 0;
			for (auto& btn : tb.buttons)
			{
				if (btn.idCommand == cmdId)
				{
					if (enable)
						btn.fsState |= TBSTATE_ENABLED;
					else
						btn.fsState &= ~TBSTATE_ENABLED;
					break;
				}
			}
			result = TRUE;
			return true;
		}

		case TB_CHECKBUTTON:
		{
			int cmdId = static_cast<int>(wParam);
			bool check = lParam != 0;
			for (auto& btn : tb.buttons)
			{
				if (btn.idCommand == cmdId)
				{
					if (check)
						btn.fsState |= TBSTATE_CHECKED;
					else
						btn.fsState &= ~TBSTATE_CHECKED;
					break;
				}
			}
			result = TRUE;
			return true;
		}

		case TB_HIDEBUTTON:
		{
			int cmdId = static_cast<int>(wParam);
			bool hide = lParam != 0;
			for (auto& btn : tb.buttons)
			{
				if (btn.idCommand == cmdId)
				{
					if (hide)
						btn.fsState |= TBSTATE_HIDDEN;
					else
						btn.fsState &= ~TBSTATE_HIDDEN;
					break;
				}
			}
			result = TRUE;
			return true;
		}

		case TB_ISBUTTONENABLED:
		{
			int cmdId = static_cast<int>(wParam);
			for (const auto& btn : tb.buttons)
			{
				if (btn.idCommand == cmdId)
				{
					result = (btn.fsState & TBSTATE_ENABLED) ? TRUE : FALSE;
					return true;
				}
			}
			result = FALSE;
			return true;
		}

		case TB_ISBUTTONCHECKED:
		{
			int cmdId = static_cast<int>(wParam);
			for (const auto& btn : tb.buttons)
			{
				if (btn.idCommand == cmdId)
				{
					result = (btn.fsState & TBSTATE_CHECKED) ? TRUE : FALSE;
					return true;
				}
			}
			result = FALSE;
			return true;
		}

		case TB_ISBUTTONHIDDEN:
		{
			int cmdId = static_cast<int>(wParam);
			for (const auto& btn : tb.buttons)
			{
				if (btn.idCommand == cmdId)
				{
					result = (btn.fsState & TBSTATE_HIDDEN) ? TRUE : FALSE;
					return true;
				}
			}
			result = FALSE;
			return true;
		}

		case TB_GETSTATE:
		{
			int cmdId = static_cast<int>(wParam);
			for (const auto& btn : tb.buttons)
			{
				if (btn.idCommand == cmdId)
				{
					result = btn.fsState;
					return true;
				}
			}
			result = -1;
			return true;
		}

		case TB_SETSTATE:
		{
			int cmdId = static_cast<int>(wParam);
			BYTE newState = static_cast<BYTE>(LOWORD(lParam));
			for (auto& btn : tb.buttons)
			{
				if (btn.idCommand == cmdId)
				{
					btn.fsState = newState;
					break;
				}
			}
			result = TRUE;
			return true;
		}

		case TB_SETBUTTONSIZE:
		{
			tb.buttonSizeCx = LOWORD(lParam);
			tb.buttonSizeCy = HIWORD(lParam);
			result = TRUE;
			return true;
		}

		case TB_GETBUTTONSIZE:
			result = MAKELONG(tb.buttonSizeCx, tb.buttonSizeCy);
			return true;

		case TB_SETBITMAPSIZE:
		case TB_SETIMAGELIST:
		case TB_GETIMAGELIST:
		case TB_SETHOTIMAGELIST:
		case TB_SETDISABLEDIMAGELIST:
		case TB_AUTOSIZE:
		case TB_SETMAXTEXTROWS:
		case TB_GETTOOLTIPS:
		case TB_SETTOOLTIPS:
		case TB_SETPARENT:
		case TB_SETEXTENDEDSTYLE:
		case TB_GETEXTENDEDSTYLE:
		case TB_ADDBITMAP:
		case TB_SETDRAWTEXTFLAGS:
		case TB_SETPADDING:
		case TB_GETPADDING:
		case TB_SETCMDID:
		case TB_CUSTOMIZE:
		case TB_SAVERESTORE:
		case TB_PRESSBUTTON:
		case TB_INDETERMINATE:
		case TB_ISBUTTONPRESSED:
		case TB_SETROWS:
		case TB_GETROWS:
		case TB_GETBITMAPFLAGS:
			result = 0;
			return true;

		case TB_GETITEMRECT:
		case TB_GETRECT:
		{
			RECT* pRect = reinterpret_cast<RECT*>(lParam);
			if (pRect)
			{
				int index = -1;
				if (msg == TB_GETITEMRECT)
				{
					// TB_GETITEMRECT: wParam is a 0-based button index
					index = static_cast<int>(wParam);
				}
				else // TB_GETRECT
				{
					// TB_GETRECT: wParam is a command ID; map it to a button index
					LRESULT idx = SendMessageW(hwnd, TB_COMMANDTOINDEX, wParam, 0);
					index = static_cast<int>(idx);
				}

				if (index >= 0)
				{
					pRect->left = index * tb.buttonSizeCx;
					pRect->top = 0;
					pRect->right = pRect->left + tb.buttonSizeCx;
					pRect->bottom = tb.buttonSizeCy;
					result = TRUE;
				}
				else
				{
					memset(pRect, 0, sizeof(RECT));
					result = FALSE;
				}
			}
			else
			{
				result = FALSE;
			}
			return true;
		}

		case TB_GETBUTTONINFOW:
		case TB_SETBUTTONINFOW:
			result = -1;
			return true;

		case TB_GETITEMDROPDOWNRECT:
		{
			RECT* pRect = reinterpret_cast<RECT*>(lParam);
			if (pRect)
				memset(pRect, 0, sizeof(RECT));
			result = FALSE;
			return true;
		}
	}

	return false;
}

// ============================================================
// ReBar message handling (data-only)
// ============================================================

static bool HandleReBarMessage(HWND hwnd, unsigned int msg, uintptr_t wParam, intptr_t lParam,
                                intptr_t& result)
{
	uintptr_t key = reinterpret_cast<uintptr_t>(hwnd);
	auto it = s_reBars.find(key);
	if (it == s_reBars.end()) return false;

	auto& rb = it->second;

	switch (msg)
	{
		case RB_INSERTBANDW:
		{
			const REBARBANDINFOW* pBand = reinterpret_cast<const REBARBANDINFOW*>(lParam);
			if (pBand)
			{
				ReBarBandData band;
				band.fMask = pBand->fMask;
				band.fStyle = pBand->fStyle;
				if (pBand->fMask & RBBIM_CHILD) band.hwndChild = pBand->hwndChild;
				if (pBand->fMask & RBBIM_CHILDSIZE) { band.cxMinChild = pBand->cxMinChild; band.cyMinChild = pBand->cyMinChild; }
				if (pBand->fMask & RBBIM_SIZE) band.cx = pBand->cx;
				if (pBand->fMask & RBBIM_ID) band.wID = pBand->wID;
				if (pBand->fMask & RBBIM_TEXT && pBand->lpText) band.text = pBand->lpText;

				int index = static_cast<int>(wParam);
				if (index < 0 || index > static_cast<int>(rb.bands.size()))
					index = static_cast<int>(rb.bands.size());
				rb.bands.insert(rb.bands.begin() + index, band);
			}
			result = TRUE;
			return true;
		}

		case RB_DELETEBAND:
		{
			int index = static_cast<int>(wParam);
			if (index >= 0 && index < static_cast<int>(rb.bands.size()))
			{
				rb.bands.erase(rb.bands.begin() + index);
				result = TRUE;
			}
			else
			{
				result = FALSE;
			}
			return true;
		}

		case RB_GETBANDCOUNT:
			result = static_cast<intptr_t>(rb.bands.size());
			return true;

		case RB_SETBANDINFOW:
		{
			int index = static_cast<int>(wParam);
			const REBARBANDINFOW* pBand = reinterpret_cast<const REBARBANDINFOW*>(lParam);
			if (pBand && index >= 0 && index < static_cast<int>(rb.bands.size()))
			{
				auto& band = rb.bands[index];
				if (pBand->fMask & RBBIM_STYLE) band.fStyle = pBand->fStyle;
				if (pBand->fMask & RBBIM_CHILD) band.hwndChild = pBand->hwndChild;
				if (pBand->fMask & RBBIM_CHILDSIZE) { band.cxMinChild = pBand->cxMinChild; band.cyMinChild = pBand->cyMinChild; }
				if (pBand->fMask & RBBIM_SIZE) band.cx = pBand->cx;
				if (pBand->fMask & RBBIM_ID) band.wID = pBand->wID;
				if (pBand->fMask & RBBIM_TEXT && pBand->lpText) band.text = pBand->lpText;
				result = TRUE;
			}
			else
			{
				result = FALSE;
			}
			return true;
		}

		case RB_GETBANDINFOW:
		{
			int index = static_cast<int>(wParam);
			REBARBANDINFOW* pBand = reinterpret_cast<REBARBANDINFOW*>(lParam);
			if (pBand && index >= 0 && index < static_cast<int>(rb.bands.size()))
			{
				const auto& band = rb.bands[index];
				if (pBand->fMask & RBBIM_STYLE) pBand->fStyle = band.fStyle;
				if (pBand->fMask & RBBIM_CHILD) pBand->hwndChild = band.hwndChild;
				if (pBand->fMask & RBBIM_CHILDSIZE) { pBand->cxMinChild = band.cxMinChild; pBand->cyMinChild = band.cyMinChild; }
				if (pBand->fMask & RBBIM_SIZE) pBand->cx = band.cx;
				if (pBand->fMask & RBBIM_ID) pBand->wID = band.wID;
				result = TRUE;
			}
			else
			{
				result = FALSE;
			}
			return true;
		}

		case RB_IDTOINDEX:
		{
			UINT id = static_cast<UINT>(wParam);
			for (int i = 0; i < static_cast<int>(rb.bands.size()); ++i)
			{
				if (rb.bands[i].wID == id)
				{
					result = i;
					return true;
				}
			}
			result = -1;
			return true;
		}

		case RB_SHOWBAND:
		{
			int index = static_cast<int>(wParam);
			if (index >= 0 && index < static_cast<int>(rb.bands.size()))
			{
				if (lParam)
					rb.bands[index].fStyle &= ~RBBS_HIDDEN;
				else
					rb.bands[index].fStyle |= RBBS_HIDDEN;
			}
			result = TRUE;
			return true;
		}

		case RB_SETBARINFO:
		case RB_GETBARINFO:
		case RB_GETROWCOUNT:
		case RB_GETROWHEIGHT:
		case RB_SIZETORECT:
		case RB_SETBKCOLOR:
		case RB_GETBKCOLOR:
		case RB_SETTEXTCOLOR:
		case RB_GETTEXTCOLOR:
		case RB_MOVEBAND:
		case RB_GETBARHEIGHT:
			result = 0;
			return true;
	}

	return false;
}

// ============================================================
// Button message handling
// ============================================================

static bool HandleButtonMessage(HWND hwnd, unsigned int msg, uintptr_t wParam, intptr_t lParam,
                                  intptr_t& result)
{
	auto* info = HandleRegistry::getWindowInfo(hwnd);
	if (!info || !info->nativeView) return false;

	NSView* view = (__bridge NSView*)info->nativeView;

	// Only handle if it's actually an NSButton
	if (![view isKindOfClass:[NSButton class]] && ![view isKindOfClass:[NSBox class]])
		return false;

	// NSBox is used for group boxes - only handle WM_SETTEXT
	if ([view isKindOfClass:[NSBox class]])
	{
		if (msg == WM_SETTEXT)
		{
			NSBox* box = (NSBox*)view;
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			box.title = text ? WideToNS(text) : @"";
			result = TRUE;
			return true;
		}
		return false;
	}

	NSButton* btn = (NSButton*)view;

	switch (msg)
	{
		case BM_GETCHECK:
			result = (btn.state == NSControlStateValueOn) ? BST_CHECKED :
			         (btn.state == NSControlStateValueMixed) ? BST_INDETERMINATE :
			         BST_UNCHECKED;
			return true;

		case BM_SETCHECK:
			if (wParam == BST_CHECKED)
				btn.state = NSControlStateValueOn;
			else if (wParam == BST_INDETERMINATE)
				btn.state = NSControlStateValueMixed;
			else
				btn.state = NSControlStateValueOff;
			result = 0;
			return true;

		case BM_CLICK:
			[btn performClick:nil];
			result = 0;
			return true;

		case BM_GETSTATE:
			result = (btn.state == NSControlStateValueOn) ? BST_CHECKED : 0;
			if (btn.isHighlighted)
				result |= BST_PUSHED;
			return true;

		case BM_SETSTATE:
			btn.highlighted = (wParam != 0);
			result = 0;
			return true;

		case WM_SETTEXT:
		{
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			btn.title = text ? WideToNS(text) : @"";
			info->windowName = text ? text : L"";
			result = TRUE;
			return true;
		}

		case WM_GETTEXT:
		{
			wchar_t* buf = reinterpret_cast<wchar_t*>(lParam);
			int maxLen = static_cast<int>(wParam);
			if (buf && maxLen > 0)
			{
				size_t copyLen = (std::min)(static_cast<size_t>(maxLen - 1), info->windowName.size());
				wcsncpy(buf, info->windowName.c_str(), copyLen);
				buf[copyLen] = L'\0';
				result = static_cast<intptr_t>(copyLen);
			}
			else
			{
				result = 0;
			}
			return true;
		}

		case WM_GETTEXTLENGTH:
			result = static_cast<intptr_t>(info->windowName.size());
			return true;

		case WM_ENABLE:
			btn.enabled = (wParam != 0);
			result = 0;
			return true;
	}

	return false;
}

// ============================================================
// Edit control message handling
// ============================================================

static bool HandleEditMessage(HWND hwnd, unsigned int msg, uintptr_t wParam, intptr_t lParam,
                                intptr_t& result)
{
	auto* info = HandleRegistry::getWindowInfo(hwnd);
	if (!info || !info->nativeView) return false;

	NSView* view = (__bridge NSView*)info->nativeView;

	// Single-line: NSTextField; multi-line: NSScrollView wrapping NSTextView
	NSTextField* textField = nil;
	if ([view isKindOfClass:[NSTextField class]])
		textField = (NSTextField*)view;

	switch (msg)
	{
		case WM_SETTEXT:
		{
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			info->windowName = text ? text : L"";
			if (textField)
				textField.stringValue = text ? WideToNS(text) : @"";
			else if ([view isKindOfClass:[NSScrollView class]])
			{
				NSTextView* tv = ((NSScrollView*)view).documentView;
				if ([tv isKindOfClass:[NSTextView class]])
					[tv setString:text ? WideToNS(text) : @""];
			}
			result = TRUE;
			return true;
		}

		case WM_GETTEXT:
		{
			wchar_t* buf = reinterpret_cast<wchar_t*>(lParam);
			int maxLen = static_cast<int>(wParam);
			if (buf && maxLen > 0)
			{
				NSString* text = nil;
				if (textField)
					text = textField.stringValue;
				else if ([view isKindOfClass:[NSScrollView class]])
				{
					NSTextView* tv = ((NSScrollView*)view).documentView;
					if ([tv isKindOfClass:[NSTextView class]])
						text = tv.string;
				}

				if (text)
				{
					NSData* data = [text dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
					size_t wcharCount = data ? data.length / sizeof(wchar_t) : 0;
					size_t copyLen = (std::min)(static_cast<size_t>(maxLen - 1), wcharCount);
					if (copyLen > 0 && data)
						memcpy(buf, data.bytes, copyLen * sizeof(wchar_t));
					buf[copyLen] = L'\0';
					result = static_cast<intptr_t>(copyLen);
				}
				else
				{
					buf[0] = L'\0';
					result = 0;
				}
			}
			else
			{
				result = 0;
			}
			return true;
		}

		case WM_GETTEXTLENGTH:
		{
			NSString* text = nil;
			if (textField)
				text = textField.stringValue;
			else if ([view isKindOfClass:[NSScrollView class]])
			{
				NSTextView* tv = ((NSScrollView*)view).documentView;
				if ([tv isKindOfClass:[NSTextView class]])
					text = tv.string;
			}
			result = text ? static_cast<intptr_t>(text.length) : 0;
			return true;
		}

		case EM_SETSEL:
		{
			if (textField)
			{
				NSText* editor = [textField.window fieldEditor:YES forObject:textField];
				if (editor)
				{
					int start = static_cast<int>(wParam);
					int end = static_cast<int>(lParam);
					if (start == 0 && end == -1)
						[editor selectAll:nil];
					else
						editor.selectedRange = NSMakeRange(MIN(start, end), abs(end - start));
				}
			}
			result = 0;
			return true;
		}

		case EM_GETSEL:
		{
			DWORD* pStart = reinterpret_cast<DWORD*>(wParam);
			DWORD* pEnd = reinterpret_cast<DWORD*>(lParam);
			if (textField)
			{
				NSText* editor = [textField.window fieldEditor:YES forObject:textField];
				if (editor)
				{
					NSRange sel = editor.selectedRange;
					if (pStart) *pStart = static_cast<DWORD>(sel.location);
					if (pEnd) *pEnd = static_cast<DWORD>(sel.location + sel.length);
					result = MAKELONG(sel.location, sel.location + sel.length);
				}
			}
			else
			{
				if (pStart) *pStart = 0;
				if (pEnd) *pEnd = 0;
				result = 0;
			}
			return true;
		}

		case EM_REPLACESEL:
		{
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			NSString* nsText = text ? WideToNS(text) : @"";
			if (textField)
			{
				NSText* editor = [textField.window fieldEditor:YES forObject:textField];
				if (editor)
					[editor replaceCharactersInRange:editor.selectedRange withString:nsText];
			}
			result = 0;
			return true;
		}

		case EM_SETREADONLY:
		{
			if (textField)
				textField.editable = (wParam == 0);
			result = TRUE;
			return true;
		}

		case EM_GETLINECOUNT:
		{
			NSString* text = nil;
			if (textField)
				text = textField.stringValue;
			if (text)
			{
				NSUInteger lines = 1;
				for (NSUInteger i = 0; i < text.length; ++i)
					if ([text characterAtIndex:i] == '\n') ++lines;
				result = static_cast<intptr_t>(lines);
			}
			else
			{
				result = 1;
			}
			return true;
		}

		case EM_SETLIMITTEXT:
		{
			// Limit text length (not enforced on macOS, just store)
			result = 0;
			return true;
		}

		case EM_GETLIMITTEXT:
			result = 0x7FFFFFFE; // unlimited
			return true;

		case EM_SETMODIFY:
		case EM_GETMODIFY:
		case EM_CANUNDO:
		case EM_UNDO:
		case EM_EMPTYUNDOBUFFER:
		case EM_SETMARGINS:
		case EM_GETMARGINS:
		case EM_SCROLL:
		case EM_SCROLLCARET:
		case EM_SETPASSWORDCHAR:
		case EM_GETFIRSTVISIBLELINE:
		case EM_LINELENGTH:
		case EM_LINEINDEX:
		case EM_GETLINE:
			result = 0;
			return true;

		case WM_ENABLE:
			if (textField)
				textField.enabled = (wParam != 0);
			result = 0;
			return true;
	}

	return false;
}

// ============================================================
// Static control message handling
// ============================================================

static bool HandleStaticMessage(HWND hwnd, unsigned int msg, uintptr_t wParam, intptr_t lParam,
                                  intptr_t& result)
{
	auto* info = HandleRegistry::getWindowInfo(hwnd);
	if (!info || !info->nativeView) return false;

	NSView* view = (__bridge NSView*)info->nativeView;
	if (![view isKindOfClass:[NSTextField class]])
		return false;

	NSTextField* label = (NSTextField*)view;

	switch (msg)
	{
		case WM_SETTEXT:
		{
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			info->windowName = text ? text : L"";
			label.stringValue = text ? WideToNS(text) : @"";
			result = TRUE;
			return true;
		}

		case WM_GETTEXT:
		{
			wchar_t* buf = reinterpret_cast<wchar_t*>(lParam);
			int maxLen = static_cast<int>(wParam);
			if (buf && maxLen > 0)
			{
				size_t copyLen = (std::min)(static_cast<size_t>(maxLen - 1), info->windowName.size());
				wcsncpy(buf, info->windowName.c_str(), copyLen);
				buf[copyLen] = L'\0';
				result = static_cast<intptr_t>(copyLen);
			}
			else
			{
				result = 0;
			}
			return true;
		}

		case WM_GETTEXTLENGTH:
			result = static_cast<intptr_t>(info->windowName.size());
			return true;

		case STM_SETIMAGE:
		case STM_GETIMAGE:
		case STM_SETICON:
		case STM_GETICON:
			result = 0;
			return true;
	}

	return false;
}

// ============================================================
// ComboBox message handling
// ============================================================

static bool HandleComboBoxMessage(HWND hwnd, unsigned int msg, uintptr_t wParam, intptr_t lParam,
                                    intptr_t& result)
{
	auto* info = HandleRegistry::getWindowInfo(hwnd);
	if (!info || !info->nativeView) return false;

	NSView* view = (__bridge NSView*)info->nativeView;

	// NSComboBox (editable) or NSPopUpButton (dropdown list)
	NSComboBox* combo = nil;
	NSPopUpButton* popup = nil;
	if ([view isKindOfClass:[NSComboBox class]])
		combo = (NSComboBox*)view;
	else if ([view isKindOfClass:[NSPopUpButton class]])
		popup = (NSPopUpButton*)view;
	else
		return false;

	switch (msg)
	{
		case CB_ADDSTRING:
		{
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			NSString* nsText = text ? WideToNS(text) : @"";
			if (combo)
			{
				[combo addItemWithObjectValue:nsText];
				result = combo.numberOfItems - 1;
			}
			else if (popup)
			{
				[popup addItemWithTitle:nsText];
				result = popup.numberOfItems - 1;
			}
			return true;
		}

		case CB_INSERTSTRING:
		{
			int index = static_cast<int>(wParam);
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			NSString* nsText = text ? WideToNS(text) : @"";
			if (combo)
			{
				[combo insertItemWithObjectValue:nsText atIndex:index];
				result = index;
			}
			else if (popup)
			{
				[popup insertItemWithTitle:nsText atIndex:index];
				result = index;
			}
			return true;
		}

		case CB_DELETESTRING:
		{
			int index = static_cast<int>(wParam);
			if (combo && index >= 0 && index < combo.numberOfItems)
			{
				[combo removeItemAtIndex:index];
				result = combo.numberOfItems;
			}
			else if (popup && index >= 0 && index < popup.numberOfItems)
			{
				[popup removeItemAtIndex:index];
				result = popup.numberOfItems;
			}
			else
			{
				result = CB_ERR;
			}
			return true;
		}

		case CB_RESETCONTENT:
			if (combo) [combo removeAllItems];
			else if (popup) [popup removeAllItems];
			result = 0;
			return true;

		case CB_GETCOUNT:
			result = combo ? combo.numberOfItems : (popup ? popup.numberOfItems : 0);
			return true;

		case CB_GETCURSEL:
			result = combo ? combo.indexOfSelectedItem : (popup ? popup.indexOfSelectedItem : -1);
			return true;

		case CB_SETCURSEL:
		{
			int index = static_cast<int>(wParam);
			if (combo)
			{
				if (index >= 0 && index < combo.numberOfItems)
					[combo selectItemAtIndex:index];
				else
					{
					NSInteger currentIndex = combo.indexOfSelectedItem;
					if (currentIndex >= 0)
						[combo deselectItemAtIndex:currentIndex];
				}
			}
			else if (popup)
			{
				if (index >= 0 && index < popup.numberOfItems)
					[popup selectItemAtIndex:index];
			}
			result = index;
			return true;
		}

		case CB_GETLBTEXTLEN:
		{
			int index = static_cast<int>(wParam);
			NSString* text = nil;
			if (combo && index >= 0 && index < combo.numberOfItems)
				text = [combo itemObjectValueAtIndex:index];
			else if (popup && index >= 0 && index < popup.numberOfItems)
				text = [popup itemTitleAtIndex:index];
			result = text ? static_cast<intptr_t>(text.length) : CB_ERR;
			return true;
		}

		case CB_GETLBTEXT:
		{
			int index = static_cast<int>(wParam);
			wchar_t* buf = reinterpret_cast<wchar_t*>(lParam);
			NSString* text = nil;
			if (combo && index >= 0 && index < combo.numberOfItems)
				text = [combo itemObjectValueAtIndex:index];
			else if (popup && index >= 0 && index < popup.numberOfItems)
				text = [popup itemTitleAtIndex:index];
			if (text && buf)
			{
				NSData* data = [text dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
				size_t wcharCount = data ? data.length / sizeof(wchar_t) : 0;
				if (wcharCount > 0 && data)
					memcpy(buf, data.bytes, wcharCount * sizeof(wchar_t));
				buf[wcharCount] = L'\0';
				result = static_cast<intptr_t>(wcharCount);
			}
			else
			{
				result = CB_ERR;
			}
			return true;
		}

		case CB_FINDSTRINGEXACT:
		{
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			NSString* nsText = text ? WideToNS(text) : @"";
			if (combo)
				result = [combo indexOfItemWithObjectValue:nsText];
			else if (popup)
				result = [popup indexOfItemWithTitle:nsText];
			if (result == NSNotFound)
				result = CB_ERR;
			return true;
		}

		case CB_FINDSTRING:
		{
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			NSString* nsText = text ? WideToNS(text) : @"";
			NSInteger count = combo ? combo.numberOfItems : (popup ? popup.numberOfItems : 0);
			result = CB_ERR;
			for (NSInteger i = 0; i < count; ++i)
			{
				NSString* item = combo ? [combo itemObjectValueAtIndex:i] : [popup itemTitleAtIndex:i];
				if ([item hasPrefix:nsText])
				{
					result = i;
					break;
				}
			}
			return true;
		}

		case CB_SETITEMDATA:
		case CB_GETITEMDATA:
		case CB_SETITEMHEIGHT:
		case CB_GETITEMHEIGHT:
		case CB_SETEXTENDEDUI:
		case CB_GETEXTENDEDUI:
		case CB_SHOWDROPDOWN:
		case CB_GETDROPPEDSTATE:
			result = 0;
			return true;

		case WM_SETTEXT:
		{
			const wchar_t* text = reinterpret_cast<const wchar_t*>(lParam);
			info->windowName = text ? text : L"";
			if (combo) combo.stringValue = text ? WideToNS(text) : @"";
			result = TRUE;
			return true;
		}

		case WM_GETTEXT:
		{
			wchar_t* buf = reinterpret_cast<wchar_t*>(lParam);
			int maxLen = static_cast<int>(wParam);
			if (buf && maxLen > 0)
			{
				NSString* text = combo ? combo.stringValue : [popup titleOfSelectedItem];
				NSData* data = [text dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
				size_t wcharCount = data ? data.length / sizeof(wchar_t) : 0;
				size_t copyLen = (std::min)(static_cast<size_t>(maxLen - 1), wcharCount);
				if (copyLen > 0 && data)
					memcpy(buf, data.bytes, copyLen * sizeof(wchar_t));
				buf[copyLen] = L'\0';
				result = static_cast<intptr_t>(copyLen);
			}
			else
			{
				result = 0;
			}
			return true;
		}

		case WM_GETTEXTLENGTH:
		{
			NSString* text = combo ? combo.stringValue : [popup titleOfSelectedItem];
			result = text ? static_cast<intptr_t>(text.length) : 0;
			return true;
		}
	}

	return false;
}

// ============================================================
// Dispatcher
// ============================================================

bool Win32Controls_HandleMessage(void* hwndVoid, ControlType type,
                                  unsigned int msg, uintptr_t wParam, intptr_t lParam,
                                  intptr_t& result)
{
	HWND hwnd = reinterpret_cast<HWND>(hwndVoid);

	switch (type)
	{
		case ControlType::TabControl:
			return Win32TabControl_HandleMessage(hwnd, msg, wParam, lParam, result);
		case ControlType::StatusBar:
			return HandleStatusBarMessage(hwnd, msg, wParam, lParam, result);
		case ControlType::Toolbar:
			return HandleToolbarMessage(hwnd, msg, wParam, lParam, result);
		case ControlType::ReBar:
			return HandleReBarMessage(hwnd, msg, wParam, lParam, result);
		case ControlType::Button:
			return HandleButtonMessage(hwnd, msg, wParam, lParam, result);
		case ControlType::Edit:
			return HandleEditMessage(hwnd, msg, wParam, lParam, result);
		case ControlType::Static:
			return HandleStaticMessage(hwnd, msg, wParam, lParam, result);
		case ControlType::ComboBox:
			return HandleComboBoxMessage(hwnd, msg, wParam, lParam, result);
		default:
			return false;
	}
}

// ============================================================
// Control lifecycle management
// ============================================================

void Win32Controls_DestroyControl(void* hwndVoid, ControlType type)
{
	uintptr_t key = reinterpret_cast<uintptr_t>(hwndVoid);

	switch (type)
	{
		case ControlType::TabControl:
			Win32TabControl_Destroy(hwndVoid);
			break;
		case ControlType::StatusBar:
			s_statusBars.erase(key);
			break;
		case ControlType::Toolbar:
			s_toolBars.erase(key);
			break;
		case ControlType::ReBar:
			s_reBars.erase(key);
			break;
		case ControlType::Button:
		{
			NSNumber* num = @(key);
			[s_buttonTargets removeObjectForKey:num];
			break;
		}
		case ControlType::Edit:
		{
			NSNumber* num = @(key);
			[s_editTargets removeObjectForKey:num];
			break;
		}
		default:
			break;
	}
}

// Called from CreateWindowExW after the HWND is created and registered
// to initialize per-control data structures.
void Win32Controls_InitControl(void* hwndVoid, ControlType type, void* parentVoid)
{
	HWND hwnd = reinterpret_cast<HWND>(hwndVoid);
	HWND parent = reinterpret_cast<HWND>(parentVoid);
	uintptr_t key = reinterpret_cast<uintptr_t>(hwnd);

	switch (type)
	{
		case ControlType::TabControl:
			Win32TabControl_Init(hwndVoid, parentVoid);
			break;
		case ControlType::StatusBar:
		{
			StatusBarData data;
			data.hwnd = hwnd;
			s_statusBars[key] = data;
			break;
		}
		case ControlType::Toolbar:
		{
			ToolbarData data;
			data.hwnd = hwnd;
			s_toolBars[key] = data;
			break;
		}
		case ControlType::ReBar:
		{
			ReBarData data;
			data.hwnd = hwnd;
			s_reBars[key] = data;
			break;
		}
		case ControlType::Button:
		{
			auto* info = HandleRegistry::getWindowInfo(hwnd);
			if (info && info->nativeView)
			{
				id view = (__bridge id)info->nativeView;
				if ([view isKindOfClass:[NSButton class]])
				{
					if (!s_buttonTargets)
						s_buttonTargets = [NSMutableDictionary dictionary];

					Win32ButtonTarget* target = [[Win32ButtonTarget alloc] init];
					target.buttonHwnd = hwnd;

					NSButton* btn = (NSButton*)view;
					btn.target = target;
					btn.action = @selector(buttonClicked:);

					s_buttonTargets[@(key)] = target;
				}
			}
			break;
		}
		case ControlType::Edit:
		{
			auto* info = HandleRegistry::getWindowInfo(hwnd);
			if (info && info->nativeView)
			{
				if (!s_editTargets)
					s_editTargets = [NSMutableDictionary dictionary];

				Win32EditTarget* target = [[Win32EditTarget alloc] init];
				target.editHwnd = hwnd;

				NSTextField* field = (__bridge NSTextField*)info->nativeView;
				field.delegate = target;

				s_editTargets[@(key)] = target;
			}
			break;
		}
		default:
			break;
	}
}

// ============================================================
// ImageList stubs (count-tracking only)
// ============================================================

struct ImageListData
{
	int cx = 0;
	int cy = 0;
	UINT flags = 0;
	int count = 0;
};

static std::unordered_map<uintptr_t, ImageListData> s_imageLists;
static uintptr_t s_nextImageList = 0x30000;

HIMAGELIST ImageList_Create(int cx, int cy, UINT flags, int cInitial, int cGrow)
{
	HIMAGELIST himl = reinterpret_cast<HIMAGELIST>(s_nextImageList++);
	ImageListData data;
	data.cx = cx;
	data.cy = cy;
	data.flags = flags;
	data.count = 0;
	s_imageLists[reinterpret_cast<uintptr_t>(himl)] = data;
	return himl;
}

BOOL ImageList_Destroy(HIMAGELIST himl)
{
	s_imageLists.erase(reinterpret_cast<uintptr_t>(himl));
	return TRUE;
}

int ImageList_Add(HIMAGELIST himl, HBITMAP hbmImage, HBITMAP hbmMask)
{
	auto it = s_imageLists.find(reinterpret_cast<uintptr_t>(himl));
	if (it != s_imageLists.end())
		return it->second.count++;
	return -1;
}

int ImageList_AddMasked(HIMAGELIST himl, HBITMAP hbmImage, COLORREF crMask)
{
	return ImageList_Add(himl, hbmImage, nullptr);
}

int ImageList_ReplaceIcon(HIMAGELIST himl, int i, HICON hicon)
{
	auto it = s_imageLists.find(reinterpret_cast<uintptr_t>(himl));
	if (it == s_imageLists.end()) return -1;

	if (i == -1) // append
		return it->second.count++;
	return i;
}

BOOL ImageList_Remove(HIMAGELIST himl, int i)
{
	auto it = s_imageLists.find(reinterpret_cast<uintptr_t>(himl));
	if (it == s_imageLists.end()) return FALSE;

	if (i == -1) // remove all
		it->second.count = 0;
	else if (it->second.count > 0)
		--it->second.count;

	return TRUE;
}

int ImageList_GetImageCount(HIMAGELIST himl)
{
	auto it = s_imageLists.find(reinterpret_cast<uintptr_t>(himl));
	return (it != s_imageLists.end()) ? it->second.count : 0;
}

BOOL ImageList_SetImageCount(HIMAGELIST himl, UINT uNewCount)
{
	auto it = s_imageLists.find(reinterpret_cast<uintptr_t>(himl));
	if (it != s_imageLists.end())
	{
		it->second.count = static_cast<int>(uNewCount);
		return TRUE;
	}
	return FALSE;
}

BOOL ImageList_Draw(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, UINT fStyle)
{
	return TRUE; // stub
}

BOOL ImageList_DrawEx(HIMAGELIST himl, int i, HDC hdcDst, int x, int y, int dx, int dy,
                       COLORREF rgbBk, COLORREF rgbFg, UINT fStyle)
{
	return TRUE; // stub
}

BOOL ImageList_SetIconSize(HIMAGELIST himl, int cx, int cy)
{
	auto it = s_imageLists.find(reinterpret_cast<uintptr_t>(himl));
	if (it != s_imageLists.end())
	{
		it->second.cx = cx;
		it->second.cy = cy;
		return TRUE;
	}
	return FALSE;
}

HICON ImageList_GetIcon(HIMAGELIST himl, int i, UINT flags)
{
	return nullptr;
}

BOOL ImageList_GetIconSize(HIMAGELIST himl, int* cx, int* cy)
{
	auto it = s_imageLists.find(reinterpret_cast<uintptr_t>(himl));
	if (it != s_imageLists.end())
	{
		if (cx) *cx = it->second.cx;
		if (cy) *cy = it->second.cy;
		return TRUE;
	}
	return FALSE;
}

BOOL ImageList_GetImageInfo(HIMAGELIST himl, int i, IMAGEINFO* pImageInfo)
{
	if (pImageInfo) memset(pImageInfo, 0, sizeof(IMAGEINFO));
	return FALSE;
}

BOOL ImageList_BeginDrag(HIMAGELIST himlTrack, int iTrack, int dxHotspot, int dyHotspot) { return TRUE; }
BOOL ImageList_DragEnter(HWND hwndLock, int x, int y) { return TRUE; }
BOOL ImageList_DragMove(int x, int y) { return TRUE; }
BOOL ImageList_DragShowNolock(BOOL fShow) { return TRUE; }
BOOL ImageList_DragLeave(HWND hwndLock) { return TRUE; }
void ImageList_EndDrag() {}
HIMAGELIST ImageList_Merge(HIMAGELIST himl1, int i1, HIMAGELIST himl2, int i2, int dx, int dy) { return nullptr; }
