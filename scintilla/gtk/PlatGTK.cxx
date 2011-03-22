// Scintilla source code edit control
// PlatGTK.cxx - implementation of platform facilities on GTK+/Linux
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>

#include <glib.h>
#include <gmodule.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "Platform.h"

#include "Scintilla.h"
#include "ScintillaWidget.h"
#include "UniConversion.h"
#include "XPM.h"

/* GLIB must be compiled with thread support, otherwise we
   will bail on trying to use locks, and that could lead to
   problems for someone.  `glib-config --libs gthread` needs
   to be used to get the glib libraries for linking, otherwise
   g_thread_init will fail */
#define USE_LOCK defined(G_THREADS_ENABLED) && !defined(G_THREADS_IMPL_NONE)
/* Use fast way of getting char data on win32 to work around problems
   with gdk_string_extents. */
#define FAST_WAY

#include "Converter.h"

#if GTK_CHECK_VERSION(2,20,0)
#define IS_WIDGET_FOCUSSED(w) (gtk_widget_has_focus(GTK_WIDGET(w)))
#else
#define IS_WIDGET_FOCUSSED(w) (GTK_WIDGET_HAS_FOCUS(w))
#endif

#if GTK_CHECK_VERSION(2,22,0)
#define USE_CAIRO 1
#endif

#ifdef USE_CAIRO
#define DISABLE_GDK_FONT 1
#endif

#ifdef _MSC_VER
// Ignore unreferenced local functions in GTK+ headers
#pragma warning(disable: 4505)
#endif

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

enum encodingType { singleByte, UTF8, dbcs};

struct LOGFONT {
	int size;
	bool bold;
	bool italic;
	int characterSet;
	char faceName[300];
};

#if USE_LOCK
static GMutex *fontMutex = NULL;

static void InitializeGLIBThreads() {
	if (!g_thread_supported()) {
		g_thread_init(NULL);
	}
}
#endif

static void FontMutexAllocate() {
#if USE_LOCK
	if (!fontMutex) {
		InitializeGLIBThreads();
		fontMutex = g_mutex_new();
	}
#endif
}

static void FontMutexFree() {
#if USE_LOCK
	if (fontMutex) {
		g_mutex_free(fontMutex);
		fontMutex = NULL;
	}
#endif
}

static void FontMutexLock() {
#if USE_LOCK
	g_mutex_lock(fontMutex);
#endif
}

static void FontMutexUnlock() {
#if USE_LOCK
	if (fontMutex) {
		g_mutex_unlock(fontMutex);
	}
#endif
}

// On GTK+ 1.x holds a GdkFont* but on GTK+ 2.x can hold a GdkFont* or a
// PangoFontDescription*.
class FontHandle {
	int width[128];
	encodingType et;
public:
	int ascent;
	GdkFont *pfont;
	PangoFontDescription *pfd;
	int characterSet;
	FontHandle(GdkFont *pfont_) {
		et = singleByte;
		ascent = 0;
		pfont = pfont_;
		pfd = 0;
		characterSet = -1;
		ResetWidths(et);
	}
	FontHandle(PangoFontDescription *pfd_, int characterSet_) {
		et = singleByte;
		ascent = 0;
		pfont = 0;
		pfd = pfd_;
		characterSet = characterSet_;
		ResetWidths(et);
	}
	~FontHandle() {
#ifndef DISABLE_GDK_FONT
		if (pfont)
			gdk_font_unref(pfont);
#endif
		pfont = 0;
		if (pfd)
			pango_font_description_free(pfd);
		pfd = 0;
	}
	void ResetWidths(encodingType et_) {
		et = et_;
		for (int i=0; i<=127; i++) {
			width[i] = 0;
		}
	}
	int CharWidth(unsigned char ch, encodingType et_) {
		int w = 0;
		FontMutexLock();
		if ((ch <= 127) && (et == et_)) {
			w = width[ch];
		}
		FontMutexUnlock();
		return w;
	}
	void SetCharWidth(unsigned char ch, int w, encodingType et_) {
		if (ch <= 127) {
			FontMutexLock();
			if (et != et_) {
				ResetWidths(et_);
			}
			width[ch] = w;
			FontMutexUnlock();
		}
	}
};

// X has a 16 bit coordinate space, so stop drawing here to avoid wrapping
static const int maxCoordinate = 32000;

static FontHandle *PFont(Font &f) {
	return reinterpret_cast<FontHandle *>(f.GetID());
}

static GtkWidget *PWidget(WindowID wid) {
	return reinterpret_cast<GtkWidget *>(wid);
}

static GtkWidget *PWidget(Window &w) {
	return PWidget(w.GetID());
}

Point Point::FromLong(long lpoint) {
	return Point(
	           Platform::LowShortFromLong(lpoint),
	           Platform::HighShortFromLong(lpoint));
}

Palette::Palette() {
	used = 0;
	allowRealization = false;
	allocatedPalette = 0;
	allocatedLen = 0;
	size = 100;
	entries = new ColourPair[size];
}

Palette::~Palette() {
	Release();
	delete []entries;
	entries = 0;
}

void Palette::Release() {
	used = 0;
	delete [](reinterpret_cast<GdkColor *>(allocatedPalette));
	allocatedPalette = 0;
	allocatedLen = 0;
	delete []entries;
	size = 100;
	entries = new ColourPair[size];
}

// This method either adds a colour to the list of wanted colours (want==true)
// or retrieves the allocated colour back to the ColourPair.
// This is one method to make it easier to keep the code for wanting and retrieving in sync.
void Palette::WantFind(ColourPair &cp, bool want) {
	if (want) {
		for (int i=0; i < used; i++) {
			if (entries[i].desired == cp.desired)
				return;
		}

		if (used >= size) {
			int sizeNew = size * 2;
			ColourPair *entriesNew = new ColourPair[sizeNew];
			for (int j=0; j<size; j++) {
				entriesNew[j] = entries[j];
			}
			delete []entries;
			entries = entriesNew;
			size = sizeNew;
		}

		entries[used].desired = cp.desired;
		entries[used].allocated.Set(cp.desired.AsLong());
		used++;
	} else {
		for (int i=0; i < used; i++) {
			if (entries[i].desired == cp.desired) {
				cp.allocated = entries[i].allocated;
				return;
			}
		}
		cp.allocated.Set(cp.desired.AsLong());
	}
}

void Palette::Allocate(Window &w) {
	if (allocatedPalette) {
		gdk_colormap_free_colors(gtk_widget_get_colormap(PWidget(w)),
		                         reinterpret_cast<GdkColor *>(allocatedPalette),
		                         allocatedLen);
		delete [](reinterpret_cast<GdkColor *>(allocatedPalette));
		allocatedPalette = 0;
		allocatedLen = 0;
	}
	GdkColor *paletteNew = new GdkColor[used];
	allocatedPalette = paletteNew;
	gboolean *successPalette = new gboolean[used];
	if (paletteNew) {
		allocatedLen = used;
		int iPal = 0;
		for (iPal = 0; iPal < used; iPal++) {
			paletteNew[iPal].red = entries[iPal].desired.GetRed() * (65535 / 255);
			paletteNew[iPal].green = entries[iPal].desired.GetGreen() * (65535 / 255);
			paletteNew[iPal].blue = entries[iPal].desired.GetBlue() * (65535 / 255);
			paletteNew[iPal].pixel = entries[iPal].desired.AsLong();
		}
		gdk_colormap_alloc_colors(gtk_widget_get_colormap(PWidget(w)),
		                          paletteNew, allocatedLen, FALSE, TRUE,
		                          successPalette);
		for (iPal = 0; iPal < used; iPal++) {
			entries[iPal].allocated.Set(paletteNew[iPal].pixel);
		}
	}
	delete []successPalette;
}

#ifndef DISABLE_GDK_FONT

static const char *CharacterSetName(int characterSet) {
	switch (characterSet) {
	case SC_CHARSET_ANSI:
		return "iso8859-*";
	case SC_CHARSET_DEFAULT:
		return "iso8859-*";
	case SC_CHARSET_BALTIC:
		return "iso8859-13";
	case SC_CHARSET_CHINESEBIG5:
		return "*-*";
	case SC_CHARSET_EASTEUROPE:
		return "*-2";
	case SC_CHARSET_GB2312:
		return "gb2312.1980-*";
	case SC_CHARSET_GREEK:
		return "*-7";
	case SC_CHARSET_HANGUL:
		return "ksc5601.1987-*";
	case SC_CHARSET_MAC:
		return "*-*";
	case SC_CHARSET_OEM:
		return "*-*";
	case SC_CHARSET_RUSSIAN:
		return "*-r";
	case SC_CHARSET_CYRILLIC:
		return "*-cp1251";
	case SC_CHARSET_SHIFTJIS:
		return "jisx0208.1983-*";
	case SC_CHARSET_SYMBOL:
		return "*-*";
	case SC_CHARSET_TURKISH:
		return "*-9";
	case SC_CHARSET_JOHAB:
		return "*-*";
	case SC_CHARSET_HEBREW:
		return "*-8";
	case SC_CHARSET_ARABIC:
		return "*-6";
	case SC_CHARSET_VIETNAMESE:
		return "*-*";
	case SC_CHARSET_THAI:
		return "iso8859-11";
	case SC_CHARSET_8859_15:
		return "iso8859-15";
	default:
		return "*-*";
	}
}

static bool IsDBCSCharacterSet(int characterSet) {
	switch (characterSet) {
	case SC_CHARSET_GB2312:
	case SC_CHARSET_HANGUL:
	case SC_CHARSET_SHIFTJIS:
	case SC_CHARSET_CHINESEBIG5:
		return true;
	default:
		return false;
	}
}

static void GenerateFontSpecStrings(const char *fontName, int characterSet,
                                    char *foundary, int foundary_len,
                                    char *faceName, int faceName_len,
                                    char *charset, int charset_len) {
	// supported font strings include:
	// foundary-fontface-isoxxx-x
	// fontface-isoxxx-x
	// foundary-fontface
	// fontface
	if (strchr(fontName, '-')) {
		char tmp[300];
		char *d1 = NULL, *d2 = NULL, *d3 = NULL;
		strncpy(tmp, fontName, sizeof(tmp) - 1);
		tmp[sizeof(tmp) - 1] = '\0';
		d1 = strchr(tmp, '-');
		// we know the first dash exists
		d2 = strchr(d1 + 1, '-');
		if (d2)
			d3 = strchr(d2 + 1, '-');
		if (d3 && d2) {
			// foundary-fontface-isoxxx-x
			*d2 = '\0';
			foundary[0] = '-';
			foundary[1] = '\0';
			strncpy(faceName, tmp, foundary_len - 1);
			strncpy(charset, d2 + 1, charset_len - 1);
		} else if (d2) {
			// fontface-isoxxx-x
			*d1 = '\0';
			strcpy(foundary, "-*-");
			strncpy(faceName, tmp, faceName_len - 1);
			strncpy(charset, d1 + 1, charset_len - 1);
		} else {
			// foundary-fontface
			foundary[0] = '-';
			foundary[1] = '\0';
			strncpy(faceName, tmp, faceName_len - 1);
			strncpy(charset, CharacterSetName(characterSet), charset_len - 1);
		}
	} else {
		strncpy(foundary, "-*-", foundary_len);
		strncpy(faceName, fontName, faceName_len - 1);
		strncpy(charset, CharacterSetName(characterSet), charset_len - 1);
	}
}

#endif

static void SetLogFont(LOGFONT &lf, const char *faceName, int characterSet, int size, bool bold, bool italic) {
	memset(&lf, 0, sizeof(lf));
	lf.size = size;
	lf.bold = bold;
	lf.italic = italic;
	lf.characterSet = characterSet;
	strncpy(lf.faceName, faceName, sizeof(lf.faceName) - 1);
}

/**
 * Create a hash from the parameters for a font to allow easy checking for identity.
 * If one font is the same as another, its hash will be the same, but if the hash is the
 * same then they may still be different.
 */
