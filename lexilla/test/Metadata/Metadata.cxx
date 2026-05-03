// Lexilla metadata list
/** @file Metadata.cxx
 ** List metadata of Lexilla shared library or check that it is the same as before
 **/
// Copyright 2026 by Neil Hodgson <neilh@scintilla.org>
// This file is in the public domain.
// If the public domain is not possible in your location then it can also be used under the same
// license as Scintilla. https://www.scintilla.org/License.txt

/*

Build

    All platforms with g++ or clang
        make

    Win32 Visual C++
        cl Metadata.cxx ../../access/LexillaAccess.cxx -EHsc -std:c++20 -I ../../include -I ../../access -I ../../../scintilla/include -Fe: Metadata

Using

    List metadata of standard Lexilla shared library in lexilla/bin.
        Metadata

    List metadata of particular shared library conforming to Lexilla protocol
        Metadata ../../examples/SimpleLexer/SimpleLexer.dll

    Check that metadata of standard Lexilla is the same as lexerMetadata.txt.
    If different, new version placed in lexerMetadata.txt.new.
        Metadata -check

*/

#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

#include "ILexer.h"
#include "Scintilla.h"
#include "Lexilla.h"

#include "LexillaAccess.h"

using namespace Lexilla;

namespace {

std::vector<std::string> StringSplit(const std::string_view &text, int separator) {
	std::vector<std::string> vs(text.empty() ? 0 : 1);
	for (const char ch : text) {
		if (ch == separator) {
			vs.push_back(std::string());
		} else {
			vs.back() += ch;
		}
	}
	return vs;
}

const char *translateType(int n) {
	switch(n) {
		case SC_TYPE_BOOLEAN: return "bool  ";
		case SC_TYPE_INTEGER: return "int   ";
		case SC_TYPE_STRING: return "string";
		default: return "error ";
	}
}

void ShowMetadata(std::ostringstream &os, ILexer5 *lexer) {
	const int lineEnds = lexer->LineEndTypesSupported();
	if (lineEnds == SC_LINE_END_TYPE_UNICODE) {
		os << "    UnicodeLineEnds\n";
	}
	const char *propertyNames = lexer->PropertyNames();
	if (propertyNames && *propertyNames) {
		os << "    Properties:\n";
		std::vector<std::string> properties = StringSplit(propertyNames, '\n');
		for (const auto &p : properties) {
			const char *type = translateType(lexer->PropertyType(p.c_str()));
			const char *description = lexer->DescribeProperty(p.c_str());
			os << "        " << type << " " << p << "\n";
			if (description && *description) {
				os << "               " << description << "\n";
			}
		}
	}
	const char *wordListSets = lexer->DescribeWordListSets();
	if (wordListSets && *wordListSets) {
		os << "    Word Lists:\n";
		std::vector<std::string> wordLists = StringSplit(wordListSets, '\n');
		for (const auto &wl : wordLists) {
			os << "        " << wl << "\n";
		}
	}
	const char *bases = lexer->GetSubStyleBases();
	if (bases && *bases) {
		os << "    Sub-Style Bases:\n";
		while (*bases) {
			os << "        " << std::setw(3) << static_cast<int>(*bases) << "\n";
			bases++;
		}
	}
	const int namedStyles = lexer->NamedStyles();
	if (namedStyles > 0) {
		os << "    Styles:\n";
		for (int s=0; s<namedStyles; s++) {
			const char *nameStyle = lexer->NameOfStyle(s);
			const char *tags = lexer->TagsOfStyle(s);
			const char *description = lexer->DescriptionOfStyle(s);
			if (*nameStyle || *tags) {
				os << "        " << std::setw(3) << s << " " << nameStyle << " [" << tags << "] " << description << "\n";
			}
		}
	}
}

void ShowAllMetadata(std::ostringstream &os, const std::vector<std::string> &lexers) {
	for (const auto &name : lexers) {
		ILexer5 *lexer = MakeLexer(name);
		if (lexer) {
			os << "\nLexer " << name << "\n";
			ShowMetadata(os, lexer);
			lexer->Release();
		} else {
			os << "\n  " << name << " Missing\n";
		}
	}
	os << "\n";
}

}

int main(int argc, char *argv[]) {
	const char szLexillaPath[] = "../../bin/" LEXILLA_LIB LEXILLA_EXTENSION;
	const char *libPath = szLexillaPath;
	bool checking = false;
	if (argc > 1) {
		for (int arg=1; arg<argc; arg++) {
			if (argv[arg][0] == '-') {
				if (std::string_view(argv[arg]) == "-check") {
					checking = true;
				}
			} else {
				libPath = argv[arg];
			}
		}
	}
	if (!Load(libPath)) {
		std::cerr << "Could not load " << libPath << "\n";
		return 1;
	}
	const std::vector<std::string> lexers = Lexers();
	if (!lexers.empty()) {
		std::ostringstream os(std::ios::binary);
		os << "There are " << lexers.size() << " lexers.\n";
		for (const auto &name : lexers) {
			os << "    " << name << "\n";
		}
		os << "\n";

		ShowAllMetadata(os, lexers);

		const std::string sMetadata = os.str();
		if (checking) {
			std::ifstream ifs("lexerMetadata.txt");
			std::string content((std::istreambuf_iterator<char>(ifs)),
				(std::istreambuf_iterator<char>()));
			if (content != sMetadata) {
				std::cerr << "Different metadata\n";
				std::ofstream ofs("lexerMetadata.txt.new");
				ofs << sMetadata;
				return 1;
			}
			return 0;
		} else {
			std::cout << sMetadata;
		}
	}
}
