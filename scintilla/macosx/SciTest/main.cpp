//
//  main.c
//  SciTest
//
//  Copyright (c) 2005-2006 ActiveState Software Inc.
//  All rights reserved.
//
//  Created by Shane Caraveo on 3/20/05.
//

#include <Carbon/Carbon.h>
#include "TView.h"
#include "TCarbonEvent.h"
#include "ScintillaMacOSX.h"

extern "C" HIViewRef scintilla_new(void);

const HILayoutInfo kBindToParentLayout = {
  kHILayoutInfoVersionZero,
  { { NULL, kHILayoutBindTop }, { NULL, kHILayoutBindLeft }, { NULL, kHILayoutBindBottom }, { NULL, kHILayoutBindRight } },
  { { NULL, kHILayoutScaleAbsolute, 0 }, { NULL, kHILayoutScaleAbsolute, 0 } },
  { { NULL, kHILayoutPositionTop, 0 }, { NULL, kHILayoutPositionLeft, 0 } }
};

using namespace Scintilla;

/* XPM */
static const char *ac_class[] = {
/* columns rows colors chars-per-pixel */
"18 12 24 1",
"  c black",
". c #403030",
"X c #473636",
"o c #4E3C3C",
"O c #474141",
"+ c #5F4C4C",
"@ c #756362",
"# c #98342C",
"$ c #A0392F",
"% c #B24235",
"& c #B2443C",
"* c #B34E3E",
"= c #B54E44",
"- c #B65146",
"; c #B7584F",
": c #B8554C",
"> c #B75A50",
", c #B95852",
"< c #B96259",
"1 c #B89B9B",
"2 c #BCA0A0",
"3 c #C1A5A5",
"4 c gray100",
"5 c None",
/* pixels */
"555555555555555555",
"55553$$$$$$$#@5555",
"55552;%&&==;=o5555",
"55551>&&*=;:=.5555",
"55551>&*=;::=.5555",
"55551>*==:::-X5555",
"55551>==:::,;.5555",
"55551<==:;,<>.5555",
"55551<;;;;<<;.5555",
"55551;-==;;;;X5555",
"55555+XX..X..O5555",
"555555555555555555"
};

const char keywords[]="and and_eq asm auto bitand bitor bool break "
"case catch char class compl const const_cast continue "
"default delete do double dynamic_cast else enum explicit export extern false float for "
"friend goto if inline int long mutable namespace new not not_eq "
"operator or or_eq private protected public "
"register reinterpret_cast return short signed sizeof static static_cast struct switch "
"template this throw true try typedef typeid typename union unsigned using "
"virtual void volatile wchar_t while xor xor_eq";

pascal OSStatus WindowEventHandler(EventHandlerCallRef	inCallRef,
				   EventRef		inEvent,
				   void*	        inUserData )
{
	HIViewRef sciView = *reinterpret_cast<HIViewRef*>( inUserData );
	WindowRef window = GetControlOwner(sciView);
	ScintillaMacOSX* scintilla;
	GetControlProperty( sciView, scintillaMacOSType, 0, sizeof( scintilla ), NULL, &scintilla );
	TCarbonEvent event( inEvent );

	// If the window is not active, let the standard window handler execute.
	if ( ! IsWindowActive( window ) ) return eventNotHandledErr;

	const HIViewRef rootView = HIViewGetRoot( window );
	assert( rootView != NULL );

	if ( event.GetKind() == kEventMouseDown )
		{
			UInt32 inKeyModifiers;
			event.GetParameter( kEventParamKeyModifiers, &inKeyModifiers );

			EventMouseButton inMouseButton;
			event.GetParameter<EventMouseButton>( kEventParamMouseButton, typeMouseButton, &inMouseButton );
			if (inMouseButton == kEventMouseButtonTertiary) {
				if (inKeyModifiers & optionKey) {
					const char *test = "\001This is a test calltip This is a test calltip This is a test calltip";
					scintilla->WndProc( SCI_CALLTIPSHOW, 0, (long int)test );
				} else {
					const char *list = "test_1?0 test_2 test_3 test_4 test_5 test_6 test_7 test_8 test_9 test_10 test_11 test_12";
					scintilla->WndProc( SCI_AUTOCSHOW, 0, (long int)list );
				}
				return noErr;
			}
		}

	return eventNotHandledErr;
}

