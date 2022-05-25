# Enumerate all styles where possible: 0..31,40..41
# 22,23,30,31,40,41 are never set and 1 switches rest of file to error state

#0 whitespace
    #
	#

#1:error, can be set with a heredoc delimiter >256 characters but that can't be recovered from
#<<ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789

#2:comment line

#3:POD
=begin
3:POD
=end

#4:number
4

#5:word
super

#6:string
"6:double quotes"

#7:single quoted string
'7:single quotes'

#8:class name
class ClassName end

#9:def name
def Function end

#10:operator
&

#11:identifier
identifier

#12:regex
/[12a-z]/

#13:global
$global13

#14:symbol
:symbol14

#15:module name
module Module15 end

#16:instance var
@instance16

#17:class var
@@class17

#18:back ticks
`18`

#19:data section at end of file

#20:here delimiter
<<DELIMITER20
DELIMITER20

#21:here doc
<<D
21:here doc
D

#22:here qq never set

#23:here qw never set

#24:q quoted string
%q!24:quotes's!

#25:Q quoted string
%Q!25:quotes"s!

#26:executed string
%x(echo 26)

#27:regex
%r(27[a-z]/[A-Z]+)

#28:string array
%w(28 cgi.rb complex.rb date.rb)

#29:demoted keyword do
while 1 do end

# 30,31,40,41 never set

#19:data section
__END__

