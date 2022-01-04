// A demonstration program
#include <stdio.h>
#if 0 /* */
#define DUMMY() \
	if (1);
#endif

// Test preprocessor expressions with parentheses 
#if ((0))
a
#elif ((1))
b
#endif

/** @file LexCPP.cxx
 <file>
 <file >filename</file>
 LexCPP.cxx.
 </file>
 **/

/** Unknown doc keywords so in SCE_C_COMMENTDOCKEYWORDERROR:
 @wrong LexCPP.cxx
 <wrong>filename</wrong>
**/

#define M\

\
 
int main() {
	double x[] = {3.14159,6.02e23,1.6e-19,1.0+1};
	int y[] = {75,0113,0x4b};
	printf("hello world %d %g\n", y[0], x[0]);

	// JavaScript regular expression (14) tests
	let a = /a/;
	let b = /[a-z]+/gi;

	// Escape sequence (27) tests
	printf("\'\"\?\\\a\b\f\n\r\t\v \P");
	printf("\0a \013a \019");
	printf("\x013ac \xdz");
	printf("\ua34df \uz");
	printf("\Ua34df7833 \Uz");
}
