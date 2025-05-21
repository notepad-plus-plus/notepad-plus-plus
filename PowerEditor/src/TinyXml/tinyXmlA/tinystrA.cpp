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

#include "tinyxmlA.h"

#ifndef TIXMLA_USE_STL




#include "tinystrA.h"

// TiXmlStringA constructor, based on a C string
TiXmlStringA::TiXmlStringA (const char* instring)
{
    unsigned newlen;
    char * newstring;

    if (!instring)
    {
        allocated = 0;
        cstring = NULL;
        current_length = 0;
        return;
    }
    newlen = strlen (instring) + 1;
    newstring = new char [newlen];
    memcpy (newstring, instring, newlen);
    // strcpy (newstring, instring);
    allocated = newlen;
    cstring = newstring;
    current_length = newlen - 1;
}

// TiXmlStringA copy constructor
TiXmlStringA::TiXmlStringA (const TiXmlStringA& copy)
{
    unsigned newlen;
    char * newstring;

	// Prevent copy to self!
	if ( &copy == this )
		return;

    if (! copy . allocated)
    {
        allocated = 0;
        cstring = NULL;
        current_length = 0;
        return;
    }
    newlen = copy . length () + 1;
    newstring = new char [newlen];
    // strcpy (newstring, copy . cstring);
    memcpy (newstring, copy . cstring, newlen);
    allocated = newlen;
    cstring = newstring;
    current_length = newlen - 1;
}

// TiXmlStringA = operator. Safe when assign own content
void TiXmlStringA ::operator = (const char * content)
{
    unsigned newlen;
    char * newstring;

    if (! content)
    {
        empty_it ();
        return;
    }
    newlen = strlen (content) + 1;
    newstring = new char [newlen];
    // strcpy (newstring, content);
    memcpy (newstring, content, newlen);
    empty_it ();
    allocated = newlen;
    cstring = newstring;
    current_length = newlen - 1;
}

// = operator. Safe when assign own content
void TiXmlStringA ::operator = (const TiXmlStringA & copy)
{
    unsigned newlen;
    char * newstring;

    if (! copy . length ())
    {
        empty_it ();
        return;
    }
    newlen = copy . length () + 1;
    newstring = new char [newlen];
    // strcpy (newstring, copy . c_str ());
    memcpy (newstring, copy . c_str (), newlen);
    empty_it ();
    allocated = newlen;
    cstring = newstring;
    current_length = newlen - 1;
}


//// Checks if a TiXmlStringA contains only whitespace (same rules as isspace)
//bool TiXmlStringA::isblank () const
//{
//    char * lookup;
//    for (lookup = cstring; * lookup; lookup++)
//        if (! isspace (* lookup))
//            return false;
//    return true;
//}

// append a const char * to an existing TiXmlStringA
void TiXmlStringA::append( const char* str, int len )
{
    char * new_string;
    unsigned new_alloc, new_size, size_suffix;

    size_suffix = strlen (str);
    if (len < (int) size_suffix)
        size_suffix = len;
    if (! size_suffix)
        return;

    new_size = length () + size_suffix + 1;
    // check if we need to expand
    if (new_size > allocated)
    {
        // compute new size
        new_alloc = assign_new_size (new_size);

        // allocate new buffer
        new_string = new char [new_alloc];        
        new_string [0] = 0;

        // copy the previous allocated buffer into this one
        if (allocated && cstring)
            // strcpy (new_string, cstring);
            memcpy (new_string, cstring, length ());

        // append the suffix. It does exist, otherwise we wouldn't be expanding 
        // strncat (new_string, str, len);
        memcpy (new_string + length (), 
                str,
                size_suffix);

        // return previsously allocated buffer if any
        if (allocated && cstring)
            delete [] cstring;

        // update member variables
        cstring = new_string;
        allocated = new_alloc;
    }
    else
    {
        // we know we can safely append the new string
        // strncat (cstring, str, len);
        memcpy (cstring + length (), 
                str,
                size_suffix);
    }
    current_length = new_size - 1;
    cstring [current_length] = 0;
}


// append a const char * to an existing TiXmlStringA
void TiXmlStringA::append( const char * suffix )
{
    char * new_string;
    unsigned new_alloc, new_size;

    new_size = length () + strlen (suffix) + 1;
    // check if we need to expand
    if (new_size > allocated)
    {
        // compute new size
        new_alloc = assign_new_size (new_size);

        // allocate new buffer
        new_string = new char [new_alloc];        
        new_string [0] = 0;

        // copy the previous allocated buffer into this one
        if (allocated && cstring)
            memcpy (new_string, cstring, 1 + length ());
            // strcpy (new_string, cstring);

        // append the suffix. It does exist, otherwise we wouldn't be expanding 
        memcpy (new_string + length (), 
                suffix,
                strlen (suffix) + 1);

        // return previsously allocated buffer if any
        if (allocated && cstring)
            delete [] cstring;

        // update member variables
        cstring = new_string;
        allocated = new_alloc;
    }
    else
    {
        // we know we can safely append the new string
        memcpy (cstring + length (), 
                suffix, 
                strlen (suffix) + 1);
    }
    current_length = new_size - 1;
}

// Check for TiXmlStringA equuivalence
//bool TiXmlStringA::operator == (const TiXmlStringA & compare) const
//{
//    return (! strcmp (c_str (), compare . c_str ()));
//}

//unsigned TiXmlStringA::length () const
//{
//    if (allocated)
//        // return strlen (cstring);
//        return current_length;
//    return 0;
//}


unsigned TiXmlStringA::find (char tofind, unsigned offset) const
{
    char * lookup;

    if (offset >= length ())
        return (unsigned) notfound;
    for (lookup = cstring + offset; * lookup; lookup++)
        if (* lookup == tofind)
            return lookup - cstring;
    return (unsigned) notfound;
}


bool TiXmlStringA::operator == (const TiXmlStringA & compare) const
{
	if ( allocated && compare.allocated )
	{
		assert( cstring );
		assert( compare.cstring );
		return ( strcmp( cstring, compare.cstring ) == 0 );
 	}
	return false;
}


bool TiXmlStringA::operator < (const TiXmlStringA & compare) const
{
	if ( allocated && compare.allocated )
	{
		assert( cstring );
		assert( compare.cstring );
		return ( strcmp( cstring, compare.cstring ) > 0 );
 	}
	return false;
}


bool TiXmlStringA::operator > (const TiXmlStringA & compare) const
{
	if ( allocated && compare.allocated )
	{
		assert( cstring );
		assert( compare.cstring );
		return ( strcmp( cstring, compare.cstring ) < 0 );
 	}
	return false;
}


#endif	// TIXMLA_USE_STL
