# Simple nesting
f" { "" } "

# Multi-line field with comment
f" {

"" # comment

} "

# Single quoted continued with \
f" \
"

# 4 nested f-strings
f'Outer {f"nested {1} {f"nested {2} {f"nested {3} {f"nested {4}"}"}"}"}'