static int HashFont(const char *faceName, int characterSet, int size, bool bold, bool italic) {
	return
	    size ^
	    (characterSet << 10) ^
	    (bold ? 0x10000000 : 0) ^
	    (italic ? 0x20000000 : 0) ^
	    faceName[0];
}

class FontCached : Font {
	FontCached *next;
	int usage;
	LOGFONT lf;
	int hash;
	FontCached(const char *faceName_, int characterSet_, int size_, bool bold_, bool italic_);
	~FontCached() {}
	bool SameAs(const char *faceName_, int characterSet_, int size_, bool bold_, bool italic_);
	virtual void Release();
	static FontID CreateNewFont(const char *fontName, int characterSet,
	                            int size, bool bold, bool italic);
	static FontCached *first;
public:
	static FontID FindOrCreate(const char *faceName_, int characterSet_, int size_, bool bold_, bool italic_);
	static void ReleaseId(FontID fid_);
};

FontCached *FontCached::first = 0;

FontCached::FontCached(const char *faceName_, int characterSet_, int size_, bool bold_, bool italic_) :
next(0), usage(0), hash(0) {
	::SetLogFont(lf, faceName_, characterSet_, size_, bold_, italic_);
	hash = HashFont(faceName_, characterSet_, size_, bold_, italic_);
	fid = CreateNewFont(faceName_, characterSet_, size_, bold_, italic_);
	usage = 1;
}

bool FontCached::SameAs(const char *faceName_, int characterSet_, int size_, bool bold_, bool italic_) {
	return
	    lf.size == size_ &&
	    lf.bold == bold_ &&
	    lf.italic == italic_ &&
	    lf.characterSet == characterSet_ &&
	    0 == strcmp(lf.faceName, faceName_);
}

void FontCached::Release() {
	if (fid)
		delete PFont(*this);
	fid = 0;
}

FontID FontCached::FindOrCreate(const char *faceName_, int characterSet_, int size_, bool bold_, bool italic_) {
	FontID ret = 0;
	FontMutexLock();
	int hashFind = HashFont(faceName_, characterSet_, size_, bold_, italic_);
	for (FontCached *cur = first; cur; cur = cur->next) {
		if ((cur->hash == hashFind) &&
		        cur->SameAs(faceName_, characterSet_, size_, bold_, italic_)) {
			cur->usage++;
			ret = cur->fid;
		}
	}
	if (ret == 0) {
		FontCached *fc = new FontCached(faceName_, characterSet_, size_, bold_, italic_);
		if (fc) {
			fc->next = first;
			first = fc;
			ret = fc->fid;
		}
	}
	FontMutexUnlock();
	return ret;
}

void FontCached::ReleaseId(FontID fid_) {
	FontMutexLock();
	FontCached **pcur = &first;
	for (FontCached *cur = first; cur; cur = cur->next) {
		if (cur->fid == fid_) {
			cur->usage--;
			if (cur->usage == 0) {
				*pcur = cur->next;
				cur->Release();
				cur->next = 0;
				delete cur;
			}
			break;
		}
		pcur = &cur->next;
	}
	FontMutexUnlock();
}

#ifndef DISABLE_GDK_FONT
static GdkFont *LoadFontOrSet(const char *fontspec, int characterSet) {
	if (IsDBCSCharacterSet(characterSet)) {
		return gdk_fontset_load(fontspec);
	} else {
		return gdk_font_load(fontspec);
	}
}
#endif

