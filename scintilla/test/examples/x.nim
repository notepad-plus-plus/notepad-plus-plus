# Tests for Nim
let s = "foobar"

# Feature #1260
{.ident.}
stdin.readLine.split.map(parseInt).max.`$`.echo(" is the maximum!")

# Feature #1261
# IsFuncName("proc") so style ticks as SCE_NIM_FUNCNAME:
proc `$` (x: myDataType): string = ...
# Style ticks as SCE_NIM_BACKTICKS:
if `==`( `+`(3,4),7): echo "True"

# Feature #1262
# Standard raw string identifier:
let standardDoubleLitRawStr = R"A raw string\"
let standardTripleLitRawStr = R"""A triple-double raw string\""""
# Style of 'customIdent' is determined by lexer.nim.raw.strings.highlight.ident. 16 if false, 6 or 10 if true
let customDoubleLitRawStr = customIdent"A string\"
let customTripleLitRawStr = customIdent"""A triple-double raw string\""""

# Feature #1268
10_000
10__000
10_
1....5
1.ident
1._ident
