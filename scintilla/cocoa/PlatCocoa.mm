/**
 * Scintilla source code edit control
 * PlatCocoa.mm - implementation of platform facilities on MacOS X/Cocoa
 *
 * Written by Mike Lischke
 * Based on PlatMacOSX.cxx
 * Based on work by Evan Jones (c) 2002 <ejones@uwaterloo.ca>
 * Based on PlatGTK.cxx Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
 * The License.txt file describes the conditions under which this software may be distributed.
 *
 * Copyright 2009 Sun Microsystems, Inc. All rights reserved.
 * This file is dual licensed under LGPL v2.1 and the Scintilla license (http://www.scintilla.org/License.txt).
 */

#import <ScintillaView.h>

#include "PlatCocoa.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <assert.h>
#include <sys/time.h>
#include <stdexcept>
#include <map>

#include "XPM.h"

#import <Foundation/NSGeometry.h>

#import <Carbon/Carbon.h> // Temporary

using namespace Scintilla;

extern sptr_t scintilla_send_message(void* sci, unsigned int iMessage, uptr_t wParam, sptr_t lParam);

//--------------------------------------------------------------------------------------------------

/**
 * Converts a PRectangle as used by Scintilla to standard Obj-C NSRect structure .
 */
NSRect PRectangleToNSRect(PRectangle& rc)
{
  return NSMakeRect(rc.left, rc.top, rc.Width(), rc.Height());
}

//--------------------------------------------------------------------------------------------------

/**
 * Converts an NSRect as used by the system to a native Scintilla rectangle.
 */
PRectangle NSRectToPRectangle(NSRect& rc)
{
  return PRectangle(rc.origin.x, rc.origin.y, rc.size.width + rc.origin.x, rc.size.height + rc.origin.y);
}

//--------------------------------------------------------------------------------------------------

/**
 * Converts a PRctangle as used by Scintilla to a Quartz-style rectangle.
 */
inline CGRect PRectangleToCGRect(PRectangle& rc)
{
  return CGRectMake(rc.left, rc.top, rc.Width(), rc.Height());
}

//--------------------------------------------------------------------------------------------------

/**
 * Converts a Quartz-style rectangle to a PRectangle structure as used by Scintilla.
 */
inline PRectangle CGRectToPRectangle(const CGRect& rect)
{
  PRectangle rc;
  rc.left = (int)(rect.origin.x + 0.5);
  rc.top = (int)(rect.origin.y + 0.5);
  rc.right = (int)(rect.origin.x + rect.size.width + 0.5);
  rc.bottom = (int)(rect.origin.y + rect.size.height + 0.5);
  return rc;
}

//----------------- Point --------------------------------------------------------------------------

/**
 * Converts a point given as a long into a native Point structure.
 */
Scintilla::Point Scintilla::Point::FromLong(long lpoint)
{
  return Scintilla::Point(
                          Platform::LowShortFromLong(lpoint),
                          Platform::HighShortFromLong(lpoint)
                          );
}

//----------------- Palette ------------------------------------------------------------------------

// The Palette implementation is only here because we would get linker errors if not.
// We don't use indexed colors in ScintillaCocoa.

Scintilla::Palette::Palette()
{
}

//--------------------------------------------------------------------------------------------------

Scintilla::Palette::~Palette()
{
}

//--------------------------------------------------------------------------------------------------

