// Lexilla lexer library
/** @file TestLexers.cxx
 ** Test lexers through Lexilla.
 **/
 // Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
 // The License.txt file describes the conditions under which this software may be distributed.

#include <cassert>

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <algorithm>

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <filesystem>

#include "ILexer.h"

#include "Lexilla.h"
#include "LexillaAccess.h"

#include "TestDocument.h"

namespace {

constexpr std::string_view suffixStyled = ".styled";
constexpr std::string_view suffixFolded = ".folded";

std::string ReadFile(std::filesystem::path path) {
	std::ifstream ifs(path, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(ifs)),
		(std::istreambuf_iterator<char>()));
	return content;
}

std::string MarkedDocument(const Scintilla::IDocument *pdoc) {
	std::ostringstream os(std::ios::binary);
	char prevStyle = -1;
	for (Sci_Position pos = 0; pos < pdoc->Length(); pos++) {
		const char styleNow = pdoc->StyleAt(pos);
		if (styleNow != prevStyle) {
			os << "{" << static_cast<unsigned int>(styleNow) << "}";
			prevStyle = styleNow;
		}
		char ch = '\0';
		pdoc->GetCharRange(&ch, pos, 1);
		os << ch;
	}
	return os.str();
}

void PrintLevel(std::ostringstream &os, int level) {
	const int levelNow = level & 0xFFF;
	const int levelNext = level >> 16;
	const int levelFlags = (level >> 12) & 0xF;
	char foldSymbol = ' ';
	if (level & 0x2000)
		foldSymbol = '+';
	else if (levelNow > 0x400)
		foldSymbol = '|';
	os << std::hex << " " << levelFlags << " "
		<< std::setw(3) << levelNow << " "
		<< std::setw(3) << levelNext << " "
		<< foldSymbol << " ";
}

std::string FoldedDocument(const Scintilla::IDocument *pdoc) {
	std::ostringstream os(std::ios::binary);
	Sci_Position linePrev = -1;
	char ch = '\0';
	for (Sci_Position pos = 0; pos < pdoc->Length(); pos++) {
		const Sci_Position lineNow = pdoc->LineFromPosition(pos);
		if (linePrev < lineNow) {
			PrintLevel(os, pdoc->GetLevel(lineNow));
			linePrev = lineNow;
		}
		pdoc->GetCharRange(&ch, pos, 1);
		os << ch;
	}
	if (ch == '\n') {
		// Extra empty line
		PrintLevel(os, pdoc->GetLevel(linePrev + 1));
	}
	return os.str();
}

std::pair<std::string, std::string> MarkedAndFoldedDocument(const Scintilla::IDocument *pdoc) {
	return { MarkedDocument(pdoc), FoldedDocument(pdoc) };
}

class PropertyMap {
public:
	using PropMap = std::map<std::string, std::string>;
	PropMap properties;

	void ReadFromFile(std::filesystem::path path) {
		std::ifstream ifs(path);
		std::string line;
		std::string logicalLine;
		while (std::getline(ifs, line)) {
			if (line.ends_with("\r")) {
				// Accidentally have \r\n line ends on Unix system
				line.pop_back();
			}
			logicalLine += line;
			if (logicalLine.ends_with("\\")) {
				logicalLine.pop_back();
			} else {
				const size_t positionEquals = logicalLine.find("=");
				if (positionEquals != std::string::npos) {
					const std::string key = logicalLine.substr(0, positionEquals);
					const std::string value = logicalLine.substr(positionEquals + 1);
					properties[key] = value;
				}
				logicalLine.clear();
			}
		}
	}

	std::optional<std::string> GetProperty(std::string_view key) const {
		const PropMap::const_iterator prop = properties.find(std::string(key));
		if (prop == properties.end())
			return std::nullopt;
		else
			return prop->second;
	}

	std::optional<std::string> GetPropertyForFile(std::string_view keyPrefix, std::string_view fileName) const {
		for (auto const &[key, val] : properties) {
			if (key.starts_with(keyPrefix)) {
				const std::string keySuffix = key.substr(keyPrefix.length());
				if (fileName.ends_with(keySuffix)) {
					return val;
				}
			}
		}
		return std::nullopt;
	}

