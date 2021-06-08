/*
www.sourceforge.net/projects/tinyxml
Original file by Yves Berquin.

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
#include "tinyxmlA.h"
#endif TINYXMLA_INCLUDED
#include <cassert>

#ifndef TIXMLA_USE_STL

#ifndef _INCLUDED
#define TIXMLA_STRING_INCLUDED

#pragma warning( disable : 4514 )


/*
   TiXmlStringA is an emulation of the std::string template.
   Its purpose is to allow compiling TinyXML on compilers with no or poor STL support.
   Only the member functions relevant to the TinyXML project have been implemented.
   The buffer allocation is made by a simplistic power of 2 like mechanism : if we increase
   a string and there's no more room, we allocate a buffer twice as big as we need.
*/
class TiXmlStringA
{
  public :
    // TiXmlStringA constructor, based on a string
    TiXmlStringA (const char * instring);

    // TiXmlStringA empty constructor
    TiXmlStringA ()
    {
        allocated = 0;
        cstring = NULL;
        current_length = 0;
    }

    // TiXmlStringA copy constructor
    TiXmlStringA (const TiXmlStringA& copy);

    // TiXmlStringA destructor
    ~ TiXmlStringA ()
    {
        empty_it ();
    }

    // Convert a TiXmlStringA into a classical char *
    const char * c_str () const
    {
        if (allocated)
            return cstring;
        return "";
    }

    // Return the length of a TiXmlStringA
    unsigned length () const
	{
		return ( allocated ) ? current_length : 0;
	}

    // TiXmlStringA = operator
    void operator = (const char * content);

    // = operator
    void operator = (const TiXmlStringA & copy);

    // += operator. Maps to append
    TiXmlStringA& operator += (const char * suffix)
    {
        append (suffix);
		return *this;
    }

    // += operator. Maps to append
    TiXmlStringA& operator += (char single)
    {
        append (single);
		return *this;
    }

    // += operator. Maps to append
    TiXmlStringA& operator += (TiXmlStringA & suffix)
    {
        append (suffix);
		return *this;
    }
    bool operator == (const TiXmlStringA & compare) const;
    bool operator < (const TiXmlStringA & compare) const;
    bool operator > (const TiXmlStringA & compare) const;

    // Checks if a TiXmlStringA is empty
    bool empty () const
    {
        return length () ? false : true;
    }

    // Checks if a TiXmlStringA contains only whitespace (same rules as isspace)
	// Not actually used in tinyxml. Conflicts with a C macro, "isblank",
	// which is a problem. Commenting out. -lee
//    bool isblank () const;

    // single char extraction
    const char& at (unsigned index) const
    {
        assert( index < length ());
        return cstring [index];
    }

    // find a char in a string. Return TiXmlStringA::notfound if not found
    unsigned find (char lookup) const
    {
        return find (lookup, 0);
    }

    // find a char in a string from an offset. Return TiXmlStringA::notfound if not found
    unsigned find (char tofind, unsigned offset) const;

    /*	Function to reserve a big amount of data when we know we'll need it. Be aware that this
		function clears the content of the TiXmlStringA if any exists.
    */
    void reserve (unsigned size)
    {
        empty_it ();
        if (size)
        {
            allocated = size;
			TIXMLA_STRING cstring = new char [size];
            cstring [0] = 0;
            current_length = 0;
        }
    }

    // [] operator 
    char& operator [] (unsigned index) const
    {
        assert( index < length ());
        return cstring [index];
    }

    // Error value for find primitive 
    enum {	notfound = 0xffffffff,
            npos = notfound };

    void append (const char *str, int len );

  protected :

    // The base string
    char * cstring;
    // Number of chars allocated
    unsigned allocated;
    // Current string size
    unsigned current_length;

    // New size computation. It is simplistic right now : it returns twice the amount
    // we need
    unsigned assign_new_size (unsigned minimum_to_allocate)
    {
        return minimum_to_allocate * 2;
    }

    // Internal function that clears the content of a TiXmlStringA
    void empty_it ()
    {
        if (cstring)
            delete [] cstring;
        cstring = NULL;
        allocated = 0;
        current_length = 0;
    }

    void append (const char *suffix );

    // append function for another TiXmlStringA
    void append (const TiXmlStringA & suffix)
    {
        append (suffix . c_str ());
    }

    // append for a single char. This could be improved a lot if needed
    void append (char single)
    {
        char smallstr [2];
        smallstr [0] = single;
        smallstr [1] = 0;
        append (smallstr);
    }

} ;

/* 
   TiXmlOutStreamA is an emulation of std::ostream. It is based on TiXmlStringA.
   Only the operators that we need for TinyXML have been developped.
*/
class TiXmlOutStreamA : public TiXmlStringA
{
public :
    TiXmlOutStreamA () : TiXmlStringA () {}

    // TiXmlOutStreamA << operator. Maps to TiXmlStringA::append
    TiXmlOutStreamA & operator << (const char * in)
    {
        append (in);
        return (* this);
    }

    // TiXmlOutStreamA << operator. Maps to TiXmlStringA::append
    TiXmlOutStreamA & operator << (const TiXmlStringA & in)
    {
        append (in . c_str ());
        return (* this);
    }
} ;

#endif	// TIXMLA_STRING_INCLUDED
#endif	// TIXMLA_USE_STL
