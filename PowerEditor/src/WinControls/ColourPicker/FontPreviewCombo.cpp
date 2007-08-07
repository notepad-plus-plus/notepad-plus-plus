/*
this file is part of notepad++
Copyright (C)2003 Don HO < donho@altern.org >

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

void FontPreviewCombo::DrawItem(LPDRAWITEMSTRUCT lpDIS) 
{
	ASSERT(lpDIS->CtlType == ODT_COMBOBOX); 
	
	CRect rc = lpDIS->rcItem;
	
	CDC dc;
	dc.Attach(lpDIS->hDC);

	if (lpDIS->itemState & ODS_FOCUS)
		dc.DrawFocusRect(&rc);
	
	if (lpDIS->itemID == -1)
		return;

	int nIndexDC = dc.SaveDC();
	
	CBrush br;
	
	COLORREF clrSample = m_clrSample;

	if (lpDIS->itemState & ODS_SELECTED)
	{
		br.CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
		dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
		clrSample = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
	}
	else
	{
		br.CreateSolidBrush(dc.GetBkColor());
	}
	
	dc.SetBkMode(TRANSPARENT);
	dc.FillRect(&rc, &br);
	
	// which one are we working on?
	CString csCurFontName;
	GetLBText(lpDIS->itemID, csCurFontName);

	// draw the cute glyph
	DWORD dwData = GetItemData(lpDIS->itemID);

	if (dwData & OPENTYPE_FONTTYPE)
	{
		m_OpenType.Draw(&dc, 0, CPoint(rc.left+5, rc.top+1),ILD_TRANSPARENT);
	}
	else if (dwData & TRUETYPE_FONTTYPE)
	{
		m_TrueType.Draw(&dc, 0, CPoint(rc.left+5, rc.top+1),ILD_TRANSPARENT);
	}

	rc.left += GLYPH_WIDTH;
	
	int iOffsetX = SPACING;

	// i feel bad creating this font on each draw. but i can't think of a 
	// better way (other than creating ALL fonts at once and saving them - yuck)
	CFont cf;
	if (m_style != NAME_GUI_FONT)
	{
		if (!cf.CreateFont(m_iFontHeight,0,0,0,FW_NORMAL,FALSE, FALSE, 
					FALSE,DEFAULT_CHARSET ,OUT_DEFAULT_PRECIS,
					CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,
					DEFAULT_PITCH, csCurFontName))
		{
			ASSERT(0);
			return;
		}
	}

	// draw the text
	CSize sz;
	int iPosY = 0;
	HFONT hf = NULL;
	switch (m_style)
	{
		case NAME_GUI_FONT:
		{
			// font name in GUI font
			sz = dc.GetTextExtent(csCurFontName);
			iPosY = (rc.Height() - sz.cy) / 2;
			dc.TextOut(rc.left + iOffsetX, rc.top + iPosY, csCurFontName);
		}
		break;

		case NAME_ONLY:
		{
			// font name in current font
			hf = (HFONT)dc.SelectObject(cf);
			sz = dc.GetTextExtent(csCurFontName);
			iPosY = (rc.Height() - sz.cy) / 2;
			dc.TextOut(rc.left+iOffsetX, rc.top + iPosY,csCurFontName);
			dc.SelectObject(hf);
		}
		break;

		case NAME_THEN_SAMPLE:
		{
			// font name in GUI font
			sz = dc.GetTextExtent(csCurFontName);
			iPosY = (rc.Height() - sz.cy) / 2;
			dc.TextOut(rc.left + iOffsetX, rc.top + iPosY, csCurFontName);

			// condense, for edit
			int iSep = m_iMaxNameWidth;
			if ((lpDIS->itemState & ODS_COMBOBOXEDIT) == ODS_COMBOBOXEDIT)
			{
				iSep = sz.cx;
			}

			// sample in current font
			hf = (HFONT)dc.SelectObject(cf);
			sz = dc.GetTextExtent(m_csSample);
			iPosY = (rc.Height() - sz.cy) / 2;
			COLORREF clr = dc.SetTextColor(clrSample);
			dc.TextOut(rc.left + iOffsetX + iSep + iOffsetX, rc.top + iPosY, 
				m_csSample);
			dc.SetTextColor(clr);
			dc.SelectObject(hf);
		}
		break;

		case SAMPLE_THEN_NAME:
		{
			// sample in current font
			hf = (HFONT)dc.SelectObject(cf);
			sz = dc.GetTextExtent(m_csSample);
			iPosY = (rc.Height() - sz.cy) / 2;
			COLORREF clr = dc.SetTextColor(clrSample);
			dc.TextOut(rc.left+iOffsetX, rc.top + iPosY, m_csSample);
			dc.SetTextColor(clr);
			dc.SelectObject(hf);

			// condense, for edit
			int iSep = m_iMaxSampleWidth;
			if ((lpDIS->itemState & ODS_COMBOBOXEDIT) == ODS_COMBOBOXEDIT)
			{
				iSep = sz.cx;
			}

			// font name in GUI font
			sz = dc.GetTextExtent(csCurFontName);
			iPosY = (rc.Height() - sz.cy) / 2;
			dc.TextOut(rc.left + iOffsetX + iSep + iOffsetX, rc.top + iPosY, 
				csCurFontName);
		}
		break;

		case SAMPLE_ONLY:
		{			
			// sample in current font
			hf = (HFONT)dc.SelectObject(cf);
			sz = dc.GetTextExtent(m_csSample);
			iPosY = (rc.Height() - sz.cy) / 2;
			dc.TextOut(rc.left+iOffsetX, rc.top + iPosY, m_csSample);
			dc.SelectObject(hf);
		}
		break;
	}

	dc.RestoreDC(nIndexDC);

	dc.Detach();
}