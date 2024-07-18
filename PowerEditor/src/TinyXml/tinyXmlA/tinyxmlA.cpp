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

#include "Common.h"
#include "tinyxmlA.h"

#ifdef TIXMLA_USE_STL
#include <sstream>
#endif

bool TiXmlBaseA::condenseWhiteSpace = true;

void TiXmlBaseA::PutString( const TIXMLA_STRING& str, TIXMLA_OSTREAM* stream )
{
	TIXMLA_STRING buffer;
	PutString( str, &buffer );
	(*stream) << buffer;
}

void TiXmlBaseA::PutString( const TIXMLA_STRING& str, TIXMLA_STRING* outString )
{
	int i=0;

	while( i<(int)str.length() )
	{
		int c = str[i];

		if (    c == '&' 
		     && i < ( static_cast<int>(str.length()) - 2 )
			 && str[i+1] == '#'
			 && str[i+2] == 'x' )
		{
			// Hexadecimal character reference.
			// Pass through unchanged.
			// &#xA9;	-- copyright symbol, for example.
			while (i < static_cast<int>(str.length()))
			{
				outString->append( str.c_str() + i, 1 );
				++i;
				if ( str[i] == ';' )
					break;
			}
		}
		else if ( c == '&' )
		{
			outString->append( entity[0].str, entity[0].strLength );
			++i;
		}
		else if ( c == '<' )
		{
			outString->append( entity[1].str, entity[1].strLength );
			++i;
		}
		else if ( c == '>' )
		{
			outString->append( entity[2].str, entity[2].strLength );
			++i;
		}
		else if ( c == '\"' )
		{
			outString->append( entity[3].str, entity[3].strLength );
			++i;
		}
		else if ( c == '\'' )
		{
			outString->append( entity[4].str, entity[4].strLength );
			++i;
		}
		// Remove the following code for that attribute value can be human readable if it contains Unicode characters
		/*
		else if ( c < 32 || c > 126 )
		{
			// Easy pass at non-alpha/numeric/symbol
			// 127 is the delete key. Below 32 is symbolic.
			char buf[ 32 ];
			sprintf( buf, "&#x%02X;", (unsigned) ( c & 0xff ) );
			outString->append( buf, strlen( buf ) );
			++i;
		}
		*/
		else
		{
			char realc = static_cast<char>(c);
			outString->append( &realc, 1 );
			++i;
		}
	}
}


// <-- Strange class for a bug fix. Search for STL_STRING_BUG
TiXmlBaseA::StringToBuffer::StringToBuffer( const TIXMLA_STRING& str )
{
	buffer = new char[ str.length()+1 ];
	if ( buffer )
	{
		strcpy( buffer, str.c_str() );
	}
}


TiXmlBaseA::StringToBuffer::~StringToBuffer()
{
	delete [] buffer;
}
// End strange bug fix. -->


TiXmlNodeA::TiXmlNodeA( NodeType _type )
{
	parent = 0;
	type = _type;
	firstChild = 0;
	lastChild = 0;
	prev = 0;
	next = 0;
	userData = 0;
}


TiXmlNodeA::~TiXmlNodeA()
{
	TiXmlNodeA* node = firstChild;
	TiXmlNodeA* temp = 0;

	while ( node )
	{
		temp = node;
		node = node->next;
		delete temp;
	}	
}


void TiXmlNodeA::Clear()
{
	TiXmlNodeA* node = firstChild;
	TiXmlNodeA* temp = 0;

	while ( node )
	{
		temp = node;
		node = node->next;
		delete temp;
	}	

	firstChild = 0;
	lastChild = 0;
}


TiXmlNodeA* TiXmlNodeA::LinkEndChild( TiXmlNodeA* node )
{
	node->parent = this;

	node->prev = lastChild;
	node->next = 0;

	if ( lastChild )
		lastChild->next = node;
	else
		firstChild = node;			// it was an empty list.

	lastChild = node;
	return node;
}


