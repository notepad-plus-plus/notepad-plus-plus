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