void Scintilla::Palette::Release()
{
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to transform a given color, if needed. If the caller tries to find a color that matches the
 * desired color then we simply pass it on, as we support the full color space.
 */
void Scintilla::Palette::WantFind(ColourPair &cp, bool want)
{
  if (!want)
    cp.allocated.Set(cp.desired.AsLong());
  
  // Don't do anything if the caller wants the color it has already set.
}

//--------------------------------------------------------------------------------------------------

void Scintilla::Palette::Allocate(Window& w)
{
  // Nothing to allocate as we don't use palettes.
}

//----------------- Font ---------------------------------------------------------------------------

Font::Font(): fid(0)
{
}

//--------------------------------------------------------------------------------------------------

Font::~Font()
{
  Release();
}

//--------------------------------------------------------------------------------------------------

/**
 * Creates a Quartz 2D font with the given properties.
 * TODO: rewrite to use NSFont.
 */
void Font::Create(const char *faceName, int characterSet, int size, bool bold, bool italic, 
                  int extraFontFlag)
{
  // TODO: How should I handle the characterSet request?
	Release();

	QuartzTextStyle* style = new QuartzTextStyle();
	fid = style;

	// Create the font with attributes
	QuartzFont font(faceName, strlen(faceName), size, bold, italic);
	CTFontRef fontRef = font.getFontID();
	style->setFontRef(fontRef);
}

//--------------------------------------------------------------------------------------------------

void Font::Release()
{
  if (fid)
    delete reinterpret_cast<QuartzTextStyle*>( fid );
  fid = 0;
}

//----------------- SurfaceImpl --------------------------------------------------------------------

SurfaceImpl::SurfaceImpl()
{
  unicodeMode = true;
  x = 0;
  y = 0;
  gc = NULL;

  textLayout = new QuartzTextLayout(NULL);
  codePage = 0;
  verticalDeviceResolution = 0;

  bitmapData = NULL; // Release will try and delete bitmapData if != NULL
  bitmapWidth = 0;
  bitmapHeight = 0;

  Release();
}

//--------------------------------------------------------------------------------------------------

SurfaceImpl::~SurfaceImpl()
{
  Release();
  delete textLayout;
}

//--------------------------------------------------------------------------------------------------

void SurfaceImpl::Release()
{
  textLayout->setContext (NULL);
  if ( bitmapData != NULL )
  {
    delete[] bitmapData;
    // We only "own" the graphics context if we are a bitmap context
    if (gc != NULL)
      CGContextRelease(gc);
  }
  bitmapData = NULL;
  gc = NULL;
  
  bitmapWidth = 0;
  bitmapHeight = 0;
  x = 0;
  y = 0;
}

//--------------------------------------------------------------------------------------------------

bool SurfaceImpl::Initialised()
{
  // We are initalised if the graphics context is not null
  return gc != NULL;// || port != NULL;
}

//--------------------------------------------------------------------------------------------------

void SurfaceImpl::Init(WindowID wid)
{
  // To be able to draw, the surface must get a CGContext handle.  We save the graphics port,
  // then aquire/release the context on an as-need basis (see above).
  // XXX Docs on QDBeginCGContext are light, a better way to do this would be good.
  // AFAIK we should not hold onto a context retrieved this way, thus the need for
  // aquire/release of the context.
  
  Release();
}

//--------------------------------------------------------------------------------------------------

void SurfaceImpl::Init(SurfaceID sid, WindowID wid)
{
  Release();
  gc = reinterpret_cast<CGContextRef>(sid);
  CGContextSetLineWidth(gc, 1.0);
  textLayout->setContext(gc);
}

//--------------------------------------------------------------------------------------------------

void SurfaceImpl::InitPixMap(int width, int height, Surface* surface_, WindowID wid)
{
  Release();
  
  // Create a new bitmap context, along with the RAM for the bitmap itself
  bitmapWidth = width;
  bitmapHeight = height;
  
  const int bitmapBytesPerRow = (width * BYTES_PER_PIXEL);
  const int bitmapByteCount = (bitmapBytesPerRow * height);
  
  // Create an RGB color space.
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  if (colorSpace == NULL)
    return;
  
  // Create the bitmap.
  bitmapData = new uint8_t[bitmapByteCount];
  if (bitmapData != NULL)
  {
    // create the context
    gc = CGBitmapContextCreate(bitmapData,
                               width,
                               height,
                               BITS_PER_COMPONENT,
                               bitmapBytesPerRow,
                               colorSpace,
                               kCGImageAlphaPremultipliedLast);
    
    if (gc == NULL)
    {
      // the context couldn't be created for some reason,
      // and we have no use for the bitmap without the context
      delete[] bitmapData;
      bitmapData = NULL;
    }
    textLayout->setContext (gc);
  }
  
  // the context retains the color space, so we can release it
  CGColorSpaceRelease(colorSpace);
  
  if (gc != NULL && bitmapData != NULL)
  {
    // "Erase" to white.
    CGContextClearRect( gc, CGRectMake( 0, 0, width, height ) );
    CGContextSetRGBFillColor( gc, 1.0, 1.0, 1.0, 1.0 );
    CGContextFillRect( gc, CGRectMake( 0, 0, width, height ) );
  }
}

//--------------------------------------------------------------------------------------------------

void SurfaceImpl::PenColour(ColourAllocated fore)
{
  if (gc)
  {
    ColourDesired colour(fore.AsLong());
    
    // Set the Stroke color to match
    CGContextSetRGBStrokeColor(gc, colour.GetRed() / 255.0, colour.GetGreen() / 255.0, 
                               colour.GetBlue() / 255.0, 1.0 );
  }
}

//--------------------------------------------------------------------------------------------------

void SurfaceImpl::FillColour(const ColourAllocated& back)
{
  if (gc)
  {
    ColourDesired colour(back.AsLong());
    
    // Set the Fill color to match
    CGContextSetRGBFillColor(gc, colour.GetRed() / 255.0, colour.GetGreen() / 255.0, 
                             colour.GetBlue() / 255.0, 1.0 );
  }
}

//--------------------------------------------------------------------------------------------------

CGImageRef SurfaceImpl::GetImage()
{
  // For now, assume that GetImage can only be called on PixMap surfaces.
  if (bitmapData == NULL)
    return NULL;
  
  CGContextFlush(gc);
  
  // Create an RGB color space.
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  if( colorSpace == NULL )
    return NULL;
  
  const int bitmapBytesPerRow = ((int) bitmapWidth * BYTES_PER_PIXEL);
  const int bitmapByteCount = (bitmapBytesPerRow * (int) bitmapHeight);
  
  // Create a data provider.
  CGDataProviderRef dataProvider = CGDataProviderCreateWithData(NULL, bitmapData, bitmapByteCount, 
                                                                NULL);
  CGImageRef image = NULL;
  if (dataProvider != NULL)
  {
    // Create the CGImage.
    image = CGImageCreate(bitmapWidth,
                          bitmapHeight,
                          BITS_PER_COMPONENT,
                          BITS_PER_PIXEL,
                          bitmapBytesPerRow,
                          colorSpace,
                          kCGImageAlphaPremultipliedLast,
                          dataProvider,
                          NULL,
                          0,
                          kCGRenderingIntentDefault);
  }
  
  // The image retains the color space, so we can release it.
  CGColorSpaceRelease(colorSpace);
  colorSpace = NULL;
  
  // Done with the data provider.
  CGDataProviderRelease(dataProvider);
  dataProvider = NULL;
  
  return image;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the vertical logical device resolution of the main monitor.
 */
int SurfaceImpl::LogPixelsY()
{
  if (verticalDeviceResolution == 0)
  {
    NSSize deviceResolution = [[[[NSScreen mainScreen] deviceDescription]
    objectForKey: NSDeviceResolution] sizeValue];
    verticalDeviceResolution = (int) deviceResolution.height;
  }
  return verticalDeviceResolution;
}

//--------------------------------------------------------------------------------------------------

/**
 * Converts the logical font height (in dpi) into a pixel height for the current main screen.
 */
int SurfaceImpl::DeviceHeightFont(int points)
{
  int logPix = LogPixelsY();
  return (points * logPix + logPix / 2) / 72;
}

//--------------------------------------------------------------------------------------------------

void SurfaceImpl::MoveTo(int x_, int y_)
{
  x = x_;
  y = y_;
}

//--------------------------------------------------------------------------------------------------

void SurfaceImpl::LineTo(int x_, int y_)
{
  CGContextBeginPath( gc );
  
  // Because Quartz is based on floating point, lines are drawn with half their colour
  // on each side of the line. Integer coordinates specify the INTERSECTION of the pixel
  // divison lines. If you specify exact pixel values, you get a line that
  // is twice as thick but half as intense. To get pixel aligned rendering,
  // we render the "middle" of the pixels by adding 0.5 to the coordinates.
  CGContextMoveToPoint( gc, x + 0.5, y + 0.5 );
  CGContextAddLineToPoint( gc, x_ + 0.5, y_ + 0.5 );
  CGContextStrokePath( gc );
  x = x_;
  y = y_;
}

//--------------------------------------------------------------------------------------------------

void SurfaceImpl::Polygon(Scintilla::Point *pts, int npts, ColourAllocated fore,
                          ColourAllocated back)
{
  // Allocate memory for the array of points.
  CGPoint *points = new CGPoint[npts];
  
  for (int i = 0;i < npts;i++)
  {
    // Quartz floating point issues: plot the MIDDLE of the pixels
    points[i].x = pts[i].x + 0.5;
    points[i].y = pts[i].y + 0.5;
  }
  
  CGContextBeginPath(gc);
  
  // Set colours
  FillColour(back);
  PenColour(fore);
  
  // Draw the polygon
  CGContextAddLines(gc, points, npts);
  
  // TODO: Should the path be automatically closed, or is that the caller's responsability?
  // Explicitly close the path, so it is closed for stroking AND filling (implicit close = filling only)
  CGContextClosePath( gc );
  CGContextDrawPath( gc, kCGPathFillStroke );
  
  // Deallocate memory.
  delete points;
  points = NULL;
}

//--------------------------------------------------------------------------------------------------

void SurfaceImpl::RectangleDraw(PRectangle rc, ColourAllocated fore, ColourAllocated back)
{
  if (gc)
  {
    CGContextBeginPath( gc );
    FillColour(back);
    PenColour(fore);
    
    // Quartz integer -> float point conversion fun (see comment in SurfaceImpl::LineTo)
    // We subtract 1 from the Width() and Height() so that all our drawing is within the area defined
    // by the PRectangle. Otherwise, we draw one pixel too far to the right and bottom.
    // TODO: Create some version of PRectangleToCGRect to do this conversion for us?
    CGContextAddRect( gc, CGRectMake( rc.left + 0.5, rc.top + 0.5, rc.Width() - 1, rc.Height() - 1 ) );
    CGContextDrawPath( gc, kCGPathFillStroke );
  }
}

//--------------------------------------------------------------------------------------------------

void SurfaceImpl::FillRectangle(PRectangle rc, ColourAllocated back)
{
  if (gc)
  {
    FillColour(back);
    CGRect rect = PRectangleToCGRect(rc);
    CGContextFillRect(gc, rect);
  }
}

//--------------------------------------------------------------------------------------------------

void drawImageRefCallback(CGImageRef pattern, CGContextRef gc)
{
  CGContextDrawImage(gc, CGRectMake(0, 0, CGImageGetWidth(pattern), CGImageGetHeight(pattern)), pattern);
}

//--------------------------------------------------------------------------------------------------

void SurfaceImpl::FillRectangle(PRectangle rc, Surface &surfacePattern)
{
  SurfaceImpl& patternSurface = static_cast<SurfaceImpl &>(surfacePattern);
  
  // For now, assume that copy can only be called on PixMap surfaces. Shows up black.
  CGImageRef image = patternSurface.GetImage();
  if (image == NULL)
  {
    FillRectangle(rc, ColourAllocated(0));
    return;
  }
  
  const CGPatternCallbacks drawImageCallbacks = { 0, 
    reinterpret_cast<CGPatternDrawPatternCallback>(drawImageRefCallback), NULL };
  
  CGPatternRef pattern = CGPatternCreate(image,
                                         CGRectMake(0, 0, patternSurface.bitmapWidth, patternSurface.bitmapHeight),
                                         CGAffineTransformIdentity,
                                         patternSurface.bitmapWidth,
                                         patternSurface.bitmapHeight,
                                         kCGPatternTilingNoDistortion,
                                         true,
                                         &drawImageCallbacks
                                        );
  if (pattern != NULL)
  {
    // Create a pattern color space
    CGColorSpaceRef colorSpace = CGColorSpaceCreatePattern( NULL );
    if( colorSpace != NULL ) {
      
      CGContextSaveGState( gc );
      CGContextSetFillColorSpace( gc, colorSpace );
      
      // Unlike the documentation, you MUST pass in a "components" parameter:
      // For coloured patterns it is the alpha value.
      const CGFloat alpha = 1.0;
      CGContextSetFillPattern( gc, pattern, &alpha );
      CGContextFillRect( gc, PRectangleToCGRect( rc ) );
      CGContextRestoreGState( gc );
      // Free the color space, the pattern and image
      CGColorSpaceRelease( colorSpace );
    } /* colorSpace != NULL */
    colorSpace = NULL;
    CGPatternRelease( pattern );
    pattern = NULL;
    CGImageRelease( image );
    image = NULL;
  } /* pattern != NULL */
}

void SurfaceImpl::RoundedRectangle(PRectangle rc, ColourAllocated fore, ColourAllocated back) {
  // TODO: Look at the Win32 API to determine what this is supposed to do:
  //  ::RoundRect(hdc, rc.left + 1, rc.top, rc.right - 1, rc.bottom, 8, 8 );
  
  // Create a rectangle with semicircles at the corners
  const int MAX_RADIUS = 4;
  int radius = Platform::Minimum( MAX_RADIUS, rc.Height()/2 );
  radius = Platform::Minimum( radius, rc.Width()/2 );
  
  // Points go clockwise, starting from just below the top left
  // Corners are kept together, so we can easily create arcs to connect them
  CGPoint corners[4][3] =
  {
    {
      { rc.left, rc.top + radius },
      { rc.left, rc.top },
      { rc.left + radius, rc.top },
    },
    {
      { rc.right - radius - 1, rc.top },
      { rc.right - 1, rc.top },
      { rc.right - 1, rc.top + radius },
    },
    {
      { rc.right - 1, rc.bottom - radius - 1 },
      { rc.right - 1, rc.bottom - 1 },
      { rc.right - radius - 1, rc.bottom - 1 },
    },
    {
      { rc.left + radius, rc.bottom - 1 },
      { rc.left, rc.bottom - 1 },
      { rc.left, rc.bottom - radius - 1 },
    },
  };
  
  // Align the points in the middle of the pixels
  // TODO: Should I include these +0.5 in the array creation code above?
  for( int i = 0; i < 4*3; ++ i )
  {
    CGPoint* c = (CGPoint*) corners;
    c[i].x += 0.5;
    c[i].y += 0.5;
  }
  
  PenColour( fore );
  FillColour( back );
  
  // Move to the last point to begin the path
  CGContextBeginPath( gc );
  CGContextMoveToPoint( gc, corners[3][2].x, corners[3][2].y );
  
  for ( int i = 0; i < 4; ++ i )
  {
    CGContextAddLineToPoint( gc, corners[i][0].x, corners[i][0].y );
    CGContextAddArcToPoint( gc, corners[i][1].x, corners[i][1].y, corners[i][2].x, corners[i][2].y, radius );
  }
  
  // Close the path to enclose it for stroking and for filling, then draw it
  CGContextClosePath( gc );
  CGContextDrawPath( gc, kCGPathFillStroke );
}

void Scintilla::SurfaceImpl::AlphaRectangle(PRectangle rc, int /*cornerSize*/, ColourAllocated fill, int alphaFill,
                                            ColourAllocated /*outline*/, int /*alphaOutline*/, int /*flags*/)
{
  if ( gc ) {
    ColourDesired colour( fill.AsLong() );
    
    // Set the Fill color to match
    CGContextSetRGBFillColor( gc, colour.GetRed() / 255.0, colour.GetGreen() / 255.0, colour.GetBlue() / 255.0, alphaFill / 255.0 );
    CGRect rect = PRectangleToCGRect( rc );
    CGContextFillRect( gc, rect );
  }
}

void SurfaceImpl::Ellipse(PRectangle rc, ColourAllocated fore, ColourAllocated back) {
  // Drawing an ellipse with bezier curves. Code modified from:
  // http://www.codeguru.com/gdi/ellipse.shtml
  // MAGICAL CONSTANT to map ellipse to beziers 2/3*(sqrt(2)-1)
  const double EToBConst = 0.2761423749154;
  
  CGSize offset = CGSizeMake((int)(rc.Width() * EToBConst), (int)(rc.Height() * EToBConst));
  CGPoint centre = CGPointMake((rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2);
  
  // The control point array
  CGPoint cCtlPt[13];
  
  // Assign values to all the control points
  cCtlPt[0].x  =
  cCtlPt[1].x  =
  cCtlPt[11].x =
  cCtlPt[12].x = rc.left + 0.5;
  cCtlPt[5].x  =
  cCtlPt[6].x  =
  cCtlPt[7].x  = rc.right - 0.5;
  cCtlPt[2].x  =
  cCtlPt[10].x = centre.x - offset.width + 0.5;
  cCtlPt[4].x  =
  cCtlPt[8].x  = centre.x + offset.width + 0.5;
  cCtlPt[3].x  =
  cCtlPt[9].x  = centre.x + 0.5;
  
  cCtlPt[2].y  =
  cCtlPt[3].y  =
  cCtlPt[4].y  = rc.top + 0.5;
  cCtlPt[8].y  =
  cCtlPt[9].y  =
  cCtlPt[10].y = rc.bottom - 0.5;
  cCtlPt[7].y  =
  cCtlPt[11].y = centre.y + offset.height + 0.5;
  cCtlPt[1].y =
  cCtlPt[5].y  = centre.y - offset.height + 0.5;
  cCtlPt[0].y =
  cCtlPt[12].y =
  cCtlPt[6].y  = centre.y + 0.5;
  
  FillColour(back);
  PenColour(fore);
  
  CGContextBeginPath( gc );
  CGContextMoveToPoint( gc, cCtlPt[0].x, cCtlPt[0].y );
  
  for ( int i = 1; i < 13; i += 3 )
  {
    CGContextAddCurveToPoint( gc, cCtlPt[i].x, cCtlPt[i].y, cCtlPt[i+1].x, cCtlPt[i+1].y, cCtlPt[i+2].x, cCtlPt[i+2].y );
  }
  
  // Close the path to enclose it for stroking and for filling, then draw it
  CGContextClosePath( gc );
  CGContextDrawPath( gc, kCGPathFillStroke );
}

void SurfaceImpl::CopyImageRectangle(Surface &surfaceSource, PRectangle srcRect, PRectangle dstRect)
{
  SurfaceImpl& source = static_cast<SurfaceImpl &>(surfaceSource);
  CGImageRef image = source.GetImage();
  
  CGRect src = PRectangleToCGRect(srcRect);
  CGRect dst = PRectangleToCGRect(dstRect);
  
  /* source from QuickDrawToQuartz2D.pdf on developer.apple.com */
  float w = (float) CGImageGetWidth(image);
  float h = (float) CGImageGetHeight(image);
  CGRect drawRect = CGRectMake (0, 0, w, h);
  if (!CGRectEqualToRect (src, dst))
  {
    float sx = CGRectGetWidth(dst) / CGRectGetWidth(src);
    float sy = CGRectGetHeight(dst) / CGRectGetHeight(src);
    float dx = CGRectGetMinX(dst) - (CGRectGetMinX(src) * sx);
    float dy = CGRectGetMinY(dst) - (CGRectGetMinY(src) * sy);
    drawRect = CGRectMake (dx, dy, w*sx, h*sy);
  }
  CGContextSaveGState (gc);
  CGContextClipToRect (gc, dst);
  CGContextDrawImage (gc, drawRect, image);
  CGContextRestoreGState (gc);
}

void SurfaceImpl::Copy(PRectangle rc, Scintilla::Point from, Surface &surfaceSource) {
  // Maybe we have to make the Surface two contexts:
  // a bitmap context which we do all the drawing on, and then a "real" context
  // which we copy the output to when we call "Synchronize". Ugh! Gross and slow!
  
  // For now, assume that copy can only be called on PixMap surfaces
  SurfaceImpl& source = static_cast<SurfaceImpl &>(surfaceSource);
  
  // Get the CGImageRef
  CGImageRef image = source.GetImage();
  // If we could not get an image reference, fill the rectangle black
  if ( image == NULL )
  {
    FillRectangle( rc, ColourAllocated( 0 ) );
    return;
  }
  
  // Now draw the image on the surface
  
  // Some fancy clipping work is required here: draw only inside of rc
  CGContextSaveGState( gc );
  CGContextClipToRect( gc, PRectangleToCGRect( rc ) );
  
  //Platform::DebugPrintf(stderr, "Copy: CGContextDrawImage: (%d, %d) - (%d X %d)\n", rc.left - from.x, rc.top - from.y, source.bitmapWidth, source.bitmapHeight );
  CGContextDrawImage( gc, CGRectMake( rc.left - from.x, rc.top - from.y, source.bitmapWidth, source.bitmapHeight ), image );
  
  // Undo the clipping fun
  CGContextRestoreGState( gc );
  
  // Done with the image
  CGImageRelease( image );
  image = NULL;
}

//--------------------------------------------------------------------------------------------------

void SurfaceImpl::DrawTextNoClip(PRectangle rc, Font &font_, int ybase, const char *s, int len,
                                 ColourAllocated fore, ColourAllocated back)
{
  FillRectangle(rc, back);
  DrawTextTransparent(rc, font_, ybase, s, len, fore);
}

//--------------------------------------------------------------------------------------------------

void SurfaceImpl::DrawTextClipped(PRectangle rc, Font &font_, int ybase, const char *s, int len,
                                  ColourAllocated fore, ColourAllocated back)
{
  CGContextSaveGState(gc);
  CGContextClipToRect(gc, PRectangleToCGRect(rc));
  DrawTextNoClip(rc, font_, ybase, s, len, fore, back);
  CGContextRestoreGState(gc);
}

//--------------------------------------------------------------------------------------------------

CFStringEncoding EncodingFromCharacterSet(bool unicode, int characterSet)
{
  if (unicode)
    return kCFStringEncodingUTF8;

  // Unsupported -> Latin1 as reasonably safe
  enum { notSupported = kCFStringEncodingISOLatin1};

  switch (characterSet)
  {
  case SC_CHARSET_ANSI:
    return kCFStringEncodingISOLatin1;
  case SC_CHARSET_DEFAULT:
    return kCFStringEncodingISOLatin1;
  case SC_CHARSET_BALTIC:
    return kCFStringEncodingWindowsBalticRim;
  case SC_CHARSET_CHINESEBIG5:
    return kCFStringEncodingBig5;
  case SC_CHARSET_EASTEUROPE:
    return kCFStringEncodingWindowsLatin2;
  case SC_CHARSET_GB2312:
    return kCFStringEncodingGB_18030_2000;
  case SC_CHARSET_GREEK:
    return kCFStringEncodingWindowsGreek;
  case SC_CHARSET_HANGUL:
    return kCFStringEncodingEUC_KR;
  case SC_CHARSET_MAC:
    return kCFStringEncodingMacRoman;
  case SC_CHARSET_OEM:
    return kCFStringEncodingISOLatin1;
  case SC_CHARSET_RUSSIAN:
    return kCFStringEncodingKOI8_R;
  case SC_CHARSET_CYRILLIC:
    return kCFStringEncodingWindowsCyrillic;
  case SC_CHARSET_SHIFTJIS:
    return kCFStringEncodingShiftJIS;
  case SC_CHARSET_SYMBOL:
    return kCFStringEncodingMacSymbol;
  case SC_CHARSET_TURKISH:
    return kCFStringEncodingWindowsLatin5;
  case SC_CHARSET_JOHAB:
    return kCFStringEncodingWindowsKoreanJohab;
  case SC_CHARSET_HEBREW:
    return kCFStringEncodingWindowsHebrew;
  case SC_CHARSET_ARABIC:
    return kCFStringEncodingWindowsArabic;
  case SC_CHARSET_VIETNAMESE:
    return kCFStringEncodingWindowsVietnamese;
  case SC_CHARSET_THAI:
    return kCFStringEncodingISOLatinThai;
  case SC_CHARSET_8859_15:
    return kCFStringEncodingISOLatin1;
  default:
    return notSupported;
  }
}

void SurfaceImpl::DrawTextTransparent(PRectangle rc, Font &font_, int ybase, const char *s, int len, 
                                      ColourAllocated fore)
{
	ColourDesired colour(fore.AsLong());
	CGColorRef color = CGColorCreateGenericRGB(colour.GetRed()/255.0,colour.GetGreen()/255.0,colour.GetBlue()/255.0,1.0);

	QuartzTextStyle* style = reinterpret_cast<QuartzTextStyle*>(font_.GetID());
	style->setCTStyleColor(color);

	textLayout->setText (reinterpret_cast<const UInt8*>(s), len, *reinterpret_cast<QuartzTextStyle*>(font_.GetID()));
	textLayout->draw(rc.left, ybase);
}

//--------------------------------------------------------------------------------------------------

void SurfaceImpl::MeasureWidths(Font &font_, const char *s, int len, int *positions)
{
	for (int i = 0; i < len; i++)
		positions [i] = 0;
	textLayout->setText (reinterpret_cast<const UInt8*>(s), len, *reinterpret_cast<QuartzTextStyle*>(font_.GetID()));
	
	CTLineRef mLine = textLayout->getCTLine();
	assert(mLine != NULL);
	
	CGFloat* secondaryOffset= 0;
	int unicodeCharStart = 0;
	for ( int  i = 0; i < len+1; i ++ ){
		unsigned char uch = s[i];
		CFIndex charIndex = unicodeCharStart+1;
		CGFloat advance = CTLineGetOffsetForStringIndex(mLine, charIndex, secondaryOffset);
		
		if ( unicodeMode ) 
		{
			unsigned char mask = 0xc0;
			int lcount = 1;
			// Add one additonal byte for each extra high order one in the byte
			while ( uch >= mask && lcount < 8 ) 
			{
				positions[i++] = (int)(advance+0.5);
				lcount ++;
				mask = mask >> 1 | 0x80; // add an additional one in the highest order position
			}
		}
		positions[i] = (int)(advance+0.5);
		unicodeCharStart++;
	}
}

int SurfaceImpl::WidthText(Font &font_, const char *s, int len) {
  if (font_.GetID())
  {
    textLayout->setText (reinterpret_cast<const UInt8*>(s), len, *reinterpret_cast<QuartzTextStyle*>(font_.GetID()));
    
	return textLayout->MeasureStringWidth();
  }
  return 1;
}

int SurfaceImpl::WidthChar(Font &font_, char ch) {
  char str[2] = { ch, '\0' };
  if (font_.GetID())
  {
    textLayout->setText (reinterpret_cast<const UInt8*>(str), 1, *reinterpret_cast<QuartzTextStyle*>(font_.GetID()));
    
    return textLayout->MeasureStringWidth();
  }
  else
    return 1;
}

// This string contains a good range of characters to test for size.
const char sizeString[] = "`~!@#$%^&*()-_=+\\|[]{};:\"\'<,>.?/1234567890"
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

int SurfaceImpl::Ascent(Font &font_) {
  if (!font_.GetID())
    return 1;
  
	float ascent = reinterpret_cast<QuartzTextStyle*>( font_.GetID() )->getAscent();
	return ascent + 0.5;

}

int SurfaceImpl::Descent(Font &font_) {
  if (!font_.GetID())
    return 1;
  
	float descent = reinterpret_cast<QuartzTextStyle*>( font_.GetID() )->getDescent();
	return descent + 0.5;

}

int SurfaceImpl::InternalLeading(Font &) {
  // TODO: How do we get EM_Size?
  // internal leading = ascent - descent - EM_size
  return 0;
}

int SurfaceImpl::ExternalLeading(Font &font_) {
  if (!font_.GetID())
    return 1;
  
	float leading = reinterpret_cast<QuartzTextStyle*>( font_.GetID() )->getLeading();
	return leading + 0.5;

}

int SurfaceImpl::Height(Font &font_) {

	int ht = Ascent(font_) + Descent(font_);
	return ht;
}

int SurfaceImpl::AverageCharWidth(Font &font_) {
  
  if (!font_.GetID())
    return 1;
  
  const int sizeStringLength = (sizeof( sizeString ) / sizeof( sizeString[0] ) - 1);
  int width = WidthText( font_, sizeString, sizeStringLength  );
  
  return (int) ((width / (float) sizeStringLength) + 0.5);
}

int SurfaceImpl::SetPalette(Scintilla::Palette *, bool) {
  // Mac OS X is always true colour (I think) so this doesn't matter
  return 0;
}

void SurfaceImpl::SetClip(PRectangle rc) {
  CGContextClipToRect( gc, PRectangleToCGRect( rc ) );
}

void SurfaceImpl::FlushCachedState() {
  CGContextSynchronize( gc );
}

void SurfaceImpl::SetUnicodeMode(bool unicodeMode_) {
  unicodeMode = unicodeMode_;
}

void SurfaceImpl::SetDBCSMode(int codePage_) {
  if (codePage_ && (codePage_ != SC_CP_UTF8))
    codePage = codePage_;
}

Surface *Surface::Allocate()
{
  return new SurfaceImpl();
}

//----------------- Window -------------------------------------------------------------------------

// Cocoa uses different types for windows and views, so a Window may
// be either an NSWindow or NSView and the code will check the type
// before performing an action.

Window::~Window()
{
}

//--------------------------------------------------------------------------------------------------

void Window::Destroy()
{
  if (wid)
  {
    id idWin = reinterpret_cast<id>(wid);
    if ([idWin isKindOfClass: [NSWindow class]])
    {
      NSWindow* win = reinterpret_cast<NSWindow*>(idWin);
      [win release];
    }
  }
  wid = 0;
}

//--------------------------------------------------------------------------------------------------

bool Window::HasFocus()
{
  NSView* container = reinterpret_cast<NSView*>(wid);
  return [[container window] firstResponder] == container;
}

//--------------------------------------------------------------------------------------------------

static int ScreenMax(NSWindow* win)
{
  NSScreen* screen = [win screen];
  NSRect frame = [screen frame];
  return frame.origin.y + frame.size.height;
}

PRectangle Window::GetPosition()
{
  if (wid)
  {
    NSRect rect;
    id idWin = reinterpret_cast<id>(wid);
    NSWindow* win;
    if ([idWin isKindOfClass: [NSView class]])
    {
      // NSView
      NSView* view = reinterpret_cast<NSView*>(idWin);
      win = [view window];
      rect = [view bounds];
      rect = [view convertRectToBase: rect];
      rect.origin = [win convertBaseToScreen:rect.origin];
    }
    else
    {
      // NSWindow
      win = reinterpret_cast<NSWindow*>(idWin);
      rect = [win frame];
    }
    int screenHeight = ScreenMax(win);
    // Invert screen positions to match Scintilla
    return PRectangle(
        NSMinX(rect), screenHeight - NSMaxY(rect),
        NSMaxX(rect), screenHeight - NSMinY(rect));
  }
  else
  {
    return PRectangle(0, 0, 1, 1);
  }
}

//--------------------------------------------------------------------------------------------------

void Window::SetPosition(PRectangle rc)
{
  if (wid)
  {
    id idWin = reinterpret_cast<id>(wid);
    if ([idWin isKindOfClass: [NSView class]])
    {
      // NSView
      // Moves this view inside the parent view
      NSRect nsrc = NSMakeRect(rc.left, rc.bottom, rc.Width(), rc.Height());
      NSView* view = reinterpret_cast<NSView*>(idWin);
      nsrc.origin = [[view window] convertScreenToBase:nsrc.origin];
      [view setFrame: nsrc];
    }
    else
    {
      // NSWindow
      NSWindow* win = reinterpret_cast<NSWindow*>(idWin);
      int screenHeight = ScreenMax(win);
      NSRect nsrc = NSMakeRect(rc.left, screenHeight - rc.bottom,
          rc.Width(), rc.Height());
      [win setFrame: nsrc display:YES];
    }
  }
}

//--------------------------------------------------------------------------------------------------

void Window::SetPositionRelative(PRectangle rc, Window window)
{
  PRectangle rcOther = window.GetPosition();
  rc.left += rcOther.left;
  rc.right += rcOther.left;
  rc.top += rcOther.top;
  rc.bottom += rcOther.top;
  SetPosition(rc);
}

//--------------------------------------------------------------------------------------------------

PRectangle Window::GetClientPosition()
{
  // This means, in MacOS X terms, get the "frame bounds". Call GetPosition, just like on Win32.
  return GetPosition();
}

//--------------------------------------------------------------------------------------------------

void Window::Show(bool show)
{
  if (wid)
  {
    id idWin = reinterpret_cast<id>(wid);
    if ([idWin isKindOfClass: [NSWindow class]])
    {
      NSWindow* win = reinterpret_cast<NSWindow*>(idWin);
      if (show)
      {
        [win orderFront:nil];
      }
      else
      {
        [win orderOut:nil];
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Invalidates the entire window or view so it is completely redrawn.
 */
void Window::InvalidateAll()
{
  if (wid)
  {
    id idWin = reinterpret_cast<id>(wid);
    NSView* container;
    if ([idWin isKindOfClass: [NSView class]])
    {
      container = reinterpret_cast<NSView*>(idWin);
    }
    else
    {
      // NSWindow
      NSWindow* win = reinterpret_cast<NSWindow*>(idWin);
      container = reinterpret_cast<NSView*>([win contentView]);
      container.needsDisplay = YES;
    }
    container.needsDisplay = YES;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Invalidates part of the window or view so only this part redrawn.
 */
void Window::InvalidateRectangle(PRectangle rc)
{
  if (wid)
  {
    id idWin = reinterpret_cast<id>(wid);
    NSView* container;
    if ([idWin isKindOfClass: [NSView class]])
    {
      container = reinterpret_cast<NSView*>(idWin);
    }
    else
    {
      // NSWindow
      NSWindow* win = reinterpret_cast<NSWindow*>(idWin);
      container = reinterpret_cast<NSView*>([win contentView]);
    }
    [container setNeedsDisplayInRect: PRectangleToNSRect(rc)];
  }
}

//--------------------------------------------------------------------------------------------------

void Window::SetFont(Font&)
{
  // Implemented on list subclass on Cocoa.
}

//--------------------------------------------------------------------------------------------------

/**
 * Converts the Scintilla cursor enum into an NSCursor and stores it in the associated NSView,
 * which then will take care to set up a new mouse tracking rectangle.
 */
void Window::SetCursor(Cursor curs)
{
  if (wid)
  {
    id idWin = reinterpret_cast<id>(wid);
    if ([idWin isMemberOfClass: [InnerView class]])
    {
      InnerView* container = reinterpret_cast<InnerView*>(idWin);
      [container setCursor: curs];
    }
  }
}

//--------------------------------------------------------------------------------------------------

void Window::SetTitle(const char* s)
{
  if (wid)
  {
    id idWin = reinterpret_cast<id>(wid);
    if ([idWin isKindOfClass: [NSWindow class]])
    {
      NSWindow* win = reinterpret_cast<NSWindow*>(idWin);
      NSString* sTitle = [NSString stringWithUTF8String:s];
      [win setTitle:sTitle];
    }
  }
}

//--------------------------------------------------------------------------------------------------

PRectangle Window::GetMonitorRect(Point)
{
  if (wid)
  {
    id idWin = reinterpret_cast<id>(wid);
    if ([idWin isKindOfClass: [NSWindow class]])
    {
      NSWindow* win = reinterpret_cast<NSWindow*>(idWin);
      NSScreen* screen = [win screen];
      NSRect rect = [screen frame];
      int screenHeight = rect.origin.y + rect.size.height;
      // Invert screen positions to match Scintilla
      return PRectangle(
          NSMinX(rect), screenHeight - NSMaxY(rect),
          NSMaxX(rect), screenHeight - NSMinY(rect));
    }
  }
  return PRectangle();
}

//----------------- ImageFromXPM -------------------------------------------------------------------

// Convert an XPM image into an NSImage for use with Cocoa

static NSImage* ImageFromXPM(XPM* pxpm)
{
  NSImage* img = nil;
  if (pxpm)
  {
    const int width = pxpm->GetWidth();
    const int height = pxpm->GetHeight();
    PRectangle rcxpm(0, 0, width, height);
    Surface* surfaceXPM = Surface::Allocate();
    if (surfaceXPM)
    {
      surfaceXPM->InitPixMap(width, height, NULL, NULL);
      SurfaceImpl* surfaceIXPM = static_cast<SurfaceImpl*>(surfaceXPM);
      CGContextClearRect(surfaceIXPM->GetContext(), CGRectMake(0, 0, width, height));
      pxpm->Draw(surfaceXPM, rcxpm);
      img = [NSImage alloc];
      [img autorelease];
      CGImageRef imageRef = surfaceIXPM->GetImage();
      [img initWithCGImage:imageRef size:NSZeroSize];
      CGImageRelease(imageRef);
      delete surfaceXPM;
    }
  }
  return img;
}

//----------------- ListBox ------------------------------------------------------------------------

ListBox::ListBox()
{
}

//--------------------------------------------------------------------------------------------------

ListBox::~ListBox()
{
}

//--------------------------------------------------------------------------------------------------

struct RowData
{
  int type;
  std::string text;
  RowData(int type_, const char* text_) :
    type(type_), text(text_)
  {
  }
};

class LinesData
{
  std::vector<RowData> lines;
public:
  LinesData()
  {
  }
  ~LinesData()
  {
  }
  int Length() const
  {
    return lines.size();
  }
  void Clear()
  {
    lines.clear();
  }
  void Add(int index, int type, char* str)
  {
    lines.push_back(RowData(type, str));
  }
  int GetType(int index) const
  {
    if (index < lines.size())
    {
      return lines[index].type;
    }
    else
    {
      return 0;
    }
  }
  const char* GetString(int index) const
  {
    if (index < lines.size())
    {
      return lines[index].text.c_str();
    }
    else
    {
      return 0;
    }
  }
};

class ListBoxImpl;

@interface AutoCompletionDataSource :
NSObject <NSTableViewDataSource>
{
  ListBoxImpl* box;
}

@end

//----------------- ListBoxImpl --------------------------------------------------------------------

// Map from icon type to an NSImage*
typedef std::map<int, NSImage*> ImageMap;

class ListBoxImpl : public ListBox
{
private:
  ControlRef lb;
  ImageMap images;
  int lineHeight;
  bool unicodeMode;
  int desiredVisibleRows;
  unsigned int maxItemWidth;
  unsigned int aveCharWidth;
  unsigned int maxIconWidth;
  Font font;
  int maxWidth;

  NSTableView* table;
  NSScrollView* scroller;
  NSTableColumn* colIcon;
  NSTableColumn* colText;

  LinesData ld;
  CallBackAction doubleClickAction;
  void* doubleClickActionData;

public:
  ListBoxImpl() : lb(NULL), lineHeight(10), unicodeMode(false),
    desiredVisibleRows(5), maxItemWidth(0), aveCharWidth(8), maxIconWidth(0),
    doubleClickAction(NULL), doubleClickActionData(NULL)
  {
  }
  ~ListBoxImpl() {};

  // ListBox methods
  void SetFont(Font& font);
  void Create(Window& parent, int ctrlID, Scintilla::Point pt, int lineHeight_, bool unicodeMode_);
  void SetAverageCharWidth(int width);
  void SetVisibleRows(int rows);
  int GetVisibleRows() const;
  PRectangle GetDesiredRect();
  int CaretFromEdge();
  void Clear();
  void Append(char* s, int type = -1);
  int Length();
  void Select(int n);
  int GetSelection();
  int Find(const char* prefix);
  void GetValue(int n, char* value, int len);
  void RegisterImage(int type, const char* xpm_data);
  void ClearRegisteredImages();
  void SetDoubleClickAction(CallBackAction action, void* data)
  {
    doubleClickAction = action;
    doubleClickActionData = data;
  }
  void SetList(const char* list, char separator, char typesep);

  // For access from AutoCompletionDataSource
  int Rows();
  NSImage* ImageForRow(int row);
  NSString* TextForRow(int row);
  void DoubleClick();
};

@implementation AutoCompletionDataSource

- (void)setBox: (ListBoxImpl*)box_
{
  box = box_;
}

- (void) doubleClick: (id) sender
{
  if (box)
  {
    box->DoubleClick();
  }
}

- (id)tableView: (NSTableView*)aTableView objectValueForTableColumn: (NSTableColumn*)aTableColumn row: (NSInteger)rowIndex
{
#pragma unused(aTableView)
  if (!box)
    return nil;
  if ([(NSString*)[aTableColumn identifier] isEqualToString: @"icon"])
  {
    return box->ImageForRow(rowIndex);
  }
  else {
    return box->TextForRow(rowIndex);
  }
}

- (void)tableView: (NSTableView*)aTableView setObjectValue: anObject forTableColumn: (NSTableColumn*)aTableColumn row: (NSInteger)rowIndex
{
#pragma unused(aTableView)
#pragma unused(anObject)
#pragma unused(aTableColumn)
#pragma unused(rowIndex)
}

- (NSInteger)numberOfRowsInTableView: (NSTableView*)aTableView
{
#pragma unused(aTableView)
  if (!box)
    return 0;
  return box->Rows();
}

@end

ListBox* ListBox::Allocate()
{
  ListBoxImpl* lb = new ListBoxImpl();
  return lb;
}

void ListBoxImpl::Create(Window& /*parent*/, int /*ctrlID*/, Scintilla::Point pt,
    int lineHeight_, bool unicodeMode_)
{
  lineHeight = lineHeight_;
  unicodeMode = unicodeMode_;
  maxWidth = 2000;

  NSRect lbRect = NSMakeRect(pt.x,pt.y, 120, lineHeight * desiredVisibleRows);
  NSWindow* winLB = [[NSWindow alloc] initWithContentRect: lbRect
    styleMask: NSBorderlessWindowMask
    backing: NSBackingStoreBuffered
    defer: NO];
  [winLB setLevel:NSFloatingWindowLevel];
  [winLB setHasShadow:YES];
  scroller = [NSScrollView alloc];
  NSRect scRect = NSMakeRect(0, 0, lbRect.size.width, lbRect.size.height);
  [scroller initWithFrame: scRect];
  [scroller setHasVerticalScroller:YES];
  table = [NSTableView alloc];
  [table initWithFrame: scRect];
  [table setHeaderView:nil];
  [scroller setDocumentView: table];
  colIcon = [[NSTableColumn alloc] initWithIdentifier:@"icon"];
  [colIcon setWidth: 20];
  [colIcon setEditable:NO];
  [colIcon setHidden:YES];
  NSImageCell* imCell = [[NSImageCell alloc] init];
  [colIcon setDataCell:imCell];
  [table addTableColumn:colIcon];
  colText = [[NSTableColumn alloc] initWithIdentifier:@"name"];
  [colText setResizingMask:NSTableColumnAutoresizingMask];
  [colText setEditable:NO];
  [table addTableColumn:colText];
  AutoCompletionDataSource* ds = [[AutoCompletionDataSource alloc] init];
  [ds setBox:this];
  [table setDataSource: ds];
  [scroller setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];
  [[winLB contentView] addSubview: scroller];

  [table setTarget:ds];
  [table setDoubleAction:@selector(doubleClick:)];
  wid = winLB;
}

void ListBoxImpl::SetFont(Font& font_)
{
  font.SetID(font_.GetID());
  // NSCell setFont takes an NSFont* rather than a CTFontRef but they
  // are the same thing toll-free bridged.
  QuartzTextStyle* style = reinterpret_cast<QuartzTextStyle*>(font_.GetID());
  NSFont *pfont = (NSFont *)style->getFontRef();
  [[colText dataCell] setFont: pfont];
  CGFloat itemHeight = lround([pfont ascender] - [pfont descender]);
  [table setRowHeight:itemHeight];
}

void ListBoxImpl::SetAverageCharWidth(int width)
{
  aveCharWidth = width;
}

void ListBoxImpl::SetVisibleRows(int rows)
{
  desiredVisibleRows = rows;
}

int ListBoxImpl::GetVisibleRows() const
{
  return desiredVisibleRows;
}

PRectangle ListBoxImpl::GetDesiredRect()
{
  PRectangle rcDesired;
  rcDesired = GetPosition();

  // There appears to be an extra pixel above and below the row contents
  int itemHeight = [table rowHeight] + 2;

  int rows = Length();
  if ((rows == 0) || (rows > desiredVisibleRows))
    rows = desiredVisibleRows;

  rcDesired.bottom = rcDesired.top + itemHeight * rows;
  rcDesired.right = rcDesired.left + maxItemWidth + aveCharWidth;

  if (Length() > rows)
  {
    [scroller setHasVerticalScroller:YES];
    rcDesired.right += [NSScroller scrollerWidth];
  }
  else
  {
    [scroller setHasVerticalScroller:NO];
  }
  rcDesired.right += maxIconWidth;
  rcDesired.right += 6;

  return rcDesired;
}

int ListBoxImpl::CaretFromEdge()
{
  if ([colIcon isHidden])
    return 3;
  else
    return 6 + [colIcon width];
}

void ListBoxImpl::Clear()
{
  maxItemWidth = 0;
  maxIconWidth = 0;
  ld.Clear();
}

void ListBoxImpl::Append(char* s, int type)
{
  int count = Length();
  ld.Add(count, type, s);

  Scintilla::SurfaceImpl surface;
  unsigned int width = surface.WidthText(font, s, strlen(s));
  if (width > maxItemWidth)
  {
    maxItemWidth = width;
    [colText setWidth: maxItemWidth];
  }
  ImageMap::iterator it = images.find(type);
  if (it != images.end())
  {
    NSImage* img = it->second;
    if (img)
    {
      unsigned int widthIcon = img.size.width;
      if (widthIcon > maxIconWidth)
      {
        [colIcon setHidden: NO];
        maxIconWidth = widthIcon;
        [colIcon setWidth: maxIconWidth];
      }
    }
  }
}

void ListBoxImpl::SetList(const char* list, char separator, char typesep)
{
  Clear();
  int count = strlen(list) + 1;
  char* words = new char[count];
  if (words)
  {
    memcpy(words, list, count);
    char* startword = words;
    char* numword = NULL;
    int i = 0;
    for (; words[i]; i++)
    {
      if (words[i] == separator)
      {
        words[i] = '\0';
        if (numword)
          *numword = '\0';
        Append(startword, numword?atoi(numword + 1):-1);
        startword = words + i + 1;
        numword = NULL;
      }
      else if (words[i] == typesep)
      {
        numword = words + i;
      }
    }
    if (startword)
    {
      if (numword)
        *numword = '\0';
      Append(startword, numword?atoi(numword + 1):-1);
    }
    delete []words;
  }
  [table reloadData];
}

int ListBoxImpl::Length()
{
  return ld.Length();
}

void ListBoxImpl::Select(int n)
{
  [table selectRowIndexes:[NSIndexSet indexSetWithIndex:n] byExtendingSelection:NO];
  [table scrollRowToVisible:n];
}

int ListBoxImpl::GetSelection()
{
  return [table selectedRow];
}

int ListBoxImpl::Find(const char* prefix)
{
  int count = Length();
  for (int i = 0; i < count; i++)
  {
    const char* s = ld.GetString(i);
    if (s && (s[0] != '\0') && (0 == strncmp(prefix, s, strlen(prefix))))
    {
      return i;
    }
  }
  return - 1;
}

void ListBoxImpl::GetValue(int n, char* value, int len)
{
  const char* textString = ld.GetString(n);
  if (textString == NULL)
  {
    value[0] = '\0';
    return;
  }
  strncpy(value, textString, len);
  value[len - 1] = '\0';
}

void ListBoxImpl::RegisterImage(int type, const char* xpm_data)
{
  XPM xpm(xpm_data);
  xpm.CopyDesiredColours();
  NSImage* img = ImageFromXPM(&xpm);
  [img retain];
  ImageMap::iterator it=images.find(type);
  if (it == images.end())
  {
    images[type] = img;
  }
  else
  {
    [it->second release];
    it->second = img;
  }
}

void ListBoxImpl::ClearRegisteredImages()
{
  for (ImageMap::iterator it=images.begin();
      it != images.end(); ++it)
  {
    [it->second release];
    it->second = nil;
  }
  images.clear();
}

int ListBoxImpl::Rows()
{
  return ld.Length();
}

NSImage* ListBoxImpl::ImageForRow(int row)
{
  ImageMap::iterator it = images.find(ld.GetType(row));
  if (it != images.end())
  {
    NSImage* img = it->second;
    [img retain];
    return img;
  }
  else
  {
    return nil;
  }
}

NSString* ListBoxImpl::TextForRow(int row)
{
  const char* textString = ld.GetString(row);
  NSString* sTitle;
  if (unicodeMode)
    sTitle = [NSString stringWithUTF8String:textString];
  else
    sTitle = [NSString stringWithCString:textString encoding:NSWindowsCP1252StringEncoding];
  return sTitle;
}

void ListBoxImpl::DoubleClick()
{
  if (doubleClickAction)
  {
    doubleClickAction(doubleClickActionData);
  }
}

//----------------- ScintillaContextMenu -----------------------------------------------------------

@implementation ScintillaContextMenu : NSMenu

// This NSMenu subclass serves also as target for menu commands and forwards them as
// notfication messages to the front end.

- (void) handleCommand: (NSMenuItem*) sender
{
  owner->HandleCommand([sender tag]);
}

//--------------------------------------------------------------------------------------------------

- (void) setOwner: (Scintilla::ScintillaCocoa*) newOwner
{
  owner = newOwner;
}

@end

//----------------- Menu ---------------------------------------------------------------------------

Menu::Menu()
  : mid(0)
{
}

//--------------------------------------------------------------------------------------------------

void Menu::CreatePopUp()
{
  Destroy();
  mid = [[ScintillaContextMenu alloc] initWithTitle: @""];
}

//--------------------------------------------------------------------------------------------------

void Menu::Destroy()
{
  ScintillaContextMenu* menu = reinterpret_cast<ScintillaContextMenu*>(mid);
  [menu release];
  mid = NULL;
}

//--------------------------------------------------------------------------------------------------

void Menu::Show(Point pt, Window &)
{
  // Cocoa menus are handled a bit differently. We only create the menu. The framework
  // takes care to show it properly.
}

//----------------- ElapsedTime --------------------------------------------------------------------

// TODO: Consider if I should be using GetCurrentEventTime instead of gettimeoday
ElapsedTime::ElapsedTime() {
  struct timeval curTime;
  int retVal;
  retVal = gettimeofday( &curTime, NULL );
  
  bigBit = curTime.tv_sec;
  littleBit = curTime.tv_usec;
}

double ElapsedTime::Duration(bool reset) {
  struct timeval curTime;
  int retVal;
  retVal = gettimeofday( &curTime, NULL );
  long endBigBit = curTime.tv_sec;
  long endLittleBit = curTime.tv_usec;
  double result = 1000000.0 * (endBigBit - bigBit);
  result += endLittleBit - littleBit;
  result /= 1000000.0;
  if (reset) {
    bigBit = endBigBit;
    littleBit = endLittleBit;
  }
  return result;
}

//----------------- Platform -----------------------------------------------------------------------

ColourDesired Platform::Chrome()
{
  return ColourDesired(0xE0, 0xE0, 0xE0);
}

//--------------------------------------------------------------------------------------------------

ColourDesired Platform::ChromeHighlight()
{
  return ColourDesired(0xFF, 0xFF, 0xFF);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the currently set system font for the user.
 */
const char *Platform::DefaultFont()
{
  NSString* name = [[NSUserDefaults standardUserDefaults] stringForKey: @"NSFixedPitchFont"];
  return [name UTF8String];
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the currently set system font size for the user.
 */
int Platform::DefaultFontSize()
{
  return [[NSUserDefaults standardUserDefaults] integerForKey: @"NSFixedPitchFontSize"];
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the time span in which two consequtive mouse clicks must occur to be considered as
 * double click.
 *
 * @return
 */
unsigned int Platform::DoubleClickTime()
{
  float threshold = [[NSUserDefaults standardUserDefaults] floatForKey: 
                     @"com.apple.mouse.doubleClickThreshold"];
  if (threshold == 0)
    threshold = 0.5;
  return static_cast<unsigned int>(threshold / kEventDurationMillisecond);
}

//--------------------------------------------------------------------------------------------------

bool Platform::MouseButtonBounce()
{
  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper method for the backend to reach through to the scintiall window.
 */
long Platform::SendScintilla(WindowID w, unsigned int msg, unsigned long wParam, long lParam) 
{
  return scintilla_send_message(w, msg, wParam, lParam);
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper method for the backend to reach through to the scintiall window.
 */
long Platform::SendScintillaPointer(WindowID w, unsigned int msg, unsigned long wParam, void *lParam)
{
  return scintilla_send_message(w, msg, wParam, (long) lParam);
}

//--------------------------------------------------------------------------------------------------

bool Platform::IsDBCSLeadByte(int codePage, char ch)
{
  // Byte ranges found in Wikipedia articles with relevant search strings in each case
  unsigned char uch = static_cast<unsigned char>(ch);
  switch (codePage)
  {
  case 932:
    // Shift_jis
    return ((uch >= 0x81) && (uch <= 0x9F)) ||
        ((uch >= 0xE0) && (uch <= 0xEF));
  case 936:
    // GBK
    return (uch >= 0x81) && (uch <= 0xFE);
  case 949:
    // Korean Wansung KS C-5601-1987
    return (uch >= 0x81) && (uch <= 0xFE);
  case 950:
    // Big5
    return (uch >= 0x81) && (uch <= 0xFE);
  case 1361:
    // Korean Johab KS C-5601-1992
    return
      ((uch >= 0x84) && (uch <= 0xD3)) ||
      ((uch >= 0xD8) && (uch <= 0xDE)) ||
      ((uch >= 0xE0) && (uch <= 0xF9));
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

int Platform::DBCSCharLength(int codePage, const char* s)
{
  // No support for DBCS.
  return 1;
}

//--------------------------------------------------------------------------------------------------

int Platform::DBCSCharMaxLength()
{
  // No support for DBCS.
  return 2;
}

//--------------------------------------------------------------------------------------------------

int Platform::Minimum(int a, int b)
{
  return (a < b) ? a : b;
}

//--------------------------------------------------------------------------------------------------

int Platform::Maximum(int a, int b) {
  return (a > b) ? a : b;
}

//--------------------------------------------------------------------------------------------------

//#define TRACE
#ifdef TRACE

void Platform::DebugDisplay(const char *s)
{
  fprintf( stderr, s );
}

//--------------------------------------------------------------------------------------------------

void Platform::DebugPrintf(const char *format, ...)
{
  const int BUF_SIZE = 2000;
  char buffer[BUF_SIZE];
  
  va_list pArguments;
  va_start(pArguments, format);
  vsnprintf(buffer, BUF_SIZE, format, pArguments);
  va_end(pArguments);
  Platform::DebugDisplay(buffer);
}

#else

void Platform::DebugDisplay(const char *) {}

void Platform::DebugPrintf(const char *, ...) {}

#endif

//--------------------------------------------------------------------------------------------------

static bool assertionPopUps = true;

bool Platform::ShowAssertionPopUps(bool assertionPopUps_)
{
  bool ret = assertionPopUps;
  assertionPopUps = assertionPopUps_;
  return ret;
}

//--------------------------------------------------------------------------------------------------

void Platform::Assert(const char *c, const char *file, int line)
{
  char buffer[2000];
  sprintf(buffer, "Assertion [%s] failed at %s %d", c, file, line);
  strcat(buffer, "\r\n");
  Platform::DebugDisplay(buffer);
#ifdef DEBUG 
  // Jump into debugger in assert on Mac (CL269835)
  ::Debugger();
#endif
}

//--------------------------------------------------------------------------------------------------

int Platform::Clamp(int val, int minVal, int maxVal)
{
  if (val > maxVal)
    val = maxVal;
  if (val < minVal)
    val = minVal;
  return val;
}

//----------------- DynamicLibrary -----------------------------------------------------------------

/**
 * Implements the platform specific part of library loading.
 * 
 * @param modulePath The path to the module to load.
 * @return A library instance or NULL if the module could not be found or another problem occured.
 */
DynamicLibrary* DynamicLibrary::Load(const char* modulePath)
{
  return NULL;
}

//--------------------------------------------------------------------------------------------------