TiXmlNodeA* TiXmlNodeA::InsertEndChild( const TiXmlNodeA& addThis )
{
	TiXmlNodeA* node = addThis.Clone();
	if ( !node )
		return 0;

	return LinkEndChild( node );
}


TiXmlNodeA* TiXmlNodeA::InsertBeforeChild( TiXmlNodeA* beforeThis, const TiXmlNodeA& addThis )
{	
	if ( !beforeThis || beforeThis->parent != this )
		return 0;

	TiXmlNodeA* node = addThis.Clone();
	if ( !node )
		return 0;
	node->parent = this;

	node->next = beforeThis;
	node->prev = beforeThis->prev;
	if ( beforeThis->prev )
	{
		beforeThis->prev->next = node;
	}
	else
	{
		assert( firstChild == beforeThis );
		firstChild = node;
	}
	beforeThis->prev = node;
	return node;
}


TiXmlNodeA* TiXmlNodeA::InsertAfterChild( TiXmlNodeA* afterThis, const TiXmlNodeA& addThis )
{
	if ( !afterThis || afterThis->parent != this )
		return 0;

	TiXmlNodeA* node = addThis.Clone();
	if ( !node )
		return 0;
	node->parent = this;

	node->prev = afterThis;
	node->next = afterThis->next;
	if ( afterThis->next )
	{
		afterThis->next->prev = node;
	}
	else
	{
		assert( lastChild == afterThis );
		lastChild = node;
	}
	afterThis->next = node;
	return node;
}


TiXmlNodeA* TiXmlNodeA::ReplaceChild( TiXmlNodeA* replaceThis, const TiXmlNodeA& withThis )
{
	if ( replaceThis->parent != this )
		return 0;

	TiXmlNodeA* node = withThis.Clone();
	if ( !node )
		return 0;

	node->next = replaceThis->next;
	node->prev = replaceThis->prev;

	if ( replaceThis->next )
		replaceThis->next->prev = node;
	else
		lastChild = node;

	if ( replaceThis->prev )
		replaceThis->prev->next = node;
	else
		firstChild = node;

	delete replaceThis;
	node->parent = this;
	return node;
}


bool TiXmlNodeA::RemoveChild( TiXmlNodeA* removeThis )
{
	if ( removeThis->parent != this )
	{	
		assert( 0 );
		return false;
	}

	if ( removeThis->next )
		removeThis->next->prev = removeThis->prev;
	else
		lastChild = removeThis->prev;

	if ( removeThis->prev )
		removeThis->prev->next = removeThis->next;
	else
		firstChild = removeThis->next;

	delete removeThis;
	return true;
}

TiXmlNodeA* TiXmlNodeA::FirstChild( const char * _value ) const
{
	TiXmlNodeA* node;
	for ( node = firstChild; node; node = node->next )
	{
		if ( node->SValue() == TIXMLA_STRING( _value ))
			return node;
	}
	return 0;
}

TiXmlNodeA* TiXmlNodeA::LastChild( const char * _value ) const
{
	TiXmlNodeA* node;
	for ( node = lastChild; node; node = node->prev )
	{
		if ( node->SValue() == TIXMLA_STRING (_value))
			return node;
	}
	return 0;
}

TiXmlNodeA* TiXmlNodeA::IterateChildren( TiXmlNodeA* previous ) const
{
	if ( !previous )
	{
		return FirstChild();
	}
	else
	{
		assert( previous->parent == this );
		return previous->NextSibling();
	}
}

TiXmlNodeA* TiXmlNodeA::IterateChildren( const char * val, TiXmlNodeA* previous ) const
{
	if ( !previous )
	{
		return FirstChild( val );
	}
	else
	{
		assert( previous->parent == this );
		return previous->NextSibling( val );
	}
}

TiXmlNodeA* TiXmlNodeA::NextSibling( const char * _value ) const
{
	TiXmlNodeA* node;
	for ( node = next; node; node = node->next )
	{
		if ( node->SValue() == TIXMLA_STRING (_value))
			return node;
	}
	return 0;
}


