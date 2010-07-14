/*
    File:		TRect.h
    
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

#ifndef TRect_H_
#define TRect_H_

#include <Carbon/Carbon.h>

class TRect
	: public HIRect
{
public:
	// Construction/Destruction
	TRect();
	TRect(
						const HIRect*		inRect );
	TRect(
						const HIRect&		inRect );
	TRect(
						const HIPoint&		inOrigin,
						const HISize&		inSize );
	TRect(
						float				inX,
						float				inY,
						float				inWidth,
						float				inHeight );
	TRect(
						const Rect&			inRect );
	~TRect();
	
	// Operators
	operator		HIRect*()
						{ return this; }
	operator		Rect() const;

	// Accessors
	float			MinX() const
						{ return CGRectGetMinX( *this ); }
	float			MaxX() const
						{ return CGRectGetMaxX( *this ); }
	float			MinY() const
						{ return CGRectGetMinY( *this ); }
	float			MaxY() const
						{ return CGRectGetMaxY( *this ); }

	float			Width() const
						{ return CGRectGetWidth( *this ); }
	float			Height() const
						{ return CGRectGetHeight( *this ); }
	
	const HIPoint&	Origin() const
						{ return origin; }
	const HISize&	Size() const
						{ return size; }

	float			CenterX() const
						{ return CGRectGetMidX( *this ); }
	float			CenterY() const
						{ return CGRectGetMidY( *this ); }
	HIPoint			Center() const;
	
	// Modifiers
	const HIRect&	Inset(
						float				inX,
						float				inY );
	const HIRect&	Outset(
						float				inX,
						float				inY );
	const HIRect&	MoveBy(
						float				inDx,
						float				inDy );
	const HIRect&	MoveTo(
						float				inX,
						float				inY );

	const HIRect&	Set(
						const HIRect*		inRect );
	const HIRect&	Set(
						const HIRect&		inRect );
	const HIRect&	Set(				
						float				inX,
						float				inY,
						float				inWidth,
						float				inHeight );
	const HIRect&	Set(
						const Rect*			inRect );

	const HIRect&	SetAroundCenter(				
						float				inCenterX,
						float				inCenterY,
						float				inWidth,
						float				inHeight );

	const HIRect&	SetWidth(
						float				inWidth );
	const HIRect&	SetHeight(
						float				inHeight );

	const HIRect&	SetOrigin(
						const HIPoint&		inOrigin );
	const HIRect&	SetOrigin(
						float				inX,
						float				inY );
	const HIRect&	SetSize(
						const HISize&		inSize );
	const HIRect&	SetSize(
						float				inWidth,
						float				inHeight );

	// Tests
	bool			Contains(
						const HIPoint&		inPoint );
	bool			Contains(
						const HIRect&		inRect );
	bool			Contains(
						const Point&		inPoint );
	bool			Contains(
						const Rect&			inRect );
};

//-----------------------------------------------------------------------------------
//	TRect constructor
//-----------------------------------------------------------------------------------
//
inline TRect::TRect()
{
}

//-----------------------------------------------------------------------------------
//	TRect constructor
//-----------------------------------------------------------------------------------
//
inline TRect::TRect(
	const HIRect*		inRect )
{
	*this = *inRect;
}

//-----------------------------------------------------------------------------------
//	TRect constructor
//-----------------------------------------------------------------------------------
//
inline TRect::TRect(
	const HIRect&		inRect )
{
	origin = inRect.origin;
	size = inRect.size;
}

//-----------------------------------------------------------------------------------
//	TRect constructor
//-----------------------------------------------------------------------------------
//
inline TRect::TRect(
	const HIPoint&		inOrigin,
	const HISize&		inSize )
{
	origin = inOrigin;
	size = inSize;
}

//-----------------------------------------------------------------------------------
//	TRect constructor
//-----------------------------------------------------------------------------------
//
inline TRect::TRect(
	float				inX,
	float				inY,
	float				inWidth,
	float				inHeight )
{
	*this = CGRectMake( inX, inY, inWidth, inHeight );
}

//-----------------------------------------------------------------------------------
//	TRect destructor
//-----------------------------------------------------------------------------------
//
inline TRect::~TRect()
{
}

//-----------------------------------------------------------------------------------
//	TRect constructor
//-----------------------------------------------------------------------------------
//
inline TRect::TRect(
	const Rect&		inRect )
{
	Set( &inRect );
}

//-----------------------------------------------------------------------------------
//	Rect operator
//-----------------------------------------------------------------------------------
//	Converts the HIRect to a QD rect and returns it
//
inline TRect::operator Rect() const
{
	Rect	qdRect;
	
	qdRect.top = (SInt16) MinY();
	qdRect.left = (SInt16) MinX();
	qdRect.bottom = (SInt16) MaxY();
	qdRect.right = (SInt16) MaxX();

	return qdRect;
}

//-----------------------------------------------------------------------------------
//	Center
//-----------------------------------------------------------------------------------
//
inline HIPoint TRect::Center() const
{
	return CGPointMake( CGRectGetMidX( *this ), CGRectGetMidY( *this ) );
}

//-----------------------------------------------------------------------------------
//	Inset
//-----------------------------------------------------------------------------------
//
inline const HIRect& TRect::Inset(
	float				inX,
	float				inY )
{
	*this = CGRectInset( *this, inX, inY );
	
	return *this;
}

//-----------------------------------------------------------------------------------
//	Outset
//-----------------------------------------------------------------------------------
//
inline const HIRect& TRect::Outset(
	float				inX,
	float				inY )
{
	*this = CGRectInset( *this, -inX, -inY );
	
	return *this;
}

//-----------------------------------------------------------------------------------
//	MoveBy
//-----------------------------------------------------------------------------------
//
inline const HIRect& TRect::MoveBy(
	float				inDx,
	float				inDy )
{
	origin = CGPointMake( MinX() + inDx, MinY() + inDy );
	
	return *this;
}

//-----------------------------------------------------------------------------------
//	MoveTo
//-----------------------------------------------------------------------------------
//
inline const HIRect& TRect::MoveTo(
	float				inX,
	float				inY )
{
	origin = CGPointMake( inX, inY );
	
	return *this;
}

//-----------------------------------------------------------------------------------
//	Set
//-----------------------------------------------------------------------------------
//
inline const HIRect& TRect::Set(
	const HIRect*		inRect )
{
	*this = *inRect;
	
	return *this;
}

//-----------------------------------------------------------------------------------
//	Set
//-----------------------------------------------------------------------------------
//
inline const HIRect& TRect::Set(
	const HIRect&			inRect )
{
	*this = inRect;
	
	return *this;
}

//-----------------------------------------------------------------------------------
//	Set
//-----------------------------------------------------------------------------------
//
inline const HIRect& TRect::Set(
	float				inX,
	float				inY,
	float				inWidth,
	float				inHeight )
{
	*this = CGRectMake( inX, inY, inWidth, inHeight );
	
	return *this;
}

//-----------------------------------------------------------------------------------
//	Set
//-----------------------------------------------------------------------------------
//
inline const HIRect& TRect::Set(
	const Rect*			inRect )
{
	origin.x = inRect->left;
	origin.y = inRect->top;
	size.width = inRect->right - inRect->left;
	size.height = inRect->bottom - inRect->top;
	
	return *this;
}

//-----------------------------------------------------------------------------------
//	SetAroundCenter
//-----------------------------------------------------------------------------------
//	Sets the rectangle by specifying dimensions around a center point
//
inline const HIRect& TRect::SetAroundCenter(
	float				inCenterX,
	float				inCenterY,
	float				inWidth,
	float				inHeight )
{
	*this = CGRectMake( inCenterX - inWidth/2, inCenterY - inHeight/2, inWidth, inHeight );
	
	return *this;
}

//-----------------------------------------------------------------------------------
//	SetWidth
//-----------------------------------------------------------------------------------
//
inline const HIRect& TRect::SetWidth(
	float				inWidth )
{
	size.width = inWidth;
	
	return *this;
}

//-----------------------------------------------------------------------------------
//	SetHeight
//-----------------------------------------------------------------------------------
//
inline const HIRect& TRect::SetHeight(
	float				inHeight )
{
	size.height = inHeight;
	
	return *this;
}

//-----------------------------------------------------------------------------------
//	SetOrigin
//-----------------------------------------------------------------------------------
//
inline const HIRect& TRect::SetOrigin(
	const HIPoint&		inOrigin )
{
	origin = inOrigin;
	
	return *this;
}

//-----------------------------------------------------------------------------------
//	SetOrigin
//-----------------------------------------------------------------------------------
//
inline const HIRect& TRect::SetOrigin(
	float				inX,
	float				inY )
{
	origin = CGPointMake( inX, inY );
	
	return *this;
}

//-----------------------------------------------------------------------------------
//	SetSize
//-----------------------------------------------------------------------------------
//
inline const HIRect& TRect::SetSize(
	const HISize&		inSize )
{
	size = inSize;
	
	return *this;
}

//-----------------------------------------------------------------------------------
//	SetSize
//-----------------------------------------------------------------------------------
//
inline const HIRect& TRect::SetSize(
	float				inWidth,
	float				inHeight )
{
	size = CGSizeMake( inWidth, inHeight );
	
	return *this;
}

//-----------------------------------------------------------------------------------
//	Contains
//-----------------------------------------------------------------------------------
//
inline bool TRect::Contains(
	const HIPoint&		inPoint )
{
	return CGRectContainsPoint( *this, inPoint );
}

//-----------------------------------------------------------------------------------
//	Contains
//-----------------------------------------------------------------------------------
//
inline bool TRect::Contains(
	const HIRect&		inRect )
{
	return CGRectContainsRect( *this, inRect );
}

//-----------------------------------------------------------------------------------
//	Contains
//-----------------------------------------------------------------------------------
//
inline bool TRect::Contains(
	const Point&		inPoint )
{
	return Contains( CGPointMake( inPoint.h, inPoint.v ) );
}

//-----------------------------------------------------------------------------------
//	Contains
//-----------------------------------------------------------------------------------
//
inline bool TRect::Contains(
	const Rect&		inRect )
{
	return Contains( CGRectMake( inRect.left, inRect.top,
			inRect.right - inRect.left, inRect.bottom - inRect.top ) );
}

#endif // TRect_H_
