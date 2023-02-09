rem remark and comment bug

findstr /c:"rem this" "file"
findstr /c:":: this" "file"

:: SingleQuoted command string
for /f %%A in ('rem this') do echo %%A

:: DoubleQuoted string
for /f %%A in ("rem this") do echo %%A

:: BackQuote command string
for /f "usebackq" %%A in (`rem this`) do echo %%A
