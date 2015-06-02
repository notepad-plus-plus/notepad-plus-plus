////////////////////////////////////////////////////////////////
// MSDN Magazine -- July 2001
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
// Compiles with Visual C++ 6.0. Runs on Win 98 and probably Win 2000 too.
// Set tabsize = 3 in your editor.
//
// Theo - Heavily modified to remove MFC dependencies.  
//        Replaced CWnd*/HWND, CRect/RECT, CSize/SIZE, CPoint/POINT


#include "WinMgr.h"

// Theo - Style Helpers
inline static DWORD GetStyle(HWND hWnd) { 
	return (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE); 
}

inline static DWORD GetExStyle(HWND hWnd) { 
	return (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE); 
}

const UINT WM_WINMGR = RegisterWindowMessage(TEXT("WM_WINMGR"));

CWinMgr::CWinMgr(WINRECT* pWinMap) : m_map(pWinMap)
{
	WINRECT::InitMap(m_map);
}

CWinMgr::~CWinMgr()
{
}

//////////////////
// Set each control's tofit (desired) size to current size. Useful for
// dialogs, to "remember" the current sizes as desired size.
//
void CWinMgr::InitToFitSizeFromCurrent(HWND hWnd)
{
	assert(hWnd);
	assert(m_map);
	GetWindowPositions(hWnd);
	for (WINRECT* w = m_map; !w->IsEnd(); ++w) {
		if (w->Type()==WRCT_TOFIT && !w->IsGroup()) {
			w->SetToFitSize(RectToSize(w->GetRect()));
		}
	}
}

//////////////////
// Load all rectangles from current window positions.
//
void CWinMgr::GetWindowPositions(HWND hWnd)
{
	assert(m_map);
	assert(hWnd);
	for (WINRECT* wrc=m_map; !wrc->IsEnd(); ++wrc) {
		if (wrc->IsWindow()) {
			HWND HChild = GetDlgItem(hWnd, wrc->GetID());
			if (HChild) {
				GetWindowRect(HChild, &wrc->GetRect());
				POINT p = RectToPoint(wrc->GetRect());
				::ScreenToClient(hWnd, &p);
			}
		}
	}
}

//////////////////
// Move all the windows. Use DeferWindowPos for speed.
//
void
CWinMgr::SetWindowPositions(HWND hWnd)
{
	int nWindows = CountWindows();
	if (m_map && hWnd && nWindows>0) {
		HDWP hdwp = ::BeginDeferWindowPos(nWindows);
		int count=0;
		for (WINRECT* wrc=m_map; !wrc->IsEnd(); ++wrc) {
			if (wrc->IsWindow()) {
				assert(count < nWindows);
				HWND hwndChild = ::GetDlgItem(hWnd, wrc->GetID());
				if (hwndChild) {
					const RECT& rc = wrc->GetRect();
					::DeferWindowPos(hdwp,
						hwndChild,
						NULL,		// HWND insert after
						rc.left,rc.top,RectWidth(rc),RectHeight(rc),
						SWP_NOZORDER);
					InvalidateRect(hwndChild,NULL,TRUE); // repaint
					++count;
				}
			} else {
				// not a window: still need to repaint background
				InvalidateRect(hWnd, &wrc->GetRect(), TRUE);
			}
		}
		::EndDeferWindowPos(hdwp);
	}
}

//////////////////
// Count number of table entries that correspond to windows--ie,
// that have a child window ID associated with the entry.
//
int CWinMgr::CountWindows()
{
	assert(m_map);
	int nWin = 0;
	for (WINRECT* w=m_map; !w->IsEnd(); ++w) {
		if (w->IsWindow())
			++nWin;
	}
	return nWin;
}

//////////////////
// Find the entry for a given control ID
//
WINRECT* CWinMgr::FindRect(int nID)
{
	assert(m_map);
	for (WINRECT* w=m_map; !w->IsEnd(); ++w) {
		if (w->GetID()==(UINT)nID)
			return w;
	}
	return NULL;
}

