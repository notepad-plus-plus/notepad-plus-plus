# Enumerate all valid styles: 0-7 10-15 17-18 20-31 40-44 54-55 57 61-62 64-66 
# Holes at 8-9,16,19,32-39,45-53,56,58-60,63

# 2=SCE_PL_COMMENTLINE comment
# comment=2

# 0=SCE_PL_DEFAULT white space
	# w

# 1=SCE_PL_ERROR error
019
# Bad digit in octal number

# 3=SCE_PL_POD pod: = at beginning of line
=pod start
=cut

# 4=SCE_PL_NUMBER number
67.89

# 5=SCE_PL_WORD keyword
wait

# 6=SCE_PL_STRING double quoted string
"String double quoted"

# 7=SCE_PL_CHARACTER single quoted string
'String single quoted'

# 8=SCE_PL_PUNCTUATION symbols / punctuation. currently not used
# Not present

# 9=SCE_PL_PREPROCESSOR preprocessor. currently not used
# Not present

# 10=SCE_PL_OPERATOR operators
a = 1
+ 4;

# 11=SCE_PL_IDENTIFIER identifiers (functions, etc.)
identifier

# 12=SCE_PL_SCALAR scalars: $var
$scalar

# 13=SCE_PL_ARRAY array: @var
@array;

# 14=SCE_PL_HASH hash: %var
%hash;

# 15=SCE_PL_SYMBOLTABLE symbol table: *var
*symboltable

# 16=SCE_PL_VARIABLE_INDEXER sce_pl_variable_indexer allocated but unused

# 17=SCE_PL_REGEX regex: /re/ or m{re}
m{re}

# 18=SCE_PL_REGSUBST substitution: s/re/ore/
s/re/ore/

# 19=SCE_PL_LONGQUOTE obsolete: replaced by qq, qx, qr, qw

# 20=SCE_PL_BACKTICKS back ticks
`interpolated string`

# 21 is at file end as __DATA__ affects everything after

# 22=SCE_PL_HERE_DELIM here-doc (delimiter)
<<

# 23=SCE_PL_HERE_Q here-doc (single quoted, q)
<<'EOF'
a
EOF

# 24=SCE_PL_HERE_QQ here-doc (double quoted, qq)
<<"EOF"
a
EOF

# 25=SCE_PL_HERE_QX here-doc (back ticks, qx)
<<`EOF`
a
EOF

# 26=SCE_PL_STRING_Q single quoted string, generic
q(a)

# 27=SCE_PL_STRING_QQ qq = double quoted string
qq(abc)

# 28=SCE_PL_STRING_QX qx = back ticks
qx(abc)

# 29=SCE_PL_STRING_QR qr = regex
qr(a)

# 30=SCE_PL_STRING_QW qw = array
qw(a b c)

# 31=SCE_PL_POD_VERB pod: verbatim paragraphs
=pod

  Verbatim
  explanation of verbatims.

=cut

# 32 predefined
# 33 predefined
# 34 predefined
# 35 predefined
# 36 predefined
# 37 predefined
# 38 predefined
# 39 predefined

# 40=SCE_PL_SUB_PROTOTYPE subroutine prototype
sub f($)
{ 1; }

# 41=SCE_PL_FORMAT_IDENT format identifier
format IDENT =
.

# 42=SCE_PL_FORMAT format body
format DATA = 
@<<<<<<<<<<<<<<<@<<<<<<<<<<<<<<<@<<<<<<<<<<<<<<<@<<<<<<<<<<<<<<<
$First_Name[$k] $Last_Name[$k]  $ID[$k]         $Phone_Number[$k]
 
.

# 43=SCE_PL_STRING_VAR double quoted string (interpolated variable)
"
$var
"

# 44=SCE_PL_XLAT translation: tr{}{} y{}{}
tr{}{}

# 45 unused
# 46 unused
# 47 unused
# 48 unused
# 49 unused
# 50 unused
# 51 unused
# 52 unused
# 53 unused

# 54=SCE_PL_REGEX_VAR regex: /re/ or m{re} (interpolated variable)
m{re
$var
}

# 55=SCE_PL_REGSUBST_VAR substitution: s/re/ore/ (interpolated variable)
s/re/
$var
/

# 56 unused

# 57=SCE_PL_BACKTICKS_VAR back ticks (interpolated variable)
`
$var
`

# 58 unused
# 59 unused
# 60 unused

# 61=SCE_PL_HERE_QQ_VAR here-doc (double quoted, qq) (interpolated variable)
a = <<"EOF"
$var
EOF

# 62=SCE_PL_HERE_QX_VAR here-doc (back ticks, qx) (interpolated variable)
<<`EOF`
$var
EOF

# 63 unused

# 64=SCE_PL_STRING_QQ_VAR qq = double quoted string (interpolated variable)
qq(
$var
)

# 65=SCE_PL_STRING_QX_VAR qx = back ticks (interpolated variable)
qx(
$var
)

# 66=SCE_PL_STRING_QR_VAR qr = regex (interpolated variable)
qr(
$var
)

# 21=SCE_PL_DATASECTION data section: __data__ or __end__ at beginning of line
__DATA__
Data Section
