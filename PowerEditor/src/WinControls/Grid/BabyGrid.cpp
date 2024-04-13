//BABYGRID code is copyrighted (C) 2002 by David Hillard
//
//This code must retain this copyright message
//
//Printed BABYGRID message reference and tutorial available.
//email: mudcat@mis.net for more information.

/*
Add WM_MOUSEWHEEL, WM_LBUTTONDBLCLK and WM_RBUTTONUP events
Modified by Don HO <don.h@free.fr>
*/

#include "BabyGrid.h"
#include "Parameters.h"

#define MAX_ROWS 32000
#define MAX_COLS 256


//global variables

HFONT g_hfontbody = nullptr;
HFONT g_hfontheader = nullptr;
HFONT g_hfonttitle = nullptr;

#define MAX_GRIDS 1 // Notepad++ uses only one GridHandleStruct, the old value "20" is not necessary 

struct GridHandleStruct
{
	HMENU gridmenu = nullptr;
	HWND hlist1 = nullptr;
	TCHAR protect[2]{ 'U', '\0' };
	TCHAR title[305]{};
	TCHAR editstring[305]{};
	TCHAR editstringdisplay[305]{};
	int rows = 100;
	int cols = 255;
	int gridwidth = 0;
	int gridheight = 0;
	int homerow = 1;
	int homecol = 1;
	int rowheight = 21;
	int leftvisiblecol = 0;
	int rightvisiblecol = 0;
	int topvisiblerow = 0;
	int bottomvisiblerow = 0;
	int headerrowheight = 21;
	int cursorrow = 1;
	int cursorcol = 1;
	int ownerdrawitem = 0;
	int visiblecolumns = 0;
	int titleheight = 0;
	int fontascentheight = 0;
	COLORREF cursorcolor = RGB(255, 255, 255);
	COLORREF protectcolor = RGB(255, 255, 255);
	COLORREF unprotectcolor = RGB(255, 255, 255);
	COLORREF textcolor = RGB(0, 0, 0);
	COLORREF highlightcolor = RGB(0, 0, 128);
	COLORREF highlightcolorNoFocus = RGB(200, 200, 200);
	COLORREF highlightcolorProtect = RGB(0, 0, 128);
	COLORREF highlightcolorProtectNoFocus = RGB(200, 200, 200);
	COLORREF gridlinecolor = RGB(220, 220, 220);
	COLORREF highlighttextcolor = RGB(255, 255, 255);
	COLORREF backgroundcolor = GetSysColor(COLOR_BTNFACE);
	COLORREF titletextcolor = RGB(0, 0, 0);
	COLORREF titlecolor = GetSysColor(COLOR_BTNFACE);
	COLORREF titlegridlinecolor = RGB(120, 120, 120);
	BOOL DRAWHIGHLIGHT = TRUE;
	BOOL ADVANCEROW = TRUE;
	BOOL CURRENTCELLPROTECTED = FALSE;
	BOOL GRIDHASFOCUS = FALSE;
	BOOL AUTOROW = TRUE;
	RECT activecellrect{};
	HFONT hfont = nullptr;
	HFONT hcolumnheadingfont = nullptr;
	HFONT htitlefont = nullptr;
	BOOL ROWSNUMBERED = TRUE;
	BOOL COLUMNSNUMBERED = TRUE;
	BOOL EDITABLE = FALSE;
	BOOL EDITING = FALSE;
	BOOL EXTENDLASTCOLUMN = TRUE;
	BOOL HSCROLL = FALSE;
	BOOL VSCROLL = FALSE;
	BOOL SHOWINTEGRALROWS = TRUE;
	BOOL ELLIPSIS = TRUE;
	BOOL COLAUTOWIDTH = FALSE;
	BOOL COLUMNSIZING = FALSE;
	BOOL ALLOWCOLUMNRESIZING = FALSE;
	int columntoresize = 0;
	int columntoresizeinitsize = 0;
	int columntoresizeinitx = 0;
	int cursortype = 0;
	BOOL REMEMBERINTEGRALROWS = FALSE;
	BOOL INITIALCONTENT = FALSE;
	int columnwidths[MAX_COLS + 1];

	GridHandleStruct() {
		for (int k = 0; k < MAX_COLS; k++)
			columnwidths[k] = 50;
	};

} BGHS[MAX_GRIDS];


int AddGrid(HMENU);
int FindGrid(HMENU);
void ShowVscroll(HWND, int);
void ShowHscroll(HWND, int);
int BinarySearchListBox(HWND, TCHAR*);
void DisplayEditString(HWND, int, const TCHAR*);
int CountGrids();


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
int HomeColumnNthVisible(int SI)
{
	if (SI < 0)
		return 0;

	int count = 0;
	int hc = BGHS[SI].homecol;
	for (int j = 1; j <= hc; ++j)
	{
		if (BGHS[SI].columnwidths[j] > 0)
		{
			++count;
		}
	}
	return count;
}


void RefreshGrid(HWND hWnd)
{
	RECT rect{};
	GetClientRect(hWnd, &rect);
	InvalidateRect(hWnd, &rect, FALSE);
	int SI = FindGrid(GetMenu(hWnd));
	if (SI >= 0 && BGHS[SI].EDITING)
	{
		DisplayEditString(hWnd, SI, TEXT(""));
	}

}

int GetNextColWithWidth(int SI, int startcol, int direction)
{
	int ReturnValue = 0;

	if (SI < 0)
		return ReturnValue;

	//calls with direction == 1 for right, direction == -1 for left
	//returns 0 if no more cols in that direction, else column number


	int j = startcol;
	if (direction == 1) { j++; }
	if (direction != 1) { j--; }

	while ((j > 0) && (j <= BGHS[SI].cols) && (BGHS[SI].columnwidths[j] == 0))
	{
		if (direction == 1) { j++; }
		if (direction != 1) { j--; }
	}

	if ((j <= BGHS[SI].cols) && (BGHS[SI].columnwidths[j] > 0))
	{
		ReturnValue = j;
	}
	else
	{
		ReturnValue = 0;
	}
	return ReturnValue;
}


int GetRowOfMouse(int SI, int y)
{
	if (SI < 0)
		return 0;

	if (y <= (BGHS[SI].titleheight))
	{
		return -1;
	}

	if ((y >= BGHS[SI].titleheight) && (y <= BGHS[SI].headerrowheight + BGHS[SI].titleheight))
	{
		return 0;
	}

	y = y - (BGHS[SI].headerrowheight + BGHS[SI].titleheight);
	y = y / BGHS[SI].rowheight;
	
	int ReturnValue = BGHS[SI].homerow + y;
	if (ReturnValue > BGHS[SI].rows) { ReturnValue = -1; }
	return ReturnValue;
}


int GetColOfMouse(int SI, int x)
{
	if (SI < 0)
		return 0;

	if (x <= BGHS[SI].columnwidths[0])
	{
		return 0;
	}

	x -= BGHS[SI].columnwidths[0];

	int j = BGHS[SI].homecol;
	while (x > 0)
	{
		x -= BGHS[SI].columnwidths[j];
		j++;
	}
	j--;

	int ReturnValue = j;
	if (BGHS[SI].EXTENDLASTCOLUMN)
	{
		if (j > BGHS[SI].cols) { ReturnValue = BGHS[SI].cols; }
	}
	else
	{
		if (j > BGHS[SI].cols) { ReturnValue = -1; }
	}
	return ReturnValue;
}

