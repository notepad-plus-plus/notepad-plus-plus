/** @file unitTest.cxx
 ** Unit Tests for Scintilla internal data structures
 **/

/*
    Currently tested:
        CellBuffer
        CharacterCategoryMap
        CharClassify
        ContractionState
        Decoration
        DecorationList
        Document
        Geometry
        Partitioning
        PerLine
        RESearch
        RunStyles
        Selection
        SplitVector
        UniConversion

    To do:
        Range
        StyledText
        CaseFolder ...
        Style
*/

#include <cstdio>
#include <cstdarg>

#include <string_view>
#include <vector>
#include <optional>
#include <memory>

#include "Debugging.h"

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

using namespace Scintilla::Internal;

// Needed for PLATFORM_ASSERT in code being tested

void Platform::Assert(const char *c, const char *file, int line) noexcept {
	fprintf(stderr, "Assertion [%s] failed at %s %d\n", c, file, line);
	abort();
}

void Platform::DebugPrintf(const char *format, ...) noexcept {
	char buffer[2000];
	va_list pArguments;
	va_start(pArguments, format);
	vsnprintf(buffer, std::size(buffer), format, pArguments);
	va_end(pArguments);
	fprintf(stderr, "%s", buffer);
}
