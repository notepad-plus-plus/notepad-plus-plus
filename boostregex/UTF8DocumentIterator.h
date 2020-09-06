#ifndef UTF8DOCUMENTITERATOR_H_3452843291318441149
#define UTF8DOCUMENTITERATOR_H_3452843291318441149

#include <stdlib.h>
#include <iterator>
#include <vector>
#include <memory>
#include "Position.h"

namespace Scintilla {

class Document;

class UTF8DocumentIterator : public std::iterator<std::bidirectional_iterator_tag, wchar_t>
{
public:
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
		wchar_t m_character[2];
		Sci::Position m_end = 0;
		int m_characterIndex = 0;
		int m_utf8Length = 0;
		int m_utf16Length = 0;
		Document* m_doc = nullptr;
		static const unsigned char m_firstByteMask[];
};

}

#endif // UTF8DOCUMENTITERATOR_H_3452843291318441149
