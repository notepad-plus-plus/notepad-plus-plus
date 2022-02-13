// Utf8_16.cxx
// Copyright (C) 2002 Scott Kirkwood
//
// Permission to use, copy, modify, distribute and sell this code
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies or
// any derived copies.  Scott Kirkwood makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////
// 
// Modificated 2006 Jens Lorenz
// 
// - Clean up the sources
// - Removing UCS-Bug in Utf8_Iter
// - Add convert function in Utf8_16_Write
////////////////////////////////////////////////////////////////////////////////

#include "Utf8_16.h"

const Utf8_16::utf8 Utf8_16::k_Boms[][3] = {
	{0x00, 0x00, 0x00},  // Unknown
	{0xEF, 0xBB, 0xBF},  // UTF8
	{0xFE, 0xFF, 0x00},  // Big endian
	{0xFF, 0xFE, 0x00},  // Little endian
};


// ==================================================================


Utf8_16_Read::~Utf8_16_Read()
{
	if ((m_eEncoding == uni16BE) || (m_eEncoding == uni16LE) || (m_eEncoding == uni16BE_NoBOM) || (m_eEncoding == uni16LE_NoBOM))
    {
		delete [] m_pNewBuf;
		m_pNewBuf = NULL;
	}
}

// Returned value :
// 0 : utf8
// 1 : 7bits
// 2 : 8bits
u78 Utf8_16_Read::utf8_7bits_8bits()
{
	int rv = 1;
	int ASCII7only = 1;
	utf8 *sx	= (utf8 *)m_pBuf;
	utf8 *endx	= sx + m_nLen;

	while (sx<endx)
	{
		if (*sx == '\0')
		{											// For detection, we'll say that NUL means not UTF8
			ASCII7only = 0;
			rv = 0;
			break;
		} 
		else if ((*sx & 0x80) == 0x0)
		{			// 0nnnnnnn If the byte's first hex code begins with 0-7, it is an ASCII character.
			++sx;
		} 
		else if ((*sx & (0x80+0x40)) == 0x80) 
		{											  // 10nnnnnn 8 through B cannot be first hex codes
			ASCII7only=0;
			rv=0;
			break;
		} 
		else if ((*sx & (0x80+0x40+0x20)) == (0x80+0x40))
		{					  // 110xxxvv 10nnnnnn, 11 bit character
			ASCII7only=0;
			if (std::distance(sx, endx) < 2) {
				rv=0; break;
			}
			if ( (sx[1]&(0x80+0x40)) != 0x80) {
				rv=0; break;
			}
			sx+=2;
		} 
		else if ((*sx & (0x80+0x40+0x20+0x10)) == (0x80+0x40+0x20))
		{								// 1110qqqq 10xxxxvv 10nnnnnn, 16 bit character
			ASCII7only=0;
			if (std::distance(sx, endx) < 3) {
				rv=0; break;
			}
			if ((sx[1]&(0x80+0x40)) != 0x80 || (sx[2]&(0x80+0x40)) != 0x80) {
				rv=0; break;
			}
			sx+=3;
		} 
		else if ((*sx & (0x80+0x40+0x20+0x10+0x8)) == (0x80+0x40+0x20+0x10))
		{								// 11110qqq 10xxxxvv 10nnnnnn 10mmmmmm, 21 bit character
			ASCII7only=0;
			if (std::distance(sx, endx) < 4) {
				rv=0; break;
			}
			if ((sx[1]&(0x80+0x40)) != 0x80 || (sx[2]&(0x80+0x40)) != 0x80 || (sx[3]&(0x80+0x40)) != 0x80) {
				rv=0; break;
			}
			sx+=4;
		} 
		else 
		{
			ASCII7only=0;
			rv=0;
			break;
		}
	}
	if (ASCII7only) 
		return ascii7bits;
	if (rv)
		return utf8NoBOM;
	return ascii8bits;
}

