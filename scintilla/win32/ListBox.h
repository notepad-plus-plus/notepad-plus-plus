// Scintilla source code edit control
/** @file ListBox.h
 ** Definitions for list box on Windows.
 **/
// Copyright 2025 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef LISTBOX_H
#define LISTBOX_H

namespace Scintilla::Internal {

bool ListBoxX_Register() noexcept;
void ListBoxX_Unregister() noexcept;

}

#endif
