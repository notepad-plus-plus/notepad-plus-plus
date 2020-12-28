// Scintilla source code edit control
/** @file ScintillaWin.h
 ** Define functions from ScintillaWin.cxx that can be called from ScintillaDLL.cxx.
 **/
// Copyright 1998-2018 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

class ScintillaWin;

namespace Scintilla {

int ResourcesRelease(bool fromDllMain);
sptr_t DirectFunction(ScintillaWin *sci, UINT iMessage, uptr_t wParam, sptr_t lParam);

}
