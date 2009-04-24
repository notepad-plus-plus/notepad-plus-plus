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


#ifndef TINYXMLA_INCLUDED
#define TINYXMLA_INCLUDED

#ifdef _MSC_VER
#pragma warning( disable : 4530 )
#pragma warning( disable : 4786 )
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <windows.h>
#include "Common.h"

// Help out windows:
#if defined( _DEBUG ) && !defined( DEBUG )
#define DEBUG
#endif

#if defined( DEBUG ) && defined( _MSC_VER )
#include <windows.h>
#define TIXMLA_LOG OutputDebugString
#else
#define TIXMLA_LOG printf
#endif

#ifdef TIXMLA_USE_STL
	#include <string>
 	#include <iostream>
    //#include <ostream>
	#define TIXMLA_STRING	std::string
	#define TIXMLA_ISTREAM	std::istream
	#define TIXMLA_OSTREAM	std::ostream
#else
	#include "tinystrA.h"
	#define TIXMLA_STRING	TiXmlStringA
	#define TIXMLA_OSTREAM	TiXmlOutStreamA
#endif

class TiXmlDocumentA;
class TiXmlElementA;
class TiXmlCommentA;
class TiXmlUnknownA;
class TiXmlAttributeA;
class TiXmlTextA;
class TiXmlDeclarationA;

class TiXmlParsingDataA;

/*	Internal structure for tracking location of items 
	in the XML file.
*/
struct TiXmlCursorA
{
	TiXmlCursorA()		{ Clear(); }
	void Clear()		{ row = col = -1; }

	int row;	// 0 based.
	int col;	// 0 based.
};


// Only used by Attribute::Query functions
enum 
{ 
	TIXMLA_SUCCESS,
	TIXMLA_NO_ATTRIBUTE,
	TIXMLA_WRONG_TYPE
};

/** TiXmlBaseA is a base class for every class in TinyXml.
	It does little except to establish that TinyXml classes
	can be printed and provide some utility functions.

	In XML, the document and elements can contain
	other elements and other types of nodes.

	@verbatim
	A Document can contain:	Element	(container or leaf)
							Comment (leaf)
							Unknown (leaf)
							Declaration( leaf )

	An Element can contain:	Element (container or leaf)
							Text	(leaf)
							Attributes (not on tree)
							Comment (leaf)
							Unknown (leaf)

	A Decleration contains: Attributes (not on tree)
	@endverbatim
*/
class TiXmlBaseA
{
	friend class TiXmlNodeA;
	friend class TiXmlElementA;
	friend class TiXmlDocumentA;

public:
	TiXmlBaseA()								{}
	virtual ~TiXmlBaseA()					{}

	/**	All TinyXml classes can print themselves to a filestream.
		This is a formatted print, and will insert tabs and newlines.
		
		(For an unformatted stream, use the << operator.)
	*/
	virtual void Print( FILE* cfile, int depth ) const = 0;

	/**	The world does not agree on whether white space should be kept or
		not. In order to make everyone happy, these global, static functions
		are provided to set whether or not TinyXml will condense all white space
		into a single space or not. The default is to condense. Note changing this
		values is not thread safe.
	*/
	static void SetCondenseWhiteSpace( bool condense )		{ condenseWhiteSpace = condense; }

	/// Return the current white space setting.
	static bool IsWhiteSpaceCondensed()						{ return condenseWhiteSpace; }

	/** Return the position, in the original source file, of this node or attribute.
		The row and column are 1-based. (That is the first row and first column is
		1,1). If the returns values are 0 or less, then the parser does not have
		a row and column value.

		Generally, the row and column value will be set when the TiXmlDocumentA::Load(),
		TiXmlDocumentA::LoadFile(), or any TiXmlNodeA::Parse() is called. It will NOT be set
		when the DOM was created from operator>>.

		The values reflect the initial load. Once the DOM is modified programmatically
		(by adding or changing nodes and attributes) the new values will NOT update to
		reflect changes in the document.

		There is a minor performance cost to computing the row and column. Computation
		can be disabled if TiXmlDocumentA::SetTabSize() is called with 0 as the value.

		@sa TiXmlDocumentA::SetTabSize()
	*/
	int Row() const			{ return location.row + 1; }
	int Column() const		{ return location.col + 1; }	///< See Row()

protected:
	// See STL_STRING_BUG
	// Utility class to overcome a bug.
	class StringToBuffer
	{
	  public:
		StringToBuffer( const TIXMLA_STRING& str );
		~StringToBuffer();
		char* buffer;
	};

	static const char*	SkipWhiteSpace( const char* );
	inline static bool	IsWhiteSpace( int c )		{ return ( isspace( c ) || c == '\n' || c == '\r' ); }

	virtual void StreamOut (TIXMLA_OSTREAM *) const = 0;

	#ifdef TIXMLA_USE_STL
	    static bool	StreamWhiteSpace( TIXMLA_ISTREAM * in, TIXMLA_STRING * tag );
	    static bool StreamTo( TIXMLA_ISTREAM * in, int character, TIXMLA_STRING * tag );
	#endif

	/*	Reads an XML name into the string provided. Returns
		a pointer just past the last character of the name,
		or 0 if the function has an error.
	*/
	static const char* ReadName( const char* p, TIXMLA_STRING* name );

	/*	Reads text. Returns a pointer past the given end tag.
		Wickedly complex options, but it keeps the (sensitive) code in one place.
	*/
	static const char* ReadText(	const char* in,				// where to start
									TIXMLA_STRING* text,			// the string read
									bool ignoreWhiteSpace,		// whether to keep the white space
									const char* endTag,			// what ends this text
									bool ignoreCase );			// whether to ignore case in the end tag

