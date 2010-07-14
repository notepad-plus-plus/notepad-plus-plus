/*
    File:		TView.cp
    
    Version:	1.0

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
				("Apple") in consideration of your agreement to the following terms, and your
				use, installation, modification or redistribution of this Apple software
				constitutes acceptance of these terms.  If you do not agree with these terms,
				please do not use, install, modify or redistribute this Apple software.

				In consideration of your agreement to abide by the following terms, and subject
				to these terms, Apple grants you a personal, non-exclusive license, under Apple’s
				copyrights in this original Apple software (the "Apple Software"), to use,
				reproduce, modify and redistribute the Apple Software, with or without
				modifications, in source and/or binary forms; provided that if you redistribute
				the Apple Software in its entirety and without modifications, you must retain
				this notice and the following text and disclaimers in all such redistributions of
				the Apple Software.  Neither the name, trademarks, service marks or logos of
				Apple Computer, Inc. may be used to endorse or promote products derived from the
				Apple Software without specific prior written permission from Apple.  Except as
				expressly stated in this notice, no other rights or licenses, express or implied,
				are granted by Apple herein, including but not limited to any patent rights that
				may be infringed by your derivative works or by other works in which the Apple
				Software may be incorporated.

				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
				COMBINATION WITH YOUR PRODUCTS.

				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	Copyright © 2002 Apple Computer, Inc., All Rights Reserved
*/

/*
 NOTE: This is NOWHERE near a completely exhaustive implementation of a view. There are
       many more carbon events one could intercept and hook into this.
*/

#include "TView.h"

//-----------------------------------------------------------------------------------
//	constants
//-----------------------------------------------------------------------------------
//
const EventTypeSpec kHIObjectEvents[] =
{	{ kEventClassHIObject, kEventHIObjectConstruct },
	{ kEventClassHIObject, kEventHIObjectInitialize },
	{ kEventClassHIObject, kEventHIObjectDestruct }
};
	
const EventTypeSpec kHIViewEvents[] =
{	{ kEventClassCommand, kEventCommandProcess },
	{ kEventClassCommand, kEventCommandUpdateStatus },
	
	{ kEventClassControl, kEventControlInitialize },
	{ kEventClassControl, kEventControlDraw },
	{ kEventClassControl, kEventControlHitTest },
	{ kEventClassControl, kEventControlGetPartRegion },
	{ kEventClassControl, kEventControlGetData },
	{ kEventClassControl, kEventControlSetData },
	{ kEventClassControl, kEventControlGetOptimalBounds },
	{ kEventClassControl, kEventControlBoundsChanged },
	{ kEventClassControl, kEventControlTrack },
	{ kEventClassControl, kEventControlGetSizeConstraints },
	{ kEventClassControl, kEventControlHit },
	
	{ kEventClassControl, kEventControlHiliteChanged },
	{ kEventClassControl, kEventControlActivate },
	{ kEventClassControl, kEventControlDeactivate },
	{ kEventClassControl, kEventControlValueFieldChanged },
	{ kEventClassControl, kEventControlTitleChanged },
	{ kEventClassControl, kEventControlEnabledStateChanged },
};

// This param name was accidentally left unexported for
// the release of Jaguar.
const EventParamName kEventParamControlLikesDrag = 'cldg';

//-----------------------------------------------------------------------------------
//	TView constructor
//-----------------------------------------------------------------------------------
//
TView::TView(
	HIViewRef			inControl )
	:	fViewRef( inControl )
{
	verify_noerr( InstallEventHandler( GetControlEventTarget( fViewRef ), ViewEventHandler,
			GetEventTypeCount( kHIViewEvents ), kHIViewEvents, this, &fHandler ) );

	mouseEventHandler = NULL;
	fAutoInvalidateFlags = 0;
	debugPrint = false;
}

//-----------------------------------------------------------------------------------
//	TView destructor
//-----------------------------------------------------------------------------------
//
TView::~TView()
{
	// If we have installed our custom mouse events handler on the window,
	// go forth and remove it. Note: -1 is used to indicate that no handler has
	// been installed yet, but we want to once we get a window.
	if ( mouseEventHandler != NULL && mouseEventHandler != reinterpret_cast<void*>( -1 ) ) 
		RemoveEventHandler( mouseEventHandler );
	mouseEventHandler = NULL;
}

//-----------------------------------------------------------------------------------
//	Initialize
//-----------------------------------------------------------------------------------
//	Called during HIObject construction, this is your subclasses' chance to extract
//	any parameters it might have added to the initialization event passed into the
//	HIObjectCreate call.
//
OSStatus TView::Initialize( TCarbonEvent& /*inEvent*/ )
{
	return noErr;
}

//-----------------------------------------------------------------------------------
//	GetBehaviors
//-----------------------------------------------------------------------------------
//	Returns our behaviors. Any subclass that overrides this should OR in its behaviors
//	into the inherited behaviors.
//
UInt32 TView::GetBehaviors()
{
	return kControlSupportsDataAccess | kControlSupportsGetRegion;
}

//-----------------------------------------------------------------------------------
//	Draw
//-----------------------------------------------------------------------------------
//	Draw your view. You should draw based on VIEW coordinates, not frame coordinates.
//
void TView::Draw(
	RgnHandle			/*inLimitRgn*/,
	CGContextRef		/*inContext*/ )
{
}

//-----------------------------------------------------------------------------------
//	HitTest
//-----------------------------------------------------------------------------------
//	Asks your view to return what part of itself (if any) is hit by the point given
//	to it. The point is in VIEW coordinates, so you should get the view rect to do
//	bounds checking.
//
ControlPartCode TView::HitTest(
	const HIPoint&		/*inWhere*/ )
{
	return kControlNoPart;
}

