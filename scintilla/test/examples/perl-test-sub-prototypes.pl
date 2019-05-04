# -*- coding: utf-8 -*-
#--------------------------------------------------------------------------
# perl-test-sub-prototypes.pl
#--------------------------------------------------------------------------
# compiled all relevant subroutine prototype test cases
#
#--------------------------------------------------------------------------
# Kein-Hong Man <keinhong@gmail.com> Public Domain
#--------------------------------------------------------------------------
# 20151227	initial document
#--------------------------------------------------------------------------

#--------------------------------------------------------------------------
# test cases for sub syntax scanner
#--------------------------------------------------------------------------
# sub syntax: simple and with added module notation
#--------------------------------------------------------------------------

sub fish($) { 123; }
sub fish::chips($) { 123; }			# module syntax
sub fish::chips::sauce($) { 123; }		# multiple module syntax

sub fish :: chips  ::		sauce ($) { 123; }	# added whitespace

sub fish :: # embedded comment
chips 	# embedded comment
 :: sauce ($) { 123; }

sub fish :: ($) { 123; }	# incomplete or bad syntax examples
sub fish :: 123 ($) { 123; }
sub fish :: chips 123 ($) { 123; }
sub 123 ($) { 123; }

#--------------------------------------------------------------------------
# sub syntax: prototype attributes
#--------------------------------------------------------------------------

sub fish:prototype($) { 123; }
sub fish : prototype	($) { 123; }		# added whitespace

sub fish:salted($) { 123; }	# wrong attribute example (must use 'prototype')
sub fish :  123($) { 123; }	# illegal attribute
sub fish:prototype:salted($) { 123; }	# wrong 'prototype' position
sub fish:salted salt:prototype($) { 123; }	# wrong attribute syntax

sub fish:const:prototype($) { 123; }		# extra attributes
sub fish:const:lvalue:prototype($) { 123; }
sub fish:const:prototype($):lvalue{ 123; }	# might be legal too
sub fish  :const	:prototype($) { 123; }	# extra whitespace

sub fish  :const	# embedded comment: a constant sub
:prototype		# embedded comment
($) { 123; }

#--------------------------------------------------------------------------
# sub syntax: mixed
#--------------------------------------------------------------------------

sub fish::chips:prototype($) { 123; }
sub fish::chips::sauce:prototype($) { 123; }
sub fish  ::chips  ::sauce	:prototype($) { 123; }	# +whitespace

sub fish::chips::sauce:const:prototype($) { 123; }
sub fish::chips::sauce	:const	:prototype($) { 123; }	# +whitespace

sub fish		# embedded comment
::chips	::sauce		# embedded comment
  : const		# embedded comment
	: prototype ($) { 123; }

# wrong syntax examples, parentheses must follow ':prototype'
sub fish :prototype :const ($) { 123;}
sub fish :prototype ::chips ($) { 123;}

#--------------------------------------------------------------------------
# perl-test-5200delta.pl
#--------------------------------------------------------------------------
# More consistent prototype parsing
#--------------------------------------------------------------------------
# - whitespace now allowed, lexer now allows spaces or tabs

sub foo ( $ $ ) {}
sub foo ( 			 ) {}		# spaces/tabs empty
sub foo (  *  ) {}
sub foo (@	) {}
sub foo (	%) {}

# untested, should probably be \[ but scanner does not check this for now
sub foo ( \ [ $ @ % & * ] ) {}

#--------------------------------------------------------------------------
# perl-test-5140delta.pl
#--------------------------------------------------------------------------
# new + prototype character, acts like (\[@%])
#--------------------------------------------------------------------------

# these samples work as before
sub mylink ($$)          # mylink $old, $new
sub myvec ($$$)          # myvec $var, $offset, 1
sub myindex ($$;$)       # myindex &getstring, "substr"
sub mysyswrite ($$$;$)   # mysyswrite $buf, 0, length($buf) - $off, $off
sub myreverse (@)        # myreverse $a, $b, $c
sub myjoin ($@)          # myjoin ":", $a, $b, $c
sub myopen (*;$)         # myopen HANDLE, $name
sub mypipe (**)          # mypipe READHANDLE, WRITEHANDLE
sub mygrep (&@)          # mygrep { /foo/ } $a, $b, $c
sub myrand (;$)          # myrand 42
sub mytime ()            # mytime

