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

constexpr char MakeLowerCase(char c) noexcept {
	if (c >= 'A' && c <= 'Z') {
		return c - 'A' + 'a';
	} else {
		return c;
	}
}

[[maybe_unused]] void LowerCaseAZ(std::string &s) {
	std::transform(s.begin(), s.end(), s.begin(), MakeLowerCase);
}

int IntFromString(std::u32string_view s) noexcept {
	if (s.empty()) {
		return 0;
	}
	const bool negate = s.front() == '-';
	if (negate) {
		s.remove_prefix(1);
	}
	int value = 0;
	while (!s.empty()) {
		value = value * 10 + s.front() - '0';
		s.remove_prefix(1);
	}
	return negate ? -value : value;
}

bool PatternMatch(std::u32string_view pattern, std::u32string_view text) noexcept {
	if (pattern == text) {
		return true;
	} else if (pattern.empty()) {
		return false;
	} else if (pattern.front() == '\\') {
		pattern.remove_prefix(1);
		if (pattern.empty()) {
			// Escape with nothing being escaped
			return false;
		}
		if (text.empty()) {
			return false;
		}
		if (pattern.front() == text.front()) {
			pattern.remove_prefix(1);
			text.remove_prefix(1);
			return PatternMatch(pattern, text);
		}
		return false;
	} else if (pattern.front() == '*') {
		pattern.remove_prefix(1);
		if (!pattern.empty() && pattern.front() == '*') {
			pattern.remove_prefix(1);
			// "**" matches anything including "/"
			while (!text.empty()) {
				if (PatternMatch(pattern, text)) {
					return true;
				}
				text.remove_prefix(1);
			}
		} else {
			while (!text.empty()) {
				if (PatternMatch(pattern, text)) {
					return true;
				}
				if (text.front() == '/') {
					// "/" not matched by single "*"
					return false;
				}
				text.remove_prefix(1);
			}
		}
		assert(text.empty());
		// Consumed whole text with wildcard so match if pattern consumed
		return pattern.empty();
	} else if (text.empty()) {
		return false;
	} else if (pattern.front() == '?') {
		if (text.front() == '/') {
			return false;
		}
		pattern.remove_prefix(1);
		text.remove_prefix(1);
		return PatternMatch(pattern, text);
	} else if (pattern.front() == '[') {
		pattern.remove_prefix(1);
		if (pattern.empty()) {
			return false;
		}
		const bool positive = pattern.front() != '!';
		if (!positive) {
			pattern.remove_prefix(1);
			if (pattern.empty()) {
				return false;
			}
		}
		bool inSet = false;
		if (!pattern.empty() && pattern.front() == ']') {
			// First is allowed to be ']'
			if (pattern.front() == text.front()) {
				inSet = true;
			}
			pattern.remove_prefix(1);
		}
		char32_t start = 0;
		while (!pattern.empty() && pattern.front() != ']') {
			if (pattern.front() == '-') {
				pattern.remove_prefix(1);
				if (!pattern.empty()) {
					const char32_t end = pattern.front();
					if ((text.front() >= start) && (text.front() <= end)) {
						inSet = true;
					}
				}
			} else if (pattern.front() == text.front()) {
				inSet = true;
			}
			if (!pattern.empty()) {
				start = pattern.front();
				pattern.remove_prefix(1);
			}
		}
		if (!pattern.empty()) {
			pattern.remove_prefix(1);
		}
		if (inSet != positive) {
			return false;
		}
		text.remove_prefix(1);
		return PatternMatch(pattern, text);
	} else if (pattern.front() == '{') {
		if (pattern.length() < 2) {
			return false;
		}
		const size_t endParen = pattern.find('}');
		if (endParen == std::u32string_view::npos) {
			// Malformed {x} pattern
			return false;
		}
		std::u32string_view parenExpression = pattern.substr(1, endParen - 1);
		bool inSet = false;
		const size_t dotdot = parenExpression.find(U"..");
		if (dotdot != std::u32string_view::npos) {
			// Numeric range: {10..20}
			const std::u32string_view firstRange = parenExpression.substr(0, dotdot);
			const std::u32string_view lastRange = parenExpression.substr(dotdot+2);
			if (firstRange.empty() || lastRange.empty()) {
				// Malformed {s..e} range pattern
				return false;
			}
			const size_t endInteger = text.find_last_of(U"-0123456789");
			if (endInteger == std::u32string_view::npos) {
				// No integer in text
				return false;
			}
			const std::u32string_view intPart = text.substr(0, endInteger+1);
			const int first = IntFromString(firstRange);
			const int last = IntFromString(lastRange);
			const int value = IntFromString(intPart);
			if ((value >= first) && (value <= last)) {
				inSet = true;
				text.remove_prefix(intPart.length());
			}
		} else {
			// Alternates: {a,b,cd}
			size_t comma = parenExpression.find(',');
			for (;;) {
				const bool finalAlt = comma == std::u32string_view::npos;
				const std::u32string_view oneAlt = finalAlt ? parenExpression :
					parenExpression.substr(0, comma);
				if (oneAlt == text.substr(0, oneAlt.length())) {
					// match
					inSet = true;
					text.remove_prefix(oneAlt.length());
					break;
				}
				if (finalAlt) {
					break;
				}
				parenExpression.remove_prefix(oneAlt.length() + 1);
				comma = parenExpression.find(',');
			}
		}
		if (!inSet) {
			return false;
		}
		pattern.remove_prefix(endParen + 1);
		return PatternMatch(pattern, text);
	} else if (pattern.front() == text.front()) {
		pattern.remove_prefix(1);
		text.remove_prefix(1);
		return PatternMatch(pattern, text);
	}
	return false;
}