FontID FontCached::CreateNewFont(const char *fontName, int characterSet,
                                 int size, bool bold, bool italic) {
	if (fontName[0] == '!') {
		PangoFontDescription *pfd = pango_font_description_new();
		if (pfd) {
			pango_font_description_set_family(pfd, fontName+1);
			pango_font_description_set_size(pfd, size * PANGO_SCALE);
			pango_font_description_set_weight(pfd, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
			pango_font_description_set_style(pfd, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
			return new FontHandle(pfd, characterSet);
		}
	}

#ifndef DISABLE_GDK_FONT
	char fontset[1024];
	char fontspec[300];
	char foundary[50];
	char faceName[100];
	char charset[50];
	fontset[0] = '\0';
	fontspec[0] = '\0';
	foundary[0] = '\0';
	faceName[0] = '\0';
	charset[0] = '\0';

	GdkFont *newid = 0;
	// If name of the font begins with a '-', assume, that it is
	// a full fontspec.
	if (fontName[0] == '-') {
		if (strchr(fontName, ',') || IsDBCSCharacterSet(characterSet)) {
			newid = gdk_fontset_load(fontName);
		} else {
			newid = gdk_font_load(fontName);
		}
		if (!newid) {
			// Font not available so substitute a reasonable code font
			// iso8859 appears to only allow western characters.
			newid = LoadFontOrSet("-*-*-*-*-*-*-*-*-*-*-*-*-iso8859-*",
				characterSet);
		}
		return new FontHandle(newid);
	}

	// it's not a full fontspec, build one.

	// This supports creating a FONT_SET
	// in a method that allows us to also set size, slant and
	// weight for the fontset.  The expected input is multiple
	// partial fontspecs seperated by comma
	// eg. adobe-courier-iso10646-1,*-courier-iso10646-1,*-*-*-*
	if (strchr(fontName, ',')) {
		// build a fontspec and use gdk_fontset_load
		int remaining = sizeof(fontset);
		char fontNameCopy[1024];
		strncpy(fontNameCopy, fontName, sizeof(fontNameCopy) - 1);
		char *fn = fontNameCopy;
		char *fp = strchr(fn, ',');
		for (;;) {
			const char *spec = "%s%s%s%s-*-*-*-%0d-*-*-*-*-%s";
			if (fontset[0] != '\0') {
				// if this is not the first font in the list,
				// append a comma seperator
				spec = ",%s%s%s%s-*-*-*-%0d-*-*-*-*-%s";
			}

			if (fp)
				*fp = '\0'; // nullify the comma
			GenerateFontSpecStrings(fn, characterSet,
			                        foundary, sizeof(foundary),
			                        faceName, sizeof(faceName),
			                        charset, sizeof(charset));

			g_snprintf(fontspec,
			         sizeof(fontspec) - 1,
			         spec,
			         foundary, faceName,
			         bold ? "-bold" : "-medium",
			         italic ? "-i" : "-r",
			         size * 10,
			         charset);

			// if this is the first font in the list, and
			// we are doing italic, add an oblique font
			// to the list
			if (italic && fontset[0] == '\0') {
				strncat(fontset, fontspec, remaining - 1);
				remaining -= strlen(fontset);

				g_snprintf(fontspec,
				         sizeof(fontspec) - 1,
				         ",%s%s%s-o-*-*-*-%0d-*-*-*-*-%s",
				         foundary, faceName,
				         bold ? "-bold" : "-medium",
				         size * 10,
				         charset);
			}

			strncat(fontset, fontspec, remaining - 1);
			remaining -= strlen(fontset);

			if (!fp)
				break;

			fn = fp + 1;
			fp = strchr(fn, ',');
		}

		newid = gdk_fontset_load(fontset);
		if (newid)
			return new FontHandle(newid);
		// if fontset load failed, fall through, we'll use
		// the last font entry and continue to try and
		// get something that matches
	}

	// single fontspec support

	GenerateFontSpecStrings(fontName, characterSet,
	                        foundary, sizeof(foundary),
	                        faceName, sizeof(faceName),
	                        charset, sizeof(charset));

	g_snprintf(fontspec,
	         sizeof(fontspec) - 1,
	         "%s%s%s%s-*-*-*-%0d-*-*-*-*-%s",
	         foundary, faceName,
	         bold ? "-bold" : "-medium",
	         italic ? "-i" : "-r",
	         size * 10,
	         charset);
	newid = LoadFontOrSet(fontspec, characterSet);
	if (!newid) {
		// some fonts have oblique, not italic
		g_snprintf(fontspec,
		         sizeof(fontspec) - 1,
		         "%s%s%s%s-*-*-*-%0d-*-*-*-*-%s",
		         foundary, faceName,
		         bold ? "-bold" : "-medium",
		         italic ? "-o" : "-r",
		         size * 10,
		         charset);
		newid = LoadFontOrSet(fontspec, characterSet);
	}
	if (!newid) {
		g_snprintf(fontspec,
		         sizeof(fontspec) - 1,
		         "-*-*-*-*-*-*-*-%0d-*-*-*-*-%s",
		         size * 10,
		         charset);
		newid = gdk_font_load(fontspec);
	}
	if (!newid) {
		// Font not available so substitute a reasonable code font
		// iso8859 appears to only allow western characters.
		newid = LoadFontOrSet("-*-*-*-*-*-*-*-*-*-*-*-*-iso8859-*",
			characterSet);
	}
	return new FontHandle(newid);
#else
	return new FontHandle(0);
#endif
}

Font::Font() : fid(0) {}

Font::~Font() {}

void Font::Create(const char *faceName, int characterSet, int size,
	bool bold, bool italic, int) {
	Release();
	fid = FontCached::FindOrCreate(faceName, characterSet, size, bold, italic);
}

void Font::Release() {
	if (fid)
		FontCached::ReleaseId(fid);
	fid = 0;
}

// Required on OS X
#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif
class SurfaceImpl : public Surface {
	encodingType et;
#ifdef USE_CAIRO
	cairo_t *context;
	cairo_surface_t *psurf;
#else
	GdkDrawable *drawable;
	GdkGC *gc;
	GdkPixmap *ppixmap;
#endif
	int x;
	int y;
	bool inited;
	bool createdGC;
	PangoContext *pcontext;
	PangoLayout *layout;
	Converter conv;
	int characterSet;
	void SetConverter(int characterSet_);
public:
	SurfaceImpl();
	virtual ~SurfaceImpl();

	void Init(WindowID wid);
	void Init(SurfaceID sid, WindowID wid);
	void InitPixMap(int width, int height, Surface *surface_, WindowID wid);

	void Release();
	bool Initialised();
	void PenColour(ColourAllocated fore);
	int LogPixelsY();
	int DeviceHeightFont(int points);
	void MoveTo(int x_, int y_);
	void LineTo(int x_, int y_);
	void Polygon(Point *pts, int npts, ColourAllocated fore, ColourAllocated back);
	void RectangleDraw(PRectangle rc, ColourAllocated fore, ColourAllocated back);
	void FillRectangle(PRectangle rc, ColourAllocated back);
	void FillRectangle(PRectangle rc, Surface &surfacePattern);
	void RoundedRectangle(PRectangle rc, ColourAllocated fore, ColourAllocated back);
	void AlphaRectangle(PRectangle rc, int cornerSize, ColourAllocated fill, int alphaFill,
		ColourAllocated outline, int alphaOutline, int flags);
	void Ellipse(PRectangle rc, ColourAllocated fore, ColourAllocated back);
	void Copy(PRectangle rc, Point from, Surface &surfaceSource);

	void DrawTextBase(PRectangle rc, Font &font_, int ybase, const char *s, int len, ColourAllocated fore);
	void DrawTextNoClip(PRectangle rc, Font &font_, int ybase, const char *s, int len, ColourAllocated fore, ColourAllocated back);
	void DrawTextClipped(PRectangle rc, Font &font_, int ybase, const char *s, int len, ColourAllocated fore, ColourAllocated back);
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

	int SetPalette(Palette *pal, bool inBackGround);
	void SetClip(PRectangle rc);
	void FlushCachedState();

	void SetUnicodeMode(bool unicodeMode_);
	void SetDBCSMode(int codePage);
};
#ifdef SCI_NAMESPACE
}
#endif

const char *CharacterSetID(int characterSet) {
	switch (characterSet) {
	case SC_CHARSET_ANSI:
		return "";
	case SC_CHARSET_DEFAULT:
		return "ISO-8859-1";
	case SC_CHARSET_BALTIC:
		return "ISO-8859-13";
	case SC_CHARSET_CHINESEBIG5:
		return "BIG-5";
	case SC_CHARSET_EASTEUROPE:
		return "ISO-8859-2";
	case SC_CHARSET_GB2312:
		return "CP936";
	case SC_CHARSET_GREEK:
		return "ISO-8859-7";
	case SC_CHARSET_HANGUL:
		return "CP949";
	case SC_CHARSET_MAC:
		return "MACINTOSH";
	case SC_CHARSET_OEM:
		return "ASCII";
	case SC_CHARSET_RUSSIAN:
		return "KOI8-R";
	case SC_CHARSET_CYRILLIC:
		return "CP1251";
	case SC_CHARSET_SHIFTJIS:
		return "SHIFT-JIS";
	case SC_CHARSET_SYMBOL:
		return "";
	case SC_CHARSET_TURKISH:
		return "ISO-8859-9";
	case SC_CHARSET_JOHAB:
		return "CP1361";
	case SC_CHARSET_HEBREW:
		return "ISO-8859-8";
	case SC_CHARSET_ARABIC:
		return "ISO-8859-6";
	case SC_CHARSET_VIETNAMESE:
		return "";
	case SC_CHARSET_THAI:
		return "ISO-8859-11";
	case SC_CHARSET_8859_15:
		return "ISO-8859-15";
	default:
		return "";
	}
}

void SurfaceImpl::SetConverter(int characterSet_) {
	if (characterSet != characterSet_) {
		characterSet = characterSet_;
		conv.Open("UTF-8", CharacterSetID(characterSet), false);
	}
}

SurfaceImpl::SurfaceImpl() : et(singleByte),
#ifdef USE_CAIRO
context(0),
psurf(0),
#else
drawable(0),
gc(0),
ppixmap(0),
#endif
x(0), y(0), inited(false), createdGC(false)
, pcontext(0), layout(0), characterSet(-1) {
}

SurfaceImpl::~SurfaceImpl() {
	Release();
}

void SurfaceImpl::Release() {
	et = singleByte;
#ifndef USE_CAIRO
	drawable = 0;
#endif
	if (createdGC) {
		createdGC = false;
#ifdef USE_CAIRO
		cairo_destroy(context);
#else
		g_object_unref(gc);
#endif
	}
#ifdef USE_CAIRO
	context = 0;
	if (psurf)
		cairo_surface_destroy(psurf);
	psurf = 0;
#else
	gc = 0;
	if (ppixmap)
		g_object_unref(ppixmap);
	ppixmap = 0;
#endif
	if (layout)
		g_object_unref(layout);
	layout = 0;
	if (pcontext)
		g_object_unref(pcontext);
	pcontext = 0;
	conv.Close();
	characterSet = -1;
	x = 0;
	y = 0;
	inited = false;
	createdGC = false;
}

bool SurfaceImpl::Initialised() {
	return inited;
}

void SurfaceImpl::Init(WindowID wid) {
	Release();
	PLATFORM_ASSERT(wid);
#ifdef USE_CAIRO
	GdkDrawable *drawable_ = GDK_DRAWABLE(PWidget(wid)->window);
	if (drawable_) {
		context = gdk_cairo_create(drawable_);
		PLATFORM_ASSERT(context);
	} else {
		// Shouldn't happen with valid window but may when calls made before
		// window completely allocated and mapped.
		psurf = cairo_image_surface_create(CAIRO_FORMAT_RGB24, 1, 1);
		context = cairo_create(psurf);
	}
	createdGC = true;
	pcontext = pango_cairo_create_context(context);
	PLATFORM_ASSERT(pcontext);
	layout = pango_cairo_create_layout(context);
	PLATFORM_ASSERT(layout);
#else
	pcontext = gtk_widget_create_pango_context(PWidget(wid));
	PLATFORM_ASSERT(pcontext);
	layout = pango_layout_new(pcontext);
	PLATFORM_ASSERT(layout);
#endif
	inited = true;
}

void SurfaceImpl::Init(SurfaceID sid, WindowID wid) {
	PLATFORM_ASSERT(sid);
	GdkDrawable *drawable_ = reinterpret_cast<GdkDrawable *>(sid);
	Release();
	PLATFORM_ASSERT(wid);
#ifdef USE_CAIRO
	context = gdk_cairo_create(drawable_);
#else
	gc = gdk_gc_new(drawable_);
#endif
#ifdef USE_CAIRO
	pcontext = pango_cairo_create_context(context);
	PLATFORM_ASSERT(pcontext);
	layout = pango_cairo_create_layout(context);
	PLATFORM_ASSERT(layout);
#else
	pcontext = gtk_widget_create_pango_context(PWidget(wid));
	layout = pango_layout_new(pcontext);
	drawable = drawable_;
#endif
#ifdef USE_CAIRO
	cairo_set_line_width(context, 1);
#else
	// Ask for lines that do not paint the last pixel so is like Win32
	gdk_gc_set_line_attributes(gc, 0, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
#endif
	createdGC = true;
	inited = true;
}

void SurfaceImpl::InitPixMap(int width, int height, Surface *surface_, WindowID wid) {
	PLATFORM_ASSERT(surface_);
	Release();
	SurfaceImpl *surfImpl = static_cast<SurfaceImpl *>(surface_);
	PLATFORM_ASSERT(wid);
#ifdef USE_CAIRO
	context = cairo_reference(surfImpl->context);
#else
	PLATFORM_ASSERT(surfImpl->drawable);
	gc = gdk_gc_new(surfImpl->drawable);
#endif
#ifdef USE_CAIRO
	pcontext = pango_cairo_create_context(context);
	PLATFORM_ASSERT(pcontext);
	layout = pango_cairo_create_layout(context);
	PLATFORM_ASSERT(layout);
	if (height > 0 && width > 0)
		psurf = gdk_window_create_similar_surface(
			gtk_widget_get_window(PWidget(wid)),
			CAIRO_CONTENT_COLOR_ALPHA, width, height);
#else
	pcontext = gtk_widget_create_pango_context(PWidget(wid));
	PLATFORM_ASSERT(pcontext);
	layout = pango_layout_new(pcontext);
	PLATFORM_ASSERT(layout);
	if (height > 0 && width > 0)
		ppixmap = gdk_pixmap_new(surfImpl->drawable, width, height, -1);
	drawable = ppixmap;
#endif
#ifdef USE_CAIRO
	cairo_destroy(context);
	context = cairo_create(psurf);
	cairo_rectangle(context, 0, 0, width, height);
	cairo_set_source_rgb(context, 1.0, 0, 0);
	cairo_fill(context);
	// This produces sharp drawing more similar to GDK:
	//cairo_set_antialias(context, CAIRO_ANTIALIAS_NONE);
#endif
#ifdef USE_CAIRO
	cairo_set_line_width(context, 1);
#else
	// Ask for lines that do not paint the last pixel so is like Win32
	gdk_gc_set_line_attributes(gc, 0, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
#endif
	createdGC = true;
	inited = true;
}

void SurfaceImpl::PenColour(ColourAllocated fore) {
#ifdef USE_CAIRO
	if (context) {
		ColourDesired cdFore(fore.AsLong());
		cairo_set_source_rgb(context,
			cdFore.GetBlue() / 255.0,
			cdFore.GetGreen() / 255.0,
			cdFore.GetRed() / 255.0);
	}
#else
	if (gc) {
		GdkColor co;
		co.pixel = fore.AsLong();
		gdk_gc_set_foreground(gc, &co);
	}
#endif
}

int SurfaceImpl::LogPixelsY() {
	return 72;
}

int SurfaceImpl::DeviceHeightFont(int points) {
	int logPix = LogPixelsY();
	return (points * logPix + logPix / 2) / 72;
}

void SurfaceImpl::MoveTo(int x_, int y_) {
	x = x_;
	y = y_;
}

#ifdef USE_CAIRO
static int Delta(int difference) {
	if (difference < 0)
		return -1;
	else if (difference > 0)
		return 1;
	else
		return 0;
}
#endif

void SurfaceImpl::LineTo(int x_, int y_) {
#ifdef USE_CAIRO
	// cairo_line_to draws the end position, unlike Win32 or GDK with GDK_CAP_NOT_LAST.
	// For simple cases, move back one pixel from end.
	if (context) {
		int xDiff = x_ - x;
		int xDelta = Delta(xDiff);
		int yDiff = y_ - y;
		int yDelta = Delta(yDiff);
		if ((xDiff == 0) || (yDiff == 0)) {
			// Horizontal or vertical lines can be more precisely drawn as a filled rectangle
			int xEnd = x_ - xDelta;
			int left = Platform::Minimum(x, xEnd);
			int width = abs(x - xEnd) + 1;
			int yEnd = y_ - yDelta;
			int top = Platform::Minimum(y, yEnd);
			int height = abs(y - yEnd) + 1;
			cairo_rectangle(context, left, top, width, height);
			cairo_fill(context);
		} else if ((abs(xDiff) == abs(yDiff))) {
			// 45 degree slope
			cairo_move_to(context, x + 0.5, y + 0.5);
			cairo_line_to(context, x_ + 0.5 - xDelta, y_ + 0.5 - yDelta);
		} else {
			// Line has a different slope so difficult to avoid last pixel
			cairo_move_to(context, x + 0.5, y + 0.5);
			cairo_line_to(context, x_ + 0.5, y_ + 0.5);
		}
		cairo_stroke(context);
	}
#else
	if (drawable && gc) {
		gdk_draw_line(drawable, gc,
		              x, y,
		              x_, y_);
	}
#endif
	x = x_;
	y = y_;
}

void SurfaceImpl::Polygon(Point *pts, int npts, ColourAllocated fore,
                          ColourAllocated back) {
#ifdef USE_CAIRO
	PenColour(back);
	cairo_move_to(context, pts[0].x + 0.5, pts[0].y + 0.5);
	for (int i = 1;i < npts;i++) {
		cairo_line_to(context, pts[i].x + 0.5, pts[i].y + 0.5);
	}
	cairo_close_path(context);
	cairo_fill_preserve(context);
	PenColour(fore);
	cairo_stroke(context);
#else
	GdkPoint gpts[20];
	if (npts < static_cast<int>((sizeof(gpts) / sizeof(gpts[0])))) {
		for (int i = 0;i < npts;i++) {
			gpts[i].x = pts[i].x;
			gpts[i].y = pts[i].y;
		}
		PenColour(back);
		gdk_draw_polygon(drawable, gc, 1, gpts, npts);
		PenColour(fore);
		gdk_draw_polygon(drawable, gc, 0, gpts, npts);
	}
#endif
}

void SurfaceImpl::RectangleDraw(PRectangle rc, ColourAllocated fore, ColourAllocated back) {
#ifdef USE_CAIRO
	if (context) {
#else
	if (gc && drawable) {
#endif
#ifdef USE_CAIRO
		cairo_rectangle(context, rc.left + 0.5, rc.top + 0.5,
	                     rc.right - rc.left - 1, rc.bottom - rc.top - 1);
		PenColour(back);
		cairo_fill_preserve(context);
		PenColour(fore);
		cairo_stroke(context);
#else
		PenColour(back);
		gdk_draw_rectangle(drawable, gc, 1,
		                   rc.left + 1, rc.top + 1,
		                   rc.right - rc.left - 2, rc.bottom - rc.top - 2);
		PenColour(fore);
		// The subtraction of 1 off the width and height here shouldn't be needed but
		// otherwise a different rectangle is drawn than would be done if the fill parameter == 1
		gdk_draw_rectangle(drawable, gc, 0,
		                   rc.left, rc.top,
		                   rc.right - rc.left - 1, rc.bottom - rc.top - 1);
#endif
	}
}

void SurfaceImpl::FillRectangle(PRectangle rc, ColourAllocated back) {
	PenColour(back);
#ifdef USE_CAIRO
	if (context && (rc.left < maxCoordinate)) {	// Protect against out of range
		cairo_rectangle(context, rc.left, rc.top,
	                     rc.right - rc.left, rc.bottom - rc.top);
		cairo_fill(context);
	}
#else
	if (drawable && (rc.left < maxCoordinate)) {	// Protect against out of range
		gdk_draw_rectangle(drawable, gc, 1,
		                   rc.left, rc.top,
		                   rc.right - rc.left, rc.bottom - rc.top);
	}
#endif
}

void SurfaceImpl::FillRectangle(PRectangle rc, Surface &surfacePattern) {
	SurfaceImpl &surfi = static_cast<SurfaceImpl &>(surfacePattern);
#ifdef USE_CAIRO
	bool canDraw = surfi.psurf;
#else
	bool canDraw = surfi.drawable;
#endif
	if (canDraw) {
		// Tile pattern over rectangle
		// Currently assumes 8x8 pattern
		int widthPat = 8;
		int heightPat = 8;
		for (int xTile = rc.left; xTile < rc.right; xTile += widthPat) {
			int widthx = (xTile + widthPat > rc.right) ? rc.right - xTile : widthPat;
			for (int yTile = rc.top; yTile < rc.bottom; yTile += heightPat) {
				int heighty = (yTile + heightPat > rc.bottom) ? rc.bottom - yTile : heightPat;
#ifdef USE_CAIRO
				cairo_set_source_surface(context, surfi.psurf, xTile, yTile);
				cairo_rectangle(context, xTile, yTile, widthx, heighty);
				cairo_fill(context);
#else
				gdk_draw_drawable(drawable,
				                gc,
				                static_cast<SurfaceImpl &>(surfacePattern).drawable,
				                0, 0,
				                xTile, yTile,
				                widthx, heighty);
#endif
			}
		}
	} else {
		// Something is wrong so try to show anyway
		// Shows up black because colour not allocated
		FillRectangle(rc, ColourAllocated(0));
	}
}

void SurfaceImpl::RoundedRectangle(PRectangle rc, ColourAllocated fore, ColourAllocated back) {
	if (((rc.right - rc.left) > 4) && ((rc.bottom - rc.top) > 4)) {
		// Approximate a round rect with some cut off corners
		Point pts[] = {
		                  Point(rc.left + 2, rc.top),
		                  Point(rc.right - 2, rc.top),
		                  Point(rc.right, rc.top + 2),
		                  Point(rc.right, rc.bottom - 2),
		                  Point(rc.right - 2, rc.bottom),
		                  Point(rc.left + 2, rc.bottom),
		                  Point(rc.left, rc.bottom - 2),
		                  Point(rc.left, rc.top + 2),
		              };
		Polygon(pts, sizeof(pts) / sizeof(pts[0]), fore, back);
	} else {
		RectangleDraw(rc, fore, back);
	}
}

#ifdef USE_CAIRO

static void PathRoundRectangle(cairo_t *context, double left, double top, double width, double height, int radius) {
	double degrees = M_PI / 180.0;

	cairo_new_sub_path(context);
	cairo_arc(context, left + width - radius, top + radius, radius, -90 * degrees, 0 * degrees);
	cairo_arc(context, left + width - radius, top + height - radius, radius, 0 * degrees, 90 * degrees);
	cairo_arc(context, left + radius, top + height - radius, radius, 90 * degrees, 180 * degrees);
	cairo_arc(context, left + radius, top + radius, radius, 180 * degrees, 270 * degrees);
	cairo_close_path(context);
}

#else

// Plot a point into a guint32 buffer symetrically to all 4 qudrants
static void AllFour(guint32 *pixels, int stride, int width, int height, int x, int y, guint32 val) {
	pixels[y*stride+x] = val;
	pixels[y*stride+width-1-x] = val;
	pixels[(height-1-y)*stride+x] = val;
	pixels[(height-1-y)*stride+width-1-x] = val;
}

static guint32 u32FromRGBA(guint8 r, guint8 g, guint8 b, guint8 a) {
	union {
		guint8 pixVal[4];
		guint32 val;
	} converter;
	converter.pixVal[0] = r;
	converter.pixVal[1] = g;
	converter.pixVal[2] = b;
	converter.pixVal[3] = a;
	return converter.val;
}

#endif

static unsigned int GetRValue(unsigned int co) {
	return (co >> 16) & 0xff;
}

static unsigned int GetGValue(unsigned int co) {
	return (co >> 8) & 0xff;
}

static unsigned int GetBValue(unsigned int co) {
	return co & 0xff;
}

void SurfaceImpl::AlphaRectangle(PRectangle rc, int cornerSize, ColourAllocated fill, int alphaFill,
		ColourAllocated outline, int alphaOutline, int flags) {
#ifdef USE_CAIRO
	if (context && rc.Width() > 0) {
		cairo_set_source_rgba(context,
			GetRValue(fill.AsLong()) / 255.0,
			GetGValue(fill.AsLong()) / 255.0,
			GetBValue(fill.AsLong()) / 255.0,
			alphaFill / 255.0);
		PathRoundRectangle(context, rc.left + 1.0, rc.top+1.0, rc.right - rc.left - 2.0, rc.bottom - rc.top - 2.0, cornerSize);
		cairo_fill(context);

		cairo_set_source_rgba(context,
			GetRValue(outline.AsLong()) / 255.0,
			GetGValue(outline.AsLong()) / 255.0,
			GetBValue(outline.AsLong()) / 255.0,
			alphaOutline / 255.0);
		PathRoundRectangle(context, rc.left +0.5, rc.top+0.5, rc.right - rc.left - 1, rc.bottom - rc.top - 1, cornerSize);
		cairo_stroke(context);
	}
#else
	if (gc && drawable && rc.Width() > 0) {
		int width = rc.Width();
		int height = rc.Height();
		// Ensure not distorted too much by corners when small
		cornerSize = Platform::Minimum(cornerSize, (Platform::Minimum(width, height) / 2) - 2);
		// Make a 32 bit deep pixbuf with alpha
		GdkPixbuf *pixalpha = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);

		guint32 valEmpty = u32FromRGBA(0,0,0,0);
		guint32 valFill = u32FromRGBA(GetRValue(fill.AsLong()),
			GetGValue(fill.AsLong()), GetBValue(fill.AsLong()), alphaFill);
		guint32 valOutline = u32FromRGBA(GetRValue(outline.AsLong()),
			GetGValue(outline.AsLong()), GetBValue(outline.AsLong()), alphaOutline);
		guint32 *pixels = reinterpret_cast<guint32 *>(gdk_pixbuf_get_pixels(pixalpha));
		int stride = gdk_pixbuf_get_rowstride(pixalpha) / 4;
		for (int yr=0; yr<height; yr++) {
			for (int xr=0; xr<width; xr++) {
				if ((xr==0) || (xr==width-1) || (yr == 0) || (yr == height-1)) {
					pixels[yr*stride+xr] = valOutline;
				} else {
					pixels[yr*stride+xr] = valFill;
				}
			}
		}
		for (int c=0;c<cornerSize; c++) {
			for (int xr=0;xr<c+1; xr++) {
				AllFour(pixels, stride, width, height, xr, c-xr, valEmpty);
			}
		}
		for (int xr=1;xr<cornerSize; xr++) {
			AllFour(pixels, stride, width, height, xr, cornerSize-xr, valOutline);
		}

		// Draw with alpha
		gdk_draw_pixbuf(drawable, gc, pixalpha,
			0,0, rc.left,rc.top, width,height, GDK_RGB_DITHER_NORMAL, 0, 0);

		g_object_unref(pixalpha);
	}
#endif
}

void SurfaceImpl::Ellipse(PRectangle rc, ColourAllocated fore, ColourAllocated back) {
	PenColour(back);
#ifdef USE_CAIRO
	cairo_arc(context, (rc.left + rc.right) / 2 + 0.5, (rc.top + rc.bottom) / 2 + 0.5,
		Platform::Minimum(rc.Width(), rc.Height()) / 2, 0, 2*M_PI);
	cairo_fill_preserve(context);
	PenColour(fore);
	cairo_stroke(context);
#else
	gdk_draw_arc(drawable, gc, 1,
	             rc.left + 1, rc.top + 1,
	             rc.right - rc.left - 2, rc.bottom - rc.top - 2,
	             0, 32767);

	// The subtraction of 1 here is similar to the case for RectangleDraw
	PenColour(fore);
	gdk_draw_arc(drawable, gc, 0,
	             rc.left, rc.top,
	             rc.right - rc.left - 1, rc.bottom - rc.top - 1,
	             0, 32767);
#endif
}

void SurfaceImpl::Copy(PRectangle rc, Point from, Surface &surfaceSource) {
	SurfaceImpl &surfi = static_cast<SurfaceImpl &>(surfaceSource);
#ifdef USE_CAIRO
	bool canDraw = surfi.psurf;
#else
	bool canDraw = surfi.drawable;
#endif
	if (canDraw) {
#ifdef USE_CAIRO
		cairo_set_source_surface(context, surfi.psurf,
			rc.left - from.x, rc.top - from.y);
		cairo_rectangle(context, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top);
		cairo_fill(context);
#else
		gdk_draw_drawable(drawable,
		                gc,
		                static_cast<SurfaceImpl &>(surfaceSource).drawable,
		                from.x, from.y,
		                rc.left, rc.top,
		                rc.right - rc.left, rc.bottom - rc.top);
#endif
	}
}

#ifndef DISABLE_GDK_FONT
static size_t UTF8Len(char ch) {
	unsigned char uch = static_cast<unsigned char>(ch);
	if (uch < 0x80)
		return 1;
	else if (uch < (0x80 + 0x40 + 0x20))
		return 2;
	else
		return 3;
}
#endif

char *UTF8FromLatin1(const char *s, int &len) {
	char *utfForm = new char[len*2+1];
	size_t lenU = 0;
	for (int i=0;i<len;i++) {
		unsigned int uch = static_cast<unsigned char>(s[i]);
		if (uch < 0x80) {
			utfForm[lenU++] = uch;
		} else {
			utfForm[lenU++] = static_cast<char>(0xC0 | (uch >> 6));
			utfForm[lenU++] = static_cast<char>(0x80 | (uch & 0x3f));
		}
	}
	utfForm[lenU] = '\0';
	len = lenU;
	return utfForm;
}

static char *UTF8FromIconv(const Converter &conv, const char *s, int &len) {
	if (conv) {
		char *utfForm = new char[len*3+1];
		char *pin = const_cast<char *>(s);
		size_t inLeft = len;
		char *pout = utfForm;
		size_t outLeft = len*3+1;
		size_t conversions = conv.Convert(&pin, &inLeft, &pout, &outLeft);
		if (conversions != ((size_t)(-1))) {
			*pout = '\0';
			len = pout - utfForm;
			return utfForm;
		}
		delete []utfForm;
	}
	return 0;
}

// Work out how many bytes are in a character by trying to convert using iconv,
// returning the first length that succeeds.
static size_t MultiByteLenFromIconv(const Converter &conv, const char *s, size_t len) {
	for (size_t lenMB=1; (lenMB<4) && (lenMB <= len); lenMB++) {
		char wcForm[2];
		char *pin = const_cast<char *>(s);
		size_t inLeft = lenMB;
		char *pout = wcForm;
		size_t outLeft = 2;
		size_t conversions = conv.Convert(&pin, &inLeft, &pout, &outLeft);
		if (conversions != ((size_t)(-1))) {
			return lenMB;
		}
	}
	return 1;
}

#ifndef DISABLE_GDK_FONT
static char *UTF8FromGdkWChar(GdkWChar *wctext, int wclen) {
	char *utfForm = new char[wclen*3+1];	// Maximum of 3 UTF-8 bytes per character
	size_t lenU = 0;
	for (int i = 0; i < wclen && wctext[i]; i++) {
		unsigned int uch = wctext[i];
		if (uch < 0x80) {
			utfForm[lenU++] = static_cast<char>(uch);
		} else if (uch < 0x800) {
			utfForm[lenU++] = static_cast<char>(0xC0 | (uch >> 6));
			utfForm[lenU++] = static_cast<char>(0x80 | (uch & 0x3f));
		} else {
			utfForm[lenU++] = static_cast<char>(0xE0 | (uch >> 12));
			utfForm[lenU++] = static_cast<char>(0x80 | ((uch >> 6) & 0x3f));
			utfForm[lenU++] = static_cast<char>(0x80 | (uch & 0x3f));
		}
	}
	utfForm[lenU] = '\0';
	return utfForm;
}
#endif

static char *UTF8FromDBCS(const char *s, int &len) {
#ifndef DISABLE_GDK_FONT
	GdkWChar *wctext = new GdkWChar[len + 1];
	GdkWChar *wcp = wctext;
	int wclen = gdk_mbstowcs(wcp, s, len);
	if (wclen < 1) {
		// In the annoying case when non-locale chars in the line.
		// e.g. latin1 chars in Japanese locale.
		delete []wctext;
		return 0;
	}

	char *utfForm = UTF8FromGdkWChar(wctext, wclen);
	delete []wctext;
	len = strlen(utfForm);
	return utfForm;
#else
	return 0;
#endif
}

static size_t UTF8CharLength(const char *s) {
	const unsigned char *us = reinterpret_cast<const unsigned char *>(s);
	unsigned char ch = *us;
	if (ch < 0x80) {
		return 1;
	} else if (ch < 0x80 + 0x40 + 0x20) {
		return 2;
	} else {
		return 3;
	}
}

// On GTK+, wchar_t is 4 bytes

const int maxLengthTextRun = 10000;

void SurfaceImpl::DrawTextBase(PRectangle rc, Font &font_, int ybase, const char *s, int len,
                                 ColourAllocated fore) {
	PenColour(fore);
#ifdef USE_CAIRO
	if (context) {
#else
	if (gc && drawable) {
#endif
		int xText = rc.left;
		if (PFont(font_)->pfd) {
			char *utfForm = 0;
			bool useGFree = false;
			if (et == UTF8) {
				pango_layout_set_text(layout, s, len);
			} else {
				if (!utfForm) {
					SetConverter(PFont(font_)->characterSet);
					utfForm = UTF8FromIconv(conv, s, len);
				}
				if (!utfForm) {	// iconv failed so try DBCS if DBCS mode
					if (et == dbcs) {
						// Convert to utf8
						utfForm = UTF8FromDBCS(s, len);
					}
				}
				if (!utfForm) {	// iconv and DBCS failed so treat as Latin1
					utfForm = UTF8FromLatin1(s, len);
				}
				pango_layout_set_text(layout, utfForm, len);
			}
			pango_layout_set_font_description(layout, PFont(font_)->pfd);
#ifdef USE_CAIRO
			pango_cairo_update_layout(context, layout);
#endif
#ifdef PANGO_VERSION
			PangoLayoutLine *pll = pango_layout_get_line_readonly(layout,0);
#else
			PangoLayoutLine *pll = pango_layout_get_line(layout,0);
#endif
#ifdef USE_CAIRO
			cairo_move_to(context, xText, ybase);
			pango_cairo_show_layout_line(context, pll);
#else
			gdk_draw_layout_line(drawable, gc, xText, ybase, pll);
#endif
			if (useGFree) {
				g_free(utfForm);
			} else {
				delete []utfForm;
			}
			return;
		}
#ifndef DISABLE_GDK_FONT
		// Draw text as a series of segments to avoid limitations in X servers
		const int segmentLength = 1000;
		bool draw8bit = true;
		if (et != singleByte) {
			GdkWChar wctext[maxLengthTextRun];
			if (len >= maxLengthTextRun)
				len = maxLengthTextRun-1;
			int wclen;
			if (et == UTF8) {
				wclen = UTF16FromUTF8(s, len,
					static_cast<wchar_t *>(static_cast<void *>(wctext)), maxLengthTextRun - 1);
			} else {	// dbcs, so convert using current locale
				char sMeasure[maxLengthTextRun];
				memcpy(sMeasure, s, len);
				sMeasure[len] = '\0';
				wclen = gdk_mbstowcs(
					wctext, sMeasure, maxLengthTextRun - 1);
			}
			if (wclen > 0) {
				draw8bit = false;
				wctext[wclen] = L'\0';
				GdkWChar *wcp = wctext;
				while ((wclen > 0) && (xText < maxCoordinate)) {
					int lenDraw = Platform::Minimum(wclen, segmentLength);
					gdk_draw_text_wc(drawable, PFont(font_)->pfont, gc,
							 xText, ybase, wcp, lenDraw);
					wclen -= lenDraw;
					if (wclen > 0) {	// Avoid next calculation if possible as may be expensive
						xText += gdk_text_width_wc(PFont(font_)->pfont,
								       wcp, lenDraw);
					}
					wcp += lenDraw;
				}
			}
		}
		if (draw8bit) {
			while ((len > 0) && (xText < maxCoordinate)) {
				int lenDraw = Platform::Minimum(len, segmentLength);
				gdk_draw_text(drawable, PFont(font_)->pfont, gc,
				              xText, ybase, s, lenDraw);
				len -= lenDraw;
				if (len > 0) {	// Avoid next calculation if possible as may be expensive
					xText += gdk_text_width(PFont(font_)->pfont, s, lenDraw);
				}
				s += lenDraw;
			}
		}
#endif
	}
}

void SurfaceImpl::DrawTextNoClip(PRectangle rc, Font &font_, int ybase, const char *s, int len,
                                 ColourAllocated fore, ColourAllocated back) {
	FillRectangle(rc, back);
	DrawTextBase(rc, font_, ybase, s, len, fore);
}

// On GTK+, exactly same as DrawTextNoClip
void SurfaceImpl::DrawTextClipped(PRectangle rc, Font &font_, int ybase, const char *s, int len,
                                  ColourAllocated fore, ColourAllocated back) {
	FillRectangle(rc, back);
	DrawTextBase(rc, font_, ybase, s, len, fore);
}

void SurfaceImpl::DrawTextTransparent(PRectangle rc, Font &font_, int ybase, const char *s, int len,
                                  ColourAllocated fore) {
	// Avoid drawing spaces in transparent mode
	for (int i=0;i<len;i++) {
		if (s[i] != ' ') {
			DrawTextBase(rc, font_, ybase, s, len, fore);
			return;
		}
	}
}

class ClusterIterator {
	PangoLayoutIter *iter;
	PangoRectangle pos;
	int lenPositions;
public:
	bool finished;
	int positionStart;
	int position;
	int distance;
	int curIndex;
	ClusterIterator(PangoLayout *layout, int len) : lenPositions(len), finished(false),
		positionStart(0), position(0), distance(0), curIndex(0) {
		iter = pango_layout_get_iter(layout);
		pango_layout_iter_get_cluster_extents(iter, NULL, &pos);
	}
	~ClusterIterator() {
		pango_layout_iter_free(iter);
	}

	void Next() {
		positionStart = position;
		if (pango_layout_iter_next_cluster(iter)) {
			pango_layout_iter_get_cluster_extents(iter, NULL, &pos);
			position = PANGO_PIXELS(pos.x);
			curIndex = pango_layout_iter_get_index(iter);
		} else {
			finished = true;
			position = PANGO_PIXELS(pos.x + pos.width);
			curIndex = lenPositions;
		}
		distance = position - positionStart;
	}
};

void SurfaceImpl::MeasureWidths(Font &font_, const char *s, int len, int *positions) {
	if (font_.GetID()) {
		const int lenPositions = len;
		if (PFont(font_)->pfd) {
			if (len == 1) {
				int width = PFont(font_)->CharWidth(*s, et);
				if (width) {
					positions[0] = width;
					return;
				}
			}
			pango_layout_set_font_description(layout, PFont(font_)->pfd);
			if (et == UTF8) {
				// Simple and direct as UTF-8 is native Pango encoding
				int i = 0;
				pango_layout_set_text(layout, s, len);
				ClusterIterator iti(layout, lenPositions);
				while (!iti.finished) {
					iti.Next();
					int places = iti.curIndex - i;
					while (i < iti.curIndex) {
						// Evenly distribute space among bytes of this cluster.
						// Would be better to find number of characters and then
						// divide evenly between characters with each byte of a character
						// being at the same position.
						positions[i] = iti.position - (iti.curIndex - 1 - i) * iti.distance / places;
						i++;
					}
				}
				PLATFORM_ASSERT(i == lenPositions);
			} else {
				int positionsCalculated = 0;
				if (et == dbcs) {
					SetConverter(PFont(font_)->characterSet);
					char *utfForm = UTF8FromIconv(conv, s, len);
					if (utfForm) {
						// Convert to UTF-8 so can ask Pango for widths, then
						// Loop through UTF-8 and DBCS forms, taking account of different
						// character byte lengths.
						Converter convMeasure("UCS-2", CharacterSetID(characterSet), false);
						pango_layout_set_text(layout, utfForm, strlen(utfForm));
						int i = 0;
						int clusterStart = 0;
						ClusterIterator iti(layout, strlen(utfForm));
						while (!iti.finished) {
							iti.Next();
							int clusterEnd = iti.curIndex;
							int places = g_utf8_strlen(utfForm + clusterStart, clusterEnd - clusterStart);
							int place = 1;
							while (clusterStart < clusterEnd) {
								size_t lenChar = MultiByteLenFromIconv(convMeasure, s+i, len-i);
								while (lenChar--) {
									positions[i++] = iti.position - (places - place) * iti.distance / places;
									positionsCalculated++;
								}
								clusterStart += UTF8CharLength(utfForm+clusterStart);
								place++;
							}
						}
						delete []utfForm;
						PLATFORM_ASSERT(i == lenPositions);
					}
				}
				if (positionsCalculated < 1 ) {
					// Either Latin1 or DBCS conversion failed so treat as Latin1.
					bool useGFree = false;
					SetConverter(PFont(font_)->characterSet);
					char *utfForm = UTF8FromIconv(conv, s, len);
					if (!utfForm) {
						utfForm = UTF8FromLatin1(s, len);
					}
					pango_layout_set_text(layout, utfForm, len);
					int i = 0;
					int clusterStart = 0;
					// Each Latin1 input character may take 1 or 2 bytes in UTF-8
					// and groups of up to 3 may be represented as ligatures.
					ClusterIterator iti(layout, strlen(utfForm));
					while (!iti.finished) {
						iti.Next();
						int clusterEnd = iti.curIndex;
						int ligatureLength = g_utf8_strlen(utfForm + clusterStart, clusterEnd - clusterStart);
						PLATFORM_ASSERT(ligatureLength > 0 && ligatureLength <= 3);
						for (int charInLig=0; charInLig<ligatureLength; charInLig++) {
							positions[i++] = iti.position - (ligatureLength - 1 - charInLig) * iti.distance / ligatureLength;
						}
						clusterStart = clusterEnd;
					}
					if (useGFree) {
						g_free(utfForm);
					} else {
						delete []utfForm;
					}
					PLATFORM_ASSERT(i == lenPositions);
				}
			}
			if (len == 1) {
				PFont(font_)->SetCharWidth(*s, positions[0], et);
			}
			return;
		}
#ifndef DISABLE_GDK_FONT
		int totalWidth = 0;
		GdkFont *gf = PFont(font_)->pfont;
		bool measure8bit = true;
		if (et != singleByte) {
			GdkWChar wctext[maxLengthTextRun];
			if (len >= maxLengthTextRun)
				len = maxLengthTextRun-1;
			int wclen;
			if (et == UTF8) {
				wclen = UTF16FromUTF8(s, len,
					static_cast<wchar_t *>(static_cast<void *>(wctext)), maxLengthTextRun - 1);
			} else {	// dbcsMode, so convert using current locale
				char sDraw[maxLengthTextRun];
				memcpy(sDraw, s, len);
				sDraw[len] = '\0';
				wclen = gdk_mbstowcs(
					wctext, sDraw, maxLengthTextRun - 1);
			}
			if (wclen > 0) {
				measure8bit = false;
				wctext[wclen] = L'\0';
				// Map widths back to utf-8 or DBCS input string
				int i = 0;
				for (int iU = 0; iU < wclen; iU++) {
					int width = gdk_char_width_wc(gf, wctext[iU]);
					totalWidth += width;
					int lenChar;
					if (et == UTF8) {
						lenChar = UTF8Len(s[i]);
					} else {
						lenChar = mblen(s+i, MB_CUR_MAX);
						if (lenChar < 0)
							lenChar = 1;
					}
					while (lenChar--) {
						positions[i++] = totalWidth;
					}
				}
				while (i < len) {	// In case of problems with lengths
					positions[i++] = totalWidth;
				}
			}
		}
		if (measure8bit) {
			// Either Latin1 or conversion failed so treat as Latin1.
			for (int i = 0; i < len; i++) {
				int width = gdk_char_width(gf, s[i]);
				totalWidth += width;
				positions[i] = totalWidth;
			}
		}
#endif
	} else {
		// No font so return an ascending range of values
		for (int i = 0; i < len; i++) {
			positions[i] = i + 1;
		}
	}
}

int SurfaceImpl::WidthText(Font &font_, const char *s, int len) {
	if (font_.GetID()) {
		if (PFont(font_)->pfd) {
			char *utfForm = 0;
			pango_layout_set_font_description(layout, PFont(font_)->pfd);
			PangoRectangle pos;
			bool useGFree = false;
			if (et == UTF8) {
				pango_layout_set_text(layout, s, len);
			} else {
				if (et == dbcs) {
					// Convert to utf8
					utfForm = UTF8FromDBCS(s, len);
				}
				if (!utfForm) {	// DBCS failed so treat as iconv
					SetConverter(PFont(font_)->characterSet);
					utfForm = UTF8FromIconv(conv, s, len);
				}
				if (!utfForm) {	// g_locale_to_utf8 failed so treat as Latin1
					utfForm = UTF8FromLatin1(s, len);
				}
				pango_layout_set_text(layout, utfForm, len);
			}
#ifdef PANGO_VERSION
			PangoLayoutLine *pangoLine = pango_layout_get_line_readonly(layout,0);
#else
			PangoLayoutLine *pangoLine = pango_layout_get_line(layout,0);
#endif
			pango_layout_line_get_extents(pangoLine, NULL, &pos);
			if (useGFree) {
				g_free(utfForm);
			} else {
				delete []utfForm;
			}
			return PANGO_PIXELS(pos.width);
		}
#ifndef DISABLE_GDK_FONT
		if (et == UTF8) {
			GdkWChar wctext[maxLengthTextRun];
			size_t wclen = UTF16FromUTF8(s, len, static_cast<wchar_t *>(static_cast<void *>(wctext)),
				sizeof(wctext) / sizeof(GdkWChar) - 1);
			wctext[wclen] = L'\0';
			return gdk_text_width_wc(PFont(font_)->pfont, wctext, wclen);
		} else {
			return gdk_text_width(PFont(font_)->pfont, s, len);
		}
#else
		return 1;
#endif
	} else {
		return 1;
	}
}

int SurfaceImpl::WidthChar(Font &font_, char ch) {
	if (font_.GetID()) {
		if (PFont(font_)->pfd) {
			return WidthText(font_, &ch, 1);
		}
#ifndef DISABLE_GDK_FONT
		return gdk_char_width(PFont(font_)->pfont, ch);
#else
		return 1;
#endif
	} else {
		return 1;
	}
}

// Three possible strategies for determining ascent and descent of font:
// 1) Call gdk_string_extents with string containing all letters, numbers and punctuation.
// 2) Use the ascent and descent fields of GdkFont.
// 3) Call gdk_string_extents with string as 1 but also including accented capitals.
// Smallest values given by 1 and largest by 3 with 2 in between.
// Techniques 1 and 2 sometimes chop off extreme portions of ascenders and
// descenders but are mostly OK except for accented characters like Å which are
// rarely used in code.

// This string contains a good range of characters to test for size.
//const char largeSizeString[] = "ÂÃÅÄ `~!@#$%^&*()-_=+\\|[]{};:\"\'<,>.?/1234567890"
//                               "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
#ifndef FAST_WAY
const char sizeString[] = "`~!@#$%^&*()-_=+\\|[]{};:\"\'<,>.?/1234567890"
                          "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
#endif

int SurfaceImpl::Ascent(Font &font_) {
	if (!(font_.GetID()))
		return 1;
#ifdef FAST_WAY
	FontMutexLock();
	int ascent = PFont(font_)->ascent;
	if ((ascent == 0) && (PFont(font_)->pfd)) {
		PangoFontMetrics *metrics = pango_context_get_metrics(pcontext,
			PFont(font_)->pfd, pango_context_get_language(pcontext));
		PFont(font_)->ascent =
			PANGO_PIXELS(pango_font_metrics_get_ascent(metrics));
		pango_font_metrics_unref(metrics);
		ascent = PFont(font_)->ascent;
	}
#ifndef DISABLE_GDK_FONT
	if ((ascent == 0) && (PFont(font_)->pfont)) {
		ascent = PFont(font_)->pfont->ascent;
	}
#endif
	if (ascent == 0) {
		ascent = 1;
	}
	FontMutexUnlock();
	return ascent;
#else

	gint lbearing;
	gint rbearing;
	gint width;
	gint ascent;
	gint descent;

	gdk_string_extents(PFont(font_)->pfont, sizeString,
					   &lbearing, &rbearing, &width, &ascent, &descent);
	return ascent;
#endif
}

int SurfaceImpl::Descent(Font &font_) {
	if (!(font_.GetID()))
		return 1;
#ifdef FAST_WAY

	if (PFont(font_)->pfd) {
		PangoFontMetrics *metrics = pango_context_get_metrics(pcontext,
			PFont(font_)->pfd, pango_context_get_language(pcontext));
		int descent = PANGO_PIXELS(pango_font_metrics_get_descent(metrics));
		pango_font_metrics_unref(metrics);
		return descent;
	}
#ifndef DISABLE_GDK_FONT
	return PFont(font_)->pfont->descent;
#else
	return 0;
#endif
#else

	gint lbearing;
	gint rbearing;
	gint width;
	gint ascent;
	gint descent;

	gdk_string_extents(PFont(font_)->pfont, sizeString,
					   &lbearing, &rbearing, &width, &ascent, &descent);
	return descent;
#endif
}

int SurfaceImpl::InternalLeading(Font &) {
	return 0;
}

int SurfaceImpl::ExternalLeading(Font &) {
	return 0;
}

int SurfaceImpl::Height(Font &font_) {
	return Ascent(font_) + Descent(font_);
}

int SurfaceImpl::AverageCharWidth(Font &font_) {
	return WidthChar(font_, 'n');
}

int SurfaceImpl::SetPalette(Palette *, bool) {
	// Handled in palette allocation for GTK so this does nothing
	return 0;
}

void SurfaceImpl::SetClip(PRectangle rc) {
#ifdef USE_CAIRO
	cairo_rectangle(context, rc.left, rc.top, rc.right, rc.bottom);
	cairo_clip(context);
#else
	GdkRectangle area = {rc.left, rc.top,
	                     rc.right - rc.left, rc.bottom - rc.top};
	gdk_gc_set_clip_rectangle(gc, &area);
#endif
}

void SurfaceImpl::FlushCachedState() {}

void SurfaceImpl::SetUnicodeMode(bool unicodeMode_) {
	if (unicodeMode_)
		et = UTF8;
}

void SurfaceImpl::SetDBCSMode(int codePage) {
	if (codePage && (codePage != SC_CP_UTF8))
		et = dbcs;
}

Surface *Surface::Allocate() {
	return new SurfaceImpl;
}

Window::~Window() {}

void Window::Destroy() {
	if (wid)
		gtk_widget_destroy(GTK_WIDGET(wid));
	wid = 0;
}

bool Window::HasFocus() {
	return IS_WIDGET_FOCUSSED(wid);
}

PRectangle Window::GetPosition() {
	// Before any size allocated pretend its 1000 wide so not scrolled
	PRectangle rc(0, 0, 1000, 1000);
	if (wid) {
		rc.left = PWidget(wid)->allocation.x;
		rc.top = PWidget(wid)->allocation.y;
		if (PWidget(wid)->allocation.width > 20) {
			rc.right = rc.left + PWidget(wid)->allocation.width;
			rc.bottom = rc.top + PWidget(wid)->allocation.height;
		}
	}
	return rc;
}

void Window::SetPosition(PRectangle rc) {
	GtkAllocation alloc;
	alloc.x = rc.left;
	alloc.y = rc.top;
	alloc.width = rc.Width();
	alloc.height = rc.Height();
	gtk_widget_size_allocate(PWidget(wid), &alloc);
}

void Window::SetPositionRelative(PRectangle rc, Window relativeTo) {
	int ox = 0;
	int oy = 0;
	gdk_window_get_origin(PWidget(relativeTo.wid)->window, &ox, &oy);
	ox += rc.left;
	if (ox < 0)
		ox = 0;
	oy += rc.top;
	if (oy < 0)
		oy = 0;

	/* do some corrections to fit into screen */
	int sizex = rc.right - rc.left;
	int sizey = rc.bottom - rc.top;
	int screenWidth = gdk_screen_width();
	int screenHeight = gdk_screen_height();
	if (sizex > screenWidth)
		ox = 0; /* the best we can do */
	else if (ox + sizex > screenWidth)
		ox = screenWidth - sizex;
	if (oy + sizey > screenHeight)
		oy = screenHeight - sizey;

	gtk_window_move(GTK_WINDOW(PWidget(wid)), ox, oy);

	gtk_widget_set_size_request(PWidget(wid), sizex, sizey);
}

PRectangle Window::GetClientPosition() {
	// On GTK+, the client position is the window position
	return GetPosition();
}

void Window::Show(bool show) {
	if (show)
		gtk_widget_show(PWidget(wid));
}

void Window::InvalidateAll() {
	if (wid) {
		gtk_widget_queue_draw(PWidget(wid));
	}
}

void Window::InvalidateRectangle(PRectangle rc) {
	if (wid) {
		gtk_widget_queue_draw_area(PWidget(wid),
		                           rc.left, rc.top,
		                           rc.right - rc.left, rc.bottom - rc.top);
	}
}

void Window::SetFont(Font &) {
	// Can not be done generically but only needed for ListBox
}

void Window::SetCursor(Cursor curs) {
	// We don't set the cursor to same value numerous times under gtk because
	// it stores the cursor in the window once it's set
	if (curs == cursorLast)
		return;

	cursorLast = curs;
	GdkCursor *gdkCurs;
	switch (curs) {
	case cursorText:
		gdkCurs = gdk_cursor_new(GDK_XTERM);
		break;
	case cursorArrow:
		gdkCurs = gdk_cursor_new(GDK_LEFT_PTR);
		break;
	case cursorUp:
		gdkCurs = gdk_cursor_new(GDK_CENTER_PTR);
		break;
	case cursorWait:
		gdkCurs = gdk_cursor_new(GDK_WATCH);
		break;
	case cursorHand:
		gdkCurs = gdk_cursor_new(GDK_HAND2);
		break;
	case cursorReverseArrow:
		gdkCurs = gdk_cursor_new(GDK_RIGHT_PTR);
		break;
	default:
		gdkCurs = gdk_cursor_new(GDK_LEFT_PTR);
		cursorLast = cursorArrow;
		break;
	}

	if (PWidget(wid)->window)
		gdk_window_set_cursor(PWidget(wid)->window, gdkCurs);
	gdk_cursor_unref(gdkCurs);
}

void Window::SetTitle(const char *s) {
	gtk_window_set_title(GTK_WINDOW(wid), s);
}

/* Returns rectangle of monitor pt is on, both rect and pt are in Window's
   gdk window coordinates */
PRectangle Window::GetMonitorRect(Point pt) {
	gint x_offset, y_offset;

	gdk_window_get_origin(PWidget(wid)->window, &x_offset, &y_offset);

#if GTK_CHECK_VERSION(2,2,0)
	// GTK+ 2.2+
	{
		GdkScreen* screen;
		gint monitor_num;
		GdkRectangle rect;

		screen = gtk_widget_get_screen(PWidget(wid));
		monitor_num = gdk_screen_get_monitor_at_point(screen, pt.x + x_offset, pt.y + y_offset);
		gdk_screen_get_monitor_geometry(screen, monitor_num, &rect);
		rect.x -= x_offset;
		rect.y -= y_offset;
		return PRectangle(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
	}
#else
	pt = pt;
	return PRectangle(-x_offset, -y_offset, (-x_offset) + gdk_screen_width(),
	                  (-y_offset) + gdk_screen_height());
#endif
}

struct ListImage {
	const char *xpm_data;
	GdkPixbuf *pixbuf;
};

static void list_image_free(gpointer, gpointer value, gpointer) {
	ListImage *list_image = (ListImage *) value;
	if (list_image->pixbuf)
		g_object_unref (list_image->pixbuf);
	g_free(list_image);
}

ListBox::ListBox() {
}

ListBox::~ListBox() {
}

enum {
	PIXBUF_COLUMN,
	TEXT_COLUMN,
	N_COLUMNS
};

class ListBoxX : public ListBox {
	WindowID list;
	WindowID scroller;
	void *pixhash;
	GtkCellRenderer* pixbuf_renderer;
	XPMSet xset;
	int desiredVisibleRows;
	unsigned int maxItemCharacters;
	unsigned int aveCharWidth;
public:
	CallBackAction doubleClickAction;
	void *doubleClickActionData;

	ListBoxX() : list(0), scroller(0), pixhash(NULL), pixbuf_renderer(0),
		desiredVisibleRows(5), maxItemCharacters(0),
		aveCharWidth(1), doubleClickAction(NULL), doubleClickActionData(NULL) {
	}
	virtual ~ListBoxX() {
		if (pixhash) {
			g_hash_table_foreach((GHashTable *) pixhash, list_image_free, NULL);
			g_hash_table_destroy((GHashTable *) pixhash);
		}
	}
	virtual void SetFont(Font &font);
	virtual void Create(Window &parent, int ctrlID, Point location_, int lineHeight_, bool unicodeMode_);
	virtual void SetAverageCharWidth(int width);
	virtual void SetVisibleRows(int rows);
	virtual int GetVisibleRows() const;
	virtual PRectangle GetDesiredRect();
	virtual int CaretFromEdge();
	virtual void Clear();
	virtual void Append(char *s, int type = -1);
	virtual int Length();
	virtual void Select(int n);
	virtual int GetSelection();
	virtual int Find(const char *prefix);
	virtual void GetValue(int n, char *value, int len);
	virtual void RegisterImage(int type, const char *xpm_data);
	virtual void ClearRegisteredImages();
	virtual void SetDoubleClickAction(CallBackAction action, void *data) {
		doubleClickAction = action;
		doubleClickActionData = data;
	}
	virtual void SetList(const char *listText, char separator, char typesep);
};

ListBox *ListBox::Allocate() {
	ListBoxX *lb = new ListBoxX();
	return lb;
}

static gboolean ButtonPress(GtkWidget *, GdkEventButton* ev, gpointer p) {
	try {
		ListBoxX* lb = reinterpret_cast<ListBoxX*>(p);
		if (ev->type == GDK_2BUTTON_PRESS && lb->doubleClickAction != NULL) {
			lb->doubleClickAction(lb->doubleClickActionData);
			return TRUE;
		}

	} catch (...) {
		// No pointer back to Scintilla to save status
	}
	return FALSE;
}

/* Change the active color to the selected color so the listbox uses the color
scheme that it would use if it had the focus. */
static void StyleSet(GtkWidget *w, GtkStyle*, void*) {
	GtkStyle* style;

	g_return_if_fail(w != NULL);

	/* Copy the selected color to active.  Note that the modify calls will cause
	recursive calls to this function after the value is updated and w->style to
	be set to a new object */
	style = gtk_widget_get_style(w);
	if (style == NULL)
		return;
	if (!gdk_color_equal(&style->base[GTK_STATE_SELECTED], &style->base[GTK_STATE_ACTIVE]))
		gtk_widget_modify_base(w, GTK_STATE_ACTIVE, &style->base[GTK_STATE_SELECTED]);

	style = gtk_widget_get_style(w);
	if (style == NULL)
		return;
	if (!gdk_color_equal(&style->text[GTK_STATE_SELECTED], &style->text[GTK_STATE_ACTIVE]))
		gtk_widget_modify_text(w, GTK_STATE_ACTIVE, &style->text[GTK_STATE_SELECTED]);
}

void ListBoxX::Create(Window &, int, Point, int, bool) {
	wid = gtk_window_new(GTK_WINDOW_POPUP);

	GtkWidget *frame = gtk_frame_new(NULL);
	gtk_widget_show(frame);
	gtk_container_add(GTK_CONTAINER(GetID()), frame);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 0);

	scroller = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(scroller), 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
	                               GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(frame), PWidget(scroller));
	gtk_widget_show(PWidget(scroller));

	/* Tree and its model */
	GtkListStore *store =
		gtk_list_store_new(N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING);

	list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_signal_connect(G_OBJECT(list), "style-set", G_CALLBACK(StyleSet), NULL);

	GtkTreeSelection *selection =
		gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list), FALSE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(list), FALSE);

	/* Columns */
	GtkTreeViewColumn *column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_title(column, "Autocomplete");

	pixbuf_renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_renderer_set_fixed_size(pixbuf_renderer, 0, -1);
	gtk_tree_view_column_pack_start(column, pixbuf_renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, pixbuf_renderer,
										"pixbuf", PIXBUF_COLUMN);

	GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer,
										"text", TEXT_COLUMN);

	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
	if (g_object_class_find_property(G_OBJECT_GET_CLASS(list), "fixed-height-mode"))
		g_object_set(G_OBJECT(list), "fixed-height-mode", TRUE, NULL);

	GtkWidget *wid = PWidget(list);	// No code inside the G_OBJECT macro
	gtk_container_add(GTK_CONTAINER(PWidget(scroller)), wid);
	gtk_widget_show(wid);
	g_signal_connect(G_OBJECT(wid), "button_press_event",
	                   G_CALLBACK(ButtonPress), this);
	gtk_widget_realize(PWidget(wid));
}

