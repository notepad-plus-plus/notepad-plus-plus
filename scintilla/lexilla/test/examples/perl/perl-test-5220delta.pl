# -*- coding: utf-8 -*-
#--------------------------------------------------------------------------
# perl-test-5220delta.pl
#--------------------------------------------------------------------------
# REF: https://metacpan.org/pod/distribution/perl/pod/perldelta.pod
# maybe future ref: https://metacpan.org/pod/distribution/perl/pod/perl5220delta.pod
# also: http://perltricks.com/article/165/2015/4/10/A-preview-of-Perl-5-22
#
#--------------------------------------------------------------------------
# Kein-Hong Man <keinhong@gmail.com> Public Domain 20151217
#--------------------------------------------------------------------------
# 20151217	initial document
# 20151218	updated tests and comments
#--------------------------------------------------------------------------

use v5.22;			# may be needed

#--------------------------------------------------------------------------
# New bitwise operators
#--------------------------------------------------------------------------

use feature 'bitwise'		# enable feature, warning enabled
use experimental "bitwise";	# enable feature, warning disabled

# numerical operands
10&20  10|20   10^20 ~10
$a&"8" $a|"8" $a^"8" ~$a ~"8"

# string operands
'0'&."8" '0'|."8" '0'^."8" ~.'0' ~."8"
# the following is AMBIGUOUS, perl sees 10 and not .10 only when bitwise feature is enabled
# so it's feature-setting-dependent, no plans to change current behaviour
 $a&.10   $a|.10   $a^.10  ~.$a  ~.10

# assignment variants
$a&=10;    $a|=10;    $a^=10;
$b&.='20'; $b|.='20'; $b^.='20';
$c&="30";  $c|="30";  $c^="30";
$d&.=$e;   $d|.=$e;   $d^.=$e;

#--------------------------------------------------------------------------
# New double-diamond operator
#--------------------------------------------------------------------------
# <<>> is like <> but each element of @ARGV will be treated as an actual file name

# example snippet from brian d foy's blog post
while( <<>> ) {  # new, safe line input operator
	...;
	}

#--------------------------------------------------------------------------
# New \b boundaries in regular expressions
#--------------------------------------------------------------------------

qr/\b{gcb}/
qr/\b{wb}/
qr/\b{sb}/

#--------------------------------------------------------------------------
# Non-Capturing Regular Expression Flag
#--------------------------------------------------------------------------
# disables capturing and filling in $1, $2, etc

"hello" =~ /(hi|hello)/n; # $1 is not set

#--------------------------------------------------------------------------
# Aliasing via reference
#--------------------------------------------------------------------------
# Variables and subroutines can now be aliased by assigning to a reference

\$c = \$d;
\&x = \&y;

# Aliasing can also be applied to foreach iterator variables

foreach \%hash (@array_of_hash_refs) { ... }

# example snippet from brian d foy's blog post

use feature qw(refaliasing);

\%other_hash = \%hash;

use v5.22;
use feature qw(refaliasing);

foreach \my %hash ( @array_of_hashes ) { # named hash control variable
	foreach my $key ( keys %hash ) { # named hash now!
		...;
		}
	}

#--------------------------------------------------------------------------
# New :const subroutine attribute
#--------------------------------------------------------------------------

my $x = 54321;
*INLINED = sub : const { $x };
$x++;

# more examples of attributes
# (not 5.22 stuff, but some general examples for study, useful for
#  handling subroutine signature and subroutine prototype highlighting)

sub foo : lvalue ;

package X;
sub Y::x : lvalue { 1 }

package X;
sub foo { 1 }
package Y;
BEGIN { *bar = \&X::foo; }
package Z;
sub Y::bar : lvalue ;

# built-in attributes for subroutines:
lvalue method prototype(..) locked const

#--------------------------------------------------------------------------
# Repetition in list assignment
#--------------------------------------------------------------------------

# example snippet from brian d foy's blog post
use v5.22;
my(undef, $card_num, (undef)x3, $count) = split /:/;

(undef,undef,$foo) = that_function()
# is equivalent to 
((undef)x2, $foo) = that_function()

#--------------------------------------------------------------------------
# Floating point parsing has been improved
#--------------------------------------------------------------------------
# Hexadecimal floating point literals

# some hex floats from a program by Rick Regan
# appropriated and extended from Lua 5.2.x test cases
# tested on perl 5.22/cygwin

0x1p-1074;
0x3.3333333333334p-5;
0xcc.ccccccccccdp-11;
0x1p+1;
0x1p-6;
0x1.b7p-1;
0x1.fffffffffffffp+1023;
0x1p-1022;
0X1.921FB4D12D84AP+1;
0x1.999999999999ap-4;

# additional test cases for characterization
0x1p-1074.		# dot is a string operator
0x.ABCDEFp10		# legal, dot immediately after 0x
0x.p10			# perl allows 0x as a zero, then concat with p10 bareword
0x.p 0x0.p		# dot then bareword
0x_0_.A_BC___DEF_p1_0	# legal hex float, underscores are mostly allowed
0x0._ABCDEFp10		# _ABCDEFp10 is a bareword, no underscore allowed after dot

# illegal, but does not use error highlighting
0x0p1ABC		# illegal, highlighted as 0x0p1 abut with bareword ABC 

# allowed to FAIL for now
0x0.ABCDEFp_10		# ABCDEFp_10 is a bareword, '_10' exponent not allowed
0xp 0xp1 0x0.0p		# syntax errors
0x41.65.65 		# hex dot number, but lexer now fails with 0x41.65 left as a partial hex float

#--------------------------------------------------------------------------
# Support for ?PATTERN? without explicit operator has been removed
#--------------------------------------------------------------------------
# ?PATTERN? must now be written as m?PATTERN?

?PATTERN?	# does not work in current LexPerl anyway, NO ACTION NEEDED
m?PATTERN?

#--------------------------------------------------------------------------
# end of test file
#--------------------------------------------------------------------------
