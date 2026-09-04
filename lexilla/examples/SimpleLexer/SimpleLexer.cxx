//  A simple lexer
/** @file SimpleLexer.cxx
 ** A lexer that follows the Lexilla protocol to allow it to be used from Lexilla clients like SciTE.
 ** The lexer applies alternating styles (0,1) to bytes of the text.
 **/
// Copyright 2021 by Neil Hodgson <neilh@scintilla.org>
// This file is in the public domain.
// If the public domain is not possible in your location then it can also be used under the same
// license as Scintilla. https://www.scintilla.org/License.txt

// Windows/MSVC
// cl -std:c++17 -EHsc -LD -I ../../../scintilla/include -I ../../include -I ../../lexlib SimpleLexer.cxx ../../lexlib/*.cxx

// macOS/clang
// clang++ -dynamiclib --std=c++17 -I ../../../scintilla/include -I ../../include -I ../../lexlib SimpleLexer.cxx ../../lexlib/*.cxx -o SimpleLexer.dylib

// Linux/g++
// g++ -fPIC -shared --std=c++17 -I ../../../scintilla/include -I ../../include -I ../../lexlib SimpleLexer.cxx ../../lexlib/*.cxx -o SimpleLexer.so

/* It can be demonstrated in SciTE like this, substituting the actual shared library location as lexilla.path:
lexilla.path=.;C:\u\hg\lexilla\examples\SimpleLexer\SimpleLexer.dll
lexer.*.xx=simple
style.simple.1=fore:#FF0000
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <string>
#include <string_view>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"
// Lexilla.h should not be included here as it declares statically linked functions without the __declspec( dllexport )

#include "WordList.h"
#include "PropSetSimple.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "LexerBase.h"

using namespace Scintilla;
using namespace Lexilla;

class LexerSimple : public LexerBase {
public:
        LexerSimple() {
        }
        void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override {
                try {
			Accessor astyler(pAccess, &props);
			if (length > 0) {
				astyler.StartAt(startPos);
				astyler.StartSegment(startPos);
				for (unsigned int k=0; k<length; k++) {
					astyler.ColourTo(startPos+k, (startPos+k)%2);
				}
			}
                        astyler.Flush();
                } catch (...) {
                        // Should not throw into caller as may be compiled with different compiler or options
                        pAccess->SetErrorStatus(SC_STATUS_FAILURE);
                }
        }
        void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override {
        }

        static ILexer5 *LexerFactorySimple() {
                try {
                        return new LexerSimple();
                } catch (...) {
                        // Should not throw into caller as may be compiled with different compiler or options
                        return nullptr;
                }
        }
};

#if defined(_WIN32)
#define EXPORT_FUNCTION __declspec(dllexport)
#define CALLING_CONVENTION __stdcall
#else
#define EXPORT_FUNCTION __attribute__((visibility("default")))
#define CALLING_CONVENTION
#endif

static const char *lexerName = "simple";

extern "C" {

EXPORT_FUNCTION int CALLING_CONVENTION GetLexerCount() {
        return 1;
}

EXPORT_FUNCTION void CALLING_CONVENTION GetLexerName(unsigned int index, char *name, int buflength) {
        *name = 0;
        if ((index == 0) && (buflength > static_cast<int>(strlen(lexerName)))) {
                strcpy(name, lexerName);
        }
}

EXPORT_FUNCTION LexerFactoryFunction CALLING_CONVENTION GetLexerFactory(unsigned int index) {
        if (index == 0)
                return LexerSimple::LexerFactorySimple;
        else
                return 0;
}

EXPORT_FUNCTION Scintilla::ILexer5* CALLING_CONVENTION CreateLexer(const char *name) {
	if (0 == strcmp(name, lexerName)) {
		return LexerSimple::LexerFactorySimple();
	}
	return nullptr;
}

EXPORT_FUNCTION const char * CALLING_CONVENTION GetNameSpace() {
	return "example";
}

}