//-----------------------------------------------------------------------------------
//	GetRegion
//-----------------------------------------------------------------------------------
//	This is called when someone wants to know certain metrics regarding this view.
//	The base class does nothing. Subclasses should handle their own parts, such as
//	the content region by overriding this method. The structure region is, by default,
//	the view's bounds. If a subclass does not have a region for a given part, it 
//	should always call the inherited method.
//
OSStatus TView::GetRegion(
	ControlPartCode		inPart,
	RgnHandle			outRgn )
{
#pragma unused( inPart, outRgn )

	return eventNotHandledErr;
}

//-----------------------------------------------------------------------------------
//	PrintDebugInfo
//-----------------------------------------------------------------------------------
//	This is called when asked to print debugging information.
//
void TView::PrintDebugInfo()
{
}

//-----------------------------------------------------------------------------------
//	GetData
//-----------------------------------------------------------------------------------
//	Gets some data from our view. Subclasses should override to handle their own
//	defined data tags. If a tag is not understood by the subclass, it should call the
//	inherited method. As a convienience, we map the request for ControlKind into our
//	GetKind method.
//
OSStatus TView::GetData(
	OSType				inTag,
	ControlPartCode		inPart,
	Size				inSize,
	Size*				outSize,
	void*				inPtr )
{
#pragma unused( inPart )

	OSStatus			err = noErr;
	
	switch( inTag )
	{
		case kControlKindTag:
			if ( inPtr )
			{
				if ( inSize != sizeof( ControlKind ) )
					err = errDataSizeMismatch;
				else
					( *(ControlKind *) inPtr ) = GetKind();
			}
			*outSize = sizeof( ControlKind );
			break;
		
		default:
			err = eventNotHandledErr;
			break;
	}
	
	return err;
}

//-----------------------------------------------------------------------------------
//	SetData
//-----------------------------------------------------------------------------------
//	Sets some data on our control. Subclasses should override to handle their own
//	defined data tags. If a tag is not understood by the subclass, it should call the
//	inherited method.
//
OSStatus TView::SetData(
	OSType				inTag,
	ControlPartCode		inPart,
	Size				inSize,
	const void*			inPtr )
{
#pragma unused( inTag, inPart, inSize, inPtr )

	return eventNotHandledErr;
}

//-----------------------------------------------------------------------------------
//	GetOptimalSize
//-----------------------------------------------------------------------------------
//	Someone wants to know this view's optimal size and text baseline, probably to help
//	do some type of layout. The base class does nothing, but subclasses should
//	override and do something meaningful here.
//
OSStatus TView::GetOptimalSize(
	HISize*				outSize,
	float*				outBaseLine )
{
#pragma unused( outSize, outBaseLine )

	return eventNotHandledErr;
}

//-----------------------------------------------------------------------------------
//	GetSizeConstraints
//-----------------------------------------------------------------------------------
//	Someone wants to know this view's minimum and maximum sizes, probably to help
//	do some type of layout. The base class does nothing, but subclasses should
//	override and do something meaningful here.
//
OSStatus TView::GetSizeConstraints(
	HISize*				outMin,
	HISize*				outMax )
{
#pragma unused( outMin, outMax )

	return eventNotHandledErr;
}

//-----------------------------------------------------------------------------------
//	BoundsChanged
//-----------------------------------------------------------------------------------
//	The bounds of our view have changed. Subclasses can override here to make note
//	of it and flush caches, etc. The base class does nothing.
//
OSStatus TView::BoundsChanged(
	UInt32 				inOptions,
	const HIRect& 		inOriginalBounds,
	const HIRect& 		inCurrentBounds,
	RgnHandle 			inInvalRgn )
{
#pragma unused( inOptions, inOriginalBounds, inCurrentBounds, inInvalRgn )

	return eventNotHandledErr;
}

//-----------------------------------------------------------------------------------
//	ControlHit
//-----------------------------------------------------------------------------------
//	The was hit.  Subclasses can overide to care about what part was hit.
//
OSStatus TView::ControlHit(
	ControlPartCode		inPart,
	UInt32				inModifiers )
{
#pragma unused( inPart, inModifiers )

	return eventNotHandledErr;
}

//-----------------------------------------------------------------------------------
//	HiliteChanged
//-----------------------------------------------------------------------------------
//	The hilite of our view has changed. Subclasses can override here to make note
//	of it and flush caches, etc. The base class does nothing.
//
OSStatus TView::HiliteChanged(
	ControlPartCode		inOriginalPart,
	ControlPartCode		inCurrentPart,
	RgnHandle 			inInvalRgn )
{
#pragma unused( inOriginalPart, inCurrentPart, inInvalRgn )

	return eventNotHandledErr;
}

//-----------------------------------------------------------------------------------
//	DragEnter
//-----------------------------------------------------------------------------------
//	A drag has entered our bounds. The Drag and Drop interface also should have been
//	activated or else this method will NOT be called. If true is returned, this view
//	likes the drag and will receive drag within/leave/receive messages as appropriate.
//	If false is returned, it is assumed the drag is not valid for this view, and no
//	further drag activity will flow into this view unless the drag leaves and is
//	re-entered.
//
bool TView::DragEnter(
	DragRef				inDrag )
{
#pragma unused( inDrag )

	return false;
}

//-----------------------------------------------------------------------------------
//	DragWithin
//-----------------------------------------------------------------------------------
//	A drag has moved within our bounds. In order for this method to be called, the
//	view must have signaled the drag as being desirable in the DragEnter method. The
//	Drag and Drop interface also should have been activated.
//
bool TView::DragWithin(
	DragRef				inDrag )
{
#pragma unused( inDrag )

	return false;
}

