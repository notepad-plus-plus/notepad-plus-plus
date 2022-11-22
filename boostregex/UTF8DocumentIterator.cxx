// This file is part of Notepad++ project
// Copyright (C) 2021 Notepad++ authors.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "UTF8DocumentIterator.h"
#include <string_view>
#include <stdexcept>
#include <optional>

#include "ILoader.h"
#include "ILexer.h"
#include "Scintilla.h"
#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"

#include "CharacterCategoryMap.h"
#include "Position.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "CellBuffer.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"

using namespace Scintilla::Internal;

UTF8DocumentIterator::UTF8DocumentIterator(Document* doc, Sci::Position pos, Sci::Position end) :
				m_pos(pos),
				m_end(end),
				m_characterIndex(0),
				m_doc(doc)
{
		// Check for debug builds
		PLATFORM_ASSERT(m_pos <= m_end);

		// Ensure for release.
		if (m_pos > m_end)
		{
				m_pos = m_end;
		}
		readCharacter();
}

UTF8DocumentIterator::UTF8DocumentIterator(const UTF8DocumentIterator& copy) :
		m_pos(copy.m_pos),
		m_end(copy.m_end),
		m_characterIndex(copy.m_characterIndex),
		m_utf8Length(copy.m_utf8Length),
		m_utf16Length(copy.m_utf16Length),
		m_doc(copy.m_doc)
{
		// Check for debug builds
		PLATFORM_ASSERT(m_pos <= m_end);
		m_character[0] = copy.m_character[0];
		m_character[1] = copy.m_character[1];

		// Ensure for release.
		if (m_pos > m_end)
		{
				m_pos = m_end;
		}
}

UTF8DocumentIterator& UTF8DocumentIterator::operator ++ ()
{
	PLATFORM_ASSERT(m_pos < m_end);
	if (m_utf16Length == 2 && m_characterIndex == 0)
	{
		m_characterIndex = 1;
	}
	else
	{
		m_pos += m_utf8Length;

		if (m_pos > m_end)
		{
			m_pos = m_end;
		}
		m_characterIndex = 0;
		readCharacter();
	}
	return *this;
}

UTF8DocumentIterator& UTF8DocumentIterator::operator -- ()
{
	if (m_utf16Length == 2 && m_characterIndex == 1)
	{
		m_characterIndex = 0;
	}
	else
	{
		--m_pos;
		// Skip past the UTF-8 extension bytes
		while (0x80 == (m_doc->CharAt(m_pos) & 0xC0) && m_pos > 0)
			--m_pos;

		readCharacter();
		if (m_utf16Length == 2)
		{
			m_characterIndex = 1;
		}
	}
	return *this;
}

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
		Sci::Position pos = m_pos;
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
			m_character[0] = static_cast<wchar_t>(0xD800 + (result >> 10));
			m_character[1] = static_cast<wchar_t>(0xDC00 + (result & 0x3FF));

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
