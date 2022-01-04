// Scintilla source code edit control
/** @file Debugging.h
 ** Assert and debug trace functions.
 ** Implemented in each platform layer.
 **/
// Copyright 1998-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef DEBUGGING_H
#define DEBUGGING_H

namespace Scintilla::Internal {

#if defined(__clang__)
# if __has_feature(attribute_analyzer_noreturn)
#  define CLANG_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
# else
#  define CLANG_ANALYZER_NORETURN
# endif
#else
# define CLANG_ANALYZER_NORETURN
#endif

/**
 * Platform namespace used to segregate debugging functions.
 */
namespace Platform {

void DebugDisplay(const char *s) noexcept;
void DebugPrintf(const char *format, ...) noexcept;
bool ShowAssertionPopUps(bool assertionPopUps_) noexcept;
void Assert(const char *c, const char *file, int line) noexcept CLANG_ANALYZER_NORETURN;

}

#ifdef  NDEBUG
#define PLATFORM_ASSERT(c) ((void)0)
#else
#define PLATFORM_ASSERT(c) ((c) ? (void)(0) : Scintilla::Internal::Platform::Assert(#c, __FILE__, __LINE__))
#endif

}

#endif
