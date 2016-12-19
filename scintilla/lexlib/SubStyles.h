// Scintilla source code edit control
/** @file SubStyles.h
 ** Manage substyles for a lexer.
 **/
// Copyright 2012 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SUBSTYLES_H
#define SUBSTYLES_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class WordClassifier {
	int baseStyle;
	int firstStyle;
	int lenStyles;
	std::map<std::string, int> wordToStyle;

public:

	explicit WordClassifier(int baseStyle_) : baseStyle(baseStyle_), firstStyle(0), lenStyles(0) {
	}

	void Allocate(int firstStyle_, int lenStyles_) {
		firstStyle = firstStyle_;
		lenStyles = lenStyles_;
		wordToStyle.clear();
	}

	int Base() const {
		return baseStyle;
	}

	int Start() const {
		return firstStyle;
	}

	int Length() const {
		return lenStyles;
	}

	void Clear() {
		firstStyle = 0;
		lenStyles = 0;
		wordToStyle.clear();
	}

	int ValueFor(const std::string &s) const {
		std::map<std::string, int>::const_iterator it = wordToStyle.find(s);
		if (it != wordToStyle.end())
			return it->second;
		else
			return -1;
	}

	bool IncludesStyle(int style) const {
		return (style >= firstStyle) && (style < (firstStyle + lenStyles));
	}

	void SetIdentifiers(int style, const char *identifiers) {
		while (*identifiers) {
			const char *cpSpace = identifiers;
			while (*cpSpace && !(*cpSpace == ' ' || *cpSpace == '\t' || *cpSpace == '\r' || *cpSpace == '\n'))
				cpSpace++;
			if (cpSpace > identifiers) {
				std::string word(identifiers, cpSpace - identifiers);
				wordToStyle[word] = style;
			}
			identifiers = cpSpace;
			if (*identifiers)
				identifiers++;
		}
	}
};

class SubStyles {
	int classifications;
	const char *baseStyles;
	int styleFirst;
	int stylesAvailable;
	int secondaryDistance;
	int allocated;
	std::vector<WordClassifier> classifiers;

	int BlockFromBaseStyle(int baseStyle) const {
		for (int b=0; b < classifications; b++) {
			if (baseStyle == baseStyles[b])
				return b;
		}
		return -1;
	}

	int BlockFromStyle(int style) const {
		int b = 0;
		for (std::vector<WordClassifier>::const_iterator it=classifiers.begin(); it != classifiers.end(); ++it) {
			if (it->IncludesStyle(style))
				return b;
			b++;
		}
		return -1;
	}

public:

	SubStyles(const char *baseStyles_, int styleFirst_, int stylesAvailable_, int secondaryDistance_) :
		classifications(0),
		baseStyles(baseStyles_),
		styleFirst(styleFirst_),
		stylesAvailable(stylesAvailable_),
		secondaryDistance(secondaryDistance_),
		allocated(0) {
		while (baseStyles[classifications]) {
			classifiers.push_back(WordClassifier(baseStyles[classifications]));
			classifications++;
		}
	}

	int Allocate(int styleBase, int numberStyles) {
		int block = BlockFromBaseStyle(styleBase);
		if (block >= 0) {
			if ((allocated + numberStyles) > stylesAvailable)
				return -1;
			int startBlock = styleFirst + allocated;
			allocated += numberStyles;
			classifiers[block].Allocate(startBlock, numberStyles);
			return startBlock;
		} else {
			return -1;
		}
	}

	int Start(int styleBase) {
		int block = BlockFromBaseStyle(styleBase);
		return (block >= 0) ? classifiers[block].Start() : -1;
	}

	int Length(int styleBase) {
		int block = BlockFromBaseStyle(styleBase);
		return (block >= 0) ? classifiers[block].Length() : 0;
	}

	int BaseStyle(int subStyle) const {
		int block = BlockFromStyle(subStyle);
		if (block >= 0)
			return classifiers[block].Base();
		else
			return subStyle;
	}

	int DistanceToSecondaryStyles() const {
		return secondaryDistance;
	}

	void SetIdentifiers(int style, const char *identifiers) {
		int block = BlockFromStyle(style);
		if (block >= 0)
			classifiers[block].SetIdentifiers(style, identifiers);
	}

	void Free() {
		allocated = 0;
		for (std::vector<WordClassifier>::iterator it=classifiers.begin(); it != classifiers.end(); ++it)
			it->Clear();
	}

	const WordClassifier &Classifier(int baseStyle) const {
		const int block = BlockFromBaseStyle(baseStyle);
		return classifiers[block >= 0 ? block : 0];
	}
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