void ListBoxX::SetFont(Font &scint_font) {
	// Only do for Pango font as there have been crashes for GDK fonts
	if (Created() && PFont(scint_font)->pfd) {
		// Current font is Pango font
		gtk_widget_modify_font(PWidget(list), PFont(scint_font)->pfd);
	}
}

void ListBoxX::SetAverageCharWidth(int width) {
	aveCharWidth = width;
}

void ListBoxX::SetVisibleRows(int rows) {
	desiredVisibleRows = rows;
}

int ListBoxX::GetVisibleRows() const {
	return desiredVisibleRows;
}

PRectangle ListBoxX::GetDesiredRect() {
	// Before any size allocated pretend its 100 wide so not scrolled
	PRectangle rc(0, 0, 100, 100);
	if (wid) {
		int rows = Length();
		if ((rows == 0) || (rows > desiredVisibleRows))
			rows = desiredVisibleRows;

		GtkRequisition req;
		int height;

		// First calculate height of the clist for our desired visible
		// row count otherwise it tries to expand to the total # of rows
		// Get cell height
		int row_width=0;
		int row_height=0;
		GtkTreeViewColumn * column =
			gtk_tree_view_get_column(GTK_TREE_VIEW(list), 0);
		gtk_tree_view_column_cell_get_size(column, NULL,
			NULL, NULL, &row_width, &row_height);
		int ythickness = PWidget(list)->style->ythickness;
		height = (rows * row_height
		          + 2 * (ythickness
		                 + GTK_CONTAINER(PWidget(list))->border_width + 1));
		gtk_widget_set_size_request(GTK_WIDGET(PWidget(list)), -1, height);

		// Get the size of the scroller because we set usize on the window
		gtk_widget_size_request(GTK_WIDGET(scroller), &req);
		rc.right = req.width;
		rc.bottom = req.height;

		gtk_widget_set_size_request(GTK_WIDGET(list), -1, -1);
		int width = maxItemCharacters;
		if (width < 12)
			width = 12;
		rc.right = width * (aveCharWidth + aveCharWidth / 3);
		if (Length() > rows)
			rc.right = rc.right + 16;
	}
	return rc;
}