TiXmlNodeA* TiXmlNodeA::PreviousSibling( const char * _value ) const
{
	TiXmlNodeA* node;
	for ( node = prev; node; node = node->prev )
	{
		if ( node->SValue() == TIXMLA_STRING (_value))
			return node;
	}
	return 0;
}

void TiXmlElementA::RemoveAttribute( const char * name )
{
	TiXmlAttributeA* node = attributeSet.Find( name );
	if ( node )
	{
		attributeSet.Remove( node );
		delete node;
	}
}

TiXmlElementA* TiXmlNodeA::FirstChildElement() const
{
	TiXmlNodeA* node;

	for (	node = FirstChild();
	node;
	node = node->NextSibling() )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}

TiXmlElementA* TiXmlNodeA::FirstChildElement( const char * _value ) const
{
	TiXmlNodeA* node;

	for (	node = FirstChild( _value );
	node;
	node = node->NextSibling( _value ) )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}


TiXmlElementA* TiXmlNodeA::NextSiblingElement() const
{
	TiXmlNodeA* node;

	for (	node = NextSibling();
	node;
	node = node->NextSibling() )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}

TiXmlElementA* TiXmlNodeA::NextSiblingElement( const char * _value ) const
{
	TiXmlNodeA* node;

	for (	node = NextSibling( _value );
	node;
	node = node->NextSibling( _value ) )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}



TiXmlDocumentA* TiXmlNodeA::GetDocument() const
{
	const TiXmlNodeA* node;

	for( node = this; node; node = node->parent )
	{
		if ( node->ToDocument() )
			return node->ToDocument();
	}
	return 0;
}


TiXmlElementA::TiXmlElementA (const char * _value)
: TiXmlNodeA( TiXmlNodeA::ELEMENT )
{
	firstChild = lastChild = 0;
	value = _value;
}

TiXmlElementA::~TiXmlElementA()
{
	while( attributeSet.First() )
	{
		TiXmlAttributeA* node = attributeSet.First();
		attributeSet.Remove( node );
		delete node;
	}
}

const char * TiXmlElementA::Attribute( const char * name ) const
{
	TiXmlAttributeA* node = attributeSet.Find( name );

	if ( node )
		return node->Value();

	return 0;
}


const char * TiXmlElementA::Attribute( const char * name, int* i ) const
{
	const char * s = Attribute( name );
	if ( i )
	{
		if ( s )
			*i = atoi( s );
		else
			*i = 0;
	}
	return s;
}


const char * TiXmlElementA::Attribute( const char * name, double* d ) const
{
	const char * s = Attribute( name );
	if ( d )
	{
		if ( s )
			*d = atof( s );
		else
			*d = 0;
	}
	return s;
}


int TiXmlElementA::QueryIntAttribute( const char* name, int* ival ) const
{
	TiXmlAttributeA* node = attributeSet.Find( name );
	if ( !node )
		return TIXMLA_NO_ATTRIBUTE;

	return node->QueryIntValue( ival );
}


int TiXmlElementA::QueryDoubleAttribute( const char* name, double* dval ) const
{
	TiXmlAttributeA* node = attributeSet.Find( name );
	if ( !node )
		return TIXMLA_NO_ATTRIBUTE;

	return node->QueryDoubleValue( dval );
}


void TiXmlElementA::SetAttribute( const char * name, int val )
{	
	char buf[64];
	sprintf( buf, "%d", val );
	SetAttribute( name, buf );
}


void TiXmlElementA::SetAttribute( const char * name, const char * _value )
{
	TiXmlAttributeA* node = attributeSet.Find( name );
	if ( node )
	{
		node->SetValue( _value );
		return;
	}

	TiXmlAttributeA* attrib = new TiXmlAttributeA( name, _value );
	if ( attrib )
	{
		attributeSet.Add( attrib );
	}
	else
	{
		TiXmlDocumentA* document = GetDocument();
		if ( document ) document->SetError( TIXMLA_ERROR_OUT_OF_MEMORY, 0, 0 );
	}
}