size_t Utf8_16_Read::convert(char* buf, size_t len)
{
	// bugfix by Jens Lorenz
	static	size_t nSkip = 0;

	m_pBuf = (ubyte*)buf;
	m_nLen = len;
	m_nNewBufSize = 0;

	if (m_bFirstRead == true)
    {
		determineEncoding();
		nSkip = m_nSkip;
		m_bFirstRead = false;
	}

    switch (m_eEncoding)
    {
		case uni7Bit:
        case uni8Bit:
        case uniCookie: {
            // Do nothing, pass through
			m_nAllocatedBufSize = 0;
            m_pNewBuf = m_pBuf;
			m_nNewBufSize = len;
            break;
        }
        case uniUTF8: {
            // Pass through after BOM
			m_nAllocatedBufSize = 0;
            m_pNewBuf = m_pBuf + nSkip;
			m_nNewBufSize = len - nSkip;
            break;
        }    
        case uni16BE_NoBOM:
        case uni16LE_NoBOM:
        case uni16BE:
        case uni16LE: {
            size_t newSize = len + len / 2 + 1;
            
			if (m_nAllocatedBufSize != newSize)
            {
				if (m_pNewBuf)
					delete [] m_pNewBuf;
                m_pNewBuf  = NULL;
                m_pNewBuf  = new ubyte[newSize];
				m_nAllocatedBufSize = newSize;
            }
            
            ubyte* pCur = m_pNewBuf;
            
            m_Iter16.set(m_pBuf + nSkip, len - nSkip, m_eEncoding);

			while (m_Iter16)
			{
				++m_Iter16;
				utf8 c;
				while (m_Iter16.get(&c))
					*pCur++ = c;
			}
			m_nNewBufSize = pCur - m_pNewBuf;

            break;
        }
        default:
            break;
    }

	// necessary for second calls and more
	nSkip = 0;

	return m_nNewBufSize;
}


void Utf8_16_Read::determineEncoding()
{
	INT uniTest = IS_TEXT_UNICODE_STATISTICS;
	m_eEncoding = uni8Bit;
	m_nSkip = 0;

    // detect UTF-16 big-endian with BOM
	if (m_nLen > 1 && m_pBuf[0] == k_Boms[uni16BE][0] && m_pBuf[1] == k_Boms[uni16BE][1])
	{
		m_eEncoding = uni16BE;
		m_nSkip = 2;
	}
    // detect UTF-16 little-endian with BOM
	else if (m_nLen > 1 && m_pBuf[0] == k_Boms[uni16LE][0] && m_pBuf[1] == k_Boms[uni16LE][1])
	{
		m_eEncoding = uni16LE;
		m_nSkip = 2;
	}
    // detect UTF-8 with BOM
	else if (m_nLen > 2 && m_pBuf[0] == k_Boms[uniUTF8][0] && 
		m_pBuf[1] == k_Boms[uniUTF8][1] && m_pBuf[2] == k_Boms[uniUTF8][2])
	{
		m_eEncoding = uniUTF8;
		m_nSkip = 3;
	}
	// try to detect UTF-16 little-endian without BOM
	else if (m_nLen > 1 && m_nLen % 2 == 0 && m_pBuf[0] != 0 && m_pBuf[1] == 0 && IsTextUnicode(m_pBuf, static_cast<int32_t>(m_nLen), &uniTest))
	{
		m_eEncoding = uni16LE_NoBOM;
		m_nSkip = 0;
	}
	/* UTF-16 big-endian without BOM detection is taken away scince this detection is very week
    // try to detect UTF-16 big-endian without BOM
    else if (m_nLen > 1 && m_pBuf[0] == NULL && m_pBuf[1] != NULL)
	{
		m_eEncoding = uni16BE_NoBOM;
		m_nSkip = 0;
	}
	*/
	else
	{
		u78 detectedEncoding = utf8_7bits_8bits();
		if (detectedEncoding == utf8NoBOM)
			m_eEncoding = uniCookie;
		else if (detectedEncoding == ascii7bits)
			m_eEncoding = uni7Bit;
		else //(detectedEncoding == ascii8bits)
			m_eEncoding = uni8Bit;
		m_nSkip = 0;
	}
}

