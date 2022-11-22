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
#include <memory>
#include "tinyxml.h"

bool TiXmlBase::condenseWhiteSpace = true;

void TiXmlBase::PutString( const TIXML_STRING& str, TIXML_OSTREAM* stream )
{
	TIXML_STRING buffer;
	PutString( str, &buffer );
	(*stream) << buffer;
}

void TiXmlBase::PutString( const TIXML_STRING& str, TIXML_STRING* outString )
{
	size_t i = 0;

	while (i < str.length())
	{
		int c = str[i];

		if (c == '&' 
		     && i < ( str.length() - 2 )
			 && str[i+1] == '#'
			 && str[i+2] == 'x' )
		{
			// Hexadecimal character reference.
			// Pass through unchanged.
			// &#xA9;	-- copyright symbol, for example.
			while ( i < str.length() )
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
		else if ( c < 32 || c > 126 )
		{
			// Easy pass at non-alpha/numeric/symbol
			// 127 is the delete key. Below 32 is symbolic.
			TCHAR buf[32];
			wsprintf( buf, TEXT("&#x%04X;"), static_cast<unsigned int>(c & 0xffff) );
			outString->append( buf, lstrlen( buf ) );
			++i;
		}
		else
		{
			TCHAR realc = static_cast<TCHAR>(c);
			outString->append( &realc, 1 );
			++i;
		}
	}
}


// <-- Strange class for a bug fix. Search for STL_STRING_BUG
TiXmlBase::StringToBuffer::StringToBuffer( const TIXML_STRING& str )
{
	const size_t strLen = str.length() + 1;
	buffer = new TCHAR[strLen];
	if (buffer)
	{
		wcscpy_s(buffer, strLen, str.c_str());
	}
}


TiXmlBase::StringToBuffer::~StringToBuffer()
{
	delete [] buffer;
}
// End strange bug fix. -->


TiXmlNode::TiXmlNode( NodeType _type )
{
	parent = 0;
	type = _type;
	firstChild = 0;
	lastChild = 0;
	prev = 0;
	next = 0;
	userData = 0;
}


TiXmlNode::~TiXmlNode()
{
	TiXmlNode* node = firstChild;
	TiXmlNode* temp = 0;

	while ( node )
	{
		temp = node;
		node = node->next;
		delete temp;
	}	
}


void TiXmlNode::Clear()
{
	TiXmlNode* node = firstChild;
	TiXmlNode* temp = 0;

	while ( node )
	{
		temp = node;
		node = node->next;
		delete temp;
	}	

	firstChild = 0;
	lastChild = 0;
}


TiXmlNode* TiXmlNode::LinkEndChild( TiXmlNode* node )
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


TiXmlNode* TiXmlNode::InsertEndChild( const TiXmlNode& addThis )
{
	TiXmlNode* node = addThis.Clone();
	if ( !node )
		return 0;

	return LinkEndChild( node );
}


TiXmlNode* TiXmlNode::InsertBeforeChild( TiXmlNode* beforeThis, const TiXmlNode& addThis )
{	
	if ( !beforeThis || beforeThis->parent != this )
		return 0;

	TiXmlNode* node = addThis.Clone();
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


TiXmlNode* TiXmlNode::InsertAfterChild( TiXmlNode* afterThis, const TiXmlNode& addThis )
{
	if ( !afterThis || afterThis->parent != this )
		return 0;

	TiXmlNode* node = addThis.Clone();
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


TiXmlNode* TiXmlNode::ReplaceChild( TiXmlNode* replaceThis, const TiXmlNode& withThis )
{
	if ( replaceThis->parent != this )
		return 0;

	TiXmlNode* node = withThis.Clone();
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


bool TiXmlNode::RemoveChild( TiXmlNode* removeThis )
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

TiXmlNode* TiXmlNode::FirstChild( const TCHAR * _value ) const
{
	TiXmlNode* node;
	for ( node = firstChild; node; node = node->next )
	{
		if ( node->SValue() == TIXML_STRING( _value ))
			return node;
	}
	return 0;
}

TiXmlNode* TiXmlNode::LastChild( const TCHAR * _value ) const
{
	TiXmlNode* node;
	for ( node = lastChild; node; node = node->prev )
	{
		if ( node->SValue() == TIXML_STRING (_value))
			return node;
	}
	return 0;
}

TiXmlNode* TiXmlNode::IterateChildren( TiXmlNode* previous ) const
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

TiXmlNode* TiXmlNode::IterateChildren( const TCHAR * val, TiXmlNode* previous ) const
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

TiXmlNode* TiXmlNode::NextSibling( const TCHAR * _value ) const
{
	TiXmlNode* node;
	for ( node = next; node; node = node->next )
	{
		if ( node->SValue() == TIXML_STRING (_value))
			return node;
	}
	return 0;
}


TiXmlNode* TiXmlNode::PreviousSibling( const TCHAR * _value ) const
{
	TiXmlNode* node;
	for ( node = prev; node; node = node->prev )
	{
		if ( node->SValue() == TIXML_STRING (_value))
			return node;
	}
	return 0;
}

void TiXmlElement::RemoveAttribute( const TCHAR * name )
{
	TiXmlAttribute* node = attributeSet.Find( name );
	if ( node )
	{
		attributeSet.Remove( node );
		delete node;
	}
}

TiXmlElement* TiXmlNode::FirstChildElement() const
{
	TiXmlNode* node;

	for (	node = FirstChild();
	node;
	node = node->NextSibling() )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}

TiXmlElement* TiXmlNode::FirstChildElement( const TCHAR * _value ) const
{
	TiXmlNode* node;

	for (	node = FirstChild( _value );
	node;
	node = node->NextSibling( _value ) )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}


TiXmlElement* TiXmlNode::NextSiblingElement() const
{
	TiXmlNode* node;

	for (	node = NextSibling();
	node;
	node = node->NextSibling() )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}

TiXmlElement* TiXmlNode::NextSiblingElement( const TCHAR * _value ) const
{
	TiXmlNode* node;

	for (	node = NextSibling( _value );
	node;
	node = node->NextSibling( _value ) )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}



TiXmlDocument* TiXmlNode::GetDocument() const
{
	const TiXmlNode* node;

	for( node = this; node; node = node->parent )
	{
		if ( node->ToDocument() )
			return node->ToDocument();
	}
	return 0;
}


TiXmlElement::TiXmlElement (const TCHAR * _value)
: TiXmlNode( TiXmlNode::ELEMENT )
{
	firstChild = lastChild = 0;
	value = _value;
}

TiXmlElement::~TiXmlElement()
{
	while( attributeSet.First() )
	{
		TiXmlAttribute* node = attributeSet.First();
		attributeSet.Remove( node );
		delete node;
	}
}

const TCHAR * TiXmlElement::Attribute( const TCHAR * name ) const
{
	TiXmlAttribute* node = attributeSet.Find( name );

	if ( node )
		return node->Value();

	return 0;
}


const TCHAR * TiXmlElement::Attribute( const TCHAR * name, int* i ) const
{
	const TCHAR * s = Attribute( name );
	if ( i )
	{
		if ( s )
			*i = generic_atoi( s );
		else
			*i = 0;
	}
	return s;
}


const TCHAR * TiXmlElement::Attribute( const TCHAR * name, double* d ) const
{
	const TCHAR * s = Attribute( name );
	if ( d )
	{
		if ( s )
			*d = generic_atof( s );
		else
			*d = 0;
	}
	return s;
}


int TiXmlElement::QueryIntAttribute( const TCHAR* name, int* ival ) const
{
	TiXmlAttribute* node = attributeSet.Find( name );
	if ( !node )
		return TIXML_NO_ATTRIBUTE;

	return node->QueryIntValue( ival );
}


int TiXmlElement::QueryDoubleAttribute( const TCHAR* name, double* dval ) const
{
	TiXmlAttribute* node = attributeSet.Find( name );
	if ( !node )
		return TIXML_NO_ATTRIBUTE;

	return node->QueryDoubleValue( dval );
}


void TiXmlElement::SetAttribute( const TCHAR * name, int val )
{	
	TCHAR buf[64];
	wsprintf( buf, TEXT("%d"), val );
	SetAttribute( name, buf );
}


void TiXmlElement::SetAttribute( const TCHAR * name, const TCHAR * _value )
{
	TiXmlAttribute* node = attributeSet.Find( name );
	if ( node )
	{
		node->SetValue( _value );
		return;
	}

	TiXmlAttribute* attrib = new TiXmlAttribute( name, _value );
	if ( attrib )
	{
		attributeSet.Add( attrib );
	}
	else
	{
		TiXmlDocument* document = GetDocument();
		if ( document ) document->SetError( TIXML_ERROR_OUT_OF_MEMORY, 0, 0 );
	}
}

void TiXmlElement::Print( std::string& outputStream, int depth ) const
{
	int i;
	for ( i=0; i<depth; i++ )
	{
		outputStream += "    ";
		//generic_fprintf( cfile, TEXT("    ") );
	}

	std::string tagOpenValue = "<";
	tagOpenValue += wstring2string(value, CP_UTF8);
	outputStream += tagOpenValue;
	//generic_fprintf( cfile, TEXT("<%ls"), value.c_str() );

	TiXmlAttribute* attrib;
	for ( attrib = attributeSet.First(); attrib; attrib = attrib->Next() )
	{
		outputStream += " ";
		//generic_fprintf(cfile, TEXT(" "));

		attrib->Print(outputStream, depth );
	}

	// There are 3 different formatting approaches:
	// 1) An element without children is printed as a <foo /> node
	// 2) An element with only a text child is printed as <foo> text </foo>
	// 3) An element with children is printed on multiple lines.
	TiXmlNode* node;
	if ( !firstChild )
	{
		outputStream += " />";
		//generic_fprintf( cfile, TEXT(" />") );
	}
	else if ( firstChild == lastChild && firstChild->ToText() )
	{
		outputStream += ">";
		//generic_fprintf( cfile, TEXT(">") );

		firstChild->Print(outputStream, depth + 1 );

		std::string tagCloseWithValue = "</";
		tagCloseWithValue += wstring2string(value, CP_UTF8) + ">";
		outputStream += tagCloseWithValue;
		//generic_fprintf( cfile, TEXT("</%ls>"), value.c_str() );
	}
	else
	{
		outputStream += ">";
		//generic_fprintf( cfile, TEXT(">") );

		for ( node = firstChild; node; node=node->NextSibling() )
		{
			if ( !node->ToText() )
			{
				outputStream += "\r\n";
				//generic_fprintf( cfile, TEXT("\n") );
			}
			node->Print(outputStream, depth+1 );
		}
		outputStream += "\r\n";
		//generic_fprintf( cfile, TEXT("\n") );

		for( i=0; i<depth; ++i )
			outputStream += "    ";
			// generic_fprintf( cfile, TEXT("    ") );

		std::string tagCloseWithValue = "</";
		tagCloseWithValue += wstring2string(value, CP_UTF8) + ">";
		outputStream += tagCloseWithValue;
		//generic_fprintf( cfile, TEXT("</%ls>"), value.c_str() );
	}
}

void TiXmlElement::StreamOut( TIXML_OSTREAM * stream ) const
{
	(*stream) << TEXT("<") << value;

	TiXmlAttribute* attrib;
	for ( attrib = attributeSet.First(); attrib; attrib = attrib->Next() )
	{	
		(*stream) << TEXT(" ");
		attrib->StreamOut( stream );
	}

	// If this node has children, give it a closing tag. Else
	// make it an empty tag.
	TiXmlNode* node;
	if ( firstChild )
	{ 		
		(*stream) << TEXT(">");

		for ( node = firstChild; node; node=node->NextSibling() )
		{
			node->StreamOut( stream );
		}
		(*stream) << TEXT("</") << value << TEXT(">");
	}
	else
	{
		(*stream) << TEXT(" />");
	}
}

TiXmlNode* TiXmlElement::Clone() const
{
	TiXmlElement* clone = new TiXmlElement( Value() );
	if ( !clone )
		return 0;

	CopyToClone( clone );

	// Clone the attributes, then clone the children.
	TiXmlAttribute* attribute = 0;
	for(	attribute = attributeSet.First();
	attribute;
	attribute = attribute->Next() )
	{
		clone->SetAttribute( attribute->Name(), attribute->Value() );
	}

	TiXmlNode* node = 0;
	for ( node = firstChild; node; node = node->NextSibling() )
	{
		clone->LinkEndChild( node->Clone() );
	}
	return clone;
}


TiXmlDocument::TiXmlDocument() : TiXmlNode( TiXmlNode::DOCUMENT )
{
	tabsize = 4;
	ClearError();
}

TiXmlDocument::TiXmlDocument( const TCHAR * documentName ) : TiXmlNode( TiXmlNode::DOCUMENT )
{
	tabsize = 4;
	value = documentName;
	ClearError();
}

bool TiXmlDocument::LoadFile()
{
	// See STL_STRING_BUG below.
	StringToBuffer buf( value );

	if ( buf.buffer && LoadFile( buf.buffer ) )
		return true;

	return false;
}


bool TiXmlDocument::SaveFile() const
{
	// See STL_STRING_BUG below.
	StringToBuffer buf( value );

	if ( buf.buffer && SaveFile( buf.buffer ) )
		return true;

	return false;
}

bool TiXmlDocument::LoadFile( const TCHAR* filename )
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
	value = filename;

	FILE* file = generic_fopen( value.c_str (), TEXT("r") );

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
		TIXML_STRING data;
		data.reserve( length );

		const int BUF_SIZE = 2048;
		TCHAR buf[BUF_SIZE];

		while( generic_fgets( buf, BUF_SIZE, file ) )
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
	SetError( TIXML_ERROR_OPENING_FILE, 0, 0 );
	return false;
}

bool TiXmlDocument::SaveFile( const TCHAR * filename ) const
{
	/*
	// The old c stuff lives on...
	FILE* fp = generic_fopen( filename, TEXT("wc") );
	if ( fp )
	{
		Print( fp, 0 );
		fflush( fp );
		fclose( fp );
		return true;
	}
	return false;
	*/

	Win32_IO_File file(filename);

	if (file.isOpened())
	{
		std::unique_ptr<std::string> outputStr = std::make_unique<std::string>();
		Print(*outputStr, 0);
		if (!outputStr->empty())
			file.writeStr(*outputStr);
		return true;
	}

	return false;
}


TiXmlNode* TiXmlDocument::Clone() const
{
	TiXmlDocument* clone = new TiXmlDocument();
	if ( !clone )
		return 0;

	CopyToClone( clone );
	clone->error = error;
	clone->errorDesc = errorDesc.c_str ();

	TiXmlNode* node = 0;
	for ( node = firstChild; node; node = node->NextSibling() )
	{
		clone->LinkEndChild( node->Clone() );
	}
	return clone;
}


void TiXmlDocument::Print( std::string& outputStream, int depth ) const
{
	TiXmlNode* node;
	for ( node=FirstChild(); node; node=node->NextSibling() )
	{
		node->Print(outputStream, depth );

		outputStream += "\r\n";
		//generic_fprintf( cfile, TEXT("\n") );
	}
}

void TiXmlDocument::StreamOut( TIXML_OSTREAM * out ) const
{
	TiXmlNode* node;
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


TiXmlAttribute* TiXmlAttribute::Next() const
{
	// We are using knowledge of the sentinel. The sentinel
	// have a value or name.
	if ( next->value.empty() && next->name.empty() )
		return 0;
	return next;
}


TiXmlAttribute* TiXmlAttribute::Previous() const
{
	// We are using knowledge of the sentinel. The sentinel
	// have a value or name.
	if ( prev->value.empty() && prev->name.empty() )
		return 0;
	return prev;
}


void TiXmlAttribute::Print( std::string& outputStream, int /*depth*/ ) const
{
	TIXML_STRING n, v;

	PutString( Name(), &n );
	PutString( Value(), &v );

	std::string attrVsValue = wstring2string(n, CP_UTF8);
	if (value.find('\"') == TIXML_STRING::npos)
	{
		attrVsValue += "=\"";
		attrVsValue += wstring2string(v, CP_UTF8);
		attrVsValue += "\"";
		//generic_fprintf(cfile, TEXT("%ls=\"%ls\""), n.c_str(), v.c_str());
	}
	else
	{
		attrVsValue += "='";
		attrVsValue += wstring2string(v, CP_UTF8);
		attrVsValue += "'";
		//generic_fprintf(cfile, TEXT("%ls='%ls'"), n.c_str(), v.c_str());
	}
	outputStream += attrVsValue;
}


void TiXmlAttribute::StreamOut( TIXML_OSTREAM * stream ) const
{
	if (value.find( '\"' ) != TIXML_STRING::npos)
	{
		PutString( name, stream );
		(*stream) << TEXT("=") << TEXT("'");
		PutString( value, stream );
		(*stream) << TEXT("'");
	}
	else
	{
		PutString( name, stream );
		(*stream) << TEXT("=") << TEXT("\"");
		PutString( value, stream );
		(*stream) << TEXT("\"");
	}
}

int TiXmlAttribute::QueryIntValue( int* ival ) const
{
	if ( generic_sscanf( value.c_str(), TEXT("%d"), ival ) == 1 )
		return TIXML_SUCCESS;
	return TIXML_WRONG_TYPE;
}

int TiXmlAttribute::QueryDoubleValue( double* dval ) const
{
	if ( generic_sscanf( value.c_str(), TEXT("%lf"), dval ) == 1 )
		return TIXML_SUCCESS;
	return TIXML_WRONG_TYPE;
}

void TiXmlAttribute::SetIntValue( int _value )
{
	TCHAR buf [64];
	wsprintf (buf, TEXT("%d"), _value);
	SetValue (buf);
}

void TiXmlAttribute::SetDoubleValue( double _value )
{
	TCHAR buf [64];
	wsprintf (buf, TEXT("%lf"), _value);
	SetValue (buf);
}

const int TiXmlAttribute::IntValue() const
{
	return generic_atoi (value.c_str ());
}

const double  TiXmlAttribute::DoubleValue() const
{
	return generic_atof (value.c_str ());
}

void TiXmlComment::Print( std::string& outputStream, int depth ) const
{
	for ( int i=0; i<depth; i++ )
	{
		outputStream += "    ";
		//generic_fprintf( cfile, TEXT("    ") );
	}

	std::string comment = "<!--";
	comment += wstring2string(value, CP_UTF8);
	comment += "-->";
	outputStream += comment;
	//generic_fprintf( cfile, TEXT("<!--%ls-->"), value.c_str() );
}

void TiXmlComment::StreamOut( TIXML_OSTREAM * stream ) const
{
	(*stream) << TEXT("<!--");
	PutString( value, stream );
	(*stream) << TEXT("-->");
}

TiXmlNode* TiXmlComment::Clone() const
{
	TiXmlComment* clone = new TiXmlComment();

	if ( !clone )
		return 0;

	CopyToClone( clone );
	return clone;
}


void TiXmlText::Print( std::string& outputStream, int /*depth*/ ) const
{
	TIXML_STRING buffer;
	PutString( value, &buffer );

	outputStream += wstring2string(buffer, CP_UTF8);
	//generic_fprintf( cfile, TEXT("%ls"), buffer.c_str() );
}


void TiXmlText::StreamOut( TIXML_OSTREAM * stream ) const
{
	PutString( value, stream );
}


TiXmlNode* TiXmlText::Clone() const
{	
	TiXmlText* clone = 0;
	clone = new TiXmlText( TEXT("") );

	if ( !clone )
		return 0;

	CopyToClone( clone );
	return clone;
}


TiXmlDeclaration::TiXmlDeclaration( const TCHAR * _version,
	const TCHAR * _encoding,
	const TCHAR * _standalone )
: TiXmlNode( TiXmlNode::DECLARATION )
{
	version = _version;
	encoding = _encoding;
	standalone = _standalone;
}

void TiXmlDeclaration::Print( std::string& outputStream, int /*depth*/ ) const
{
	std::string xmlDcl = "<?xml ";
	//generic_fprintf (cfile, TEXT("<?xml "));

	if (!version.empty())
	{
		xmlDcl += "version=\"";
		xmlDcl += wstring2string(version, CP_UTF8);
		xmlDcl += "\" ";
		//generic_fprintf(cfile, TEXT("version=\"%ls\" "), version.c_str());
	}

	if (!encoding.empty())
	{
		xmlDcl += "encoding=\"";
		xmlDcl += wstring2string(encoding, CP_UTF8);
		xmlDcl += "\" ";
		//generic_fprintf(cfile, TEXT("encoding=\"%ls\" "), encoding.c_str());
	}
	if (!standalone.empty())
	{
		xmlDcl += "standalone=\"";
		xmlDcl += wstring2string(standalone, CP_UTF8);
		xmlDcl += "\" ";
		//generic_fprintf(cfile, TEXT("standalone=\"%ls\" "), standalone.c_str());
	}

	xmlDcl += "?>";
	//generic_fprintf (cfile, TEXT("?>"));

	outputStream += xmlDcl;
}

void TiXmlDeclaration::StreamOut( TIXML_OSTREAM * stream ) const
{
	(*stream) << TEXT("<?xml ");

	if ( !version.empty() )
	{
		(*stream) << TEXT("version=\"");
		PutString( version, stream );
		(*stream) << TEXT("\" ");
	}
	if ( !encoding.empty() )
	{
		(*stream) << TEXT("encoding=\"");
		PutString( encoding, stream );
		(*stream ) << TEXT("\" ");
	}
	if ( !standalone.empty() )
	{
		(*stream) << TEXT("standalone=\"");
		PutString( standalone, stream );
		(*stream) << TEXT("\" ");
	}
	(*stream) << TEXT("?>");
}

TiXmlNode* TiXmlDeclaration::Clone() const
{	
	TiXmlDeclaration* clone = new TiXmlDeclaration();

	if ( !clone )
		return 0;

	CopyToClone( clone );
	clone->version = version;
	clone->encoding = encoding;
	clone->standalone = standalone;
	return clone;
}


void TiXmlUnknown::Print( std::string& outputStream, int depth ) const
{
	for ( int i=0; i<depth; i++ )
		outputStream += "    ";
		//generic_fprintf( cfile, TEXT("    ") );

	outputStream += wstring2string(value, CP_UTF8);
	//generic_fprintf( cfile, TEXT("%ls"), value.c_str() );
}

void TiXmlUnknown::StreamOut( TIXML_OSTREAM * stream ) const
{
	(*stream) << TEXT("<") << value << TEXT(">");		// Don't use entities hear! It is unknown.
}

TiXmlNode* TiXmlUnknown::Clone() const
{
	TiXmlUnknown* clone = new TiXmlUnknown();

	if ( !clone )
		return 0;

	CopyToClone( clone );
	return clone;
}


TiXmlAttributeSet::TiXmlAttributeSet()
{
	sentinel.next = &sentinel;
	sentinel.prev = &sentinel;
}


TiXmlAttributeSet::~TiXmlAttributeSet()
{
	assert( sentinel.next == &sentinel );
	assert( sentinel.prev == &sentinel );
}


void TiXmlAttributeSet::Add( TiXmlAttribute* addMe )
{
	assert( !Find( addMe->Name() ) );	// Shouldn't be multiply adding to the set.

	addMe->next = &sentinel;
	addMe->prev = sentinel.prev;

	sentinel.prev->next = addMe;
	sentinel.prev      = addMe;
}

void TiXmlAttributeSet::Remove( TiXmlAttribute* removeMe )
{
	TiXmlAttribute* node;

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

TiXmlAttribute*	TiXmlAttributeSet::Find( const TCHAR * name ) const
{
	TiXmlAttribute* node;

	for( node = sentinel.next; node != &sentinel; node = node->next )
	{
		if ( node->name == name )
			return node;
	}
	return 0;
}


#ifdef TIXML_USE_STL	
TIXML_ISTREAM & operator >> (TIXML_ISTREAM & in, TiXmlNode & base)
{
	TIXML_STRING tag;
	tag.reserve( 8 * 1000 );
	base.StreamIn( &in, &tag );

	base.Parse( tag.c_str(), 0 );
	return in;
}
#endif


TIXML_OSTREAM & operator<< (TIXML_OSTREAM & out, const TiXmlNode & base)
{
	base.StreamOut (& out);
	return out;
}


#ifdef TIXML_USE_STL	
generic_string & operator<< (generic_string& out, const TiXmlNode& base )
{
	
   //std::ostringstream os_stream( std::ostringstream::out );
	std::basic_ostringstream<TCHAR> os_stream( std::ostringstream::out );
   base.StreamOut( &os_stream );
   
   out.append( os_stream.str() );
   return out;
}
#endif


TiXmlHandle TiXmlHandle::FirstChild() const
{
	if ( node )
	{
		TiXmlNode* child = node->FirstChild();
		if ( child )
			return TiXmlHandle( child );
	}
	return TiXmlHandle( 0 );
}


TiXmlHandle TiXmlHandle::FirstChild( const TCHAR * value ) const
{
	if ( node )
	{
		TiXmlNode* child = node->FirstChild( value );
		if ( child )
			return TiXmlHandle( child );
	}
	return TiXmlHandle( 0 );
}


TiXmlHandle TiXmlHandle::FirstChildElement() const
{
	if ( node )
	{
		TiXmlElement* child = node->FirstChildElement();
		if ( child )
			return TiXmlHandle( child );
	}
	return TiXmlHandle( 0 );
}


TiXmlHandle TiXmlHandle::FirstChildElement( const TCHAR * value ) const
{
	if ( node )
	{
		TiXmlElement* child = node->FirstChildElement( value );
		if ( child )
			return TiXmlHandle( child );
	}
	return TiXmlHandle( 0 );
}

TiXmlHandle TiXmlHandle::Child( int count ) const
{
	if ( node )
	{
		int i;
		TiXmlNode* child = node->FirstChild();
		for (	i=0;
				child && i<count;
				child = child->NextSibling(), ++i )
		{
			// nothing
		}
		if ( child )
			return TiXmlHandle( child );
	}
	return TiXmlHandle( 0 );
}


TiXmlHandle TiXmlHandle::Child( const TCHAR* value, int count ) const
{
	if ( node )
	{
		int i;
		TiXmlNode* child = node->FirstChild( value );
		for (	i=0;
				child && i<count;
				child = child->NextSibling( value ), ++i )
		{
			// nothing
		}
		if ( child )
			return TiXmlHandle( child );
	}
	return TiXmlHandle( 0 );
}


TiXmlHandle TiXmlHandle::ChildElement( int count ) const
{
	if ( node )
	{
		int i;
		TiXmlElement* child = node->FirstChildElement();
		for (	i=0;
				child && i<count;
				child = child->NextSiblingElement(), ++i )
		{
			// nothing
		}
		if ( child )
			return TiXmlHandle( child );
	}
	return TiXmlHandle( 0 );
}


TiXmlHandle TiXmlHandle::ChildElement( const TCHAR* value, int count ) const
{
	if ( node )
	{
		int i;
		TiXmlElement* child = node->FirstChildElement( value );
		for (	i=0;
				child && i<count;
				child = child->NextSiblingElement( value ), ++i )
		{
			// nothing
		}
		if ( child )
			return TiXmlHandle( child );
	}
	return TiXmlHandle( 0 );
}
