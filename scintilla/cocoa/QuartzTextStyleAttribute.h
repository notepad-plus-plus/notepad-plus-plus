/**
 *  QuartzTextStyleAttribute.h
 *
 *  Original Code by Evan Jones on Wed Oct 02 2002.
 *  Contributors:
 *  Shane Caraveo, ActiveState
 *  Bernd Paradies, Adobe
 *
 */


#ifndef _QUARTZ_TEXT_STYLE_ATTRIBUTE_H
#define _QUARTZ_TEXT_STYLE_ATTRIBUTE_H

class QuartzTextStyleAttribute
{
public:
    QuartzTextStyleAttribute() {}
    virtual ~QuartzTextStyleAttribute() {}
    virtual ByteCount getSize() const = 0;
    virtual ATSUAttributeValuePtr getValuePtr() = 0;
    virtual ATSUAttributeTag getTag() const = 0;
};

class QuartzTextSize : public QuartzTextStyleAttribute
{
public: 
    QuartzTextSize( float points )
    {
        size = X2Fix( points );
    }
    
    ByteCount getSize() const
    {
        return sizeof( size );
    }

    ATSUAttributeValuePtr getValuePtr()
    {
        return &size;
    }

    ATSUAttributeTag getTag() const
    {
        return kATSUSizeTag;
    }
    
private:
        Fixed size;
};

class QuartzTextStyleAttributeBoolean : public QuartzTextStyleAttribute
{
public:
    QuartzTextStyleAttributeBoolean( bool newVal ) : value( newVal ) {}

    ByteCount getSize() const
    {
        return sizeof( value );
    }
    ATSUAttributeValuePtr getValuePtr()
    {
        return &value;
    }
    
    virtual ATSUAttributeTag getTag() const = 0;
    
private:
        Boolean value;
};

class QuartzTextBold : public QuartzTextStyleAttributeBoolean
{
public:
    QuartzTextBold( bool newVal ) : QuartzTextStyleAttributeBoolean( newVal ) {}
    ATSUAttributeTag getTag() const
    {
        return kATSUQDBoldfaceTag;
    }
};

class QuartzTextItalic : public QuartzTextStyleAttributeBoolean
{
public:
    QuartzTextItalic( bool newVal ) : QuartzTextStyleAttributeBoolean( newVal ) {}
    ATSUAttributeTag getTag() const
    {
        return kATSUQDItalicTag;
    }
};

class QuartzTextUnderline : public QuartzTextStyleAttributeBoolean
{
public:
    QuartzTextUnderline( bool newVal ) : QuartzTextStyleAttributeBoolean( newVal ) {}
    ATSUAttributeTag getTag() const {
        return kATSUQDUnderlineTag;
    }
};

class QuartzFont : public QuartzTextStyleAttribute
{
public:
    /** Create a font style from a name. */
    QuartzFont( const char* name, int length )
    {
        assert( name != NULL && length > 0 && name[length] == '\0' );
        // try to create font
        OSStatus err = ATSUFindFontFromName( const_cast<char*>( name ), length, kFontFullName, (unsigned) kFontNoPlatform, kFontRomanScript, (unsigned) kFontNoLanguage, &fontid );

        // need a fallback if font isn't installed
        if( err != noErr || fontid == kATSUInvalidFontID )
                ::ATSUFindFontFromName( "Lucida Grande", 13, kFontFullName, (unsigned) kFontNoPlatform, kFontRomanScript, (unsigned) kFontNoLanguage, &fontid );
    }

    ByteCount getSize() const
    {
        return sizeof( fontid );
    }

    ATSUAttributeValuePtr getValuePtr()
    {
        return &fontid;
    }

    ATSUAttributeTag getTag() const
    {
        return kATSUFontTag;
    }

    ATSUFontID getFontID() const
    {
        return fontid;
    }

private:
    ATSUFontID fontid;
};


#endif