//-----------------------------------------------------------------------------------
//	DragLeave
//-----------------------------------------------------------------------------------
//	A drag has left. Deal with it. Subclasses should override as necessary. The
//	Drag and Drop interface should be activated in order for this method to be valid.
//	The drag must have also been accepted in the DragEnter method, else this method
//	will NOT be called.
//
bool TView::DragLeave(
	DragRef				inDrag )
{
#pragma unused( inDrag )

	return false;
}

//-----------------------------------------------------------------------------------
//	DragReceive
//-----------------------------------------------------------------------------------
//	Deal with receiving a drag. By default we return dragNotAcceptedErr. I'm not sure
//	if this is correct, or eventNotHandledErr. Time will tell...
//
OSStatus TView::DragReceive(
	DragRef				inDrag )
{
#pragma unused( inDrag )

	return dragNotAcceptedErr;
}

//-----------------------------------------------------------------------------------
//	Track
//-----------------------------------------------------------------------------------
//	Default tracking method. Subclasses should override as necessary. We do nothing
//	here in the base class, so we return eventNotHandledErr.
//
OSStatus TView::Track(
	TCarbonEvent&		inEvent,
	ControlPartCode*	outPart )
{
#pragma unused( inEvent, outPart )

	return eventNotHandledErr;
}

//-----------------------------------------------------------------------------------
//	SetFocusPart
//-----------------------------------------------------------------------------------
//	Handle focusing. Our base behavior is to punt.
//
OSStatus TView::SetFocusPart(
	ControlPartCode		inDesiredFocus,
	RgnHandle			inInvalidRgn,
	Boolean				inFocusEverything,
	ControlPartCode*	outActualFocus )
{
#pragma unused( inDesiredFocus, inInvalidRgn, inFocusEverything, outActualFocus )

	return eventNotHandledErr;
}

//-----------------------------------------------------------------------------------
//	ProcessCommand
//-----------------------------------------------------------------------------------
//	Process a command. Subclasses should override as necessary.
//
OSStatus TView::ProcessCommand(
	const HICommand&	inCommand )
{
#pragma unused( inCommand )

	return eventNotHandledErr;
}

//-----------------------------------------------------------------------------------
//	UpdateCommandStatus
//-----------------------------------------------------------------------------------
//	Update the status for a command. Subclasses should override as necessary.
//
OSStatus
TView::UpdateCommandStatus(
	const HICommand&	inCommand )
{
#pragma unused( inCommand )

	return eventNotHandledErr;
}

OSStatus TView::MouseDown(HIPoint& /*inMouseLocation*/, UInt32 /*inKeyModifiers*/, EventMouseButton /*inMouseButton*/, UInt32 /*inClickCount*/ ,
			  TCarbonEvent& /*inEvent*/)
{
	return eventNotHandledErr;
}

OSStatus TView::MouseUp(HIPoint& /*inMouseLocation*/, UInt32 /*inKeyModifiers*/, EventMouseButton /*inMouseButton*/, UInt32 /*inClickCount*/ )
{
	return eventNotHandledErr;
}

OSStatus TView::MouseDragged(HIPoint& /*inMouseLocation*/, UInt32 /*inKeyModifiers*/, EventMouseButton /*inMouseButton*/, UInt32 /*inClickCount*/ )
{
	return eventNotHandledErr;
}

OSStatus TView::MouseEntered(HIPoint& /*inMouseLocation*/, UInt32 /*inKeyModifiers*/, EventMouseButton /*inMouseButton*/, UInt32 /*inClickCount*/ )
{
	return eventNotHandledErr;
}

OSStatus TView::MouseExited(HIPoint& /*inMouseLocation*/, UInt32 /*inKeyModifiers*/, EventMouseButton /*inMouseButton*/, UInt32 /*inClickCount*/ )
{
	return eventNotHandledErr;
}

OSStatus TView::MouseWheelMoved( EventMouseWheelAxis /*inAxis*/, SInt32 /*inDelta*/, UInt32 /*inKeyModifiers*/ )
{
	return eventNotHandledErr;
}

OSStatus TView::ContextualMenuClick( HIPoint& /*inMouseLocation*/ )
{
	return eventNotHandledErr;
}


