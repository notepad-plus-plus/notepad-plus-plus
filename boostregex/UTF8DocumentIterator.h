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

#ifndef UTF8DOCUMENTITERATOR_H_3452843291318441149
#define UTF8DOCUMENTITERATOR_H_3452843291318441149

#include <stdlib.h>
#include <vector>
#include <memory>
#include "Position.h"

namespace Scintilla::Internal {

class Document;

class UTF8DocumentIterator
{
public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = wchar_t;
	using difference_type = ptrdiff_t;
	using pointer = wchar_t*;
	using reference = wchar_t&;

	UTF8DocumentIterator() {};

	UTF8DocumentIterator(Document* doc, Sci::Position pos, Sci::Position end);
	UTF8DocumentIterator(const UTF8DocumentIterator& copy);

	bool operator == (const UTF8DocumentIterator& other) const
	{
		return (ended() == other.ended()) && (m_doc == other.m_doc) && (m_pos == other.m_pos);
	}

	bool operator != (const UTF8DocumentIterator& other) const
	{
		return !(*this == other);
	}

	wchar_t operator * () const
	{
		return m_character[m_characterIndex];
	}

	UTF8DocumentIterator& operator = (Sci::Position other)
	{
		m_pos = other;
		return *this;
	}

	UTF8DocumentIterator& operator = (const UTF8DocumentIterator& other) = default;

	UTF8DocumentIterator& operator ++ ();
	UTF8DocumentIterator& operator -- ();

	Sci::Position pos() const
	{
		return m_pos;
	}

private:
	void readCharacter();


	bool ended() const
	{
		return m_pos >= m_end;
	}

	Sci::Position m_pos = 0;
	wchar_t m_character[2]{};
	Sci::Position m_end = 0;
	int m_characterIndex = 0;
	int m_utf8Length = 0;
	int m_utf16Length = 0;
	Document* m_doc = nullptr;
	static const unsigned char m_firstByteMask[];
};

}

#endif // UTF8DOCUMENTITERATOR_H_3452843291318441149
