/*******************************************************************************

Copyright (c) 2007 Adobe Systems Incorporated

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

********************************************************************************/

#ifndef _ExtInput_h
#define _ExtInput_h

#include <Carbon/Carbon.h>
#include "Scintilla.h"

namespace Scintilla
{

/**
The ExtInput class provides TSM input services to Scintilla.
It uses the indicators 0 and 1 (see SCI_INDICSETSTYLE) to apply
underlines to partially converted text.
*/

class ExtInput
{
public:
	/**
	Attach extended input to a HIView with attached Scintilla. This installs the needed
	event handlers etc.
	*/
	static	void		attach (HIViewRef ref);
	/**
	Detach extended input from a HIViewwith attached Scintilla. 
	*/
	static	void		detach (HIViewRef ref);
	/**
	Activate or deactivate extended input. This method should be called whenever
	the view gains or loses focus.
	*/
	static	void		activate (HIViewRef ref, bool on);
	/**
	Terminate extended input.
	*/
	static	void		stop (HIViewRef ref);
};

}	// end namespace

#endif