# backslash group notation to specify more than one allowed argument type
sub myref (\[$@%&*]) {}

sub mysub (_)            # underscore can be optionally used FIXED 20151211

# these uses the new '+' prototype character
sub mypop (+)            # mypop @array
sub mysplice (+$$@)      # mysplice @array, 0, 2, @pushme
sub mykeys (+)           # mykeys %{$hashref}

#--------------------------------------------------------------------------
# perl-test-5200delta.pl
#--------------------------------------------------------------------------
# Experimental Subroutine signatures (mostly works)
#--------------------------------------------------------------------------
# INCLUDED FOR COMPLETENESS ONLY
# IMPORTANT NOTE the subroutine prototypes lexing implementation has
# no effect on subroutine signature syntax highlighting

# subroutine signatures mostly looks fine except for the @ and % slurpy
# notation which are highlighted as operators (all other parameters are
# highlighted as vars of some sort), a minor aesthetic issue

use feature 'signatures';

sub foo ($left, $right) {		# mandatory positional parameters
    return $left + $right;
}
sub foo ($first, $, $third) {		# ignore second argument
    return "first=$first, third=$third";
}
sub foo ($left, $right = 0) {		# optional parameter with default value
    return $left + $right;
}
my $auto_id = 0;			# default value expression, evaluated if default used only
sub foo ($thing, $id = $auto_id++) {
    print "$thing has ID $id";
}
sub foo ($first_name, $surname, $nickname = $first_name) {	# 3rd parm may depend on 1st parm
    print "$first_name $surname is known as \"$nickname\"";
}
sub foo ($thing, $ = 1) {		# nameless default parameter
    print $thing;
}
sub foo ($thing, $=) {			# (this does something, I'm not sure what...)
    print $thing;
}
sub foo ($filter, @inputs) {		# additional arguments (slurpy parameter)
    print $filter->($_) foreach @inputs;
}
sub foo ($thing, @) {			# nameless slurpy parameter FAILS for now
    print $thing;
}
sub foo ($filter, %inputs) {		# slurpy parameter, hash type
    print $filter->($_, $inputs{$_}) foreach sort keys %inputs;
}
sub foo ($thing, %) {			# nameless slurpy parm, hash type FAILS for now
    print $thing;
}
sub foo () {				# empty signature no arguments (styled as prototype)
    return 123;
}

#--------------------------------------------------------------------------
# perl-test-5200delta.pl
#--------------------------------------------------------------------------
# subs now take a prototype attribute
#--------------------------------------------------------------------------

sub foo :prototype($) { $_[0] }

sub foo :prototype($$) ($left, $right) {
    return $left + $right;
}

sub foo : prototype($$){}		# whitespace allowed

# additional samples from perl-test-cases.pl with ':prototype' added:
sub mylink :prototype($$) {}		sub myvec :prototype($$$) {}
sub myindex :prototype($$;$) {}		sub mysyswrite :prototype($$$;$) {}
sub myreverse :prototype(@) {}		sub myjoin :prototype($@) {}
sub mypop :prototype(\@) {}		sub mysplice :prototype(\@$$@) {}
sub mykeys :prototype(\%) {}		sub myopen :prototype(*;$) {}
sub mypipe :prototype(**) {}		sub mygrep :prototype(&@) {}
sub myrand :prototype($) {}		sub mytime :prototype() {}
# backslash group notation to specify more than one allowed argument type
sub myref :prototype(\[$@%&*]) {}

# additional attributes may complicate scanning for prototype syntax,
# for example (from https://metacpan.org/pod/perlsub):
# Lvalue subroutines

my $val;
sub canmod : lvalue {
    $val;  # or:  return $val;
}
canmod() = 5;   # assigns to $val

#--------------------------------------------------------------------------
# perl-test-5220delta.pl
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
sub Y::z : lvalue { 1 }

package X;
sub foo { 1 }
package Y;
BEGIN { *bar = \&X::foo; }
package Z;
sub Y::bar : lvalue ;

# built-in attributes for subroutines:
lvalue method prototype(..) locked const

#--------------------------------------------------------------------------
# end of test file
#--------------------------------------------------------------------------
