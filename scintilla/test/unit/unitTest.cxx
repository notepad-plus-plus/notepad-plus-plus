// Unit Tests for Scintilla internal data structures

/*
    Currently tested:
        SplitVector
        Partitioning
        RunStyles
        ContractionState

    To do:
        Decoration
        DecorationList
        PerLine *
        CellBuffer *
        Range
        StyledText
        CaseFolder ...
        Document
        RESearch
        Selection
        UniConversion
        Style

        lexlib:
        Accessor
        LexAccessor
        CharacterSet
        OptionSet
        PropSetSimple
        StyleContext
        WordList
*/

#include <stdio.h>

#include "Platform.h"

#include <gtest/gtest.h>

// Needed for PLATFORM_ASSERT in code being tested

void Platform::Assert(const char *c, const char *file, int line) {
	fprintf(stderr, "Assertion [%s] failed at %s %d\n", c, file, line);
	abort();
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