UniMode Utf8_16_Read::determineEncoding(const unsigned char *buf, size_t bufLen)
{
    // detect UTF-16 big-endian with BOM
	if (bufLen > 1 && buf[0] == k_Boms[uni16BE][0] && buf[1] == k_Boms[uni16BE][1])
	{
		return uni16BE;
	}
    
    // detect UTF-16 little-endian with BOM
    if (bufLen > 1 && buf[0] == k_Boms[uni16LE][0] && buf[1] == k_Boms[uni16LE][1])
	{
		return uni16LE;
	}
    
    // detect UTF-8 with BOM
	if (bufLen > 2 && buf[0] == k_Boms[uniUTF8][0] && 
		buf[1] == k_Boms[uniUTF8][1] && buf[2] == k_Boms[uniUTF8][2])
	{
		return uniUTF8;
	}

    return uni8Bit;
}


// ==================================================================

Utf8_16_Write::Utf8_16_Write()
{
	m_eEncoding = uni8Bit;
	m_pNewBuf = NULL;
	m_bFirstWrite = true;
	m_nBufSize = 0;
}

Utf8_16_Write::~Utf8_16_Write()
{
	closeFile();
}

bool Utf8_16_Write::openFile(const TCHAR *name)
{
	m_pFile = std::make_unique<Win32_IO_File>(name);

	if (!m_pFile)
		return false;

	if (!m_pFile->isOpened())
	{
		m_pFile = nullptr;
		return false;
	}

	m_bFirstWrite = true;

	return true;
}

bool Utf8_16_Write::writeFile(const void* p, unsigned long _size)
{
    // no file open
	if (!m_pFile)
    {
		return false;
	}

	if (m_bFirstWrite)
    {
        switch (m_eEncoding)
        {
            case uniUTF8: {
                if (!m_pFile->write(k_Boms[m_eEncoding], 3))
					return false;
                break;
            }    
            case uni16BE:
            case uni16LE:
                if (!m_pFile->write(k_Boms[m_eEncoding], 2))
					return false;
                break;
            default:
                // nothing to do
                break;
        }
		m_bFirstWrite = false;
    }

    bool isOK = false;

    switch (m_eEncoding)
    {
		case uni7Bit:
        case uni8Bit:
        case uniCookie:
        case uniUTF8: {
            // Normal write
			if (m_pFile->write(p, _size))
				isOK = true;
            break;
        }
        case uni16BE_NoBOM:
        case uni16LE_NoBOM:
        case uni16BE:
        case uni16LE: {
			static const unsigned int bufSize = 64*1024;
			utf16* buf = new utf16[bufSize];
            
            Utf8_Iter iter8;
            iter8.set(static_cast<const ubyte*>(p), _size, m_eEncoding);

			unsigned int bufIndex = 0;
			while (iter8) {
				++iter8;
				while ((bufIndex < bufSize) && iter8.canGet())
					iter8.get(&buf [bufIndex++]);

				if (bufIndex == bufSize || !iter8) {
					if (!m_pFile->write(buf, bufIndex*sizeof(utf16))) return 0;
					bufIndex = 0;
				}
			}
			isOK = true;
			delete[] buf;
            break;
        }    
        default:
            break;
    }

    return isOK;
}


