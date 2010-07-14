/*
    File:		TCarbonEvent.h
    
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

#ifndef TCarbonEvent_H_
#define TCarbonEvent_H_

#include <Carbon/Carbon.h>

class TCarbonEvent
{
public:
	// Construction/Destruction
	TCarbonEvent(
					UInt32				inClass,
					UInt32				inKind );
	TCarbonEvent(
					EventRef			inEvent );
	virtual ~TCarbonEvent();
	
	UInt32		GetClass() const;
	UInt32		GetKind() const;
	
	// Time
	void		SetTime(
					EventTime inTime );
	EventTime	GetTime() const;
	
	// Retention
	void		Retain();
	void		Release();
	
	// Accessors
	operator	EventRef&()
					{ return fEvent; };
	EventRef	GetEventRef()
					{ return fEvent; }
	
	// Posting
	OSStatus 	PostToQueue(
					EventQueueRef		inQueue,
					EventPriority		inPriority = kEventPriorityStandard );

	// Parameters
	OSStatus	SetParameter(
					EventParamName		inName,
					EventParamType		inType,
					UInt32				inSize,
					const void*			inData );
	OSStatus	GetParameter(
					EventParamName		inName,
					EventParamType		inType,
					UInt32				inBufferSize,
					void*				outData );

	OSStatus	GetParameterType(
					EventParamName		inName,
					EventParamType*		outType );
	OSStatus	GetParameterSize(
					EventParamName		inName,
					UInt32*				outSize );

	// Simple parameters
	OSStatus	SetParameter(
					EventParamName		inName,
					Boolean				inValue );
	OSStatus	GetParameter(
					EventParamName		inName,
					Boolean*			outValue );

	OSStatus	SetParameter(
					EventParamName		inName,
					bool				inValue );
	OSStatus	GetParameter(
					EventParamName		inName,
					bool*				outValue );

	OSStatus	SetParameter(
					EventParamName		inName,
					Point				inPt );
	OSStatus	GetParameter(
					EventParamName		inName,
					Point*				outPt );

	OSStatus	SetParameter(
					EventParamName		inName,
					const HIPoint&		inPt );

	OSStatus	GetParameter(
					EventParamName		inName,
					HIPoint*			outPt );

	OSStatus	SetParameter(
					EventParamName		inName,
					const Rect&			inRect );
	OSStatus	GetParameter(
					EventParamName		inName,
					Rect*				outRect );

	OSStatus	SetParameter(
					EventParamName		inName,
					const HIRect&		inRect );
	OSStatus	GetParameter(
					EventParamName		inName,
					HIRect*				outRect );

	OSStatus	SetParameter(
					EventParamName		inName,
					const HISize&		inSize );
	OSStatus	GetParameter(
					EventParamName		inName,
					HISize*				outSize );

	OSStatus	SetParameter(
					EventParamName		inName,
					RgnHandle			inRegion );
	OSStatus	GetParameter(
					EventParamName		inName,
					RgnHandle*			outRegion );

	OSStatus	SetParameter(
					EventParamName		inName,
					WindowRef			inWindow );
	OSStatus	GetParameter(
					EventParamName		inName,
					WindowRef*			outWindow );

	OSStatus	SetParameter(
					EventParamName		inName,
					ControlRef			inControl );
	OSStatus	GetParameter(
					EventParamName		inName,
					ControlRef* outControl );

	OSStatus	SetParameter(
					EventParamName		inName,
					MenuRef				inMenu );
	OSStatus	GetParameter(
					EventParamName		inName,
					MenuRef*			outMenu );

	OSStatus	SetParameter(
					EventParamName		inName,
					DragRef				inDrag );
	OSStatus	GetParameter(
					EventParamName		inName,
					DragRef*			outDrag );

	OSStatus	SetParameter(
					EventParamName		inName,
					UInt32				inValue );
	OSStatus	GetParameter(
					EventParamName		inName,
					UInt32*				outValue );
	
	OSStatus	SetParameter(
					EventParamName		inName,
					const HICommand&	inValue );
	OSStatus	GetParameter(
					EventParamName		inName,
					HICommand*			outValue );

	OSStatus	SetParameter(
					EventParamName		inName,
					const ControlPartCode&	inValue );
	OSStatus	GetParameter(
					EventParamName		inName,
					ControlPartCode*			outValue );

	// Template parameters
	template <class T> OSStatus SetParameter(
		EventParamName	inName,
		EventParamType	inType,
		const T&		inValue )
	{
		return SetParameter( inName, inType, sizeof( T ), &inValue );
	}
			
	template <class T> OSStatus GetParameter(
		EventParamName	inName,
		EventParamType	inType,
		T*				outValue )
	{
		return GetParameter( inName, inType, sizeof( T ), outValue );
	}
	
private:
	EventRef	fEvent;
};

#endif // TCarbonEvent_H_
