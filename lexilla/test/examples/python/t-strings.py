# Simple nesting
t" { "" } "

# Multi-line field with comment
t" {

"" # comment

} "

# Single quoted continued with \
t" \
"

# 4 nested t-strings
t'Outer {t"nested {1} {t"nested {2} {t"nested {3} {t"nested {4}"}"}"}"}'