int ListBoxX::CaretFromEdge() {
	gint renderer_width, renderer_height;
	gtk_cell_renderer_get_fixed_size(pixbuf_renderer, &renderer_width,
						&renderer_height);
	return 4 + renderer_width;
}

void ListBoxX::Clear() {
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	gtk_list_store_clear(GTK_LIST_STORE(model));
	maxItemCharacters = 0;
}

static void init_pixmap(ListImage *list_image) {
	const char *textForm = list_image->xpm_data;
	const char * const * xpm_lineform = reinterpret_cast<const char * const *>(textForm);
	const char **xpm_lineformfromtext = 0;
	// The XPM data can be either in atext form as will be read from a file
	// or in a line form (array of char  *) as will be used for images defined in code.
	// Test for text form and convert to line form
	if ((0 == memcmp(textForm, "/* X", 4)) && (0 == memcmp(textForm, "/* XPM */", 9))) {
		// Test done is two parts to avoid possibility of overstepping the memory
		// if memcmp implemented strangely. Must be 4 bytes at least at destination.
		xpm_lineformfromtext = XPM::LinesFormFromTextForm(textForm);
		xpm_lineform = xpm_lineformfromtext;
	}

	// Drop any existing pixmap/bitmap as data may have changed
	if (list_image->pixbuf)
		g_object_unref(list_image->pixbuf);
	list_image->pixbuf =
		gdk_pixbuf_new_from_xpm_data((const gchar**)xpm_lineform);
	delete []xpm_lineformfromtext;
}

