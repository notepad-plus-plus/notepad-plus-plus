////////////////////////////////////////////////////////////////
// MSDN Magazine -- July 2001
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
// Compiles with Visual C++ 6.0. Runs on Win 98 and probably Win 2000 too.
// Set tabsize = 3 in your editor.
//
// WinMgr.h -- Main header file for WinMgr library.
//
// Theo - Heavily modified to remove MFC dependencies
//        Replaced CWnd*/HWND, CRect/RECT, CSize/SIZE, CPoint/POINT


#pragma once

#include <algorithm>
#include <assert.h>
#include <windows.h>


const SIZE SIZEZERO = {0, 0};
const SIZE SIZEMAX = {SHRT_MAX, SHRT_MAX};

inline SIZE GetSize(LONG w, LONG h) { 
	SIZE sz = {w, h}; return sz; 
}

inline POINT GetPoint(LONG x, LONG y) { 
	POINT pt = {x, y}; return pt; 
}

inline LONG RectWidth(const RECT& rc) { 
	return rc.right - rc.left; 
}

inline LONG RectHeight(const RECT& rc) { 
	return rc.bottom - rc.top; 
}

inline SIZE RectToSize(const RECT& rc) { 
	return GetSize(RectWidth(rc), RectHeight(rc));
}

inline POINT RectToPoint(const RECT& rc) { 
	POINT pt = {rc.left, rc.top};
	return pt; 
}

inline POINT SizeToPoint(SIZE sz) { 
	return GetPoint(sz.cx, sz.cy);
}

inline RECT &OffsetRect(RECT& rc, POINT pt) {
	rc.left += pt.x; rc.right += pt.x;
	rc.top += pt.y; rc.bottom += pt.y;
	return rc;
}

// handy functions to take the min or max of a SIZE
inline SIZE minsize(SIZE a, SIZE b) {
	return GetSize(std::min<LONG>(a.cx, b.cx), std::min<LONG>(a.cy, b.cy));
}

inline SIZE maxsize(SIZE a, SIZE b) {
	return GetSize(std::max<LONG>(a.cx, b.cx), std::max<LONG>(a.cy, b.cy));
}

//////////////////
// Size info about a rectangle/row/column
//
struct SIZEINFO {
	SIZE szAvail;		// total size avail (passed)
	SIZE szDesired;	// desired size: default=current
	SIZE szMin;			// minimum size: default=SIZEZERO
	SIZE szMax;			// maximum size: default=MAXSIZE
};

// types of rectangles:
#define	WRCT_END			0				// end of table
#define	WRCT_FIXED		0x0001		// height/width is fixed
#define	WRCT_PCT			0x0002		// height/width is percent of total
#define	WRCT_REST		0x0003		// height/width is whatever remains
#define	WRCT_TOFIT		0x0004		// height/width to fit contents
#define	WRCF_TYPEMASK	0x000F

// group flags
#define	WRCF_ROWGROUP	0x0010		// beginning of row group
#define	WRCF_COLGROUP	0x0020		// beginning of column group
#define	WRCF_ENDGROUP	0x00F0		// end of group
#define	WRCF_GROUPMASK	0x00F0

//////////////////
// This structure is used to hold a rectangle and describe its layout. Each
// WINRECT corresponds to a child rectangle/window. Each window that uses
// WinMgr provides a table (C array) of these to describe its layout.
//
class WINRECT {
protected:
	// pointers initialized by the window manager for easy traversing:
	WINRECT* next;			// next at this level
	WINRECT* prev;			// prev at this level

	// data
	RECT  rc;				// current rectangle position/size
	WORD  flags;			// flags (see above)
	UINT	nID;				// window ID if this WINRECT represents a window
	LONG	param;			// arg depends on type

public:
	WINRECT(WORD f, int id, LONG p);

	static WINRECT* InitMap(WINRECT* map, const WINRECT* parent=NULL);