size_t Utf8_16_Write::convert(char* p, size_t _size)
{
	if (m_pNewBuf)
    {
		delete [] m_pNewBuf;
		m_pNewBuf = NULL;
	}

    switch (m_eEncoding)
    {
		case uni7Bit:
        case uni8Bit:
        case uniCookie: {
            // Normal write
            m_nBufSize = _size;
            m_pNewBuf = (ubyte*)new ubyte[m_nBufSize];
            memcpy(m_pNewBuf, p, _size);
            break;
        }
        case uniUTF8: {
            m_nBufSize = _size + 3;
            m_pNewBuf = (ubyte*)new ubyte[m_nBufSize];
            memcpy(m_pNewBuf, k_Boms[m_eEncoding], 3);
            memcpy(&m_pNewBuf[3], p, _size);
            break;
        }
        case uni16BE_NoBOM:
        case uni16LE_NoBOM:
        case uni16BE:
        case uni16LE:
		{
			utf16* pCur = NULL;
            
            if (m_eEncoding == uni16BE || m_eEncoding == uni16LE) {
                // Write the BOM
				m_pNewBuf = (ubyte*)new ubyte[sizeof(utf16) * (_size + 1)];
                memcpy(m_pNewBuf, k_Boms[m_eEncoding], 2);
	            pCur = (utf16*)&m_pNewBuf[2];
            } else {
				m_pNewBuf = (ubyte*)new ubyte[sizeof(utf16) * _size];
	            pCur = (utf16*)m_pNewBuf;
			}

            Utf8_Iter iter8;
            iter8.set(reinterpret_cast<const ubyte*>(p), _size, m_eEncoding);
            
            for (; iter8; ++iter8) {
                if (iter8.canGet()) {
                    iter8.get(pCur++);
                }
            }
            m_nBufSize = (const char*)pCur - (const char*)m_pNewBuf;
			break;
        }
        default:
            break;
    }
    
	return m_nBufSize;
}


void Utf8_16_Write::setEncoding(UniMode eType)
{
	m_eEncoding = eType;
}


void Utf8_16_Write::closeFile()
{
	if (m_pNewBuf)
	{
		delete [] m_pNewBuf;
		m_pNewBuf = NULL;
	}

	if (m_pFile)
		m_pFile = nullptr;
}


//=================================================================
Utf8_Iter::Utf8_Iter()
{
	reset();
}

void Utf8_Iter::reset()
{
	m_pBuf = NULL;
	m_pRead = NULL;
	m_pEnd = NULL;
	m_eState = eStart;
	m_out1st = 0;
	m_outLst = 0;
	m_eEncoding = uni8Bit;
}

void Utf8_Iter::set(const ubyte* pBuf, size_t nLen, UniMode eEncoding)
{
	m_pBuf      = pBuf;
	m_pRead     = pBuf;
	m_pEnd      = pBuf + nLen;
	m_eEncoding = eEncoding;
	// Note: m_eState, m_nCur not set
}

bool Utf8_Iter::get(utf16* c)
{
#ifdef _DEBUG
	assert(m_out1st != m_outLst);
#endif
	if (m_out1st == m_outLst) return false;
	*c = m_out [m_out1st];
	m_out1st = (m_out1st + 1) % _countof (m_out);
	return true;
}

// Go to the next byte.
void Utf8_Iter::operator++()
{
	if (m_out1st != m_outLst) return;
	switch (m_eState)
    {
        case eStart:
            if (*m_pRead < 0x80) {
                m_code = *m_pRead;
				toStart();
            } else if (*m_pRead < 0xE0) {
                m_code = static_cast<utf16>(0x1f & *m_pRead);
                m_eState = eFollow;
				m_count = 1;
			} else if (*m_pRead < 0xF0) {
				m_code = static_cast<utf16>(0x0f & *m_pRead);
				m_eState = eFollow;
				m_count = 2;
            } else {
                m_code = static_cast<utf16>(0x07 & *m_pRead);
                m_eState = eFollow;
				m_count = 3;
            }
            break;

        case eFollow:
            m_code = (m_code << 6) | static_cast<utf8>(0x3F & *m_pRead);
			m_count--;
			if (m_count == 0)
				toStart();
            break;
	}
	++m_pRead;
}

