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

#ifndef ANSIDOCUMENTITERATOR_H_12481491281240
#define ANSIDOCUMENTITERATOR_H_12481491281240

#include "Position.h"

namespace Scintilla::Internal {

class AnsiDocumentIterator
{
public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = char;
	using difference_type = ptrdiff_t;
	using pointer = char*;
	using reference = char&;

	AnsiDocumentIterator() {};

	AnsiDocumentIterator(Document* doc, Sci::Position pos, Sci::Position end) :
		m_pos(pos),
		m_end(end),
		m_doc(doc)
	{
		// Check for debug builds
		PLATFORM_ASSERT(m_pos <= m_end);

		// Ensure for release.
		if (m_pos > m_end)
		{
			m_pos = m_end;
		}
	}

	AnsiDocumentIterator(const AnsiDocumentIterator& copy) :
		m_pos(copy.m_pos),
		m_end(copy.m_end),
		m_doc(copy.m_doc)

	{
		// Check for debug builds
		PLATFORM_ASSERT(m_pos <= m_end);

		// Ensure for release.
		if (m_pos > m_end)
		{
			m_pos = m_end;
		}
	}

	bool operator == (const AnsiDocumentIterator& other) const
	{
		return (ended() == other.ended()) && (m_doc == other.m_doc) && (m_pos == other.m_pos);
	}

	bool operator != (const AnsiDocumentIterator& other) const
	{
		return !(*this == other);
	}

	char operator * () const
	{
		return charAt(m_pos);
	}

	AnsiDocumentIterator& operator = (const AnsiDocumentIterator& other) = default;

	AnsiDocumentIterator& operator ++ ()
	{
		PLATFORM_ASSERT(m_pos < m_end);

		m_pos++;
		return *this;
	}

	AnsiDocumentIterator& operator -- ()
	{
		m_pos--;
		return *this;
	}

	Sci::Position pos() const
	{
		return m_pos;
	}

private:
	char charAt(Sci::Position position) const
	{
		return m_doc->CharAt(position);
	}

	bool ended() const
	{
		return m_pos == m_end;
	}

	Sci::Position m_pos = 0;
	Sci::Position m_end = 0;
	Document* m_doc = nullptr;
};
}

#endif