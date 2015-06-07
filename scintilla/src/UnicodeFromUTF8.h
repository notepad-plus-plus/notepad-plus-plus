// Scintilla source code edit control
/** @file UnicodeFromUTF8.h
 ** Lexer infrastructure.
 **/
// Copyright 2013 by Neil Hodgson <neilh@scintilla.org>
// This file is in the public domain.

#ifndef UNICODEFROMUTF8_H
#define UNICODEFROMUTF8_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

inline int UnicodeFromUTF8(const unsigned char *us) {
	if (us[0] < 0xC2) {
		return us[0];
	} else if (us[0] < 0xE0) {
		return ((us[0] & 0x1F) << 6) + (us[1] & 0x3F);
	} else if (us[0] < 0xF0) {
		return ((us[0] & 0xF) << 12) + ((us[1] & 0x3F) << 6) + (us[2] & 0x3F);
	} else if (us[0] < 0xF5) {
		return ((us[0] & 0x7) << 18) + ((us[1] & 0x3F) << 12) + ((us[2] & 0x3F) << 6) + (us[3] & 0x3F);
	}
	return us[0];
}

#ifdef SCI_NAMESPACE
}
#endif

#endif
