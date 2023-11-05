# https://cran.r-project.org/doc/manuals/r-release/R-lang.html#Reserved-words
if

# base keyword (3)
abbreviate

# other keyword (4)
acme

# infix operator
# https://cran.r-project.org/doc/manuals/r-release/R-lang.html#Special-operators
%x%

# https://cran.r-project.org/doc/manuals/r-release/R-lang.html#Literal-constants
# Valid integer constants
1L, 0x10L, 1000000L, 1e6L

# Valid numeric constants
1 10 0.1 .2 1e-7 1.2e+7
1.1L, 1e-3L, 0x1.1p-2

# Valid complex constants
2i 4.1i 1e-2i

# https://search.r-project.org/R/refmans/base/html/Quotes.html
# single quotes
'"It\'s alive!", he screamed.'

# double quotes
"\"It's alive!\", he screamed."

# escape sequence
"\n0\r1\t2\b3\a4\f5\\6\'7\"8\`9"
"\1230\x121\u12342\U000123453\u{1234}4\U{00012345}5\
6\ 7"
# issue #206
"\n"
"\r\n"

# Backticks
d$`1st column`

# double quoted raw string
r"---(\1--)-)---"

# single quoted raw string
R'---(\1--)-)---'

# infix EOL (11)
%a
#back to comment
