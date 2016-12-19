// Scintilla source code edit control
/** @file HanjaDic.h
 ** Korean Hanja Dictionary
 ** Convert between Korean Hanja and Hangul by COM interface.
 **/
// Copyright 2015 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef HANJADIC_H
#define HANJADIC_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

namespace HanjaDict {

int GetHangulOfHanja(wchar_t *inout);

}

#ifdef SCI_NAMESPACE
}
#endif

#endif
