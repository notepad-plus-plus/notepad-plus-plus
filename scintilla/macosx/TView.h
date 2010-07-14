/*
	File:		TView.h

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

#ifndef TView_H_
#define TView_H_

#include <Carbon/Carbon.h>

#include "TCarbonEvent.h"
#include "TRect.h"

#define PURE_VIRTUAL 0

class TView
{
public:
	// Bounds and Frame
	OSStatus				SetFrame(
								const HIRect&		inBounds );
	HIRect					Frame();
	HIRect					Bounds();
	
	// Visibility
	OSStatus				Show();
	OSStatus				Hide();

	EventTargetRef			GetEventTarget();

	OSStatus				AddSubView(
								TView*				inSubView );
	OSStatus				RemoveFromSuperView();

	// Accessors
	HIViewRef				GetViewRef() const
								{ return fViewRef; }
	void					Hilite(
								ControlPartCode		inPart );
	ControlPartCode			GetHilite();
	WindowRef				GetOwner();
	SInt32					GetValue();
	void					SetValue(
								SInt32				inValue );
	SInt32					GetMinimum();
	void					SetMinimum(
								SInt32				inMinimum );
	SInt32					GetMaximum();
	void					SetMaximum(
								SInt32				inMaximum );

	// State
	Boolean					IsVisible();
	Boolean					IsEnabled();
	Boolean					IsActive();
	
	OSStatus				Invalidate();		// was SetNeedsDisplay()

	// A "fake" event handler
	virtual void			TimerFired( EventLoopTimerRef inTimer );

protected:
	// Autoinvalidation
	enum					{ 
								kAutoInvalidateOnActivate 		= (1 << 0),
								kAutoInvalidateOnHilite			= (1 << 1),
								kAutoInvalidateOnEnable			= (1 << 2),
								kAutoInvalidateOnValueChange	= (1 << 3),
								kAutoInvalidateOnTitleChange	= (1 << 4)
							};
	void					ChangeAutoInvalidateFlags(
								OptionBits			inSetFlags,
								OptionBits			inClearFlags );
	OptionBits				GetAutoInvalidateFlags()
								{ return fAutoInvalidateFlags; }

	// Types
	typedef OSStatus		(*ConstructProc)(
								ControlRef			inBaseControl,
								TView**				outView );

	// Construction/Destruction
							TView( HIViewRef inControl );
	virtual					~TView();
	
	virtual ControlKind		GetKind() = PURE_VIRTUAL;
	virtual UInt32			GetBehaviors();

	// Handlers
	virtual OSStatus		ActiveStateChanged();
	virtual OSStatus		BoundsChanged(
								UInt32				inOptions,
								const HIRect&		inOriginalBounds,
								const HIRect&		inCurrentBounds,
								RgnHandle			inInvalRgn );
	virtual OSStatus		ControlHit(
								ControlPartCode		inPart,
								UInt32				inModifiers );
	virtual OSStatus		EnabledStateChanged();
	virtual void			Draw(
								RgnHandle			inLimitRgn,
								CGContextRef		inContext );
	virtual OSStatus		GetData(
								OSType				inTag,
								ControlPartCode		inPart,
								Size				inSize,
								Size*				outSize,
								void*				inPtr );
	virtual OSStatus		GetRegion(
								ControlPartCode		inPart,
								RgnHandle			outRgn );
	virtual OSStatus		HiliteChanged(
								ControlPartCode		inOriginalPart,
								ControlPartCode		inCurrentPart,
								RgnHandle			inInvalRgn );
	virtual ControlPartCode	HitTest(
								const HIPoint&		inWhere );
	virtual OSStatus		Initialize(
								TCarbonEvent&		inEvent );
	virtual OSStatus		SetData(
								OSType				inTag,
								ControlPartCode		inPart,
								Size				inSize,
								const void*			inPtr );
	virtual OSStatus		SetFocusPart(
								ControlPartCode		inDesiredFocus,
								RgnHandle			inNnvalidRgn,
								Boolean				inFocusEverything,
								ControlPartCode*	outActualFocus );
	virtual OSStatus		TextInput(
								TCarbonEvent&		inEvent );
	virtual OSStatus		TitleChanged();
	virtual OSStatus		Track(
								TCarbonEvent&		inEvent,
								ControlPartCode*	outPartHit );
	virtual OSStatus		ValueChanged();
	
	// Sizing
	virtual OSStatus		GetSizeConstraints(
								HISize*				outMin,
								HISize*				outMax );
	virtual OSStatus		GetOptimalSize(
								HISize*				outSize,
								float*				outBaseLine );

	// Accessors
	WindowRef				GetWindowRef()
								{ return GetControlOwner( GetViewRef() ); }
	
	
	// Drag and drop
	virtual bool			DragEnter(
								DragRef				inDrag );
	virtual bool			DragWithin(
								DragRef				inDrag );
	virtual bool			DragLeave(
								DragRef				inDrag );
	virtual OSStatus		DragReceive(
								DragRef				inDrag );

	// Command processing
	virtual OSStatus		ProcessCommand(
								const HICommand&	inCommand );
	virtual OSStatus		UpdateCommandStatus(
								const HICommand&	inCommand );

	// Mouse events
	virtual OSStatus		MouseDown(
								HIPoint& inMouseLocation,
								UInt32 inKeyModifiers,
								EventMouseButton inMouseButton,
								UInt32 inClickCount,
								TCarbonEvent& inEvent);
	virtual OSStatus		MouseUp(
								HIPoint& inMouseLocation,
								UInt32 inKeyModifiers,
								EventMouseButton inMouseButton,
								UInt32 inClickCount );
	virtual OSStatus		MouseDragged(
								HIPoint& inMouseLocation,
								UInt32 inKeyModifiers,
								EventMouseButton inMouseButton,
								UInt32 inClickCount );
	virtual OSStatus		MouseEntered(
								HIPoint& inMouseLocation,
								UInt32 inKeyModifiers,
								EventMouseButton inMouseButton,
								UInt32 inClickCount );
	virtual OSStatus		MouseExited(
								HIPoint& inMouseLocation,
								UInt32 inKeyModifiers,
								EventMouseButton inMouseButton,
								UInt32 inClickCount );
	virtual OSStatus 		MouseWheelMoved( EventMouseWheelAxis inAxis, SInt32 inDelta, UInt32 inKeyModifiers );
	virtual OSStatus		ContextualMenuClick( HIPoint& inMouseLocation );
	
	// Utility
	static OSStatus			RegisterSubclass(
								CFStringRef			inID,
								ConstructProc		inProc );
	static EventRef			CreateInitializationEvent();
	enum Interface			{
								kDragAndDrop		= 1,
								kKeyboardFocus,
								kMouse,
								kMouseTracking
							};
	virtual OSStatus		ActivateInterface(
								Interface			inInterface );

	OSStatus				InstallTimer(
								EventTimerInterval	inFireDelay,
								EventLoopTimerRef*	outTimer );
	
	// Debugging
	virtual void			PrintDebugInfo();
	Boolean debugPrint;
private:
	static pascal OSStatus	ObjectEventHandler(
								EventHandlerCallRef	inCallRef,
								EventRef			inEvent,
								void*				inUserData );
	static pascal OSStatus	ViewEventHandler(
								EventHandlerCallRef	inCallRef,
								EventRef			inEvent,
								void*				inUserData );
	static pascal OSStatus	WindowEventHandler(
								EventHandlerCallRef	inCallRef,
								EventRef			inEvent,
								void*				inUserData );
	static pascal void 		TimerEventHandler(
								EventLoopTimerRef	inTimer,
										void*		inUserData );
	OSStatus				HandleEvent(
								EventHandlerCallRef	inCallRef,
								TCarbonEvent&		inEvent );

	HIViewRef				fViewRef;
	EventHandlerRef			fHandler;

	EventHandlerRef			mouseEventHandler;
	OptionBits				fAutoInvalidateFlags;

};

typedef TView*				TViewPtr;

#endif // TView_H_