//-----------------------------------------------------------------------------------
//	ActivateInterface
//-----------------------------------------------------------------------------------
//	This routine is used to allow a subclass to turn on a specific event or suite of
//	events, like Drag and Drop. This allows us to keep event traffic down if we are
//	not interested, but register for the events if we are.
//
OSStatus TView::ActivateInterface(
	TView::Interface	inInterface )
{
	OSStatus		result = noErr;
	
	switch( inInterface )
	{
		case kDragAndDrop:
			{
				static const EventTypeSpec kDragEvents[] =
				{
					{ kEventClassControl, kEventControlDragEnter },			
					{ kEventClassControl, kEventControlDragLeave },			
					{ kEventClassControl, kEventControlDragWithin },			
					{ kEventClassControl, kEventControlDragReceive }
				};
				
				result = AddEventTypesToHandler( fHandler, GetEventTypeCount( kDragEvents ),
						kDragEvents );

				SetControlDragTrackingEnabled( GetViewRef(), true);
			}
			break;

		case kKeyboardFocus:
			{
				static const EventTypeSpec kKeyboardFocusEvents[] =
				{
					{ kEventClassControl, kEventControlSetFocusPart },
					{ kEventClassTextInput, kEventTextInputUnicodeForKeyEvent },
				};
				
				result = AddEventTypesToHandler( fHandler, GetEventTypeCount( kKeyboardFocusEvents ),
						kKeyboardFocusEvents );
			}
			break;

		case kMouse:
			{
				if ( mouseEventHandler == NULL )
					{
					// This case is quite different: Mouse events are delivered to the window, not the controls.
					// mouse wheel events are the exception: They ARE delivered to the control
					static const EventTypeSpec kControlMouseEvents[] =
					{
						{ kEventClassMouse, kEventMouseWheelMoved },
						{ kEventClassControl, kEventControlContextualMenuClick },
						// So we can reinstall our events when the window changes
						{ kEventClassControl, kEventControlOwningWindowChanged }, 
					};

					result = AddEventTypesToHandler( fHandler, GetEventTypeCount( kControlMouseEvents ),
									  kControlMouseEvents );
					}

				if ( this->GetOwner() != NULL )
					{
					// We use "-1" to indicate that we want to install an event handler once we get a window
					if ( mouseEventHandler != NULL &&  mouseEventHandler != reinterpret_cast<void*>( -1 ) )
						{
						result = RemoveEventHandler( mouseEventHandler );
						}
					mouseEventHandler = NULL;
					
					static const EventTypeSpec kWindowMouseEvents[] =
					{
						{ kEventClassMouse, kEventMouseDown },
						{ kEventClassMouse, kEventMouseUp },
						{ kEventClassMouse, kEventMouseMoved },
						{ kEventClassMouse, kEventMouseDragged },
					};

					result = InstallEventHandler( GetWindowEventTarget( this->GetOwner() ), WindowEventHandler,
								      GetEventTypeCount( kWindowMouseEvents ), kWindowMouseEvents, 
								      this, &mouseEventHandler );
					}
				// If we have no window yet. Set the mouseEventHandler to -1 so when we get one we
				// will install the event handler
				else
					{
					mouseEventHandler = reinterpret_cast<EventHandlerRef>( -1 );
					}
			}
			break;
		case kMouseTracking:
		  {
		    {
		      static const EventTypeSpec kControlMouseEvents[] =
			{
			  { kEventClassMouse, kEventMouseEntered }, // only works if mousetracking is started
			  { kEventClassMouse, kEventMouseExited }, // only works if mousetracking is started
			};
		      
		      result = AddEventTypesToHandler( fHandler, GetEventTypeCount( kControlMouseEvents ),
						       kControlMouseEvents );
		    }
		  }

		//default:
		//	assert( false );
	}
	
	return result;
}

OSStatus TView::InstallTimer( EventTimerInterval inFireDelay, EventLoopTimerRef* outTimer )
{
	return InstallEventLoopTimer( GetCurrentEventLoop(), inFireDelay, inFireDelay, TimerEventHandler, this, outTimer );
}

//-----------------------------------------------------------------------------------
//	RegisterSubclass
//-----------------------------------------------------------------------------------
//	This routine should be called by subclasses so they can be created as HIObjects.
//
OSStatus TView::RegisterSubclass(
	CFStringRef			inID,
	ConstructProc		inProc )
{
	return HIObjectRegisterSubclass( inID, kHIViewClassID, 0, ObjectEventHandler,
			GetEventTypeCount( kHIObjectEvents ), kHIObjectEvents, (void*) inProc, NULL );
}

//-----------------------------------------------------------------------------------
//	ObjectEventHandler
//-----------------------------------------------------------------------------------
//	Our static event handler proc. We handle any HIObject based events directly in
// 	here at present.
//
pascal OSStatus TView::ObjectEventHandler(
	EventHandlerCallRef	inCallRef,
	EventRef			inEvent,
	void*				inUserData )
{
	OSStatus			result = eventNotHandledErr;
	TView*				view = (TView*) inUserData;
	TCarbonEvent		event( inEvent );
	
	switch ( event.GetClass() )
	{
		case kEventClassHIObject:
			switch ( event.GetKind() )
			{
				case kEventHIObjectConstruct:
					{
						ControlRef		control; // ControlRefs are HIObjectRefs
						TView*			view;

						result = event.GetParameter<HIObjectRef>( kEventParamHIObjectInstance,
								typeHIObjectRef, (HIObjectRef*)&control );
						require_noerr( result, ParameterMissing );
						
						// on entry for our construct event, we're passed the
						// creation proc we registered with for this class.
						// we use it now to create the instance, and then we
						// replace the instance parameter data with said instance
						// as type void.

						result = (*(ConstructProc)inUserData)( control, &view );
						if ( result == noErr )
							event.SetParameter<TViewPtr>( kEventParamHIObjectInstance,
									typeVoidPtr, view ); 
					}
					break;
				
				case kEventHIObjectInitialize:
					result = CallNextEventHandler( inCallRef, inEvent );
					if ( result == noErr )
						result = view->Initialize( event );
					break;
				
				case kEventHIObjectDestruct:
					delete view;
					break;
			}
			break;
	}

ParameterMissing:

	return result;
}

//-----------------------------------------------------------------------------------
//	ViewEventHandler
//-----------------------------------------------------------------------------------
//	Our static event handler proc. We handle all non-HIObject events here.
//
pascal OSStatus TView::ViewEventHandler(
	EventHandlerCallRef	inCallRef,
	EventRef			inEvent,
	void*				inUserData )
{
	OSStatus			result;
	TView*				view = (TView*) inUserData;
	TCarbonEvent		event( inEvent );
	//if (view->debugPrint)
	//  fprintf(stderr,"TView::ViewEventHandler\n");
	result = view->HandleEvent( inCallRef, event );

	return result;
}


