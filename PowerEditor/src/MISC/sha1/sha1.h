/*
  100% free public domain implementation of the SHA-1 algorithm
  by Dominik Reichl <dominik.reichl@t-online.de>
  Web: http://www.dominik-reichl.de/

  Version 2.1 - 2012-06-19
  - Deconstructor (resetting internal variables) is now only
    implemented if SHA1_WIPE_VARIABLES is defined (which is the
    default).
  - Renamed inclusion guard to contain a GUID.
  - Demo application is now using C++/STL objects and functions.
  - Unicode build of the demo application now outputs the hashes of both
    the ANSI and Unicode representations of strings.
  - Various other demo application improvements.

  Version 2.0 - 2012-06-14
  - Added 'limits.h' include.
  - Renamed inclusion guard and macros for compliancy (names beginning
    with an underscore are reserved).

  Version 1.9 - 2011-11-10
  - Added Unicode test vectors.
  - Improved support for hashing files using the HashFile method that
    are larger than 4 GB.
  - Improved file hashing performance (by using a larger buffer).
  - Disabled unnecessary compiler warnings.
  - Internal variables are now private.

  Version 1.8 - 2009-03-16
  - Converted project files to Visual Studio 2008 format.
  - Added Unicode support for HashFile utility method.
  - Added support for hashing files using the HashFile method that are
    larger than 2 GB.
  - HashFile now returns an error code instead of copying an error
    message into the output buffer.
  - GetHash now returns an error code and validates the input parameter.
  - Added ReportHashStl STL utility method.
  - Added REPORT_HEX_SHORT reporting mode.
  - Improved Linux compatibility of test program.

  Version 1.7 - 2006-12-21
  - Fixed buffer underrun warning that appeared when compiling with
    Borland C Builder (thanks to Rex Bloom and Tim Gallagher for the
    patch).
  - Breaking change: ReportHash writes the final hash to the start
    of the buffer, i.e. it's not appending it to the string anymore.
  - Made some function parameters const.
  - Added Visual Studio 2005 project files to demo project.

  Version 1.6 - 2005-02-07 (thanks to Howard Kapustein for patches)
  - You can set the endianness in your files, no need to modify the
    header file of the CSHA1 class anymore.
  - Aligned data support.
  - Made support/compilation of the utility functions (ReportHash and
    HashFile) optional (useful when bytes count, for example in embedded
    environments).

  Version 1.5 - 2005-01-01
  - 64-bit compiler compatibility added.
  - Made variable wiping optional (define SHA1_WIPE_VARIABLES).
  - Removed unnecessary variable initializations.
  - ROL32 improvement for the Microsoft compiler (using _rotl).

  Version 1.4 - 2004-07-22
  - CSHA1 now compiles fine with GCC 3.3 under Mac OS X (thanks to Larry
    Hastings).

  Version 1.3 - 2003-08-17
  - Fixed a small memory bug and made a buffer array a class member to
    ensure correct working when using multiple CSHA1 class instances at
    one time.

  Version 1.2 - 2002-11-16
  - Borlands C++ compiler seems to have problems with string addition
    using sprintf. Fixed the bug which caused the digest report function
    not to work properly. CSHA1 is now Borland compatible.

  Version 1.1 - 2002-10-11
  - Removed two unnecessary header file includes and changed BOOL to
    bool. Fixed some minor bugs in the web page contents.

  Version 1.0 - 2002-06-20
  - First official release.

  ================ Test Vectors ================

  SHA1("abc" in ANSI) =
    A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D
  SHA1("abc" in Unicode LE) =
    9F04F41A 84851416 2050E3D6 8C1A7ABB 441DC2B5

  SHA1("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
    in ANSI) =
    84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1
  SHA1("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
    in Unicode LE) =
    51D7D876 9AC72C40 9C5B0E3F 69C60ADC 9A039014

  SHA1(A million repetitions of "a" in ANSI) =
    34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
  SHA1(A million repetitions of "a" in Unicode LE) =
    C4609560 A108A0C6 26AA7F2B 38A65566 739353C5
*/

#ifndef SHA1_H_A545E61D43E9404E8D736869AB3CBFE7
#define SHA1_H_A545E61D43E9404E8D736869AB3CBFE7

#if !defined(SHA1_UTILITY_FUNCTIONS) && !defined(SHA1_NO_UTILITY_FUNCTIONS)
#define SHA1_UTILITY_FUNCTIONS
#endif

#if !defined(SHA1_STL_FUNCTIONS) && !defined(SHA1_NO_STL_FUNCTIONS)
#define SHA1_STL_FUNCTIONS
#if !defined(SHA1_UTILITY_FUNCTIONS)
#error STL functions require SHA1_UTILITY_FUNCTIONS.
#endif
#endif

#include <memory.h>
#include <limits.h>

#ifdef SHA1_UTILITY_FUNCTIONS
#include <stdio.h>
#include <string.h>
#endif