#define SPACING 5

void ListBoxX::Append(char *s, int type) {
	ListImage *list_image = NULL;
	if ((type >= 0) && pixhash) {
		list_image = (ListImage *) g_hash_table_lookup((GHashTable *) pixhash
		             , (gconstpointer) GINT_TO_POINTER(type));
	}
	GtkTreeIter iter;
	GtkListStore *store =
		GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));
	gtk_list_store_append(GTK_LIST_STORE(store), &iter);
	if (list_image) {
		if (NULL == list_image->pixbuf)
			init_pixmap(list_image);
		if (list_image->pixbuf) {
			gtk_list_store_set(GTK_LIST_STORE(store), &iter,
								PIXBUF_COLUMN, list_image->pixbuf,
								TEXT_COLUMN, s, -1);

			gint pixbuf_width = gdk_pixbuf_get_width(list_image->pixbuf);
			gint renderer_height, renderer_width;
			gtk_cell_renderer_get_fixed_size(pixbuf_renderer,
								&renderer_width, &renderer_height);
			if (pixbuf_width > renderer_width)
				gtk_cell_renderer_set_fixed_size(pixbuf_renderer,
								pixbuf_width, -1);
		} else {
			gtk_list_store_set(GTK_LIST_STORE(store), &iter,
								TEXT_COLUMN, s, -1);
		}
	} else {
			gtk_list_store_set(GTK_LIST_STORE(store), &iter,
								TEXT_COLUMN, s, -1);
	}
	size_t len = strlen(s);
	if (maxItemCharacters < len)
		maxItemCharacters = len;
}