pascal OSStatus TView::WindowEventHandler(
	EventHandlerCallRef	inCallRef,
	EventRef			inEvent,
	void*				inUserData )
{
	TView* view = reinterpret_cast<TView*>( inUserData );
	TCarbonEvent event( inEvent );

	const WindowRef window = view->GetOwner();
	if (NULL == window)
		return eventNotHandledErr;

	// If the window is not active, let the standard window handler execute.
	if ( ! IsWindowActive( window ) ) return eventNotHandledErr;
	if (view->debugPrint)
	  fprintf(stderr,"TView::WindowEventHandler\n");
	
	const HIViewRef rootView = HIViewGetRoot( window );
	if (NULL == rootView)
		return eventNotHandledErr;

	// TODO: On OS X 10.3, test if this bug still exists
	// This is a hack to work around a bug in the OS. See:
	// http://lists.apple.com/archives/carbon-development/2002/Sep/29/keventmousemovedeventsno.004.txt
	OSStatus err;
	if ( event.GetKind() == kEventMouseMoved )
	{
		// We need to set some parameters correctly
		event.SetParameter( kEventParamWindowRef, window );

		HIPoint ptMouse;
		event.GetParameter( kEventParamMouseLocation, &ptMouse );

		// convert screen coords to window relative
		Rect bounds;
		err = GetWindowBounds( window, kWindowStructureRgn, &bounds );
		if( err == noErr )
		{
			ptMouse.x -= bounds.left;
			ptMouse.y -= bounds.top;
			event.SetParameter( kEventParamWindowMouseLocation, ptMouse );
		}
	}
	
	HIViewRef targetView = NULL;
	err = HIViewGetViewForMouseEvent( rootView, inEvent, &targetView );
	//if (view->debugPrint)
	//  fprintf(stderr,"TView::WindowEventHandler root[%08X] viewRef[%08X] targetView[%08X]\n", rootView, view->GetViewRef(), targetView);
	if ( targetView == view->GetViewRef() || event.GetKind() == kEventMouseDragged )
		{
		return view->HandleEvent( inCallRef, event );
		}

	return eventNotHandledErr;
}

pascal void TView::TimerEventHandler( EventLoopTimerRef inTimer, void* view )
{
	reinterpret_cast<TView*>( view )->TimerFired( inTimer );
}

