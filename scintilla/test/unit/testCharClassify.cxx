// Unit Tests for Scintilla internal data structures

#include <string.h>

#include <algorithm>

#include "Platform.h"

#include "CharClassify.h"

#include <gtest/gtest.h>

// Test CharClassify.

class CharClassifyTest : public::testing::Test {
protected:
	virtual void SetUp() {
		pcc = new CharClassify();
		for (int ch = 0; ch < 256; ch++) {
			if (ch == '\r' || ch == '\n')
				charClass[ch] = CharClassify::ccNewLine;
			else if (ch < 0x20 || ch == ' ')
				charClass[ch] = CharClassify::ccSpace;
			else if (ch >= 0x80 || isalnum(ch) || ch == '_')
				charClass[ch] = CharClassify::ccWord;
			else
				charClass[ch] = CharClassify::ccPunctuation;
		}
	}

	virtual void TearDown() {
		delete pcc;
		pcc = 0;
	}

	CharClassify *pcc;
	CharClassify::cc charClass[256];

	static const char* GetClassName(CharClassify::cc charClass) {
		switch(charClass) {
			#define CASE(c) case CharClassify::c: return #c
			CASE(ccSpace);
			CASE(ccNewLine);
			CASE(ccWord);
			CASE(ccPunctuation);
			#undef CASE
			default:
				return "<unknown>";
		}
	}
};

TEST_F(CharClassifyTest, Defaults) {
	for (int i = 0; i < 256; i++) {
		EXPECT_EQ(charClass[i], pcc->GetClass(i))
			<< "Character " << i
			<< " should be class " << GetClassName(charClass[i])
			<< ", but got " << GetClassName(pcc->GetClass(i));
	}
}

TEST_F(CharClassifyTest, Custom) {
	unsigned char buf[2] = {0, 0};
	for (int i = 0; i < 256; i++) {
		CharClassify::cc thisClass = CharClassify::cc(i % 4);
		buf[0] = i;
		pcc->SetCharClasses(buf, thisClass);
		charClass[i] = thisClass;
	}
	for (int i = 0; i < 256; i++) {
		EXPECT_EQ(charClass[i], pcc->GetClass(i))
			<< "Character " << i
			<< " should be class " << GetClassName(charClass[i])
			<< ", but got " << GetClassName(pcc->GetClass(i));
	}
}

TEST_F(CharClassifyTest, CharsOfClass) {
	unsigned char buf[2] = {0, 0};
	for (int i = 1; i < 256; i++) {
		CharClassify::cc thisClass = CharClassify::cc(i % 4);
		buf[0] = i;
		pcc->SetCharClasses(buf, thisClass);
		charClass[i] = thisClass;
	}
	for (int classVal = 0; classVal < 4; ++classVal) {
		CharClassify::cc thisClass = CharClassify::cc(classVal % 4);
		int size = pcc->GetCharsOfClass(thisClass, NULL);
		unsigned char* buffer = reinterpret_cast<unsigned char*>(malloc(size + 1));
		ASSERT_TRUE(buffer);
		buffer[size] = '\0';
		pcc->GetCharsOfClass(thisClass, buffer);
		for (int i = 1; i < 256; i++) {
			if (charClass[i] == thisClass) {
				EXPECT_TRUE(memchr(reinterpret_cast<char*>(buffer), i, size))
					<< "Character " << i
					<< " should be class " << GetClassName(thisClass)
					<< ", but was not in GetCharsOfClass;"
					<< " it is reported to be "
					<< GetClassName(pcc->GetClass(i));
			} else {
				EXPECT_FALSE(memchr(reinterpret_cast<char*>(buffer), i, size))
					<< "Character " << i
					<< " should not be class " << GetClassName(thisClass)
					<< ", but was in GetCharsOfClass"
					<< " it is reported to be "
					<< GetClassName(pcc->GetClass(i));
			}
		}
		free(buffer);
	}
}
