/*
www.sourceforge.net/projects/tinyxml
Original code (2.0 and earlier )copyright (c) 2000-2002 Lee Thomason (www.grinninglizard.com)

This software is provided 'as-is', without any express or implied 
warranty. In no event will the authors be held liable for any 
damages arising from the use of this software.

Permission is granted to anyone to use this software for any 
purpose, including commercial applications, and to alter it and 
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must 
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and 
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source 
distribution.
*/


#include <sstream>
#include "tinyxml.h"

//#define DEBUG_PARSER

// Note tha "PutString" hardcodes the same list. This
// is less flexible than it appears. Changing the entries
// or order will break putstring.	
TiXmlBase::Entity TiXmlBase::entity[ NUM_ENTITY ] = 
{
	{ L"&amp;",  5, '&' },
	{ L"&lt;",   4, '<' },
	{ L"&gt;",   4, '>' },
	{ L"&quot;", 6, '\"' },
	{ L"&apos;", 6, '\'' }
};


class TiXmlParsingData
{
	friend class TiXmlDocument;
  public:
	//TiXmlParsingData( const wchar_t* now, const TiXmlParsingData* prevData );
	void Stamp( const wchar_t* now );

	const TiXmlCursor& Cursor()	{ return cursor; }
	//void Update( const wchar_t* now );

  private:
	// Only used by the document!
	TiXmlParsingData( const wchar_t* start, int _tabsize, int row, int col )
	{
		assert( start );
		stamp = start;
		tabsize = _tabsize;
		cursor.row = row;
		cursor.col = col;
	}

	TiXmlCursor		cursor;
	const wchar_t*		stamp;
	int				tabsize;
};


void TiXmlParsingData::Stamp( const wchar_t* now )
{
	assert( now );

	// Do nothing if the tabsize is 0.
	if ( tabsize < 1 )
	{
		return;
	}

	// Get the current row, column.
	int row = cursor.row;
	int col = cursor.col;
	const wchar_t* p = stamp;
	assert( p );

	while ( p < now )
	{
		// Code contributed by Fletcher Dunn: (modified by lee)
		switch (*p) {
			case 0:
				// We *should* never get here, but in case we do, don't
				// advance past the terminating null character, ever
				return;

			case '\r':
				// bump down to the next line
				++row;
				col = 0;				
				// Eat the character
				++p;

				// Check for \r\n sequence, and treat this as a single character
				if (*p == '\n') {
					++p;
				}
				break;

			case '\n':
				// bump down to the next line
				++row;
				col = 0;

				// Eat the character
				++p;

				// Check for \n\r sequence, and treat this as a single
				// character.  (Yes, this bizarre thing does occur still
				// on some arcane platforms...)
				if (*p == '\r') {
					++p;
				}
				break;

			case '\t':
				// Eat the character
				++p;

				// Skip to next tab stop
				col = (col / tabsize + 1) * tabsize;
				break;

			default:
				// Eat the character
				++p;

				// Normal wchar_t - just advance one column
				++col;
				break;
		}
	}
	cursor.row = row;
	cursor.col = col;
	assert( cursor.row >= -1 );
	assert( cursor.col >= -1 );
	stamp = p;
	assert( stamp );
}


const wchar_t* TiXmlBase::SkipWhiteSpace( const wchar_t* p )
{
	if ( !p || !*p )
	{
		return 0;
	}
	while ( p && *p )
	{
		if ( _istspace( *p ) || *p == '\n' || *p =='\r' )		// Still using old rules for white space.
			++p;
		else
			break;
	}

	return p;
}

#ifdef TIXML_USE_STL
/*static*/ bool TiXmlBase::StreamWhiteSpace( TIXML_ISTREAM * in, TIXML_STRING * tag )
{
	for ( ;; )
	{
		if ( !in->good() ) return false;

		int c = in->peek();
		if ( !IsWhiteSpace( c ) )
			return true;
		*tag += static_cast<wchar_t>(in->get());
	}
}

