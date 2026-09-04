#!/bin/zsh
# Tests for zsh extensions
# Can be executed by zsh with reasonable results
# Some of these were implemented by commit [87286d] for Scintilla bug #1794
# https://zsh.sourceforge.io/Doc/Release/Expansion.html

# Where # does not start a comment


## Formatting base
print $(( [#8] y = 33 ))
print $(( [##8] 32767 ))

# Formatting base and grouping
print $(( [#16_4] 65536 ** 2 ))


## Character values
print $(( ##T+0 ))
print $(( ##^G+0 ))
# Failure: does not work when - included for bindkey syntax. \M-\C-x means Meta+Ctrl+x.
print $(( ##\M-\C-x+0 ))

# Value of first character of variable in expression
var=Tree
print $(( #var+0 ))


## Extended glob
setopt extended_glob

# # is similar to *, ## similar to +
echo [A-Za-z]#.bsh
echo [A-Za-z]##.bsh

# 13 character file names
echo **/[a-zA-Z.](#c13)
# 13-15 character file names
echo **/[a-zA-Z.](#c13,15)


## Glob flag

# i=case-insensitive
echo (#i)a*

# b=back-references
foo="a_string_with_a_message"
if [[ $foo = (a|an)_(#b)(*) ]]; then
  print ${foo[$mbegin[1],$mend[1]]}
fi
