// Win32 Status Bar shim: Win32StatusBarView, StatusBarData, HandleStatusBarMessage
// Extracted from win32_controls.mm as part of shim modularization.

#import <Cocoa/Cocoa.h>
#include "windows.h"
#include "commctrl.h"
#include "handle_registry.h"
#include "win32_string_helpers.h"
#include "win32_status_bar_impl.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

// ============================================================
// Custom NSView that draws a simple status bar with partitions
// ============================================================

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

// ============================================================
// StatusBar data
// ============================================================

struct StatusBarData
{
	HWND hwnd = nullptr;
	std::vector<int> partWidths;
	std::vector<std::wstring> partTexts;
};

static std::unordered_map<uintptr_t, StatusBarData> s_statusBars;

// ============================================================
// Status bar init/destroy
// ============================================================

void Win32StatusBar_Init(void* hwndVoid)
{
	HWND hwnd = reinterpret_cast<HWND>(hwndVoid);
	uintptr_t key = reinterpret_cast<uintptr_t>(hwnd);

	StatusBarData data;
	data.hwnd = hwnd;
	s_statusBars[key] = data;
}

void Win32StatusBar_Destroy(void* hwndVoid)
{
	uintptr_t key = reinterpret_cast<uintptr_t>(hwndVoid);
	s_statusBars.erase(key);
}

// ============================================================
// StatusBar message handling
// ============================================================

bool Win32StatusBar_HandleMessage(void* hwndVoid, unsigned int msg,
                                   uintptr_t wParam, intptr_t lParam,
                                   intptr_t& result)
{
	HWND hwnd = reinterpret_cast<HWND>(hwndVoid);
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
