/*
 *  QuartzTextLayout.h
 *
 *  Original Code by Evan Jones on Wed Oct 02 2002.
 *  Contributors:
 *  Shane Caraveo, ActiveState
 *  Bernd Paradies, Adobe
 *
 */

#ifndef _QUARTZ_TEXT_LAYOUT_H
#define _QUARTZ_TEXT_LAYOUT_H

#include <Carbon/Carbon.h>

#include "QuartzTextStyle.h"

class QuartzTextLayout
{
public:
    /** Create a text layout for drawing on the specified context. */
    QuartzTextLayout( CGContextRef context ) : layout( NULL ), unicode_string( NULL ), unicode_length( 0 )
    {
        OSStatus err = ATSUCreateTextLayout( &layout );
        if (0 != err)
                layout = NULL;

        setContext(context);

        ATSUAttributeTag tag = kATSULineLayoutOptionsTag;
        ByteCount size = sizeof( ATSLineLayoutOptions );
        ATSLineLayoutOptions rendering = kATSLineUseDeviceMetrics; //| kATSLineFractDisable | kATSLineUseQDRendering
        ATSUAttributeValuePtr valuePtr = &rendering;
        err = ATSUSetLayoutControls( layout, 1, &tag, &size, &valuePtr );
    }

    ~QuartzTextLayout()
    {
        if (NULL != layout)
            ATSUDisposeTextLayout( layout );
        layout = NULL;

        if ( unicode_string != NULL )
        {
            delete[] unicode_string;
            unicode_string = NULL;
            unicode_length = 0;
        }
    }

    /** Assign a string to the text layout object. */
    // TODO: Create a UTF8 version
    // TODO: Optimise the ASCII version by not copying so much
    OSStatus setText( const UInt8* buffer, size_t byteLength, CFStringEncoding encoding )
    {
        if (NULL == layout)
                return -1;
        CFStringRef str = CFStringCreateWithBytes( NULL, buffer, byteLength, encoding, false );
        if (!str)
            return -1;

        unicode_length = CFStringGetLength( str );
        if (unicode_string)
            delete[] unicode_string;
        unicode_string = new UniChar[ unicode_length ];
        CFStringGetCharacters( str, CFRangeMake( 0, unicode_length ), unicode_string );

        CFRelease( str );
        str = NULL;

        OSStatus err;
        err = ATSUSetTextPointerLocation( layout, unicode_string, kATSUFromTextBeginning, kATSUToTextEnd, unicode_length );
        if( err != noErr ) return err;

        // Turn on the default font fallbacks
        return ATSUSetTransientFontMatching( layout, true );
    }

    inline void setText( const UInt8* buffer, size_t byteLength, const QuartzTextStyle& r )
    {
        this->setText( buffer, byteLength, kCFStringEncodingUTF8 );
        ATSUSetRunStyle( layout, r.getATSUStyle(), 0, unicode_length );
    }

    /** Apply the specified text style on the entire range of text. */
    void setStyle( const QuartzTextStyle& style )
    {
        ATSUSetRunStyle( layout, style.getATSUStyle(), kATSUFromTextBeginning, kATSUToTextEnd );
    }

    /** Draw the text layout into the current CGContext at the specified position, flipping the CGContext's Y axis if required.
    * @param x The x axis position to draw the baseline in the current CGContext.
    * @param y The y axis position to draw the baseline in the current CGContext.
    * @param flipTextYAxis If true, the CGContext's Y axis will be flipped before drawing the text, and restored afterwards. Use this when drawing in an HIView's CGContext, where the origin is the top left corner. */
    void draw( float x, float y, bool flipTextYAxis = false )
    {
        if (NULL == layout || 0 == unicode_length)
                return;
        if ( flipTextYAxis )
        {
            CGContextSaveGState( gc );
            CGContextScaleCTM( gc, 1.0, -1.0 );
            y = -y;
        }
        
        OSStatus err;
        err = ATSUDrawText( layout, kATSUFromTextBeginning, kATSUToTextEnd, X2Fix( x ), X2Fix( y ) );

        if ( flipTextYAxis ) CGContextRestoreGState( gc );
    }

    /** Sets a single text layout control on the ATSUTextLayout object.
    * @param tag The control to set.
    * @param size The size of the parameter pointed to by value.
    * @param value A pointer to the new value for the control.
    */
    void setControl( ATSUAttributeTag tag, ByteCount size, ATSUAttributeValuePtr value )
    {
        ATSUSetLayoutControls( layout, 1, &tag, &size, &value );
    }

    ATSUTextLayout getLayout() {
        return layout;
    }

    inline CFIndex getLength() const { return unicode_length; }
    inline void setContext (CGContextRef context)
    {
        gc = context;
        if (NULL != layout)
            setControl( kATSUCGContextTag, sizeof( gc ), &gc );
    }

private:
    ATSUTextLayout layout;
    UniChar* unicode_string;
    int unicode_length;
    CGContextRef gc;
};

#endif