	WINRECT* Prev()			{ return prev; }
	WINRECT* Next()			{ return next; }
	WINRECT* Children()		{ return IsGroup() ? this+1 : NULL; }
	WINRECT* Parent();
	WORD GetFlags()			{ return flags; }
	WORD SetFlags(WORD f)	{ return flags=f; }
	LONG GetParam()			{ return param; }
	LONG SetParam(LONG p)	{ return param=p; }
	UINT GetID()				{ return nID; }
	UINT SetID(UINT id)		{ return nID=id; }
	RECT& GetRect()					{ return rc; }
	void SetRect(const RECT& r)	{ rc = r; }
	WORD Type() const			{ return flags & WRCF_TYPEMASK; }
	WORD GroupType() const	{ return flags & WRCF_GROUPMASK; }
	BOOL IsGroup() const		{ return GroupType() && GroupType()!=WRCF_ENDGROUP; }
	BOOL IsEndGroup() const { return flags==0 || flags==WRCF_ENDGROUP; }
	BOOL IsEnd() const		{ return flags==0; }
	BOOL IsWindow() const	{ return nID>0; }
	BOOL IsRowGroup()	const { return (flags & WRCF_GROUPMASK)==WRCF_ROWGROUP; }
	void SetHeight(LONG h)	{ rc.bottom = rc.top + h; }
	void SetWidth(LONG w)	{ rc.right = rc.left + w; }
	LONG GetHeightOrWidth(BOOL bHeight) const {
		return bHeight ? RectHeight(rc) : RectWidth(rc);
	}
	void SetHeightOrWidth(LONG horw, BOOL bHeight) {
		bHeight ? SetHeight(horw) : SetWidth(horw);
	}
	BOOL GetMargins(int& w, int& h);

	// For TOFIT types, param is the TOFIT size, if nonzero. Used in dialogs,
	// with CWinMgr::InitToFitSizeFromCurrent.
	BOOL HasToFitSize()			{ return param != 0; }
	SIZE GetToFitSize()			{ SIZE sz = {LOWORD(param),HIWORD(param)}; return sz; }
	void SetToFitSize(SIZE sz)	{ param = MAKELONG(sz.cx,sz.cy); }
};

//////////////////
// Below are all the macros to build your window map. 
//

// Begin/end window map. 'name' can be anything you want
#define BEGIN_WINDOW_MAP(name)	WINRECT name[] = {
#define END_WINDOW_MAP()			WINRECT(WRCT_END,-1,0) }; 

// Begin/end a group.
// The first entry in your map must be BEGINROWS or BEGINCOLS.
#define BEGINROWS(type,id,m)	WINRECT(WRCF_ROWGROUP|type,id,m),
#define BEGINCOLS(type,id,m)  WINRECT(WRCF_COLGROUP|type,id,m),
#define ENDGROUP()				WINRECT(WRCF_ENDGROUP,-1,0),

// This macros is used only with BEGINGROWS or BEGINCOLS to specify margins
#define RCMARGINS(w,h)			MAKELONG(w,h)

// Macros for primitive (non-group) entries.
// val applies to height for a row entry; width for a column entry.
#define RCFIXED(id,val)		WINRECT(WRCT_FIXED,id,val),
#define RCPERCENT(id,val)	WINRECT(WRCT_PCT,id,val),
#define RCREST(id)			WINRECT(WRCT_REST,id,0),
#define RCTOFIT(id)			WINRECT(WRCT_TOFIT,id,0),
#define RCSPACE(val)			RCFIXED(-1,val)

//////////////////
// Use this to iterate the entries in a group.
//
//	CWinGroupIterator it;
//	for (it=pGroup; it; it.Next()) {
//   WINRECT* wrc = it;
//   ..
// }
//
class CWinGroupIterator {
protected:
	WINRECT* pCur;	  // current entry
public:
	CWinGroupIterator() { pCur = NULL; }
	CWinGroupIterator& operator=(WINRECT* pg) {
		assert(pg->IsGroup()); // can only iterate a group!
		pCur = pg->Children();
		return *this;
	}
	operator WINRECT*()	{ return pCur; }
	WINRECT* pWINRECT()	{ return pCur; }
	WINRECT* Next()		{ return pCur = pCur ? pCur->Next() : NULL;}
};

