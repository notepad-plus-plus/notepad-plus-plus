# For issue #390.
# Demonstrate inconsistent CRLF line ends where \r is in one style and \n a different style.
# Lines 6 and 11 show the problem.

# Heredoc SCE_PL_HERE_Q
print <<'EOF'
a
EOF

# Format data SCE_PL_FORMAT
format IDENT=
.

