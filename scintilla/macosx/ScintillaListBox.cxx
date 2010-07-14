
#include "ScintillaMacOSX.h"
#include "ScintillaListBox.h"

using namespace Scintilla;

const CFStringRef ScintillaListBox::kScintillaListBoxClassID = CFSTR( "org.scintilla.listbox" );
const ControlKind ScintillaListBox::kScintillaListBoxKind = { 'ejon', 'Sclb' };

ScintillaListBox::ScintillaListBox( void* windowid ) :
		TView( reinterpret_cast<HIViewRef>( windowid ) )
{
  ActivateInterface( kMouse );
  //  debugPrint = true;
}

void ScintillaListBox::Draw(
	RgnHandle			/*inLimitRgn*/,
	CGContextRef		inContext )
{
	Rect contentBounds;
	GetControlBounds(GetViewRef(), &contentBounds);

        HIRect controlFrame;
        HIViewGetFrame( GetViewRef(), &controlFrame );

	// what is the global pos?
	Surface *surfaceWindow = Surface::Allocate();
	if (surfaceWindow) {
		surfaceWindow->Init(inContext, GetViewRef());
		ctip->PaintCT(surfaceWindow);
		surfaceWindow->Release();
		delete surfaceWindow;
	}

}

ControlPartCode ScintillaListBox::HitTest( const HIPoint& where )
{
	if ( CGRectContainsPoint( Bounds(), where ) )
		return 1;
	else
		return kControlNoPart;
}

OSStatus ScintillaListBox::MouseDown(HIPoint& location, UInt32 /*inKeyModifiers*/, EventMouseButton button, UInt32 /*inClickCount*/ )
{
    if ( button != kEventMouseButtonPrimary ) return eventNotHandledErr;
    ListBox* ctip = NULL;
    ScintillaMacOSX *sciThis = NULL;
    OSStatus err = GetControlProperty( GetViewRef(), scintillaListBoxType, 0, sizeof( ctip ), NULL, &ctip );
    err = GetControlProperty( GetViewRef(), scintillaMacOSType, 0, sizeof( sciThis ), NULL, &sciThis );
    ctip->MouseClick( Scintilla::Point( static_cast<int>( location.x ), static_cast<int>( location.y ) ));
    sciThis->ListBoxClick();
    return noErr;
}

OSStatus ScintillaListBox::MouseUp(HIPoint& /*inMouseLocation*/, UInt32 /*inKeyModifiers*/, EventMouseButton button, UInt32 /*inClickCount*/ )
{
    if ( button != kEventMouseButtonPrimary ) return eventNotHandledErr;
    return noErr;
}

HIViewRef ScintillaListBox::Create()
{
	// Register the HIView, if needed
	static bool registered = false;

	if ( not registered )
		{
		TView::RegisterSubclass( kScintillaListBoxClassID, Construct );
		registered = true;
		}

	OSStatus err = noErr;
	EventRef event = CreateInitializationEvent();
	assert( event != NULL );

	HIViewRef control = NULL;
	err = HIObjectCreate( kScintillaListBoxClassID, event, reinterpret_cast<HIObjectRef*>( &control ) );
	ReleaseEvent( event );
	if ( err == noErr ) {
	  Platform::DebugPrintf("ScintillaListBox::Create control %08X\n",control);
	  return control;
	}
	return NULL;	
}

OSStatus ScintillaListBox::Construct( HIViewRef inControl, TView** outView )
{
	*outView = new ScintillaListBox( inControl );
	Platform::DebugPrintf("ScintillaListBox::Construct scintilla %08X\n",*outView);
	if ( *outView != NULL )
		return noErr;
	else
		return memFullErr;
}

extern "C" {
HIViewRef scintilla_listbox_new() {
	return ScintillaListBox::Create();
}
}