void Utf8_Iter::toStart()
{
	bool swap = (m_eEncoding == uni16BE || m_eEncoding == uni16BE_NoBOM);
	if (m_code < 0x10000)
	{
		utf16 c = swap ? _byteswap_ushort((utf16)m_code) : (utf16)m_code;
		pushout  (c);
	}
	else
	{
		m_code -= 0x10000;
		utf16 c1 = (utf16)(0xD800 | (m_code >> 10));
		utf16 c2 = (utf16)(0xDC00 | (m_code & 0x3ff));
		if (swap)
		{
			c1 = _byteswap_ushort(c1);
			c2 = _byteswap_ushort(c2);
		}
		pushout(c1);
		pushout(c2);
	}
	m_eState = eStart;
}

void Utf8_Iter::pushout(utf16 c)
{
	m_out [m_outLst] = c;
	m_outLst = (m_outLst + 1) % _countof(m_out);
}

//==================================================
Utf16_Iter::Utf16_Iter()
{
	reset();
}

void Utf16_Iter::reset()
{
	m_pBuf = NULL;
	m_pRead = NULL;
	m_pEnd = NULL;
	m_eState = eStart;
	m_out1st = 0;
	m_outLst = 0;
	m_nCur16 = 0;
	m_eEncoding = uni8Bit;
}

bool Utf16_Iter::get(utf8 *c)
{ 
	if (m_out1st != m_outLst)
	{
		*c = m_out [m_out1st];
		m_out1st = (m_out1st + 1) % _countof(m_out);
		return true;
	}
	return false;
};

void Utf16_Iter::pushout(ubyte c)
{
	m_out [m_outLst] = c;
	m_outLst = (m_outLst + 1) % _countof(m_out);
}

void Utf16_Iter::set(const ubyte* pBuf, size_t nLen, UniMode eEncoding)
{
	m_pBuf = pBuf;
	m_pRead = pBuf;
	m_pEnd = pBuf + nLen;
	m_eEncoding = eEncoding;
	// Note: m_eState, m_out*, m_nCur16 not reinitalized.
}

void Utf16_Iter::read()
{
    if (m_eEncoding == uni16LE || m_eEncoding == uni16LE_NoBOM) 
    {
        m_nCur16 = *m_pRead++;
        m_nCur16 |= static_cast<utf16>(*m_pRead << 8);
    }
    else //(m_eEncoding == uni16BE || m_eEncoding == uni16BE_NoBOM)
    {
        m_nCur16 = static_cast<utf16>(*m_pRead++ << 8);
        m_nCur16 |= *m_pRead;
    }
    ++m_pRead;
}

// Goes to the next byte.
// Not the next symbol which you might expect.
// This way we can continue from a partial buffer that doesn't align
void Utf16_Iter::operator++()
{
	if (m_out1st != m_outLst) return;
	switch (m_eState)
	{
        case eStart:
			read();
			if ((m_nCur16 >= 0xd800) && (m_nCur16 < 0xdc00)) {
				m_eState = eSurrogate;
				m_highSurrogate = m_nCur16;
			}
            else if (m_nCur16 < 0x80) {
                pushout(static_cast<ubyte>(m_nCur16));
                m_eState = eStart;
            } else if (m_nCur16 < 0x800) {
                pushout(static_cast<ubyte>(0xC0 | m_nCur16 >> 6));
                pushout(0x80 | m_nCur16 & 0x3f);
                m_eState = eStart;
            } else {
                pushout(0xE0 | (m_nCur16 >> 12));
                pushout(0x80 | (m_nCur16 >> 6) & 0x3f);
                pushout(0x80 | m_nCur16 & 0x3f);
                m_eState = eStart;
            }
            break;
		case eSurrogate:
			read();
			if ((m_nCur16 >= 0xDC00) && (m_nCur16 < 0xE000))
			{ // valid surrogate pair
				UINT code = 0x10000 + ((m_highSurrogate & 0x3ff) << 10) + (m_nCur16 & 0x3ff);
				pushout(0xf0 | (code >> 18) & 0x07);
				pushout(0x80 | (code >> 12) & 0x3f);
				pushout(0x80 | (code >>  6) & 0x3f);
				pushout(0x80 | code & 0x3f);
			}
			m_eState = eStart;
			break;
    }
}