/*static*/ bool TiXmlBase::StreamTo( TIXML_ISTREAM * in, int character, TIXML_STRING * tag )
{
	while ( in->good() )
	{
		int c = in->peek();
		if ( c == character )
			return true;

		in->get();
		*tag += static_cast<wchar_t>(c);
	}
	return false;
}
#endif

const wchar_t* TiXmlBase::ReadName( const wchar_t* p, TIXML_STRING * name )
{
	*name = L"";
	assert( p );

	// Names start with letters or underscores.
	// After that, they can be letters, underscores, numbers,
	// hyphens, or colons. (Colons are valid ony for namespaces,
	// but tinyxml can't tell namespaces from names.)
	if (    p && *p 
		 && ( _istalpha( *p ) || *p == '_' ) )
	{
		while(		p && *p
				&&	(		_istalnum( *p ) 
						 || *p == '_'
						 || *p == '-'
						 || *p == '.'
						 || *p == ':' ) )
		{
			(*name) += *p;
			++p;
		}
		return p;
	}
	return 0;
}

const wchar_t* TiXmlBase::GetEntity( const wchar_t* p, wchar_t* value )
{
	// Presume an entity, and pull it out.
	int i;

	// Handle the &#x entities.
	if (wcsncmp( L"&#x", p, 3 ) == 0)
	{
		const wchar_t* end = wcschr(p+3, L';');
		if (end && end - p <= 3 + 4)
		{
			wchar_t* hexend;
			auto val = wcstol(p + 3, &hexend, 16);
			if (hexend == end)
			{
				*value = static_cast<wchar_t>(val);
				return end + 1;
			}
		}
	}

	// Now try to match it.
	for (i=0; i<NUM_ENTITY; ++i)
	{
		if (wcsncmp( entity[i].str, p, entity[i].strLength ) == 0 )
		{
			assert(static_cast<unsigned int>(lstrlen(entity[i].str)) == entity[i].strLength );
			*value = entity[i].chr;
			return (p + entity[i].strLength);
		}
	}

	// So it wasn't an entity, its unrecognized, or something like that.
	*value = *p;	// Don't put back the last one, since we return it!
	return p+1;
}


bool TiXmlBase::StringEqual( const wchar_t* p,
							 const wchar_t* tag,
							 bool ignoreCase )
{
	assert( p );
	if ( !p || !*p )
	{
		assert( 0 );
		return false;
	}

    if ( _totlower( *p ) == _totlower( *tag ) )
	{
		const wchar_t* q = p;

		if (ignoreCase)
		{
			while ( *q && *tag && *q == *tag )
			{
				++q;
				++tag;
			}

			if ( *tag == 0 )		// Have we found the end of the tag, and everything equal?
			{
				return true;
			}
		}
		else
		{
			while ( *q && *tag && _totlower( *q ) == _totlower( *tag ) )
			{
				++q;
				++tag;
			}

			if ( *tag == 0 )
			{
				return true;
			}
		}
	}
	return false;
}

const wchar_t* TiXmlBase::ReadText(	const wchar_t* p, 
									TIXML_STRING * text, 
									bool trimWhiteSpace, 
									const wchar_t* endTag, 
									bool caseInsensitive )
{
    *text = L"";
	if (    !trimWhiteSpace			// certain tags always keep whitespace
		 || !condenseWhiteSpace )	// if true, whitespace is always kept
	{
		// Keep all the white space.
		while (	   p && *p
				&& !StringEqual( p, endTag, caseInsensitive )
			  )
		{
			wchar_t c;
			p = GetChar( p, &c );
            (* text) += c;
		}
	}
	else
	{
		bool whitespace = false;

		// Remove leading white space:
		p = SkipWhiteSpace( p );
		while (	   p && *p
				&& !StringEqual( p, endTag, caseInsensitive ) )
		{
			if ( *p == '\r' || *p == '\n' )
			{
				whitespace = true;
				++p;
			}
			else if ( _istspace( *p ) )
			{
				whitespace = true;
				++p;
			}
			else
			{
				// If we've found whitespace, add it before the
				// new character. Any whitespace just becomes a space.
				if ( whitespace )
				{
               (* text) += ' ';
					whitespace = false;
				}
				wchar_t c;
				p = GetChar( p, &c );
            (* text) += c;
			}
		}
	}
	return p + lstrlen( endTag );
}

