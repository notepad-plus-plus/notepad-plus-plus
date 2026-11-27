// Test JavaScript line continuation option for issue 55.
// JavaScript, unlike C++, only allows line continuation inside string literals.

if( test ) // toto \
{
	alert();
}
else
{
}

// Multi-line string literals usin \ to continue to next line
// string=6
"str\
ing"

// character=7
'c\
d'
