# encoding: utf-8
puts <<A中
#{1+2}
A中

puts <<中
#{1+2}
中

def STDERR::error(x) = puts(x)
def STDERR.error(x) = puts(x)

STDERR.error <<EOF
STDERR heredoc
EOF