#ifdef TIXML_USE_STL

void TiXmlDocument::StreamIn( TIXML_ISTREAM * in, TIXML_STRING * tag )
{
	// The basic issue with a document is that we don't know what we're
	// streaming. Read something presumed to be a tag (and hope), then
	// identify it, and call the appropriate stream method on the tag.
	//
	// This "pre-streaming" will never read the closing ">" so the
	// sub-tag can orient itself.

	if ( !StreamTo( in, '<', tag ) ) 
	{
		SetError( TIXML_ERROR_PARSING_EMPTY, 0, 0 );
		return;
	}

	while ( in->good() )
	{
		size_t tagIndex = tag->length();
		while ( in->good() && in->peek() != '>' )
		{
			int c = in->get();
			(*tag) += static_cast<wchar_t>(c);
		}

		if ( in->good() )
		{
			// We now have something we presume to be a node of 
			// some sort. Identify it, and call the node to
			// continue streaming.
			TiXmlNode* node = Identify( tag->c_str() + tagIndex );

			if ( node )
			{
				node->StreamIn( in, tag );
				bool isElement = node->ToElement() != 0;
				delete node;
				node = 0;

				// If this is the root element, we're done. Parsing will be
				// done by the >> operator.
				if ( isElement )
				{
					return;
				}
			}
			else
			{
				SetError( TIXML_ERROR, 0, 0 );
				return;
			}
		}
	}
	// We should have returned sooner.
	SetError( TIXML_ERROR, 0, 0 );
}

#endif

const wchar_t* TiXmlDocument::Parse( const wchar_t* p, TiXmlParsingData* prevData )
{
	ClearError();

	// Parse away, at the document level. Since a document
	// contains nothing but other tags, most of what happens
	// here is skipping white space.
	if ( !p || !*p )
	{
		SetError( TIXML_ERROR_DOCUMENT_EMPTY, 0, 0 );
		return 0;
	}

	// Note that, for a document, this needs to come
	// before the while space skip, so that parsing
	// starts from the pointer we are given.
	location.Clear();
	if ( prevData )
	{
		location.row = prevData->cursor.row;
		location.col = prevData->cursor.col;
	}
	else
	{
		location.row = 0;
		location.col = 0;
	}
	TiXmlParsingData data( p, TabSize(), location.row, location.col );
	location = data.Cursor();

    p = SkipWhiteSpace( p );
	if ( !p )
	{
		SetError( TIXML_ERROR_DOCUMENT_EMPTY, 0, 0 );
		return 0;
	}

	while ( p && *p )
	{
		TiXmlNode* node = Identify( p );
		if ( node )
		{
			p = node->Parse( p, &data );
			LinkEndChild( node );
		}
		else
		{
			break;
		}
		p = SkipWhiteSpace( p );
	}

	// All is well.
	return p;
}

void TiXmlDocument::SetError( int err, const wchar_t* pError, TiXmlParsingData* data )
{	
	// The first error in a chain is more accurate - don't set again!
	if ( error )
		return;

	assert( err > 0 && err < TIXML_ERROR_STRING_COUNT );
	error   = true;
	errorId = err;
	errorDesc = errorString[ errorId ];

	errorLocation.Clear();
	if ( pError && data )
	{
		//TiXmlParsingData data( pError, prevData );
		data->Stamp( pError );
		errorLocation = data->Cursor();
	}
}