	virtual const char* Parse( const char* p, TiXmlParsingDataA* data ) = 0;

	// If an entity has been found, transform it into a character.
	static const char* GetEntity( const char* in, char* value );

	// Get a character, while interpreting entities.
	inline static const char* GetChar( const char* p, char* _value )
	{
		assert( p );
		if ( *p == '&' )
		{
			return GetEntity( p, _value );
		}
		else
		{
			*_value = *p;
			return p+1;
		}
	}

	// Puts a string to a stream, expanding entities as it goes.
	// Note this should not contian the '<', '>', etc, or they will be transformed into entities!
	static void PutString( const TIXMLA_STRING& str, TIXMLA_OSTREAM* out );

	static void PutString( const TIXMLA_STRING& str, TIXMLA_STRING* out );

	// Return true if the next characters in the stream are any of the endTag sequences.
	static bool StringEqual(	const char* p,
								const char* endTag,
								bool ignoreCase );


	enum
	{
		TIXMLA_NO_ERROR = 0,
		TIXMLA_ERROR,
		TIXMLA_ERROR_OPENING_FILE,
		TIXMLA_ERROR_OUT_OF_MEMORY,
		TIXMLA_ERROR_PARSING_ELEMENT,
		TIXMLA_ERROR_FAILED_TO_READ_ELEMENT_NAME,
		TIXMLA_ERROR_READING_ELEMENT_VALUE,
		TIXMLA_ERROR_READING_ATTRIBUTES,
		TIXMLA_ERROR_PARSING_EMPTY,
		TIXMLA_ERROR_READING_END_TAG,
		TIXMLA_ERROR_PARSING_UNKNOWN,
		TIXMLA_ERROR_PARSING_COMMENT,
		TIXMLA_ERROR_PARSING_DECLARATION,
		TIXMLA_ERROR_DOCUMENT_EMPTY,

		TIXMLA_ERROR_STRING_COUNT
	};
	static const char* errorString[ TIXMLA_ERROR_STRING_COUNT ];

	TiXmlCursorA location;

private:
	struct Entity
	{
		const char*     str;
		unsigned int	strLength;
		char		    chr;
	};
	enum
	{
		NUM_ENTITY = 5,
		MAX_ENTITY_LENGTH = 6

	};
	static Entity entity[ NUM_ENTITY ];
	static bool condenseWhiteSpace;
};


/** The parent class for everything in the Document Object Model.
	(Except for attributes).
	Nodes have siblings, a parent, and children. A node can be
	in a document, or stand on its own. The type of a TiXmlNodeA
	can be queried, and it can be cast to its more defined type.
*/
class TiXmlNodeA : public TiXmlBaseA
{
	friend class TiXmlDocumentA;
	friend class TiXmlElementA;

public:
	#ifdef TIXMLA_USE_STL	

	    /** An input stream operator, for every class. Tolerant of newlines and
		    formatting, but doesn't expect them.
	    */
	    friend std::istream& operator >> (std::istream& in, TiXmlNodeA& base);

	    /** An output stream operator, for every class. Note that this outputs
		    without any newlines or formatting, as opposed to Print(), which
		    includes tabs and new lines.

		    The operator<< and operator>> are not completely symmetric. Writing
		    a node to a stream is very well defined. You'll get a nice stream
		    of output, without any extra whitespace or newlines.
		    
		    But reading is not as well defined. (As it always is.) If you create
		    a TiXmlElementA (for example) and read that from an input stream,
		    the text needs to define an element or junk will result. This is
		    true of all input streams, but it's worth keeping in mind.

		    A TiXmlDocumentA will read nodes until it reads a root element, and
			all the children of that root element.
	    */	
	    friend std::ostream& operator<< (std::ostream& out, const TiXmlNodeA& base);

		/// Appends the XML node or attribute to a std::string.
		friend std::string& operator<< (std::string& out, const TiXmlNodeA& base );

	#else
	    // Used internally, not part of the public API.
	    friend TIXMLA_OSTREAM& operator<< (TIXMLA_OSTREAM& out, const TiXmlNodeA& base);
	#endif

	/** The types of XML nodes supported by TinyXml. (All the
			unsupported types are picked up by UNKNOWN.)
	*/
	enum NodeType
	{
		DOCUMENT,
		ELEMENT,
		COMMENT,
		UNKNOWN,
		TEXT,
		DECLARATION,
		TYPECOUNT
	};

	virtual ~TiXmlNodeA();

	/** The meaning of 'value' changes for the specific type of
		TiXmlNodeA.
		@verbatim
		Document:	filename of the xml file
		Element:	name of the element
		Comment:	the comment text
		Unknown:	the tag contents
		Text:		the text string
		@endverbatim

		The subclasses will wrap this function.
	*/
	const char * Value() const { return value.c_str (); }

	/** Changes the value of the node. Defined as:
		@verbatim
		Document:	filename of the xml file
		Element:	name of the element
		Comment:	the comment text
		Unknown:	the tag contents
		Text:		the text string
		@endverbatim
	*/
	void SetValue(const char * _value) { value = _value;}

    #ifdef TIXMLA_USE_STL
	/// STL std::string form.
	void SetValue( const std::string& _value )    
	{	  
		StringToBuffer buf( _value );
		SetValue( buf.buffer ? buf.buffer : "" );    	
	}	
	#endif

