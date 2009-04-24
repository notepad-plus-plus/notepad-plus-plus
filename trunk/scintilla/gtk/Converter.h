// Scintilla source code edit control
// Converter.h - Encapsulation of iconv
// Copyright 2004 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <iconv.h>
#if GTK_MAJOR_VERSION >= 2
	typedef GIConv ConverterHandle;
#else
	typedef iconv_t ConverterHandle;
#endif
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
#if GTK_MAJOR_VERSION >= 2
		iconvh = g_iconv_open(fullDestination, charSetSource);
#else
		iconvh = iconv_open(fullDestination, charSetSource);
#endif
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
				strcpy(fullDest, charSetDestination);
				strcat(fullDest, "//TRANSLIT");
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
#if GTK_MAJOR_VERSION >= 2
			g_iconv_close(iconvh);
#else
			iconv_close(iconvh);
#endif
			iconvh = iconvhBad;
		}
	}
	size_t Convert(char** src, size_t *srcleft, char **dst, size_t *dstleft) const {
		if (!Succeeded()) {
			return (size_t)(-1);
		} else {
#if GTK_MAJOR_VERSION >= 2
			return iconv_adaptor(g_iconv, iconvh, src, srcleft, dst, dstleft);
#else
			return iconv_adaptor(iconv, iconvh, src, srcleft, dst, dstleft);
#endif
		}
	}
};
