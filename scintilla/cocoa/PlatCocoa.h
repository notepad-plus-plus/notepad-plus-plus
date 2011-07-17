
/**
 * Copyright 2009 Sun Microsystems, Inc. All rights reserved.
 * This file is dual licensed under LGPL v2.1 and the Scintilla license (http://www.scintilla.org/License.txt).
 */

#ifndef PLATCOCOA_H
#define PLATCOCOA_H

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <assert.h>

#include <sys/time.h>

#include <Cocoa/Cocoa.h>
#include "QuartzTextLayout.h"

#include "Platform.h"
#include "Scintilla.h"

NSRect PRectangleToNSRect(Scintilla::PRectangle& rc);
Scintilla::PRectangle NSRectToPRectangle(NSRect& rc);
CFStringEncoding EncodingFromCharacterSet(bool unicode, int characterSet);

@interface ScintillaContextMenu : NSMenu
{
  Scintilla::ScintillaCocoa* owner;
}
- (void) handleCommand: (NSMenuItem*) sender;
- (void) setOwner: (Scintilla::ScintillaCocoa*) newOwner;

@end

namespace Scintilla {

// A class to do the actual text rendering for us using Quartz 2D.
class SurfaceImpl : public Surface
{
private:
  bool unicodeMode;
  float x;
  float y;

  CGContextRef gc;

  /** The text layout instance */
  QuartzTextLayout*	textLayout;
  int codePage;
  int verticalDeviceResolution;
	
  /** If the surface is a bitmap context, contains a reference to the bitmap data. */
  uint8_t* bitmapData;
  /** If the surface is a bitmap context, stores the dimensions of the bitmap. */
  int bitmapWidth;
  int bitmapHeight;

  /** Set the CGContext's fill colour to the specified allocated colour. */
  void FillColour( const ColourAllocated& back );


  // 24-bit RGB+A bitmap data constants
  static const int BITS_PER_COMPONENT = 8;
  static const int BITS_PER_PIXEL = BITS_PER_COMPONENT * 4;
  static const int BYTES_PER_PIXEL = BITS_PER_PIXEL / 8;
public:
  SurfaceImpl();
  ~SurfaceImpl();

  void Init(WindowID wid);
  void Init(SurfaceID sid, WindowID wid);
  void InitPixMap(int width, int height, Surface *surface_, WindowID wid);
  CGContextRef GetContext() { return gc; }

  void Release();
  bool Initialised();
  void PenColour(ColourAllocated fore);

  /** Returns a CGImageRef that represents the surface. Returns NULL if this is not possible. */
  CGImageRef GetImage();
  void CopyImageRectangle(Surface &surfaceSource, PRectangle srcRect, PRectangle dstRect);

  int LogPixelsY();
  int DeviceHeightFont(int points);
  void MoveTo(int x_, int y_);
  void LineTo(int x_, int y_);
  void Polygon(Scintilla::Point *pts, int npts, ColourAllocated fore, ColourAllocated back);
  void RectangleDraw(PRectangle rc, ColourAllocated fore, ColourAllocated back);
  void FillRectangle(PRectangle rc, ColourAllocated back);
  void FillRectangle(PRectangle rc, Surface &surfacePattern);
  void RoundedRectangle(PRectangle rc, ColourAllocated fore, ColourAllocated back);
  void AlphaRectangle(PRectangle rc, int cornerSize, ColourAllocated fill, int alphaFill,
                     ColourAllocated outline, int alphaOutline, int flags);
  void Ellipse(PRectangle rc, ColourAllocated fore, ColourAllocated back);
  void Copy(PRectangle rc, Scintilla::Point from, Surface &surfaceSource);
  void DrawTextNoClip(PRectangle rc, Font &font_, int ybase, const char *s, int len, ColourAllocated fore,
                     ColourAllocated back);
  void DrawTextClipped(PRectangle rc, Font &font_, int ybase, const char *s, int len, ColourAllocated fore, 
                      ColourAllocated back);
  void DrawTextTransparent(PRectangle rc, Font &font_, int ybase, const char *s, int len, ColourAllocated fore);
  void MeasureWidths(Font &font_, const char *s, int len, int *positions);
  int WidthText(Font &font_, const char *s, int len);
  int WidthChar(Font &font_, char ch);
  int Ascent(Font &font_);
  int Descent(Font &font_);
  int InternalLeading(Font &font_);
  int ExternalLeading(Font &font_);
  int Height(Font &font_);
  int AverageCharWidth(Font &font_);

  int SetPalette(Scintilla::Palette *pal, bool inBackGround);
  void SetClip(PRectangle rc);
  void FlushCachedState();

  void SetUnicodeMode(bool unicodeMode_);
  void SetDBCSMode(int codePage_);
}; // SurfaceImpl class
  
} // Scintilla namespace

#endif