//////////////////
// Calculate size/positions for a row or column group This is the main
// algorithm. If a window is given, it's used to get the min/max size and
// desired size for TOFIT types.
//
void
CWinMgr::CalcGroup(WINRECT* pGroup, HWND hWnd)
{
	// If this bombs, most likely the first entry in your map is not a group!
	assert(pGroup && pGroup->IsGroup());
	assert(hWnd);

	// adjust total avail by margins
	RECT rcTotal = pGroup->GetRect();
	int w,h;
	if (pGroup->GetMargins(w,h)) {
		w = min(abs(w), RectWidth(rcTotal)/2);
		h = min(abs(h), RectHeight(rcTotal)/2);
		::InflateRect(&rcTotal, -w, -h);
	}
	
	BOOL bRow = pGroup->IsRowGroup();		 // Is this a row group?

	// Running height or width: start with total
	int hwRemaining = bRow ? RectHeight(rcTotal) : RectWidth(rcTotal);

	// First, set all rects to their minimum sizes.
	// This ensures that each rect gets its min size.
	CWinGroupIterator it;
	for (it=pGroup; it; it.Next()) {
		WINRECT* wrc = it;
		SIZEINFO szi;
		OnGetSizeInfo(szi, wrc, hWnd);
		int hwMin = bRow ? szi.szMin.cy : szi.szMin.cx;
		hwMin = min(hwMin, hwRemaining);		// truncate
		wrc->SetHeightOrWidth(hwMin, bRow);	// set
		hwRemaining -= hwMin;					// decrement remaining height/width
		assert(hwRemaining>=0);
	}

	// Now adjust all rects upward to desired size. Save REST rect for last.
	WINRECT* pRestRect = NULL;
	for (it=pGroup; it; it.Next()) {
		WINRECT* wrc = it;
		if (wrc->Type()==WRCT_REST) {
			assert(pRestRect==NULL);		 // can only be one REST rect!
			pRestRect = wrc;					 // remember it
		} else {
			AdjustSize(wrc, bRow, hwRemaining, hWnd);
		}
	}
	assert(hwRemaining>=0);

	// Adjust REST rect if any
	if (pRestRect) {
		AdjustSize(pRestRect, bRow, hwRemaining, hWnd);
		assert(hwRemaining==0);
	}

	// All the sizes of the entries have been calculated, including
	// groups (but not their children). Now move all the rects so they're
	// adjacent to one another, without altering sizes.
	PositionRects(pGroup, rcTotal, bRow);

	// Finally, descend recursively into each subgroup.
	for (it=pGroup; it; it.Next()) {
		WINRECT* wrc = it;
		if (wrc->IsGroup())
			CalcGroup(wrc, hWnd); // recurse!
	}
}

//////////////////
// Adjust the size of a single entry upwards to its desired size.
// Decrement hwRemaining by amount increased.
//
void
CWinMgr::AdjustSize(WINRECT* wrc, BOOL bRow,
	int& hwRemaining, HWND hWnd)
{
	SIZEINFO szi;
	OnGetSizeInfo(szi, wrc, hWnd);
	int hw = bRow ? szi.szDesired.cy : szi.szDesired.cx; // desired ht or wid
	if (wrc->Type() == WRCT_REST) {
		// for REST type, use all remaining space
		RECT& rc = wrc->GetRect();
		hw = hwRemaining + (bRow ? RectHeight(rc) : RectWidth(rc));
	}

	// Now hw is the desired height or width, and the current size of the
	// entry is the min size. So adjust the size upwards, and decrement
	// hwRemaining appropriately. This is a little confusing, but necessary so
	// each entry gets its min size.
	//
	int hwCurrent = wrc->GetHeightOrWidth(bRow); // current size
	int hwExtra = hw - hwCurrent;						// amount extra
	hwExtra = min(max(hwExtra, 0), hwRemaining);	// truncate 
	hw = hwCurrent + hwExtra;							// new height-or-width
	wrc->SetHeightOrWidth(hw, bRow);				// set...
	hwRemaining -= hwExtra;								// and adjust remaining
}

//////////////////
// Position all the rects so they're as wide/high as the total and follow one
// another; ie, are adjacent. This operation leaves the height (rows) and
// width (columns) unaffected. For rows, set each row's width to rcTotal and
// one below the other; for columns, set each column as tall as rcTotal and
// each to the right of the previous.
//
void
CWinMgr::PositionRects(WINRECT* pGroup, const RECT& rcTotal, BOOL bRow)
{
	LONG xoryPos = bRow ? rcTotal.top : rcTotal.left;

	CWinGroupIterator it;
	for (it=pGroup; it; it.Next()) {
		WINRECT* wrc = it;
		RECT& rc = wrc->GetRect();
		if (bRow) {							 // for ROWS:
			LONG height = RectHeight(rc);		 // height of row = total height
			rc.top    = xoryPos;				 // top = running yPos
			rc.bottom = rc.top + height;	 // ...
			rc.left   = rcTotal.left;		 // ...
			rc.right  = rcTotal.right;		 // ...
			xoryPos += height;				 // increment yPos

		} else {									 // for COLS:
			LONG width = RectWidth(rc);		 // width = total width
			rc.left    = xoryPos;			 // left = running xPos
			rc.right   = rc.left + width;	 // ...
			rc.top     = rcTotal.top;		 // ...
			rc.bottom  = rcTotal.bottom;	 // ...
			xoryPos += width;					 // increment xPos
		}
	}
}