bool PathMatch(std::string pattern, std::string relPath) {
#if defined(_WIN32)
	// Convert Windows path separators to Unix
	std::replace(relPath.begin(), relPath.end(), '\\', '/');
#endif
#if defined(_WIN32) || defined(__APPLE__)
	// Case-insensitive, only does ASCII but fine for test example files
	LowerCaseAZ(pattern);
	LowerCaseAZ(relPath);
#endif
	const std::u32string patternU32 = UTF32FromUTF8(pattern);
	const std::u32string relPathU32 = UTF32FromUTF8(relPath);
	if (PatternMatch(patternU32, relPathU32)) {
		return true;
	}
	const size_t lastSlash = relPathU32.rfind('/');
	if (lastSlash == std::string::npos) {
		return false;
	}
	// Match against just filename
	const std::u32string fileNameU32 = relPathU32.substr(lastSlash+1);
	return PatternMatch(patternU32, fileNameU32);
}

constexpr std::string_view suffixStyled = ".styled";
constexpr std::string_view suffixFolded = ".folded";
constexpr std::string_view lexerPrefix = "lexer.*";
constexpr std::string_view prefixIf = "if ";
constexpr std::string_view prefixMatch = "match ";
constexpr std::string_view prefixEqual = "= ";
constexpr std::string_view prefixComment = "#";

std::string ReadFile(std::filesystem::path path) {
	std::ifstream ifs(path, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(ifs)),
		(std::istreambuf_iterator<char>()));
	return content;
}