TiXmlNode* TiXmlNode::Identify( const wchar_t* p )
{
	TiXmlNode* returnNode = 0;

	p = SkipWhiteSpace( p );
	if ( !p || !*p || *p != '<' )
	{
		return 0;
	}

	TiXmlDocument* doc = GetDocument();
	p = SkipWhiteSpace( p );

	if ( !p || !*p )
	{
		return 0;
	}

	// What is this thing? 
	// - Elements start with a letter or underscore, but xml is reserved.
	// - Comments: <!--
	// - Decleration: <?xml
	// - Everthing else is unknown to tinyxml.
	//

	const wchar_t* xmlHeader = { L"<?xml" };
	const wchar_t* commentHeader = { L"<!--" };

	if ( StringEqual( p, xmlHeader, true ) )
	{
		#ifdef DEBUG_PARSER
			TIXML_LOG( "XML parsing Declaration\n" );
		#endif
		returnNode = new TiXmlDeclaration();
	}
	else if (    _istalpha( *(p+1) )
			  || *(p+1) == '_' )
	{
		#ifdef DEBUG_PARSER
			TIXML_LOG( "XML parsing Element\n" );
		#endif
		returnNode = new TiXmlElement( L"" );
	}
	else if ( StringEqual( p, commentHeader, false ) )
	{
		#ifdef DEBUG_PARSER
			TIXML_LOG( "XML parsing Comment\n" );
		#endif
		returnNode = new TiXmlComment();
	}
	else
	{
		#ifdef DEBUG_PARSER
			TIXML_LOG( "XML parsing Unknown\n" );
		#endif
		returnNode = new TiXmlUnknown();
	}

	if ( returnNode )
	{
		// Set the parent, so it can report errors
		returnNode->parent = this;
	}
	else
	{
		if ( doc )
			doc->SetError( TIXML_ERROR_OUT_OF_MEMORY, 0, 0 );
	}
	return returnNode;
}

#ifdef TIXML_USE_STL

void TiXmlElement::StreamIn (TIXML_ISTREAM * in, TIXML_STRING * tag)
{
	// We're called with some amount of pre-parsing. That is, some of "this"
	// element is in "tag". Go ahead and stream to the closing ">"
	while( in->good() )
	{
		int c = in->get();
		(*tag) += static_cast<wchar_t>(c);
		
		if ( c == '>' )
			break;
	}

	if ( tag->length() < 3 ) return;

	// Okay...if we are a "/>" tag, then we're done. We've read a complete tag.
	// If not, identify and stream.

	if (    tag->at( tag->length() - 1 ) == '>' 
		 && tag->at( tag->length() - 2 ) == '/' )
	{
		// All good!
		return;
	}
	else if ( tag->at( tag->length() - 1 ) == '>' )
	{
		// There is more. Could be:
		//		text
		//		closing tag
		//		another node.
		for ( ;; )
		{
			StreamWhiteSpace( in, tag );

			// Do we have text?
			if ( in->good() && in->peek() != '<' ) 
			{
				// Yep, text.
				TiXmlText text( L"" );
				text.StreamIn( in, tag );

				// What follows text is a closing tag or another node.
				// Go around again and figure it out.
				continue;
			}

			// We now have either a closing tag...or another node.
			// We should be at a "<", regardless.
			if ( !in->good() ) return;
			assert( in->peek() == '<' );
			size_t tagIndex = tag->length();

			bool closingTag = false;
			bool firstCharFound = false;

			for ( ;; )
			{
				if ( !in->good() )
					return;

				int c = in->peek();
				
				if ( c == '>' )
					break;

				*tag += static_cast<wchar_t>(c);
				in->get();

				if ( !firstCharFound && c != '<' && !IsWhiteSpace( c ) )
				{
					firstCharFound = true;
					if ( c == '/' )
						closingTag = true;
				}
			}
			// If it was a closing tag, then read in the closing '>' to clean up the input stream.
			// If it was not, the streaming will be done by the tag.
			if ( closingTag )
			{
				int c = in->get();
				assert( c == '>' );
				*tag += static_cast<wchar_t>(c);

				// We are done, once we've found our closing tag.
				return;
			}
			else
			{
				// If not a closing tag, id it, and stream.
				const wchar_t* tagloc = tag->c_str() + tagIndex;
				TiXmlNode* node = Identify( tagloc );
				if ( !node )
					return;
				node->StreamIn( in, tag );
				delete node;
				node = 0;

				// No return: go around from the beginning: text, closing tag, or node.
			}
		}
	}
}
#endif