int main(int argc, char* argv[])
{
    IBNibRef 		nibRef;
    WindowRef 		window;

    OSStatus		err;

    // Create a Nib reference passing the name of the nib file (without the .nib extension)
    // CreateNibReference only searches into the application bundle.
    err = CreateNibReference(CFSTR("main"), &nibRef);
    require_noerr( err, CantGetNibRef );

    // Once the nib reference is created, set the menu bar. "MainMenu" is the name of the menu bar
    // object. This name is set in InterfaceBuilder when the nib is created.
    err = SetMenuBarFromNib(nibRef, CFSTR("MenuBar"));
    require_noerr( err, CantSetMenuBar );

    // Then create a window. "MainWindow" is the name of the window object. This name is set in
    // InterfaceBuilder when the nib is created.
    err = CreateWindowFromNib(nibRef, CFSTR("MainWindow"), &window);
    require_noerr( err, CantCreateWindow );

    // We don't need the nib reference anymore.
    DisposeNibReference(nibRef);

    HIRect boundsRect;
    // GOOD and BAD methods off embedding into a window.  This is used
    // to test Window::SetPositionRelative under different situations.
#define GOOD
#ifdef GOOD
#ifdef USE_CONTROL
    ControlRef root;
    GetRootControl(window, &root);
#else
    HIViewRef root;
    HIViewFindByID(HIViewGetRoot(window),
    		   kHIViewWindowContentID,
    		   &root);
#endif
    HIViewGetBounds(root, &boundsRect);

#else // BAD like mozilla
    HIViewRef root;
    root = HIViewGetRoot(window);

    Rect cBounds, sBounds;
    GetWindowBounds(window, kWindowContentRgn, &cBounds);
    GetWindowBounds(window, kWindowStructureRgn, &sBounds);
    boundsRect.origin.x = cBounds.left - sBounds.left;
    boundsRect.origin.y = cBounds.top - sBounds.top;
    boundsRect.size.width = cBounds.right - cBounds.left;
    boundsRect.size.height = cBounds.bottom - cBounds.top;
#endif

    // get a scintilla control, and add it to it's parent container
    HIViewRef sciView;
    sciView = scintilla_new();
    HIViewAddSubview(root, sciView);

	// some scintilla init
	ScintillaMacOSX* scintilla;
	GetControlProperty( sciView, scintillaMacOSType, 0, sizeof( scintilla ), NULL, &scintilla );

	scintilla->WndProc( SCI_SETLEXER, SCLEX_CPP, 0);
	scintilla->WndProc( SCI_SETSTYLEBITS, 5, 0);

	scintilla->WndProc(SCI_STYLESETFORE, 0, 0x808080);	// White space
	scintilla->WndProc(SCI_STYLESETFORE, 1, 0x007F00);	// Comment
	scintilla->WndProc(SCI_STYLESETITALIC, 1, 1);	// Comment
	scintilla->WndProc(SCI_STYLESETFORE, 2, 0x007F00);	// Line comment
	scintilla->WndProc(SCI_STYLESETITALIC, 2, 1);	// Line comment
	scintilla->WndProc(SCI_STYLESETFORE, 3, 0x3F703F);	// Doc comment
	scintilla->WndProc(SCI_STYLESETITALIC, 3, 1);	// Doc comment
	scintilla->WndProc(SCI_STYLESETFORE, 4, 0x7F7F00);	// Number
	scintilla->WndProc(SCI_STYLESETFORE, 5, 0x7F0000);	// Keyword
	scintilla->WndProc(SCI_STYLESETBOLD, 5, 1);	// Keyword
	scintilla->WndProc(SCI_STYLESETFORE, 6, 0x7F007F);	// String
	scintilla->WndProc(SCI_STYLESETFORE, 7, 0x7F007F);	// Character
	scintilla->WndProc(SCI_STYLESETFORE, 8, 0x804080);	// UUID
	scintilla->WndProc(SCI_STYLESETFORE, 9, 0x007F7F);	// Preprocessor
	scintilla->WndProc(SCI_STYLESETFORE,10, 0x000000);	// Operators
	scintilla->WndProc(SCI_STYLESETBOLD,10, 1);	// Operators
	scintilla->WndProc(SCI_STYLESETFORE,11, 0x000000);	// Identifiers


	scintilla->WndProc(SCI_SETKEYWORDS, 0, (sptr_t)(char *)keywords);	// Keyword

	/*
	these fail compilation on osx now
	scintilla->WndProc( SCI_SETPROPERTY, "fold", (long int)"1");
	scintilla->WndProc( SCI_SETPROPERTY, "fold.compact", (long int)"0");
	scintilla->WndProc( SCI_SETPROPERTY, "fold.comment", (long int)"1");
	scintilla->WndProc( SCI_SETPROPERTY, "fold.preprocessor", (long int)"1");
	*/

	scintilla->WndProc( SCI_REGISTERIMAGE, 0, (long int)ac_class);

	scintilla->WndProc( SCI_SETMARGINTYPEN, 0, (long int)SC_MARGIN_NUMBER);
	scintilla->WndProc( SCI_SETMARGINWIDTHN, 0, (long int)30);
	scintilla->WndProc( SCI_SETMARGINTYPEN, 1, (long int)SC_MARGIN_SYMBOL);
	scintilla->WndProc( SCI_SETMARGINMASKN, 1, (long int)SC_MASK_FOLDERS);
	scintilla->WndProc( SCI_SETMARGINWIDTHN, 1, (long int)20);
	scintilla->WndProc( SCI_SETMARGINTYPEN, 2, (long int)SC_MARGIN_SYMBOL);
	scintilla->WndProc( SCI_SETMARGINWIDTHN, 2, (long int)16);
	//scintilla->WndProc( SCI_SETWRAPMODE, SC_WRAP_WORD, 0);
	//scintilla->WndProc( SCI_SETWRAPVISUALFLAGS, SC_WRAPVISUALFLAG_END | SC_WRAPVISUALFLAG_START, 0);

    // set the size of scintilla to the size of the container
    HIViewSetFrame( sciView, &boundsRect );

    // bind the size of scintilla to the size of it's container window
    HIViewSetLayoutInfo(sciView, &kBindToParentLayout);

    // setup some event handling
    static const EventTypeSpec kWindowMouseEvents[] =
      {
	{ kEventClassMouse, kEventMouseDown },
      };

    InstallEventHandler( GetWindowEventTarget( window ), WindowEventHandler,
			 GetEventTypeCount( kWindowMouseEvents ), kWindowMouseEvents, &sciView, NULL );

    // show scintilla
    ShowControl(sciView);

    SetAutomaticControlDragTrackingEnabledForWindow(window, true);

    // The window was created hidden so show it.
    ShowWindow( window );

    // Call the event loop
    RunApplicationEventLoop();

CantCreateWindow:
CantSetMenuBar:
CantGetNibRef:
	return err;
}

