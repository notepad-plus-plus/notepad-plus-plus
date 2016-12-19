// Scintilla source code edit control
// Converter.h - Encapsulation of iconv
// Copyright 2004 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CONVERTER_H
#define CONVERTER_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

typedef GIConv ConverterHandle;
const ConverterHandle iconvhBad = (ConverterHandle)(-1);
// Since various versions of iconv can not agree on whether the src argument
// is char ** or const char ** provide a templatised adaptor.
template<typename T>
size_t iconv_adaptor(size_t(*f_iconv)(ConverterHandle, T, size_t *, char **, size_t *),
		ConverterHandle cd, char** src, size_t *srcleft,
		char **dst, size_t *dstleft) {
	return f_iconv(cd, (T)src, srcleft, dst, dstleft);
}
/**
 * Encapsulate iconv safely and avoid iconv_adaptor complexity in client code.
 */
class Converter {
	ConverterHandle iconvh;
	void OpenHandle(const char *fullDestination, const char *charSetSource) {
		iconvh = g_iconv_open(fullDestination, charSetSource);
	}
	bool Succeeded() const {
		return iconvh != iconvhBad;
	}
public:
	Converter() {
		iconvh = iconvhBad;
	}
	Converter(const char *charSetDestination, const char *charSetSource, bool transliterations) {
		iconvh = iconvhBad;
	    	Open(charSetDestination, charSetSource, transliterations);
	}
	~Converter() {
		Close();
	}
	operator bool() const {
		return Succeeded();
	}
	void Open(const char *charSetDestination, const char *charSetSource, bool transliterations=true) {
		Close();
		if (*charSetSource) {
			// Try allowing approximate transliterations
			if (transliterations) {
				char fullDest[200];
				g_strlcpy(fullDest, charSetDestination, sizeof(fullDest));
				g_strlcat(fullDest, "//TRANSLIT", sizeof(fullDest));
				OpenHandle(fullDest, charSetSource);
			}
			if (!Succeeded()) {
				// Transliterations failed so try basic name
				OpenHandle(charSetDestination, charSetSource);
			}
		}
	}
	void Close() {
		if (Succeeded()) {
			g_iconv_close(iconvh);
			iconvh = iconvhBad;
		}
	}
	size_t Convert(char** src, size_t *srcleft, char **dst, size_t *dstleft) const {
		if (!Succeeded()) {
			return (size_t)(-1);
		} else {
			return iconv_adaptor(g_iconv, iconvh, src, srcleft, dst, dstleft);
		}
	}
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