int ListBoxX::Length() {
	if (wid)
		return gtk_tree_model_iter_n_children(gtk_tree_view_get_model
											   (GTK_TREE_VIEW(list)), NULL);
	return 0;
}

void ListBoxX::Select(int n) {
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	GtkTreeSelection *selection =
		gtk_tree_view_get_selection(GTK_TREE_VIEW(list));

	if (n < 0) {
		gtk_tree_selection_unselect_all(selection);
		return;
	}

	bool valid = gtk_tree_model_iter_nth_child(model, &iter, NULL, n) != FALSE;
	if (valid) {
		gtk_tree_selection_select_iter(selection, &iter);

		// Move the scrollbar to show the selection.
		int total = Length();
		GtkAdjustment *adj =
			gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(list));
		gfloat value = ((gfloat)n / total) * (adj->upper - adj->lower)
							+ adj->lower - adj->page_size / 2;

		// Get cell height
		int row_width;
		int row_height;
		GtkTreeViewColumn * column =
			gtk_tree_view_get_column(GTK_TREE_VIEW(list), 0);
		gtk_tree_view_column_cell_get_size(column, NULL, NULL,
											NULL, &row_width, &row_height);

		int rows = Length();
		if ((rows == 0) || (rows > desiredVisibleRows))
			rows = desiredVisibleRows;
		if (rows & 0x1) {
			// Odd rows to display -- We are now in the middle.
			// Align it so that we don't chop off rows.
			value += (gfloat)row_height / 2.0;
		}
		// Clamp it.
		value = (value < 0)? 0 : value;
		value = (value > (adj->upper - adj->page_size))?
					(adj->upper - adj->page_size) : value;

		// Set it.
		gtk_adjustment_set_value(adj, value);
	} else {
		gtk_tree_selection_unselect_all(selection);
	}
}

