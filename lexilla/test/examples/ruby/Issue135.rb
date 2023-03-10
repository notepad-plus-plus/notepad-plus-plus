a = <<XXX # :nodoc:
heredoc
XXX

puts(<<-ONE, <<-TWO)
content for heredoc one
ONE
content for heredoc two
TWO
