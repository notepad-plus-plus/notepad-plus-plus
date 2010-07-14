/*
 *  ScintillaMacOSX.h
 *  tutorial
 *
 *  Created by Evan Jones on Sun Sep 01 2002.
 *
 */
#ifndef SCINTILLA_CALLTIP_H
#define SCINTILLA_CALLTIP_H

#include "TView.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#include "Platform.h"
#include "Scintilla.h"

static const OSType scintillaCallTipType = 'Scct';

namespace Scintilla {

class ScintillaCallTip : public TView
{
public:
    // Private so ScintillaCallTip objects can not be copied
    ScintillaCallTip(const ScintillaCallTip &) : TView( NULL ) {}
    ScintillaCallTip &operator=(const ScintillaCallTip &) { return * this; }
    ~ScintillaCallTip() {};

public:
    /** This is the class ID that we've assigned to Scintilla. */
    static const CFStringRef kScintillaCallTipClassID;
    static const ControlKind kScintillaCallTipKind;

    ScintillaCallTip( void* windowid );

    /** Returns the HIView object kind, needed to subclass TView. */
    virtual ControlKind GetKind() { return kScintillaCallTipKind; }

private:

    virtual ControlPartCode HitTest( const HIPoint& where );
    virtual void Draw( RgnHandle rgn, CGContextRef gc );
    virtual OSStatus MouseDown( HIPoint& location, UInt32 modifiers, EventMouseButton button, UInt32 clickCount );
    virtual OSStatus MouseUp( HIPoint& location, UInt32 modifiers, EventMouseButton button, UInt32 clickCount );
    virtual OSStatus MouseDragged( HIPoint& location, UInt32 modifiers, EventMouseButton button, UInt32 clickCount );

public:
    static HIViewRef Create();
private:
    static OSStatus Construct( HIViewRef inControl, TView** outView );
    
};


}


#endif