//-----------------------------------------------------------------------------------
//	HandleEvent
//-----------------------------------------------------------------------------------
//	Our object's virtual event handler method. I'm not sure if we need this these days.
//	We used to do various things with it, but those days are long gone...
//
OSStatus TView::HandleEvent(
	EventHandlerCallRef	inCallRef,
	TCarbonEvent&		inEvent )
{
#pragma unused( inCallRef )

	OSStatus			result = eventNotHandledErr;
	HIPoint				where;
	OSType				tag;
	void *				ptr;
	Size				size, outSize;
	UInt32				features;
	RgnHandle			region = NULL;
	ControlPartCode		part;
	RgnHandle			invalRgn;
	
	switch ( inEvent.GetClass() )
	{
		case kEventClassCommand:
			{
				HICommand		command;
				
				result = inEvent.GetParameter( kEventParamDirectObject, &command );
				require_noerr( result, MissingParameter );
				
				switch ( inEvent.GetKind() )
				{
					case kEventCommandProcess:
						result = ProcessCommand( command );
						break;
					
					case kEventCommandUpdateStatus:
						result = UpdateCommandStatus( command );
						break;
				}
			}
			break;

		case kEventClassControl:
			switch ( inEvent.GetKind() )
			{
				case kEventControlInitialize:
					features = GetBehaviors();
					inEvent.SetParameter( kEventParamControlFeatures, features );
					result = noErr;
					break;
					
				case kEventControlDraw:
					{
						CGContextRef		context = NULL;
						
						inEvent.GetParameter( kEventParamRgnHandle, &region );
						inEvent.GetParameter<CGContextRef>( kEventParamCGContextRef, typeCGContextRef, &context );

						Draw( region, context );
						result = noErr;
					}
					break;

				case kEventControlHitTest:
					inEvent.GetParameter<HIPoint>( kEventParamMouseLocation, typeHIPoint, &where );
					part = HitTest( where );
					inEvent.SetParameter<ControlPartCode>( kEventParamControlPart, typeControlPartCode, part );
					result = noErr;
					break;
					
				case kEventControlGetPartRegion:
					inEvent.GetParameter<ControlPartCode>( kEventParamControlPart, typeControlPartCode, &part );
					inEvent.GetParameter( kEventParamControlRegion, &region );
					result = GetRegion( part, region );
					break;
				
				case kEventControlGetData:
					inEvent.GetParameter<ControlPartCode>( kEventParamControlPart, typeControlPartCode, &part );
					inEvent.GetParameter<OSType>( kEventParamControlDataTag, typeEnumeration, &tag );
					inEvent.GetParameter<Ptr>( kEventParamControlDataBuffer, typePtr, (Ptr*)&ptr );
					inEvent.GetParameter<Size>( kEventParamControlDataBufferSize, typeLongInteger, &size );

					result = GetData( tag, part, size, &outSize, ptr );

					if ( result == noErr )
						verify_noerr( inEvent.SetParameter<Size>( kEventParamControlDataBufferSize, typeLongInteger, outSize ) );
					break;
				
				case kEventControlSetData:
					inEvent.GetParameter<ControlPartCode>( kEventParamControlPart, typeControlPartCode, &part );
					inEvent.GetParameter<OSType>( kEventParamControlDataTag, typeEnumeration, &tag );
					inEvent.GetParameter<Ptr>( kEventParamControlDataBuffer, typePtr, (Ptr*)&ptr );
					inEvent.GetParameter<Size>( kEventParamControlDataBufferSize, typeLongInteger, &size );

					result = SetData( tag, part, size, ptr );
					break;
				
				case kEventControlGetOptimalBounds:
					{
						HISize		size;
						float		floatBaseLine;
						
						result = GetOptimalSize( &size, &floatBaseLine );
						if ( result == noErr )
						{
							Rect		bounds;
							SInt16		baseLine;

							GetControlBounds( GetViewRef(), &bounds );

							bounds.bottom = bounds.top + (SInt16)size.height;
							bounds.right = bounds.left + (SInt16)size.width;
							baseLine = (SInt16)floatBaseLine;
							
							inEvent.SetParameter( kEventParamControlOptimalBounds, bounds );
							inEvent.SetParameter<SInt16>( kEventParamControlOptimalBaselineOffset, typeShortInteger, baseLine );
						}
					}
					break;
				
				case kEventControlBoundsChanged:
					{
						HIRect		prevRect, currRect;
						UInt32		attrs;
						
						inEvent.GetParameter( kEventParamAttributes, &attrs );
						inEvent.GetParameter( kEventParamOriginalBounds, &prevRect );
						inEvent.GetParameter( kEventParamCurrentBounds, &currRect );
						inEvent.GetParameter( kEventParamControlInvalRgn, &invalRgn );

						result = BoundsChanged( attrs, prevRect, currRect, invalRgn );

						if ( mouseEventHandler != NULL )
							{
							ActivateInterface( kMouse );
							}

					}
					break;

				case kEventControlHit:
					{
						UInt32		modifiers;
						
						inEvent.GetParameter<ControlPartCode>( kEventParamControlPart, typeControlPartCode, &part );
						inEvent.GetParameter( kEventParamKeyModifiers, &modifiers );
	
						result = ControlHit( part, modifiers );
					}
					break;
				
				case kEventControlHiliteChanged:
					{
						ControlPartCode	prevPart, currPart;
						
						inEvent.GetParameter<ControlPartCode>( kEventParamControlPreviousPart, typeControlPartCode, &prevPart );
						inEvent.GetParameter<ControlPartCode>( kEventParamControlCurrentPart, typeControlPartCode, &currPart );
						inEvent.GetParameter( kEventParamControlInvalRgn, &invalRgn );

						result = HiliteChanged( prevPart, currPart, invalRgn );
						
						if ( GetAutoInvalidateFlags() & kAutoInvalidateOnHilite )
							Invalidate();
					}
					break;
					
				case kEventControlActivate:
					result = ActiveStateChanged();

					if ( GetAutoInvalidateFlags() & kAutoInvalidateOnActivate )
						Invalidate();
					break;
					
				case kEventControlDeactivate:
					result = ActiveStateChanged();

					if ( GetAutoInvalidateFlags() & kAutoInvalidateOnActivate )
						Invalidate();
					break;
					
				case kEventControlValueFieldChanged:
					result = ValueChanged();

					if ( GetAutoInvalidateFlags() & kAutoInvalidateOnValueChange )
						Invalidate();
					break;
					
				case kEventControlTitleChanged:
					result = TitleChanged();

					if ( GetAutoInvalidateFlags() & kAutoInvalidateOnTitleChange )
						Invalidate();
					break;
					
				case kEventControlEnabledStateChanged:
					result = EnabledStateChanged();

					if ( GetAutoInvalidateFlags() & kAutoInvalidateOnEnable )
						Invalidate();
					break;
					
				case kEventControlDragEnter:
				case kEventControlDragLeave:
				case kEventControlDragWithin:
					{
						DragRef		drag;
						bool		likesDrag;
						
						inEvent.GetParameter( kEventParamDragRef, &drag );

						switch ( inEvent.GetKind() )
						{
							case kEventControlDragEnter:
								likesDrag = DragEnter( drag );
								// Why only if likesDrag?  What if it doesn't?  No parameter?
								if ( likesDrag )
									result = inEvent.SetParameter( kEventParamControlLikesDrag, likesDrag );
								break;
							
							case kEventControlDragLeave:
								DragLeave( drag );
								result = noErr;
								break;
							
							case kEventControlDragWithin:
								DragWithin( drag );
								result = noErr;
								break;
						}
					}
					break;
				
				case kEventControlDragReceive:
					{
						DragRef		drag;
						
						inEvent.GetParameter( kEventParamDragRef, &drag );

						result = DragReceive( drag );
					}
					break;
				
				case kEventControlTrack:
					{
						ControlPartCode		part;
						
						result = Track( inEvent, &part );
						if ( result == noErr )
							verify_noerr( inEvent.SetParameter<ControlPartCode>( kEventParamControlPart, typeControlPartCode, part ) );
					}
					break;

				case kEventControlGetSizeConstraints:
					{
						HISize		minSize, maxSize;
						
						result = GetSizeConstraints( &minSize, &maxSize );

						if ( result == noErr )
						{
							verify_noerr( inEvent.SetParameter( kEventParamMinimumSize, minSize ) );
							verify_noerr( inEvent.SetParameter( kEventParamMaximumSize, maxSize ) );
						}
					}
					break;

				case kEventControlSetFocusPart:
					{
						ControlPartCode		desiredFocus;
						RgnHandle			invalidRgn;
						Boolean				focusEverything;
						ControlPartCode		actualFocus;
						
						result = inEvent.GetParameter<ControlPartCode>( kEventParamControlPart, typeControlPartCode, &desiredFocus ); 
						require_noerr( result, MissingParameter );
						
						inEvent.GetParameter( kEventParamControlInvalRgn, &invalidRgn );

						focusEverything = false; // a good default in case the parameter doesn't exist

						inEvent.GetParameter( kEventParamControlFocusEverything, &focusEverything );

						result = SetFocusPart( desiredFocus, invalidRgn, focusEverything, &actualFocus );
						
						if ( result == noErr )
							verify_noerr( inEvent.SetParameter<ControlPartCode>( kEventParamControlPart, typeControlPartCode, actualFocus ) );
					}
					break;

				case kEventControlOwningWindowChanged:
					{
						// If our owning window has changed, reactivate the mouse interface
						if ( mouseEventHandler != NULL )
							{
							ActivateInterface( kMouse );
							}
					}
					break;

				case kEventControlContextualMenuClick:
					{
						HIPoint ptMouse;
						inEvent.GetParameter( kEventParamMouseLocation, &ptMouse );
						result = ContextualMenuClick( ptMouse );
					}
					break;
					
				// some other kind of Control event
				default:
					break;
			}
			break;
			
		case kEventClassTextInput:
			result = TextInput( inEvent );
			break;

		case kEventClassMouse:
		{
			result = inEvent.GetParameter<HIPoint>( kEventParamWindowMouseLocation, typeHIPoint, &where );
			HIViewConvertPoint( &where, NULL, fViewRef );
			
			UInt32 inKeyModifiers;
			result = inEvent.GetParameter( kEventParamKeyModifiers, &inKeyModifiers );
			if( result != noErr )
				inKeyModifiers = 0;
			EventMouseButton inMouseButton = 0;
			result = inEvent.GetParameter<EventMouseButton>( kEventParamMouseButton, typeMouseButton, &inMouseButton );
			if (noErr != result)
				// assume no button is pressed if there is no button info
				inMouseButton = 0;
			UInt32 inClickCount;
			result = inEvent.GetParameter( kEventParamClickCount, &inClickCount );
			if( result != noErr )
				inClickCount = 0;
				
			switch ( inEvent.GetKind() )
			{
				case kEventMouseWheelMoved:
				{
					EventMouseWheelAxis inAxis;
					result = inEvent.GetParameter<EventMouseWheelAxis>( kEventParamMouseWheelAxis, typeMouseWheelAxis, &inAxis );

					SInt32 inDelta;
					result = inEvent.GetParameter<SInt32>( kEventParamMouseWheelDelta, typeSInt32, &inDelta );

					result = MouseWheelMoved( inAxis, inDelta, inKeyModifiers );
					break;				
				}
				case kEventMouseDown:
					result = MouseDown( where, inKeyModifiers, inMouseButton, inClickCount, inEvent );
					break;
				case kEventMouseUp:
					result = MouseUp( where, inKeyModifiers, inMouseButton, inClickCount );
					break;
				case kEventMouseExited:
					result = MouseExited( where, inKeyModifiers, inMouseButton, inClickCount );
					break;
				case kEventMouseEntered:
					result = MouseEntered( where, inKeyModifiers, inMouseButton, inClickCount );
					break;
				case kEventMouseMoved:
				case kEventMouseDragged:
					result = MouseDragged( where, inKeyModifiers, inMouseButton, inClickCount );
					break;
				default:;
			}
			break;
                }
		// some other event class
		default:
			break;
	}

MissingParameter:
	return result;
}

