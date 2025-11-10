// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include <stdexcept>
#include "ColourPopup.h"
#include "ColourPopupResource.h"
#include "NppDarkMode.h"
#include "dpiManagerV2.h"

#include <commdlg.h>

#include <array>

static constexpr std::array<COLORREF, 48> colorItems{ {
	RGB(  0,   0,   0), RGB( 64,   0,   0), RGB(128,   0,   0), RGB(128,  64,  64), RGB(255,   0,   0), RGB(255, 128, 128),
	RGB(255, 255, 128), RGB(255, 255,   0), RGB(255, 128,  64), RGB(255, 128,   0), RGB(128,  64,   0), RGB(128, 128,   0),
	RGB(128, 128,  64), RGB(  0,  64,   0), RGB(  0, 128,   0), RGB(  0, 255,   0), RGB(128, 255,   0), RGB(128, 255, 128),
	RGB(  0, 255, 128), RGB(  0, 255,  64), RGB(  0, 128, 128), RGB(  0, 128,  64), RGB(  0,  64,  64), RGB(128, 128, 128),
	RGB( 64, 128, 128), RGB(  0,   0, 128), RGB(  0,   0, 255), RGB(  0,  64, 128), RGB(  0, 255, 255), RGB(128, 255, 255),
	RGB(  0, 128, 255), RGB(  0, 128, 192), RGB(128, 128, 255), RGB(  0,   0, 160), RGB(  0,   0,  64), RGB(192, 192, 192),
	RGB( 64,   0,  64), RGB( 64,   0,  64), RGB(128,   0, 128), RGB(128,   0,  64), RGB(128, 128, 192), RGB(255, 128, 192),
	RGB(255, 128, 255), RGB(255,   0, 255), RGB(255,   0, 128), RGB(128,   0, 255), RGB( 64,   0, 128), RGB(255, 255, 255),
} };

void ColourPopup::createColorPopup()
{
	_hSelf = StaticDialog::myCreateDialogIndirectParam(IDD_COLOUR_POPUP, false, 8, ColourPopup::dlgClrPopupProc);

	if (!_hSelf)
	{
		throw std::runtime_error("ColourPopup::createColorPopup : StaticDialog::myCreateDialogIndirectParam() function returns nullptr");
	}
}

void ColourPopup::doDialog(POINT p)
{
	if (!isCreated())
	{
		const auto dpiContext = DPIManagerV2::setThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		createColorPopup();
		DPIManagerV2::setThreadDpiAwarenessContext(dpiContext);

		::SetWindowPos(_hSelf, HWND_TOP, p.x, p.y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
	}
}

intptr_t CALLBACK ColourPopup::dlgClrPopupProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_MEASUREITEM: // sent before WM_INITDIALOG
		{
			RECT rc;
			LPMEASUREITEMSTRUCT lpmis = reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);
			::GetWindowRect(::GetDlgItem(hwnd, lpmis->CtlID), &rc);
			lpmis->itemHeight = (rc.bottom-rc.top)/6; 
			lpmis->itemWidth = (rc.right-rc.left)/8;
			return TRUE;
		}

		case WM_INITDIALOG :
		{
			ColourPopup *pColourPopup = reinterpret_cast<ColourPopup *>(lParam);
			pColourPopup->_hSelf = hwnd;
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, static_cast<LONG_PTR>(lParam));
			pColourPopup->run_dlgProc(message, wParam, lParam);
			return TRUE;
		}

		default :
		{
			ColourPopup *pColourPopup = reinterpret_cast<ColourPopup *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
			if (!pColourPopup)
				return FALSE;
			return pColourPopup->run_dlgProc(message, wParam, lParam);
		}
	}
}