	std::optional<int> GetPropertyValue(std::string_view key) const {
		std::optional<std::string> value = GetProperty(key);
		try {
			if (value)
				return std::stoi(value->c_str());
		}
		catch (std::invalid_argument &) {
			// Just return empty
		}
		return {};
	}

};

size_t FirstLineDifferent(std::string_view a, std::string_view b) {
	size_t i = 0;
	while (i < std::min(a.size(), b.size()) && a.at(i) == b.at(i)) {
		i++;
	}
	return std::count(a.begin(), a.begin() + i, '\n');
}

bool CheckSame(std::string_view augmentedText, std::string_view augmentedTextNew, std::string_view item, std::string_view suffix, const std::filesystem::path &path) {
	if (augmentedTextNew == augmentedText) {
		return true;
	}
	const size_t lineNumber = FirstLineDifferent(augmentedText, augmentedTextNew) + 1;
	std::cout << "\n" << path.string() << ":" << lineNumber << ":";
	std::cout << " has different " << item << "\n\n";
	std::filesystem::path pathNew = path;
	pathNew += suffix;
	pathNew += ".new";
	std::ofstream ofs(pathNew, std::ios::binary);
	ofs << augmentedTextNew;
	return false;
}

int Substitute(std::string &s, const std::string &sFind, const std::string &sReplace) {
	int c = 0;
	const size_t lenFind = sFind.size();
	const size_t lenReplace = sReplace.size();
	size_t posFound = s.find(sFind);
	while (posFound != std::string::npos) {
		s.replace(posFound, lenFind, sReplace);
		posFound = s.find(sFind, posFound + lenReplace);
		c++;
	}
	return c;
}

int WindowsToUnix(std::string &s) {
	return Substitute(s, "\r\n", "\n");
}

int UnixToWindows(std::string &s) {
	return Substitute(s, "\n", "\r\n");
}

const std::string BOM = "\xEF\xBB\xBF";

void StyleLineByLine(TestDocument &doc, Scintilla::ILexer5 *plex) {
	Scintilla::IDocument *pdoc = &doc;
	const Sci_Position lines = doc.LineFromPosition(doc.Length());
	Sci_Position startLine = 0;
	for (Sci_Position line = 0; line <= lines; line++) {
		const Sci_Position endLine = doc.LineStart(line + 1);
		int styleStart = 0;
		if (startLine > 0)
			styleStart = doc.StyleAt(startLine - 1);
		plex->Lex(startLine, endLine - startLine, styleStart, pdoc);
		plex->Fold(startLine, endLine - startLine, styleStart, pdoc);
		startLine = endLine;
	}
}