#ifdef SHA1_STL_FUNCTIONS
#include <string>
#endif

#ifdef _MSC_VER
#include <stdlib.h>
#endif

// You can define the endian mode in your files without modifying the SHA-1
// source files. Just #define SHA1_LITTLE_ENDIAN or #define SHA1_BIG_ENDIAN
// in your files, before including the SHA1.h header file. If you don't
// define anything, the class defaults to little endian.
#if !defined(SHA1_LITTLE_ENDIAN) && !defined(SHA1_BIG_ENDIAN)
#define SHA1_LITTLE_ENDIAN
#endif

// If you want variable wiping, #define SHA1_WIPE_VARIABLES, if not,
// #define SHA1_NO_WIPE_VARIABLES. If you don't define anything, it
// defaults to wiping.
#if !defined(SHA1_WIPE_VARIABLES) && !defined(SHA1_NO_WIPE_VARIABLES)
#define SHA1_WIPE_VARIABLES
#endif

#if defined(SHA1_HAS_TCHAR)
#include <tchar.h>
#else
#ifdef _MSC_VER
#include <tchar.h>
#else
#ifndef TCHAR
#define TCHAR char
#endif
#ifndef _T
#define _T(__x) (__x)
#define _tmain main
#define _tprintf printf
#define _getts gets
#define _tcslen strlen
#define _tfopen fopen
#define _tcscpy strcpy
#define _tcscat strcat
#define _sntprintf snprintf
#endif
#endif
#endif

///////////////////////////////////////////////////////////////////////////
// Define variable types

#ifndef UINT_8
#ifdef _MSC_VER // Compiling with Microsoft compiler
#define UINT_8 unsigned __int8
#else // !_MSC_VER
#define UINT_8 unsigned char
#endif // _MSC_VER
#endif

#ifndef UINT_32
#ifdef _MSC_VER // Compiling with Microsoft compiler
#define UINT_32 unsigned __int32
#else // !_MSC_VER
#if (ULONG_MAX == 0xFFFFFFFFUL)
#define UINT_32 unsigned long
#else
#define UINT_32 unsigned int
#endif
#endif // _MSC_VER
#endif // UINT_32

#ifndef INT_64
#ifdef _MSC_VER // Compiling with Microsoft compiler
#define INT_64 __int64
#else // !_MSC_VER
#define INT_64 long long
#endif // _MSC_VER
#endif // INT_64

#ifndef UINT_64
#ifdef _MSC_VER // Compiling with Microsoft compiler
#define UINT_64 unsigned __int64
#else // !_MSC_VER
#define UINT_64 unsigned long long
#endif // _MSC_VER
#endif // UINT_64

///////////////////////////////////////////////////////////////////////////
// Declare SHA-1 workspace

typedef union
{
	UINT_8 c[64];
	UINT_32 l[16];
} SHA1_WORKSPACE_BLOCK;

class CSHA1
{
public:
#ifdef SHA1_UTILITY_FUNCTIONS
	// Different formats for ReportHash(Stl)
	enum REPORT_TYPE
	{
		REPORT_HEX = 0,
		REPORT_DIGIT = 1,
		REPORT_HEX_SHORT = 2
	};
#endif

	// Constructor and destructor
	CSHA1();

#ifdef SHA1_WIPE_VARIABLES
	~CSHA1();
#endif

	void Reset();

	// Hash in binary data and strings
	void Update(const UINT_8* pbData, UINT_32 uLen);

#ifdef SHA1_UTILITY_FUNCTIONS
	// Hash in file contents
	bool HashFile(const TCHAR* tszFileName);
#endif

	// Finalize hash; call it before using ReportHash(Stl)
	void Final();

#ifdef SHA1_UTILITY_FUNCTIONS
	bool ReportHash(TCHAR* tszReport, REPORT_TYPE rtReportType = REPORT_HEX) const;
#endif

#ifdef SHA1_STL_FUNCTIONS
	bool ReportHashStl(std::basic_string<TCHAR>& strOut, REPORT_TYPE rtReportType =
		REPORT_HEX) const;
#endif

	// Get the raw message digest (20 bytes)
	bool GetHash(UINT_8* pbDest20) const;

private:
	// Private SHA-1 transformation
	void Transform(UINT_32* pState, const UINT_8* pBuffer);

	// Member variables
	UINT_32 m_state[5];
	UINT_32 m_count[2];
	UINT_32 m_reserved0[1]; // Memory alignment padding
	UINT_8 m_buffer[64];
	UINT_8 m_digest[20];
	UINT_32 m_reserved1[3]; // Memory alignment padding

	UINT_8 m_workspace[64];
	SHA1_WORKSPACE_BLOCK* m_block; // SHA1 pointer to the byte array above
};

#endif // SHA1_H_A545E61D43E9404E8D736869AB3CBFE7
