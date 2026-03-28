#include "function_list_parser.h"
#include "function_list_panel.h"
#include "language_defs.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <regex>
#include <stdexcept>

static FILE* dbgLog()
{
	static FILE* f = fopen("/tmp/MacNotePP.log", "a");
	return f;
}
#define FLLOG(fmt, ...) do { if (FILE* _f = dbgLog()) { fprintf(_f, "[FLParser] " fmt "\n", ##__VA_ARGS__); fflush(_f); } } while(0)
#include <unordered_map>
#include <unordered_set>

namespace
{
struct FlatSymbol
{
	std::string name;
	std::string container;
	int line = 0;
	int position = 0;
};

struct ContainerScope
{
	std::string name;
	int depth = 0;
};

struct ParseAccum
{
	std::vector<FlatSymbol> symbols;
	std::unordered_map<std::string, std::pair<int, int>> containerPos;
	std::vector<std::string> containerOrder;
	std::unordered_set<std::string> containerSeen;

	void addContainer(const std::string& name, int line, int pos)
	{
		if (name.empty())
			return;
		containerPos.emplace(name, std::make_pair(line, pos));
		if (containerSeen.insert(name).second)
			containerOrder.push_back(name);
	}

	void addSymbol(const std::string& name, const std::string& container, int line, int pos)
	{
		if (name.empty())
			return;
		FlatSymbol sym;
		sym.name = name;
		sym.container = container;
		sym.line = line;
		sym.position = pos;
		symbols.push_back(std::move(sym));
	}
};

static std::string trim(const std::string& s)
{
	size_t start = 0;
	while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
		++start;
	size_t end = s.size();
	while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
		--end;
	return s.substr(start, end - start);
}

static int leadingIndent(const std::string& s)
{
	int out = 0;
	for (char c : s)
	{
		if (c == ' ')
			++out;
		else if (c == '\t')
			out += 4;
		else
			break;
	}
	return out;
}

static bool isControlKeyword(const std::string& name)
{
	static const std::unordered_set<std::string> kKeywords{
		"if", "for", "while", "switch", "catch", "return", "sizeof", "new", "delete", "throw"
	};
	return kKeywords.find(name) != kKeywords.end();
}

struct PendingFunc
{
	std::string name;
	std::string container;
	bool isScoped = false;
	int line = 0;
	int namePosition = 0;
	int containerPosition = 0;
};

static bool endsWithSemicolon(const std::string& trimmed)
{
	return !trimmed.empty() && trimmed.back() == ';';
}

static void parsePython(const std::string& utf8Text, ParseAccum& accum)
{
	static const std::regex classRe(R"(^\s*class\s+([A-Za-z_]\w*)\b)");
	static const std::regex defRe(R"(^\s*def\s+([A-Za-z_]\w*)\s*\()");
	std::vector<std::pair<std::string, int>> classStack;

	size_t lineStart = 0;
	int lineNo = 1;
	while (lineStart <= utf8Text.size())
	{
		size_t lineEnd = utf8Text.find('\n', lineStart);
		if (lineEnd == std::string::npos)
			lineEnd = utf8Text.size();

		const std::string line = utf8Text.substr(lineStart, lineEnd - lineStart);
		const int indent = leadingIndent(line);

		while (!classStack.empty() && indent <= classStack.back().second)
			classStack.pop_back();

		if (line.size() <= 1000) try
		{
		std::smatch m;
		if (std::regex_search(line, m, classRe))
		{
			const std::string className = m[1].str();
			const int classPos = static_cast<int>(lineStart + static_cast<size_t>(m.position(1)));
			accum.addContainer(className, lineNo, classPos);
			classStack.emplace_back(className, indent);
		}

		if (std::regex_search(line, m, defRe))
		{
			std::string container;
			if (!classStack.empty())
				container = classStack.back().first;
			const int pos = static_cast<int>(lineStart + static_cast<size_t>(m.position(1)));
			accum.addSymbol(m[1].str(), container, lineNo, pos);
		}
		} catch (const std::regex_error&) {}

		if (lineEnd == utf8Text.size())
			break;
		lineStart = lineEnd + 1;
		++lineNo;
	}
}

static void parseBraceLanguage(const std::string& utf8Text, int languageIndex, ParseAccum& accum)
{
	static const std::regex classRe(R"(\b(class|struct|interface|protocol|extension)\s+([A-Za-z_]\w*))");
	static const std::regex cStructRe(R"(\bstruct\s+([A-Za-z_]\w*))");
	static const std::regex rustImplRe(R"(\bimpl(?:\s*<[^>]+>)?\s+([A-Za-z_]\w*))");
	static const std::regex goTypeRe(R"(^\s*type\s+([A-Za-z_]\w*)\s+(?:struct|interface)\b)");

	static const std::regex cppScopedFuncRe(R"(\b([A-Za-z_]\w*)::([A-Za-z_~]\w*)\s*\([^;{}]*\)\s*(?:const\s*)?\{)");
	static const std::regex cppFuncRe(R"(^\s*[\w:<>\~\*&\s]+\s+([A-Za-z_~]\w*)\s*\([^;{}]*\)\s*(?:const\s*)?\{)");
	static const std::regex javaFuncRe(R"(^\s*(?:public|protected|private|static|final|synchronized|abstract|native|strictfp|\s)+[\w\<\>\[\],\s]+\s+([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{)");
	static const std::regex jsFunctionRe(R"(^\s*function\s+([A-Za-z_$]\w*)\s*\()");
	static const std::regex jsArrowRe(R"(^\s*(?:const|let|var)\s+([A-Za-z_$]\w*)\s*=\s*(?:async\s*)?\([^)]*\)\s*=>)");
	static const std::regex jsMethodRe(R"(^\s*(?:async\s+)?([A-Za-z_$]\w*)\s*\([^)]*\)\s*\{)");
	static const std::regex goFuncRe(R"(^\s*func\s*(?:\(\s*[^)]*?\s+\*?([A-Za-z_]\w*)\s*\)\s*)?([A-Za-z_]\w*)\s*\()");
	static const std::regex rustFuncRe(R"(^\s*(?:pub\s+)?(?:async\s+)?fn\s+([A-Za-z_]\w*)\s*\()");
	static const std::regex swiftFuncRe(R"(^\s*(?:public|internal|private|fileprivate|open|static|class|final|override|mutating|nonmutating|required|convenience|\s)*\s*func\s+([A-Za-z_]\w*)\s*\()");

	// Allman-style signature regexes (no trailing \{)
	static const std::regex cppScopedSigRe(R"(\b([A-Za-z_]\w*)::([A-Za-z_~]\w*)\s*\([^;{}]*\))");
	static const std::regex cppSigRe(R"(^\s*(?:[\w*&<>:~]+\s+)+([A-Za-z_~]\w*)\s*\([^;{}]*\))");

	std::vector<ContainerScope> containerStack;
	std::string pendingContainer;
	int braceDepth = 0;
	PendingFunc pendingFunc;
	bool hasPendingFunc = false;

	size_t lineStart = 0;
	int lineNo = 1;
	int linesProcessed = 0;
	int linesSkippedLength = 0;
	int linesSkippedRegexErr = 0;
	int matchCount = 0;
	int pendingSetCount = 0;
	int pendingFlushCount = 0;
	FLLOG("parseBrace: starting lang=%d textLen=%zu", languageIndex, utf8Text.size());
	while (lineStart <= utf8Text.size())
	{
		if ((lineNo & 0x3FF) == 0 && isFunctionListShuttingDown())
		{
			FLLOG("parseBrace: CANCELLED at line %d (shutting down)", lineNo);
			return;
		}
		size_t lineEnd = utf8Text.find('\n', lineStart);
		if (lineEnd == std::string::npos)
			lineEnd = utf8Text.size();

		const std::string line = utf8Text.substr(lineStart, lineEnd - lineStart);
		const std::string clean = trim(line);

		// Allman-style: flush pending function when brace-only line found
		if (hasPendingFunc)
		{
			if (clean == "{")
			{
				std::string topCont;
				if (!containerStack.empty())
					topCont = containerStack.back().name;

				if (pendingFunc.isScoped)
				{
					accum.addContainer(pendingFunc.container, pendingFunc.line, pendingFunc.containerPosition);
					accum.addSymbol(pendingFunc.name, pendingFunc.container, pendingFunc.line, pendingFunc.namePosition);
				}
				else
				{
					accum.addSymbol(pendingFunc.name, topCont, pendingFunc.line, pendingFunc.namePosition);
				}
				++pendingFlushCount;
				++matchCount;
				hasPendingFunc = false;
			}
			else if (!clean.empty() && (endsWithSemicolon(clean) || clean[0] == '}'))
			{
				hasPendingFunc = false;
			}
		}

		// Skip regex matching on very long lines — no function signature
		// is this long, and complex regexes can hit backtracking limits
		// (std::regex error_complexity) on lines common in large C files.
		// Brace counting below still runs on every line.
		++linesProcessed;
		if (line.size() > 1000) { ++linesSkippedLength; }
		else try
		{
		// Skip comment lines — they cause false matches (e.g. "interface does" in sqlite3.c)
		bool isComment = false;
		if (!clean.empty())
		{
			char c0 = clean[0];
			isComment = (c0 == '*') || (c0 == '/' && clean.size() >= 2 && (clean[1] == '/' || clean[1] == '*'));
		}

		std::smatch m;
		if (!isComment)
		{
		if (languageIndex == LANG_RUST)
		{
			if (std::regex_search(line, m, rustImplRe))
			{
				pendingContainer = m[1].str();
				accum.addContainer(pendingContainer, lineNo, static_cast<int>(lineStart + static_cast<size_t>(m.position(1))));
			}
		}
		else if (languageIndex == LANG_GO)
		{
			if (std::regex_search(line, m, goTypeRe))
			{
				pendingContainer = m[1].str();
				accum.addContainer(pendingContainer, lineNo, static_cast<int>(lineStart + static_cast<size_t>(m.position(1))));
			}
		}
		else if (languageIndex == LANG_C)
		{
			// C has no class/interface — only match struct
			if (std::regex_search(line, m, cStructRe))
			{
				pendingContainer = m[1].str();
				accum.addContainer(pendingContainer, lineNo, static_cast<int>(lineStart + static_cast<size_t>(m.position(1))));
			}
		}
		else
		{
			if (std::regex_search(line, m, classRe))
			{
				pendingContainer = m[2].str();
				accum.addContainer(pendingContainer, lineNo, static_cast<int>(lineStart + static_cast<size_t>(m.position(2))));
			}
		}

		std::string topContainer;
		if (!containerStack.empty())
			topContainer = containerStack.back().name;

		if (languageIndex == LANG_C || languageIndex == LANG_CPP || languageIndex == LANG_OBJC)
		{
			if (std::regex_search(line, m, cppScopedFuncRe))
			{
				const std::string container = m[1].str();
				const std::string func = m[2].str();
				accum.addContainer(container, lineNo, static_cast<int>(lineStart + static_cast<size_t>(m.position(1))));
				accum.addSymbol(func, container, lineNo, static_cast<int>(lineStart + static_cast<size_t>(m.position(2))));
				++matchCount;
				if (matchCount <= 5) FLLOG("  match[%d] scoped: %s::%s line=%d", matchCount, container.c_str(), func.c_str(), lineNo);
			}
			else if (std::regex_search(line, m, cppFuncRe))
			{
				const std::string func = m[1].str();
				if (!isControlKeyword(func))
				{
					accum.addSymbol(func, topContainer, lineNo, static_cast<int>(lineStart + static_cast<size_t>(m.position(1))));
					++matchCount;
					if (matchCount <= 5) FLLOG("  match[%d] inline: %s line=%d", matchCount, func.c_str(), lineNo);
				}
			}
			else if (!endsWithSemicolon(clean))
			{
				// Allman-style: signature on this line, { on the next
				if (std::regex_search(line, m, cppScopedSigRe))
				{
					hasPendingFunc = true;
					pendingFunc.name = m[2].str();
					pendingFunc.container = m[1].str();
					pendingFunc.isScoped = true;
					pendingFunc.line = lineNo;
					pendingFunc.namePosition = static_cast<int>(lineStart + static_cast<size_t>(m.position(2)));
					pendingFunc.containerPosition = static_cast<int>(lineStart + static_cast<size_t>(m.position(1)));
					++pendingSetCount;
					if (pendingSetCount <= 5) FLLOG("  pending scoped: %s::%s line=%d", pendingFunc.container.c_str(), pendingFunc.name.c_str(), lineNo);
				}
				else if (std::regex_search(line, m, cppSigRe))
				{
					const std::string func = m[1].str();
					if (!isControlKeyword(func))
					{
						hasPendingFunc = true;
						pendingFunc.name = func;
						pendingFunc.isScoped = false;
						pendingFunc.line = lineNo;
						pendingFunc.namePosition = static_cast<int>(lineStart + static_cast<size_t>(m.position(1)));
					}
				}
			}
		}
		else if (languageIndex == LANG_JAVA)
		{
			if (std::regex_search(line, m, javaFuncRe))
				accum.addSymbol(m[1].str(), topContainer, lineNo, static_cast<int>(lineStart + static_cast<size_t>(m.position(1))));
		}
		else if (languageIndex == LANG_JAVASCRIPT || languageIndex == LANG_TYPESCRIPT)
		{
			if (std::regex_search(line, m, jsFunctionRe))
				accum.addSymbol(m[1].str(), "", lineNo, static_cast<int>(lineStart + static_cast<size_t>(m.position(1))));
			else if (std::regex_search(line, m, jsArrowRe))
				accum.addSymbol(m[1].str(), "", lineNo, static_cast<int>(lineStart + static_cast<size_t>(m.position(1))));
			else if (std::regex_search(line, m, jsMethodRe))
			{
				const std::string func = m[1].str();
				if (!isControlKeyword(func))
					accum.addSymbol(func, topContainer, lineNo, static_cast<int>(lineStart + static_cast<size_t>(m.position(1))));
			}
		}
		else if (languageIndex == LANG_GO)
		{
			if (std::regex_search(line, m, goFuncRe))
			{
				std::string container;
				if (m.size() > 2)
				{
					container = m[1].str();
					if (!container.empty())
						accum.addContainer(container, lineNo, static_cast<int>(lineStart + static_cast<size_t>(m.position(1))));
				}
				accum.addSymbol(m[2].str(), container, lineNo, static_cast<int>(lineStart + static_cast<size_t>(m.position(2))));
			}
		}
		else if (languageIndex == LANG_RUST)
		{
			if (std::regex_search(line, m, rustFuncRe))
				accum.addSymbol(m[1].str(), topContainer, lineNo, static_cast<int>(lineStart + static_cast<size_t>(m.position(1))));
		}
		else if (languageIndex == LANG_SWIFT)
		{
			if (std::regex_search(line, m, swiftFuncRe))
				accum.addSymbol(m[1].str(), topContainer, lineNo, static_cast<int>(lineStart + static_cast<size_t>(m.position(1))));
		}
		else
		{
			if (std::regex_search(line, m, cppFuncRe))
			{
				const std::string func = m[1].str();
				if (!isControlKeyword(func))
					accum.addSymbol(func, topContainer, lineNo, static_cast<int>(lineStart + static_cast<size_t>(m.position(1))));
			}
			else if (!endsWithSemicolon(clean))
			{
				if (std::regex_search(line, m, cppSigRe))
				{
					const std::string func = m[1].str();
					if (!isControlKeyword(func))
					{
						hasPendingFunc = true;
						pendingFunc.name = func;
						pendingFunc.isScoped = false;
						pendingFunc.line = lineNo;
						pendingFunc.namePosition = static_cast<int>(lineStart + static_cast<size_t>(m.position(1)));
					}
				}
			}
		}
		} // if (!isComment)
		} catch (const std::regex_error&) { ++linesSkippedRegexErr; }

		for (char c : line)
		{
			if (c == '{')
			{
				++braceDepth;
				if (!pendingContainer.empty())
				{
					containerStack.push_back({pendingContainer, braceDepth});
					pendingContainer.clear();
				}
			}
			else if (c == '}')
			{
				while (!containerStack.empty() && containerStack.back().depth == braceDepth)
					containerStack.pop_back();
				if (braceDepth > 0)
					--braceDepth;
			}
		}

		if (clean.find(';') != std::string::npos && clean.find('{') == std::string::npos)
			pendingContainer.clear();

		if (lineEnd == utf8Text.size())
			break;
		lineStart = lineEnd + 1;
		++lineNo;
	}
	FLLOG("parseBrace: done — lines=%d skippedLen=%d skippedRegex=%d matches=%d pendingSet=%d pendingFlush=%d",
		  linesProcessed, linesSkippedLength, linesSkippedRegexErr, matchCount, pendingSetCount, pendingFlushCount);
}

static std::vector<FunctionListNode> buildTree(const ParseAccum& accum)
{
	std::vector<FunctionListNode> roots;
	std::unordered_map<std::string, size_t> containerIndex;

	for (const auto& name : accum.containerOrder)
	{
		auto it = accum.containerPos.find(name);
		if (it == accum.containerPos.end())
			continue;
		FunctionListNode node;
		node.title = name;
		node.isContainer = true;
		node.line = it->second.first;
		node.position = it->second.second;
		containerIndex.emplace(name, roots.size());
		roots.push_back(std::move(node));
	}

	for (const auto& sym : accum.symbols)
	{
		if (!sym.container.empty())
		{
			auto it = containerIndex.find(sym.container);
			if (it == containerIndex.end())
			{
				FunctionListNode parent;
				parent.title = sym.container;
				parent.isContainer = true;
				parent.line = sym.line;
				parent.position = sym.position;
				containerIndex.emplace(sym.container, roots.size());
				roots.push_back(parent);
				it = containerIndex.find(sym.container);
			}

			FunctionListNode child;
			child.title = sym.name;
			child.line = sym.line;
			child.position = sym.position;
			roots[it->second].children.push_back(std::move(child));
		}
		else
		{
			FunctionListNode node;
			node.title = sym.name;
			node.line = sym.line;
			node.position = sym.position;
			roots.push_back(std::move(node));
		}
	}

	return roots;
}
}

std::vector<FunctionListNode> parseFunctionListNodes(int languageIndex, const std::string& utf8Text)
{
	FLLOG("parseFunctionListNodes: langIndex=%d textLen=%zu", languageIndex, utf8Text.size());
	ParseAccum accum;
	if (utf8Text.empty())
	{
		FLLOG("parseFunctionListNodes: EMPTY text");
		return {};
	}

	if (languageIndex == LANG_PYTHON)
		parsePython(utf8Text, accum);
	else
		parseBraceLanguage(utf8Text, languageIndex, accum);

	FLLOG("parseFunctionListNodes: symbols=%zu containers=%zu", accum.symbols.size(), accum.containerOrder.size());
	auto roots = buildTree(accum);
	FLLOG("parseFunctionListNodes: returning %zu root nodes", roots.size());
	return roots;
}
