// UnitTester.cpp : Defines the entry point for the console application.
//

// Catch uses std::uncaught_exception which is deprecated in C++17.
// This define silences a warning from Visual C++.
#define _SILENCE_CXX17_UNCAUGHT_EXCEPTION_DEPRECATION_WARNING

#include <cstdio>
#include <cstdarg>

#include <string_view>
#include <vector>
#include <memory>

#include "Platform.h"

#define CATCH_CONFIG_WINDOWS_CRTDBG
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

// Needed for PLATFORM_ASSERT in code being tested

namespace Scintilla {

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

}

int main(int argc, char* argv[]) {
	const int result = Catch::Session().run(argc, argv);

	return result;
}