void TiXmlElementA::Print( FILE* cfile, int depth ) const
{
	int i;
	for ( i=0; i<depth; i++ )
	{
		fprintf( cfile, "    " );
	}

	fprintf( cfile, "<%s", value.c_str() );

	TiXmlAttributeA* attrib;
	for ( attrib = attributeSet.First(); attrib; attrib = attrib->Next() )
	{
		fprintf( cfile, " " );
		attrib->Print( cfile, depth );
	}

	// There are 3 different formatting approaches:
	// 1) An element without children is printed as a <foo /> node
	// 2) An element with only a text child is printed as <foo> text </foo>
	// 3) An element with children is printed on multiple lines.
	TiXmlNodeA* node;
	if ( !firstChild )
	{
		fprintf( cfile, " />" );
	}
	else if ( firstChild == lastChild && firstChild->ToText() )
	{
		fprintf( cfile, ">" );
		firstChild->Print( cfile, depth + 1 );
		fprintf( cfile, "</%s>", value.c_str() );
	}
	else
	{
		fprintf( cfile, ">" );

		for ( node = firstChild; node; node=node->NextSibling() )
		{
			if ( !node->ToText() )
			{
				fprintf( cfile, "\n" );
			}
			node->Print( cfile, depth+1 );
		}
		fprintf( cfile, "\n" );
		for( i=0; i<depth; ++i )
		fprintf( cfile, "    " );
		fprintf( cfile, "</%s>", value.c_str() );
	}
}

void TiXmlElementA::StreamOut( TIXMLA_OSTREAM * stream ) const
{
	(*stream) << "<" << value;

	TiXmlAttributeA* attrib;
	for ( attrib = attributeSet.First(); attrib; attrib = attrib->Next() )
	{	
		(*stream) << " ";
		attrib->StreamOut( stream );
	}

	// If this node has children, give it a closing tag. Else
	// make it an empty tag.
	TiXmlNodeA* node;
	if ( firstChild )
	{ 		
		(*stream) << ">";

		for ( node = firstChild; node; node=node->NextSibling() )
		{
			node->StreamOut( stream );
		}
		(*stream) << "</" << value << ">";
	}
	else
	{
		(*stream) << " />";
	}
}

TiXmlNodeA* TiXmlElementA::Clone() const
{
	TiXmlElementA* clone = new TiXmlElementA( Value() );
	if ( !clone )
		return 0;

	CopyToClone( clone );

	// Clone the attributes, then clone the children.
	TiXmlAttributeA* attribute = 0;
	for(	attribute = attributeSet.First();
	attribute;
	attribute = attribute->Next() )
	{
		clone->SetAttribute( attribute->Name(), attribute->Value() );
	}

	TiXmlNodeA* node = 0;
	for ( node = firstChild; node; node = node->NextSibling() )
	{
		clone->LinkEndChild( node->Clone() );
	}
	return clone;
}


TiXmlDocumentA::TiXmlDocumentA() : TiXmlNodeA( TiXmlNodeA::DOCUMENT )
{
	tabsize = 4;
	ClearError();
}

TiXmlDocumentA::TiXmlDocumentA( const char * documentName ) : TiXmlNodeA( TiXmlNodeA::DOCUMENT )
{
	tabsize = 4;
	value = documentName;
	ClearError();
}

bool TiXmlDocumentA::LoadFile()
{
	// See STL_STRING_BUG below.
	StringToBuffer buf( value );

	if ( buf.buffer && LoadFile( buf.buffer ) )
		return true;

	return false;
}


bool TiXmlDocumentA::SaveFile() const
{
	// See STL_STRING_BUG below.
	StringToBuffer buf( value );

	if ( buf.buffer && SaveFile( buf.buffer ) )
		return true;

	return false;
}