//-----------------------------------------------------------------------------------
//	CreateInitializationEvent
//-----------------------------------------------------------------------------------
// 	Create a basic intialization event containing the parent control and bounds. At
//	present we set the bounds to empty and the parent to NULL. In theory, after creating
//	this event, any subclass could add its own parameter to receive in its
//	Initialize method.
//
EventRef TView::CreateInitializationEvent()
{
	OSStatus		result = noErr;
	EventRef		event;

	result = CreateEvent( NULL, kEventClassHIObject, kEventHIObjectInitialize,
					GetCurrentEventTime(), 0, &event );
	require_noerr_action( result, CantCreateEvent, event = NULL );
		
CantCreateEvent:
	return event;
}

//-----------------------------------------------------------------------------------
//	Frame
//-----------------------------------------------------------------------------------
//
HIRect TView::Frame()
{
	HIRect		frame;

	HIViewGetFrame( GetViewRef(), &frame );
	
	return frame;
}

//-----------------------------------------------------------------------------------
//	SetFrame
//-----------------------------------------------------------------------------------
//
OSStatus TView::SetFrame(
	const HIRect&			inFrame )
{
	OSStatus				err;
	
	err = HIViewSetFrame( GetViewRef(), &inFrame );
	
	return err;
}

//-----------------------------------------------------------------------------------
//	Bounds
//-----------------------------------------------------------------------------------
//
HIRect TView::Bounds()
{
	HIRect		bounds;
	
	HIViewGetBounds( GetViewRef(), &bounds );
	
	return bounds;
}

//-----------------------------------------------------------------------------------
//	Show
//-----------------------------------------------------------------------------------
//
OSStatus TView::Show()
{
	return HIViewSetVisible( GetViewRef(), true );
}

//-----------------------------------------------------------------------------------
//	Hide
//-----------------------------------------------------------------------------------
//
OSStatus TView::Hide()
{
	return HIViewSetVisible( GetViewRef(), false );
}

//-----------------------------------------------------------------------------------
//	GetEventTarget
//-----------------------------------------------------------------------------------
//
EventTargetRef TView::GetEventTarget()
{
	return HIObjectGetEventTarget( (HIObjectRef) GetViewRef() );
}

