// Win32 Controls Shim: dispatcher and control creation for macOS.
// Individual control implementations are in dedicated modules:
//   win32_tab_control.mm, win32_status_bar.mm, win32_toolbar_rebar.mm,
//   win32_dialog_controls.mm

#import <Cocoa/Cocoa.h>
#include "windows.h"
#include "commctrl.h"
#include "handle_registry.h"
#include "win32_string_helpers.h"
#include "win32_controls_impl.h"
#include "win32_tab_control_impl.h"
#include "win32_status_bar_impl.h"
#include "win32_toolbar_rebar_impl.h"
#include "win32_dialog_controls_impl.h"

#include <string>

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
			// Delegate to status bar module for Win32StatusBarView creation
			return Win32StatusBar_CreateView(parentView, width);
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
			return Win32StatusBar_HandleMessage(hwnd, msg, wParam, lParam, result);
		case ControlType::Toolbar:
			return Win32Toolbar_HandleMessage(hwnd, msg, wParam, lParam, result);
		case ControlType::ReBar:
			return Win32ReBar_HandleMessage(hwnd, msg, wParam, lParam, result);
		case ControlType::Button:
			return Win32Button_HandleMessage(hwnd, msg, wParam, lParam, result);
		case ControlType::Edit:
			return Win32Edit_HandleMessage(hwnd, msg, wParam, lParam, result);
		case ControlType::Static:
			return Win32Static_HandleMessage(hwnd, msg, wParam, lParam, result);
		case ControlType::ComboBox:
			return Win32ComboBox_HandleMessage(hwnd, msg, wParam, lParam, result);
		default:
			return false;
	}
}

// ============================================================
// Control lifecycle management
// ============================================================

void Win32Controls_DestroyControl(void* hwndVoid, ControlType type)
{
	switch (type)
	{
		case ControlType::TabControl:
			Win32TabControl_Destroy(hwndVoid);
			break;
		case ControlType::StatusBar:
			Win32StatusBar_Destroy(hwndVoid);
			break;
		case ControlType::Toolbar:
			Win32Toolbar_Destroy(hwndVoid);
			break;
		case ControlType::ReBar:
			Win32ReBar_Destroy(hwndVoid);
			break;
		case ControlType::Button:
			Win32Button_Destroy(hwndVoid);
			break;
		case ControlType::Edit:
			Win32Edit_Destroy(hwndVoid);
			break;
		default:
			break;
	}
}

// Called from CreateWindowExW after the HWND is created and registered
// to initialize per-control data structures.
void Win32Controls_InitControl(void* hwndVoid, ControlType type, void* parentVoid)
{
	switch (type)
	{
		case ControlType::TabControl:
			Win32TabControl_Init(hwndVoid, parentVoid);
			break;
		case ControlType::StatusBar:
			Win32StatusBar_Init(hwndVoid);
			break;
		case ControlType::Toolbar:
			Win32Toolbar_Init(hwndVoid);
			break;
		case ControlType::ReBar:
			Win32ReBar_Init(hwndVoid);
			break;
		case ControlType::Button:
			Win32Button_Init(hwndVoid);
			break;
		case ControlType::Edit:
			Win32Edit_Init(hwndVoid);
			break;
		default:
			break;
	}
}