const wchar_t* TiXmlElement::Parse( const wchar_t* p, TiXmlParsingData* data )
{
	p = SkipWhiteSpace( p );
	TiXmlDocument* document = GetDocument();

	if ( !p || !*p )
	{
		if ( document ) document->SetError( TIXML_ERROR_PARSING_ELEMENT, 0, 0 );
		return 0;
	}

//	TiXmlParsingData data( p, prevData );
	if ( data )
	{
		data->Stamp( p );
		location = data->Cursor();
	}

	if ( *p != '<' )
	{
		if ( document ) document->SetError( TIXML_ERROR_PARSING_ELEMENT, p, data );
		return 0;
	}

	p = SkipWhiteSpace( p+1 );

	// Read the name.
	const wchar_t* pErr = p;

    p = ReadName( p, &value );
	if ( !p || !*p )
	{
		if ( document )	document->SetError( TIXML_ERROR_FAILED_TO_READ_ELEMENT_NAME, pErr, data );
		return 0;
	}

    TIXML_STRING endTag (L"</");
	endTag += value;
	endTag += L">";

	// Check for and read attributes. Also look for an empty
	// tag or an end tag.
	while ( p && *p )
	{
		pErr = p;
		p = SkipWhiteSpace( p );
		if ( !p || !*p )
		{
			if ( document ) document->SetError( TIXML_ERROR_READING_ATTRIBUTES, pErr, data );
			return 0;
		}
		if ( *p == '/' )
		{
			++p;
			// Empty tag.
			if ( *p  != '>' )
			{
				if ( document ) document->SetError( TIXML_ERROR_PARSING_EMPTY, p, data );		
				return 0;
			}
			return (p+1);
		}
		else if ( *p == '>' )
		{
			// Done with attributes (if there were any.)
			// Read the value -- which can include other
			// elements -- read the end tag, and return.
			++p;
			p = ReadValue( p, data );		// Note this is an Element method, and will set the error if one happens.
			if ( !p || !*p )
				return 0;

			// We should find the end tag now
			if ( StringEqual( p, endTag.c_str(), false ) )
			{
				p += endTag.length();
				return p;
			}
			else
			{
				if ( document ) document->SetError( TIXML_ERROR_READING_END_TAG, p, data );
				return 0;
			}
		}
		else
		{
			// Try to read an attribute:
			TiXmlAttribute* attrib = new TiXmlAttribute();
			if ( !attrib )
			{
				if ( document ) document->SetError( TIXML_ERROR_OUT_OF_MEMORY, pErr, data );
				return 0;
			}

			attrib->SetDocument( document );
			const wchar_t* pErr = p;
			p = attrib->Parse( p, data );

			if ( !p || !*p )
			{
				if ( document ) document->SetError( TIXML_ERROR_PARSING_ELEMENT, pErr, data );
				delete attrib;
				return 0;
			}

			// Handle the strange case of double attributes:
			TiXmlAttribute* node = attributeSet.Find( attrib->Name() );
			if ( node )
			{
				node->SetValue( attrib->Value() );
				delete attrib;
				return 0;
			}

			attributeSet.Add( attrib );
		}
	}
	return p;
}


