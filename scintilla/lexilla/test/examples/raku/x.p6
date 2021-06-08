use v6;

# Normal single line comment
my Int $i = 0;
my Rat $r = 3.142;
my Str $s = "Hello, world! \$i == $i and \$r == $r";
say $s;

#`{{
*** This is a multi-line comment ***
}}

my @array = #`[[ inline comment ]] <f fo foo food>;
my %hash = ( AAA => 1, BBB => 2 );

say q[This back\slash stays];
say q[This back\\slash stays]; # Identical output
say Q:q!Just a literal "\n" here!;

=begin pod
POD Documentation...
=end pod

say qq:to/END/;
A multi-line
string with interpolated vars: $i, $r
END

sub function {
	return q:to/END/;
Here is
some multi-line
string
END
}

my $func = &function;
say $func();

grammar Calculator {
	token TOP					{ <calc-op> }
	proto rule calc-op			{*}
		  rule calc-op:sym<add>	{ <num> '+' <num> }
		  rule calc-op:sym<sub>	{ <num> '-' <num> }
    token num					{ \d+ }
}

class Calculations {
	method TOP              ($/) { make $<calc-op>.made; }
	method calc-op:sym<add> ($/) { make [+] $<num>; }
	method calc-op:sym<sub> ($/) { make [-] $<num>; }
}

say Calculator.parse('2 + 3', actions => Calculations).made;