void TestCRLF(std::filesystem::path path, const std::string s, Scintilla::ILexer5 *plex, bool disablePerLineTests) {
	// Convert all line ends to \r\n to check if styles change between \r and \n which makes
	// it difficult to test on different platforms when files may have line ends changed.
	std::string text = s;
	WindowsToUnix(text);
	const bool originalIsUnix = text == s;
	std::string textUnix = text;
	UnixToWindows(text);
	TestDocument doc;
	doc.Set(text);
	Scintilla::IDocument *pdoc = &doc;
	plex->Lex(0, pdoc->Length(), 0, pdoc);
	plex->Fold(0, pdoc->Length(), 0, pdoc);
	const auto [styledText, foldedText] = MarkedAndFoldedDocument(pdoc);

	int prevStyle = -1;
	Sci_Position line = 1;
	for (Sci_Position pos = 0; pos < pdoc->Length(); pos++) {
		const int styleNow = pdoc->StyleAt(pos);
		char ch = '\0';
		pdoc->GetCharRange(&ch, pos, 1);
		if (ch == '\n') {
			if (styleNow != prevStyle) {
				std::cout << path.string() << ":" << line << ":" <<
					" different styles between \\r and \\n at " <<
					pos << ": " << prevStyle << ", " << styleNow << "\n";
			}
			line++;
		}
		prevStyle = styleNow;
	}

	// Lex and fold with \n line ends then check result is same

	TestDocument docUnix;
	docUnix.Set(textUnix);
	Scintilla::IDocument *pdocUnix = &docUnix;
	plex->Lex(0, pdocUnix->Length(), 0, pdocUnix);
	plex->Fold(0, pdocUnix->Length(), 0, pdocUnix);
	auto [styledTextUnix, foldedTextUnix] = MarkedAndFoldedDocument(pdocUnix);

	// Convert results from \n to \r\n run
	UnixToWindows(styledTextUnix);
	UnixToWindows(foldedTextUnix);

	if (styledText != styledTextUnix) {
		std::cout << "\n" << path.string() << ":1: has different styles with \\n versus \\r\\n line ends\n\n";
	}
	if (foldedText != foldedTextUnix) {
		std::cout << "\n" << path.string() << ":1: has different folds with \\n versus \\r\\n line ends\n\n";
	}

	// Test line by line lexing/folding with Unix \n line ends
	if (!disablePerLineTests && !originalIsUnix) {
		StyleLineByLine(docUnix, plex);
		auto [styledTextNewPerLine, foldedTextNewPerLine] = MarkedAndFoldedDocument(pdocUnix);
		// Convert results from \n to \r\n run
		UnixToWindows(styledTextNewPerLine);
		UnixToWindows(foldedTextNewPerLine);
		CheckSame(styledTextUnix, styledTextNewPerLine, "per-line styles \\n", suffixStyled, path);
		CheckSame(foldedTextUnix, foldedTextNewPerLine, "per-line folds \\n", suffixFolded, path);
	}

	plex->Release();
}

void TestILexer(Scintilla::ILexer5 *plex) {
	// Test each method of the ILexer interface.
	// Mostly ensures there are no crashes when calling methods.
	// Some methods are tested later (Release, Lex, Fold).
	// PrivateCall performs arbitrary actions so is not safe to call.

	[[maybe_unused]] const int version = plex->Version();
	assert(version == Scintilla::lvRelease5);

	[[maybe_unused]] const char *language = plex->GetName();
	assert(language);

	[[maybe_unused]] const int ident = plex->GetIdentifier();
	assert(ident >= 0);

	[[maybe_unused]] const char *propertyNames = plex->PropertyNames();
	assert(propertyNames);

	[[maybe_unused]] const int propertyType = plex->PropertyType("unknown");
	assert(propertyType >= 0 && propertyType <= 2);

	[[maybe_unused]] const char *propertyDescription = plex->DescribeProperty("unknown");
	assert(propertyDescription);

	[[maybe_unused]] const Sci_Position invalidation = plex->PropertySet("unknown", "unknown");
	assert(invalidation == 0 || invalidation == -1);

	[[maybe_unused]] const char *wordListDescription = plex->DescribeWordListSets();
	assert(wordListDescription);

	[[maybe_unused]] const Sci_Position invalidationWordList = plex->WordListSet(9, "unknown");
	assert(invalidationWordList == 0 || invalidationWordList == -1);

	[[maybe_unused]] const int lineEndTypes = plex->LineEndTypesSupported();
	assert(lineEndTypes == 0 || lineEndTypes == 1);

	if (const char *bases = plex->GetSubStyleBases()) {
		// Allocate a substyle for each possible style
		while (*bases) {
			constexpr int newStyles = 3;
			const int base = *bases;
			const int baseStyle = plex->AllocateSubStyles(base, newStyles);
			[[maybe_unused]] const int styleBack = plex->StyleFromSubStyle(baseStyle);
			assert(styleBack == base);
			plex->SetIdentifiers(baseStyle, "int nullptr");
			[[maybe_unused]] const int start = plex->SubStylesStart(base);
			assert(start == baseStyle);
			[[maybe_unused]] const int len = plex->SubStylesLength(base);
			assert(len == newStyles);
			bases++;
		}
		plex->FreeSubStyles();
	}

	[[maybe_unused]] const int primary = plex->PrimaryStyleFromStyle(2);
	assert(primary == 2);

	[[maybe_unused]] const int distance = plex->DistanceToSecondaryStyles();
	assert(distance >= 0);

	// Just see if crashes - nullptr is valid return to indicate not present.
	[[maybe_unused]] const char *propertyUnknownValue = plex->PropertyGet("unknown");

	const int styles = plex->NamedStyles();
	for (int style = 0; style < styles; style++) {
		[[maybe_unused]] const char *name = plex->NameOfStyle(style);
		assert(name);
		[[maybe_unused]] const char *tags = plex->TagsOfStyle(style);
		assert(tags);
		[[maybe_unused]] const char *description = plex->DescriptionOfStyle(style);
		assert(description);
	}
}

