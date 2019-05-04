#ifndef UTF8DOCUMENTITERATOR_H_3452843291318441149
#define UTF8DOCUMENTITERATOR_H_3452843291318441149

#include <stdlib.h>
#include <iterator>
#include <vector>
#include "Platform.h"

namespace Scintilla {

class Document;

class UTF8DocumentIterator : public std::iterator<std::bidirectional_iterator_tag, wchar_t>
{
public:
        UTF8DocumentIterator() : 
                m_doc(0), 
                m_pos(0),
                m_end(0),
				m_characterIndex(0),
				m_utf8Length(0),
				m_utf16Length(0)
        {
        }

        UTF8DocumentIterator(Document* doc, int pos, int end);
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

		UTF8DocumentIterator& operator = (int other)
		{
			m_pos = other;
			return *this;
		}

        UTF8DocumentIterator& operator ++ ()
        {
                PLATFORM_ASSERT(m_pos < m_end);
				if (2 == m_utf16Length && 0 == m_characterIndex)
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

        UTF8DocumentIterator& operator -- ();

        int pos() const
        {
                return m_pos;
        }

private:
		void readCharacter();


        bool ended() const
        {
                return m_pos >= m_end;
        }

        int m_pos;
		wchar_t m_character[2];
		int m_characterIndex;
        int m_end;
		int m_utf8Length;
		int m_utf16Length;
        Document* m_doc;
		static const unsigned char m_firstByteMask[];
};

}

#endif // UTF8DOCUMENTITERATOR_H_3452843291318441149