bool TiXmlDocumentA::LoadFile( const char* filename )
{
	// Delete the existing data:
	Clear();
	location.Clear();

	// There was a really terrifying little bug here. The code:
	//		value = filename
	// in the STL case, cause the assignment method of the std::string to
	// be called. What is strange, is that the std::string had the same
	// address as it's c_str() method, and so bad things happen. Looks
	// like a bug in the Microsoft STL implementation.
	// See STL_STRING_BUG above.
	// Fixed with the StringToBuffer class.
	value = filename;

	FILE* file = fopen( value.c_str (), "r" );

	if ( file )
	{
		// Get the file size, so we can pre-allocate the string. HUGE speed impact.
		long length = 0;
		fseek( file, 0, SEEK_END );
		length = ftell( file );
		fseek( file, 0, SEEK_SET );

		// Strange case, but good to handle up front.
		if ( length == 0 )
		{
			fclose( file );
			return false;
		}

		// If we have a file, assume it is all one big XML file, and read it in.
		// The document parser may decide the document ends sooner than the entire file, however.
		TIXMLA_STRING data;
		data.reserve( length );

		const int BUF_SIZE = 2048;
		char buf[BUF_SIZE];

		while( fgets( buf, BUF_SIZE, file ) )
		{
			data += buf;
		}
		fclose( file );

		Parse( data.c_str(), 0 );

		if (  Error() )
            return false;
        else
			return true;
	}
	SetError( TIXMLA_ERROR_OPENING_FILE, 0, 0 );
	return false;
}

bool TiXmlDocumentA::LoadUnicodeFilePath( const wchar_t* filename )
{
	
	// Delete the existing data:
	Clear();
	location.Clear();

	// There was a really terrifying little bug here. The code:
	//		value = filename
	// in the STL case, cause the assignment method of the string to
	// be called. What is strange, is that the string had the same
	// address as it's c_str() method, and so bad things happen. Looks
	// like a bug in the Microsoft STL implementation.
	// See STL_STRING_BUG above.
	// Fixed with the StringToBuffer class.

	FILE* file = _wfopen(filename, L"r");

	if ( file )
	{
		// Get the file size, so we can pre-allocate the string. HUGE speed impact.
		long length = 0;
		fseek( file, 0, SEEK_END );
		length = ftell( file );
		fseek( file, 0, SEEK_SET );

		// Strange case, but good to handle up front.
		if ( length == 0 )
		{
			fclose( file );
			return false;
		}

		// If we have a file, assume it is all one big XML file, and read it in.
		// The document parser may decide the document ends sooner than the entire file, however.
		TIXMLA_STRING data;
		data.reserve( length );

		const int BUF_SIZE = 2048;
		char buf[BUF_SIZE];

		while( fgets( buf, BUF_SIZE, file ) )
		{
			data += buf;
		}
		fclose( file );

		Parse( data.c_str(), 0 );

		if (  Error() )
            return false;
        else
			return true;
	}
	SetError( TIXMLA_ERROR_OPENING_FILE, 0, 0 );
	return false;
}

bool TiXmlDocumentA::SaveFile( const char * filename ) const
{
	// The old c stuff lives on...
	FILE* fp = fopen( filename, "wc" );
	if ( fp )
	{
		Print( fp, 0 );
		fflush( fp );
		fclose( fp );
		return true;
	}
	return false;
}
bool TiXmlDocumentA::SaveUnicodeFilePath( const wchar_t* filename ) const
{
	// The old c stuff lives on...
	FILE* fp = _wfopen( filename, L"wc" );
	if ( fp )
	{
		Print( fp, 0 );
		fflush( fp );
		fclose( fp );
		return true;
	}
	return false;
}


TiXmlNodeA* TiXmlDocumentA::Clone() const
{
	TiXmlDocumentA* clone = new TiXmlDocumentA();
	if ( !clone )
		return 0;

	CopyToClone( clone );
	clone->error = error;
	clone->errorDesc = errorDesc.c_str ();

	TiXmlNodeA* node = 0;
	for ( node = firstChild; node; node = node->NextSibling() )
	{
		clone->LinkEndChild( node->Clone() );
	}
	return clone;
}