int ListBoxX::GetSelection() {
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		int *indices = gtk_tree_path_get_indices(path);
		// Don't free indices.
		if (indices)
			return indices[0];
	}
	return -1;
}

int ListBoxX::Find(const char *prefix) {
	GtkTreeIter iter;
	GtkTreeModel *model =
		gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	bool valid = gtk_tree_model_get_iter_first(model, &iter) != FALSE;
	int i = 0;
	while(valid) {
		gchar *s;
		gtk_tree_model_get(model, &iter, TEXT_COLUMN, &s, -1);
		if (s && (0 == strncmp(prefix, s, strlen(prefix)))) {
			g_free(s);
			return i;
		}
		g_free(s);
		valid = gtk_tree_model_iter_next(model, &iter) != FALSE;
		i++;
	}
	return -1;
}

void ListBoxX::GetValue(int n, char *value, int len) {
	char *text = NULL;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	bool valid = gtk_tree_model_iter_nth_child(model, &iter, NULL, n) != FALSE;
	if (valid) {
		gtk_tree_model_get(model, &iter, TEXT_COLUMN, &text, -1);
	}
	if (text && len > 0) {
		strncpy(value, text, len);
		value[len - 1] = '\0';
	} else {
		value[0] = '\0';
	}
	g_free(text);
}

// g_return_if_fail causes unnecessary compiler warning in release compile.
#ifdef _MSC_VER
#pragma warning(disable: 4127)
#endif

void ListBoxX::RegisterImage(int type, const char *xpm_data) {
	g_return_if_fail(xpm_data);

	// Saved and use the saved copy so caller's copy can disappear.
	xset.Add(type, xpm_data);
	XPM *pxpm = xset.Get(type);
	xpm_data = reinterpret_cast<const char *>(pxpm->InLinesForm());

	if (!pixhash) {
		pixhash = g_hash_table_new(g_direct_hash, g_direct_equal);
	}
	ListImage *list_image = (ListImage *) g_hash_table_lookup((GHashTable *) pixhash,
		(gconstpointer) GINT_TO_POINTER(type));
	if (list_image) {
		// Drop icon already registered
		if (list_image->pixbuf)
			g_object_unref(list_image->pixbuf);
		list_image->pixbuf = NULL;
		list_image->xpm_data = xpm_data;
	} else {
		list_image = g_new0(ListImage, 1);
		list_image->xpm_data = xpm_data;
		g_hash_table_insert((GHashTable *) pixhash, GINT_TO_POINTER(type),
			(gpointer) list_image);
	}
}

void ListBoxX::ClearRegisteredImages() {
	xset.Clear();
}

void ListBoxX::SetList(const char *listText, char separator, char typesep) {
	Clear();
	int count = strlen(listText) + 1;
	char *words = new char[count];
	if (words) {
		memcpy(words, listText, count);
		char *startword = words;
		char *numword = NULL;
		int i = 0;
		for (; words[i]; i++) {
			if (words[i] == separator) {
				words[i] = '\0';
				if (numword)
					*numword = '\0';
				Append(startword, numword?atoi(numword + 1):-1);
				startword = words + i + 1;
				numword = NULL;
			} else if (words[i] == typesep) {
				numword = words + i;
			}
		}
		if (startword) {
			if (numword)
				*numword = '\0';
			Append(startword, numword?atoi(numword + 1):-1);
		}
		delete []words;
	}
}

Menu::Menu() : mid(0) {}

void Menu::CreatePopUp() {
	Destroy();
	mid = gtk_menu_new();
#if GLIB_CHECK_VERSION(2,10,0)
	 g_object_ref_sink(G_OBJECT(mid));
#else
	g_object_ref(G_OBJECT(mid));
	gtk_object_sink(GTK_OBJECT(G_OBJECT(mid)));
#endif
}

void Menu::Destroy() {
	if (mid)
		g_object_unref(G_OBJECT(mid));
	mid = 0;
}

static void  MenuPositionFunc(GtkMenu *, gint *x, gint *y, gboolean *, gpointer userData) {
	sptr_t intFromPointer = reinterpret_cast<sptr_t>(userData);
	*x = intFromPointer & 0xffff;
	*y = intFromPointer >> 16;
}

void Menu::Show(Point pt, Window &) {
	int screenHeight = gdk_screen_height();
	int screenWidth = gdk_screen_width();
	GtkMenu *widget = reinterpret_cast<GtkMenu *>(mid);
	gtk_widget_show_all(GTK_WIDGET(widget));
	GtkRequisition requisition;
	gtk_widget_size_request(GTK_WIDGET(widget), &requisition);
	if ((pt.x + requisition.width) > screenWidth) {
		pt.x = screenWidth - requisition.width;
	}
	if ((pt.y + requisition.height) > screenHeight) {
		pt.y = screenHeight - requisition.height;
	}
	gtk_menu_popup(widget, NULL, NULL, MenuPositionFunc,
		reinterpret_cast<void *>((pt.y << 16) | pt.x), 0,
		gtk_get_current_event_time());
}

ElapsedTime::ElapsedTime() {
	GTimeVal curTime;
	g_get_current_time(&curTime);
	bigBit = curTime.tv_sec;
	littleBit = curTime.tv_usec;
}

class DynamicLibraryImpl : public DynamicLibrary {
protected:
	GModule* m;
public:
	DynamicLibraryImpl(const char *modulePath) {
		m = g_module_open(modulePath, G_MODULE_BIND_LAZY);
	}

	virtual ~DynamicLibraryImpl() {
		if (m != NULL)
			g_module_close(m);
	}

	// Use g_module_symbol to get a pointer to the relevant function.
	virtual Function FindFunction(const char *name) {
		if (m != NULL) {
			gpointer fn_address = NULL;
			gboolean status = g_module_symbol(m, name, &fn_address);
			if (status)
				return static_cast<Function>(fn_address);
			else
				return NULL;
		} else
			return NULL;
	}

	virtual bool IsValid() {
		return m != NULL;
	}
};

DynamicLibrary *DynamicLibrary::Load(const char *modulePath) {
	return static_cast<DynamicLibrary *>( new DynamicLibraryImpl(modulePath) );
}

double ElapsedTime::Duration(bool reset) {
	GTimeVal curTime;
	g_get_current_time(&curTime);
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

ColourDesired Platform::Chrome() {
	return ColourDesired(0xe0, 0xe0, 0xe0);
}

ColourDesired Platform::ChromeHighlight() {
	return ColourDesired(0xff, 0xff, 0xff);
}

const char *Platform::DefaultFont() {
#ifdef G_OS_WIN32
	return "Lucida Console";
#else
	return "!Sans";
#endif
}

int Platform::DefaultFontSize() {
#ifdef G_OS_WIN32
	return 10;
#else
	return 12;
#endif
}

unsigned int Platform::DoubleClickTime() {
	return 500; 	// Half a second
}

bool Platform::MouseButtonBounce() {
	return true;
}

void Platform::DebugDisplay(const char *s) {
	fprintf(stderr, "%s", s);
}

bool Platform::IsKeyDown(int) {
	// TODO: discover state of keys in GTK+/X
	return false;
}

long Platform::SendScintilla(
    WindowID w, unsigned int msg, unsigned long wParam, long lParam) {
	return scintilla_send_message(SCINTILLA(w), msg, wParam, lParam);
}

long Platform::SendScintillaPointer(
    WindowID w, unsigned int msg, unsigned long wParam, void *lParam) {
	return scintilla_send_message(SCINTILLA(w), msg, wParam,
	                              reinterpret_cast<sptr_t>(lParam));
}

bool Platform::IsDBCSLeadByte(int codePage, char ch) {
	// Byte ranges found in Wikipedia articles with relevant search strings in each case
	unsigned char uch = static_cast<unsigned char>(ch);
	switch (codePage) {
		case 932:
			// Shift_jis
			return ((uch >= 0x81) && (uch <= 0x9F)) ||
				((uch >= 0xE0) && (uch <= 0xEF));
		case 936:
			// GBK
			return (uch >= 0x81) && (uch <= 0xFE);
		case 950:
			// Big5
			return (uch >= 0x81) && (uch <= 0xFE);
		// Korean EUC-KR may be code page 949.
	}
	return false;
}

int Platform::DBCSCharLength(int codePage, const char *s) {
	if (codePage == 932 || codePage == 936 || codePage == 950) {
		return IsDBCSLeadByte(codePage, s[0]) ? 2 : 1;
	} else {
		int bytes = mblen(s, MB_CUR_MAX);
		if (bytes >= 1)
			return bytes;
		else
			return 1;
	}
}

int Platform::DBCSCharMaxLength() {
	return MB_CUR_MAX;
	//return 2;
}

// These are utility functions not really tied to a platform

int Platform::Minimum(int a, int b) {
	if (a < b)
		return a;
	else
		return b;
}

int Platform::Maximum(int a, int b) {
	if (a > b)
		return a;
	else
		return b;
}

//#define TRACE

#ifdef TRACE
void Platform::DebugPrintf(const char *format, ...) {
	char buffer[2000];
	va_list pArguments;
	va_start(pArguments, format);
	vsprintf(buffer, format, pArguments);
	va_end(pArguments);
	Platform::DebugDisplay(buffer);
}
#else
void Platform::DebugPrintf(const char *, ...) {}

#endif

// Not supported for GTK+
static bool assertionPopUps = true;

bool Platform::ShowAssertionPopUps(bool assertionPopUps_) {
	bool ret = assertionPopUps;
	assertionPopUps = assertionPopUps_;
	return ret;
}

void Platform::Assert(const char *c, const char *file, int line) {
	char buffer[2000];
	sprintf(buffer, "Assertion [%s] failed at %s %d", c, file, line);
	strcat(buffer, "\r\n");
	Platform::DebugDisplay(buffer);
	abort();
}

int Platform::Clamp(int val, int minVal, int maxVal) {
	if (val > maxVal)
		val = maxVal;
	if (val < minVal)
		val = minVal;
	return val;
}

void Platform_Initialise() {
	FontMutexAllocate();
}

void Platform_Finalise() {
	FontMutexFree();
}