//-----------------------------------------------------------------------------------
//	AddSubView
//-----------------------------------------------------------------------------------
//
OSStatus TView::AddSubView(
	TView*				inSubView )
{
	return HIViewAddSubview( GetViewRef(), inSubView->GetViewRef() );;
}

//-----------------------------------------------------------------------------------
//	RemoveFromSuperView
//-----------------------------------------------------------------------------------
//
OSStatus TView::RemoveFromSuperView()
{
	return HIViewRemoveFromSuperview( GetViewRef() );
}

//-----------------------------------------------------------------------------------
//	GetHilite
//-----------------------------------------------------------------------------------
//
ControlPartCode TView::GetHilite()
{
	return GetControlHilite( GetViewRef() );
}

//-----------------------------------------------------------------------------------
//	GetValue
//-----------------------------------------------------------------------------------
//
SInt32 TView::GetValue()
{
	return GetControl32BitValue( GetViewRef() );
}

//-----------------------------------------------------------------------------------
//	SetValue
//-----------------------------------------------------------------------------------
//
void TView::SetValue(
	SInt32					inValue )
{
	SetControl32BitValue( GetViewRef(), inValue );
}

//-----------------------------------------------------------------------------------
//	GetMinimum
//-----------------------------------------------------------------------------------
//
SInt32 TView::GetMinimum()
{
	return GetControlMinimum( GetViewRef() );
}

//-----------------------------------------------------------------------------------
//	SetMinimum
//-----------------------------------------------------------------------------------
//
void TView::SetMinimum(
	SInt32					inMinimum )
{
	SetControlMinimum( GetViewRef(), inMinimum );
}

//-----------------------------------------------------------------------------------
//	GetMaximum
//-----------------------------------------------------------------------------------
//
SInt32 TView::GetMaximum()
{
	return GetControlMaximum( GetViewRef() );
}

//-----------------------------------------------------------------------------------
//	SetMaximum
//-----------------------------------------------------------------------------------
//
void TView::SetMaximum(
	SInt32					inMaximum )
{
	SetControlMaximum( GetViewRef(), inMaximum );
}

//-----------------------------------------------------------------------------------
//	GetOwner
//-----------------------------------------------------------------------------------
//
WindowRef TView::GetOwner()
{
	return GetControlOwner( GetViewRef() );
}

//-----------------------------------------------------------------------------------
//	Hilite
//-----------------------------------------------------------------------------------
//
void TView::Hilite(
	ControlPartCode			inPart)
{
	return HiliteControl( GetViewRef(), inPart );
}

//-----------------------------------------------------------------------------------
//	Invalidate
//-----------------------------------------------------------------------------------
//
OSStatus TView::Invalidate()
{
	return HIViewSetNeedsDisplay( GetViewRef(), true );
}

void TView::TimerFired( EventLoopTimerRef inTimer )
{
#pragma unused( inTimer )
}


//-----------------------------------------------------------------------------------
//	IsVisible
//-----------------------------------------------------------------------------------
//
Boolean TView::IsVisible()
{
	return IsControlVisible( GetViewRef() );
}

//-----------------------------------------------------------------------------------
//	IsEnabled
//-----------------------------------------------------------------------------------
//
Boolean TView::IsEnabled()
{
	return IsControlEnabled( GetViewRef() );
}

//-----------------------------------------------------------------------------------
//	IsActive
//-----------------------------------------------------------------------------------
//
Boolean TView::IsActive()
{
	return IsControlActive( GetViewRef() );
}

//-----------------------------------------------------------------------------------
//	ActiveStateChanged
//-----------------------------------------------------------------------------------
//	Default activation method. Subclasses should override as necessary. We do nothing
//	here in the base class, so we return eventNotHandledErr.
//
OSStatus TView::ActiveStateChanged()
{
	return eventNotHandledErr;
}

//-----------------------------------------------------------------------------------
//	ValueChanged
//-----------------------------------------------------------------------------------
//	Default value changed method. Subclasses should override as necessary. We do
//	nothing here in the base class, so we return eventNotHandledErr.
//
OSStatus TView::ValueChanged()
{
	return eventNotHandledErr;
}

//-----------------------------------------------------------------------------------
//	TitleChanged
//-----------------------------------------------------------------------------------
//	Default title changed method. Subclasses should override as necessary. We
//	do nothing here in the base class, so we return eventNotHandledErr.
//
OSStatus TView::TitleChanged()
{
	return eventNotHandledErr;
}

//-----------------------------------------------------------------------------------
//	EnabledStateChanged
//-----------------------------------------------------------------------------------
//	Default enable method. Subclasses should override as necessary. We
//	do nothing here in the base class, so we return eventNotHandledErr.
//
OSStatus TView::EnabledStateChanged()
{
	return eventNotHandledErr;
}

//-----------------------------------------------------------------------------------
//	TextInput
//-----------------------------------------------------------------------------------
//	Default text (Unicode) input method. Subclasses should override as necessary. We
//	do nothing here in the base class, so we return eventNotHandledErr.
//
OSStatus TView::TextInput(
	TCarbonEvent&		inEvent )
{
#pragma unused( inEvent )

	return eventNotHandledErr;
}

//-----------------------------------------------------------------------------------
//	ChangeAutoInvalidateFlags
//-----------------------------------------------------------------------------------
//	Change behavior for auto-invalidating views on certain actions.
//
void TView::ChangeAutoInvalidateFlags(
	OptionBits			inSetThese,
	OptionBits			inClearThese )
{
    fAutoInvalidateFlags = ( ( fAutoInvalidateFlags | inSetThese ) & ( ~inClearThese ) );
}