	/// Delete all the children of this node. Does not affect 'this'.
	void Clear();

	/// One step up the DOM.
	TiXmlNodeA* Parent() const					{ return parent; }

	TiXmlNodeA* FirstChild()	const	{ return firstChild; }		///< The first child of this node. Will be null if there are no children.
	TiXmlNodeA* FirstChild( const char * value ) const;			///< The first child of this node with the matching 'value'. Will be null if none found.

	TiXmlNodeA* LastChild() const	{ return lastChild; }		/// The last child of this node. Will be null if there are no children.
	TiXmlNodeA* LastChild( const char * value ) const;			/// The last child of this node matching 'value'. Will be null if there are no children.

    #ifdef TIXMLA_USE_STL
	TiXmlNodeA* FirstChild( const std::string& _value ) const	{	return FirstChild (_value.c_str ());	}	///< STL std::string form.
	TiXmlNodeA* LastChild( const std::string& _value ) const		{	return LastChild (_value.c_str ());	}	///< STL std::string form.
	#endif

	/** An alternate way to walk the children of a node.
		One way to iterate over nodes is:
		@verbatim
			for( child = parent->FirstChild(); child; child = child->NextSibling() )
		@endverbatim

		IterateChildren does the same thing with the syntax:
		@verbatim
			child = 0;
			while( child = parent->IterateChildren( child ) )
		@endverbatim

		IterateChildren takes the previous child as input and finds
		the next one. If the previous child is null, it returns the
		first. IterateChildren will return null when done.
	*/
	TiXmlNodeA* IterateChildren( TiXmlNodeA* previous ) const;

	/// This flavor of IterateChildren searches for children with a particular 'value'
	TiXmlNodeA* IterateChildren( const char * value, TiXmlNodeA* previous ) const;

    #ifdef TIXMLA_USE_STL
	TiXmlNodeA* IterateChildren( const std::string& _value, TiXmlNodeA* previous ) const	{	return IterateChildren (_value.c_str (), previous);	}	///< STL std::string form.
	#endif

	/** Add a new node related to this. Adds a child past the LastChild.
		Returns a pointer to the new object or NULL if an error occured.
	*/
	TiXmlNodeA* InsertEndChild( const TiXmlNodeA& addThis );


	/** Add a new node related to this. Adds a child past the LastChild.

		NOTE: the node to be added is passed by pointer, and will be
		henceforth owned (and deleted) by tinyXml. This method is efficient
		and avoids an extra copy, but should be used with care as it
		uses a different memory model than the other insert functions.

		@sa InsertEndChild
	*/
	TiXmlNodeA* LinkEndChild( TiXmlNodeA* addThis );

	/** Add a new node related to this. Adds a child before the specified child.
		Returns a pointer to the new object or NULL if an error occured.
	*/
	TiXmlNodeA* InsertBeforeChild( TiXmlNodeA* beforeThis, const TiXmlNodeA& addThis );

	/** Add a new node related to this. Adds a child after the specified child.
		Returns a pointer to the new object or NULL if an error occured.
	*/
	TiXmlNodeA* InsertAfterChild(  TiXmlNodeA* afterThis, const TiXmlNodeA& addThis );

	/** Replace a child of this node.
		Returns a pointer to the new object or NULL if an error occured.
	*/
	TiXmlNodeA* ReplaceChild( TiXmlNodeA* replaceThis, const TiXmlNodeA& withThis );

	/// Delete a child of this node.
	bool RemoveChild( TiXmlNodeA* removeThis );

	/// Navigate to a sibling node.
	TiXmlNodeA* PreviousSibling() const			{ return prev; }

	/// Navigate to a sibling node.
	TiXmlNodeA* PreviousSibling( const char * ) const;

    #ifdef TIXMLA_USE_STL
	TiXmlNodeA* PreviousSibling( const std::string& _value ) const	{	return PreviousSibling (_value.c_str ());	}	///< STL std::string form.
	TiXmlNodeA* NextSibling( const std::string& _value) const		{	return NextSibling (_value.c_str ());	}	///< STL std::string form.
	#endif

	/// Navigate to a sibling node.
	TiXmlNodeA* NextSibling() const				{ return next; }

	/// Navigate to a sibling node with the given 'value'.
	TiXmlNodeA* NextSibling( const char * ) const;

	/** Convenience function to get through elements.
		Calls NextSibling and ToElement. Will skip all non-Element
		nodes. Returns 0 if there is not another element.
	*/
	TiXmlElementA* NextSiblingElement() const;

	/** Convenience function to get through elements.
		Calls NextSibling and ToElement. Will skip all non-Element
		nodes. Returns 0 if there is not another element.
	*/
	TiXmlElementA* NextSiblingElement( const char * ) const;

    #ifdef TIXMLA_USE_STL
	TiXmlElementA* NextSiblingElement( const std::string& _value) const	{	return NextSiblingElement (_value.c_str ());	}	///< STL std::string form.
	#endif

	/// Convenience function to get through elements.
	TiXmlElementA* FirstChildElement()	const;

	/// Convenience function to get through elements.
	TiXmlElementA* FirstChildElement( const char * value ) const;

    #ifdef TIXMLA_USE_STL
	TiXmlElementA* FirstChildElement( const std::string& _value ) const	{	return FirstChildElement (_value.c_str ());	}	///< STL std::string form.
	#endif

	/** Query the type (as an enumerated value, above) of this node.
		The possible types are: DOCUMENT, ELEMENT, COMMENT,
								UNKNOWN, TEXT, and DECLARATION.
	*/
	virtual int Type() const	{ return type; }

