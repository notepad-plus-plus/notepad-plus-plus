// Scintilla source code edit control
/** @file UniConversion.h
 ** Functions to handle UFT-8 and UCS-2 strings.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

unsigned int UTF8Length(const wchar_t * uptr, unsigned int tlen);
void UTF8FromUCS2(const wchar_t * uptr, unsigned int tlen, char * putf, unsigned int len);
unsigned int UCS2Length(const char * s, unsigned int len);
unsigned int UCS2FromUTF8(const char * s, unsigned int len, wchar_t * tbuf, unsigned int tlen);
unsigned int ascii_to_utf8(const char * pszASCII, unsigned int lenASCII, char * pszUTF8);
int utf8_to_ascii(const char * pszUTF8, unsigned int lenUTF8, char * pszASCII);
