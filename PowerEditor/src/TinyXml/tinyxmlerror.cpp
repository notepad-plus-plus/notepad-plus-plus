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


#include "tinyxml.h"

// The goal of the separate error file is to make the first
// step towards localization. tinyxml (currently) only supports
// latin-1, but at least the error messages could now be translated.
//
// It also cleans up the code a bit.
//

const wchar_t* TiXmlBase::errorString[ TIXML_ERROR_STRING_COUNT ] =
{
	L"No error",
	L"Error",
	L"Failed to open file",
	L"Memory allocation failed.",
	L"Error parsing Element.",
	L"Failed to read Element name",
	L"Error reading Element value.",
	L"Error reading Attributes.",
	L"Error: empty tag.",
	L"Error reading end tag.",
	L"Error parsing Unknown.",
	L"Error parsing Comment.",
	L"Error parsing Declaration.",
	L"Error document empty."
};
