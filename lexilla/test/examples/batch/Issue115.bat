rem remark and comment bug

findstr /c:"rem this" "file"
findstr /c:":: this" "file"

:: SingleQuoted command string
for /f %%A in ('rem this') do echo %%A

:: DoubleQuoted string
for /f %%A in ("rem this") do echo %%A

:: BackQuote command string
for /f "usebackq" %%A in (`rem this`) do echo %%A

:: Test the handling of quotes ' and " and escape ^
:: Comment

:: With quotes
":: Text
"":: Comment
':: Text
'':: Comment
:: Mixing quotes - likely incorrect as lexer tries ' and " separately, leaving an active quote
"'":: Text

:: With escapes
^:: Text
^":: Comment
^"":: Text
^""":: Comment
^^":: Text
^^"":: Comment
^^""":: Text

:: With preceding command
mkdir archive ":: Text
mkdir archive "":: Comment
mkdir archive ^":: Comment
mkdir archive ^"":: Text