const wchar_t* TiXmlElement::ReadValue( const wchar_t* p, TiXmlParsingData* data )
{
	TiXmlDocument* document = GetDocument();

	// Read in text and elements in any order.
	p = SkipWhiteSpace( p );
	while ( p && *p )
	{
		if ( *p != '<' )
		{
			// Take what we have, make a text element.
			TiXmlText* textNode = new TiXmlText( L"" );

			if ( !textNode )
			{
				if ( document ) document->SetError( TIXML_ERROR_OUT_OF_MEMORY, 0, 0 );
				    return 0;
			}

			p = textNode->Parse( p, data );

			if ( !textNode->Blank() )
				LinkEndChild( textNode );
			else
				delete textNode;
		} 
		else 
		{
			// We hit a '<'
			// Have we hit a new element or an end tag?
			if ( StringEqual( p, L"</", false ) )
			{
				return p;
			}
			else
			{
				TiXmlNode* node = Identify( p );
				if ( node )
				{
					p = node->Parse( p, data );
					LinkEndChild( node );
				}				
				else
				{
					return 0;
				}
			}
		}
		p = SkipWhiteSpace( p );
	}

	if ( !p )
	{
		if ( document ) document->SetError( TIXML_ERROR_READING_ELEMENT_VALUE, 0, 0 );
	}	
	return p;
}


#ifdef TIXML_USE_STL
void TiXmlUnknown::StreamIn( TIXML_ISTREAM * in, TIXML_STRING * tag )
{
	while ( in->good() )
	{
		int c = in->get();	
		(*tag) += static_cast<wchar_t>(c);

		if ( c == '>' )
		{
			// All is well.
			return;		
		}
	}
}
#endif


const wchar_t* TiXmlUnknown::Parse( const wchar_t* p, TiXmlParsingData* data )
{
	TiXmlDocument* document = GetDocument();
	p = SkipWhiteSpace( p );

//	TiXmlParsingData data( p, prevData );
	if ( data )
	{
		data->Stamp( p );
		location = data->Cursor();
	}
	if ( !p || !*p || *p != '<' )
	{
		if ( document ) document->SetError( TIXML_ERROR_PARSING_UNKNOWN, p, data );
		return 0;
	}
	++p;
    value = L"";

	while ( p && *p && *p != '>' )
	{
		value += *p;
		++p;
	}

	if ( !p )
	{
		if ( document )	document->SetError( TIXML_ERROR_PARSING_UNKNOWN, 0, 0 );
	}
	if ( *p == '>' )
		return p+1;
	return p;
}

#ifdef TIXML_USE_STL
void TiXmlComment::StreamIn( TIXML_ISTREAM * in, TIXML_STRING * tag )
{
	while ( in->good() )
	{
		int c = in->get();	
		(*tag) += static_cast<wchar_t>(c);

		if ( c == '>' 
			 && tag->at( tag->length() - 2 ) == '-'
			 && tag->at( tag->length() - 3 ) == '-' )
		{
			// All is well.
			return;		
		}
	}
}
#endif


const wchar_t* TiXmlComment::Parse( const wchar_t* p, TiXmlParsingData* data )
{
	TiXmlDocument* document = GetDocument();
	value = L"";

	p = SkipWhiteSpace( p );

//	TiXmlParsingData data( p, prevData );
	if ( data )
	{
		data->Stamp( p );
		location = data->Cursor();
	}
	const wchar_t* startTag = L"<!--";
	const wchar_t* endTag   = L"-->";

	if ( !StringEqual( p, startTag, false ) )
	{
		document->SetError( TIXML_ERROR_PARSING_COMMENT, p, data );
		return 0;
	}
	p += lstrlen( startTag );
	p = ReadText( p, &value, false, endTag, false );
	return p;
}


