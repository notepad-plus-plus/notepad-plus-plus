# heredoc method call, other argument
puts <<~EOT.chomp
	squiggly heredoc
EOT

puts <<ONE, __FILE__, __LINE__
content for heredoc one
ONE

# heredoc prevStyle == SCE_RB_GLOBAL
$stdout.puts <<~EOT.chomp
	squiggly heredoc
EOT

# Issue #236: modifier if, unless, while and until
alias error puts

error <<EOF if true
heredoc if true
EOF

error <<EOF unless false
heredoc unless false
EOF

error <<EOF while false
heredoc while false
EOF

error <<EOF until true
heredoc until true
EOF