	/** Return a pointer to the Document this node lives in.
		Returns null if not in a document.
	*/
	TiXmlDocumentA* GetDocument() const;

	/// Returns true if this node has no children.
	bool NoChildren() const						{ return !firstChild; }

	TiXmlDocumentA* ToDocument()	const		{ return ( this && type == DOCUMENT ) ? (TiXmlDocumentA*) this : 0; } ///< Cast to a more defined type. Will return null not of the requested type.
	TiXmlElementA*  ToElement() const		{ return ( this && type == ELEMENT  ) ? (TiXmlElementA*)  this : 0; } ///< Cast to a more defined type. Will return null not of the requested type.
	TiXmlCommentA*  ToComment() const		{ return ( this && type == COMMENT  ) ? (TiXmlCommentA*)  this : 0; } ///< Cast to a more defined type. Will return null not of the requested type.
	TiXmlUnknownA*  ToUnknown() const		{ return ( this && type == UNKNOWN  ) ? (TiXmlUnknownA*)  this : 0; } ///< Cast to a more defined type. Will return null not of the requested type.
	TiXmlTextA*	   ToText()    const		{ return ( this && type == TEXT     ) ? (TiXmlTextA*)     this : 0; } ///< Cast to a more defined type. Will return null not of the requested type.
	TiXmlDeclarationA* ToDeclaration() const	{ return ( this && type == DECLARATION ) ? (TiXmlDeclarationA*) this : 0; } ///< Cast to a more defined type. Will return null not of the requested type.

	virtual TiXmlNodeA* Clone() const = 0;

	void  SetUserData( void* user )			{ userData = user; }
	void* GetUserData()						{ return userData; }

protected:
	TiXmlNodeA( NodeType type );

	#ifdef TIXMLA_USE_STL
	    // The real work of the input operator.
	    virtual void StreamIn( TIXMLA_ISTREAM* in, TIXMLA_STRING* tag ) = 0;
	#endif

	// Figure out what is at *p, and parse it. Returns null if it is not an xml node.
	TiXmlNodeA* Identify( const char* start );
	void CopyToClone( TiXmlNodeA* target ) const	{ target->SetValue (value.c_str() );
												  target->userData = userData; }

	// Internal Value function returning a TIXMLA_STRING
	TIXMLA_STRING SValue() const	{ return value ; }

	TiXmlNodeA*		parent;
	NodeType		type;

	TiXmlNodeA*		firstChild;
	TiXmlNodeA*		lastChild;

	TIXMLA_STRING	value;

	TiXmlNodeA*		prev;
	TiXmlNodeA*		next;
	void*			userData;
};


/** An attribute is a name-value pair. Elements have an arbitrary
	number of attributes, each with a unique name.

	@note The attributes are not TiXmlNodes, since they are not
		  part of the tinyXML document object model. There are other
		  suggested ways to look at this problem.
*/
class TiXmlAttributeA : public TiXmlBaseA
{
	friend class TiXmlAttributeSetA;

public:
	/// Construct an empty attribute.
	TiXmlAttributeA()
	{
		document = 0;
		prev = next = 0;
	}

	#ifdef TIXMLA_USE_STL
	/// std::string constructor.
	TiXmlAttributeA( const std::string& _name, const std::string& _value )
	{
		name = _name;
		value = _value;
		document = 0;
		prev = next = 0;
	}
	#endif

	/// Construct an attribute with a name and value.
	TiXmlAttributeA( const char * _name, const char * _value )
	{
		name = _name;
		value = _value;
		document = 0;
		prev = next = 0;
	}

	const char*		Name()  const		{ return name.c_str (); }		///< Return the name of this attribute.
	const char*		Value() const		{ return value.c_str (); }		///< Return the value of this attribute.
	const int       IntValue() const;									///< Return the value of this attribute, converted to an integer.
	const double	DoubleValue() const;								///< Return the value of this attribute, converted to a double.

	/** QueryIntValue examines the value string. It is an alternative to the
		IntValue() method with richer error checking.
		If the value is an integer, it is stored in 'value' and 
		the call returns TIXMLA_SUCCESS. If it is not
		an integer, it returns TIXMLA_WRONG_TYPE.

		A specialized but useful call. Note that for success it returns 0,
		which is the opposite of almost all other TinyXml calls.
	*/
	int QueryIntValue( int* value ) const;
	/// QueryDoubleValue examines the value string. See QueryIntValue().
	int QueryDoubleValue( double* value ) const;

	void SetName( const char* _name )	{ name = _name; }				///< Set the name of this attribute.
	void SetValue( const char* _value )	{ value = _value; }				///< Set the value.

	void SetIntValue( int value );										///< Set the value from an integer.
	void SetDoubleValue( double value );								///< Set the value from a double.

    #ifdef TIXMLA_USE_STL
	/// STL std::string form.
	void SetName( const std::string& _name )	
	{	
		StringToBuffer buf( _name );
		SetName ( buf.buffer ? buf.buffer : "error" );	
	}
	/// STL std::string form.	
	void SetValue( const std::string& _value )	
	{	
		StringToBuffer buf( _value );
		SetValue( buf.buffer ? buf.buffer : "error" );	
	}
	#endif

	/// Get the next sibling attribute in the DOM. Returns null at end.
	TiXmlAttributeA* Next() const;
	/// Get the previous sibling attribute in the DOM. Returns null at beginning.
	TiXmlAttributeA* Previous() const;

