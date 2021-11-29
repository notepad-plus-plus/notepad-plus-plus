// Scintilla source code edit control
/** @file ScintillaWin.h
 ** Define functions from ScintillaWin.cxx that can be called from ScintillaDLL.cxx.
 **/
// Copyright 1998-2018 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SCINTILLAWIN_H
#define SCINTILLAWIN_H

class ScintillaWin;

namespace Scintilla {

int ResourcesRelease(bool fromDllMain) noexcept;
sptr_t DirectFunction(ScintillaWin *sci, UINT iMessage, uptr_t wParam, sptr_t lParam);

}

#endif
