-- Enumerate all styles: 0 to 20
-- 3 (comment doc) is not currently produced by lexer

--[[ comment=1 ]]

--[[ whitespace=0 ]]
	-- w

-- comment line=2

--- comment doc=3
-- still comment doc

-- still comment doc
3	-- comment doc broken only by code

-- number=4
37

-- keyword=5
local a

-- double-quoted-string=6
"str"

-- single-quoted-string=7
'str'

-- literal string=8
[[ literal ]]

-- unused preprocessor=9
$if

-- operator=10
*

-- identifier=11
identifier=1

-- string EOL=12
"unclosed

-- keyword 2=13
print

-- keyword 3=14
keyword3

-- keyword 4=15
keyword4

-- keyword 5=16
keyword5

-- keyword 6=17
keyword6

-- keyword 7=18
keyword7

-- keyword 8=19
keyword8

-- label=20
::label::

-- identifier substyles.11.1=128
moon