const wchar_t* TiXmlAttribute::Parse( const wchar_t* p, TiXmlParsingData* data )
{
	p = SkipWhiteSpace( p );
	if ( !p || !*p ) return 0;

//	TiXmlParsingData data( p, prevData );
	if ( data )
	{
		data->Stamp( p );
		location = data->Cursor();
	}
	// Read the name, the '=' and the value.
	const wchar_t* pErr = p;
	p = ReadName( p, &name );
	if ( !p || !*p )
	{
		if ( document ) document->SetError( TIXML_ERROR_READING_ATTRIBUTES, pErr, data );
		return 0;
	}
	p = SkipWhiteSpace( p );
	if ( !p || !*p || *p != '=' )
	{
		if ( document ) document->SetError( TIXML_ERROR_READING_ATTRIBUTES, p, data );
		return 0;
	}

	++p;	// skip '='
	p = SkipWhiteSpace( p );
	if ( !p || !*p )
	{
		if ( document ) document->SetError( TIXML_ERROR_READING_ATTRIBUTES, p, data );
		return 0;
	}
	
	const wchar_t* end;

	if ( *p == '\'' )
	{
		++p;
		end = L"\'";
		p = ReadText( p, &value, false, end, false );
	}
	else if ( *p == '"' )
	{
		++p;
		end = L"\"";
		p = ReadText( p, &value, false, end, false );
	}
	else
	{
		// All attribute values should be in single or double quotes.
		// But this is such a common error that the parser will try
		// its best, even without them.
		value = L"";
		while (    p && *p										// existence
				&& !_istspace( *p ) && *p != '\n' && *p != '\r'	// whitespace
				&& *p != '/' && *p != '>' )						// tag end
		{
			value += *p;
			++p;
		}
	}
	return p;
}

#ifdef TIXML_USE_STL
void TiXmlText::StreamIn( TIXML_ISTREAM * in, TIXML_STRING * tag )
{
	while ( in->good() )
	{
		int c = in->peek();	
		if ( c == '<' )
			return;

		(*tag) += static_cast<wchar_t>(c);
		in->get();
	}
}
#endif

const wchar_t* TiXmlText::Parse( const wchar_t* p, TiXmlParsingData* data )
{
	value = L"";
//	TiXmlParsingData data( p, prevData );
	if ( data )
	{
		data->Stamp( p );
		location = data->Cursor();
	}
	bool ignoreWhite = true;

	const wchar_t* end = L"<";
	p = ReadText( p, &value, ignoreWhite, end, false );
	if ( p )
		return p-1;	// don't truncate the '<'
	return 0;
}

#ifdef TIXML_USE_STL
void TiXmlDeclaration::StreamIn( TIXML_ISTREAM * in, TIXML_STRING * tag )
{
	while ( in->good() )
	{
		int c = in->get();
		(*tag) += static_cast<wchar_t>(c);

		if ( c == '>' )
		{
			// All is well.
			return;
		}
	}
}
#endif

const wchar_t* TiXmlDeclaration::Parse( const wchar_t* p, TiXmlParsingData* data )
{
	p = SkipWhiteSpace( p );
	// Find the beginning, find the end, and look for
	// the stuff in-between.
	TiXmlDocument* document = GetDocument();
	if ( !p || !*p || !StringEqual( p, L"<?xml", true ) )
	{
		if ( document ) document->SetError( TIXML_ERROR_PARSING_DECLARATION, 0, 0 );
		return 0;
	}
//	TiXmlParsingData data( p, prevData );
	if ( data )
	{
		data->Stamp( p );
		location = data->Cursor();
	}
	p += 5;

	version = L"";
	encoding = L"";
	standalone = L"";

	while ( p && *p )
	{
		if ( *p == '>' )
		{
			++p;
			return p;
		}

		p = SkipWhiteSpace( p );
		if ( StringEqual( p, L"version", true ) )
		{
			TiXmlAttribute attrib;
			p = attrib.Parse( p, data );		
			version = attrib.Value();
		}
		else if ( StringEqual( p, L"encoding", true ) )
		{
			TiXmlAttribute attrib;
			p = attrib.Parse( p, data );		
			encoding = attrib.Value();
		}
		else if ( StringEqual( p, L"standalone", true ) )
		{
			TiXmlAttribute attrib;
			p = attrib.Parse( p, data );		
			standalone = attrib.Value();
		}
		else
		{
			// Read over whatever it is.
			while( p && *p && *p != '>' && !_istspace( *p ) )
				++p;
		}
	}
	return 0;
}

bool TiXmlText::Blank() const
{
	for (size_t i = 0, len = value.length(); i < len; i++)
		if ( !_istspace( value[i] ) )
			return false;
	return true;
}

