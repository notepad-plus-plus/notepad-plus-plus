/** @file UnitTester.cxx
 ** UnitTester.cpp : Defines the entry point for the console application.
 **/

// Catch uses std::uncaught_exception which is deprecated in C++17.
// This define silences a warning from Visual C++.
#define _SILENCE_CXX17_UNCAUGHT_EXCEPTION_DEPRECATION_WARNING

#include <cstdio>
#include <cstdarg>

#include <string_view>
#include <vector>
#include <optional>
#include <memory>

#include "Debugging.h"

#if defined(_WIN32)
#define CATCH_CONFIG_WINDOWS_CRTDBG
#endif
#define CATCH_CONFIG_RUNNER
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

int main(int argc, char* argv[]) {
	const int result = Catch::Session().run(argc, argv);

	return result;
}