intptr_t CALLBACK ColourPopup::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
		case WM_INITDIALOG:
		{
			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			for (const auto& color : colorItems)
			{
				const auto i = ::SendDlgItemMessage(_hSelf, IDC_COLOUR_LIST, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(""));
				if (i != LB_ERR)
					::SendDlgItemMessage(_hSelf, IDC_COLOUR_LIST, LB_SETITEMDATA, i, static_cast<LPARAM>(color));
			}
			return TRUE;
		}
		
		case WM_CTLCOLORLISTBOX:
		{
			if (NppDarkMode::isEnabled())
			{
				return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
			}
			return reinterpret_cast<LRESULT>(::GetStockObject(NULL_BRUSH));
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_DRAWITEM:
		{
			HDC hdc;
			COLORREF	cr;
			HBRUSH		hbrush;
	
			DRAWITEMSTRUCT *pdis = (DRAWITEMSTRUCT *)lParam;
			hdc = pdis->hDC;
			RECT rc = pdis->rcItem;
	
			// Transparent.
			SetBkMode(hdc,TRANSPARENT);
	
			// NULL object
			if (pdis->itemID == UINT(-1)) return 0; 

			switch (pdis->itemAction)
			{
				case ODA_DRAWENTIRE:
					switch (pdis->CtlID)
					{
						case IDC_COLOUR_LIST:
							rc = pdis->rcItem;
							cr = (COLORREF) pdis->itemData;
							InflateRect(&rc, -3, -3);
							hbrush = CreateSolidBrush((COLORREF)cr);
							FillRect(hdc, &rc, hbrush);
							DeleteObject(hbrush);
							hbrush = CreateSolidBrush(NppDarkMode::isEnabled() ? NppDarkMode::getEdgeColor() : RGB(0, 0, 0));
							FrameRect(hdc, &rc, hbrush);
							DeleteObject(hbrush);
							break;
					}
					// *** FALL THROUGH ***
					[[fallthrough]];
				case ODA_SELECT:
					rc = pdis->rcItem;
					if (pdis->itemState & ODS_SELECTED)
					{
						rc.bottom --;
						rc.right --;
						// Draw the lighted side.
						HPEN hpen = CreatePen(PS_SOLID, 1, NppDarkMode::isEnabled() ? NppDarkMode::getEdgeColor() : GetSysColor(COLOR_BTNSHADOW));
						HPEN holdPen = (HPEN)SelectObject(hdc, hpen);
						MoveToEx(hdc, rc.left, rc.bottom, NULL);
						LineTo(hdc, rc.left, rc.top);
						LineTo(hdc, rc.right, rc.top);
						SelectObject(hdc, holdPen);
						DeleteObject(hpen);
						// Draw the darkened side.
						hpen = CreatePen(PS_SOLID, 1, NppDarkMode::isEnabled() ? NppDarkMode::getEdgeColor() : GetSysColor(COLOR_BTNHIGHLIGHT));
						holdPen = (HPEN)SelectObject(hdc, hpen);
						LineTo(hdc, rc.right, rc.bottom);
						LineTo(hdc, rc.left, rc.bottom);
						SelectObject(hdc, holdPen);
						DeleteObject(hpen);
					}
					else 
					{
						hbrush = CreateSolidBrush(NppDarkMode::isEnabled() ? NppDarkMode::getDlgBackgroundColor() : GetSysColor(COLOR_3DFACE));
						FrameRect(hdc, &rc, hbrush);
						DeleteObject(hbrush);
					}
					break;
				case ODA_FOCUS:
					rc = pdis->rcItem;
					InflateRect(&rc, -2, -2);
					DrawFocusRect(hdc, &rc);
					break;
				default:
					break;
			}
			return TRUE;
		}

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					static constexpr COLORREF white = RGB(0xFF, 0xFF, 0xFF);
					static constinit std::array<COLORREF, 16> customColors{ {
						white, white, white, white, white, white, white, white,
						white, white, white, white, white, white, white, white,
					} };

					CHOOSECOLOR cc{};
					cc.lStructSize = sizeof(CHOOSECOLOR);
					cc.hwndOwner = _hParent;

					cc.lpCustColors = customColors.data();
					cc.rgbResult = _colour;
					cc.lpfnHook = static_cast<LPCCHOOKPROC>(chooseColorDlgProc);
					cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ENABLEHOOK;

					Window::display(false);

					const auto dpiContext = DPIManagerV2::setThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED);
					ChooseColorW(&cc);
					DPIManagerV2::setThreadDpiAwarenessContext(dpiContext);

					::SendMessage(_hParent, WM_PICKUP_COLOR, cc.rgbResult, 0);

					return TRUE;
				}

				case IDC_COLOUR_LIST:
				{
					if (HIWORD(wParam) == LBN_SELCHANGE)
					{
						const auto i = ::SendMessage(reinterpret_cast<HWND>(lParam), LB_GETCURSEL, 0, 0);
						_colour = static_cast<COLORREF>(::SendMessage(reinterpret_cast<HWND>(lParam), LB_GETITEMDATA, i, 0));
						::SendMessage(_hParent, WM_PICKUP_COLOR, _colour, 0);
						return TRUE;
					}
					return FALSE;
				}

				default:
					return FALSE;
			}

		case WM_ACTIVATE:
		{
			if (LOWORD(wParam) == WA_INACTIVE && ::IsWindowVisible(_hSelf))
			{
				Window::display(false);
				return TRUE;
			}
			break;
		}
	}
	return FALSE;
}

uintptr_t CALLBACK ColourPopup::chooseColorDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			if (NppDarkMode::isExperimentalSupported())
			{
				NppDarkMode::setDarkTitleBar(hwnd);
			}
			NppDarkMode::autoSubclassAndThemeChildControls(hwnd);
			break;
		}

		case WM_CTLCOLOREDIT:
		{
			return NppDarkMode::onCtlColorCtrl(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORLISTBOX:
		{
			[[fallthrough]];
		}
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_ERASEBKGND:
		{
			if (NppDarkMode::isEnabled())
			{
				RECT rc = {};
				::GetClientRect(hwnd, &rc);
				::FillRect(reinterpret_cast<HDC>(wParam), &rc, NppDarkMode::getDlgBackgroundBrush());
				return TRUE;
			}
			break;
		}
	}
	return FALSE;
}
