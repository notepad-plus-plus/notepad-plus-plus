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