void SetProperties(Scintilla::ILexer5 *plex, const PropertyMap &propertyMap, std::string_view fileName) {
	// Set keywords, keywords2, ... keywords9, for this file
	for (int kw = 0; kw < 10; kw++) {
		std::string kwChoice("keywords");
		if (kw > 0) {
			kwChoice.push_back('1' + kw);
		}
		kwChoice.append(".*");
		std::optional<std::string> keywordN = propertyMap.GetPropertyForFile(kwChoice, fileName);
		if (keywordN) {
			plex->WordListSet(kw, keywordN->c_str());
		}
	}

	// Set parameters of lexer
	for (auto const &[key, val] : propertyMap.properties) {
		if (key.starts_with("#")) {
			// Ignore comments
		} else if (key.starts_with("lexer.*")) {
			// Ignore as processed earlier
		} else if (key.starts_with("keywords")) {
			// Ignore as processed earlier
		} else {
			plex->PropertySet(key.c_str(), val.c_str());
		}
	}
}

const char *lexerPrefix = "lexer.*";

bool TestFile(const std::filesystem::path &path, const PropertyMap &propertyMap) {
	// Find and create correct lexer
	std::optional<std::string> language = propertyMap.GetPropertyForFile(lexerPrefix, path.filename().string());
	if (!language) {
		std::cout << "\n" << path.string() << ":1: has no language\n\n";
		return false;
	}
	Scintilla::ILexer5 *plex = Lexilla::MakeLexer(*language);
	if (!plex) {
		std::cout << "\n" << path.string() << ":1: has no lexer for " << *language << "\n\n";
		return false;
	}

	SetProperties(plex, propertyMap, path.filename().string());

	TestILexer(plex);

	std::string text = ReadFile(path);
	if (text.starts_with(BOM)) {
		text.erase(0, BOM.length());
	}

	std::filesystem::path pathStyled = path;
	pathStyled += suffixStyled;
	const std::string styledText = ReadFile(pathStyled);

	std::filesystem::path pathFolded = path;
	pathFolded += suffixFolded;
	const std::string foldedText = ReadFile(pathFolded);

	const int repeatLex = propertyMap.GetPropertyValue("testlexers.repeat.lex").value_or(1);
	const int repeatFold = propertyMap.GetPropertyValue("testlexers.repeat.fold").value_or(1);

	TestDocument doc;
	doc.Set(text);
	Scintilla::IDocument *pdoc = &doc;
	for (int i = 0; i < repeatLex; i++) {
		plex->Lex(0, pdoc->Length(), 0, pdoc);
	}
	for (int i = 0; i < repeatFold; i++) {
		plex->Fold(0, pdoc->Length(), 0, pdoc);
	}

	bool success = true;

	const auto [styledTextNew, foldedTextNew] = MarkedAndFoldedDocument(pdoc);
	if (!CheckSame(styledText, styledTextNew, "styles", suffixStyled, path)) {
		success = false;
	}
	if (!CheckSame(foldedText, foldedTextNew, "folds", suffixFolded, path)) {
		success = false;
	}

	const std::optional<int> perLineDisable = propertyMap.GetPropertyValue("testlexers.per.line.disable");
	const bool disablePerLineTests = perLineDisable.value_or(false);

	// Test line by line lexing/folding
	if (success && !disablePerLineTests) {
		doc.Set(text);
		StyleLineByLine(doc, plex);
		const auto [styledTextNewPerLine, foldedTextNewPerLine] = MarkedAndFoldedDocument(pdoc);
		success = success && CheckSame(styledText, styledTextNewPerLine, "per-line styles", suffixStyled, path);
		success = success && CheckSame(foldedText, foldedTextNewPerLine, "per-line folds", suffixFolded, path);
	}

	plex->Release();

	Scintilla::ILexer5 *plexCRLF = Lexilla::MakeLexer(*language);
	SetProperties(plexCRLF, propertyMap, path.filename().string());
	TestCRLF(path, text, plexCRLF, disablePerLineTests);

	return success;
}

