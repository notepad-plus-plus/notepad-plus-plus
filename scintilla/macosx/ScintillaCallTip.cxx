
#include "ScintillaMacOSX.h"
#include "ScintillaCallTip.h"
#include "CallTip.h"

using namespace Scintilla;

const CFStringRef ScintillaCallTip::kScintillaCallTipClassID = CFSTR( "org.scintilla.calltip" );
const ControlKind ScintillaCallTip::kScintillaCallTipKind = { 'ejon', 'Scct' };

ScintillaCallTip::ScintillaCallTip( void* windowid ) :
        TView( reinterpret_cast<HIViewRef>( windowid ) )
{
    ActivateInterface( kMouse );
    //  debugPrint = true;
}

void ScintillaCallTip::Draw(
    RgnHandle           /*inLimitRgn*/,
    CGContextRef        inContext )
{
    // Get a reference to the Scintilla C++ object
    CallTip* ctip = NULL;
    OSStatus err;
    err = GetControlProperty( GetViewRef(), scintillaCallTipType, 0, sizeof( ctip ), NULL, &ctip );
    assert(err == noErr);
    if (ctip == NULL) return;
    
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

ControlPartCode ScintillaCallTip::HitTest( const HIPoint& where )
{
    if ( CGRectContainsPoint( Bounds(), where ) )
        return 1;
    else
        return kControlNoPart;
}

OSStatus ScintillaCallTip::MouseDown(HIPoint& location, UInt32 /*inKeyModifiers*/, EventMouseButton button, UInt32 /*inClickCount*/ )
{
    if ( button != kEventMouseButtonPrimary ) return eventNotHandledErr;
    CallTip* ctip = NULL;
    ScintillaMacOSX *sciThis = NULL;
    OSStatus err = GetControlProperty( GetViewRef(), scintillaCallTipType, 0, sizeof( ctip ), NULL, &ctip );
    err = GetControlProperty( GetViewRef(), scintillaMacOSType, 0, sizeof( sciThis ), NULL, &sciThis );
    ctip->MouseClick( Scintilla::Point( static_cast<int>( location.x ), static_cast<int>( location.y ) ));
    sciThis->CallTipClick();
    return noErr;
}

OSStatus ScintillaCallTip::MouseUp(HIPoint& /*inMouseLocation*/, UInt32 /*inKeyModifiers*/, EventMouseButton button, UInt32 /*inClickCount*/ )
{
    if ( button != kEventMouseButtonPrimary ) return eventNotHandledErr;
    return noErr;
}

OSStatus ScintillaCallTip::MouseDragged( HIPoint& location, UInt32 /*modifiers*/, EventMouseButton /*button*/, UInt32 /*clickCount*/ )
{
    SetThemeCursor( kThemeArrowCursor );
    return noErr;
}

HIViewRef ScintillaCallTip::Create()
{
    // Register the HIView, if needed
    static bool registered = false;

    if ( not registered )
    {
        TView::RegisterSubclass( kScintillaCallTipClassID, Construct );
        registered = true;
    }

    OSStatus err = noErr;
    EventRef event = CreateInitializationEvent();
    assert( event != NULL );

    HIViewRef control = NULL;
    err = HIObjectCreate( kScintillaCallTipClassID, event, reinterpret_cast<HIObjectRef*>( &control ) );
    ReleaseEvent( event );
    if ( err == noErr ) {
        Platform::DebugPrintf("ScintillaCallTip::Create control %08X\n",control);
        return control;
    }
    return NULL;    
}

OSStatus ScintillaCallTip::Construct( HIViewRef inControl, TView** outView )
{
    *outView = new ScintillaCallTip( inControl );
    Platform::DebugPrintf("ScintillaCallTip::Construct scintilla %08X\n",*outView);
    if ( *outView != NULL )
        return noErr;
    else
        return memFullErr;
}

extern "C" {
HIViewRef scintilla_calltip_new() {
    return ScintillaCallTip::Create();
}
}