std::string MarkedDocument(const Scintilla::IDocument *pdoc) {
	assert(pdoc);
	std::ostringstream os(std::ios::binary);
	char prevStyle = -1;
	for (Sci_Position pos = 0; pos < pdoc->Length(); pos++) {
		const char styleNow = pdoc->StyleAt(pos);
		if (styleNow != prevStyle) {
			const unsigned char uStyleNow = styleNow;
			const unsigned int uiStyleNow = uStyleNow;
			os << "{" << uiStyleNow << "}";
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
	assert(pdoc);
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

std::vector<std::string> StringSplit(const std::string_view &text, int separator) {
	std::vector<std::string> vs(text.empty() ? 0 : 1);
	for (std::string_view::const_iterator it = text.begin(); it != text.end(); ++it) {
		if (*it == separator) {
			vs.push_back(std::string());
		} else {
			vs.back() += *it;
		}
	}
	return vs;
}

constexpr bool IsSpaceOrTab(char ch) noexcept {
	return (ch == ' ') || (ch == '\t');
}

void PrintRanges(const std::vector<bool> &v) {
	std::cout << "    ";
	std::optional<size_t> startRange;
	for (size_t style = 0; style <= v.size(); style++) {
		// Goes one past size so that final range is closed
		if ((style < v.size()) && v.at(style)) {
			if (!startRange) {
				startRange = style;
			}
		} else if (startRange) {
			const size_t endRange = style - 1;
			std::cout << *startRange;
			if (*startRange != endRange) {
				std::cout << "-" << endRange;
			}
			std::cout << " ";
			startRange.reset();
		}
	}
	std::cout << "\n";
}

class PropertyMap {

	std::string Evaluate(std::string_view text) {
		if (text.find(' ') != std::string_view::npos) {
			if (text.starts_with(prefixEqual)) {
				const std::string_view sExpressions = text.substr(prefixEqual.length());
				std::vector<std::string> parts = StringSplit(sExpressions, ';');
				if (parts.size() > 1) {
					for (size_t part = 1; part < parts.size(); part++) {
						if (parts.at(part) != parts.at(0)) {
							return "0";
						}
					}
					return "1";
				}
			}
			return {};
		} else {
			std::optional<std::string> value = GetProperty(text);
			if (value) {
				return *value;
			}
			return {};
		}
	}

	std::string Expand(std::string withVars) {
		constexpr size_t maxVars = 100;
		size_t varStart = withVars.rfind("$(");
		for (size_t count = 0; (count < maxVars) && (varStart != std::string::npos); count++) {
			const size_t varEnd = withVars.find(')', varStart + 2);
			if (varEnd == std::string::npos) {
				break;
			}

			const std::string_view whole = withVars;
			const std::string_view var = whole.substr(varStart + 2, varEnd - (varStart + 2));
			const std::string val = Evaluate(var);

			withVars.erase(varStart, varEnd - varStart + 1);
			withVars.insert(varStart, val);

			varStart = withVars.rfind("$(");
		}
		return withVars;
	}

	std::vector<std::string> GetFilePatterns(const std::string &key) const {
		std::vector<std::string> exts;
		// Malformed patterns are skipped if we require the whole prefix here;
		// a fuzzy search lets us collect and report them
		const size_t patternStart = key.find('*');
		if (patternStart == std::string::npos)
			return exts;

		const std::string patterns = key.substr(patternStart);
		for (const std::string &pat : StringSplit(patterns, ';')) {
			// Only accept patterns in the form *.xyz
			if (pat.starts_with("*.") && pat.length() > 2) {
				exts.push_back(pat.substr(1));
			} else {
				std::cout << "\n"
					  << "Ignoring bad file pattern '" << pat << "' in list " << patterns << "\n";
			}
		}
		return exts;
	}

	bool ProcessLine(std::string_view text, bool ifIsTrue) {
		// If clause ends with first non-indented line
		if (!ifIsTrue && (text.empty() || IsSpaceOrTab(text.at(0)))) {
			return false;
		}
		ifIsTrue = true;
		if (text.starts_with(prefixIf)) {
			const std::string value = Expand(std::string(text.substr(prefixIf.length())));
			if (value == "0" || value == "") {
				ifIsTrue = false;
			}
		} else if (text.starts_with(prefixMatch)) {
			std::optional<std::string> fileNameExt = GetProperty("FileNameExt");
			if (fileNameExt) {
				std::string pattern(text.substr(prefixMatch.length()));
				// Remove trailing white space
				while (!pattern.empty() && IsSpaceOrTab(pattern.back())) {
					pattern.pop_back();
				}
				ifIsTrue = PathMatch(pattern, *fileNameExt);
			} else {
				ifIsTrue = false;
			}
		} else {
			while (!text.empty() && IsSpaceOrTab(text.at(0))) {
				text.remove_prefix(1);
			}
			if (text.starts_with(prefixComment)) {
				return ifIsTrue;
			}
			const size_t positionEquals = text.find("=");
			if (positionEquals != std::string::npos) {
				const std::string key(text.substr(0, positionEquals));
				const std::string_view value = text.substr(positionEquals + 1);
				properties[key] = value;
			}
		}
		return ifIsTrue;
	}

public:
	using PropMap = std::map<std::string, std::string>;
	PropMap properties;

	void ReadFromFile(std::filesystem::path path) {
		bool ifIsTrue = true;
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
				ifIsTrue = ProcessLine(logicalLine, ifIsTrue);
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
				} else if (key.find(';') != std::string::npos) {
					// It may be the case that a suite of test files with various extensions are
					// meant to share a common configuration, so try to find a matching
					// extension in a delimited list, e.g., lexer.*.html;*.php;*.asp=hypertext
					for (const std::string &ext : GetFilePatterns(key)) {
						if (fileName.ends_with(ext)) {
							return val;
						}
					}
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
	const std::string differenceType = augmentedText.empty() ? "new" : "different";
	std::cout << " has " << differenceType << " " << item << "\n\n";
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
	assert(plex);
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

bool TestCRLF(std::filesystem::path path, const std::string s, Scintilla::ILexer5 *plex, bool disablePerLineTests) {
	assert(plex);
	bool success = true;
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
	assert(pdoc);
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
				success = false;
			}
			line++;
		}
		prevStyle = styleNow;
	}

	// Lex and fold with \n line ends then check result is same

	TestDocument docUnix;
	docUnix.Set(textUnix);
	Scintilla::IDocument *pdocUnix = &docUnix;
	assert(pdocUnix);
	plex->Lex(0, pdocUnix->Length(), 0, pdocUnix);
	plex->Fold(0, pdocUnix->Length(), 0, pdocUnix);
	auto [styledTextUnix, foldedTextUnix] = MarkedAndFoldedDocument(pdocUnix);

	// Convert results from \n to \r\n run
	UnixToWindows(styledTextUnix);
	UnixToWindows(foldedTextUnix);

	if (styledText != styledTextUnix) {
		std::cout << "\n" << path.string() << ":1: has different styles with \\n versus \\r\\n line ends\n\n";
		success = false;
	}
	if (foldedText != foldedTextUnix) {
		std::cout << "\n" << path.string() << ":1: has different folds with \\n versus \\r\\n line ends\n\n";
		success = false;
	}

	// Test line by line lexing/folding with Unix \n line ends
	if (!disablePerLineTests && !originalIsUnix) {
		StyleLineByLine(docUnix, plex);
		auto [styledTextNewPerLine, foldedTextNewPerLine] = MarkedAndFoldedDocument(pdocUnix);
		// Convert results from \n to \r\n run
		UnixToWindows(styledTextNewPerLine);
		UnixToWindows(foldedTextNewPerLine);
		if (!CheckSame(styledTextUnix, styledTextNewPerLine, "per-line styles \\n", suffixStyled, path)) {
			success = false;
		}
		if (!CheckSame(foldedTextUnix, foldedTextNewPerLine, "per-line folds \\n", suffixFolded, path)) {
			success = false;
		}
	}

	plex->Release();
	return success;
}

void TestILexer(Scintilla::ILexer5 *plex) {
	assert(plex);

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

	if (std::string_view bases = plex->GetSubStyleBases(); !bases.empty()) {
		// Allocate a substyle for each possible style
		while (!bases.empty()) {
			constexpr int newStyles = 3;
			const int base = bases.front();
			const int baseStyle = plex->AllocateSubStyles(base, newStyles);
			[[maybe_unused]] const int styleBack = plex->StyleFromSubStyle(baseStyle);
			assert(styleBack == base);
			plex->SetIdentifiers(baseStyle, "int nullptr");
			[[maybe_unused]] const int start = plex->SubStylesStart(base);
			assert(start == baseStyle);
			[[maybe_unused]] const int len = plex->SubStylesLength(base);
			assert(len == newStyles);
			bases.remove_prefix(1);
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

bool SetProperties(Scintilla::ILexer5 *plex, const std::string &language, const PropertyMap &propertyMap, std::filesystem::path path) {
	assert(plex);

	const std::string fileName = path.filename().string();

	if (std::string_view bases = plex->GetSubStyleBases(); !bases.empty()) {
		// Allocate a substyle for each possible style
		while (!bases.empty()) {
			const int baseStyle = bases.front();
			//	substyles.cpp.11=2
			const std::string base = std::to_string(baseStyle);
			const std::string substylesForBase = "substyles." + language + "." + base;
			std::optional<std::string> substylesN = propertyMap.GetProperty(substylesForBase);
			if (substylesN) {
				const int substyles = atoi(substylesN->c_str());
				const int baseStyleNum = plex->AllocateSubStyles(baseStyle, substyles);
				//	substylewords.11.1.$(file.patterns.cpp)=std map string vector
				for (int kw = 0; kw < substyles; kw++) {
					const std::string substyleWords = "substylewords." + base + "." + std::to_string(kw + 1) + ".*";
					const std::optional<std::string> keywordN = propertyMap.GetPropertyForFile(substyleWords, fileName);
					if (keywordN) {
						plex->SetIdentifiers(baseStyleNum + kw, keywordN->c_str());
					}
				}
			}
			bases.remove_prefix(1);
		}
	}

	// Set keywords, keywords2, ... keywords9, for this file
	for (int kw = 0; kw < 10; kw++) {
		std::string kwChoice("keywords");
		if (kw > 0) {
			kwChoice.push_back(static_cast<char>('1' + kw));
		}
		kwChoice.append(".*");
		std::optional<std::string> keywordN = propertyMap.GetPropertyForFile(kwChoice, fileName);
		if (keywordN) {
			// New lexer object has all word lists empty so check null effect from setting empty
			const Sci_Position changedEmpty = plex->WordListSet(kw, "");
			if (changedEmpty != -1) {
				std::cout << path.string() << ":1: does not return -1 for null WordListSet(" << kw << ")\n";
				return false;
			}
			const Sci_Position changedAt = plex->WordListSet(kw, keywordN->c_str());
			if (keywordN->empty()) {
				if (changedAt != -1) {
					std::cout << path.string() << ":1: does not return -1 for WordListSet(" << kw << ") to same empty" << "\n";
					return false;
				}
			} else {
				if (changedAt == -1) {
					std::cout << path.string() << ":1: returns -1 for WordListSet(" << kw << ")\n";
					return false;
				}
			}
		}
	}

	// Set parameters of lexer
	for (auto const &[key, val] : propertyMap.properties) {
		if (key.starts_with("lexer.*")) {
			// Ignore as processed earlier
		} else if (key.starts_with("keywords")) {
			// Ignore as processed earlier
		} else if (key.starts_with("substyle")) {
			// Ignore as processed earlier
		} else {
			plex->PropertySet(key.c_str(), val.c_str());
		}
	}

	return true;
}


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

	TestILexer(plex);

	if (!SetProperties(plex, *language, propertyMap, path)) {
		return false;
	}

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
	assert(pdoc);
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

	if (propertyMap.GetPropertyValue("testlexers.list.styles").value_or(0)) {
		std::vector<bool> used(0x100);
		for (Sci_Position pos = 0; pos < pdoc->Length(); pos++) {
			const unsigned char uchStyle = pdoc->StyleAt(pos);
			const unsigned style = uchStyle;
			used.at(style) = true;
		}
		PrintRanges(used);
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

	if (success) {
		Scintilla::ILexer5 *plexCRLF = Lexilla::MakeLexer(*language);
		SetProperties(plexCRLF, *language, propertyMap, path.filename().string());
		success = TestCRLF(path, text, plexCRLF, disablePerLineTests);
	}

	return success;
}

bool TestDirectory(std::filesystem::path directory, std::filesystem::path basePath) {
	bool success = true;
	for (auto &p : std::filesystem::directory_iterator(directory)) {
		if (!p.is_directory()) {
			const std::string extension = p.path().extension().string();
			if (extension != ".properties" && extension != suffixStyled && extension != ".new" &&
				extension != suffixFolded) {
				const std::filesystem::path relativePath = p.path().lexically_relative(basePath);
				std::cout << "Lexing " << relativePath.string() << '\n';
				PropertyMap properties;
				properties.properties["FileNameExt"] = p.path().filename().string();
				properties.ReadFromFile(directory / "SciTE.properties");
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



int main(int argc, char **argv) {
	bool success = false;
	const std::filesystem::path baseDirectory = FindLexillaDirectory(std::filesystem::current_path());
	if (!baseDirectory.empty()) {
#if !defined(LEXILLA_STATIC)
		const std::filesystem::path sharedLibrary = baseDirectory / "bin" / LEXILLA_LIB;
		if (!Lexilla::Load(sharedLibrary.string())) {
			std::cout << "Failed to load " << sharedLibrary << "\n";
			return 1;	// Indicate failure
		}
#endif
		std::filesystem::path examplesDirectory = baseDirectory / "test" / "examples";
		for (int i = 1; i < argc; i++) {
			if (argv[i][0] != '-') {
				examplesDirectory = argv[i];
			}
		}
		success = AccessLexilla(examplesDirectory);
	}
	return success ? 0 : 1;
}
