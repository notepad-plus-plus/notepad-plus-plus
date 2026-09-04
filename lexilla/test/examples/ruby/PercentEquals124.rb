# Issue 124, disambiguating %= which may be a quote or modulo assignment
# %-quoting with '=' as the quote
s = %=3=
puts s
x = 7
# Modulo assignment, equivalent to x = x % 2
x %=2
puts x