void TiXmlDocumentA::Print( FILE* cfile, int depth ) const
{
	TiXmlNodeA* node;
	for ( node=FirstChild(); node; node=node->NextSibling() )
	{
		node->Print( cfile, depth );
		fprintf( cfile, "\n" );
	}
}

void TiXmlDocumentA::StreamOut( TIXMLA_OSTREAM * out ) const
{
	TiXmlNodeA* node;
	for ( node=FirstChild(); node; node=node->NextSibling() )
	{
		node->StreamOut( out );

		// Special rule for streams: stop after the root element.
		// The stream in code will only read one element, so don't
		// write more than one.
		if ( node->ToElement() )
			break;
	}
}


TiXmlAttributeA* TiXmlAttributeA::Next() const
{
	// We are using knowledge of the sentinel. The sentinel
	// have a value or name.
	if ( next->value.empty() && next->name.empty() )
		return 0;
	return next;
}


TiXmlAttributeA* TiXmlAttributeA::Previous() const
{
	// We are using knowledge of the sentinel. The sentinel
	// have a value or name.
	if ( prev->value.empty() && prev->name.empty() )
		return 0;
	return prev;
}


void TiXmlAttributeA::Print( FILE* cfile, int /*depth*/ ) const
{
	TIXMLA_STRING n, v;

	PutString( Name(), &n );
	PutString( Value(), &v );

	if (value.find ('\"') == TIXMLA_STRING::npos)
		fprintf (cfile, "%s=\"%s\"", n.c_str(), v.c_str() );
	else
		fprintf (cfile, "%s='%s'", n.c_str(), v.c_str() );
}


void TiXmlAttributeA::StreamOut( TIXMLA_OSTREAM * stream ) const
{
	if (value.find( '\"' ) != TIXMLA_STRING::npos)
	{
		PutString( name, stream );
		(*stream) << "=" << "'";
		PutString( value, stream );
		(*stream) << "'";
	}
	else
	{
		PutString( name, stream );
		(*stream) << "=" << "\"";
		PutString( value, stream );
		(*stream) << "\"";
	}
}

int TiXmlAttributeA::QueryIntValue( int* ival ) const
{
	if ( sscanf( value.c_str(), "%d", ival ) == 1 )
		return TIXMLA_SUCCESS;
	return TIXMLA_WRONG_TYPE;
}

int TiXmlAttributeA::QueryDoubleValue( double* dval ) const
{
	if ( sscanf( value.c_str(), "%lf", dval ) == 1 )
		return TIXMLA_SUCCESS;
	return TIXMLA_WRONG_TYPE;
}

void TiXmlAttributeA::SetIntValue( int _value )
{
	char buf [64];
	sprintf (buf, "%d", _value);
	SetValue (buf);
}

void TiXmlAttributeA::SetDoubleValue( double _value )
{
	char buf [64];
	sprintf (buf, "%lf", _value);
	SetValue (buf);
}

int TiXmlAttributeA::IntValue() const
{
	return atoi (value.c_str ());
}

double TiXmlAttributeA::DoubleValue() const
{
	return atof (value.c_str ());
}

void TiXmlCommentA::Print( FILE* cfile, int depth ) const
{
	for ( int i=0; i<depth; i++ )
	{
		fputs( "    ", cfile );
	}
	fprintf( cfile, "<!--%s-->", value.c_str() );
}

void TiXmlCommentA::StreamOut( TIXMLA_OSTREAM * stream ) const
{
	(*stream) << "<!--";
	PutString( value, stream );
	(*stream) << "-->";
}

TiXmlNodeA* TiXmlCommentA::Clone() const
{
	TiXmlCommentA* clone = new TiXmlCommentA();

	if ( !clone )
		return 0;

	CopyToClone( clone );
	return clone;
}


void TiXmlTextA::Print( FILE* cfile, int /*depth*/ ) const
{
	TIXMLA_STRING buffer;
	PutString( value, &buffer );
	fprintf( cfile, "%s", buffer.c_str() );
}


