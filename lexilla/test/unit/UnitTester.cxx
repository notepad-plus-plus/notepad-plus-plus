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
#include <memory>

#define CATCH_CONFIG_WINDOWS_CRTDBG
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

int main(int argc, char* argv[]) {
	const int result = Catch::Session().run(argc, argv);

	return result;
}
