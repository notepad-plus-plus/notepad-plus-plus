a = {r: /\w+/, h: <<EOF
heredoc
EOF
}

puts a

def b # :nodoc:
<<EOF
heredoc
EOF
end

def c # :nodoc:
/\w+/
end

puts b
puts c

$stdout . puts <<EOF
heredoc
EOF