//////////////////
// Get size information for a single entry (WINRECT). Returns size info in
// the SIZEINFO argument. For a group, calculate size info as aggregate of
// subentries.
//
void
CWinMgr::OnGetSizeInfo(SIZEINFO& szi, WINRECT* wrc, HWND hWnd)
{
	szi.szMin = SIZEZERO;				// default min size = zero
	szi.szMax = SIZEMAX;					// default max size = infinite
	szi.szDesired = RectToSize(wrc->GetRect());	// default desired size = current 

	if (wrc->IsGroup()) {
		// For groups, calculate min, max, desired size as aggregate of children
		szi.szDesired = SIZEZERO;
		BOOL bRow = wrc->IsRowGroup();

		CWinGroupIterator it;
		for (it=wrc; it; it.Next()) {
			WINRECT* wrc2 = it;
			SIZEINFO szi2;
			OnGetSizeInfo(szi2, wrc2, hWnd);
			if (bRow) {
				szi.szMin.cx = max(szi.szMin.cx, szi2.szMin.cx);
				szi.szMin.cy += szi2.szMin.cy;
				szi.szMax.cx = min(szi.szMax.cx, szi2.szMax.cx);
				szi.szMax.cy = min(szi.szMax.cy + szi2.szMax.cy, SHRT_MAX);
				szi.szDesired.cx = max(szi.szDesired.cx, szi2.szDesired.cx);
				szi.szDesired.cy += szi2.szDesired.cy;

			} else {
				szi.szMin.cx += szi2.szMin.cx;
				szi.szMin.cy = max(szi.szMin.cy, szi2.szMin.cy);
				szi.szMax.cx = min(szi.szMax.cx + szi2.szMax.cx, SHRT_MAX);
				szi.szMax.cy = min(szi.szMax.cy, szi2.szMax.cy);
				szi.szDesired.cx += szi2.szDesired.cx;
				szi.szDesired.cy = max(szi.szDesired.cy, szi2.szDesired.cy);
			}
		}

		// Add margins. 
		int w2,h2;
		wrc->GetMargins(w2,h2);			// get margins
		w2<<=1; h2<<=1;					// double
		szi.szMin.cx += max(0,w2);		// negative margins ==> don't include in min
		szi.szMin.cy += max(0,h2);		// ditto
		szi.szDesired.cx += abs(w2);	// for desired size, use abs vallue
		szi.szDesired.cy += abs(h2);	// ditto

	} else {
		// not a group
		WINRECT* parent = wrc->Parent();
		assert(parent);
		RECT& rcParent = parent->GetRect();
		BOOL bRow = parent->IsRowGroup();
		int hw, hwMin, hwTotal, pct;

		switch (wrc->Type()) {
		case WRCT_FIXED:
			hw = hwMin = wrc->GetParam();	 // ht/wid is parameter
			if (hw<0) {							 // if fixed val is negative:
				hw = -hw;						 // use absolute val for desired..
				hwMin = 0;						 // ..and zero for minimum
			}
			if (bRow) {
				szi.szMax.cy = szi.szDesired.cy = hw;
				szi.szMin.cy = hwMin;
			} else {
				szi.szMax.cx = szi.szDesired.cx = hw;
				szi.szMin.cx = hwMin;
			}
			break;

		case WRCT_PCT:
			pct = wrc->GetParam();
			assert(0<pct && pct<100);
			hwTotal = bRow ? RectHeight(rcParent) : RectWidth(rcParent);
			hw = (hwTotal * pct) / 100;
			szi.szDesired = bRow ? GetSize(RectWidth(rcParent), hw) : GetSize(hw, RectHeight(rcParent));
			break;

		case WRCT_TOFIT:
			if (wrc->HasToFitSize()) {
				szi.szDesired = wrc->GetToFitSize();
			}
			break;

		case WRCT_REST:
			break;

		default:
			assert(FALSE);
		}

		// If the entry is a window, send message to get min/max/tofit size.
		// Only set tofit size if type is TOFIT.
		//
		if (wrc->IsWindow() && hWnd) {
			HWND hChild = GetDlgItem(hWnd, wrc->GetID());
			if (hChild) {
				if (!IsWindowVisible(hChild) && IsWindowVisible(hWnd)) {
					// parent visible but child not ==> tofit size is zero
					// important so hidden windows use no space
					szi.szDesired = SIZEZERO;
				} else {
					szi.szAvail = RectToSize(rcParent);
					SendGetSizeInfo(szi, hWnd, wrc->GetID());
				}
			}
		}
		szi.szDesired = maxsize(minsize(szi.szDesired,szi.szMax), szi.szMin);
	}
}

