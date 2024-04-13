rem Keywords with colon

rem with spacing
call file.bat arg1
call "file.bat" arg1
call :label arg1
goto :label
goto :eof
goto label
echo: %var%
echo: text
echo text

rem no spacing
call:label arg1
goto:label
goto:eof
echo:%var%
echo:text
(call)
(echo:)
(goto)

rem call internal commands
call echo text
call set "a=b"