BOOL OutOfRange(BGCELL* cell)
{
	if ((cell->row > MAX_ROWS) || (cell->col > MAX_COLS))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void SetCell(BGCELL* cell, int row, int col)
{
	cell->row = row;
	cell->col = col;
}

void CalcVisibleCellBoundaries(int SelfIndex)
{
	int gridx = BGHS[SelfIndex].gridwidth;
	int gridy = BGHS[SelfIndex].gridheight;

	int j = BGHS[SelfIndex].homecol;
	BGHS[SelfIndex].leftvisiblecol = BGHS[SelfIndex].homecol;
	BGHS[SelfIndex].topvisiblerow = BGHS[SelfIndex].homerow;
	//calc columns visible
	//first subtract the width of col 0;
	gridx = gridx - BGHS[SelfIndex].columnwidths[0];
	do
	{
		gridx = gridx - BGHS[SelfIndex].columnwidths[j];
		j++;
	} while ((gridx >= 0) && (j < BGHS[SelfIndex].cols));

	if (j > BGHS[SelfIndex].cols) { j = BGHS[SelfIndex].cols; }
	BGHS[SelfIndex].rightvisiblecol = j;

	//calc rows visible;
	gridy = gridy - BGHS[SelfIndex].headerrowheight;
	j = BGHS[SelfIndex].homerow;
	do
	{
		gridy = gridy - BGHS[SelfIndex].rowheight;
		j++;
	} while ((gridy > 0) && (j < BGHS[SelfIndex].rows));

	if (j > BGHS[SelfIndex].rows) { j = BGHS[SelfIndex].rows; }
	BGHS[SelfIndex].bottomvisiblerow = j;
}


RECT GetCellRect(HWND hWnd, int SI, int r, int c)
{
	RECT rect{};

	if (SI < 0)
		return rect;

	//c and r must be greater than zero

	//get column offset
	  //first get col 0 width
	int offset = BGHS[SI].columnwidths[0];
	for (int j = BGHS[SI].homecol; j < c; j++)
	{
		offset += BGHS[SI].columnwidths[j];
	}
	rect.left = offset;
	rect.right = offset + BGHS[SI].columnwidths[c];

	if (BGHS[SI].EXTENDLASTCOLUMN)
	{
		//see if this is the last column
		if (!GetNextColWithWidth(SI, c, 1))
		{
			//extend this column
			RECT trect;
			int temp;
			GetClientRect(hWnd, &trect);
			temp = (offset + (trect.right - rect.left)) - rect.left;
			if (temp > BGHS[SI].columnwidths[c])
			{
				rect.right = offset + (trect.right - rect.left);
			}
		}
	}

	//now get the top and bottom of the rect
	offset = BGHS[SI].headerrowheight + BGHS[SI].titleheight;
	for (int j = BGHS[SI].homerow; j < r; j++)
	{
		offset += BGHS[SI].rowheight;
	}
	rect.top = offset;
	rect.bottom = offset + BGHS[SI].rowheight;
	return rect;
}


void DisplayTitle(HWND hWnd, int SI, HFONT hfont)
{
	if (SI < 0)
		return;

	RECT rect{};
	GetClientRect(hWnd, &rect);

	HDC gdc = GetDC(hWnd);
	SetBkMode(gdc, TRANSPARENT);
	HFONT holdfont = (HFONT)SelectObject(gdc, hfont);
	rect.bottom = BGHS[SI].titleheight;
	DrawEdge(gdc, &rect, EDGE_ETCHED, BF_MIDDLE | BF_RECT | BF_ADJUST);
	DrawTextEx(gdc, BGHS[SI].title, -1, &rect, DT_END_ELLIPSIS | DT_CENTER | DT_WORDBREAK | DT_NOPREFIX, NULL);
	SelectObject(gdc, holdfont);
	ReleaseDC(hWnd, gdc);
}

const size_t bufferLen = 1000;

void DisplayColumn(HWND hWnd, int SI, int c, int offset, HFONT hfont, HFONT hcolumnheadingfont)
{
	if (SI < 0)
		return;

	if (BGHS[SI].columnwidths[c] == 0) 
		return;

	HDC gdc = GetDC(hWnd);
	SetBkMode(gdc, TRANSPARENT);
	ShowHscroll(hWnd, SI);
	ShowVscroll(hWnd, SI);


	HFONT holdfont = (HFONT)SelectObject(gdc, hcolumnheadingfont);
	SetTextColor(gdc, BGHS[SI].titletextcolor);
	//display header row
	int r = 0;

	RECT rect{};
	rect.left = offset + 0;
	rect.top = BGHS[SI].titleheight;//0
	rect.right = BGHS[SI].columnwidths[c] + offset;
	rect.bottom = BGHS[SI].headerrowheight + BGHS[SI].titleheight;

	if (BGHS[SI].EXTENDLASTCOLUMN)
	{
		//see if this is the last column
		if (!GetNextColWithWidth(SI, c, 1))
		{
			//extend this column
			RECT trect;
			GetClientRect(hWnd, &trect);

			rect.right = offset + (trect.right - rect.left);
		}
	}
	else
	{
		if (!GetNextColWithWidth(SI, c, 1))
		{
			//repaint right side of grid
			RECT trect;
			HBRUSH holdbrush;
			HPEN holdpen;
			GetClientRect(hWnd, &trect);
			trect.left = offset + (rect.right - rect.left);
			holdbrush = (HBRUSH)SelectObject(gdc, GetStockObject(GRAY_BRUSH));
			holdpen = (HPEN)SelectObject(gdc, GetStockObject(NULL_PEN));
			Rectangle(gdc, trect.left, trect.top + BGHS[SI].titleheight, trect.right + 1, trect.bottom + 1);
			SelectObject(gdc, holdbrush);
			SelectObject(gdc, holdpen);

		}
	}

	BGCELL BGcell;
	SetCell(&BGcell, r, c);

	TCHAR buffer[bufferLen]{};
	wcscpy_s(buffer, TEXT(""));
	if (BGHS[SI].COLUMNSNUMBERED)
	{
		if (c > 0)
		{
			int high, low;
			high = ((c - 1) / 26);
			low = c % 26;
			if (high == 0) { high = 32; }
			else { high += 64; }
			if (low == 0) { low = 26; }
			low += 64;
			wsprintf(buffer, TEXT("%c%c"), high, low);
		}
	}
	else
		SendMessage(hWnd, BGM_GETCELLDATA, reinterpret_cast<WPARAM>(&BGcell), reinterpret_cast<LPARAM>(buffer));

	RECT rectsave = rect;
	HBRUSH hbrushtitle = CreateSolidBrush(BGHS[SI].titlecolor);
	HPEN hpentitle = CreatePen(PS_SOLID, 1, BGHS[SI].titlegridlinecolor);
	HBRUSH holdbrushtitle = (HBRUSH)SelectObject(gdc, hbrushtitle);
	HPEN holdpentitle = (HPEN)SelectObject(gdc, hpentitle);
	Rectangle(gdc, rect.left, rect.top, rect.right, rect.bottom);
	SelectObject(gdc, holdbrushtitle);
	SelectObject(gdc, holdpentitle);
	DeleteObject(hbrushtitle);
	DeleteObject(hpentitle);
	DrawTextEx(gdc, buffer, -1, &rect, DT_END_ELLIPSIS | DT_CENTER | DT_WORDBREAK | DT_NOPREFIX, NULL);
	rect = rectsave;

	r = BGHS[SI].topvisiblerow;
	//set font for grid body
	SelectObject(gdc, hfont);
	while (r <= BGHS[SI].bottomvisiblerow)
	{

		//try to set cursor row to different display color
		if ((r == BGHS[SI].cursorrow) && (c > 0) && (BGHS[SI].DRAWHIGHLIGHT))
		{
			if (BGHS[SI].GRIDHASFOCUS)
			{
				SetTextColor(gdc, BGHS[SI].highlighttextcolor);
			}
			else
			{
				SetTextColor(gdc, BGHS[SI].textcolor);//set black text for nonfocus grid hilight
			}
		}
		else
		{
			SetTextColor(gdc, BGHS[SI].textcolor);
		}

		rect.top = rect.bottom;
		rect.bottom = rect.top + BGHS[SI].rowheight;
		rectsave = rect;

		BGCELL BGcell;
		SetCell(&BGcell, r, c);
		wcscpy_s(buffer, TEXT(""));
		int iProperty = 0;
		if ((c == 0) && (BGHS[SI].ROWSNUMBERED))
		{
			wsprintf(buffer, TEXT("%d"), r);
			iProperty = 2 << 4; // iDataType = NUMERIC
		}
		else
		{
			// iProperty will combine (iDataType << 4) and (iProtection & 0xf), 
			// this will reduce some unnecessary and 'heavy' message calls for getting iDataType and iProtection separately
			iProperty = static_cast<int32_t>(SendMessage(hWnd, BGM_GETCELLDATA, reinterpret_cast<WPARAM>(&BGcell), reinterpret_cast<LPARAM>(buffer)));
		}

		if (c == 0)
		{
			SetTextColor(gdc, BGHS[SI].titletextcolor);
			HBRUSH hbrush = CreateSolidBrush(BGHS[SI].titlecolor);
			HPEN hpen = CreatePen(PS_SOLID, 1, BGHS[SI].titlegridlinecolor);
			HBRUSH holdbrush = (HBRUSH)SelectObject(gdc, hbrush);
			HPEN holdpen = (HPEN)SelectObject(gdc, hpen);
			Rectangle(gdc, rect.left, rect.top, rect.right, rect.bottom);
			SelectObject(gdc, holdbrush);
			SelectObject(gdc, holdpen);
			DeleteObject(hbrush);
			DeleteObject(hpen);
		}
		else
		{
			HBRUSH hbrush = nullptr;
			int iProtection = iProperty & 0xf;
			if (BGHS[SI].DRAWHIGHLIGHT)//highlight on
			{
				if (r == BGHS[SI].cursorrow)
				{
					if (BGHS[SI].GRIDHASFOCUS)
					{
						if (iProtection == 1)
							hbrush = CreateSolidBrush(BGHS[SI].highlightcolorProtect);
						else
							hbrush = CreateSolidBrush(BGHS[SI].highlightcolor);
					}
					else
					{
						if (iProtection == 1)
							hbrush = CreateSolidBrush(BGHS[SI].highlightcolorProtectNoFocus);
						else
							hbrush = CreateSolidBrush(BGHS[SI].highlightcolorNoFocus);
					}
				}

				else
				{
					if (iProtection == 1)
					{
						hbrush = CreateSolidBrush(BGHS[SI].protectcolor);
					}
					else
					{
						hbrush = CreateSolidBrush(BGHS[SI].unprotectcolor);
					}
				}
			}
			else
			{
				if (iProtection == 1)
				{
					hbrush = CreateSolidBrush(BGHS[SI].protectcolor);
				}
				else
				{
					hbrush = CreateSolidBrush(BGHS[SI].unprotectcolor);
				}

			}
			HPEN hpen = CreatePen(PS_SOLID, 1, BGHS[SI].gridlinecolor);
			HBRUSH holdbrush = (HBRUSH)SelectObject(gdc, hbrush);
			HPEN holdpen = (HPEN)SelectObject(gdc, hpen);
			Rectangle(gdc, rect.left, rect.top, rect.right, rect.bottom);
			SelectObject(gdc, holdbrush);
			SelectObject(gdc, holdpen);
			DeleteObject(hbrush);
			DeleteObject(hpen);
		}
		rect.right -= 2;
		rect.left += 2;

		int iDataType = iProperty >> 4 & 0xf;
		if ((iDataType < 1) || (iDataType > 5))
		{
			iDataType = 1;//default to alphanumeric data type.. can't happen
		}
		//if(c==0){iDataType = 2;}

		if (iDataType == 1)//ALPHA
		{
			if (BGHS[SI].ELLIPSIS)
			{
				DrawTextEx(gdc, buffer, -1, &rect, DT_END_ELLIPSIS | DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX, NULL);
			}
			else
			{
				DrawTextEx(gdc, buffer, -1, &rect, DT_LEFT | DT_WORDBREAK | DT_EDITCONTROL | DT_NOPREFIX, NULL);
			}
		}

		if (iDataType == 2)//NUMERIC
		{
			DrawTextEx(gdc, buffer, -1, &rect, DT_END_ELLIPSIS | DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX, NULL);
		}

		if (iDataType == 3)//BOOLEAN TRUE
		{
			int k = 2;
			rect.top += k;
			rect.bottom -= k;
			rect.left += 0;
			rect.right -= 0;
			if ((rect.bottom - rect.top) > 24)
			{
				int excess = (rect.bottom - rect.top) - 16;
				rect.top += excess / 2;
				rect.bottom -= excess / 2;
			}
			DrawFrameControl(gdc, &rect, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_CHECKED);
		}

		if (iDataType == 4)//BOOLEAN FALSE
		{
			int k = 2;
			rect.top += k;
			rect.bottom -= k;
			rect.left += 0;
			rect.right -= 0;
			if ((rect.bottom - rect.top) > 24)
			{
				int excess = (rect.bottom - rect.top) - 16;
				rect.top += excess / 2;
				rect.bottom -= excess / 2;
			}
			DrawFrameControl(gdc, &rect, DFC_BUTTON, DFCS_BUTTONCHECK);
		}

		if (iDataType == 5) //user drawn graphic
		{
			buffer[0] = 0x20;
			BGHS[SI].ownerdrawitem = _wtoi(buffer);
			WPARAM wParam = MAKEWPARAM(::GetMenu(hWnd), BGN_OWNERDRAW);
			SendMessage(GetParent(hWnd), WM_COMMAND, wParam, reinterpret_cast<LPARAM>(&rect));
		}

		if (BGHS[SI].EDITING)
		{
			DisplayEditString(hWnd, SI, TEXT(""));
		}

		rect = rectsave;
		r++;
	}//end while r<=bottomvisiblerow

	{
		//repaint bottom of grid
		RECT trect{};
		GetClientRect(hWnd, &trect);
		trect.top = rect.bottom;
		trect.left = rect.left;
		trect.right = rect.right;

		HBRUSH hbrush = CreateSolidBrush(BGHS[SI].backgroundcolor);
		HBRUSH holdbrush = (HBRUSH)SelectObject(gdc, hbrush);
		HPEN holdpen = (HPEN)SelectObject(gdc, GetStockObject(NULL_PEN));

		Rectangle(gdc, trect.left, trect.top, trect.right + 1, trect.bottom + 1);

		SelectObject(gdc, holdbrush);
		SelectObject(gdc, holdpen);
		DeleteObject(hbrush);

	}

	SelectObject(gdc, holdfont);
	DeleteObject(holdfont);
	ReleaseDC(hWnd, gdc);
}


void DrawCursor(HWND hWnd, int SI)
{
	if (SI < 0)
		return;

	RECT rect{};
	if (BGHS[SI].rows == 0) { return; }
	GetClientRect(hWnd, &rect);
	//if active cell has scrolled off the top, don't draw a focus rectangle
	if (BGHS[SI].cursorrow < BGHS[SI].homerow) { return; }
	//if active cell has scrolled off to the left, don't draw a focus rectangle
	if (BGHS[SI].cursorcol < BGHS[SI].homecol) { return; }

	rect = GetCellRect(hWnd, SI, BGHS[SI].cursorrow, BGHS[SI].cursorcol);
	RECT rectwhole = rect;
	HDC gdc = GetDC(hWnd);
	BGHS[SI].activecellrect = rect;
	int rop = GetROP2(gdc);
	SetROP2(gdc, R2_XORPEN);
	SelectObject(gdc, (HBRUSH)GetStockObject(NULL_BRUSH));
	HPEN hpen = CreatePen(PS_SOLID, 3, BGHS[SI].cursorcolor);  //width of 3
	HPEN holdpen = (HPEN)SelectObject(gdc, hpen);
	Rectangle(gdc, rect.left, rect.top, rect.right, rect.bottom);
	SelectObject(gdc, holdpen);
	DeleteObject(hpen);
	SetROP2(gdc, rop);
	ReleaseDC(hWnd, gdc);
}

void SetCurrentCellStatus(HWND hWnd, int SelfIndex)
{
	BGCELL BGcell;
	SetCell(&BGcell, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
	if (SendMessage(hWnd, BGM_GETPROTECTION, reinterpret_cast<WPARAM>(&BGcell), 0))
	{
		BGHS[SelfIndex].CURRENTCELLPROTECTED = TRUE;
	}
	else
	{
		BGHS[SelfIndex].CURRENTCELLPROTECTED = FALSE;
	}

}

TCHAR GetASCII(WPARAM wParam, LPARAM lParam)
{
	TCHAR mbuffer[100]{};
	BYTE keys[256]{};
	WORD dwReturnedValue = 0;
	GetKeyboardState(keys);
	int result = ToAscii(static_cast<UINT>(wParam), (lParam >> 16) & 0xff, keys, &dwReturnedValue, 0);
	int returnvalue = (TCHAR)dwReturnedValue;
	if (returnvalue < 0) { returnvalue = 0; }
	wsprintf(mbuffer, TEXT("return value = %d"), returnvalue);
	if (result != 1) { returnvalue = 0; }
	return (TCHAR)returnvalue;

}



void SetHomeRow(HWND hWnd, int SI, int row, int col)
{
	if (SI < 0)
		return;

	RECT gridrect{};
	//get rect of grid window
	GetClientRect(hWnd, &gridrect);
	//get rect of current cell
	RECT cellrect = GetCellRect(hWnd, SI, row, col);
	if ((cellrect.bottom > gridrect.bottom) && ((cellrect.bottom - cellrect.top) < (gridrect.bottom - (BGHS[SI].headerrowheight + BGHS[SI].titleheight))))
	{
		while (cellrect.bottom > gridrect.bottom)
		{
			BGHS[SI].homerow++;
			if (row == BGHS[SI].rows)
			{
				gridrect.top = gridrect.bottom - (BGHS[SI].rowheight);
				InvalidateRect(hWnd, &gridrect, TRUE);
			}
			else
			{
				InvalidateRect(hWnd, &gridrect, FALSE);
			}
			cellrect = GetCellRect(hWnd, SI, row, col);
		}
	}
	else
	{
		if ((cellrect.bottom - cellrect.top) >= (gridrect.bottom - (BGHS[SI].headerrowheight + BGHS[SI].titleheight)))
		{
			BGHS[SI].homerow++;
		}
	}
	cellrect = GetCellRect(hWnd, SI, row, col);
	{
		while ((row < BGHS[SI].homerow))
		{
			BGHS[SI].homerow--;
			InvalidateRect(hWnd, &gridrect, FALSE);
			cellrect = GetCellRect(hWnd, SI, row, col);
		}
	}
	//set the vertical scrollbar position
	SetScrollPos(hWnd, SB_VERT, BGHS[SI].homerow, TRUE);
}



void SetHomeCol(HWND hWnd, int SI, int row, int col)
{
	if (SI < 0)
		return;

	RECT gridrect{};
	//get rect of grid window
	GetClientRect(hWnd, &gridrect);
	//get rect of current cell
	RECT cellrect = GetCellRect(hWnd, SI, row, col);
	//determine if scroll left or right is needed
	while ((cellrect.right > gridrect.right) && (cellrect.left != BGHS[SI].columnwidths[0]))
	{
		//scroll right is needed
		BGHS[SI].homecol++;
		cellrect = GetCellRect(hWnd, SI, row, col);
		InvalidateRect(hWnd, &gridrect, FALSE);
	}
	cellrect = GetCellRect(hWnd, SI, row, col);

	while ((BGHS[SI].cursorcol < BGHS[SI].homecol) && (BGHS[SI].homecol > 1))

	{
		//scroll left is needed
		BGHS[SI].homecol--;

		cellrect = GetCellRect(hWnd, SI, row, col);
		InvalidateRect(hWnd, &gridrect, FALSE);
	}

	{
		int k = HomeColumnNthVisible(SI);
		SetScrollPos(hWnd, SB_HORZ, k, TRUE);
	}
}



void ShowVscroll(HWND hWnd, int SI)
{
	if (SI < 0)
		return;

	//if more rows than can be visible on grid, display vertical scrollbar
	//otherwise, hide it.
	RECT gridrect{};
	GetClientRect(hWnd, &gridrect);
	int totalpixels = gridrect.bottom;
	totalpixels -= BGHS[SI].titleheight;
	totalpixels -= BGHS[SI].headerrowheight;
	totalpixels -= (BGHS[SI].rowheight * BGHS[SI].rows);
	int rowsvisibleonscreen = (gridrect.bottom - (BGHS[SI].headerrowheight + BGHS[SI].titleheight)) / BGHS[SI].rowheight;
	if (totalpixels < 0)
	{
		//show vscrollbar
		ShowScrollBar(hWnd, SB_VERT, TRUE);
		SetScrollRange(hWnd, SB_VERT, 1, (BGHS[SI].rows - rowsvisibleonscreen) + 1, TRUE);
		BGHS[SI].VSCROLL = TRUE;
	}
	else
	{
		//hide vscrollbar
		ShowScrollBar(hWnd, SB_VERT, FALSE);
		BGHS[SI].VSCROLL = FALSE;
	}

}

void ShowHscroll(HWND hWnd, int SI)
{
	if (SI < 0)
		return;

	//if more rows than can be visible on grid, display vertical scrollbar
	//otherwise, hide it.
	RECT gridrect{};
	GetClientRect(hWnd, &gridrect);
	int totalpixels = gridrect.right;
	totalpixels -= BGHS[SI].columnwidths[0];
	int colswithwidth = 0;
	for (int j = 1; j <= BGHS[SI].cols; j++)
	{
		totalpixels -= BGHS[SI].columnwidths[j];
		if (BGHS[SI].columnwidths[j] > 0)
		{
			colswithwidth++;
		}
	}
	if (totalpixels < 0)
	{
		//show hscrollbar
		ShowScrollBar(hWnd, SB_HORZ, TRUE);
		SetScrollRange(hWnd, SB_HORZ, 1, colswithwidth, TRUE);
		BGHS[SI].HSCROLL = TRUE;
	}
	else
	{
		//hide hscrollbar
		ShowScrollBar(hWnd, SB_HORZ, FALSE);
		BGHS[SI].HSCROLL = FALSE;
	}

}



void NotifyRowChanged(HWND hWnd, int SI)
{
	if (SI < 0)
		return;

	LPARAM lParam = MAKELPARAM(BGHS[SI].cursorrow, BGHS[SI].cursorcol);
	WPARAM wParam = MAKEWPARAM(BGHS[SI].gridmenu, BGN_ROWCHANGED);
	SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);
	wParam = MAKEWPARAM(BGHS[SI].gridmenu, BGN_SELCHANGE);
	SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);
}


