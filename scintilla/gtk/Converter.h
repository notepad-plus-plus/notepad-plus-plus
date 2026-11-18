// Scintilla source code edit control
// Converter.h - Encapsulation of iconv
// Copyright 2004 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CONVERTER_H
#define CONVERTER_H

namespace Scintilla {

const GIConv iconvhBad = (GIConv)(-1);
const gsize sizeFailure = static_cast<gsize>(-1);
/**
 * Encapsulate g_iconv safely.
 */
class Converter {
	GIConv iconvh = iconvhBad;
	void OpenHandle(const char *fullDestination, const char *charSetSource) noexcept {
		iconvh = g_iconv_open(fullDestination, charSetSource);
	}
	bool Succeeded() const noexcept {
		return iconvh != iconvhBad;
	}
public:
	Converter() noexcept = default;
	Converter(const char *charSetDestination, const char *charSetSource, bool transliterations) {
		Open(charSetDestination, charSetSource, transliterations);
	}
	// Deleted so Converter objects can not be copied.
	Converter(const Converter &) = delete;
	Converter(Converter &&) = delete;
	Converter &operator=(const Converter &) = delete;
	Converter &operator=(Converter &&) = delete;
	~Converter() {
		Close();
	}
	operator bool() const noexcept {
		return Succeeded();
	}
	void Open(const char *charSetDestination, const char *charSetSource, bool transliterations) {
		Close();
		if (*charSetSource) {
			// Try allowing approximate transliterations
			if (transliterations) {
				std::string fullDest(charSetDestination);
				fullDest.append("//TRANSLIT");
				OpenHandle(fullDest.c_str(), charSetSource);
			}
			if (!Succeeded()) {
				// Transliterations failed so try basic name
				OpenHandle(charSetDestination, charSetSource);
			}
		}
	}
	void Close() noexcept {
		if (Succeeded()) {
			g_iconv_close(iconvh);
			iconvh = iconvhBad;
		}
	}
	gsize Convert(char **src, gsize *srcleft, char **dst, gsize *dstleft) const noexcept {
		if (!Succeeded()) {
			return sizeFailure;
		} else {
			return g_iconv(iconvh, src, srcleft, dst, dstleft);
		}
	}
};

}

#endif
