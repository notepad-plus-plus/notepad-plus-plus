// Scintilla source code edit control
/** @file DBCS.h
 ** Functions to handle DBCS double byte encodings like Shift-JIS.
 **/
// Copyright 2017 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef DBCS_H
#define DBCS_H

namespace Scintilla {

bool DBCSIsLeadByte(int codePage, char ch) noexcept;

}

#endif
