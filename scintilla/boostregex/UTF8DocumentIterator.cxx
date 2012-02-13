#include "UTF8DocumentIterator.h"


void UTF8DocumentIterator::readCharacter()
{
	unsigned char currentChar = m_doc->CharAt(m_pos);
	if (currentChar & 0x80)
	{
		int mask = 0x40;
		int nBytes = 1;
			
		do 
		{
			mask >>= 1;
			++nBytes;
		} while (currentChar & mask);

		int result = currentChar & m_firstByteMask[nBytes];
		int pos = m_pos;
		m_utf8Length = 1;
		// work out the unicode point, and count the actual bytes.
		// If a byte does not start with 10xxxxxx then it's not part of the 
		// the code. Therefore invalid UTF-8 encodings are dealt with, simply by stopping when 
		// the UTF8 extension bytes are no longer valid.
		while ((--nBytes) && (pos < m_end) && (0x80 == ((currentChar = m_doc->CharAt(++pos)) & 0xC0)))
		{
			result = (result << 6) | (currentChar & 0x3F);
			++m_utf8Length;
		}

		if (result >= 0x10000)
		{
			result -= 0x10000;
			m_utf16Length = 2;
			// UTF-16 Pair
			m_character[0] = 0xD800 + (result >> 10);
			m_character[1] = 0xDC00 + (result & 0x3FF);
				
		}
		else
		{
			m_utf16Length = 1;
			m_character[0] = static_cast<wchar_t>(result);
		}
	}
	else
	{
		m_utf8Length = 1;
		m_utf16Length = 1;
		m_characterIndex = 0;
		m_character[0] = static_cast<wchar_t>(currentChar);
	}
}
		
		
const unsigned char UTF8DocumentIterator::m_firstByteMask[7] = { 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01 };
		