	bool operator==( const TiXmlAttributeA& rhs ) const { return rhs.name == name; }
	bool operator<( const TiXmlAttributeA& rhs )	 const { return name < rhs.name; }
	bool operator>( const TiXmlAttributeA& rhs )  const { return name > rhs.name; }

	/*	[internal use]
		Attribtue parsing starts: first letter of the name
						 returns: the next char after the value end quote
	*/
	virtual const char* Parse( const char* p, TiXmlParsingDataA* data );

	// [internal use]
	virtual void Print( FILE* cfile, int depth ) const;

	virtual void StreamOut( TIXMLA_OSTREAM * out ) const;
	// [internal use]
	// Set the document pointer so the attribute can report errors.
	void SetDocument( TiXmlDocumentA* doc )	{ document = doc; }

private:
	TiXmlDocumentA*	document;	// A pointer back to a document, for error reporting.
	TIXMLA_STRING name;
	TIXMLA_STRING value;
	TiXmlAttributeA*	prev;
	TiXmlAttributeA*	next;
};


/*	A class used to manage a group of attributes.
	It is only used internally, both by the ELEMENT and the DECLARATION.
	
	The set can be changed transparent to the Element and Declaration
	classes that use it, but NOT transparent to the Attribute
	which has to implement a next() and previous() method. Which makes
	it a bit problematic and prevents the use of STL.

	This version is implemented with circular lists because:
		- I like circular lists
		- it demonstrates some independence from the (typical) doubly linked list.
*/
class TiXmlAttributeSetA
{
public:
	TiXmlAttributeSetA();
	~TiXmlAttributeSetA();

	void Add( TiXmlAttributeA* attribute );
	void Remove( TiXmlAttributeA* attribute );

	TiXmlAttributeA* First() const	{ return ( sentinel.next == &sentinel ) ? 0 : sentinel.next; }
	TiXmlAttributeA* Last()  const	{ return ( sentinel.prev == &sentinel ) ? 0 : sentinel.prev; }
	TiXmlAttributeA*	Find( const char * name ) const;

private:
	TiXmlAttributeA sentinel;
};


/** The element is a container class. It has a value, the element name,
	and can contain other elements, text, comments, and unknowns.
	Elements also contain an arbitrary number of attributes.
*/
class TiXmlElementA : public TiXmlNodeA
{
public:
	/// Construct an element.
	TiXmlElementA (const char * in_value);

	#ifdef TIXMLA_USE_STL
	/// std::string constructor.
	TiXmlElementA( const std::string& _value ) : 	TiXmlNodeA( TiXmlNodeA::ELEMENT )
	{
		firstChild = lastChild = 0;
		value = _value;
	}
	#endif

	virtual ~TiXmlElementA();

	/** Given an attribute name, Attribute() returns the value
		for the attribute of that name, or null if none exists.
	*/
	const char* Attribute( const char* name ) const;

	/** Given an attribute name, Attribute() returns the value
		for the attribute of that name, or null if none exists.
		If the attribute exists and can be converted to an integer,
		the integer value will be put in the return 'i', if 'i'
		is non-null.
	*/
	const char* Attribute( const char* name, int* i ) const;

	/** Given an attribute name, Attribute() returns the value
		for the attribute of that name, or null if none exists.
		If the attribute exists and can be converted to an double,
		the double value will be put in the return 'd', if 'd'
		is non-null.
	*/
	const char* Attribute( const char* name, double* d ) const;

	/** QueryIntAttribute examines the attribute - it is an alternative to the
		Attribute() method with richer error checking.
		If the attribute is an integer, it is stored in 'value' and 
		the call returns TIXMLA_SUCCESS. If it is not
		an integer, it returns TIXMLA_WRONG_TYPE. If the attribute
		does not exist, then TIXMLA_NO_ATTRIBUTE is returned.
	*/	
	int QueryIntAttribute( const char* name, int* value ) const;
	/// QueryDoubleAttribute examines the attribute - see QueryIntAttribute().
	int QueryDoubleAttribute( const char* name, double* value ) const;

	/** Sets an attribute of name to a given value. The attribute
		will be created if it does not exist, or changed if it does.
	*/
	void SetAttribute( const char* name, const char * value );

    #ifdef TIXMLA_USE_STL
	const char* Attribute( const std::string& name ) const				{ return Attribute( name.c_str() ); }
	const char* Attribute( const std::string& name, int* i ) const		{ return Attribute( name.c_str(), i ); }

	/// STL std::string form.
	void SetAttribute( const std::string& name, const std::string& _value )	
	{	
		StringToBuffer n( name );
		StringToBuffer v( _value );
		if ( n.buffer && v.buffer )
			SetAttribute (n.buffer, v.buffer );	
	}	
	///< STL std::string form.
	void SetAttribute( const std::string& name, int _value )	
	{	
		StringToBuffer n( name );
		if ( n.buffer )
			SetAttribute (n.buffer, _value);	
	}	
	#endif

	/** Sets an attribute of name to a given value. The attribute
		will be created if it does not exist, or changed if it does.
	*/
	void SetAttribute( const char * name, int value );

	/** Deletes an attribute with the given name.
	*/
	void RemoveAttribute( const char * name );
    #ifdef TIXMLA_USE_STL
	void RemoveAttribute( const std::string& name )	{	RemoveAttribute (name.c_str ());	}	///< STL std::string form.
	#endif

	TiXmlAttributeA* FirstAttribute() const	{ return attributeSet.First(); }		///< Access the first attribute in this element.
	TiXmlAttributeA* LastAttribute()	const 	{ return attributeSet.Last(); }		///< Access the last attribute in this element.

