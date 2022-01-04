/** @file testCharClassify.cxx
 ** Unit Tests for Scintilla internal data structures
 **/

#include <cstring>

#include <string_view>
#include <vector>
#include <optional>
#include <algorithm>
#include <memory>
#include <iostream>

#include "Debugging.h"

#include "CharClassify.h"

#include "catch.hpp"

using namespace Scintilla::Internal;

// Test CharClassify.

class CharClassifyTest {
	// Avoid warnings, deleted so never called.
	CharClassifyTest(const CharClassifyTest &) = delete;
protected:
	CharClassifyTest() {
		pcc = std::make_unique<CharClassify>();
		for (int ch = 0; ch < 256; ch++) {
			if (ch == '\r' || ch == '\n')
				charClass[ch] = CharacterClass::newLine;
			else if (ch < 0x20 || ch == ' ' || ch == '\x7f')
				charClass[ch] = CharacterClass::space;
			else if (ch >= 0x80 || isalnum(ch) || ch == '_')
				charClass[ch] = CharacterClass::word;
			else
				charClass[ch] = CharacterClass::punctuation;
		}
	}

	std::unique_ptr<CharClassify> pcc;
	CharacterClass charClass[256];

	static const char* GetClassName(CharacterClass charClass) {
		switch(charClass) {
			#define CASE(c) case CharacterClass::c: return #c
			CASE(space);
			CASE(newLine);
			CASE(word);
			CASE(punctuation);
			#undef CASE
			default:
				return "<unknown>";
		}
	}
};

TEST_CASE_METHOD(CharClassifyTest, "Defaults") {
	for (int i = 0; i < 256; i++) {
		if (charClass[i] != pcc->GetClass(i))
			std::cerr
			<< "Character " << i
			<< " should be class " << GetClassName(charClass[i])
			<< ", but got " << GetClassName(pcc->GetClass(i)) << std::endl;
		REQUIRE(charClass[i] == pcc->GetClass(i));
	}
}

TEST_CASE_METHOD(CharClassifyTest, "Custom") {
	unsigned char buf[2] = {0, 0};
	for (int i = 0; i < 256; i++) {
		CharacterClass thisClass = CharacterClass(i % 4);
		buf[0] = i;
		pcc->SetCharClasses(buf, thisClass);
		charClass[i] = thisClass;
	}
	for (int i = 0; i < 256; i++) {
		if (charClass[i] != pcc->GetClass(i))
			std::cerr
			<< "Character " << i
			<< " should be class " << GetClassName(charClass[i])
			<< ", but got " << GetClassName(pcc->GetClass(i)) << std::endl;
		REQUIRE(charClass[i] == pcc->GetClass(i));
	}
}

TEST_CASE_METHOD(CharClassifyTest, "CharsOfClass") {
	unsigned char buf[2] = {0, 0};
	for (int i = 1; i < 256; i++) {
		CharacterClass thisClass = CharacterClass(i % 4);
		buf[0] = i;
		pcc->SetCharClasses(buf, thisClass);
		charClass[i] = thisClass;
	}
	for (int classVal = 0; classVal < 4; ++classVal) {
		CharacterClass thisClass = CharacterClass(classVal % 4);
		int size = pcc->GetCharsOfClass(thisClass, NULL);
		std::vector<unsigned char> buffer(size+1);
		void *pBuffer = static_cast<void *>(buffer.data());
		pcc->GetCharsOfClass(thisClass, buffer.data());
		for (int i = 1; i < 256; i++) {
			if (charClass[i] == thisClass) {
				if (!memchr(pBuffer, i, size))
					std::cerr
					<< "Character " << i
					<< " should be class " << GetClassName(thisClass)
					<< ", but was not in GetCharsOfClass;"
					<< " it is reported to be "
					<< GetClassName(pcc->GetClass(i)) << std::endl;
				REQUIRE(memchr(pBuffer, i, size));
			} else {
				if (memchr(pBuffer, i, size))
					std::cerr
					<< "Character " << i
					<< " should not be class " << GetClassName(thisClass)
					<< ", but was in GetCharsOfClass"
					<< " it is reported to be "
					<< GetClassName(pcc->GetClass(i)) << std::endl;
				REQUIRE_FALSE(memchr(pBuffer, i, size));
			}
		}
	}
}
