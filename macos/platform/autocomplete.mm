// autocomplete.mm — Word auto-completion
// Part of the Notepad++ macOS port modular refactor.

#include "autocomplete.h"
#include "npp_constants.h"
#include "app_state.h"
#include "scintilla_bridge.h"
#include <set>
#include <algorithm>

void showAutoComplete()
{
	void* sci = ctx().activeScintillaView();
	if (!sci) return;

	intptr_t pos = ScintillaBridge_sendMessage(sci, SCI_GETCURRENTPOS, 0, 0);
	intptr_t wordStart = ScintillaBridge_sendMessage(sci, SCI_WORDSTARTPOSITION, pos, 1);
	intptr_t wordLen = pos - wordStart;

	if (wordLen < 1) return;

	char partial[256] = {};
	for (intptr_t i = 0; i < wordLen && i < 255; ++i)
		partial[i] = static_cast<char>(ScintillaBridge_sendMessage(sci, SCI_GETCHARAT, wordStart + i, 0));
	partial[wordLen] = '\0';

	intptr_t docLen = ScintillaBridge_sendMessage(sci, SCI_GETLENGTH, 0, 0);
	if (docLen <= 0) return;

	std::string docText;
	docText.resize(docLen + 1);
	ScintillaBridge_sendMessage(sci, SCI_GETTEXT, docLen + 1, (intptr_t)docText.data());

	std::set<std::string> words;
	std::string currentWord;
	for (intptr_t i = 0; i < docLen; ++i)
	{
		unsigned char ch = static_cast<unsigned char>(docText[i]);
		if (isalnum(ch) || ch == '_')
		{
			currentWord += ch;
		}
		else
		{
			if (!currentWord.empty() && currentWord.length() >= 3)
				words.insert(currentWord);
			currentWord.clear();
		}
	}
	if (!currentWord.empty() && currentWord.length() >= 3)
		words.insert(currentWord);

	std::string partialLower(partial);
	std::transform(partialLower.begin(), partialLower.end(), partialLower.begin(),
	               [](unsigned char c){ return std::tolower(c); });

	std::vector<std::string> matches;
	for (const auto& w : words)
	{
		if (static_cast<intptr_t>(w.length()) <= wordLen) continue;
		std::string wLower = w.substr(0, wordLen);
		std::transform(wLower.begin(), wLower.end(), wLower.begin(),
		               [](unsigned char c){ return std::tolower(c); });
		if (wLower == partialLower)
			matches.push_back(w);
	}

	if (matches.empty()) return;

	std::string wordList;
	for (size_t i = 0; i < matches.size(); ++i)
	{
		if (i > 0) wordList += ' ';
		wordList += matches[i];
	}

	ScintillaBridge_sendMessage(sci, SCI_AUTOCSETIGNORECASE, 1, 0);
	ScintillaBridge_sendMessage(sci, SCI_AUTOCSETORDER, 1, 0);
	ScintillaBridge_sendMessage(sci, SCI_AUTOCSHOW, wordLen, (intptr_t)wordList.c_str());
}