//////////////////
// Send message to parent, then window itself, to get size info.
//
BOOL CWinMgr::SendGetSizeInfo(SIZEINFO& szi, HWND hWnd, UINT nID)
{
	NMWINMGR nmw;
	nmw.code = NMWINMGR::GET_SIZEINFO;	// request size info
	nmw.idFrom = nID;							// ID of child I'm computing
	nmw.sizeinfo = szi;						// copy

	if (!SendMessage(hWnd, WM_WINMGR, nID, (LPARAM)&nmw) && !nmw.processed) {
		HWND hwndChild = ::GetDlgItem(hWnd, nID);
		if (!hwndChild || !::SendMessage(hwndChild,WM_WINMGR,nID,(LPARAM)&nmw))
			return FALSE;
	}
	szi = nmw.sizeinfo; // copy back to caller's struct
	return TRUE;
}
		
//////////////////
// Get min/max info.
//
void
CWinMgr::GetMinMaxInfo(HWND hWnd, MINMAXINFO* lpMMI)
{
	SIZEINFO szi;
	GetMinMaxInfo(hWnd, szi); // call overloaded version
	lpMMI->ptMinTrackSize = SizeToPoint(szi.szMin);
	lpMMI->ptMaxTrackSize = SizeToPoint(szi.szMax);
}

//////////////////
// Get min/max info. 
//
void CWinMgr::GetMinMaxInfo(HWND hWnd, SIZEINFO& szi)
{
	OnGetSizeInfo(szi, m_map, hWnd);  // get size info
	if (!hWnd)					 // window not created ==> done
		return;

	// Add extra space for frame/dialog screen junk.
	DWORD dwStyle = GetStyle(hWnd);
	DWORD dwExStyle = GetExStyle(hWnd);
	if (dwStyle & WS_VISIBLE) {
		SIZE& szMin = szi.szMin; // ref!
		if (!(dwStyle & WS_CHILD)) {
			if (dwStyle & WS_CAPTION)
				szMin.cy += GetSystemMetrics(SM_CYCAPTION);
			if (::GetMenu(hWnd))
				szMin.cy += GetSystemMetrics(SM_CYMENU);
		}
		if (dwStyle & WS_THICKFRAME) {
			szMin.cx += 2*GetSystemMetrics(SM_CXSIZEFRAME);
			szMin.cy += 2*GetSystemMetrics(SM_CYSIZEFRAME);
		} else if (dwStyle & WS_BORDER) {
			szMin.cx += 2*GetSystemMetrics(SM_CXBORDER);
			szMin.cy += 2*GetSystemMetrics(SM_CYBORDER);
		}
		if (dwExStyle & WS_EX_CLIENTEDGE) {
			szMin.cx += 2*GetSystemMetrics(SM_CXEDGE);
			szMin.cy += 2*GetSystemMetrics(SM_CYEDGE);
		}
	}
}

//////////////////
// Move desired rectangle by a given vector amount.
// Call this when a sizer bar tells you it has moved.
//
void CWinMgr::MoveRect(WINRECT* pwrcMove, POINT ptMove, HWND pParentWnd)
{
	assert(pwrcMove);
	WINRECT* prev = pwrcMove->Prev();
	assert(prev);
	WINRECT* next = pwrcMove->Next();
	assert(next);

	BOOL bIsRow = pwrcMove->Parent()->IsRowGroup();

	RECT& rcNext = next->GetRect();
	RECT& rcPrev = prev->GetRect();
	if (bIsRow) {
		// a row can only be moved up or down
		ptMove.x = 0;
		rcPrev.bottom += ptMove.y;
		rcNext.top += ptMove.y;
	} else {
		// a column can only be moved left or right
		ptMove.y = 0;
		rcPrev.right += ptMove.x;
		rcNext.left += ptMove.x;
	}
	OffsetRect(pwrcMove->GetRect(), ptMove);
	if (prev->IsGroup())
		CalcGroup(prev, pParentWnd);
	if (next->IsGroup())
		CalcGroup(next, pParentWnd);
}

// Theo - Removed Tracing
