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


#ifndef TIXML_USE_STL


#include "tinystr.h"

// TiXmlString constructor, based on a C string
TiXmlString::TiXmlString (const wchar_t* instring)
{
    unsigned newlen;
    wchar_t * newstring;

    if (!instring)
    {
        allocated = 0;
        cstring = NULL;
        current_length = 0;
        return;
    }
    newlen = lstrlen (instring) + 1;
    newstring = new wchar_t [newlen];
    memcpy (newstring, instring, newlen);
    allocated = newlen;
    cstring = newstring;
    current_length = newlen - 1;
}

// TiXmlString copy constructor
TiXmlString::TiXmlString (const TiXmlString& copy)
{
    unsigned newlen;
    wchar_t * newstring;

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
    newstring = new wchar_t [newlen];
    memcpy (newstring, copy . cstring, newlen);
    allocated = newlen;
    cstring = newstring;
    current_length = newlen - 1;
}

// TiXmlString = operator. Safe when assign own content
void TiXmlString ::operator = (const wchar_t * content)
{
    unsigned newlen;
    wchar_t * newstring;

    if (! content)
    {
        empty_it ();
        return;
    }
    newlen = lstrlen (content) + 1;
    newstring = new wchar_t [newlen];
    memcpy (newstring, content, newlen);
    empty_it ();
    allocated = newlen;
    cstring = newstring;
    current_length = newlen - 1;
}

// = operator. Safe when assign own content
void TiXmlString ::operator = (const TiXmlString & copy)
{
    unsigned newlen;
    wchar_t * newstring;

    if (! copy . length ())
    {
        empty_it ();
        return;
    }
    newlen = copy . length () + 1;
    newstring = new wchar_t [newlen];
    memcpy (newstring, copy . c_str (), newlen);
    empty_it ();
    allocated = newlen;
    cstring = newstring;
    current_length = newlen - 1;
}


//// Checks if a TiXmlString contains only whitespace (same rules as isspace)
//bool TiXmlString::isblank () const
//{
//    wchar_t * lookup;
//    for (lookup = cstring; * lookup; lookup++)
//        if (! isspace (* lookup))
//            return false;
//    return true;
//}

// append a const wchar_t * to an existing TiXmlString
void TiXmlString::append( const wchar_t* str, int len )
{
    wchar_t * new_string;
    unsigned new_alloc, new_size, size_suffix;

    size_suffix = lstrlen (str);
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
        new_string = new wchar_t [new_alloc];        
        new_string [0] = 0;

        // copy the previous allocated buffer into this one
        if (allocated && cstring)
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


// append a const wchar_t * to an existing TiXmlString
void TiXmlString::append( const wchar_t * suffix )
{
    wchar_t * new_string;
    unsigned new_alloc, new_size;

    new_size = length () + lstrlen (suffix) + 1;
    // check if we need to expand
    if (new_size > allocated)
    {
        // compute new size
        new_alloc = assign_new_size (new_size);

        // allocate new buffer
        new_string = new wchar_t [new_alloc];        
        new_string [0] = 0;

        // copy the previous allocated buffer into this one
        if (allocated && cstring)
            memcpy (new_string, cstring, 1 + length ());

        // append the suffix. It does exist, otherwise we wouldn't be expanding 
        memcpy (new_string + length (), 
                suffix,
                lstrlen (suffix) + 1);

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
                lstrlen (suffix) + 1);
    }
    current_length = new_size - 1;
}

// Check for TiXmlString equuivalence
//bool TiXmlString::operator == (const TiXmlString & compare) const
//{
//    return (! lstrcmp (c_str (), compare . c_str ()));
//}

//unsigned TiXmlString::length () const
//{
//    if (allocated)
//        // return lstrlen (cstring);
//        return current_length;
//    return 0;
//}


unsigned TiXmlString::find (wchar_t tofind, unsigned offset) const
{
    wchar_t * lookup;

    if (offset >= length ())
        return (unsigned) notfound;
    for (lookup = cstring + offset; * lookup; lookup++)
        if (* lookup == tofind)
            return lookup - cstring;
    return (unsigned) notfound;
}


bool TiXmlString::operator == (const TiXmlString & compare) const
{
	if ( allocated && compare.allocated )
	{
		assert( cstring );
		assert( compare.cstring );
		return ( lstrcmp( cstring, compare.cstring ) == 0 );
 	}
	return false;
}


bool TiXmlString::operator < (const TiXmlString & compare) const
{
	if ( allocated && compare.allocated )
	{
		assert( cstring );
		assert( compare.cstring );
		return ( lstrcmp( cstring, compare.cstring ) > 0 );
 	}
	return false;
}


bool TiXmlString::operator > (const TiXmlString & compare) const
{
	if ( allocated && compare.allocated )
	{
		assert( cstring );
		assert( compare.cstring );
		return ( lstrcmp( cstring, compare.cstring ) < 0 );
 	}
	return false;
}


#endif	// TIXML_USE_STL