void TiXmlTextA::StreamOut( TIXMLA_OSTREAM * stream ) const
{
	PutString( value, stream );
}


TiXmlNodeA* TiXmlTextA::Clone() const
{	
	TiXmlTextA* clone = 0;
	clone = new TiXmlTextA( "" );

	if ( !clone )
		return 0;

	CopyToClone( clone );
	return clone;
}


TiXmlDeclarationA::TiXmlDeclarationA( const char * _version,
	const char * _encoding,
	const char * _standalone )
: TiXmlNodeA( TiXmlNodeA::DECLARATION )
{
	version = _version;
	encoding = _encoding;
	standalone = _standalone;
}


void TiXmlDeclarationA::Print( FILE* cfile, int /*depth*/ ) const
{
	fprintf (cfile, "<?xml ");

	if ( !version.empty() )
		fprintf (cfile, "version=\"%s\" ", version.c_str ());
	if ( !encoding.empty() )
		fprintf (cfile, "encoding=\"%s\" ", encoding.c_str ());
	if ( !standalone.empty() )
		fprintf (cfile, "standalone=\"%s\" ", standalone.c_str ());
	fprintf (cfile, "?>");
}

void TiXmlDeclarationA::StreamOut( TIXMLA_OSTREAM * stream ) const
{
	(*stream) << "<?xml ";

	if ( !version.empty() )
	{
		(*stream) << "version=\"";
		PutString( version, stream );
		(*stream) << "\" ";
	}
	if ( !encoding.empty() )
	{
		(*stream) << "encoding=\"";
		PutString( encoding, stream );
		(*stream ) << "\" ";
	}
	if ( !standalone.empty() )
	{
		(*stream) << "standalone=\"";
		PutString( standalone, stream );
		(*stream) << "\" ";
	}
	(*stream) << "?>";
}

TiXmlNodeA* TiXmlDeclarationA::Clone() const
{	
	TiXmlDeclarationA* clone = new TiXmlDeclarationA();

	if ( !clone )
		return 0;

	CopyToClone( clone );
	clone->version = version;
	clone->encoding = encoding;
	clone->standalone = standalone;
	return clone;
}


void TiXmlUnknownA::Print( FILE* cfile, int depth ) const
{
	for ( int i=0; i<depth; i++ )
		fprintf( cfile, "    " );
	fprintf( cfile, "%s", value.c_str() );
}

void TiXmlUnknownA::StreamOut( TIXMLA_OSTREAM * stream ) const
{
	(*stream) << "<" << value << ">";		// Don't use entities hear! It is unknown.
}

TiXmlNodeA* TiXmlUnknownA::Clone() const
{
	TiXmlUnknownA* clone = new TiXmlUnknownA();

	if ( !clone )
		return 0;

	CopyToClone( clone );
	return clone;
}


TiXmlAttributeSetA::TiXmlAttributeSetA()
{
	sentinel.next = &sentinel;
	sentinel.prev = &sentinel;
}


TiXmlAttributeSetA::~TiXmlAttributeSetA()
{
	assert( sentinel.next == &sentinel );
	assert( sentinel.prev == &sentinel );
}


void TiXmlAttributeSetA::Add( TiXmlAttributeA* addMe )
{
	assert( !Find( addMe->Name() ) );	// Shouldn't be multiply adding to the set.

	addMe->next = &sentinel;
	addMe->prev = sentinel.prev;

	sentinel.prev->next = addMe;
	sentinel.prev      = addMe;
}

void TiXmlAttributeSetA::Remove( TiXmlAttributeA* removeMe )
{
	TiXmlAttributeA* node;

	for( node = sentinel.next; node != &sentinel; node = node->next )
	{
		if ( node == removeMe )
		{
			node->prev->next = node->next;
			node->next->prev = node->prev;
			node->next = 0;
			node->prev = 0;
			return;
		}
	}
	assert( 0 );		// we tried to remove a non-linked attribute.
}

