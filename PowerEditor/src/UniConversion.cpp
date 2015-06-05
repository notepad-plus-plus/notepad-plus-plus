// Scintilla source code edit control
/** @file UniConversion.cxx
 ** Functions to handle UFT-8 and UCS-2 strings.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <windows.h>
#include "UniConversion.h"

unsigned int UTF8Length(const wchar_t *uptr, unsigned int tlen) {
	unsigned int len = 0;
	for (unsigned int i = 0; i < tlen && uptr[i]; ++i) {
		unsigned int uch = uptr[i];
		if (uch < 0x80)
			++len;
		else if (uch < 0x800)
			len += 2;
		else 
			len +=3;
	}
	return len;
}

void UTF8FromUCS2(const wchar_t *uptr, unsigned int tlen, char *putf, unsigned int len) {
	int k = 0;
	for (unsigned int i = 0; i < tlen && uptr[i]; ++i) {
		unsigned int uch = uptr[i];
		if (uch < 0x80) {
			putf[k++] = static_cast<char>(uch);
		} else if (uch < 0x800) {
			putf[k++] = static_cast<char>(0xC0 | (uch >> 6));
			putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
		} else {
			putf[k++] = static_cast<char>(0xE0 | (uch >> 12));
			putf[k++] = static_cast<char>(0x80 | ((uch >> 6) & 0x3f));
			putf[k++] = static_cast<char>(0x80 | (uch & 0x3f));
		}
	}
	putf[len] = '\0';
}

unsigned int UCS2Length(const char *s, unsigned int len) {
	unsigned int ulen = 0;
	for (unsigned int i=0; i<len; ++i) {
		UCHAR ch = static_cast<UCHAR>(s[i]);
		if ((ch < 0x80) || (ch > (0x80 + 0x40)))
			++ulen;
	}
	return ulen;
}

unsigned int UCS2FromUTF8(const char *s, unsigned int len, wchar_t *tbuf, unsigned int tlen)
{
	unsigned int ui=0;
	const UCHAR *us = reinterpret_cast<const UCHAR *>(s);
	unsigned int i=0;
	while ((i<len) && (ui<tlen)) {
		UCHAR ch = us[i++];
		if (ch < 0x80) {
			tbuf[ui] = ch;
		} else if (ch < 0x80 + 0x40 + 0x20) {
			tbuf[ui] = static_cast<wchar_t>((ch & 0x1F) << 6);
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + (ch & 0x7F));
		} else {
			tbuf[ui] = static_cast<wchar_t>((ch & 0xF) << 12);
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + ((ch & 0x7F) << 6));
			ch = us[i++];
			tbuf[ui] = static_cast<wchar_t>(tbuf[ui] + (ch & 0x7F));
		}
		ui++;
	}
	return ui;
}


unsigned int ascii_to_utf8(const char * pszASCII, unsigned int lenASCII, char * pszUTF8)
{
  // length of pszUTF8 must be enough; 
  // its maximum is (lenASCII*3 + 1)
  
  if (!lenASCII || !pszASCII)
  {
    pszUTF8[0] = 0;
    return 0;  
  }

  unsigned int lenUCS2;
  unsigned int lenUTF8;
  wchar_t *pszUCS2 = new wchar_t[lenASCII * 3 + 1];
  if (!pszUCS2)
  {
    pszUTF8[0] = 0;
    return 0;  
  }

  lenUCS2 = ::MultiByteToWideChar(CP_ACP, 0, pszASCII, lenASCII, pszUCS2, lenASCII + 1);
  lenUTF8 = UTF8Length(pszUCS2, lenUCS2);
  // length of pszUTF8 must be >= (lenUTF8 + 1)
  UTF8FromUCS2(pszUCS2, lenUCS2, pszUTF8, lenUTF8);
  delete [] pszUCS2;
  return lenUTF8;
}

int utf8_to_ascii(const char * pszUTF8, unsigned int lenUTF8, char * pszASCII)
{
  // length of pszASCII must be enough;
  // its maximum is (lenUTF8 + 1)
  
  if (!lenUTF8 || !pszUTF8)
  {
    pszASCII[0] = 0;
    return 0;
  }  

  unsigned int lenUCS2;
  wchar_t*     pszUCS2;

  pszUCS2 = new wchar_t[lenUTF8 + 1];
  if (!pszUCS2)
  {
    pszASCII[0] = 0;
    return 0;
  }

  lenUCS2 = UCS2FromUTF8(pszUTF8, lenUTF8, pszUCS2, lenUTF8);
  pszUCS2[lenUCS2] = 0;
  // length of pszASCII must be >= (lenUCS2 + 1)
  int nbByte = ::WideCharToMultiByte(CP_ACP, 0, pszUCS2, lenUCS2, pszASCII, lenUCS2 + 1, NULL, NULL);
  delete [] pszUCS2;
  return nbByte;
}