	// [internal use] Creates a new Element and returs it.
	virtual TiXmlNodeA* Clone() const;
	// [internal use]

	virtual void Print( FILE* cfile, int depth ) const;

protected:

	// Used to be public [internal use]
	#ifdef TIXMLA_USE_STL
	    virtual void StreamIn( TIXMLA_ISTREAM * in, TIXMLA_STRING * tag );
	#endif
	virtual void StreamOut( TIXMLA_OSTREAM * out ) const;

	/*	[internal use]
		Attribtue parsing starts: next char past '<'
						 returns: next char past '>'
	*/
	virtual const char* Parse( const char* p, TiXmlParsingDataA* data );

	/*	[internal use]
		Reads the "value" of the element -- another element, or text.
		This should terminate with the current end tag.
	*/
	const char* ReadValue( const char* in, TiXmlParsingDataA* prevData );

private:
	TiXmlAttributeSetA attributeSet;
};


/**	An XML comment.
*/
class TiXmlCommentA : public TiXmlNodeA
{
public:
	/// Constructs an empty comment.
	TiXmlCommentA() : TiXmlNodeA( TiXmlNodeA::COMMENT ) {}
	virtual ~TiXmlCommentA()	{}

	// [internal use] Creates a new Element and returs it.
	virtual TiXmlNodeA* Clone() const;
	// [internal use]
	virtual void Print( FILE* cfile, int depth ) const;
protected:
	// used to be public
	#ifdef TIXMLA_USE_STL
	    virtual void StreamIn( TIXMLA_ISTREAM * in, TIXMLA_STRING * tag );
	#endif
	virtual void StreamOut( TIXMLA_OSTREAM * out ) const;
	/*	[internal use]
		Attribtue parsing starts: at the ! of the !--
						 returns: next char past '>'
	*/
	virtual const char* Parse( const char* p, TiXmlParsingDataA* data );
};


/** XML text. Contained in an element.
*/
class TiXmlTextA : public TiXmlNodeA
{
	friend class TiXmlElementA;
public:
	/// Constructor.
	TiXmlTextA (const char * initValue) : TiXmlNodeA (TiXmlNodeA::TEXT)
	{
		SetValue( initValue );
	}
	virtual ~TiXmlTextA() {}

	#ifdef TIXMLA_USE_STL
	/// Constructor.
	TiXmlTextA( const std::string& initValue ) : TiXmlNodeA (TiXmlNodeA::TEXT)
	{
		SetValue( initValue );
	}
	#endif

	// [internal use]
	virtual void Print( FILE* cfile, int depth ) const;

protected :
	// [internal use] Creates a new Element and returns it.
	virtual TiXmlNodeA* Clone() const;
	virtual void StreamOut ( TIXMLA_OSTREAM * out ) const;
	// [internal use]
	bool Blank() const;	// returns true if all white space and new lines
	/*	[internal use]
			Attribtue parsing starts: First char of the text
							 returns: next char past '>'
	*/
	virtual const char* Parse( const char* p, TiXmlParsingDataA* data );
	// [internal use]
	#ifdef TIXMLA_USE_STL
	    virtual void StreamIn( TIXMLA_ISTREAM * in, TIXMLA_STRING * tag );
	#endif
};


/** In correct XML the declaration is the first entry in the file.
	@verbatim
		<?xml version="1.0" standalone="yes"?>
	@endverbatim

	TinyXml will happily read or write files without a declaration,
	however. There are 3 possible attributes to the declaration:
	version, encoding, and standalone.

	Note: In this version of the code, the attributes are
	handled as special cases, not generic attributes, simply
	because there can only be at most 3 and they are always the same.
*/
class TiXmlDeclarationA : public TiXmlNodeA
{
public:
	/// Construct an empty declaration.
	TiXmlDeclarationA()   : TiXmlNodeA( TiXmlNodeA::DECLARATION ) {}

#ifdef TIXMLA_USE_STL
	/// Constructor.
	TiXmlDeclarationA(	const std::string& _version,
						const std::string& _encoding,
						const std::string& _standalone )
			: TiXmlNodeA( TiXmlNodeA::DECLARATION )
	{
		version = _version;
		encoding = _encoding;
		standalone = _standalone;
	}
#endif

	/// Construct.
	TiXmlDeclarationA(	const char* _version,
						const char* _encoding,
						const char* _standalone );

	virtual ~TiXmlDeclarationA()	{}

	/// Version. Will return empty if none was found.
	const char * Version() const		{ return version.c_str (); }
	/// Encoding. Will return empty if none was found.
	const char * Encoding() const		{ return encoding.c_str (); }
	/// Is this a standalone document?
	const char * Standalone() const		{ return standalone.c_str (); }

	// [internal use] Creates a new Element and returs it.
	virtual TiXmlNodeA* Clone() const;
	// [internal use]
	virtual void Print( FILE* cfile, int depth ) const;

protected:
	// used to be public
	#ifdef TIXMLA_USE_STL
	    virtual void StreamIn( TIXMLA_ISTREAM * in, TIXMLA_STRING * tag );
	#endif
	virtual void StreamOut ( TIXMLA_OSTREAM * out) const;
	//	[internal use]
	//	Attribtue parsing starts: next char past '<'
	//					 returns: next char past '>'

	virtual const char* Parse( const char* p, TiXmlParsingDataA* data );

private:
	TIXMLA_STRING version;
	TIXMLA_STRING encoding;
	TIXMLA_STRING standalone;
};


