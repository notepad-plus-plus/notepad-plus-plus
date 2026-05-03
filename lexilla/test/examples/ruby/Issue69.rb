# -*- coding: utf-8 -*-

# single character strings
puts ?a
puts ?\n
puts ?\s
puts ?\\#
puts ?\u{41}
puts ?\C-a
puts ?\M-a
puts ?\M-\C-a
puts ?\C-\M-a
puts ?あ
puts ?"
puts ?/
puts ?[[1, 2]
puts ?/\

puts ?\176.ord
puts ?\x7A.ord
puts ?\uAB12.ord
puts ?\u{A}.ord
puts ?\u{012345}.ord
puts ?\cA.bytes
puts ?\c\x41.bytes
puts ?\C-\x41.bytes
puts ?\M-\x41.bytes
puts ?\M-\C-\x41.bytes
puts ?\M-\c\x41.bytes
puts ?\c\M-\x41.bytes
puts ?\c?.bytes
puts ?\C-?.bytes

# symbol and ternary operator
ab = /\d+/
cd = /\w+/
puts :ab, :cd, :/, :[]
puts :/\

# TODO: space after '?' and ':' is not needed
puts true ?ab : cd
puts true ? /\d+/ : /\w+/
puts false ?ab : cd
puts false ? /\d+/ : /\w+/

# Issue #355
puts ?\あ.ord
puts ?\
.ord
# invalid, test out-of-bounds styling at document end
puts ?\