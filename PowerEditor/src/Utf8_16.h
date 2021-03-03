// Utf8_16.h
// Copyright (C) 2002 Scott Kirkwood
//
// Permission to use, copy, modify, distribute and sell this code
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies or
// any derived copies.  Scott Kirkwood makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.
//
// Notes: Used the UTF information I found at:
//   http://www.cl.cam.ac.uk/~mgk25/unicode.html
////////////////////////////////////////////////////////////////////////////////
// 
// Modificated 2006 Jens Lorenz
// 
// - Clean up the sources
// - Removing UCS-Bug in Utf8_Iter
// - Add convert function in Utf8_16_Write
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Parameters.h"

#ifdef _MSC_VER
#pragma warning(disable: 4514) // nreferenced inline function has been removed
#endif

class Utf8_16 {
public:
	typedef unsigned short utf16; // 16 bits
	typedef UCHAR utf8; // 8 bits
	typedef UCHAR ubyte;
	static const utf8 k_Boms[uniEnd][3];
};

// Reads UTF-16 and outputs UTF-8
class Utf16_Iter : public Utf8_16 {
public:
	enum eState {
	    eStart,
	    e2Bytes2,
	    e3Bytes2,
	    e3Bytes3,
		eSurrogate
	};

	Utf16_Iter();
	void reset();
	void set(const ubyte* pBuf, size_t nLen, UniMode eEncoding);
	bool get(utf8 *c);
	void operator++();
	eState getState() { return m_eState; };
	operator bool() { return m_pRead < m_pEnd; };

protected:
	void read();
	void pushout(ubyte c);

protected:
	UniMode m_eEncoding;
	eState m_eState;
	utf8 m_out [16];
	int m_out1st;
	int m_outLst;
	utf16 m_nCur16;
	utf16 m_highSurrogate;
	const ubyte* m_pBuf;
	const ubyte* m_pRead;
	const ubyte* m_pEnd;
};

// Reads UTF-8 and outputs UTF-16
class Utf8_Iter : public Utf8_16 {
public:
	Utf8_Iter();
	void reset();
	void set(const ubyte* pBuf, size_t nLen, UniMode eEncoding);
	bool get(utf16* c);
	bool canGet() const { return m_out1st != m_outLst; }
	void toStart();
	void operator++();
	operator bool() { return m_pRead < m_pEnd; }

protected:
	enum eState {eStart, eFollow};
	void pushout(utf16 c);
protected:
	UniMode m_eEncoding;
	eState m_eState;
	int m_code;
	int m_count;
	utf16 m_out [4];
	int m_out1st, m_outLst;
	const ubyte* m_pBuf;
	const ubyte* m_pRead;
	const ubyte* m_pEnd;
};

// Reads UTF16 and outputs UTF8
enum u78 {utf8NoBOM=0, ascii7bits=1, ascii8bits=2};
class Utf8_16_Read : public Utf8_16 {
public:
	Utf8_16_Read();
	~Utf8_16_Read();

	size_t convert(char* buf, size_t len);
	const char* getNewBuf() const { return (const char*) m_pNewBuf; }
	size_t getNewSize() const { return m_nNewBufSize; }

	UniMode getEncoding() const { return m_eEncoding; }
    static UniMode determineEncoding(const unsigned char *buf, size_t bufLen);

protected:
	void determineEncoding();

	u78 utf8_7bits_8bits();
private:
	UniMode    m_eEncoding;
	ubyte*          m_pBuf;
	ubyte*          m_pNewBuf;
	// size of the new buffer
	size_t          m_nNewBufSize;
	// size of the previously allocated buffer (if != 0)
	size_t          m_nAllocatedBufSize;
	size_t			m_nSkip;
	bool            m_bFirstRead;
	size_t          m_nLen;
	Utf16_Iter      m_Iter16;
};

// Read in a UTF-8 buffer and write out to UTF-16 or UTF-8
class Utf8_16_Write : public Utf8_16 {
public:
	Utf8_16_Write();
	~Utf8_16_Write();

	void setEncoding(UniMode eType);

	FILE * fopen(const TCHAR *_name, const TCHAR *_type);
	size_t fwrite(const void* p, size_t _size);
	void   fclose();

	size_t convert(char* p, size_t _size);
	char* getNewBuf() { return reinterpret_cast<char*>(m_pNewBuf); }

protected:
	UniMode m_eEncoding;
	FILE* m_pFile;
	ubyte* m_pNewBuf;
	size_t m_nBufSize;
	bool m_bFirstWrite;
};