void NotifyColChanged(HWND hWnd, int SI)
{
	if (SI < 0)
		return;

	LPARAM lParam = MAKELPARAM(BGHS[SI].cursorrow, BGHS[SI].cursorcol);
	WPARAM wParam = MAKEWPARAM(BGHS[SI].gridmenu, BGN_COLCHANGED);
	SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);
	wParam = MAKEWPARAM(BGHS[SI].gridmenu, BGN_SELCHANGE);
	SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);

}


void NotifyEndEdit(HWND hWnd, int SI)
{
	if (SI < 0)
		return;

	LPARAM lParam = MAKELPARAM(BGHS[SI].cursorrow, BGHS[SI].cursorcol);
	WPARAM wParam = MAKEWPARAM(BGHS[SI].gridmenu, BGN_EDITEND);
	SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);

}


void NotifyDelete(HWND hWnd, int SI)
{
	if (SI < 0)
		return;

	LPARAM lParam = MAKELPARAM(BGHS[SI].cursorrow, BGHS[SI].cursorcol);
	WPARAM wParam = MAKEWPARAM(BGHS[SI].gridmenu, BGN_DELETECELL);
	SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);

}


void NotifyEditBegin(HWND hWnd, int SI)
{
	if (SI < 0)
		return;

	LPARAM lParam = MAKELPARAM(BGHS[SI].cursorrow, BGHS[SI].cursorcol);
	WPARAM wParam = MAKEWPARAM(BGHS[SI].gridmenu, BGN_EDITBEGIN);
	SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);

}

void NotifyEditEnd(HWND hWnd, int SI)
{
	if (SI < 0)
		return;

	LPARAM lParam = MAKELPARAM(BGHS[SI].cursorrow, BGHS[SI].cursorcol);
	WPARAM wParam = MAKEWPARAM(BGHS[SI].gridmenu, BGN_EDITEND);
	SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);

}

void NotifyCellClicked(HWND hWnd, int SI)
{
	if (SI < 0)
		return;

	WPARAM wParam;
	LPARAM lParam;
	lParam = MAKELPARAM(BGHS[SI].cursorrow, BGHS[SI].cursorcol);
	wParam = MAKEWPARAM(BGHS[SI].gridmenu, BGN_CELLCLICKED);
	SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);

}

void NotifyCellDbClicked(HWND hWnd, int SI)
{
	if (SI < 0)
		return;

	LPARAM lParam = MAKELPARAM(BGHS[SI].cursorrow, BGHS[SI].cursorcol);
	WPARAM wParam = MAKEWPARAM(BGHS[SI].gridmenu, BGN_CELLDBCLICKED);
	SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);
}

void NotifyCellRClicked(HWND hWnd, int SI)
{
	LPARAM lParam = MAKELPARAM(BGHS[SI].cursorrow, BGHS[SI].cursorcol);
	WPARAM wParam = MAKEWPARAM(BGHS[SI].gridmenu, BGN_CELLRCLICKED);
	SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);
}
void GetVisibleColumns(HWND hWnd, int SI)
{
	if (SI < 0)
		return;

	int value = 0;
	for (int j = 1; j <= BGHS[SI].cols; j++)
	{
		if (BGHS[SI].columnwidths[j] > 0)
		{
			value++;
		}
	}
	BGHS[SI].visiblecolumns = value;
	SetScrollRange(hWnd, SB_HORZ, 1, value, TRUE);
}

int GetNthVisibleColumn(HWND, int SI, int n)
{
	if (SI < 0)
		return 0;

	int j = 1;
	int count = 0;
	int value = n - 1;
	while (j <= BGHS[SI].cols)
	{
		if (BGHS[SI].columnwidths[j] > 0)
		{
			count++;
			if (count == n)
			{
				value = j;
			}
		}
		j++;
	}
	return value;
}


void CloseEdit(HWND hWnd, int SI)
{
	if (SI < 0)
		return;

	BGCELL cell;
	int r = BGHS[SI].cursorrow;
	int c = BGHS[SI].cursorcol;
	cell.row = r;
	cell.col = c;
	SendMessage(hWnd, BGM_SETCELLDATA, reinterpret_cast<WPARAM>(&cell), reinterpret_cast<LPARAM>(BGHS[SI].editstring));
	wcscpy_s(BGHS[SI].editstring, TEXT(""));
	RefreshGrid(hWnd);
	BGHS[SI].EDITING = FALSE;
	HideCaret(hWnd);
	NotifyEditEnd(hWnd, SI);
}