/** Any tag that tinyXml doesn't recognize is saved as an
	unknown. It is a tag of text, but should not be modified.
	It will be written back to the XML, unchanged, when the file
	is saved.
*/
class TiXmlUnknownA : public TiXmlNodeA
{
public:
	TiXmlUnknownA() : TiXmlNodeA( TiXmlNodeA::UNKNOWN ) {}
	virtual ~TiXmlUnknownA() {}

	// [internal use]
	virtual TiXmlNodeA* Clone() const;
	// [internal use]
	virtual void Print( FILE* cfile, int depth ) const;
protected:
	#ifdef TIXMLA_USE_STL
	    virtual void StreamIn( TIXMLA_ISTREAM * in, TIXMLA_STRING * tag );
	#endif
	virtual void StreamOut ( TIXMLA_OSTREAM * out ) const;
	/*	[internal use]
		Attribute parsing starts: First char of the text
						 returns: next char past '>'
	*/
	virtual const char* Parse( const char* p, TiXmlParsingDataA* data );
};


/** Always the top level node. A document binds together all the
	XML pieces. It can be saved, loaded, and printed to the screen.
	The 'value' of a document node is the xml file name.
*/
class TiXmlDocumentA : public TiXmlNodeA
{
public:
	/// Create an empty document, that has no name.
	TiXmlDocumentA();
	/// Create a document with a name. The name of the document is also the filename of the xml.
	TiXmlDocumentA( const char * documentName );

	#ifdef TIXMLA_USE_STL
	/// Constructor.
	TiXmlDocumentA( const std::string& documentName ) :
	    TiXmlNodeA( TiXmlNodeA::DOCUMENT )
	{
        value = documentName;
		error = false;
	}
	#endif

	virtual ~TiXmlDocumentA() {}

	/** Load a file using the current document value.
		Returns true if successful. Will delete any existing
		document data before loading.
	*/
	bool LoadFile();
	/// Save a file using the current document value. Returns true if successful.
	bool SaveFile() const;
	/// Load a file using the given filename. Returns true if successful.
	bool LoadFile( const char * filename );
	/// Save a file using the given filename. Returns true if successful.
	bool SaveFile( const char * filename ) const;

	bool LoadUnicodeFilePath(const TCHAR* filename);

	#ifdef TIXMLA_USE_STL
	bool LoadFile( const std::string& filename )			///< STL std::string version.
	{
		StringToBuffer f( filename );
		return ( f.buffer && LoadFile( f.buffer ));
	}
	bool SaveFile( const std::string& filename ) const		///< STL std::string version.
	{
		StringToBuffer f( filename );
		return ( f.buffer && SaveFile( f.buffer ));
	}
	#endif

	/** Parse the given null terminated block of xml data.
	*/
	virtual const char* Parse( const char* p, TiXmlParsingDataA* data = 0 );

	/** Get the root element -- the only top level element -- of the document.
		In well formed XML, there should only be one. TinyXml is tolerant of
		multiple elements at the document level.
	*/
	TiXmlElementA* RootElement() const		{ return FirstChildElement(); }

	/** If an error occurs, Error will be set to true. Also,
		- The ErrorId() will contain the integer identifier of the error (not generally useful)
		- The ErrorDesc() method will return the name of the error. (very useful)
		- The ErrorRow() and ErrorCol() will return the location of the error (if known)
	*/	
	bool Error() const						{ return error; }

	/// Contains a textual (english) description of the error if one occurs.
	const char * ErrorDesc() const	{ return errorDesc.c_str (); }

	/** Generally, you probably want the error string ( ErrorDesc() ). But if you
		prefer the ErrorId, this function will fetch it.
	*/
	const int ErrorId()	const				{ return errorId; }

	/** Returns the location (if known) of the error. The first column is column 1, 
		and the first row is row 1. A value of 0 means the row and column wasn't applicable
		(memory errors, for example, have no row/column) or the parser lost the error. (An
		error in the error reporting, in that case.)

		@sa SetTabSize, Row, Column
	*/
	int ErrorRow()	{ return errorLocation.row+1; }
	int ErrorCol()	{ return errorLocation.col+1; }	///< The column where the error occured. See ErrorRow()

	/** By calling this method, with a tab size
		greater than 0, the row and column of each node and attribute is stored
		when the file is loaded. Very useful for tracking the DOM back in to
		the source file.

		The tab size is required for calculating the location of nodes. If not
		set, the default of 4 is used. The tabsize is set per document. Setting
		the tabsize to 0 disables row/column tracking.

		Note that row and column tracking is not supported when using operator>>.

		The tab size needs to be enabled before the parse or load. Correct usage:
		@verbatim
		TiXmlDocumentA doc;
		doc.SetTabSize( 8 );
		doc.Load( "myfile.xml" );
		@endverbatim

		@sa Row, Column
	*/
	void SetTabSize( int _tabsize )		{ tabsize = _tabsize; }

	int TabSize() const	{ return tabsize; }

	/** If you have handled the error, it can be reset with this call. The error
		state is automatically cleared if you Parse a new XML block.
	*/
	void ClearError()						{	error = false; 
												errorId = 0; 
												errorDesc = ""; 
												errorLocation.row = errorLocation.col = 0; 
												//errorLocation.last = 0; 
											}

	/** Dump the document to standard out. */
	void Print() const						{ Print( stdout, 0 ); }