// Registered WinMgr message
extern const UINT WM_WINMGR;

// Notification struct, passed as LPARAM
struct NMWINMGR : public NMHDR {
	enum {								// notification codes:
		GET_SIZEINFO = 1,				// WinMgr is requesting size info
		SIZEBAR_MOVED					// user moved sizer bar
	};

	// each notification code has its own part of union
	union {
		SIZEINFO sizeinfo{};	// used for GET_SIZEINFO
		struct {					// used for SIZEBAR_MOVED
			POINT ptMoved{};		//  distance moved (x or y = zero)
		} sizebar;
	};
	BOOL processed = FALSE;

	// ctor: initialize to zeroes
	NMWINMGR() : NMHDR{ nullptr, 0, 0 }, sizeinfo{}, processed(FALSE) {}
};

///////////////////
// Window manager. This class calculates all the sizes and positions of the
// rectangles in the window map.
//
class CWinMgr /*: public CObject*/ {
public:
	explicit CWinMgr(WINRECT* map);
	virtual ~CWinMgr();

	virtual void GetWindowPositions(HWND hWnd); // load map from window posns
	virtual void SetWindowPositions(HWND hWnd); // set window posns from map

	// get min/max/desired size of a rectangle
	virtual void OnGetSizeInfo(SIZEINFO& szi, WINRECT* pwrc, HWND hWnd=NULL);

	// calc layout using client area as total area
	void CalcLayout(HWND hWnd) {
		assert(hWnd);
		RECT rcClient;
		GetClientRect(hWnd, &rcClient);
		CalcLayout(rcClient, hWnd);
	}

	// calc layout using cx, cy (for OnSize)
	void CalcLayout(int cx, int cy, HWND hWnd=NULL) {
		RECT rc = {0,0,cx,cy};
		CalcLayout(rc, hWnd);
	}

	// calc layout using given rect as total area
	void CalcLayout(RECT rcTotal, HWND hWnd=NULL) {
		assert(m_map);
		m_map->SetRect(rcTotal);
		CalcGroup(m_map, hWnd);
	}

	// Move rectangle vertically or horizontally. Used with sizer bars.
	void MoveRect(int nID, POINT ptMove, HWND pParentWnd) {
		MoveRect(FindRect(nID), ptMove, pParentWnd);
	}
	void MoveRect(WINRECT* pwrcMove, POINT ptMove, HWND pParentWnd);

	RECT GetRect(UINT nID)						 { return FindRect(nID)->GetRect(); }
	void SetRect(UINT nID, const RECT& rc) { FindRect(nID)->SetRect(rc); }

	// get WINRECT corresponding to ID
	WINRECT* FindRect(int nID);

	// Calculate MINMAXINFO
	void GetMinMaxInfo(HWND hWnd, MINMAXINFO* lpMMI);
	void GetMinMaxInfo(HWND hWnd, SIZEINFO& szi);

	// set TOFIT size for all windows from current window sizes
	void InitToFitSizeFromCurrent(HWND hWnd);


protected:
	WINRECT*	m_map = nullptr;			// THE window map

	int  CountWindows();
	BOOL SendGetSizeInfo(SIZEINFO& szi, HWND hWnd, UINT nID);

	// you can override to do wierd stuff or fix bugs
	virtual void CalcGroup(WINRECT* group, HWND hWnd);
	virtual void AdjustSize(WINRECT* pEntry, BOOL bRow,
		int& hwRemaining, HWND hWnd);
	virtual void PositionRects(WINRECT* pGroup,
		const RECT& rcTotal,BOOL bRow);

private:
	CWinMgr() { assert(FALSE); } // no default constructor
};
