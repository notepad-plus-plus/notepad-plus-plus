// Unit Tests for Scintilla internal data structures

/*
    Currently tested:
        SplitVector
        Partitioning
        RunStyles
        ContractionState
        CharClassify
        Decoration
        DecorationList
        CellBuffer
        UniConversion

    To do:
        PerLine *
        Range
        StyledText
        CaseFolder ...
        Document
        RESearch
        Selection
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

#include <cstdio>
#include <cstdarg>

#include <string_view>
#include <vector>
#include <memory>

#include "Platform.h"

#if defined(__GNUC__)
// Want to avoid misleading indentation warnings in catch.hpp but the pragma
// may not be available so protect by turning off pragma warnings
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wpragmas"
#if !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif
#endif

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

using namespace Scintilla;

// Needed for PLATFORM_ASSERT in code being tested

void Platform::Assert(const char *c, const char *file, int line) {
	fprintf(stderr, "Assertion [%s] failed at %s %d\n", c, file, line);
	abort();
}

void Platform::DebugPrintf(const char *format, ...) {
	char buffer[2000];
	va_list pArguments;
	va_start(pArguments, format);
	vsprintf(buffer, format, pArguments);
	va_end(pArguments);
	fprintf(stderr, "%s", buffer);
}