	// [internal use]
	virtual void Print( FILE* cfile, int depth = 0 ) const;
	// [internal use]
	void SetError( int err, const char* errorLocation, TiXmlParsingDataA* prevData );

protected :
	virtual void StreamOut ( TIXMLA_OSTREAM * out) const;
	// [internal use]
	virtual TiXmlNodeA* Clone() const;
	#ifdef TIXMLA_USE_STL
	    virtual void StreamIn( TIXMLA_ISTREAM * in, TIXMLA_STRING * tag );
	#endif

private:
	bool error;
	int  errorId;
	TIXMLA_STRING errorDesc;
	int tabsize;
	TiXmlCursorA errorLocation;
};


/**
	A TiXmlHandleA is a class that wraps a node pointer with null checks; this is
	an incredibly useful thing. Note that TiXmlHandleA is not part of the TinyXml
	DOM structure. It is a separate utility class.

	Take an example:
	@verbatim
	<Document>
		<Element attributeA = "valueA">
			<Child attributeB = "value1" />
			<Child attributeB = "value2" />
		</Element>
	<Document>
	@endverbatim

	Assuming you want the value of "attributeB" in the 2nd "Child" element, it's very 
	easy to write a *lot* of code that looks like:

	@verbatim
	TiXmlElementA* root = document.FirstChildElement( "Document" );
	if ( root )
	{
		TiXmlElementA* element = root->FirstChildElement( "Element" );
		if ( element )
		{
			TiXmlElementA* child = element->FirstChildElement( "Child" );
			if ( child )
			{
				TiXmlElementA* child2 = child->NextSiblingElement( "Child" );
				if ( child2 )
				{
					// Finally do something useful.
	@endverbatim

	And that doesn't even cover "else" cases. TiXmlHandleA addresses the verbosity
	of such code. A TiXmlHandleA checks for null	pointers so it is perfectly safe 
	and correct to use:

	@verbatim
	TiXmlHandleA docHandle( &document );
	TiXmlElementA* child2 = docHandle.FirstChild( "Document" ).FirstChild( "Element" ).Child( "Child", 1 ).Element();
	if ( child2 )
	{
		// do something useful
	@endverbatim

	Which is MUCH more concise and useful.

	It is also safe to copy handles - internally they are nothing more than node pointers.
	@verbatim
	TiXmlHandleA handleCopy = handle;
	@endverbatim

	What they should not be used for is iteration:

	@verbatim
	int i=0; 
	while ( true )
	{
		TiXmlElementA* child = docHandle.FirstChild( "Document" ).FirstChild( "Element" ).Child( "Child", i ).Element();
		if ( !child )
			break;
		// do something
		++i;
	}
	@endverbatim

	It seems reasonable, but it is in fact two embedded while loops. The Child method is 
	a linear walk to find the element, so this code would iterate much more than it needs 
	to. Instead, prefer:

	@verbatim
	TiXmlElementA* child = docHandle.FirstChild( "Document" ).FirstChild( "Element" ).FirstChild( "Child" ).Element();

	for( child; child; child=child->NextSiblingElement() )
	{
		// do something
	}
	@endverbatim
*/
class TiXmlHandleA
{
public:
	/// Create a handle from any node (at any depth of the tree.) This can be a null pointer.
	TiXmlHandleA( TiXmlNodeA* node )			{ this->node = node; }
	/// Copy constructor
	TiXmlHandleA( const TiXmlHandleA& ref )	{ this->node = ref.node; }

	/// Return a handle to the first child node.
	TiXmlHandleA FirstChild() const;
	/// Return a handle to the first child node with the given name.
	TiXmlHandleA FirstChild( const char * value ) const;
	/// Return a handle to the first child element.
	TiXmlHandleA FirstChildElement() const;
	/// Return a handle to the first child element with the given name.
	TiXmlHandleA FirstChildElement( const char * value ) const;

	/** Return a handle to the "index" child with the given name. 
		The first child is 0, the second 1, etc.
	*/
	TiXmlHandleA Child( const char* value, int index ) const;
	/** Return a handle to the "index" child. 
		The first child is 0, the second 1, etc.
	*/
	TiXmlHandleA Child( int index ) const;
	/** Return a handle to the "index" child element with the given name. 
		The first child element is 0, the second 1, etc. Note that only TiXmlElements
		are indexed: other types are not counted.
	*/
	TiXmlHandleA ChildElement( const char* value, int index ) const;
	/** Return a handle to the "index" child element. 
		The first child element is 0, the second 1, etc. Note that only TiXmlElements
		are indexed: other types are not counted.
	*/
	TiXmlHandleA ChildElement( int index ) const;

	#ifdef TIXMLA_USE_STL
	TiXmlHandleA FirstChild( const std::string& _value ) const			{ return FirstChild( _value.c_str() ); }
	TiXmlHandleA FirstChildElement( const std::string& _value ) const		{ return FirstChildElement( _value.c_str() ); }

	TiXmlHandleA Child( const std::string& _value, int index ) const			{ return Child( _value.c_str(), index ); }
	TiXmlHandleA ChildElement( const std::string& _value, int index ) const	{ return ChildElement( _value.c_str(), index ); }
	#endif

	/// Return the handle as a TiXmlNodeA. This may return null.
	TiXmlNodeA* Node() const			{ return node; } 
	/// Return the handle as a TiXmlElementA. This may return null.
	TiXmlElementA* Element() const	{ return ( ( node && node->ToElement() ) ? node->ToElement() : 0 ); }
	/// Return the handle as a TiXmlTextA. This may return null.
	TiXmlTextA* Text() const			{ return ( ( node && node->ToText() ) ? node->ToText() : 0 ); }

private:
	TiXmlNodeA* node;
};


#endif