bool TestDirectory(std::filesystem::path directory, std::filesystem::path basePath) {
	PropertyMap properties;
	properties.ReadFromFile(directory / "SciTE.properties");
	bool success = true;
	for (auto &p : std::filesystem::directory_iterator(directory)) {
		if (!p.is_directory()) {
			const std::string extension = p.path().extension().string();
			if (extension != ".properties" && extension != suffixStyled && extension != ".new" &&
				extension != suffixFolded) {
				const std::filesystem::path relativePath = p.path().lexically_relative(basePath);
				std::cout << "Lexing " << relativePath.string() << '\n';
				if (!TestFile(p, properties)) {
					success = false;
				}
			}
		}
	}
	return success;
}

bool AccessLexilla(std::filesystem::path basePath) {
	if (!std::filesystem::exists(basePath)) {
		std::cout << "No examples at " << basePath.string() << "\n";
		return false;
	}

	bool success = true;
	for (auto &p : std::filesystem::recursive_directory_iterator(basePath)) {
		if (p.is_directory()) {
			//std::cout << p.path().string() << '\n';
			if (!TestDirectory(p, basePath)) {
				success = false;
			}
		}
	}
	return success;
}

std::filesystem::path FindLexillaDirectory(std::filesystem::path startDirectory) {
	// Search up from startDirectory for a directory named "lexilla" or containing a "bin" subdirectory
	std::filesystem::path directory = startDirectory;
	while (!directory.empty()) {
		//std::cout << "Searching " << directory.string() << "\n";
		const std::filesystem::path parent = directory.parent_path();
		const std::filesystem::path localLexilla = directory / "lexilla";
		const std::filesystem::directory_entry entry(localLexilla);
		if (entry.is_directory()) {
			std::cout << "Found Lexilla at " << entry.path().string() << "\n";
			return localLexilla;
		}
		const std::filesystem::path localBin = directory / "bin";
		const std::filesystem::directory_entry entryBin(localBin);
		if (entryBin.is_directory()) {
			std::cout << "Found Lexilla at " << directory.string() << "\n";
			return directory;
		}
		if (parent == directory) {
			std::cout << "Reached root at " << directory.string() << "\n";
			return std::filesystem::path();
		}
		directory = parent;
	}
	return std::filesystem::path();
}

}



int main() {
	bool success = false;
	// TODO: Allow specifying the base directory through a command line argument
	const std::filesystem::path baseDirectory = FindLexillaDirectory(std::filesystem::current_path());
	if (!baseDirectory.empty()) {
		const std::filesystem::path examplesDirectory = baseDirectory / "test" / "examples";
#ifdef LEXILLA_STATIC
		success = AccessLexilla(examplesDirectory);
#else
		const std::filesystem::path sharedLibrary = baseDirectory / "bin" / LEXILLA_LIB;
		if (Lexilla::Load(sharedLibrary.string())) {
			success = AccessLexilla(examplesDirectory);
		} else {
			std::cout << "Failed to load " << sharedLibrary << "\n";
		}
#endif
	}
	return success ? 0 : 1;
}