TiXmlAttributeA*	TiXmlAttributeSetA::Find( const char * name ) const
{
	TiXmlAttributeA* node;

	for( node = sentinel.next; node != &sentinel; node = node->next )
	{
		if ( node->name == name )
			return node;
	}
	return 0;
}


#ifdef TIXMLA_USE_STL	
TIXMLA_ISTREAM & operator >> (TIXMLA_ISTREAM & in, TiXmlNodeA & base)
{
	TIXMLA_STRING tag;
	tag.reserve( 8 * 1000 );
	base.StreamIn( &in, &tag );

	base.Parse( tag.c_str(), 0 );
	return in;
}
#endif


TIXMLA_OSTREAM & operator<< (TIXMLA_OSTREAM & out, const TiXmlNodeA & base)
{
	base.StreamOut (& out);
	return out;
}


#ifdef TIXMLA_USE_STL	
std::string & operator<< (std::string& out, const TiXmlNodeA& base )
{
   std::ostringstream os_stream( std::ostringstream::out );
   base.StreamOut( &os_stream );
   
   out.append( os_stream.str() );
   return out;
}
#endif


TiXmlHandleA TiXmlHandleA::FirstChild() const
{
	if ( node )
	{
		TiXmlNodeA* child = node->FirstChild();
		if ( child )
			return TiXmlHandleA( child );
	}
	return TiXmlHandleA( 0 );
}


TiXmlHandleA TiXmlHandleA::FirstChild( const char * value ) const
{
	if ( node )
	{
		TiXmlNodeA* child = node->FirstChild( value );
		if ( child )
			return TiXmlHandleA( child );
	}
	return TiXmlHandleA( 0 );
}


TiXmlHandleA TiXmlHandleA::FirstChildElement() const
{
	if ( node )
	{
		TiXmlElementA* child = node->FirstChildElement();
		if ( child )
			return TiXmlHandleA( child );
	}
	return TiXmlHandleA( 0 );
}


TiXmlHandleA TiXmlHandleA::FirstChildElement( const char * value ) const
{
	if ( node )
	{
		TiXmlElementA* child = node->FirstChildElement( value );
		if ( child )
			return TiXmlHandleA( child );
	}
	return TiXmlHandleA( 0 );
}

TiXmlHandleA TiXmlHandleA::Child( int count ) const
{
	if ( node )
	{
		int i;
		TiXmlNodeA* child = node->FirstChild();
		for (	i=0;
				child && i<count;
				child = child->NextSibling(), ++i )
		{
			// nothing
		}
		if ( child )
			return TiXmlHandleA( child );
	}
	return TiXmlHandleA( 0 );
}


TiXmlHandleA TiXmlHandleA::Child( const char* value, int count ) const
{
	if ( node )
	{
		int i;
		TiXmlNodeA* child = node->FirstChild( value );
		for (	i=0;
				child && i<count;
				child = child->NextSibling( value ), ++i )
		{
			// nothing
		}
		if ( child )
			return TiXmlHandleA( child );
	}
	return TiXmlHandleA( 0 );
}


TiXmlHandleA TiXmlHandleA::ChildElement( int count ) const
{
	if ( node )
	{
		int i;
		TiXmlElementA* child = node->FirstChildElement();
		for (	i=0;
				child && i<count;
				child = child->NextSiblingElement(), ++i )
		{
			// nothing
		}
		if ( child )
			return TiXmlHandleA( child );
	}
	return TiXmlHandleA( 0 );
}


TiXmlHandleA TiXmlHandleA::ChildElement( const char* value, int count ) const
{
	if ( node )
	{
		int i;
		TiXmlElementA* child = node->FirstChildElement( value );
		for (	i=0;
				child && i<count;
				child = child->NextSiblingElement( value ), ++i )
		{
			// nothing
		}
		if ( child )
			return TiXmlHandleA( child );
	}
	return TiXmlHandleA( 0 );
}
