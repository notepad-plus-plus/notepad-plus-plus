/*
 *  ScintillaMacOSX.h
 *  tutorial
 *
 *  Created by Evan Jones on Sun Sep 01 2002.
 *  Copyright (c) 2002 __MyCompanyName__. All rights reserved.
 *
 */
#ifndef SCINTILLA_LISTBOX_H
#define SCINTILLA_LISTBOX_H

#include "TView.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#include "Platform.h"
#include "Scintilla.h"

static const OSType scintillaListBoxType = 'sclb';

namespace Scintilla {

class ScintillaListBox : public TView
{
public:
        // Private so ScintillaListBox objects can not be copied
        ScintillaListBox(const ScintillaListBox &) : TView( NULL ) {}
        ScintillaListBox &operator=(const ScintillaListBox &) { return * this; }
	~ScintillaListBox() {};

public:
	/** This is the class ID that we've assigned to Scintilla. */
	static const CFStringRef kScintillaListBoxClassID;
	static const ControlKind kScintillaListBoxKind;

	ScintillaListBox( void* windowid );

	/** Returns the HIView object kind, needed to subclass TView. */
	virtual ControlKind GetKind() { return kScintillaListBoxKind; }

private:

	virtual ControlPartCode HitTest( const HIPoint& where );
	virtual void Draw( RgnHandle rgn, CGContextRef gc );
	virtual OSStatus MouseDown( HIPoint& location, UInt32 modifiers, EventMouseButton button, UInt32 clickCount );
	virtual OSStatus MouseUp( HIPoint& location, UInt32 modifiers, EventMouseButton button, UInt32 clickCount );

public:
	static HIViewRef Create();
private:
	static OSStatus Construct( HIViewRef inControl, TView** outView );
	
};


}


#endif