void DisplayEditString(HWND hWnd, int SI, const TCHAR* tstring)
{
	if (SI < 0)
		return;

	int r = BGHS[SI].cursorrow;
	int c = BGHS[SI].cursorcol;
	ShowCaret(hWnd);
	if ((r < BGHS[SI].homerow) || (c < BGHS[SI].homecol))
	{
		HideCaret(hWnd);
		return;
	}
	RECT rt = GetCellRect(hWnd, SI, r, c);
	rt.top += 2;
	rt.bottom -= 2;
	rt.right -= 2;
	rt.left += 2;

	HDC cdc = GetDC(hWnd);
	Rectangle(cdc, rt.left, rt.top, rt.right, rt.bottom);
	rt.top += 2;
	rt.bottom -= 2;
	rt.right -= 2;
	rt.left += 2;

	if (lstrlen(BGHS[SI].editstring) <= 300)
	{
		wcscat_s(BGHS[SI].editstring, tstring);
		wcscat_s(BGHS[SI].editstringdisplay, BGHS[SI].editstring);
	}
	else
	{
		if (!NppParameters::getInstance().getNppGUI()._muteSounds)
			MessageBeep(0);
	}

	HFONT holdfont = (HFONT)SelectObject(cdc, BGHS[SI].hfont);
	rt.right -= 5;
	DrawText(cdc, BGHS[SI].editstringdisplay, -1, &rt, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
	rt.right += 5;
	ShowCaret(hWnd);


	int rh = BGHS[SI].rowheight;
	int ah = BGHS[SI].fontascentheight;

	SetCaretPos(rt.right - 4, rt.top + (rh / 2) - ah + 2);

	SelectObject(cdc, holdfont);
	ReleaseDC(hWnd, cdc);
}
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////


ATOM RegisterGridClass(HINSTANCE hInstance)
{
	WNDCLASS wclass{};

	wclass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wclass.lpfnWndProc = (WNDPROC)GridProc;
	wclass.cbClsExtra = 0;
	wclass.cbWndExtra = 0;
	wclass.hInstance = hInstance;
	wclass.hIcon = NULL;
	wclass.hCursor = ::LoadCursor(NULL, IDC_ARROW);

	wclass.hbrBackground = (HBRUSH)(GetStockObject(GRAY_BRUSH));
	wclass.lpszClassName = TEXT("BABYGRID");
	wclass.lpszMenuName = NULL;

	return RegisterClass(&wclass);
}


void SizeGrid(HWND hWnd)
{
	SendMessage(hWnd, WM_SIZE, SIZE_MAXIMIZED, 0);
}

int FindLongestLine(HDC hdc, wchar_t* text, SIZE* size)
{
	int longest = 0;
	wchar_t* buffer = nullptr;
	wchar_t* token = WCSTOK(text, TEXT("\n"), &buffer);

	while (token)
	{
		::GetTextExtentPoint32(hdc, token, lstrlen(token), size);
		if (size->cx > longest)
		{
			longest = size->cx;
		}

		token = WCSTOK(nullptr, TEXT("\n"), &buffer);
	}
	return longest;
}


LRESULT CALLBACK GridProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR buffer[bufferLen]{};
	int ReturnValue = FALSE;

	int SelfIndex = FindGrid(GetMenu(hWnd));

	// the following check will prevent memory overwriting bug by BabyGrid during the WM_NCCREATE, WM_NCCALCSIZE, WM_CREATE and WM_NCDESTROY
	// - TODO: if the above FindGrid can theoretically return -1, the whole BabyGrid code here is theoretically flawed and needs a review
	//         (luckily it seems it never returns -1 apart from the aforementioned 4 WMs above)
	if (SelfIndex != -1)
	{
		// update the grid width and height variable
		RECT rect{};
		::GetClientRect(hWnd, &rect);
		BGHS[SelfIndex].gridwidth = rect.right - rect.left;
		BGHS[SelfIndex].gridheight = rect.bottom - rect.top;
	}

	switch (message)
	{
		case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);
			// Parse the menu selections:
			switch (wmId)
			{
				case 1:
				default:
					return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps{};
			BeginPaint(hWnd, &ps);
			RECT rt{};
			GetClientRect(hWnd, &rt);
			CalcVisibleCellBoundaries(SelfIndex);
			//display title
			if (BGHS[SelfIndex].titleheight > 0)
				DisplayTitle(hWnd, SelfIndex, BGHS[SelfIndex].htitlefont);
			//display column 0;

			DisplayColumn(hWnd, SelfIndex, 0, 0, BGHS[SelfIndex].hfont, BGHS[SelfIndex].hcolumnheadingfont);
			{
				int offset = BGHS[SelfIndex].columnwidths[0];
				int j = BGHS[SelfIndex].leftvisiblecol;
				int k = BGHS[SelfIndex].rightvisiblecol;

				for (int c = j; c <= k; c++)
				{
					DisplayColumn(hWnd, SelfIndex, c, offset, BGHS[SelfIndex].hfont, BGHS[SelfIndex].hcolumnheadingfont);
					offset += BGHS[SelfIndex].columnwidths[c];
				}

			}
			EndPaint(hWnd, &ps);
			//
			if (GetFocus() == hWnd)
			{
				PostMessage(hWnd, BGM_DRAWCURSOR, (UINT)SelfIndex, 0);
			}

		}
		break;


		case BGM_PAINTGRID:
		{
			RECT rect{};
			GetClientRect(hWnd, &rect);
			InvalidateRect(hWnd, &rect, TRUE);
			UpdateWindow(hWnd);
			if (!NppParameters::getInstance().getNppGUI()._muteSounds)
				MessageBeep(0);
		}
		break;

		case WM_SETTEXT:
		{
			if (lstrlen((TCHAR*)lParam) > 300)
			{
				wcscpy_s(BGHS[SelfIndex].title, TEXT("Title too long (300 chars max)"));
			}
			else
			{
				wcscpy_s(BGHS[SelfIndex].title, (TCHAR*)lParam);
			}

			HDC gdc = GetDC(hWnd);
			//get linecount of title;
			if (lstrlen(BGHS[SelfIndex].title) > 0)
			{
				int linecount = 1;
				for (int j = 0; j < static_cast<int>(lstrlen(BGHS[SelfIndex].title)); j++)
				{
					if (BGHS[SelfIndex].title[j] == '\n')
					{
						linecount++;
					}
				}
				HFONT holdfont = (HFONT)SelectObject(gdc, BGHS[SelfIndex].htitlefont);
				SIZE size{};
				GetTextExtentPoint32(gdc, BGHS[SelfIndex].title, lstrlen(BGHS[SelfIndex].title), &size);
				SelectObject(gdc, holdfont);
				BGHS[SelfIndex].titleheight = static_cast<int>((size.cy * 1.2) * linecount);
			}
			else
			{
				//no title
				BGHS[SelfIndex].titleheight = 0;
			}
			ReleaseDC(hWnd, gdc);


			RefreshGrid(hWnd);
			SizeGrid(hWnd);

		}
		break;

		case BGM_GETROWS:
			ReturnValue = BGHS[SelfIndex].rows;
			break;

		case BGM_GETCOLS:
			ReturnValue = BGHS[SelfIndex].cols;
			break;

		case BGM_GETCOLWIDTH:
			ReturnValue = BGHS[SelfIndex].columnwidths[wParam];
			break;

		case BGM_GETROWHEIGHT:
			ReturnValue = BGHS[SelfIndex].rowheight;
			break;

		case BGM_GETHEADERROWHEIGHT:
			ReturnValue = BGHS[SelfIndex].headerrowheight;
			break;

		case BGM_GETOWNERDRAWITEM:
			ReturnValue = BGHS[SelfIndex].ownerdrawitem;
			break;

		case BGM_DRAWCURSOR:
			DrawCursor(hWnd, static_cast<int32_t>(wParam));
			break;
		case BGM_SETCURSORPOS:
			DrawCursor(hWnd, SelfIndex);
			if ((((int)wParam <= BGHS[SelfIndex].rows) && ((int)wParam > 0)) &&
				(((int)lParam <= BGHS[SelfIndex].cols) && ((int)lParam > 0)))
			{
				BGHS[SelfIndex].cursorrow = static_cast<int32_t>(wParam);
				BGHS[SelfIndex].cursorcol = static_cast<int32_t>(lParam);
			}
			else
			{
				DrawCursor(hWnd, SelfIndex);
				break;
			}
			SetHomeRow(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
			SetHomeCol(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
			DrawCursor(hWnd, SelfIndex);
			RefreshGrid(hWnd);

			break;

		case BGM_SETLASTVIEW:
			if ((((int)wParam <= BGHS[SelfIndex].rows) && ((int)wParam > 0)) &&
				(((int)lParam <= BGHS[SelfIndex].rows) && ((int)lParam > 0)))
			{
				BGHS[SelfIndex].homerow = static_cast<int32_t>(wParam);
				BGHS[SelfIndex].homecol = 1;
				BGHS[SelfIndex].cursorrow = static_cast<int32_t>(lParam);
				BGHS[SelfIndex].cursorcol = 1;

				SetHomeRow(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				RefreshGrid(hWnd);

				NotifyRowChanged(hWnd, SelfIndex);
			}
			break;

		case BGM_SETINITIALCONTENT:
			BGHS[SelfIndex].INITIALCONTENT = (BOOL)wParam;
			break;

		case BGM_SHOWHILIGHT:
			BGHS[SelfIndex].DRAWHIGHLIGHT = (BOOL)wParam;
			RefreshGrid(hWnd);
			break;
		case BGM_EXTENDLASTCOLUMN:
			BGHS[SelfIndex].EXTENDLASTCOLUMN = (BOOL)wParam;
			RefreshGrid(hWnd);
			break;

		case BGM_SHOWINTEGRALROWS:
			BGHS[SelfIndex].SHOWINTEGRALROWS = (BOOL)wParam;
			SizeGrid(hWnd);
			RefreshGrid(hWnd);
			break;

		case BGM_SETCOLAUTOWIDTH:
			BGHS[SelfIndex].COLAUTOWIDTH = (BOOL)wParam;
			break;

		case BGM_SETALLOWCOLRESIZE:
			BGHS[SelfIndex].ALLOWCOLUMNRESIZING = (BOOL)wParam;
			break;

		case BGM_PROTECTCELL:
		{
			BGCELL* LPBGcell = (BGCELL*)wParam;
			if (OutOfRange(LPBGcell))
			{
				wParam = MAKEWPARAM(GetMenu(hWnd), BGN_OUTOFRANGE);
				lParam = 0;
				SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);
				ReturnValue = -1;
				break;
			}
			wsprintf(buffer, TEXT("%05d-%03d"), LPBGcell->row, LPBGcell->col);
			//see if that cell is already loaded
			int FindResult = BinarySearchListBox(BGHS[SelfIndex].hlist1, buffer);
			if (FindResult != LB_ERR)
			{
				//it was found, get the text, modify text delete it from list, add modified to list
				auto lbTextLen = ::SendMessage(BGHS[SelfIndex].hlist1, LB_GETTEXTLEN, FindResult, 0);
				if (static_cast<size_t>(lbTextLen) > bufferLen)
					return TRUE;

				SendMessage(BGHS[SelfIndex].hlist1, LB_GETTEXT, FindResult, reinterpret_cast<LPARAM>(buffer));
				if ((BOOL)lParam)
				{
					buffer[10] = 'P';
				}
				else
				{
					buffer[10] = 'U';
				}
				SendMessage(BGHS[SelfIndex].hlist1, LB_DELETESTRING, FindResult, 0);
				SendMessage(BGHS[SelfIndex].hlist1, LB_ADDSTRING, FindResult, reinterpret_cast<LPARAM>(buffer));
			}
			else
			{
				//protecting or unprotecting a cell that isn't in the list
				//add it as blank;
				wcscat_s(buffer, TEXT("|"));
				if ((BOOL)lParam)
				{
					wcscat_s(buffer, TEXT("PA"));
				}
				else
				{
					wcscat_s(buffer, TEXT("UA"));
				}
				wcscat_s(buffer, TEXT("|"));
				SendMessage(BGHS[SelfIndex].hlist1, LB_ADDSTRING, FindResult, reinterpret_cast<LPARAM>(buffer));
			}
		}
		break;

		case BGM_NOTIFYROWCHANGED:
			NotifyRowChanged(hWnd, SelfIndex);
			break;
		case BGM_NOTIFYCOLCHANGED:
			NotifyColChanged(hWnd, SelfIndex);
			break;
		case BGM_SETPROTECT:
			if ((BOOL)wParam)
			{
				wcscpy_s(BGHS[SelfIndex].protect, TEXT("P"));
			}
			else
			{
				wcscpy_s(BGHS[SelfIndex].protect, TEXT("U"));
			}
			break;

		case BGM_AUTOROW:
			if ((BOOL)wParam)
			{
				BGHS[SelfIndex].AUTOROW = TRUE;
			}
			else
			{
				BGHS[SelfIndex].AUTOROW = FALSE;
			}
			break;
		case BGM_SETEDITABLE:
			if ((BOOL)wParam)
			{
				BGHS[SelfIndex].EDITABLE = TRUE;
			}
			else
			{
				BGHS[SelfIndex].EDITABLE = FALSE;
			}
			break;

		case BGM_SETCELLDATA:
		{
			BGCELL* LPBGcell = (BGCELL*)wParam;
			if (OutOfRange(LPBGcell))
			{
				wParam = MAKEWPARAM(GetMenu(hWnd), BGN_OUTOFRANGE);
				lParam = 0;
				SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);
				ReturnValue = -1;
				break;
			}
			wsprintf(buffer, TEXT("%05d-%03d"), LPBGcell->row, LPBGcell->col);

			if (!BGHS[SelfIndex].INITIALCONTENT) // performance enhancement while adding new data
			{
				//see if that cell is already loaded
				int FindResult = BinarySearchListBox(BGHS[SelfIndex].hlist1, buffer);
				if (FindResult != LB_ERR)
				{
					//it was found, delete it
					SendMessage(BGHS[SelfIndex].hlist1, LB_DELETESTRING, FindResult, 0);
				}
			}

			//now add it
			wcscat_s(buffer, TEXT("|"));
			wcscat_s(buffer, BGHS[SelfIndex].protect);

			int iDataType = 1;

			if (iDataType == 1) { wcscat_s(buffer, TEXT("A")); }
			if (iDataType == 2) { wcscat_s(buffer, TEXT("N")); }
			if (iDataType == 3) { wcscat_s(buffer, TEXT("T")); }
			if (iDataType == 4) { wcscat_s(buffer, TEXT("F")); }
			if (iDataType == 5) { wcscat_s(buffer, TEXT("G")); }

			wcscat_s(buffer, TEXT("|"));
			wcscat_s(buffer, (TCHAR*)lParam);
			int FindResult = static_cast<int32_t>(SendMessage(BGHS[SelfIndex].hlist1, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(buffer)));

			if (FindResult == LB_ERR)
			{
				if (!NppParameters::getInstance().getNppGUI()._muteSounds)
					MessageBeep(0);
			}
			{
				RECT rect{};
				rect = GetCellRect(hWnd, SelfIndex, LPBGcell->row, LPBGcell->col);
				InvalidateRect(hWnd, &rect, FALSE);
			}
			//get the last line and adjust grid dimmensions
			if (BGHS[SelfIndex].AUTOROW)
			{
				int j = static_cast<int32_t>(SendMessage(BGHS[SelfIndex].hlist1, LB_GETCOUNT, 0, 0));
				if (j > 0)
				{
					auto lbTextLen = ::SendMessage(BGHS[SelfIndex].hlist1, LB_GETTEXTLEN, j - 1, 0);
					if (static_cast<size_t>(lbTextLen) > bufferLen)
						return TRUE;
					SendMessage(BGHS[SelfIndex].hlist1, LB_GETTEXT, j - 1, reinterpret_cast<LPARAM>(buffer));
					buffer[5] = 0x00;
					j = _wtoi(buffer);
					if (j > SendMessage(hWnd, BGM_GETROWS, 0, 0))
					{
						SendMessage(hWnd, BGM_SETGRIDDIM, j, BGHS[SelfIndex].cols);
					}
				}
				else
				{
					//no items in the list
					SendMessage(hWnd, BGM_SETGRIDDIM, j, BGHS[SelfIndex].cols);
				}
			}

			//adjust the column width if COLAUTOWIDTH==TRUE
			if ((BGHS[SelfIndex].COLAUTOWIDTH) || (LPBGcell->row == 0))
			{
				SIZE size{ 0, 0 };
				int required_height = 30;
				int current_height = 0;
				HFONT holdfont = nullptr;
				HDC hdc = GetDC(hWnd);
				if (LPBGcell->row == 0)
				{
					holdfont = (HFONT)SelectObject(hdc, BGHS[SelfIndex].hcolumnheadingfont);
				}
				else
				{
					holdfont = (HFONT)SelectObject(hdc, BGHS[SelfIndex].hfont);
				}
				//if there are \n codes in the generic_string, find the longest line
				int longestline = FindLongestLine(hdc, (TCHAR*)lParam, &size);
				int required_width = longestline + 15;
				required_height = size.cy;
				//count lines
				{
					int count = 1;
					TCHAR tbuffer[255] = { '\0' };
					wcscpy_s(tbuffer, (TCHAR*)lParam);
					for (int j = 0; j < (int)lstrlen(tbuffer); j++)
					{
						if (tbuffer[j] == '\n') { count++; }
					}
					if ((!BGHS[SelfIndex].ELLIPSIS) || (LPBGcell->row == 0))
					{
						required_height *= count;
					}
					required_height += 5;
				}
				SelectObject(hdc, holdfont);
				ReleaseDC(hWnd, hdc);
				int current_width = BGHS[SelfIndex].columnwidths[LPBGcell->col];
				if (LPBGcell->row == 0)
				{
					current_height = BGHS[SelfIndex].headerrowheight;
					if (required_height > current_height)
					{
						SendMessage(hWnd, BGM_SETHEADERROWHEIGHT, required_height, 0);
					}
				}
				else
				{
					current_height = BGHS[SelfIndex].rowheight;
					if (required_height > current_height)
					{
						SendMessage(hWnd, BGM_SETROWHEIGHT, required_height, 0);
					}

				}
				if (required_width > current_width)
				{
					SendMessage(hWnd, BGM_SETCOLWIDTH, LPBGcell->col, required_width);
				}
				ReleaseDC(hWnd, hdc);
			}
		}
		break;

		case BGM_GETCELLDATA:
		{
			BGCELL* LPBGcell = (BGCELL*)wParam;
			if (OutOfRange(LPBGcell))
			{
				wParam = MAKEWPARAM(GetMenu(hWnd), BGN_OUTOFRANGE);
				lParam = 0;
				SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);
				ReturnValue = -1;
				break;
			}
			wsprintf(buffer, TEXT("%05d-%03d"), LPBGcell->row, LPBGcell->col);
			//see if that cell is already loaded
			int FindResult = BinarySearchListBox(BGHS[SelfIndex].hlist1, buffer);
			if (FindResult != LB_ERR)
			{
				//it was found, get it
				auto lbTextLen = ::SendMessage(BGHS[SelfIndex].hlist1, LB_GETTEXTLEN, FindResult, 0);
				if (static_cast<size_t>(lbTextLen) > bufferLen)
					return TRUE;
				SendMessage(BGHS[SelfIndex].hlist1, LB_GETTEXT, FindResult, reinterpret_cast<LPARAM>(buffer));
				switch (buffer[10]) // no need to call BGM_GETPROTECTION separately for this
				{
					case 'U': ReturnValue = 0; break;
					case 'P': ReturnValue = 1; break;
					default: ReturnValue = 0; break;
				}
				switch (buffer[11]) // no need to call BGM_GETTYPE separately for this
				{
					case 'A': ReturnValue |= 1 << 4; break;
					case 'N': ReturnValue |= 2 << 4; break;
					case 'T': ReturnValue |= 3 << 4; break;
					case 'F': ReturnValue |= 4 << 4; break;
					case 'G': ReturnValue |= 5 << 4; break;
					default: ReturnValue |= 1 << 4; break;
				}

				TCHAR tbuffer[1000]{};
				wcscpy_s(tbuffer, buffer);
				int k = lstrlen(tbuffer);
				int c = 0;
				for (int j = 13; j < k; j++)
				{
					buffer[c] = tbuffer[j];
					c++;
				}
				buffer[c] = 0x00;
				wcscpy_s((TCHAR*)lParam, bufferLen, buffer);
			}
			else
			{
				wcscpy_s((TCHAR*)lParam, bufferLen, TEXT(""));
			}
		}
		break;

		case BGM_CLEARGRID:
			SendMessage(BGHS[SelfIndex].hlist1, LB_RESETCONTENT, 0, 0);
			BGHS[SelfIndex].rows = 0;
			BGHS[SelfIndex].cursorrow = 1;
			BGHS[SelfIndex].homerow = 1;
			BGHS[SelfIndex].homecol = 1;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, TRUE);
			}
			break;

		case BGM_DELETECELL:
		{
			BGCELL* LPBGcell = (BGCELL*)wParam;
			if (OutOfRange(LPBGcell))
			{
				wParam = MAKEWPARAM(GetMenu(hWnd), BGN_OUTOFRANGE);
				lParam = 0;
				SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);
				ReturnValue = -1;
				break;
			}
			wsprintf(buffer, TEXT("%05d-%03d"), LPBGcell->row, LPBGcell->col);
			//see if that cell is already loaded
			int FindResult = BinarySearchListBox(BGHS[SelfIndex].hlist1, buffer);
			if (FindResult != LB_ERR)
			{
				//it was found, delete it
				SendMessage(BGHS[SelfIndex].hlist1, LB_DELETESTRING, FindResult, 0);
				NotifyEndEdit(hWnd, SelfIndex);
			}
		}
		break;

		case BGM_SETGRIDDIM:
			if (wParam <= MAX_ROWS)
			{
				BGHS[SelfIndex].rows = static_cast<int32_t>(wParam);
			}
			else
			{
				BGHS[SelfIndex].rows = MAX_ROWS;
			}

			if ((lParam > 0) && (lParam <= MAX_COLS))
			{
				BGHS[SelfIndex].cols = static_cast<int32_t>(lParam);
			}
			else
			{
				if (lParam <= 0)
				{
					BGHS[SelfIndex].cols = 1;
				}
				else
				{
					BGHS[SelfIndex].cols = MAX_COLS;
				}
			}
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, TRUE);
			}
			GetVisibleColumns(hWnd, SelfIndex);
			break;

		case BGM_SETCOLWIDTH:
			if ((wParam <= MAX_COLS) && (lParam >= 0))
			{
				RECT rect{};
				BGHS[SelfIndex].columnwidths[wParam] = static_cast<int32_t>(lParam);
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
				GetVisibleColumns(hWnd, SelfIndex);
			}
			break;

		case BGM_SETHEADERROWHEIGHT:
		{
			RECT rect{};
			BGHS[SelfIndex].headerrowheight = static_cast<int32_t>(wParam);
			SizeGrid(hWnd);
			GetClientRect(hWnd, &rect);
			InvalidateRect(hWnd, &rect, FALSE);
		}
		break;

		case BGM_GETHOMEROW:
			ReturnValue = BGHS[SelfIndex].homerow;
			break;

		case BGM_GETROW:
			ReturnValue = BGHS[SelfIndex].cursorrow;
			break;
		case BGM_GETCOL:
			ReturnValue = BGHS[SelfIndex].cursorcol;
			break;

		case BGM_GETTYPE:
		{
			BGCELL* LPBGcell = (BGCELL*)wParam;
			if (OutOfRange(LPBGcell))
			{
				wParam = MAKEWPARAM(GetMenu(hWnd), BGN_OUTOFRANGE);
				lParam = 0;
				SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);
				ReturnValue = -1;
				break;
			}
			wsprintf(buffer, TEXT("%05d-%03d"), LPBGcell->row, LPBGcell->col);
			//see if that cell is already loaded
			int FindResult = BinarySearchListBox(BGHS[SelfIndex].hlist1, buffer);
			if (FindResult != LB_ERR)
			{
				//it was found, get it
				auto lbTextLen = ::SendMessage(BGHS[SelfIndex].hlist1, LB_GETTEXTLEN, FindResult, 0);
				if (static_cast<size_t>(lbTextLen) > bufferLen)
					return TRUE;
				SendMessage(BGHS[SelfIndex].hlist1, LB_GETTEXT, FindResult, reinterpret_cast<LPARAM>(buffer));
				switch (buffer[11])
				{
					case 'A':ReturnValue = 1; break;
					case 'N':ReturnValue = 2; break;
					case 'T':ReturnValue = 3; break;
					case 'F':ReturnValue = 4; break;
					case 'G':ReturnValue = 5; break;
					default: ReturnValue = 1; break;
				}
			}
		}
		break;

		case BGM_GETPROTECTION:
		{
			BGCELL* LPBGcell = (BGCELL*)wParam;
			if (OutOfRange(LPBGcell))
			{
				wParam = MAKEWPARAM(GetMenu(hWnd), BGN_OUTOFRANGE);
				lParam = 0;
				SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);
				ReturnValue = -1;
				break;
			}
			wsprintf(buffer, TEXT("%05d-%03d"), LPBGcell->row, LPBGcell->col);
			//see if that cell is already loaded
			ReturnValue = 0;
			int FindResult = BinarySearchListBox(BGHS[SelfIndex].hlist1, buffer);
			if (FindResult != LB_ERR)
			{
				//it was found, get it
				auto lbTextLen = ::SendMessage(BGHS[SelfIndex].hlist1, LB_GETTEXTLEN, FindResult, 0);
				if (static_cast<size_t>(lbTextLen) > bufferLen)
					return TRUE;

				SendMessage(BGHS[SelfIndex].hlist1, LB_GETTEXT, FindResult, reinterpret_cast<LPARAM>(buffer));
				switch (buffer[10])
				{
					case 'U':ReturnValue = 0; break;
					case 'P':ReturnValue = 1; break;
					default: ReturnValue = 0; break;
				}
			}
		}
		break;

		case BGM_SETROWHEIGHT:
			if (wParam < 1) { wParam = 1; }
			BGHS[SelfIndex].rowheight = static_cast<int32_t>(wParam);
			SetHomeRow(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
			SetHomeCol(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
			SizeGrid(hWnd);

			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}
			break;

		case BGM_SETTITLEHEIGHT:
			BGHS[SelfIndex].titleheight = static_cast<int32_t>(wParam);
			SetHomeRow(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
			SetHomeCol(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}
			break;
		case BGM_SETGRIDLINECOLOR:
			DrawCursor(hWnd, SelfIndex);
			BGHS[SelfIndex].gridlinecolor = (COLORREF)wParam;
			DrawCursor(hWnd, SelfIndex);
			RefreshGrid(hWnd);
			break;

		case BGM_SETCURSORCOLOR:
			DrawCursor(hWnd, SelfIndex);
			BGHS[SelfIndex].cursorcolor = (COLORREF)wParam;
			DrawCursor(hWnd, SelfIndex);
			RefreshGrid(hWnd);
			break;

		case BGM_SETHILIGHTTEXTCOLOR:
			BGHS[SelfIndex].highlighttextcolor = (COLORREF)wParam;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}
			break;

		case BGM_SETHILIGHTCOLOR:
			BGHS[SelfIndex].highlightcolor = (COLORREF)wParam;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}
			break;

		case BGM_SETHILIGHTCOLOR_NOFOCUS:
			BGHS[SelfIndex].highlightcolorNoFocus = (COLORREF)wParam;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}
			break;

		case BGM_SETHILIGHTCOLOR_PROTECT:
			BGHS[SelfIndex].highlightcolorProtect = (COLORREF)wParam;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}
			break;

		case BGM_SETHILIGHTCOLOR_PROTECT_NOFOCUS:
			BGHS[SelfIndex].highlightcolorProtectNoFocus = (COLORREF)wParam;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}
			break;

		case BGM_SETPROTECTCOLOR:
			BGHS[SelfIndex].protectcolor = (COLORREF)wParam;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}
			break;
		case BGM_SETUNPROTECTCOLOR:
			BGHS[SelfIndex].unprotectcolor = (COLORREF)wParam;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}
			break;
		case BGM_SETTEXTCOLOR:
			BGHS[SelfIndex].textcolor = (COLORREF)wParam;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}
			break;
		case BGM_SETBACKGROUNDCOLOR:
			BGHS[SelfIndex].backgroundcolor = (COLORREF)wParam;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}
			break;
		case BGM_SETTITLETEXTCOLOR:
			BGHS[SelfIndex].titletextcolor = (COLORREF)wParam;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}
			break;
		case BGM_SETTITLECOLOR:
			BGHS[SelfIndex].titlecolor = (COLORREF)wParam;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}
			break;
		case BGM_SETTITLEGRIDLINECOLOR:
			BGHS[SelfIndex].titlegridlinecolor = (COLORREF)wParam;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}
			break;

		case BGM_SETELLIPSIS:
			BGHS[SelfIndex].ELLIPSIS = (BOOL)wParam;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}

			break;


		case BGM_SETTITLEFONT:
			BGHS[SelfIndex].htitlefont = (HFONT)wParam;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}

			break;

		case BGM_SETHEADINGFONT:
			BGHS[SelfIndex].hcolumnheadingfont = (HFONT)wParam;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}

			break;




		case BGM_SETROWSNUMBERED:
			BGHS[SelfIndex].ROWSNUMBERED = (BOOL)wParam;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}

			break;

		case BGM_SETCOLSNUMBERED:
			BGHS[SelfIndex].COLUMNSNUMBERED = (BOOL)wParam;
			{
				RECT rect{};
				GetClientRect(hWnd, &rect);
				InvalidateRect(hWnd, &rect, FALSE);
			}

			break;

		case WM_ENABLE:
			if (wParam == FALSE)
			{
				BGHS[SelfIndex].textcolor = RGB(120, 120, 120);
			}
			else
			{
				BGHS[SelfIndex].textcolor = RGB(0, 0, 0);
			}
			break;

		case WM_MOUSEMOVE:
		{
			int x = LOWORD(lParam);
			int y = HIWORD(lParam);
			int r = GetRowOfMouse(SelfIndex, y);
			int c = GetColOfMouse(SelfIndex, x);
			int t = GetColOfMouse(SelfIndex, x + 10);
			int z = GetColOfMouse(SelfIndex, x - 10);

			if (BGHS[SelfIndex].COLUMNSIZING)
			{
				int dx = x - BGHS[SelfIndex].columntoresizeinitx;
				int nx = BGHS[SelfIndex].columntoresizeinitsize + dx;
				if (nx <= 0) { nx = 0; }
				int cr = BGHS[SelfIndex].columntoresize;
				SendMessage(hWnd, BGM_SETCOLWIDTH, cr, nx);

			}
			if ((r == 0) && (c >= -1) && ((t != c) || (z != c)) && (!BGHS[SelfIndex].COLUMNSIZING))
			{
				if ((BGHS[SelfIndex].cursortype != 2) && (BGHS[SelfIndex].ALLOWCOLUMNRESIZING))
				{
					BGHS[SelfIndex].cursortype = 2;
					SetCursor(LoadCursor(NULL, IDC_SIZEWE));
				}

			}
			else
			{
				if ((BGHS[SelfIndex].cursortype != 1) && (!BGHS[SelfIndex].COLUMNSIZING))
				{
					BGHS[SelfIndex].cursortype = 1;
					SetCursor(LoadCursor(NULL, IDC_ARROW));
				}
			}
		}
		break;

		case WM_LBUTTONUP:
			if (BGHS[SelfIndex].COLUMNSIZING)
			{
				BGHS[SelfIndex].COLUMNSIZING = FALSE;
				SetCursor(LoadCursor(NULL, IDC_ARROW));
				BGHS[SelfIndex].cursortype = 1;
				BGHS[SelfIndex].SHOWINTEGRALROWS = BGHS[SelfIndex].REMEMBERINTEGRALROWS;
				SizeGrid(hWnd);
			}
			break;

		case WM_RBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		{
			//check for column sizing
			if (BGHS[SelfIndex].cursortype == 2)
			{
				//start column sizing
				if (!BGHS[SelfIndex].COLUMNSIZING)
				{
					BGHS[SelfIndex].REMEMBERINTEGRALROWS = BGHS[SelfIndex].SHOWINTEGRALROWS;
				}
				BGHS[SelfIndex].COLUMNSIZING = TRUE;
				BGHS[SelfIndex].SHOWINTEGRALROWS = FALSE;
				int x = LOWORD(lParam);
				BGHS[SelfIndex].columntoresizeinitx = x;
				int t = GetColOfMouse(SelfIndex, x + 10);
				int z = GetColOfMouse(SelfIndex, x - 10);
				int c = GetColOfMouse(SelfIndex, x);
				if (t != c)
				{
					//resizing column c
					BGHS[SelfIndex].columntoresize = c;
				}
				if (z != c)
				{
					//resizing hidden column to the left of cursor
					if (c == -1)
					{
						c = static_cast<int32_t>(SendMessage(hWnd, BGM_GETCOLS, 0, 0));
					}
					else
					{
						c -= 1;
					}
					BGHS[SelfIndex].columntoresize = c;
				}

				BGHS[SelfIndex].columntoresizeinitsize = BGHS[SelfIndex].columnwidths[c];
			}

			if (BGHS[SelfIndex].EDITING)
			{
				CloseEdit(hWnd, SelfIndex);
			}
			else
			{

				SetFocus(hWnd);
			}
			
			BOOL NRC = FALSE;
			BOOL NCC = FALSE;

			if (GetFocus() == hWnd)
			{

				int x = LOWORD(lParam);
				int y = HIWORD(lParam);
				int r = GetRowOfMouse(SelfIndex, y);
				int c = GetColOfMouse(SelfIndex, x);
				DrawCursor(hWnd, SelfIndex);
				if ((r > 0) && (c > 0))
				{
					if (r != BGHS[SelfIndex].cursorrow)
					{
						BGHS[SelfIndex].cursorrow = r;
						NRC = TRUE;
					}
					else
					{
						BGHS[SelfIndex].cursorrow = r;
					}
					if (c != BGHS[SelfIndex].cursorcol)
					{
						BGHS[SelfIndex].cursorcol = c;
						NCC = TRUE;
					}
					else
					{
						BGHS[SelfIndex].cursorcol = c;
					}

				}
				if (NRC) { NotifyRowChanged(hWnd, SelfIndex); }
				if (NCC) { NotifyColChanged(hWnd, SelfIndex); }

				DrawCursor(hWnd, SelfIndex);
				SetHomeRow(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				SetHomeCol(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				RefreshGrid(hWnd);

				if (message == WM_LBUTTONDOWN)
					NotifyCellClicked(hWnd, SelfIndex);
				else if (message == WM_LBUTTONDBLCLK)
					NotifyCellDbClicked(hWnd, SelfIndex);
				else // (message == WM_RBUTTONUP)
					NotifyCellRClicked(hWnd, SelfIndex);
			}
			else
			{
				SetFocus(hWnd);
			}
		}
		break;

		case WM_ERASEBKGND:
			return TRUE;
			break;

		case WM_GETDLGCODE:

			ReturnValue = DLGC_WANTARROWS | DLGC_WANTCHARS | DLGC_DEFPUSHBUTTON;
			if (wParam == 13)
			{
				NotifyCellDbClicked(hWnd, SelfIndex);
				if (BGHS[SelfIndex].EDITING)
				{
					CloseEdit(hWnd, SelfIndex);
				}
				DrawCursor(hWnd, SelfIndex);

				SetHomeRow(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				RefreshGrid(hWnd);
				BGHS[SelfIndex].EDITING = FALSE;
				break;
			}

			if (wParam == VK_ESCAPE)
			{
				if (BGHS[SelfIndex].EDITING)
				{
					BGHS[SelfIndex].EDITING = FALSE;
					wcscpy_s(BGHS[SelfIndex].editstring, TEXT(""));
					HideCaret(hWnd);
					RefreshGrid(hWnd);
					NotifyEditEnd(hWnd, SelfIndex);
				}
				else
				{
					ReturnValue = 0;
				}
				break;
			}
			break;

		case WM_KEYDOWN:

			if (wParam == VK_ESCAPE)
			{
				if (BGHS[SelfIndex].EDITING)
				{
					BGHS[SelfIndex].EDITING = FALSE;
					wcscpy_s(BGHS[SelfIndex].editstring, TEXT(""));
					HideCaret(hWnd);
					RefreshGrid(hWnd);
					NotifyEditEnd(hWnd, SelfIndex);
				}
				break;
			}
			
			if (wParam == VK_DELETE)
			{
				NotifyDelete(hWnd, SelfIndex);
				break;
			}

			if (wParam == VK_TAB)
			{
				SetFocus(GetParent(hWnd));
				break;
			}

			if (wParam == VK_NEXT)
			{
				RECT gridrect{};
				if (BGHS[SelfIndex].EDITING)
				{
					CloseEdit(hWnd, SelfIndex);
				}

				if (BGHS[SelfIndex].rows == 0) { break; }
				if (BGHS[SelfIndex].cursorrow == BGHS[SelfIndex].rows) { break; }
				//get rows per page
				GetClientRect(hWnd, &gridrect);
				int rpp = (gridrect.bottom - (BGHS[SelfIndex].headerrowheight + BGHS[SelfIndex].titleheight)) / BGHS[SelfIndex].rowheight;
				DrawCursor(hWnd, SelfIndex);
				BGHS[SelfIndex].cursorrow += rpp;

				if (BGHS[SelfIndex].cursorrow > BGHS[SelfIndex].rows)
				{
					BGHS[SelfIndex].cursorrow = BGHS[SelfIndex].rows;
				}
				NotifyRowChanged(hWnd, SelfIndex);
				DrawCursor(hWnd, SelfIndex);
				SetHomeRow(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				SetHomeCol(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				RefreshGrid(hWnd);
				break;

			}

			if (wParam == VK_PRIOR)
			{
				RECT gridrect{};
				if (BGHS[SelfIndex].EDITING)
				{
					CloseEdit(hWnd, SelfIndex);
				}

				if (BGHS[SelfIndex].rows == 0) { break; }
				if (BGHS[SelfIndex].cursorrow == 1) { break; }
				//get rows per page
				GetClientRect(hWnd, &gridrect);
				int rpp = (gridrect.bottom - (BGHS[SelfIndex].headerrowheight + BGHS[SelfIndex].titleheight)) / BGHS[SelfIndex].rowheight;
				DrawCursor(hWnd, SelfIndex);
				BGHS[SelfIndex].cursorrow -= rpp;
				if (BGHS[SelfIndex].cursorrow < 1)
				{
					BGHS[SelfIndex].cursorrow = 1;
				}
				NotifyRowChanged(hWnd, SelfIndex);
				DrawCursor(hWnd, SelfIndex);
				SetHomeRow(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				SetHomeCol(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				RefreshGrid(hWnd);
				break;
			}

			if (wParam == VK_DOWN)
			{
				if (BGHS[SelfIndex].EDITING)
				{
					CloseEdit(hWnd, SelfIndex);
				}

				if (BGHS[SelfIndex].rows == 0) { break; }
				if (BGHS[SelfIndex].cursorrow == BGHS[SelfIndex].rows) { break; }
				DrawCursor(hWnd, SelfIndex);
				BGHS[SelfIndex].cursorrow++;
				if (BGHS[SelfIndex].cursorrow > BGHS[SelfIndex].rows)
				{
					BGHS[SelfIndex].cursorrow = BGHS[SelfIndex].rows;
				}
				else
				{
					NotifyRowChanged(hWnd, SelfIndex);
				}
				DrawCursor(hWnd, SelfIndex);
				SetHomeRow(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				SetHomeCol(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				RefreshGrid(hWnd);
				break;
			}

			if (wParam == VK_UP)
			{
				if (BGHS[SelfIndex].EDITING)
				{
					CloseEdit(hWnd, SelfIndex);
				}

				if (BGHS[SelfIndex].rows == 0) { break; }
				if (BGHS[SelfIndex].cursorrow == 1) { break; }

				DrawCursor(hWnd, SelfIndex);
				BGHS[SelfIndex].cursorrow--;
				if (BGHS[SelfIndex].cursorrow < 1)
				{
					BGHS[SelfIndex].cursorrow = 1;
				}
				else
				{
					NotifyRowChanged(hWnd, SelfIndex);
				}
				DrawCursor(hWnd, SelfIndex);
				SetHomeRow(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				SetHomeCol(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				RefreshGrid(hWnd);
				break;
			}

			if (wParam == VK_LEFT)
			{
				if (BGHS[SelfIndex].EDITING)
				{
					CloseEdit(hWnd, SelfIndex);
				}

				if (!GetNextColWithWidth(SelfIndex, BGHS[SelfIndex].cursorcol, -1))
				{
					break;
				}
				DrawCursor(hWnd, SelfIndex);
				int k = GetNextColWithWidth(SelfIndex, BGHS[SelfIndex].cursorcol, -1);
				if (k)
				{
					BGHS[SelfIndex].cursorcol = k;
					NotifyColChanged(hWnd, SelfIndex);
				}
				DrawCursor(hWnd, SelfIndex);
				SetHomeRow(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				SetHomeCol(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				break;
			}

			if (wParam == VK_RIGHT)
			{
				if (BGHS[SelfIndex].EDITING)
				{
					CloseEdit(hWnd, SelfIndex);
				}
				DrawCursor(hWnd, SelfIndex);
				int k = GetNextColWithWidth(SelfIndex, BGHS[SelfIndex].cursorcol, 1);
				if (k)
				{
					BGHS[SelfIndex].cursorcol = k;
					NotifyColChanged(hWnd, SelfIndex);
				}
				DrawCursor(hWnd, SelfIndex);
				SetHomeRow(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				SetHomeCol(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				RefreshGrid(hWnd);
				break;
			}

			SetCurrentCellStatus(hWnd, SelfIndex);



			if ((BGHS[SelfIndex].CURRENTCELLPROTECTED) && (wParam == 13))
			{
				DrawCursor(hWnd, SelfIndex);
				BGHS[SelfIndex].cursorrow++;
				if (BGHS[SelfIndex].cursorrow > BGHS[SelfIndex].rows)
				{
					BGHS[SelfIndex].cursorrow = BGHS[SelfIndex].rows;
				}
				else
				{
					NotifyRowChanged(hWnd, SelfIndex);
				}
				DrawCursor(hWnd, SelfIndex);
				SetHomeRow(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				RefreshGrid(hWnd);
				break;
			}

			if (BGHS[SelfIndex].CURRENTCELLPROTECTED) { break; }

			if (!BGHS[SelfIndex].EDITABLE)
			{
				int ascii = GetASCII(wParam, lParam);
				if (ascii == 13) //enter pressed, treat as arrow down
				{
					//same as arrow down
					DrawCursor(hWnd, SelfIndex);
					BGHS[SelfIndex].cursorrow++;
					if (BGHS[SelfIndex].cursorrow > BGHS[SelfIndex].rows)
					{
						BGHS[SelfIndex].cursorrow = BGHS[SelfIndex].rows;
					}
					else
					{
						NotifyRowChanged(hWnd, SelfIndex);
					}
					DrawCursor(hWnd, SelfIndex);
					SetHomeRow(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
					RefreshGrid(hWnd);
					break;

				}

			}

			//if it's not an arrow key, make an edit box in the active cell rectangle
			if ((BGHS[SelfIndex].EDITABLE) && (BGHS[SelfIndex].rows > 0))
			{

				SetHomeRow(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
				DrawCursor(hWnd, SelfIndex);
				DrawCursor(hWnd, SelfIndex);

				{
					int ascii = GetASCII(wParam, lParam);
					wParam = ascii;
					if ((wParam >= 32) && (wParam <= 125))
					{
						TCHAR tstring[2];
						if (!BGHS[SelfIndex].EDITING)
						{
							NotifyEditBegin(hWnd, SelfIndex);
						}
						BGHS[SelfIndex].EDITING = TRUE;
						tstring[0] = (TCHAR)wParam;
						tstring[1] = 0x00;
						DisplayEditString(hWnd, SelfIndex, tstring);
						break;
					}
					if (wParam == 8) //backspace
					{
						if (!BGHS[SelfIndex].EDITING)
						{
							NotifyEditBegin(hWnd, SelfIndex);
						}

						BGHS[SelfIndex].EDITING = TRUE;
						if (lstrlen(BGHS[SelfIndex].editstring) == 0)
						{
							DisplayEditString(hWnd, SelfIndex, TEXT(""));
							break;
						}
						else
						{
							int j;
							j = lstrlen(BGHS[SelfIndex].editstring);
							BGHS[SelfIndex].editstring[j - 1] = 0x00;
							DisplayEditString(hWnd, SelfIndex, TEXT(""));
						}
						break;
					}
					if (wParam == 13)
					{
						//same as arrow down
						if (BGHS[SelfIndex].EDITING)
						{
							CloseEdit(hWnd, SelfIndex);
						}
						DrawCursor(hWnd, SelfIndex);
						BGHS[SelfIndex].cursorrow++;
						if (BGHS[SelfIndex].cursorrow > BGHS[SelfIndex].rows)
						{
							BGHS[SelfIndex].cursorrow = BGHS[SelfIndex].rows;
						}
						else
						{
							NotifyRowChanged(hWnd, SelfIndex);
						}
						DrawCursor(hWnd, SelfIndex);
						SetHomeRow(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
						RefreshGrid(hWnd);
						BGHS[SelfIndex].EDITING = FALSE;
						break;
					}
				}
			}
			break;
		case WM_HSCROLL:
			SetFocus(hWnd);
			if ((LOWORD(wParam == SB_LINERIGHT)) || (LOWORD(wParam) == SB_PAGERIGHT))
			{
				int cp = GetScrollPos(hWnd, SB_HORZ);
				SetScrollPos(hWnd, SB_HORZ, cp + 1, TRUE);
				cp = GetScrollPos(hWnd, SB_HORZ);
				int np = GetNthVisibleColumn(hWnd, SelfIndex, cp);
				BGHS[SelfIndex].homecol = np;
				SetScrollPos(hWnd, SB_HORZ, cp, TRUE);
				RefreshGrid(hWnd);
			}
			if ((LOWORD(wParam == SB_LINELEFT)) || (LOWORD(wParam) == SB_PAGELEFT))
			{
				int cp = GetScrollPos(hWnd, SB_HORZ);
				SetScrollPos(hWnd, SB_HORZ, cp - 1, TRUE);
				cp = GetScrollPos(hWnd, SB_HORZ);
				int np = GetNthVisibleColumn(hWnd, SelfIndex, cp);
				BGHS[SelfIndex].homecol = np;
				SetScrollPos(hWnd, SB_HORZ, cp, TRUE);
				RefreshGrid(hWnd);
			}
			if (LOWORD(wParam) == SB_THUMBTRACK)
			{
				int cp = HIWORD(wParam);
				int np = GetNthVisibleColumn(hWnd, SelfIndex, cp);
				SetScrollPos(hWnd, SB_HORZ, np, TRUE);
				BGHS[SelfIndex].homecol = np;
				SetScrollPos(hWnd, SB_HORZ, cp, TRUE);
				RefreshGrid(hWnd);
			}


			break;

		case WM_MOUSEWHEEL:
		{
			if (!BGHS[SelfIndex].VSCROLL)
				return TRUE;
			short zDelta = (short)HIWORD(wParam);
			::SendMessage(hWnd, WM_VSCROLL, zDelta < 0 ? SB_LINEDOWN : SB_LINEUP, 0);
			return TRUE;
		}

		case WM_VSCROLL:
			SetFocus(hWnd);
			if (LOWORD(wParam) == SB_THUMBTRACK)
			{
				RECT gridrect{};
				int min = 0;
				int max = 0;
				BGHS[SelfIndex].homerow = HIWORD(wParam);
				SetScrollPos(hWnd, SB_VERT, HIWORD(wParam), TRUE);
				GetClientRect(hWnd, &gridrect);
				GetScrollRange(hWnd, SB_VERT, &min, &max);
				if (HIWORD(wParam) == max)
				{
					gridrect.top = gridrect.bottom - (BGHS[SelfIndex].rowheight);
					InvalidateRect(hWnd, &gridrect, TRUE);
				}
				else
				{
					InvalidateRect(hWnd, &gridrect, FALSE);
				}
			}

			if (LOWORD(wParam) == SB_PAGEDOWN)
			{
				RECT gridrect{};
				int min = 0;
				int max = 0;
				//get rows per page
				GetClientRect(hWnd, &gridrect);
				int rpp = (gridrect.bottom - (BGHS[SelfIndex].headerrowheight + BGHS[SelfIndex].titleheight)) / BGHS[SelfIndex].rowheight;
				GetScrollRange(hWnd, SB_VERT, &min, &max);
				int sp = GetScrollPos(hWnd, SB_VERT);
				sp += rpp;
				if (sp > max) { sp = max; }
				BGHS[SelfIndex].homerow = sp;
				SetScrollPos(hWnd, SB_VERT, sp, TRUE);
				SetHomeRow(hWnd, SelfIndex, sp, BGHS[SelfIndex].homecol);
				if (sp == max)
				{
					gridrect.top = gridrect.bottom - (BGHS[SelfIndex].rowheight);
					InvalidateRect(hWnd, &gridrect, TRUE);
				}
				else
				{
					InvalidateRect(hWnd, &gridrect, FALSE);
				}

			}

			if (LOWORD(wParam) == SB_LINEDOWN)
			{
				RECT gridrect{};
				int min = 0;
				int max = 0;
				//get rows per page
				GetClientRect(hWnd, &gridrect);
				GetScrollRange(hWnd, SB_VERT, &min, &max);
				int sp = GetScrollPos(hWnd, SB_VERT);
				sp += 1;
				if (sp > max) { sp = max; }
				BGHS[SelfIndex].homerow = sp;
				SetScrollPos(hWnd, SB_VERT, sp, TRUE);
				SetHomeRow(hWnd, SelfIndex, sp, BGHS[SelfIndex].homecol);
				if (sp == max)
				{
					gridrect.top = gridrect.bottom - (BGHS[SelfIndex].rowheight);
					InvalidateRect(hWnd, &gridrect, TRUE);
				}
				else
				{
					InvalidateRect(hWnd, &gridrect, FALSE);
				}

			}

			if (LOWORD(wParam) == SB_PAGEUP)
			{
				RECT gridrect{};
				int min = 0;
				int max = 0;
				//get rows per page
				GetClientRect(hWnd, &gridrect);
				int rpp = (gridrect.bottom - (BGHS[SelfIndex].headerrowheight + BGHS[SelfIndex].titleheight)) / BGHS[SelfIndex].rowheight;
				GetScrollRange(hWnd, SB_VERT, &min, &max);
				int sp = GetScrollPos(hWnd, SB_VERT);
				sp -= rpp;
				if (sp < 1) { sp = 1; }
				BGHS[SelfIndex].homerow = sp;
				SetScrollPos(hWnd, SB_VERT, sp, TRUE);
				SetHomeRow(hWnd, SelfIndex, sp, BGHS[SelfIndex].homecol);
				if (sp == max)
				{
					gridrect.top = gridrect.bottom - (BGHS[SelfIndex].rowheight);
					InvalidateRect(hWnd, &gridrect, TRUE);
				}
				else
				{
					InvalidateRect(hWnd, &gridrect, FALSE);
				}

			}
			if (LOWORD(wParam) == SB_LINEUP)
			{
				RECT gridrect{};
				int min = 0;
				int max = 0;
				//get rows per page
				GetClientRect(hWnd, &gridrect);
				int sp = GetScrollPos(hWnd, SB_VERT);
				GetScrollRange(hWnd, SB_VERT, &min, &max);
				sp -= 1;
				if (sp < 1) { sp = 1; }
				BGHS[SelfIndex].homerow = sp;
				SetScrollPos(hWnd, SB_VERT, sp, TRUE);
				SetHomeRow(hWnd, SelfIndex, sp, BGHS[SelfIndex].homecol);
				if (sp == max)
				{
					gridrect.top = gridrect.bottom - (BGHS[SelfIndex].rowheight);
					InvalidateRect(hWnd, &gridrect, TRUE);
				}
				else
				{
					InvalidateRect(hWnd, &gridrect, FALSE);
				}

			}
			RefreshGrid(hWnd);

			break;
		case WM_DESTROY:
		{
			if (CountGrids() == 0)
			{
				DeleteObject(g_hfontbody);
				DeleteObject(g_hfontheader);
				DeleteObject(g_hfonttitle);
			}
			SendMessage(BGHS[SelfIndex].hlist1, LB_RESETCONTENT, 0, 0);
			DestroyWindow(BGHS[SelfIndex].hlist1);
			BGHS[SelfIndex].gridmenu = 0;
			BGHS[SelfIndex].hlist1 = NULL;
			BGHS[SelfIndex].hfont = NULL;
			wcscpy_s(BGHS[SelfIndex].protect, TEXT("U"));
			BGHS[SelfIndex].rows = 100;
			BGHS[SelfIndex].cols = 255;
			BGHS[SelfIndex].homerow = 1;
			BGHS[SelfIndex].homecol = 1;
			BGHS[SelfIndex].rowheight = 20;
			BGHS[SelfIndex].headerrowheight = 20;
			BGHS[SelfIndex].ROWSNUMBERED = TRUE;
			BGHS[SelfIndex].COLUMNSNUMBERED = TRUE;
			BGHS[SelfIndex].DRAWHIGHLIGHT = TRUE;

			BGHS[SelfIndex].cursorcol = 1;
			BGHS[SelfIndex].cursorrow = 1;
			BGHS[SelfIndex].columnwidths[0] = 40;
			BGHS[SelfIndex].ADVANCEROW = TRUE;
			BGHS[SelfIndex].cursorcolor = RGB(255, 255, 255);
			BGHS[SelfIndex].protectcolor = RGB(128, 128, 128);
			BGHS[SelfIndex].unprotectcolor = RGB(255, 255, 255);
			for (int k = 1; k < MAX_COLS; k++)
			{
				BGHS[SelfIndex].columnwidths[k] = 50;
			}
		}
		break;
		case WM_SETFOCUS:
			DrawCursor(hWnd, SelfIndex);
			BGHS[SelfIndex].GRIDHASFOCUS = TRUE;
			DrawCursor(hWnd, SelfIndex);
			SetHomeRow(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);
			SetHomeCol(hWnd, SelfIndex, BGHS[SelfIndex].cursorrow, BGHS[SelfIndex].cursorcol);

			wParam = MAKEWPARAM(GetMenu(hWnd), BGN_GOTFOCUS);
			lParam = 0;
			SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);
			{
				TEXTMETRIC tm{};
				HDC hdc = GetDC(hWnd);
				GetTextMetrics(hdc, &tm);
				ReleaseDC(hWnd, hdc);
				BGHS[SelfIndex].fontascentheight = (int)tm.tmAscent;
				CreateCaret(hWnd, NULL, 3, tm.tmAscent);
			}
			RefreshGrid(hWnd);
			break;
		case WM_KILLFOCUS:
			DestroyCaret();
			DrawCursor(hWnd, SelfIndex);
			BGHS[SelfIndex].GRIDHASFOCUS = FALSE;

			wParam = MAKEWPARAM(GetMenu(hWnd), BGN_LOSTFOCUS);
			lParam = 0;
			SendMessage(GetParent(hWnd), WM_COMMAND, wParam, lParam);
			RefreshGrid(hWnd);


			break;
		case WM_SETFONT:
			BGHS[SelfIndex].hfont = (HFONT)wParam;
			if (!BGHS[SelfIndex].hcolumnheadingfont)
			{
				BGHS[SelfIndex].hcolumnheadingfont = (HFONT)wParam;
			}
			if (!BGHS[SelfIndex].htitlefont)
			{
				BGHS[SelfIndex].htitlefont = (HFONT)wParam;
			}
			RefreshGrid(hWnd);
			break;

		case WM_SIZE:
		{
			//This function needs a static placement position inside a parent window (default in Npp).
			//For a dynamic position (e.g. sizing of the parenet window) an adjustment to this function is needed!

			if (!BGHS[SelfIndex].SHOWINTEGRALROWS)
				break;

			ShowHscroll(hWnd, SelfIndex);
			ShowVscroll(hWnd, SelfIndex);

			if (BGHS[SelfIndex].VSCROLL)
			{
				static int masterHeight = 0; //initial height

				WINDOWPLACEMENT wp;
				wp.length = sizeof(wp);

				::GetWindowPlacement(hWnd, &wp);

				if (masterHeight < 1)
				{
					masterHeight = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
					if (masterHeight < 1)
						break;
				}

				int outerHeight = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;

				int innerHeight = outerHeight;
				innerHeight -= BGHS[SelfIndex].titleheight;
				innerHeight -= BGHS[SelfIndex].headerrowheight;
				if (::GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_CLIENTEDGE)
					innerHeight -= ::GetSystemMetrics(SM_CYEDGE) * 2;
				if (BGHS[SelfIndex].HSCROLL)
					innerHeight -= ::GetSystemMetrics(SM_CYHSCROLL);

				if (innerHeight <= BGHS[SelfIndex].rowheight * 4)
					break;
				else
				{
					int remainder = innerHeight % BGHS[SelfIndex].rowheight;

					if ((outerHeight + BGHS[SelfIndex].rowheight - remainder) <= masterHeight)
						outerHeight += BGHS[SelfIndex].rowheight - remainder;
					else
						outerHeight -= remainder;

					wp.rcNormalPosition.bottom = wp.rcNormalPosition.top + outerHeight;

					::SetWindowPlacement(hWnd, &wp);
				}
			}
		}
		break;

		case WM_CREATE:
		{
			CREATESTRUCT* lpcs = (LPCREATESTRUCT)lParam;
			HINSTANCE hInst = lpcs->hInstance;
			int BG_GridIndex = AddGrid(GetMenu(hWnd));

			if (CountGrids() == 1)
			{
				g_hfontbody = CreateFont(16, 0, 0, 0,
					100,
					FALSE,
					FALSE, FALSE, DEFAULT_CHARSET,
					OUT_DEFAULT_PRECIS,
					CLIP_DEFAULT_PRECIS,
					0,
					0,
					TEXT("MS Shell Dlg"));
				g_hfontheader = CreateFont(18, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 0, 0, TEXT("MS Shell Dlg"));
				g_hfonttitle = CreateFont(20, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 0, 0, TEXT("MS Shell Dlg"));
			}


			if ((BG_GridIndex >= 0) && (BG_GridIndex < MAX_GRIDS))//if you aren't over the MAX_GRIDS limit, add a grid
			{

				BGHS[BG_GridIndex].gridmenu = GetMenu(hWnd);

				BGHS[BG_GridIndex].hlist1 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("LISTBOX"), TEXT(""),
					WS_CHILD | LBS_STANDARD, 50, 150, 200, 100, hWnd, NULL, hInst, NULL);

				BGHS[BG_GridIndex].hfont = g_hfontbody;
				BGHS[BG_GridIndex].htitlefont = g_hfonttitle;
				BGHS[BG_GridIndex].hcolumnheadingfont = g_hfontheader;
				wcscpy_s(BGHS[BG_GridIndex].title, lpcs->lpszName);
				SendMessage(hWnd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(lpcs->lpszName));


			}
			if (BG_GridIndex == -1)
			{
				DestroyWindow(hWnd);
			}
		}
		break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return ReturnValue;
}

int CountGrids()
{
	int count = 0;
	for (int j = 0; j < MAX_GRIDS; j++)
		if (BGHS[j].gridmenu != 0)
			count++;
	return count;
}


int AddGrid(HMENU menuid)
{
	//if grid doesn't exist, add it.  otherwise return existing index + MAX_GRIDS
	//if trying to add more than MAX_GRIDS, return -1;
	int empty_space = -1;
	int returnvalue = 0;
	BOOL MATCH = FALSE;
	for (int j = 0; j < MAX_GRIDS; j++)
	{
		if (BGHS[j].gridmenu == menuid)
		{
			MATCH = TRUE;
			returnvalue = j;
		}
		if (BGHS[j].gridmenu == 0)
		{
			empty_space = j;
		}
	}

	if ((!MATCH) && (empty_space >= 0))
	{
		BGHS[empty_space].gridmenu = menuid;
		returnvalue = empty_space;
	}
	if (MATCH)
	{
		return returnvalue + MAX_GRIDS;
	}
	if ((!MATCH) && (empty_space == -1))
	{
		return -1;
	}
	return returnvalue;

}

int FindGrid(HMENU menuid)
{
	//if grid doesn't exist, return -1, else return gridindex
	int returnvalue = -1;
	for (int j = 0; j < MAX_GRIDS; j++)
	{
		if (BGHS[j].gridmenu == menuid)
		{
			returnvalue = j;
		}
	}

	return returnvalue;
}


int BinarySearchListBox(HWND lbhWnd, TCHAR* searchtext)
{
	const size_t bufLen = 1000;
	TCHAR tbuffer[bufLen]{};
	TCHAR headtext[bufLen]{};
	TCHAR tailtext[bufLen]{};
	
	BOOL FOUND = FALSE;
	//get count of items in listbox
	int lbcount = static_cast<int32_t>(SendMessage(lbhWnd, LB_GETCOUNT, 0, 0));
	if (lbcount == 0)
	{
		return LB_ERR;
	}

	if (lbcount < 12)
	{
		//not worth doing binary search, do regular search
		return static_cast<int32_t>(SendMessage(lbhWnd, LB_FINDSTRING, static_cast<unsigned int>(-1), reinterpret_cast<LPARAM>(searchtext)));
	}

	// do a binary search
	int head = 0;
	int tail = lbcount - 1;

	//is it the head?
	auto lbTextLen = ::SendMessage(lbhWnd, LB_GETTEXTLEN, head, 0);
	if (static_cast<size_t>(lbTextLen) > bufLen)
		return 0;

	SendMessage(lbhWnd, LB_GETTEXT, head, reinterpret_cast<LPARAM>(headtext));
	headtext[9] = 0x00;

	int p = lstrcmp(searchtext, headtext);
	if (p == 0)
	{
		//it was the head
		return head;
	}
	if (p < 0)
	{
		//it was less than the head... not found
		return LB_ERR;
	}



	//is it the tail?
	lbTextLen = ::SendMessage(lbhWnd, LB_GETTEXTLEN, tail, 0);
	if (static_cast<size_t>(lbTextLen) > bufLen)
		return 0;
	SendMessage(lbhWnd, LB_GETTEXT, tail, reinterpret_cast<LPARAM>(tailtext));
	tailtext[9] = 0x00;
	p = lstrcmp(searchtext, tailtext);
	if (p == 0)
	{
		//it was the tail
		return tail;
	}
	if (p > 0)
	{
		//it was greater than the tail... not found
		return LB_ERR;
	}

	//is it the finger?
	int ReturnValue = LB_ERR;
	FOUND = FALSE;


	while ((!FOUND) && ((tail - head) > 1))
	{
		int finger = head + ((tail - head) / 2);
		lbTextLen = ::SendMessage(lbhWnd, LB_GETTEXTLEN, finger, 0);
		if (static_cast<size_t>(lbTextLen) > bufLen)
			return 0;
		SendMessage(lbhWnd, LB_GETTEXT, finger, reinterpret_cast<LPARAM>(tbuffer));
		tbuffer[9] = 0x00;
		p = lstrcmp(tbuffer, searchtext);
		if (p == 0)
		{
			FOUND = TRUE;
			ReturnValue = finger;
		}

		if (p < 0)
		{
			//change  tail to finger
			head = finger;
		}
		if (p > 0)
		{
			//change head to finger
			tail = finger;
		}


	}
	return ReturnValue;
}

