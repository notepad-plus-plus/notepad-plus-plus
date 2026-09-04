varname = 1
# identifier in first line was mis-styled as attribute when bug existed

# left comment ends with period.  this was okay before bug.
varname = 2

x = 1 # comment after code ends with period. this failed when bug existed.
varname = 3

def dummy():
    # indented comment ends with period.this failed when bug existed.
    varname = 4

x = 2 ## comment block is the same.
varname = 5

# per issue#294, identifiers were mis-styled as attributes when at beginning of file
# and when after a non-left-justifed comment
