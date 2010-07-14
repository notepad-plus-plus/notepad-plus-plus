/*
 *  QuartzTextStyle.h
 *
 *  Created by Evan Jones on Wed Oct 02 2002.
 *
 */

#ifndef _QUARTZ_TEXT_STYLE_H
#define _QUARTZ_TEXT_STYLE_H

#include "QuartzTextStyleAttribute.h"

class QuartzTextStyle
{
public:
    QuartzTextStyle()
    {
        ATSUCreateStyle( &style );
    }

    ~QuartzTextStyle()
    {
        if ( style != NULL )
            ATSUDisposeStyle( style );
        style = NULL;
    }

    void setAttribute( ATSUAttributeTag tag, ByteCount size, ATSUAttributeValuePtr value )
    {
        ATSUSetAttributes( style, 1, &tag, &size, &value );
    }

    void setAttribute( QuartzTextStyleAttribute& attribute )
    {
        setAttribute( attribute.getTag(), attribute.getSize(), attribute.getValuePtr() );
    }

    void getAttribute( ATSUAttributeTag tag, ByteCount size, ATSUAttributeValuePtr value, ByteCount* actualSize )
    {
        ATSUGetAttribute( style, tag, size, value, actualSize );
    }

    template <class T>
    T getAttribute( ATSUAttributeTag tag )
    {
        T value;
        ByteCount actualSize;
        ATSUGetAttribute( style, tag, sizeof( T ), &value, &actualSize );
        return value;
    }

    // TODO: Is calling this actually faster than calling setAttribute multiple times?
    void setAttributes( QuartzTextStyleAttribute* attributes[], int number )
    {
        // Create the parallel arrays and initialize them properly
        ATSUAttributeTag* tags = new ATSUAttributeTag[ number ];
        ByteCount* sizes = new ByteCount[ number ];
        ATSUAttributeValuePtr* values = new ATSUAttributeValuePtr[ number ];

        for ( int i = 0; i < number; ++ i )
        {
            tags[i] = attributes[i]->getTag();
            sizes[i] = attributes[i]->getSize();
            values[i] = attributes[i]->getValuePtr();
        }
        
        ATSUSetAttributes( style, number, tags, sizes, values );

        // Free the arrays that were allocated
        delete[] tags;
        delete[] sizes;
        delete[] values;
    }

    void setFontFeature( ATSUFontFeatureType featureType, ATSUFontFeatureSelector selector )
    {
        ATSUSetFontFeatures( style, 1, &featureType, &selector );
    }

    const ATSUStyle& getATSUStyle() const
    {
        return style;
    }

private:
    ATSUStyle style;
};

#endif

